#ifndef __MODBUS_TASK_H__
#define __MODBUS_TASK_H__

#include "modbus_app.h"

typedef enum {
    MBT_IDLE = 0x81,    // 空闲

    MBT_TX_ENTRY,       // 发送
    MBT_TX_ING,         // 发送进行
    MBT_TX_CPLT,        // 发送完成
    MBT_TX_ERR,         // 发送出错
    MBT_TX_TIMEOUT,     // 发送超时
    MBT_RX_ENTRY,       // 接收
    MBT_RX_ING,         // 接收进行
    MBT_RX_CPLT,        // 接收完成
    MBT_RX_ERR,         // 接收出错
    MBT_RX_TIMEOUT,     // 接收超时

    MBT_REQUESTING,     // 请求
    MBT_RESPONDING,     // 响应
    MBT_PARSING,        // 解析
    MBT_CALLBACK,       // 回调
} mbTask_Status_t;

// 定义函数指针类型
typedef struct
{
    mbTask_Status_t status;
    uint32_t        timeout; // 超时时间(ms)
    uint8_t         retry;   // 重试计数器
}Modbus_Task_t;

#if ENABLE_MOUBUS_MASTER  
extern Modbus_Task_t mbMaster_Task;
void mbMaster_Poll(void);
void mbMaster_Callback(void);
#endif // ENABLE_MOUBUS_MASTER

#if  ENABLE_MOUBUS_SLAVE 
extern Modbus_Task_t mbSlave_Task;
void mbSlave_Poll(void);
void mbSlave_Callback(void);
#endif // ENABLE_MOUBUS_SLAVE

#endif // __MODBUS_TASK_H__
