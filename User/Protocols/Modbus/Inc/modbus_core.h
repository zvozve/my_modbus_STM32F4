#ifndef __MODBUS_CORE_H__
#define __MODBUS_CORE_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MB_RTU_DRVIER_VERSION "5.0.1"
#define MB_RTU_DRVIER_DATE "2026-08-14"

// ===========================
// 系统配置
// ===========================
#define MB_USE_RTOS       0    // 0: 裸机, 1: RTOS

// ===========================
// 时间相关宏
// ===========================
#if MB_USE_RTOS
    #include "FreeRTOS.h"
    #include "task.h"
    #define MB_GET_TICK()      xTaskGetTickCount()
    #define MB_Delay_ms(ms)    vTaskDelay(pdMS_TO_TICKS(ms))
#else
    #include "stm32f4xx_hal.h"
    #define MB_GET_TICK()      HAL_GetTick()
    #define MB_Delay_ms(ms)    HAL_Delay(ms)
#endif

// ===========================
// 配置
// ===========================
#define MODBUS_MAX_INSTANCES    4
#define MODBUS_RTU_BUF_SIZE     256
#define MODBUS_BROADCAST_ADDR   0

// ===========================
// 功能码
// ===========================
typedef enum {
    MODBUS_FC_READ_COILS           = 0x01,
    MODBUS_FC_READ_DISCRETE_INPUTS = 0x02,
    MODBUS_FC_READ_HOLDING_REGS    = 0x03,
    MODBUS_FC_READ_INPUT_REGS      = 0x04,
    MODBUS_FC_WRITE_SINGLE_COIL    = 0x05,
    MODBUS_FC_WRITE_SINGLE_REG     = 0x06,
    MODBUS_FC_WRITE_MULTIPLE_COILS = 0x0F,
    MODBUS_FC_WRITE_MULTIPLE_REGS  = 0x10,
    MODBUS_FC_MASK_WRITE_REG       = 0x16,
    MODBUS_FC_READ_WRITE_MULT_REGS = 0x17,
} modbus_func_code_t;

// ===========================
// 异常码
// ===========================
typedef enum {
    MODBUS_EXCEPTION_ILLEGAL_FUNCTION   = 0x01,
    MODBUS_EXCEPTION_ILLEGAL_DATA_ADDR  = 0x02,
    MODBUS_EXCEPTION_ILLEGAL_DATA_VALUE = 0x03,
    MODBUS_EXCEPTION_SLAVE_DEVICE_FAIL  = 0x04,
    MODBUS_EXCEPTION_ACKNOWLEDGE        = 0x05,
    MODBUS_EXCEPTION_SLAVE_DEVICE_BUSY  = 0x06,
    MODBUS_EXCEPTION_MEMORY_PARITY_ERR  = 0x08,
    MODBUS_EXCEPTION_GATEWAY_PATH_ERR   = 0x0A,
    MODBUS_EXCEPTION_GATEWAY_TARGET_ERR = 0x0B,
} modbus_exception_t;

// ===========================
// 模式
// ===========================
typedef enum {
    MODBUS_MODE_RTU,
    MODBUS_MODE_ASCII,
} modbus_mode_t;

// ===========================
// 角色
// ===========================
typedef enum {
    MODBUS_ROLE_MASTER,
    MODBUS_ROLE_SLAVE,
} modbus_role_t;

// ===========================
// 断线状态
// ===========================
typedef enum {
    MODBUS_LINE_OK = 0,
    MODBUS_LINE_TIMEOUT,
    MODBUS_LINE_DISCONNECTED,
} modbus_line_state_t;

// ===========================
// 实例状态
// ===========================
typedef enum {
    MODBUS_STATE_IDLE,
    MODBUS_STATE_SENDING,
    MODBUS_STATE_WAITING_RESPONSE,
    MODBUS_STATE_RECEIVING,
    MODBUS_STATE_PROCESSING,
    MODBUS_STATE_ERROR,
} modbus_state_t;

// ===========================
// 传输接口
// ===========================
typedef struct {
    int (*send)(void *ctx, const uint8_t *data, uint16_t len);
    uint16_t (*peek)(void *ctx);
    uint16_t (*recv)(void *ctx, uint8_t *buf, uint16_t len);
    uint32_t (*get_tick)(void);
    void (*delay)(uint32_t ms);
    void *ctx;
} modbus_transport_t;

// ===========================
// Modbus实例（核心）
// ===========================
typedef struct modbus_instance {
    // 基本属性
    uint8_t slave_addr;
    modbus_role_t role;
    modbus_mode_t mode;
    modbus_state_t state;
    
    // 传输接口
    modbus_transport_t transport;
    
    // 帧缓冲区
    uint8_t tx_buf[MODBUS_RTU_BUF_SIZE];
    uint8_t rx_buf[MODBUS_RTU_BUF_SIZE];
    uint16_t tx_len;
    uint16_t rx_len;
    
    // 超时管理
    uint32_t send_tick;
    uint32_t response_timeout;
    uint32_t last_activity_tick;
    
    // 断线检测
    modbus_line_state_t line_state;
    uint8_t timeout_count;
    uint8_t max_timeout_count;
    uint32_t slave_timeout_ms;
    uint32_t reconnect_interval;
    uint32_t reconnect_tick;
    
    // 断线回调
    void (*on_line_break)(struct modbus_instance *ctx);
    void (*on_line_recover)(struct modbus_instance *ctx);
    
    // 从机数据映射
    struct {
        uint8_t *coils;
        uint16_t coils_size;
        uint8_t *discrete_inputs;
        uint16_t discrete_size;
        uint16_t *holding_regs;
        uint16_t holding_size;
        uint16_t *input_regs;
        uint16_t input_size;
    } data_map;
    
    // 主机事务
    struct {
        uint8_t *req_data;
        uint16_t req_len;
        uint8_t *resp_data;
        uint16_t *resp_len;
        bool pending;
        bool completed;
        int result;
    } transaction;
    
    // 主机轮询
    void (*poll_callback)(struct modbus_instance *ctx);
    uint32_t poll_interval;
    uint32_t last_poll_tick;
    
    // 主机缓存（检测变化用）
    uint16_t *last_regs;
    uint8_t *last_coils;
    uint16_t last_regs_count;
    uint16_t last_coils_count;
    uint16_t last_regs_start_addr;
    uint16_t last_coils_start_addr;

    // 主机变化回调
    void (*on_master_reg_change)(uint16_t addr, uint16_t old_val, uint16_t new_val);
    void (*on_master_coil_change)(uint16_t addr, bool old_val, bool new_val);
    
    // 实例链
    struct modbus_instance *next;
} modbus_t;

// ===========================
// 核心API
// ===========================
void modbus_init(modbus_t *ctx);
void modbus_set_transport(modbus_t *ctx, const modbus_transport_t *transport);
void modbus_set_role(modbus_t *ctx, modbus_role_t role);
void modbus_set_slave_addr(modbus_t *ctx, uint8_t addr);
void modbus_set_timeouts(modbus_t *ctx, uint32_t response_timeout_ms, 
                         uint32_t slave_timeout_ms, uint8_t max_retries);
void modbus_set_reconnect_interval(modbus_t *ctx, uint32_t interval_ms);
void modbus_set_line_callbacks(modbus_t *ctx, 
    void (*on_break)(modbus_t *), void (*on_recover)(modbus_t *));

void modbus_process(modbus_t *ctx);
void modbus_process_all(void);

modbus_state_t modbus_get_state(modbus_t *ctx);
modbus_line_state_t modbus_get_line_state(modbus_t *ctx);
uint32_t modbus_get_last_activity(modbus_t *ctx);

void modbus_register_instance(modbus_t *ctx);
void modbus_unregister_instance(modbus_t *ctx);

uint16_t modbus_crc16(const uint8_t *data, uint16_t len);

#ifdef __cplusplus
}
#endif

#endif // __MODBUS_CORE_H__