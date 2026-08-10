#include "task_modbus_m.h"
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
// static uint8_t g_coil_data1[16];

static void on_reg_change1(uint16_t addr, uint16_t old_val, uint16_t new_val) {
    SYS_LOG("[MASTER1] Reg %d: 0x%04X -> 0x%04X", addr, old_val, new_val);
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
// static uint8_t g_coil_data2[16];

static void on_reg_change2(uint16_t addr, uint16_t old_val, uint16_t new_val) {
    SYS_LOG("[MASTER2] Reg %d: 0x%04X -> 0x%04X", addr, old_val, new_val);
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

    modbus_master_config_t cfg1 = {
        .target_slave_addr = 1,
        .poll_interval_ms = 1000,
        .response_timeout_ms = 1000,
        .max_retries = 3,
        .reconnect_interval_ms = 10000,
        .reg_start_addr = 0,
        .reg_count = 10,
        .reg_buffer = g_reg_data1,
        .reg_buffer_size = sizeof(g_reg_data1),
        // .coil_start_addr = 0,
        // .coil_count = 16,
        // .coil_buffer = g_coil_data1,
        // .coil_buffer_size = sizeof(g_coil_data1),
    };
    modbus_master_init(&g_modbus_m1, &cfg1);
    modbus_master_set_reg_change_callback(&g_modbus_m1, on_reg_change1);
    modbus_master_set_coil_change_callback(&g_modbus_m1, NULL);
    modbus_uart_adapter_init(&g_modbus_m1, &g_uart_m1);

    // ---- 主机2 (UART2, 访问从机2) ----
    uart_drv_init(&g_uart_m2, &huart2, NULL);
    uart_drv_reg_cb(&g_uart_m2, NULL, NULL, NULL);
    uart_reconfig2(115200);

    modbus_master_config_t cfg2 = {
        .target_slave_addr = 2,
        .poll_interval_ms = 500,
        .response_timeout_ms = 1000,
        .max_retries = 3,
        .reconnect_interval_ms = 10000,
        .reg_start_addr = 0,
        .reg_count = 10,
        .reg_buffer = g_reg_data2,
        .reg_buffer_size = sizeof(g_reg_data2),
        // 主机2 不读线圈
        // .coil_start_addr = 0,
        // .coil_count = 16,
        // .coil_buffer = g_coil_data2,
        // .coil_buffer_size = sizeof(g_coil_data2),
    };
    modbus_master_init(&g_modbus_m2, &cfg2);
    modbus_master_set_reg_change_callback(&g_modbus_m2, on_reg_change2);
    modbus_master_set_coil_change_callback(&g_modbus_m2, NULL);
    modbus_uart_adapter_init(&g_modbus_m2, &g_uart_m2);

    SYS_LOG("[MASTER] Two masters initialized: UART1(addr1), UART2(addr2)");
}

// ===========================
// 处理
// ===========================
void TaskModbus_M_Process(void) {
    modbus_process(&g_modbus_m1);
    modbus_process(&g_modbus_m2);
}