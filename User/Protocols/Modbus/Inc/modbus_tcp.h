#ifndef __MODBUS_TCP_H__
#define __MODBUS_TCP_H__

#include "modbus_core.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 本文件内容依赖 MODBUS_ENABLE_TCP（在 modbus_core.h 定义，默认 1，可被 CMake -D 覆盖）。
 * 已强制 include modbus_core.h，保证门控宏一定已定义（与 modbus_rtu.h 同约定）。
 *
 * 本头文件保持 LwIP-free：netconn 指针以 void* 存放、远端 IP 以字符串传入，
 * 协议层头不依赖任何 BSP 类型；LwIP 头只在 modbus_tcp.c 内部（#ifdef 门内）引用。 */

#ifdef MODBUS_ENABLE_TCP

#define MODBUS_TCP_DEFAULT_PORT   502

/* TCP 端口上下文（每实例一份）：
 *  - server(从机侧连接)：listen_conn 监听，accept 得 conn；应答回显请求 tid
 *  - client(主机侧连接)：conn 主动连接；每请求自增 tid 供应答匹配
 *  - 收包重组：netconn 是字节流，先累积到 accum，按 MBAP 长度字段切出完整帧
 *    放入 ready 单帧槽，core 的 peek/recv 从 ready 取；一次 TCP 段可能含多帧
 *    或半帧，port_poll 每个轮询节拍最多向 ready 投递一帧，天然逐帧串行。 */
typedef struct {
    modbus_t *mb;               /* 关联的 core 实例 */
    void *conn;                 /* 活动连接 netconn*（server: accept 所得 / client: 已连接） */
    void *listen_conn;          /* server: 监听 netconn* */
    uint16_t port;              /* server: 监听端口；client: 远端端口 */
    char remote_ip_str[16];     /* client: 远端 IP（可读字符串，重连时重新解析） */
    uint8_t is_server;          /* 1=server（从机侧），0=client（主机侧） */
    uint8_t is_connected;

    /* 事务 ID */
    uint16_t next_tx_tid;       /* client(master): 每发一请求自增 */
    uint16_t last_rx_tid;       /* server(slave): 应答回显请求的 tid */

    /* TCP 流重组 */
    uint8_t accum[MODBUS_BUF_SIZE + 8];
    uint16_t accum_len;
    uint8_t ready[MODBUS_BUF_SIZE + 8];
    uint16_t ready_len;

    uint32_t reconnect_tick;    /* client: 重连节拍 */
} modbus_tcp_ctx_t;

/* 从机(TCP server)适配器：modbus 实例须先 modbus_init() 并设置 data_map/slave_addr。
 * 内部创建监听 socket 并注册 transport（frame_tx/frame_rx/port_poll 全部挂上）。
 * 返回 0=成功，<0=失败。 */
int modbus_tcp_server_init(modbus_t *mb, modbus_tcp_ctx_t *tcp, uint16_t port);

/* 主机(TCP client)适配器：modbus 实例须先 modbus_master_init()（传输无关仲裁器）。
 * 内部创建 socket 并尝试连接（失败由 port_poll 按 2s 间隔重连）。
 * 返回 0=成功（仅指参数合法，不代表已连上），<0=失败。 */
int modbus_tcp_client_init(modbus_t *mb, modbus_tcp_ctx_t *tcp,
                           const char *ip_str, uint16_t port);

/* MBAP 帧封装（TCP 端口层专属）：
 *  - tcp_frame_tx: core 的 [unit][func][data...] → 线上 [tid][pid=0][len][unit][func][data...]
 *  - tcp_frame_rx: 线上帧校验(pid=0、长度匹配)后重排为 [unit][func][data...] 给 core，
 *    并把请求 tid 存入 last_rx_tid 供应答回显；返回 0=成功，<0=帧错误。 */
uint16_t tcp_frame_tx(void *ctx, const uint8_t *pdu, uint16_t pdu_len,
                      uint8_t *out, uint16_t out_cap);
int      tcp_frame_rx(void *ctx, uint8_t *raw, uint16_t *raw_len);

#endif /* MODBUS_ENABLE_TCP */

#ifdef __cplusplus
}
#endif

#endif /* __MODBUS_TCP_H__ */
