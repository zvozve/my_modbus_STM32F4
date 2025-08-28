#ifndef __MODBUS_APP_H__
#define __MODBUS_APP_H__

#include "modbus_driver.h"
#include "modbus_func.h"

void Modbus_Init(void);

#if  ENABLE_MB_RTU_MASTER
void mbMaster_Init(void);
void mbMaster_Test(void);
void mbMaster_Poll(void);
void mbMaster_Callback(void);
#endif // ENABLE_MB_RTU_MASTER

#if  ENABLE_MB_RTU_SLAVE
void mbSlave_Init(void);
void mbSlave_Test(void);
void mbSlave_Poll(void);
void mbSlave_Callback(void);
#endif // ENABLE_MB_RTU_SLAVE

void MBT_UART_Print_STA(const char *sFormat, mbTask_Status_t mbtStatus, mbFxStatus_t fxStatus);
#endif // __MODBUS_APP_H__
