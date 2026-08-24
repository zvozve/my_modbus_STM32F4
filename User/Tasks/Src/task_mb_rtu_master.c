#include "task_mb_rtu_master.h"
#include "bsp_uart_drv.h"
#include "modbus_core.h"
#include "modbus_master.h"
#include "modbus_uart_adapter.h"
#include "SEGGER_RTT_Log.h"
#include "usart.h"

// ===========================
// 主机1 (UART1, 访问从机1)
// ===========================
static uart_drv_t g_uart_m1;
static modbus_t g_modbus_m1;
static uint16_t g_reg_data1[64];
static uint8_t g_coil_data1[16];

static void on_reg_change1(uint16_t addr, uint16_t old_val, uint16_t new_val) {
    SYS_LOG("[MASTER1] Reg %d: 0x%04X -> 0x%04X", addr, old_val, new_val);
}

static void on_coil_change1(uint16_t addr, bool old_val, bool new_val) {
    SYS_LOG("[MASTER1] Coil %d: %d -> %d", addr, old_val, new_val);
}

static void uart_reconfig1(uint32_t baudrate) {
    uart_drv_cfg_t cfg = {
        .baudrate = baudrate,
        .word_length = UART_WORDLENGTH_8B,
        .stop_bits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE
    };
    uart_drv_reconfig(&g_uart_m1, &cfg);
}

// ===========================
// 主机2 (UART2, 访问从机2)
// ===========================
static uart_drv_t g_uart_m2;
static modbus_t g_modbus_m2;
static uint16_t g_reg_data2[64];
static uint8_t g_coil_data2[16];

static void on_reg_change2(uint16_t addr, uint16_t old_val, uint16_t new_val) {
    SYS_LOG("[MASTER2] Reg %d: 0x%04X -> 0x%04X", addr, old_val, new_val);
}

static void on_coil_change2(uint16_t addr, bool old_val, bool new_val) {
    SYS_LOG("[MASTER2] Coil %d: %d -> %d", addr, old_val, new_val);
}

static void uart_reconfig2(uint32_t baudrate) {
    uart_drv_cfg_t cfg = {
        .baudrate = baudrate,
        .word_length = UART_WORDLENGTH_8B,
        .stop_bits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE
    };
    uart_drv_reconfig(&g_uart_m2, &cfg);
}

// ===========================
// 初始化
// ===========================
void TaskModbus_M_Init(void) {
    // ---- 主机1 (UART1, 访问从机1) ----
    uart_drv_init(&g_uart_m1, &huart1, NULL);
    uart_drv_reg_cb(&g_uart_m1, NULL, NULL, NULL);
    uart_reconfig1(115200);

    static modbus_master_config_t cfg1 = {
        .target_slave_addr = 1,
        .poll_interval_ms = 100,
        .min_frame_gap_ms = 100,
        .response_timeout_ms = 1000,
        .max_retries = 3,
        .reconnect_interval_ms = 2000,
    };
    modbus_master_init(&g_modbus_m1, &cfg1);
    modbus_master_set_reg_change_callback(&g_modbus_m1, on_reg_change1);
    modbus_master_set_coil_change_callback(&g_modbus_m1, on_coil_change1);
    modbus_uart_adapter_init(&g_modbus_m1, &g_uart_m1);

    /* 多段轮询注册：寄存器段 + 线圈段 */
    modbus_master_add_reg_range(&g_modbus_m1, 0, 10, g_reg_data1, 64, 200, 0);
    modbus_master_add_coil_range(&g_modbus_m1, 0, 16, g_coil_data1, 16, 200, 100);

    // ---- 主机2 (UART2, 访问从机2) ----
    uart_drv_init(&g_uart_m2, &huart2, NULL);
    uart_drv_reg_cb(&g_uart_m2, NULL, NULL, NULL);
    uart_reconfig2(115200);

    static modbus_master_config_t cfg2 = {
        .target_slave_addr = 2,
        .poll_interval_ms = 100,
        .min_frame_gap_ms = 100,
        .response_timeout_ms = 1000,
        .max_retries = 3,
        .reconnect_interval_ms = 2000,
    };
    modbus_master_init(&g_modbus_m2, &cfg2);
    modbus_master_set_reg_change_callback(&g_modbus_m2, on_reg_change2);
    modbus_master_set_coil_change_callback(&g_modbus_m2, on_coil_change2);
    modbus_uart_adapter_init(&g_modbus_m2, &g_uart_m2);

    modbus_master_add_reg_range(&g_modbus_m2, 0, 10, g_reg_data2, 64, 500, 0);
    modbus_master_add_coil_range(&g_modbus_m2, 0, 16, g_coil_data2, 16, 500, 250);

    SYS_LOG("[MASTER] Two masters initialized: UART1(addr1), UART2(addr2)");
}

// ===========================
// 处理
// ===========================
modbus_t* TaskModbus_M_GetMaster1(void) { return &g_modbus_m1; }
modbus_t* TaskModbus_M_GetMaster2(void) { return &g_modbus_m2; }

void TaskModbus_M_Process(void) {
    modbus_process(&g_modbus_m1);
    modbus_process(&g_modbus_m2);
}