#ifndef __MODBUS_UART_H__
#define __MODBUS_UART_H__

#include "modbus_app.h"

#if  ENABLE_MOUBUS_MASTER
#define MBM_UARTx          USART2
#define MBM_UARTx_Handle   huart2
#define MBM_SLAVE_ID       0x01
extern UART_HandleTypeDef MBM_UARTx_Handle;
extern uint8_t  mbMaster_Rx_Buf[MB_BUF_LEN];
extern uint16_t mbMaster_Rx_Byte;
#endif // ENABLE_MOUBUS_MASTER

#if  ENABLE_MOUBUS_SLAVE
#define MBS_UARTx          USART2
#define MBS_UARTx_Handle   huart2
#define MBS_SLAVE_ID       0x01
extern UART_HandleTypeDef MBS_UARTx_Handle;
extern uint8_t  mbSlave_Rx_Buf[MB_BUF_LEN];
extern uint16_t mbSlave_Rx_Byte;
#endif // ENABLE_MOUBUS_SLAVE

enum {
    MB_MASTER_TX = 1,
    MB_MASTER_RX,
    MB_SLAVE_TX,
    MB_SLAVE_RX,
};

uint8_t USR_UART_Unstable(UART_HandleTypeDef *huart);
void USR_UART_Repair(UART_HandleTypeDef *huart);

void USR_UART_Transmit_DMA(UART_HandleTypeDef *huart);
void USR_UART_Receive_DMA(UART_HandleTypeDef *huart);
void USR_UART_Print_CMD(uint8_t id, uint8_t *pCmd, uint16_t len);
void USR_UART_Print_STA(void);

#endif // __MODBUS_UART_H__
