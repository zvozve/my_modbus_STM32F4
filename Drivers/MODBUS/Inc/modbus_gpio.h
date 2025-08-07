#ifndef __MODBUS_GPIO_H__
#define __MODBUS_GPIO_H__

#include "modbus_app.h"

#if  ENABLE_MOUBUS_MASTER
#define MBM_USE_RS485      1
#if  MBM_USE_RS485
#define MBM_RS485_TX_MODE() HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_SET)   // 发送模式
#define MBM_RS485_RX_MODE() HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_RESET) // 接收模式
#endif // MBM_USE_RS485
#endif // ENABLE_MOUBUS_MASTER

#if  ENABLE_MOUBUS_SLAVE
#define MBS_USE_RS485      1
#if  MBS_USE_RS485
#define MBS_RS485_TX_MODE() HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_SET)   // 发送模式
#define MBS_RS485_RX_MODE() HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_RESET) // 接收模式
#endif // MBM_USE_RS485
#endif // ENABLE_MOUBUS_SLAVE

#endif // __MODBUS_GPIO_H__
