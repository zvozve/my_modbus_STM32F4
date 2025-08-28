#ifndef __MODBUS_DRIVER_H__
#define __MODBUS_DRIVER_H__

#include "main.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include <math.h>

#include "SEGGER_RTT.h"

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

/*
***********************************************
**  MODBUS_BASIC CONFIG
***********************************************
*/

#define ENABLE_MB_RTU_MASTER 1
#define ENABLE_MB_RTU_SLAVE  1

#define ENABLE_MOUBUS_DEBUG  0
#define ENABLE_MOUBUS_DMACPY 0

#define ENABLE_MASTER_TX_BLOCK 1

/*
***********************************************
**  MODBUS_TIM CONFIG
***********************************************
*/

#define TIM_MB              TIM6
#define htim_mb             htim6
extern TIM_HandleTypeDef    htim_mb;

#define MB_TIM_PERIOD_US    (1000)
#define MB_TIM_MAX_DELAY    (1000 * 1000 / MB_TIM_PERIOD_US)
#define MB_TIM_MAX_RETRY    (3)

extern volatile uint32_t us_count;

void MB_TIM_Delay_us(uint16_t us);
void MB_TIM_Delay_ms(uint16_t ms);

/*
***********************************************
**  MODBUS_USART_GPIO CONFIG
***********************************************
*/

#define MB_BUF_LEN 1024

#if  ENABLE_MB_RTU_MASTER 
extern Modbus_Task_t mbMaster_Task;

#define UART_MBM            USART2
#define huart_mbm           huart2
#define MBM_SLAVE_ID        0x01
extern UART_HandleTypeDef huart_mbm;

#define MBM_USE_RS485      1
#if  MBM_USE_RS485
#define MBM_RS485_TX_MODE() HAL_GPIO_WritePin(RS485_EN_U2_GPIO_Port, RS485_EN_U2_Pin, GPIO_PIN_SET)   // 发送模式
#define MBM_RS485_RX_MODE() HAL_GPIO_WritePin(RS485_EN_U2_GPIO_Port, RS485_EN_U2_Pin, GPIO_PIN_RESET) // 接收模式
#endif // MBM_USE_RS485

extern uint8_t  mbMaster_Tx_Buf[MB_BUF_LEN];
extern uint16_t mbMaster_Tx_Byte;
extern uint8_t  mbMaster_Rx_Buf[MB_BUF_LEN];
extern uint16_t mbMaster_Rx_Byte;
#endif // ENABLE_MB_RTU_MASTER

#if  ENABLE_MB_RTU_SLAVE
extern Modbus_Task_t mbSlave_Task;

#define UART_MBS            USART3
#define huart_mbs           huart3
#define MBS_SLAVE_ID        0x01
extern UART_HandleTypeDef huart_mbs;

#define MBS_USE_RS485      0
#if  MBS_USE_RS485
#define MBS_RS485_TX_MODE() HAL_GPIO_WritePin(RS485_EN_U2_GPIO_Port, RS485_EN_U3_Pin, GPIO_PIN_SET)   // 发送模式
#define MBS_RS485_RX_MODE() HAL_GPIO_WritePin(RS485_EN_U2_GPIO_Port, RS485_EN_U3_Pin, GPIO_PIN_RESET) // 接收模式
#endif // MBM_USE_RS485

extern uint8_t  mbSlave_Tx_Buf[MB_BUF_LEN];
extern uint16_t mbSlave_Tx_Byte;
extern uint8_t  mbSlave_Rx_Buf[MB_BUF_LEN];
extern uint16_t mbSlave_Rx_Byte;
#endif // ENABLE_MB_RTU_SLAVE

uint8_t MBT_UART_Unstable(UART_HandleTypeDef *huart);
void MBT_UART_Repair(UART_HandleTypeDef *huart);

void MBT_UART_Transmit_DMA(UART_HandleTypeDef *huart);
void MBT_UART_Receive_DMA(UART_HandleTypeDef *huart);
void MBT_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void USR_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size);
void MBT_UART_Print_CMD(const char *sFormat, uint8_t *pCmd, uint16_t len);
#endif // __MODBUS_DRIVER_H__
