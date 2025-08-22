#include "modbus_uart.h"
#include "modbus_task.h"
#include "modbus_func.h"
#include "modbus_gpio.h"

#if ENABLE_MOUBUS_MASTER
uint8_t  mbMaster_Rx_Buf[MB_BUF_LEN];
uint16_t mbMaster_Rx_Byte;
#endif // ENABLE_MOUBUS_MASTER

#if  ENABLE_MOUBUS_SLAVE
uint8_t  mbSlave_Rx_Buf[MB_BUF_LEN];
uint16_t mbSlave_Rx_Byte;
#endif // ENABLE_MOUBUS_SLAVE   

uint8_t USR_UART_Unstable(UART_HandleTypeDef *huart) {
    if (huart->ErrorCode != HAL_UART_ERROR_NONE) {
        if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
            SEGGER_RTT_printf(0, "USR_UART_Unstable: Overrun!\n");
            return 1;
        }
        if (huart->ErrorCode & HAL_UART_ERROR_NE)  {
            SEGGER_RTT_printf(0, "USR_UART_Unstable: Noise!\n");
            return 2;
        }
        if (huart->ErrorCode & HAL_UART_ERROR_FE)  {
            SEGGER_RTT_printf(0, "USR_UART_Unstable: Framing!\n");
            return 3;
        }
    }
    return 0;
}

void USR_UART_Repair(UART_HandleTypeDef *huart) {
    // 停止DMA传输
    HAL_UART_DMAStop(huart);
    
    // 根据芯片系列选择清除方式
    #if defined(USART_ISR_FE) // STM32G0/G4/H7等
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_FEF | UART_CLEAR_OREF);
    #elif defined(UART_SR_FE) // STM32F1/F4/L1等
        volatile uint32_t tmp = huart->Instance->SR;
        tmp = huart->Instance->DR;
        (void)tmp;
    #endif

    // 清除所有错误标志
    huart->ErrorCode = HAL_UART_ERROR_NONE;
    
    // 重新初始化串口
    HAL_UART_DeInit(huart);
    HAL_UART_Init(huart);
    
    #if ENABLE_MOUBUS_MASTER
    if (huart == &huart_mbm){
    }
    #endif // ENABLE_MOUBUS_MASTER

    #if  ENABLE_MOUBUS_SLAVE
    if (huart == &huart_mbs){
    }
    #endif // ENABLE_MOUBUS_SLAVE
}

void USR_UART_Transmit_DMA(UART_HandleTypeDef *huart) {
    #if ENABLE_MOUBUS_MASTER
    if (huart == &huart_mbm){
        if (mbMaster_Task.status != MBT_TX_ING){
//            USR_UART_Print_CMD(MB_MASTER_TX, mbMaster.Tx.cmdBuf, mbMaster.Tx.cmdByte);
            #if MBM_USE_RS485
            MBM_RS485_TX_MODE();
            #endif // MBM_USE_RS485
            if (HAL_UART_Transmit_DMA(huart, mbMaster.Tx.cmdBuf, mbMaster.Tx.cmdByte) != HAL_OK) {                
                mbMaster_Task.status = MBT_TX_ERR;
            } else {
                mbMaster_Task.status = MBT_TX_ING;
            } 
        }
    }
    #endif // ENABLE_MOUBUS_MASTER

    #if  ENABLE_MOUBUS_SLAVE
    if (huart == &huart_mbs){
        if (mbSlave_Task.status != MBT_TX_ING){
            USR_UART_Print_CMD(MB_SLAVE_TX, mbSlave.Tx.cmdBuf, mbSlave.Tx.cmdByte);
            #if MBS_USE_RS485
            MBS_RS485_TX_MODE();
            #endif // MBS_USE_RS485
            if (HAL_UART_Transmit_DMA(huart, mbSlave.Tx.cmdBuf, mbSlave.Tx.cmdByte) != HAL_OK) {
                mbSlave_Task.status = MBT_TX_ERR;
            } else {
                mbSlave_Task.status = MBT_TX_ING;
            }        
        }
    }
    #endif // ENABLE_MOUBUS_SLAVE
}
 
void USR_UART_Receive_DMA(UART_HandleTypeDef *huart) {
    #if ENABLE_MOUBUS_MASTER  
    if (huart == &huart_mbm){
        if (mbMaster_Task.status != MBT_RX_ING){
            memset(mbMaster_Rx_Buf, 0, MB_BUF_LEN);
            #if MBM_USE_RS485
            MBM_RS485_RX_MODE();
            #endif // MBM_USE_RS485
            if (HAL_UARTEx_ReceiveToIdle_DMA(huart, mbMaster_Rx_Buf, MB_BUF_LEN) != HAL_OK) {
                mbMaster_Task.status = MBT_RX_ERR;
            } else {
                mbMaster_Task.status = MBT_RX_ING;
            }
        }
    }
    #endif // ENABLE_MOUBUS_MASTER
        
    #if  ENABLE_MOUBUS_SLAVE
    if (huart == &huart_mbs){
        if (mbSlave_Task.status != MBT_RX_ING){
            memset(mbSlave_Rx_Buf, 0, MB_BUF_LEN);
            #if MBS_USE_RS485
            MBS_RS485_RX_MODE();
            #endif // MBS_USE_RS485
            if (HAL_UARTEx_ReceiveToIdle_DMA(huart, mbSlave_Rx_Buf, MB_BUF_LEN) != HAL_OK) {
                mbSlave_Task.status = MBT_RX_ERR;
            } else {
                mbSlave_Task.status = MBT_RX_ING;
            }        
        }
    }
    #endif // ENABLE_MOUBUS_SLAVE  
}

// 发送完成回调
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
//    SEGGER_RTT_printf(0, "HAL_UART_TxCpltCallback!\n");

    #if ENABLE_MOUBUS_MASTER
    if (huart == &huart_mbm) {
        if(mbMaster_Task.status == MBT_TX_ING){
            mbMaster_Task.status = MBT_TX_CPLT;
        }
    }
    #endif // ENABLE_MOUBUS_MASTER
    
    #if ENABLE_MOUBUS_SLAVE
    if (huart == &huart_mbs) {
        if(mbSlave_Task.status == MBT_TX_ING){
            mbSlave_Task.status = MBT_TX_CPLT;
        }
    }
    #endif // ENABLE_MOUBUS_SLAVE
}

// 接收完成回调
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
//    SEGGER_RTT_printf(0, "HAL_UARTEx_RxEventCallback!\n");
    #if ENABLE_MOUBUS_MASTER  
    if (huart == &huart_mbm) {
        if (mbMaster_Task.status == MBT_RX_ING){
            mbMaster_Rx_Byte = Size;
//            USR_UART_Print_CMD(MB_MASTER_RX, mbMaster_Rx_Buf, mbMaster_Rx_Byte);
            mbMaster_Task.status = MBT_RX_CPLT;
        }
    }
    #endif // ENABLE_MOUBUS_MASTER

    #if ENABLE_MOUBUS_SLAVE  
    // USR_UART_Print_STA();
    // USR_UART_Print_CMD(MB_SLAVE_RX, mbSlave_Rx_Buf, mbSlave_Rx_Byte);
    if (huart == &huart_mbs) {
        if (mbSlave_Task.status == MBT_RX_ING) {
            mbSlave_Rx_Byte = Size;
            USR_UART_Print_CMD(MB_SLAVE_RX, mbSlave_Rx_Buf, mbSlave_Rx_Byte);
            mbSlave_Task.status = MBT_RX_CPLT;
        }
    }
    #endif // ENABLE_MOUBUS_SLAVE
}

// 中断处理函数
void USART2_IRQHandler(void) {
    #if ENABLE_MOUBUS_MASTER
    HAL_UART_IRQHandler(&huart_mbm);
    #endif // ENABLE_MOUBUS_MASTER

    #if ENABLE_MOUBUS_SLAVE  
    HAL_UART_IRQHandler(&huart_mbs);
    #endif // ENABLE_MOUBUS_SLAVE
}

// 调试打印
void USR_UART_Print_CMD(uint8_t id, uint8_t *pCmd, uint16_t len) {
    #if ENABLE_MOUBUS_DEBUG
    if (id == MB_MASTER_TX) {
        SEGGER_RTT_printf(0, "MB_MASTER_TX CPLT: len %d\n", len);
    } else if (id == MB_MASTER_RX) {
        SEGGER_RTT_printf(0, "MB_MASTER_RX CPLT: len %d\n", len);    
    } else if (id == MB_SLAVE_TX) {
        SEGGER_RTT_printf(0, "MB_SLAVE_TX CPLT: len %d\n", len);        
    } else if (id == MB_SLAVE_RX) {
        SEGGER_RTT_printf(0, "MB_SLAVE_RX CPLT: len %d\n", len);            
    } 

    for (uint16_t i = 0; i < len; i++) {
        SEGGER_RTT_printf(0, "%02X ", pCmd[i]);  // 输出16进制数据
    }
    SEGGER_RTT_printf(0, "\n");
    #endif // ENABLE_MOUBUS_DEBUG
}

void USR_UART_Print_STA(void) {
    #if ENABLE_MOUBUS_DEBUG
    #if ENABLE_MOUBUS_MASTER  
    SEGGER_RTT_printf(0, "mbMaster mbTask_Status_t: %02X, mbFxStatus_t %02X\n", \
                          mbMaster_Task.status, mbMaster.FxStatus);
    #endif // ENABLE_MOUBUS_MASTER
    
    #if ENABLE_MOUBUS_SLAVE 
    SEGGER_RTT_printf(0, "mbSlave mbTask_Status_t: %02X, mbFxStatus_t %02X\n", \
                          mbSlave_Task.status, mbSlave.FxStatus);
    #endif // ENABLE_MOUBUS_SLAVE

    #endif // ENABLE_MOUBUS_DEBUG
}
