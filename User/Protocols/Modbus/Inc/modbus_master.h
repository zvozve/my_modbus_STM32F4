#ifndef __MODBUS_MASTER_H__
#define __MODBUS_MASTER_H__

#include "modbus_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t target_slave_addr;
    uint32_t poll_interval_ms;
    uint32_t response_timeout_ms;
    uint8_t max_retries;
    uint32_t reconnect_interval_ms;
    
    // 轮询点位
    uint16_t reg_start_addr;
    uint16_t reg_count;
    uint16_t *reg_buffer;
    uint16_t reg_buffer_size;
    
    uint16_t coil_start_addr;
    uint16_t coil_count;
    uint8_t *coil_buffer;
    uint16_t coil_buffer_size;
} modbus_master_config_t;

void modbus_master_init(modbus_t *ctx, const modbus_master_config_t *cfg);

int modbus_master_read_coils(modbus_t *ctx, uint16_t addr, uint16_t count,
                              uint8_t *buf, uint16_t *len);
int modbus_master_read_holding_regs(modbus_t *ctx, uint16_t addr, uint16_t count,
                                     uint16_t *buf, uint16_t *len);
int modbus_master_write_single_reg(modbus_t *ctx, uint16_t addr, uint16_t value);
int modbus_master_write_single_coil(modbus_t *ctx, uint16_t addr, bool value);
int modbus_master_write_multiple_regs(modbus_t *ctx, uint16_t addr,
                                       uint16_t count, const uint16_t *values);

void modbus_master_set_reg_change_callback(modbus_t *ctx,
    void (*callback)(uint16_t addr, uint16_t old_val, uint16_t new_val));
void modbus_master_set_coil_change_callback(modbus_t *ctx,
    void (*callback)(uint16_t addr, bool old_val, bool new_val));

void modbus_master_set_poll_interval(modbus_t *ctx, uint32_t interval_ms);
void modbus_master_trigger_poll(modbus_t *ctx);

#ifdef __cplusplus
}
#endif

#endif // __MODBUS_MASTER_H__