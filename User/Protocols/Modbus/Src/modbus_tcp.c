#include "modbus_core.h"    // 提供 MODBUS_ENABLE_TCP（默认 1，可被 CMake -D 覆盖）
#include "modbus_tcp.h"

#ifdef MODBUS_ENABLE_TCP

/* 诊断日志开关：1=打印每个 slot 的 recv/send 详情（定位多客户端哪个连接卡住），
 * 实测稳定后可改 0 关闭以减少刷屏。 */
#ifndef MODBUS_TCP_DIAG
#define MODBUS_TCP_DIAG 1
#endif

#include "lwip/api.h"       /* LwIP 2.1.x 的 netconn API 声明在 api.h（无 netconn.h） */
#include "lwip/ip_addr.h"
#include "lwip/netbuf.h"
#include "lwip/err.h"
#include "SEGGER_RTT_Log.h"
#include <string.h>

// ===========================
// MBAP 帧封装（纯字节操作，不依赖 LwIP 类型）
// ===========================

/* core 视角 PDU 恒为 [unit][func][data...]：
 *  - TX: pdu[0]=unit → MBAP 第 6 字节，pdu[1..]=func+data 顺延；
 *  - RX: 校验后把 MBAP 的 unit 放到 raw[0]，PDU 顺延，供 core 直接解析。
 * 返回：TX 输出长度 = pdu_len + 6（MBAP 7 字节中 unit 与 PDU 共用 1 字节） */
uint16_t tcp_frame_tx(void *ctx, const uint8_t *pdu, uint16_t pdu_len,
                      uint8_t *out, uint16_t out_cap) {
    modbus_tcp_ctx_t *t = (modbus_tcp_ctx_t*)ctx;
    if (!t || !pdu || !out || pdu_len < 2 || pdu_len > MODBUS_BUF_SIZE) return 0;
    if (pdu_len + 6 > out_cap) return 0;

    uint16_t tid = t->is_server ? t->last_rx_tid : t->next_tx_tid++;
    out[0] = (tid >> 8) & 0xFF;
    out[1] = tid & 0xFF;
    out[2] = 0x00;               /* protocol id 高 */
    out[3] = 0x00;               /* protocol id 低（Modbus 恒为 0） */
    out[4] = (pdu_len >> 8) & 0xFF;   /* length = unit(1) + PDU */
    out[5] = pdu_len & 0xFF;
    out[6] = pdu[0];             /* unit id */
    memcpy(out + 7, pdu + 1, pdu_len - 1);
    return pdu_len + 6;
}

int tcp_frame_rx(void *ctx, uint8_t *raw, uint16_t *raw_len) {
    modbus_tcp_ctx_t *t = (modbus_tcp_ctx_t*)ctx;
    if (!t || !raw || !raw_len || *raw_len < 8) return -1;

    uint16_t pid       = ((uint16_t)raw[2] << 8) | raw[3];
    uint16_t len_field = ((uint16_t)raw[4] << 8) | raw[5];
    if (pid != 0) return -1;
    if (len_field != *raw_len - 6) return -1;   /* length = unit(1) + PDU = 总长 - 6 */

    uint16_t rx_tid = ((uint16_t)raw[0] << 8) | raw[1];
    if (!t->is_server && rx_tid != t->next_tx_tid - 1) return -1;  /* 丢弃过期/错配响应 */
    t->last_rx_tid = rx_tid;                    /* server: 应答回显用 */

    /* 重排为 core 布局 [unit][func][data...] */
    raw[0] = raw[6];
    memmove(raw + 1, raw + 7, *raw_len - 7);
    *raw_len -= 6;
    return 0;
}

// ===========================
// transport 回调（netconn 字节管道 + 帧重组）
// ===========================

static int tcp_send(void *ctx, const uint8_t *data, uint16_t len) {
    modbus_tcp_ctx_t *t = (modbus_tcp_ctx_t*)ctx;
    if (!t->is_connected || !t->conn) {
        MODBUS_LOG("[TCP] TX drop: not connected, len=%u", len);
        return -1;
    }
    HEX_LOG("TCP-TX: ", data, len);   /* 实际交给 LwIP 的 MBAP 帧 */
    size_t written = 0;
    /* 非阻塞连接必须用 netconn_write_partly：netconn_write() 无法返回已写
     * 字节数，在非阻塞连接上直接返回 ERR_VAL（api_lib.c:1014-1018）。 */
    err_t err = netconn_write_partly((struct netconn*)t->conn, data, len,
                                     NETCONN_COPY, &written);
    if (err != ERR_OK || written != len) {
        MODBUS_LOG("[TCP] slot%d netconn_write err=%d len=%u written=%u",
                   (t->slot_id >= 0 ? t->slot_id : -1), (int)err, len, (unsigned)written);
        return -1;
    }
    return 0;
}

static uint16_t tcp_peek(void *ctx) {
    modbus_tcp_ctx_t *t = (modbus_tcp_ctx_t*)ctx;
    return t->ready_len;
}

static uint16_t tcp_recv(void *ctx, uint8_t *buf, uint16_t len) {
    modbus_tcp_ctx_t *t = (modbus_tcp_ctx_t*)ctx;
    uint16_t n = (t->ready_len < len) ? t->ready_len : len;
    if (n > 0) {
        memcpy(buf, t->ready, n);
        t->ready_len = 0;
    }
    return n;
}

static uint32_t tcp_get_tick(void) { return MB_GET_TICK(); }

static void tcp_delay(uint32_t ms) { MB_Delay_ms(ms); }

/* 把 netconn 收到的数据追加进 accum（溢出则整体丢弃重新同步） */
static void tcp_accumulate(modbus_tcp_ctx_t *t, const uint8_t *data, uint16_t len) {
    if (t->accum_len + len <= sizeof(t->accum)) {
        memcpy(t->accum + t->accum_len, data, len);
        t->accum_len += len;
    } else {
        t->accum_len = 0;   /* 协议异常/洪泛 → 重新同步 */
        MODBUS_LOG("[TCP] accum overflow, resync");
    }
}

/* 从 accum 切出一帧完整帧到 ready（每轮询节拍最多一帧，core 消费后下次再切） */
static void tcp_extract_frame(modbus_tcp_ctx_t *t) {
    if (t->ready_len != 0 || t->accum_len < 6) return;
    uint16_t len_field = ((uint16_t)t->accum[4] << 8) | t->accum[5];
    uint16_t frame_total = 6 + len_field;
    if (frame_total > MODBUS_BUF_SIZE) {        /* 长度字段非法 → 丢整段重同步 */
        t->accum_len = 0;
        MODBUS_LOG("[TCP] bogus length %u, resync", frame_total);
        return;
    }
    if (t->accum_len < frame_total) return;     /* 半帧，等后续数据 */
    memcpy(t->ready, t->accum, frame_total);
    t->ready_len = frame_total;
    memmove(t->accum, t->accum + frame_total, t->accum_len - frame_total);
    t->accum_len -= frame_total;
}

static void tcp_port_poll(void *ctx) {
    modbus_tcp_ctx_t *t = (modbus_tcp_ctx_t*)ctx;
    uint32_t now = MB_GET_TICK();

    if (t->is_server) {
        /* ---- server：接受挂起连接 ---- */
        if (!t->is_connected) {
            struct netconn *nc = NULL;
            err_t err = netconn_accept((struct netconn*)t->listen_conn, &nc);
            if (err == ERR_OK && nc) {
                netconn_set_nonblocking(nc, 1);
                t->conn = nc;
                t->is_connected = 1;
                t->accum_len = 0;
                t->ready_len = 0;
                MODBUS_LOG("[TCP] client connected");
            }
        }
    } else {
        /* ---- client：按 2s 间隔尝试(重)连接 ---- */
        if (!t->is_connected) {
            if (now - t->reconnect_tick >= 2000) {
                t->reconnect_tick = now;
                struct netconn *nc = netconn_new(NETCONN_TCP);
                if (nc) {
                    ip_addr_t ip;
                    if (ipaddr_aton(t->remote_ip_str, &ip)) {
                        err_t err = netconn_connect(nc, &ip, t->port);
                        if (err == ERR_OK) {
                            netconn_set_nonblocking(nc, 1);
                            t->conn = nc;
                            t->is_connected = 1;
                            t->accum_len = 0;
                            t->ready_len = 0;
                            MODBUS_LOG("[TCP] connected to %s:%u",
                                       t->remote_ip_str, t->port);
                            /* 重连成功 → 通知线路恢复（参考 RTU 主机 on_line_recover）。
                             * 仅在确为断线恢复时触发，避免与 check_line_status 定时器重复。 */
                            if (t->mb->line_state == MODBUS_LINE_DISCONNECTED) {
                                t->mb->line_state = MODBUS_LINE_OK;
                                if (t->mb->on_line_recover) t->mb->on_line_recover(t->mb);
                            }
                        } else {
                            netconn_close(nc);
                            netconn_delete(nc);
                        }
                    } else {
                        netconn_delete(nc);
                    }
                }
            }
        }
    }

    if (!t->is_connected || !t->conn) return;

    /* ---- 非阻塞收包（NETCONN_FLAG_NON_BLOCKING，recv 立即返回）---- */
    struct netbuf *nbuf = NULL;
    err_t err = netconn_recv((struct netconn*)t->conn, &nbuf);
    if (err == ERR_OK && nbuf) {
        for (struct pbuf *p = nbuf->p; p != NULL; p = p->next) {
            if (p->len > 0) tcp_accumulate(t, (const uint8_t*)p->payload, p->len);
        }
        netbuf_delete(nbuf);
    } else if (err == ERR_CLSD) {
        MODBUS_LOG("[TCP] connection closed by peer");
        netconn_close((struct netconn*)t->conn);
        netconn_delete((struct netconn*)t->conn);
        t->conn = NULL;
        t->is_connected = 0;
        t->accum_len = 0;
        t->ready_len = 0;
        /* 对端关闭 → 通知线路断开（参考 RTU 主机 on_line_break）。
         * 置 DISCONNECTED + 重置 reconnect_tick，避免 check_line_status 定时器在真正
         * 重连成功前误触发恢复通知；重连由本函数按 2s 间隔驱动。 */
        t->mb->line_state = MODBUS_LINE_DISCONNECTED;
        t->mb->reconnect_tick = now;
        if (t->mb->on_line_break) t->mb->on_line_break(t->mb);
        return;
    }

    tcp_extract_frame(t);
}

// ===========================
// 多客户端 server：slot 轮询 / 空闲踢 / 初始化
// ===========================
/* slot 的 port_poll：只做 recv + 帧重组 + 对端关闭处理（不含 accept/connect）。
 * 连接由 server 容器的 modbus_tcp_server_process 统一 accept 后挂入 slot。 */
static void tcp_slot_port_poll(void *ctx) {
    modbus_tcp_ctx_t *t = (modbus_tcp_ctx_t*)ctx;
    if (!t->is_connected || !t->conn) return;

    struct netbuf *nbuf = NULL;
    err_t err = netconn_recv((struct netconn*)t->conn, &nbuf);
    if (err == ERR_OK && nbuf) {
        uint16_t got = 0;
        for (struct pbuf *p = nbuf->p; p != NULL; p = p->next) {
            if (p->len > 0) { tcp_accumulate(t, (const uint8_t*)p->payload, p->len); got += p->len; }
        }
        netbuf_delete(nbuf);
        if (got > 0 && MODBUS_TCP_DIAG && t->slot_id >= 0)
            MODBUS_LOG("[TCP] slot%d recv %u B", t->slot_id, got);
    } else if (err == ERR_CLSD) {
        MODBUS_LOG("[TCP] slot connection closed by peer");
        netconn_close((struct netconn*)t->conn);
        netconn_delete((struct netconn*)t->conn);
        t->conn = NULL;
        t->is_connected = 0;
        t->accum_len = 0;
        t->ready_len = 0;
        return;
    }

    tcp_extract_frame(t);
}

/* slot 的 on_line_break：空闲超时（check_line_status 从机分支）触发 → 关闭本 slot socket。
 * 真正释放 slot + 上下线通知在 modbus_tcp_server_process 循环里统一做。 */
static void tcp_slot_line_break(modbus_t *ctx) {
    modbus_tcp_ctx_t *t = (modbus_tcp_ctx_t*)ctx->transport.ctx;
    if (t && t->conn) {
        netconn_close((struct netconn*)t->conn);
        netconn_delete((struct netconn*)t->conn);
    }
    if (t) { t->conn = NULL; t->is_connected = 0; }
}

/* 轻量初始化一个 slot（不注册进全局实例表，避免 modbus_process_all 重复处理）。
 * 共享模板的 data_map 指针（不拷贝数据，符合多主站网关语义）。 */
static void tcp_slot_init(modbus_tcp_client_slot_t *s, const modbus_t *tpl) {
    memset(s, 0, sizeof(*s));
    modbus_t *mb = &s->mb;
    mb->slave_addr = tpl->slave_addr;
    mb->role       = MODBUS_ROLE_SLAVE;
    mb->mode       = MODBUS_MODE_TCP;
    mb->state      = MODBUS_STATE_IDLE;
    mb->line_state = MODBUS_LINE_OK;
    mb->response_timeout   = 1000;
    mb->slave_timeout_ms   = 5000;   /* 5s 无请求 → 踢下线（check_line_status 从机分支） */
    mb->max_timeout_count  = 3;
    mb->poll_interval      = 100;
    mb->reconnect_interval = 10000;
    mb->data_map = tpl->data_map;    /* 共享指针 */
    mb->on_master_reg_change  = tpl->on_master_reg_change;
    mb->on_master_coil_change = tpl->on_master_coil_change;
    mb->on_line_break = tcp_slot_line_break;

    modbus_tcp_ctx_t *t = &s->tcp;
    memset(t, 0, sizeof(*t));
    t->mb = mb;
    t->slot_id = -1;                /* accept 时再赋具体下标 */
    t->is_server = 1;                /* server 侧：应答回显请求 tid */
    modbus_transport_t tr = {
        .ctx       = t,
        .send      = tcp_send,
        .peek      = tcp_peek,
        .recv      = tcp_recv,
        .get_tick  = tcp_get_tick,
        .delay     = tcp_delay,
        .frame_tx  = tcp_frame_tx,
        .frame_rx  = tcp_frame_rx,
        .port_poll = tcp_slot_port_poll,
    };
    modbus_set_transport(mb, &tr);
    mb->last_activity_tick = tcp_get_tick();
}

// ===========================
// 适配器初始化
// ===========================

static void tcp_attach_transport(modbus_t *mb, modbus_tcp_ctx_t *t) {
    modbus_transport_t tr = {
        .ctx       = t,
        .send      = tcp_send,
        .peek      = tcp_peek,
        .recv      = tcp_recv,
        .get_tick  = tcp_get_tick,
        .delay     = tcp_delay,
        .frame_tx  = tcp_frame_tx,
        .frame_rx  = tcp_frame_rx,
        .port_poll = tcp_port_poll,
    };
    modbus_set_transport(mb, &tr);
    mb->mode = MODBUS_MODE_TCP;
    /* 端口接管线路活性：TCP 连接的断开/恢复由 port_poll 直接反映，
     * 防止 core 在首次请求前因 last_activity_tick=0 误判从机断线 */
    mb->last_activity_tick = tcp_get_tick();
}

int modbus_tcp_server_init(modbus_t *mb, modbus_tcp_server_t *srv, uint16_t port) {
    if (!mb || !srv) return -1;
    memset(srv, 0, sizeof(*srv));

    struct netconn *ln = netconn_new(NETCONN_TCP);
    if (!ln) return -1;
    if (netconn_bind(ln, IP_ADDR_ANY, port) != ERR_OK) {
        netconn_delete(ln);
        return -1;
    }
    if (netconn_listen(ln) != ERR_OK) {
        netconn_delete(ln);
        return -1;
    }
    netconn_set_nonblocking(ln, 1);   /* 非阻塞 accept */

    srv->listen_conn = ln;
    srv->port = port;
    srv->slave_tpl = mb;
    for (int i = 0; i < MODBUS_TCP_MAX_CLIENTS; i++) {
        tcp_slot_init(&srv->slots[i], mb);
    }
    MODBUS_LOG("[TCP] multi-client server listening on :%u (unit=%u, max=%d)",
               port, mb->slave_addr, MODBUS_TCP_MAX_CLIENTS);
    return 0;
}

void modbus_tcp_server_process(modbus_tcp_server_t *srv) {
    if (!srv || !srv->listen_conn) return;
    uint32_t now = MB_GET_TICK();
    ip_addr_t peer;
    u16_t peer_port;

    /* 1. 批量 accept 所有挂起连接（nonblocking，直到无连接）。
     *    改为 while 循环：Modbus Poll 等工具常“同时”发起多连接，
     *    若每 tick 只 accept 一次，并发连接会积压甚至被丢弃（表现为第二个 client 超时）。 */
    struct netconn *nc = NULL;
    while (netconn_accept((struct netconn*)srv->listen_conn, &nc) == ERR_OK && nc) {
        int idx = -1;
        for (int i = 0; i < MODBUS_TCP_MAX_CLIENTS; i++) {
            if (!srv->slots[i].active) { idx = i; break; }
        }
        if (idx >= 0) {
            modbus_tcp_client_slot_t *s = &srv->slots[idx];
            netconn_set_nonblocking(nc, 1);
            s->tcp.conn = nc;
            s->tcp.is_connected = 1;
            s->tcp.slot_id = idx;
            s->tcp.accum_len = 0;
            s->tcp.ready_len = 0;
            s->tcp.last_rx_tid = 0;
            s->active = 1;
            s->mb.state = MODBUS_STATE_IDLE;
            s->mb.line_state = MODBUS_LINE_OK;
            s->mb.timeout_count = 0;
            s->mb.last_activity_tick = now;
            if (netconn_peer(nc, &peer, &peer_port) == ERR_OK)
                ipaddr_ntoa_r(&peer, s->ip, sizeof(s->ip));
            else
                s->ip[0] = '\0';
            MODBUS_LOG("[TCP] accept -> client %d (%s)", idx, s->ip);
            if (srv->on_client_connect) srv->on_client_connect(idx, s->ip);
        } else {
            MODBUS_LOG("[TCP] max clients reached, reject");
            netconn_close(nc);
            netconn_delete(nc);
        }
        nc = NULL;  /* 防御：避免 while 条件误判（accept 返回非 OK 即退出，此处仅保险） */
    }

    /* 2. 逐 slot 处理；断开/空闲超时（on_line_break 已关 socket）则释放 + 通知 */
    for (int i = 0; i < MODBUS_TCP_MAX_CLIENTS; i++) {
        modbus_tcp_client_slot_t *s = &srv->slots[i];
        if (!s->active) continue;
        modbus_process(&s->mb);
        if (!s->tcp.is_connected) {
            MODBUS_LOG("[TCP] client %d disconnected", i);
            if (srv->on_client_disconnect) srv->on_client_disconnect(i, s->ip);
            s->active = 0;
            s->tcp.conn = NULL;
            s->ip[0] = '\0';
        }
    }
}

int modbus_tcp_client_init(modbus_t *mb, modbus_tcp_ctx_t *tcp,
                           const char *ip_str, uint16_t port) {
    if (!mb || !tcp || !ip_str) return -1;
    memset(tcp, 0, sizeof(*tcp));

    strncpy(tcp->remote_ip_str, ip_str, sizeof(tcp->remote_ip_str) - 1);
    tcp->remote_ip_str[sizeof(tcp->remote_ip_str) - 1] = '\0';

    tcp->mb = mb;
    tcp->is_server = 0;
    tcp->port = port;
    tcp_attach_transport(mb, tcp);
    MODBUS_LOG("[TCP] client target %s:%u (master, target unit=%u)",
               ip_str, port, mb->slave_addr);
    return 0;
}

#endif /* MODBUS_ENABLE_TCP */
