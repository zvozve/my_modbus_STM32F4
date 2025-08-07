#ifndef __MODBUS_TASK_H__
#define __MODBUS_TASK_H__

#include "modbus_app.h"

typedef enum {
    MBT_IDLE = 0x81,    // ����

    MBT_TX_ENTRY,       // ����
    MBT_TX_ING,         // ���ͽ���
    MBT_TX_CPLT,        // �������
    MBT_TX_ERR,         // ���ͳ���
    MBT_TX_TIMEOUT,     // ���ͳ�ʱ
    MBT_RX_ENTRY,       // ����
    MBT_RX_ING,         // ���ս���
    MBT_RX_CPLT,        // �������
    MBT_RX_ERR,         // ���ճ���
    MBT_RX_TIMEOUT,     // ���ճ�ʱ

    MBT_REQUESTING,     // ����
    MBT_RESPONDING,     // ��Ӧ
    MBT_PARSING,        // ����
    MBT_CALLBACK,       // �ص�
} mbTask_Status_t;

// ���庯��ָ������
typedef struct
{
    mbTask_Status_t status;
    uint32_t        timeout; // ��ʱʱ��(ms)
    uint8_t         retry;   // ���Լ�����
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
