#include "modbus_master.h"
#include "SEGGER_RTT_Log.h"
#include <string.h>

// ===========================
// 内部：主机请求
// ===========================
// master_request_async 改成非阻塞
static int master_request_async(modbus_t *ctx, uint8_t func_code,
                                    const uint8_t *req_data, uint16_t req_len,
                                    uint8_t *resp_buf, uint16_t *resp_len,
                                    uint32_t timeout_ms) {
    if (!ctx || ctx->role != MODBUS_ROLE_MASTER) return -1;
    if (ctx->state == MODBUS_STATE_WAITING_RESPONSE) return -2;
    if (ctx->line_state == MODBUS_LINE_DISCONNECTED) return -3;
    
    ctx->tx_buf[0] = ctx->slave_addr;
    ctx->tx_buf[1] = func_code;
    if (req_data && req_len > 0) {
        memcpy(ctx->tx_buf + 2, req_data, req_len);
    }
    uint16_t len = 2 + req_len;
    uint16_t crc = modbus_crc16(ctx->tx_buf, len);
    ctx->tx_buf[len] = crc & 0xFF;
    ctx->tx_buf[len + 1] = (crc >> 8) & 0xFF;
    ctx->tx_len = len + 2;
    
    MODBUS_LOG("Master send: func=0x%02X", func_code);
    HEX_LOG("TX: ", ctx->tx_buf, ctx->tx_len);
    
    ctx->transaction.req_data = ctx->tx_buf;
    ctx->transaction.req_len = ctx->tx_len;
    ctx->transaction.resp_data = resp_buf;
    ctx->transaction.resp_len = resp_len;
    ctx->transaction.pending = true;
    ctx->transaction.completed = false;
    ctx->transaction.result = -1;
    *resp_len = 0;
    
    int ret = ctx->transport.send(ctx->transport.ctx, ctx->tx_buf, ctx->tx_len);
    if (ret != 0) {
        ctx->transaction.pending = false;
        return ret;
    }
    
    ctx->state = MODBUS_STATE_WAITING_RESPONSE;
    SYS_LOG("[MASTER] state set to WAITING_RESPONSE");
    ctx->send_tick = ctx->transport.get_tick();
    ctx->timeout_count = 0;
    
    // ★★★ 不阻塞，直接返回，由 modbus_process 处理超时 ★★★
    return 0;
}

// ===========================
// 内部：轮询状态机
// ===========================
typedef enum {
    POLL_STATE_REG = 0,
    POLL_STATE_COIL,
    POLL_STATE_DONE
} poll_state_t;

static void master_poll_task(modbus_t *ctx) {
    if (!ctx) return;
    if (ctx->state == MODBUS_STATE_WAITING_RESPONSE) return;
    
    static poll_state_t poll_state = POLL_STATE_REG;
    int ret;
    
    switch (poll_state) {
        case POLL_STATE_REG:
            if (ctx->data_map.holding_regs && ctx->last_regs && ctx->last_regs_count > 0) {
                uint8_t req_data[4];
                req_data[0] = (ctx->last_regs_start_addr >> 8) & 0xFF;
                req_data[1] = ctx->last_regs_start_addr & 0xFF;
                req_data[2] = (ctx->last_regs_count >> 8) & 0xFF;
                req_data[3] = ctx->last_regs_count & 0xFF;
                
                uint16_t len = ctx->last_regs_count * 2;
                ret = master_request_async(ctx, MODBUS_FC_READ_HOLDING_REGS,
                                              req_data, 4, (uint8_t*)ctx->data_map.holding_regs, &len,
                                              ctx->response_timeout);
                if (ret == 0 && ctx->on_master_reg_change) {
                    for (uint16_t i = 0; i < ctx->last_regs_count; i++) {
                        if (ctx->data_map.holding_regs[i] != ctx->last_regs[i]) {
                            ctx->on_master_reg_change(i, ctx->last_regs[i], 
                                                       ctx->data_map.holding_regs[i]);
                            ctx->last_regs[i] = ctx->data_map.holding_regs[i];
                        }
                    }
                }
            }
            // ★★★ 只有配置了线圈才切换到 COIL ★★★
            if (ctx->data_map.coils && ctx->last_coils && ctx->last_coils_count > 0) {
                poll_state = POLL_STATE_COIL;
            }
            // 否则保持 REG，下次继续读寄存器
            break;
            
        case POLL_STATE_COIL:
            if (ctx->data_map.coils && ctx->last_coils && ctx->last_coils_count > 0) {
                uint8_t req_data[4];
                req_data[0] = (ctx->last_coils_start_addr >> 8) & 0xFF;
                req_data[1] = ctx->last_coils_start_addr & 0xFF;
                req_data[2] = (ctx->last_coils_count >> 8) & 0xFF;
                req_data[3] = ctx->last_coils_count & 0xFF;
                
                uint16_t len = (ctx->last_coils_count + 7) / 8;
                ret = master_request_async(ctx, MODBUS_FC_READ_COILS,
                                              req_data, 4, ctx->data_map.coils, &len,
                                              ctx->response_timeout);
                if (ret == 0 && ctx->on_master_coil_change) {
                    for (uint16_t i = 0; i < ctx->last_coils_count; i++) {
                        uint8_t byte_idx = i / 8;
                        uint8_t bit_idx = i % 8;
                        bool new_val = (ctx->data_map.coils[byte_idx] >> bit_idx) & 0x01;
                        bool old_val = (ctx->last_coils[byte_idx] >> bit_idx) & 0x01;
                        if (new_val != old_val) {
                            ctx->on_master_coil_change(i, old_val, new_val);
                            if (new_val) {
                                ctx->last_coils[byte_idx] |= (1 << bit_idx);
                            } else {
                                ctx->last_coils[byte_idx] &= ~(1 << bit_idx);
                            }
                        }
                    }
                }
            }
            poll_state = POLL_STATE_REG;
            break;
            
        default:
            poll_state = POLL_STATE_REG;
            break;
    }
}

// ===========================
// 主机初始化
// ===========================
void modbus_master_init(modbus_t *ctx, const modbus_master_config_t *cfg) {
    if (!ctx || !cfg) return;
    
    modbus_init(ctx);
    modbus_set_role(ctx, MODBUS_ROLE_MASTER);
    modbus_set_slave_addr(ctx, cfg->target_slave_addr);
    modbus_set_timeouts(ctx, cfg->response_timeout_ms, 0, cfg->max_retries);
    modbus_set_reconnect_interval(ctx, cfg->reconnect_interval_ms);
    
    ctx->poll_interval = cfg->poll_interval_ms;
    MODBUS_LOG("poll_interval = %lu", ctx->poll_interval);
    ctx->data_map.holding_regs = cfg->reg_buffer;
    ctx->data_map.holding_size = cfg->reg_buffer_size / 2;
    ctx->data_map.coils = cfg->coil_buffer;
    ctx->data_map.coils_size = cfg->coil_buffer_size * 8;
    
    // ★★★ 分配缓存 ★★★
    if (cfg->reg_count > 0) {
        ctx->last_regs = malloc(cfg->reg_count * sizeof(uint16_t));
        ctx->last_regs_count = cfg->reg_count;
        ctx->last_regs_start_addr = cfg->reg_start_addr;
        memset(ctx->last_regs, 0, cfg->reg_count * sizeof(uint16_t));
    }
    if (cfg->coil_count > 0) {
        uint16_t byte_count = (cfg->coil_count + 7) / 8;
        ctx->last_coils = malloc(byte_count);
        ctx->last_coils_count = cfg->coil_count;
        ctx->last_coils_start_addr = cfg->coil_start_addr;
        memset(ctx->last_coils, 0, byte_count);
    }
    
    ctx->poll_callback = master_poll_task;
    MODBUS_LOG("Master initialized, target=%d, interval=%lu ms",
               cfg->target_slave_addr, cfg->poll_interval_ms);
}

// ===========================
// 主机手动API
// ===========================
int modbus_master_read_coils(modbus_t *ctx, uint16_t addr, uint16_t count,
                              uint8_t *buf, uint16_t *len) {
    if (!ctx || !buf || !len || count == 0) return -1;
    if (count > 2000) return -2;
    uint8_t req_data[4] = {(addr >> 8) & 0xFF, addr & 0xFF,
                           (count >> 8) & 0xFF, count & 0xFF};
    *len = 0;
    return master_request_async(ctx, MODBUS_FC_READ_COILS, req_data, 4, buf, len,
                                   ctx->response_timeout * 2);
}

int modbus_master_read_holding_regs(modbus_t *ctx, uint16_t addr, uint16_t count,
                                     uint16_t *buf, uint16_t *len) {
    if (!ctx || !buf || !len || count == 0) return -1;
    if (count > 125) return -2;
    uint8_t req_data[4] = {(addr >> 8) & 0xFF, addr & 0xFF,
                           (count >> 8) & 0xFF, count & 0xFF};
    *len = 0;
    return master_request_async(ctx, MODBUS_FC_READ_HOLDING_REGS, req_data, 4,
                                   (uint8_t*)buf, len, ctx->response_timeout * 2);
}

int modbus_master_write_single_reg(modbus_t *ctx, uint16_t addr, uint16_t value) {
    if (!ctx) return -1;
    uint8_t req_data[4] = {(addr >> 8) & 0xFF, addr & 0xFF,
                           (value >> 8) & 0xFF, value & 0xFF};
    uint8_t resp_buf[8];
    uint16_t resp_len = sizeof(resp_buf);
    int ret = master_request_async(ctx, MODBUS_FC_WRITE_SINGLE_REG, req_data, 4,
                                      resp_buf, &resp_len, ctx->response_timeout * 2);
    if (ret == 0 && ctx->last_regs && addr < ctx->last_regs_count) {
        ctx->last_regs[addr] = value;
    }
    return ret;
}

int modbus_master_write_single_coil(modbus_t *ctx, uint16_t addr, bool value) {
    if (!ctx) return -1;
    uint16_t val = value ? 0xFF00 : 0x0000;
    uint8_t req_data[4] = {(addr >> 8) & 0xFF, addr & 0xFF,
                           (val >> 8) & 0xFF, val & 0xFF};
    uint8_t resp_buf[8];
    uint16_t resp_len = sizeof(resp_buf);
    int ret = master_request_async(ctx, MODBUS_FC_WRITE_SINGLE_COIL, req_data, 4,
                                      resp_buf, &resp_len, ctx->response_timeout * 2);
    if (ret == 0 && ctx->last_coils && addr < ctx->last_coils_count) {
        uint8_t byte_idx = addr / 8;
        uint8_t bit_idx = addr % 8;
        if (value) {
            ctx->last_coils[byte_idx] |= (1 << bit_idx);
        } else {
            ctx->last_coils[byte_idx] &= ~(1 << bit_idx);
        }
    }
    return ret;
}

int modbus_master_write_multiple_regs(modbus_t *ctx, uint16_t addr,
                                       uint16_t count, const uint16_t *values) {
    if (!ctx || !values || count == 0) return -1;
    if (count > 123) return -2;
    uint8_t req_data[256];
    uint16_t idx = 0;
    req_data[idx++] = (addr >> 8) & 0xFF;
    req_data[idx++] = addr & 0xFF;
    req_data[idx++] = (count >> 8) & 0xFF;
    req_data[idx++] = count & 0xFF;
    req_data[idx++] = count * 2;
    for (uint16_t i = 0; i < count; i++) {
        req_data[idx++] = (values[i] >> 8) & 0xFF;
        req_data[idx++] = values[i] & 0xFF;
    }
    uint8_t resp_buf[8];
    uint16_t resp_len = sizeof(resp_buf);
    int ret = master_request_async(ctx, MODBUS_FC_WRITE_MULTIPLE_REGS, req_data, idx,
                                      resp_buf, &resp_len, ctx->response_timeout * 2);
    if (ret == 0 && ctx->last_regs) {
        for (uint16_t i = 0; i < count && (addr + i) < ctx->last_regs_count; i++) {
            ctx->last_regs[addr + i] = values[i];
        }
    }
    return ret;
}

void modbus_master_set_reg_change_callback(modbus_t *ctx,
    void (*callback)(uint16_t addr, uint16_t old_val, uint16_t new_val)) {
    if (ctx) ctx->on_master_reg_change = callback;
}

void modbus_master_set_coil_change_callback(modbus_t *ctx,
    void (*callback)(uint16_t addr, bool old_val, bool new_val)) {
    if (ctx) ctx->on_master_coil_change = callback;
}

void modbus_master_set_poll_interval(modbus_t *ctx, uint32_t interval_ms) {
    if (ctx) ctx->poll_interval = interval_ms;
}

void modbus_master_trigger_poll(modbus_t *ctx) {
    if (ctx && ctx->poll_callback && ctx->state == MODBUS_STATE_IDLE) {
        ctx->poll_callback(ctx);
    }
}