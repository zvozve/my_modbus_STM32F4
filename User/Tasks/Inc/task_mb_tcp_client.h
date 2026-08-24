#ifndef __TASK_MB_TCP_CLIENT_H__
#define __TASK_MB_TCP_CLIENT_H__

#include "modbus_core.h"   /* 提供 modbus_t 类型 */

/* ============================================================
 * TCP client（主机/master）多实例测试任务
 *
 * 注册 MB_TCP_CLIENT_COUNT 个 client 实例，每个连一个远端 Modbus TCP 从机，
 * 验证"多 client 实例并存"+ 每个实例的读/写/断线通知。
 *
 * RAM 提示：每个 client 实例约 +1.5KB（modbus_t + ctx + 回读缓冲）。
 * 与 server 并存时如 RAM 紧张，在 app_main.c 把 APP_TASK_MB_TCP_SERVER 设 0。
 * ============================================================ */
#ifndef MB_TCP_CLIENT_COUNT
#define MB_TCP_CLIENT_COUNT   2      /* 注册的 client 实例数（按需调大，受 RAM 限制） */
#endif

void TaskModbus_TCP_Client_Init(void);
void TaskModbus_TCP_Client_Process(void);

/* 取第 i 个 client 实例（0-based），供其它模块触发一次性读写测试 */
modbus_t *TaskModbus_TCP_Client_Get(uint8_t idx);

#endif
