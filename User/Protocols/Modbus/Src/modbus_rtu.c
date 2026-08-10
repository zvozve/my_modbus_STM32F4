#include "modbus_rtu.h"

void modbus_rtu_init(modbus_rtu_ctx_t *ctx, uint32_t baudrate) {
    if (!ctx || baudrate == 0) return;
    ctx->baudrate = baudrate;
    // 1字符时间 = 10位 / 波特率 (1起始+8数据+1停止)
    ctx->char_time_us = 1000000 / (baudrate / MODBUS_RTU_CHAR_BITS);
    // 3.5字符时间
    ctx->frame_timeout_us = ctx->char_time_us * 35 / 10;
    ctx->last_byte_tick = 0;
    ctx->rx_state = 0;
}

void modbus_rtu_reset(modbus_rtu_ctx_t *ctx) {
    if (!ctx) return;
    ctx->rx_state = 0;
    ctx->last_byte_tick = 0;
}

uint32_t modbus_rtu_get_char_time_us(modbus_rtu_ctx_t *ctx) {
    return ctx ? ctx->char_time_us : 0;
}

uint32_t modbus_rtu_get_frame_timeout_us(modbus_rtu_ctx_t *ctx) {
    return ctx ? ctx->frame_timeout_us : 0;
}

int modbus_rtu_check_frame(modbus_rtu_ctx_t *ctx, uint32_t now_us,
                           uint8_t *buf, uint16_t *len) {
    if (!ctx || !buf || !len) return 0;
    
    if (*len == 0) {
        ctx->rx_state = 0;
        ctx->last_byte_tick = now_us;
        return 0;
    }
    
    // 检查帧间隔
    uint32_t interval = now_us - ctx->last_byte_tick;
    ctx->last_byte_tick = now_us;
    
    if (ctx->rx_state == 0) {
        // 等待帧头，只要有数据就开始接收
        ctx->rx_state = 1;
        return 0;
    }
    
    // 接收中，检查是否超时（帧结束）
    if (interval > ctx->frame_timeout_us && *len >= 4) {
        // 完整帧，重置状态
        ctx->rx_state = 0;
        ctx->last_byte_tick = now_us;
        return 1;
    }
    
    // 缓冲区满
    if (*len >= 256) {
        ctx->rx_state = 0;
        *len = 0;
        return 0;
    }
    
    return 0;
}