#ifndef __MODBUS_RTU_H__
#define __MODBUS_RTU_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MODBUS_RTU_CHAR_BITS    10  // 1起始位 + 8数据位 + 1停止位

typedef struct {
    uint32_t baudrate;
    uint32_t char_time_us;
    uint32_t frame_timeout_us;
    uint32_t last_byte_tick;
    uint8_t rx_state;  // 0=等待帧头, 1=接收中
} modbus_rtu_ctx_t;

void modbus_rtu_init(modbus_rtu_ctx_t *ctx, uint32_t baudrate);
void modbus_rtu_reset(modbus_rtu_ctx_t *ctx);
uint32_t modbus_rtu_get_char_time_us(modbus_rtu_ctx_t *ctx);
uint32_t modbus_rtu_get_frame_timeout_us(modbus_rtu_ctx_t *ctx);
int modbus_rtu_check_frame(modbus_rtu_ctx_t *ctx, uint32_t now_us,
                           uint8_t *buf, uint16_t *len);

#ifdef __cplusplus
}
#endif

#endif // __MODBUS_RTU_H__