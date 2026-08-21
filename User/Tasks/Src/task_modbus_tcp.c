#include "task_modbus_tcp.h"
#include "modbus_core.h"
#include "modbus_master.h"
#include "modbus_tcp.h"
#include "SEGGER_RTT_Log.h"
#include "tim.h"  // 使用硬件定时器
#include <math.h>
#include <string.h>

// ===========================
// 配置
// ===========================
#define WAVE_START_ADDR     0
#define WAVE_LENGTH         100
#define TRIGGER_ADDR        100
#define TIMESTAMP_ADDR      101
#define WAVE_OFFSET         1650
#define WAVE_AMPLITUDE      1650
#define UPDATE_INTERVAL_US  10000  // 10ms = 100 FPS

// 可选择：使用 TICK 模式（DWT）或 TIM 模式
#define USE_TIM_MODE        1      // 1: 使用硬件定时器, 0: 使用 DWT

// ===========================
// 数据
// ===========================
static modbus_t          g_modbus_tcp_srv;
static modbus_tcp_ctx_t  g_tcp_srv_ctx;
static uint16_t          g_tcp_holding[125];
static uint8_t           g_tcp_coils[16];

static uint16_t g_wave_data[WAVE_LENGTH];
static uint16_t g_sin_table[WAVE_LENGTH];
static uint32_t g_last_update_time = 0;
static uint32_t g_phase = 0;
static uint32_t g_frame_counter = 0;
static uint8_t  g_trigger_state = 0;

// ===========================
// 时间获取（支持 DWT 和 TIM 两种模式）
// ===========================
#if USE_TIM_MODE
// 使用 TIM2 作为微秒计时器（需要提前初始化）
static inline uint32_t get_us_timestamp(void)
{
    return __HAL_TIM_GET_COUNTER(&htim2);
}
#else
#include "bsp_dwt.h"
static inline uint32_t get_us_timestamp(void)
{
    return bsp_GetCycleCount() / (SystemCoreClock / 1000000);
}
#endif

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
void TaskModbus_TCP_Init(void) {
    generate_sin_table();
    memcpy(g_wave_data, g_sin_table, WAVE_LENGTH * sizeof(uint16_t));
    
    // Modbus 从机
    modbus_init(&g_modbus_tcp_srv);
    modbus_set_slave_addr(&g_modbus_tcp_srv, 1);
    g_modbus_tcp_srv.data_map.holding_regs = g_tcp_holding;
    g_modbus_tcp_srv.data_map.holding_size = 125;
    g_modbus_tcp_srv.data_map.coils = g_tcp_coils;
    g_modbus_tcp_srv.data_map.coils_size = 16;
    
    memset(g_tcp_holding, 0, sizeof(g_tcp_holding));
    memset(g_tcp_coils, 0, sizeof(g_tcp_coils));
    
    // 初始化寄存器
    g_frame_counter = 1;
    memcpy(&g_tcp_holding[WAVE_START_ADDR], g_wave_data, WAVE_LENGTH * sizeof(uint16_t));
    g_tcp_holding[TIMESTAMP_ADDR] = (uint16_t)(g_frame_counter >> 16);
    g_tcp_holding[TIMESTAMP_ADDR + 1] = (uint16_t)(g_frame_counter & 0xFFFF);
    g_tcp_holding[TRIGGER_ADDR] = 0;
    
    if (modbus_tcp_server_init(&g_modbus_tcp_srv, &g_tcp_srv_ctx,
                               MODBUS_TCP_DEFAULT_PORT) == 0) {
        SYS_LOG("[TCP] Modbus server ready");
    }
    
    g_last_update_time = get_us_timestamp();
}

// ===========================
// 处理（合并所有操作）
// ===========================
void TaskModbus_TCP_Process(void) {
    modbus_process(&g_modbus_tcp_srv);
    
    uint32_t now = get_us_timestamp();
    uint32_t elapsed = now - g_last_update_time;
    
    // 使用 >= 而不是每次固定间隔，防止累积误差
    if (elapsed >= UPDATE_INTERVAL_US) {
        g_last_update_time = now;
        
        // 波形左移
        g_phase++;
        if (g_phase >= WAVE_LENGTH) g_phase = 0;
        memmove(&g_wave_data[0], &g_wave_data[1], (WAVE_LENGTH - 1) * sizeof(uint16_t));
        g_wave_data[WAVE_LENGTH - 1] = g_sin_table[g_phase];
        
        // 更新寄存器
        g_frame_counter++;
        memcpy(&g_tcp_holding[WAVE_START_ADDR], g_wave_data, WAVE_LENGTH * sizeof(uint16_t));
        g_tcp_holding[TIMESTAMP_ADDR] = (uint16_t)(g_frame_counter >> 16);
        g_tcp_holding[TIMESTAMP_ADDR + 1] = (uint16_t)(g_frame_counter & 0xFFFF);
        g_trigger_state = !g_trigger_state;
        g_tcp_holding[TRIGGER_ADDR] = g_trigger_state ? 1 : 0;
    }
}