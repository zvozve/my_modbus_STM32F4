#include "task_modbus_tcp.h"
#include "modbus_core.h"
#include "modbus_master.h"
#include "modbus_tcp.h"
#include "SEGGER_RTT_Log.h"
#include "bsp_dwt.h"
#include <math.h>
#include <string.h>

// ===========================
// 配置 - 只需要改这几个宏
// ===========================
#define WAVE_START_ADDR     0
#define WAVE_LENGTH         300     // ← 改成 300
#define TRIGGER_ADDR        300     // ← 改成 300
#define TIMESTAMP_ADDR      301     // ← 改成 301
#define WAVE_OFFSET         1650
#define WAVE_AMPLITUDE      1650
#define WAVE_MAX            3300
#define WAVE_MIN            0
#define UPDATE_INTERVAL_US  10000

// ===========================
// 数据 - 只需要改数组大小
// ===========================
static modbus_t          g_modbus_tcp_srv;
static modbus_tcp_server_t g_tcp_srv;
static uint16_t          g_tcp_holding[350];  // ← 125 → 350
static uint8_t           g_tcp_coils[16];

static uint16_t g_wave_data[WAVE_LENGTH];
static uint16_t g_sin_table[WAVE_LENGTH];
static uint32_t g_last_update_time = 0;
static uint32_t g_phase = 0;
static uint32_t g_frame_counter = 0;
static uint8_t  g_trigger_state = 0;

// ===========================
// 时间获取（使用 DWT）
// ===========================
static inline uint32_t get_us_timestamp(void)
{
    return bsp_GetCycleCount() / (SystemCoreClock / 1000000);
}

// ===========================
// 正弦查找表生成
// ===========================
static void generate_sin_table(void)
{
    for (uint32_t i = 0; i < WAVE_LENGTH; i++) {
        float angle = (2.0f * 3.14159265f * i) / WAVE_LENGTH;
        float sin_val = sinf(angle);
        int32_t value = (int32_t)(WAVE_OFFSET + WAVE_AMPLITUDE * sin_val);
        if (value > WAVE_MAX) value = WAVE_MAX;
        if (value < WAVE_MIN) value = WAVE_MIN;
        g_sin_table[i] = (uint16_t)value;
    }
}

// ===========================
// 初始化
// ===========================
static void tcp_on_client_connect(int id, const char *ip) {
    SYS_LOG("[TCP] client %d connected, ip=%s", id, ip ? ip : "?");
}

static void tcp_on_client_disconnect(int id, const char *ip) {
    SYS_LOG("[TCP] client %d disconnected, ip=%s", id, ip ? ip : "?");
}

void TaskModbus_TCP_Init(void) {
    generate_sin_table();
    memcpy(g_wave_data, g_sin_table, WAVE_LENGTH * sizeof(uint16_t));
    
    modbus_init(&g_modbus_tcp_srv);
    modbus_set_slave_addr(&g_modbus_tcp_srv, 1);
    g_modbus_tcp_srv.data_map.holding_regs = g_tcp_holding;
    g_modbus_tcp_srv.data_map.holding_size = 350;  // ← 改成 350
    g_modbus_tcp_srv.data_map.coils = g_tcp_coils;
    g_modbus_tcp_srv.data_map.coils_size = 16;
    
    memset(g_tcp_holding, 0, sizeof(g_tcp_holding));
    memset(g_tcp_coils, 0, sizeof(g_tcp_coils));
    
    g_frame_counter = 1;
    memcpy(&g_tcp_holding[WAVE_START_ADDR], g_wave_data, WAVE_LENGTH * sizeof(uint16_t));
    g_tcp_holding[TIMESTAMP_ADDR] = (uint16_t)(g_frame_counter >> 16);
    g_tcp_holding[TIMESTAMP_ADDR + 1] = (uint16_t)(g_frame_counter & 0xFFFF);
    g_tcp_holding[TRIGGER_ADDR] = 0;
    
    /* 模板仅提供 data_map/addr，不进全局实例表（避免被 modbus_process_all 重复处理；
     * 各客户端 slot 由 modbus_tcp_server_process 统一驱动，且不注册进实例表）。 */
    modbus_unregister_instance(&g_modbus_tcp_srv);

    g_tcp_srv.on_client_connect    = tcp_on_client_connect;
    g_tcp_srv.on_client_disconnect = tcp_on_client_disconnect;
    if (modbus_tcp_server_init(&g_modbus_tcp_srv, &g_tcp_srv,
                               MODBUS_TCP_DEFAULT_PORT) == 0) {
        SYS_LOG("[TCP] Modbus multi-client server ready, WAVE_LENGTH=%d", WAVE_LENGTH);
    }
    
    g_last_update_time = get_us_timestamp();
}

// ===========================
// 处理
// ===========================
void TaskModbus_TCP_Process(void) {
    modbus_tcp_server_process(&g_tcp_srv);
    
    uint32_t now = get_us_timestamp();
    uint32_t elapsed = now - g_last_update_time;
    
    if (elapsed >= UPDATE_INTERVAL_US) {
        g_last_update_time = now;
        
        g_phase++;
        if (g_phase >= WAVE_LENGTH) g_phase = 0;
        memmove(&g_wave_data[0], &g_wave_data[1], (WAVE_LENGTH - 1) * sizeof(uint16_t));
        g_wave_data[WAVE_LENGTH - 1] = g_sin_table[g_phase];
        
        g_frame_counter++;
        memcpy(&g_tcp_holding[WAVE_START_ADDR], g_wave_data, WAVE_LENGTH * sizeof(uint16_t));
        g_tcp_holding[TIMESTAMP_ADDR] = (uint16_t)(g_frame_counter >> 16);
        g_tcp_holding[TIMESTAMP_ADDR + 1] = (uint16_t)(g_frame_counter & 0xFFFF);
        g_trigger_state = !g_trigger_state;
        g_tcp_holding[TRIGGER_ADDR] = g_trigger_state ? 1 : 0;
    }
}