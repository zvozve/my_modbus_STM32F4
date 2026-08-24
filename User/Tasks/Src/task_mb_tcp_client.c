#include "task_mb_tcp_client.h"
#include "modbus_core.h"
#include "modbus_master.h"
#include "modbus_tcp.h"
#include "SEGGER_RTT_Log.h"

/* 每个 client 实例的目标 + 测试行为（改成你 PC 上 Modbus TCP 从机的地址）。
 * 测试多实例时通常连不同从机；也可连同一从机模拟“多主站”并发。
 * 下面把两个实例做成“明显不一样”，方便从日志一眼区分谁是谁：
 *   实例0：连 .200，快轮询(200ms) 只读 holding，每 2s 写 reg0
 *   实例1：连 .101，慢轮询(1s)   只读 coils，每 5s 写 reg10 */
typedef struct {
    const char *ip;
    uint16_t    port;
    uint8_t     unit;
    uint16_t    poll_ms;           /* 周期读间隔 */
    uint16_t    reg_start;         /* holding 读起始（reg_len=0 则不读） */
    uint16_t    reg_len;
    uint16_t    coil_start;        /* coil 读起始（coil_len=0 则不读） */
    uint16_t    coil_len;
    uint16_t    write_addr;        /* 写测试目标寄存器 */
    uint16_t    write_interval_ms; /* 写测试间隔 */
} cli_cfg_t;

static const cli_cfg_t g_cli_cfg[MB_TCP_CLIENT_COUNT] = {
    {   /* 实例0：快、读 holding、写 reg0 */
        .ip = "192.168.31.200", .port = MODBUS_TCP_DEFAULT_PORT, .unit = 1,
        .poll_ms = 200,
        .reg_start = 0, .reg_len = 10,
        .coil_start = 0, .coil_len = 0,
        .write_addr = 0, .write_interval_ms = 2000,
    },
    {   /* 实例1：慢、读 coils、写 reg10 */
        .ip = "192.168.31.101", .port = MODBUS_TCP_DEFAULT_PORT, .unit = 1,
        .poll_ms = 1000,
        .reg_start = 0, .reg_len = 0,
        .coil_start = 0, .coil_len = 16,
        .write_addr = 10, .write_interval_ms = 5000,
    },
    /* 需要更多实例，在此继续追加（并调大 MB_TCP_CLIENT_COUNT） */
};

/* ---- 实例与缓冲（数组，互不共享） ---- */
static modbus_t          g_mb[MB_TCP_CLIENT_COUNT];
static modbus_tcp_ctx_t  g_ctx[MB_TCP_CLIENT_COUNT];
static uint16_t          g_reg_shadow[MB_TCP_CLIENT_COUNT][64];
static uint8_t           g_coil_shadow[MB_TCP_CLIENT_COUNT][16];
static uint16_t          g_write_cnt[MB_TCP_CLIENT_COUNT];

/* 由 modbus_t* 反推实例下标（g_mb 是连续数组，指针相减即下标） */
static int cli_idx(modbus_t *mb) { return (int)(mb - &g_mb[0]); }

/* ---- 断线/恢复通知（验证 TCP client 的 line_break / line_recover） ---- */
static void on_line_break(modbus_t *mb) {
    int i = cli_idx(mb);
    SYS_LOG("[TCP-CLI%d] line BREAK: peer closed / connect failed", i);
}
static void on_line_recover(modbus_t *mb) {
    int i = cli_idx(mb);
    SYS_LOG("[TCP-CLI%d] line RECOVER: reconnected to %s:%u", i,
            g_cli_cfg[i].ip, g_cli_cfg[i].port);
}

/* ---- 变化回调：对端寄存器/线圈被别的客户端改了，这里能看到 ---- */
static int s_cur_idx = -1;   /* 单线程顺序处理，modbus_process 前由 Process 循环置位 */
static void on_reg_change(uint16_t addr, uint16_t old_val, uint16_t new_val) {
    SYS_LOG("[TCP-CLI%d] Reg %d: 0x%04X -> 0x%04X", s_cur_idx, addr, old_val, new_val);
}
static void on_coil_change(uint16_t addr, bool old_val, bool new_val) {
    SYS_LOG("[TCP-CLI%d] Coil %d: %d -> %d", s_cur_idx, addr, old_val, new_val);
}

/* ---- 一次性写测试完成回调 ---- */
static void on_write_done(int result) {
    SYS_LOG("[TCP-CLI%d] write test result=%d", s_cur_idx, result);
}

void TaskModbus_TCP_Client_Init(void) {
    static modbus_master_config_t cfg[MB_TCP_CLIENT_COUNT];  /* 必须 static：master 长期持有其指针 */
    for (int i = 0; i < MB_TCP_CLIENT_COUNT; i++) {
        /* 1. 仲裁器（与 RTU 主机一致）：建 master 角色 + 调度队列，参数由 g_cli_cfg[i] 驱动 */
        cfg[i] = (modbus_master_config_t){
            .target_slave_addr   = g_cli_cfg[i].unit,
            .poll_interval_ms    = g_cli_cfg[i].poll_ms,
            .min_frame_gap_ms    = 50,     /* TCP 快，帧间隔放宽到 50ms */
            .response_timeout_ms = 1000,
            .max_retries         = 3,
            .reconnect_interval_ms = 2000, /* 与 port_poll 的 2s 重连节拍一致 */
        };
        modbus_master_init(&g_mb[i], &cfg[i]);
        modbus_master_set_reg_change_callback(&g_mb[i], on_reg_change);
        modbus_master_set_coil_change_callback(&g_mb[i], on_coil_change);

        /* 2. TCP 传输（client 模式：port_poll 负责 connect/2s 重连） */
        modbus_tcp_client_init(&g_mb[i], &g_ctx[i],
                               g_cli_cfg[i].ip, g_cli_cfg[i].port);

        /* 3. 断线通知 */
        modbus_set_line_callbacks(&g_mb[i], on_line_break, on_line_recover);

        /* 4. 周期轮询：按本实例配置添加寄存器段 / 线圈段（len=0 则不添加） */
        if (g_cli_cfg[i].reg_len)
            modbus_master_add_reg_range(&g_mb[i], g_cli_cfg[i].reg_start,
                                        g_cli_cfg[i].reg_len, g_reg_shadow[i], 64,
                                        g_cli_cfg[i].poll_ms, 0);
        if (g_cli_cfg[i].coil_len)
            modbus_master_add_coil_range(&g_mb[i], g_cli_cfg[i].coil_start,
                                         g_cli_cfg[i].coil_len, g_coil_shadow[i], 16,
                                         g_cli_cfg[i].poll_ms, 100);

        SYS_LOG("[TCP-CLI%d] init: target=%s:%u unit=%u poll=%ums %s%s",
                i, g_cli_cfg[i].ip, g_cli_cfg[i].port, g_cli_cfg[i].unit,
                g_cli_cfg[i].poll_ms,
                g_cli_cfg[i].reg_len ? "reg " : "",
                g_cli_cfg[i].coil_len ? "coil" : "");
    }
    SYS_LOG("[TCP-CLI] %d client instance(s) registered", MB_TCP_CLIENT_COUNT);
}

modbus_t *TaskModbus_TCP_Client_Get(uint8_t idx) {
    return (idx < MB_TCP_CLIENT_COUNT) ? &g_mb[idx] : NULL;
}

void TaskModbus_TCP_Client_Process(void) {
    static uint32_t g_last_write[MB_TCP_CLIENT_COUNT];   /* 每实例独立的写测试节拍 */

    /* 逐实例驱动：port_poll（connect/重连/收包）+ 仲裁器调度（读轮询/写队列） */
    for (int i = 0; i < MB_TCP_CLIENT_COUNT; i++) {
        s_cur_idx = i;   /* 供 on_reg/coil_change / on_write_done 回调识别实例 */
        modbus_process(&g_mb[i]);

        /* 按本实例的写间隔，向 write_addr 写自增计数，验证该 client 的写 + 往返 */
        uint32_t now = xTaskGetTickCount();
        if ((now - g_last_write[i]) >= pdMS_TO_TICKS(g_cli_cfg[i].write_interval_ms)) {
            g_last_write[i] = now;
            modbus_master_write_reg_async(&g_mb[i], g_cli_cfg[i].write_addr,
                                          g_write_cnt[i]++, on_write_done);
        }
    }
}
