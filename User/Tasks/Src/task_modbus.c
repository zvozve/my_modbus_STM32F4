// task_modbus.c
#include "task_modbus.h"
#include "bsp_uart_drv.h"
#include "modbus_core.h"
#include "modbus_slave.h"
#include "modbus_uart_adapter.h"
#include "SEGGER_RTT_Log.h"
#include "usart.h"

// ===========================
// 从机1 (UART1, 地址1)
// ===========================
static uart_drv_t g_uart1;
static modbus_t g_modbus1;
static uint8_t g_coils1[16];
static uint16_t g_holding_regs1[128];
static uint16_t g_counter1 = 0;
static uint32_t g_last_update1 = 0;

static void on_coil_change1(uint16_t addr, bool old_val, bool new_val) {
    SYS_LOG("[SLAVE1] Coil %d: %d -> %d", addr, old_val, new_val);
}

static void on_reg_change1(uint16_t addr, uint16_t old_val, uint16_t new_val) {
    SYS_LOG("[SLAVE1] Reg %d: 0x%04X -> 0x%04X", addr, old_val, new_val);
}

static void on_line_break1(modbus_t *ctx) {
    SYS_LOG("[SLAVE1] Line BREAK");
}

static void on_line_recover1(modbus_t *ctx) {
    SYS_LOG("[SLAVE1] Line RECOVER");
}

static void uart_reconfig1(uint32_t baudrate) {
    uart_drv_cfg_t cfg = {
        .baudrate = baudrate,
        .word_length = UART_WORDLENGTH_8B,
        .stop_bits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE
    };
    uart_drv_reconfig(&g_uart1, &cfg);
}

static void slave1_update(void) {
    uint32_t now = MB_GET_TICK();
    if (now - g_last_update1 >= 1000) {
        g_last_update1 = now;
        g_counter1++;
        modbus_slave_set_reg(&g_modbus1, 0, g_counter1);
        modbus_slave_set_coil(&g_modbus1, 0, g_counter1 % 2);
    }
}

// ===========================
// 从机2 (UART2, 地址2)
// ===========================
static uart_drv_t g_uart2;
static modbus_t g_modbus2;
static uint8_t g_coils2[16];
static uint16_t g_holding_regs2[128];
static uint16_t g_counter2 = 0;
static uint32_t g_last_update2 = 0;

static void on_coil_change2(uint16_t addr, bool old_val, bool new_val) {
    SYS_LOG("[SLAVE2] Coil %d: %d -> %d", addr, old_val, new_val);
}

static void on_reg_change2(uint16_t addr, uint16_t old_val, uint16_t new_val) {
    SYS_LOG("[SLAVE2] Reg %d: 0x%04X -> 0x%04X", addr, old_val, new_val);
}

static void on_line_break2(modbus_t *ctx) {
    SYS_LOG("[SLAVE2] Line BREAK");
}

static void on_line_recover2(modbus_t *ctx) {
    SYS_LOG("[SLAVE2] Line RECOVER");
}

static void uart_reconfig2(uint32_t baudrate) {
    uart_drv_cfg_t cfg = {
        .baudrate = baudrate,
        .word_length = UART_WORDLENGTH_8B,
        .stop_bits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE
    };
    uart_drv_reconfig(&g_uart2, &cfg);
}

static void slave2_update(void) {
    uint32_t now = MB_GET_TICK();
    if (now - g_last_update2 >= 2000) {
        g_last_update2 = now;
        g_counter2 += 2;
        modbus_slave_set_reg(&g_modbus2, 0, g_counter2);
    }
}

// ===========================
// 初始化
// ===========================
void TaskModbus_Init(void) {
    // ---- 从机1 (UART1) ----
    uart_drv_init(&g_uart1, &huart1, NULL);
    uart_drv_reg_cb(&g_uart1, NULL, NULL, NULL);
    uart_reconfig1(115200);

    modbus_slave_config_t cfg1 = {
        .slave_addr = 1,
        .slave_timeout_ms = 5000,
        .coils = g_coils1,
        .coil_count = sizeof(g_coils1) * 8,
        .holding_regs = g_holding_regs1,
        .holding_reg_count = sizeof(g_holding_regs1) / sizeof(uint16_t),
    };
    modbus_slave_init(&g_modbus1, &cfg1);

    modbus_slave_set_coil_change_callback(&g_modbus1, on_coil_change1);
    modbus_slave_set_reg_change_callback(&g_modbus1, on_reg_change1);
    modbus_set_line_callbacks(&g_modbus1, on_line_break1, on_line_recover1);

    // ★ 直接调用，内部自动分配 adapter_ctx ★
    modbus_uart_adapter_init(&g_modbus1, &g_uart1);

    // ---- 从机2 (UART2) ----
    uart_drv_init(&g_uart2, &huart2, NULL);
    uart_drv_reg_cb(&g_uart2, NULL, NULL, NULL);
    uart_reconfig2(115200);

    modbus_slave_config_t cfg2 = {
        .slave_addr = 2,
        .slave_timeout_ms = 5000,
        .coils = g_coils2,
        .coil_count = sizeof(g_coils2) * 8,
        .holding_regs = g_holding_regs2,
        .holding_reg_count = sizeof(g_holding_regs2) / sizeof(uint16_t),
    };
    modbus_slave_init(&g_modbus2, &cfg2);

    modbus_slave_set_coil_change_callback(&g_modbus2, on_coil_change2);
    modbus_slave_set_reg_change_callback(&g_modbus2, on_reg_change2);
    modbus_set_line_callbacks(&g_modbus2, on_line_break2, on_line_recover2);

    // ★ 第二次调用，内部自动分配下一个 adapter_ctx ★
    modbus_uart_adapter_init(&g_modbus2, &g_uart2);

    SYS_LOG("[TASK] Two slaves initialized: UART1(addr1), UART2(addr2)");
}

// ===========================
// 处理
// ===========================
void TaskModbus_Process(void) {
    modbus_process(&g_modbus1);
    slave1_update();

    modbus_process(&g_modbus2);
    slave2_update();
}