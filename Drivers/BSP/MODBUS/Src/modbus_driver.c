#include "modbus_driver.h"
#include <string.h>

/*
***********************************************
**  MODBUS_TIM DRIVER
***********************************************
*/
volatile uint32_t us_count = 0; // 在中断中递增的计数器

void MB_TIM_Delay_us(uint16_t us) {
    uint32_t start = us_count;
    int32_t ticks = us / MB_TIM_PERIOD_US; // 需要等待的ticks数
    while ((int32_t)(us_count - start) < ticks); // 正确处理溢出
}

// 毫秒级延时
void MB_TIM_Delay_ms(uint16_t ms) {
    for (uint16_t i = 0; i < ms; i++)
    {
        MB_TIM_Delay_us(1000);  // 1ms = 1000us
    }
}

/*
***********************************************
**  MODBUS_USART DRIVER
***********************************************
*/

#if ENABLE_MB_RTU_MASTER
Modbus_Task_t mbMaster_Task;
uint8_t  mbMaster_Tx_Buf[MB_BUF_LEN];
uint16_t mbMaster_Tx_Byte;
uint8_t  mbMaster_Rx_Buf[MB_BUF_LEN];
uint16_t mbMaster_Rx_Byte;
#endif // ENABLE_MB_RTU_MASTER

#if  ENABLE_MB_RTU_SLAVE
Modbus_Task_t mbSlave_Task;
uint8_t  mbSlave_Tx_Buf[MB_BUF_LEN];
uint16_t mbSlave_Tx_Byte;
uint8_t  mbSlave_Rx_Buf[MB_BUF_LEN];
uint16_t mbSlave_Rx_Byte;
#endif // ENABLE_MB_RTU_SLAVE   

uint8_t MBT_UART_Unstable(UART_HandleTypeDef *huart) {
    if (huart->ErrorCode != HAL_UART_ERROR_NONE) {
        if (huart->ErrorCode & HAL_UART_ERROR_ORE) {
            SEGGER_RTT_printf(0, "MBT_UART_Unstable: Overrun!\n");
            return 1;
        }
        if (huart->ErrorCode & HAL_UART_ERROR_NE)  {
            SEGGER_RTT_printf(0, "MBT_UART_Unstable: Noise!\n");
            return 2;
        }
        if (huart->ErrorCode & HAL_UART_ERROR_FE)  {
            SEGGER_RTT_printf(0, "MBT_UART_Unstable: Framing!\n");
            return 3;
        }
    }
    return 0;
}

void MBT_UART_Repair(UART_HandleTypeDef *huart) {
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
    
    #if ENABLE_MB_RTU_MASTER
    if (huart == &huart_mbm){
    }
    #endif // ENABLE_MB_RTU_MASTER

    #if  ENABLE_MB_RTU_SLAVE
    if (huart == &huart_mbs){
    }
    #endif // ENABLE_MB_RTU_SLAVE
}

void MBT_UART_Transmit_DMA(UART_HandleTypeDef *huart) {
    #if ENABLE_MB_RTU_MASTER
    if (huart == &huart_mbm){
        if (mbMaster_Task.status != MBT_TX_ING){
            MBT_UART_Print_CMD("MB_MASTER_TX CPLT: ", mbMaster_Tx_Buf, mbMaster_Tx_Byte);
            #if MBM_USE_RS485
            MBM_RS485_TX_MODE();
            #endif // MBM_USE_RS485
            if (HAL_UART_Transmit_DMA(huart, mbMaster_Tx_Buf, mbMaster_Tx_Byte) != HAL_OK) {                
                mbMaster_Task.status = MBT_TX_ERR;
            } else {
                mbMaster_Task.status = MBT_TX_ING;
            } 
        }
    }
    #endif // ENABLE_MB_RTU_MASTER

    #if  ENABLE_MB_RTU_SLAVE
    if (huart == &huart_mbs){
        if (mbSlave_Task.status != MBT_TX_ING){
           MBT_UART_Print_CMD("MB_SLAVE_TX CPLT: ", mbSlave_Tx_Buf, mbSlave_Tx_Byte);
            #if MBS_USE_RS485
            MBS_RS485_TX_MODE();
            #endif // MBS_USE_RS485
            if (HAL_UART_Transmit_DMA(huart, mbSlave_Tx_Buf, mbSlave_Tx_Byte) != HAL_OK) {
                mbSlave_Task.status = MBT_TX_ERR;
            } else {
                mbSlave_Task.status = MBT_TX_ING;
            }        
        }
    }
    #endif // ENABLE_MB_RTU_SLAVE
}

void MBT_UART_Receive_DMA(UART_HandleTypeDef *huart) {
    #if ENABLE_MB_RTU_MASTER  
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
    #endif // ENABLE_MB_RTU_MASTER
        
    #if  ENABLE_MB_RTU_SLAVE
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
    #endif // ENABLE_MB_RTU_SLAVE  
}

void MBT_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    #if ENABLE_MOUBUS_DEBUG
    SEGGER_RTT_printf(0, "HAL_UART_TxCpltCallback!\n");
    #endif // ENABLE_MOUBUS_DEBUG

    #if ENABLE_MB_RTU_MASTER
    if (huart == &huart_mbm) {
        if(mbMaster_Task.status == MBT_TX_ING){
            mbMaster_Task.status = MBT_TX_CPLT;
        }
    }
    #endif // ENABLE_MB_RTU_MASTER
    
    #if ENABLE_MB_RTU_SLAVE
    if (huart == &huart_mbs) {
        if(mbSlave_Task.status == MBT_TX_ING){
            mbSlave_Task.status = MBT_TX_CPLT;
        }
    }
    #endif // ENABLE_MB_RTU_SLAVE
}

void USR_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    #if ENABLE_MOUBUS_DEBUG
    SEGGER_RTT_printf(0, "HAL_UARTEx_RxEventCallback!\n");
    #endif // ENABLE_MOUBUS_DEBUG

    #if ENABLE_MB_RTU_MASTER  
    if (huart == &huart_mbm) {
        if (mbMaster_Task.status == MBT_RX_ING){
            mbMaster_Rx_Byte = Size;
            MBT_UART_Print_CMD("MB_MASTER_RX CPLT: ", mbMaster_Rx_Buf, mbMaster_Rx_Byte);
            mbMaster_Task.status = MBT_RX_CPLT;
        }
    }
    #endif // ENABLE_MB_RTU_MASTER

    #if ENABLE_MB_RTU_SLAVE  
    if (huart == &huart_mbs) {
        if (mbSlave_Task.status == MBT_RX_ING) {
            mbSlave_Rx_Byte = Size;
            MBT_UART_Print_CMD("MB_SLAVE_RX CPLT: ", mbSlave_Rx_Buf, mbSlave_Rx_Byte);
            mbSlave_Task.status = MBT_RX_CPLT;
        }
    }
    #endif // ENABLE_MB_RTU_SLAVE
}

// 调试打印
void MBT_UART_Print_CMD(const char *sFormat, uint8_t *pCmd, uint16_t len) {
    #if ENABLE_MOUBIS_DEBUG
    // 先输出格式化的前缀信息
    SEGGER_RTT_WriteString(0, sFormat);
    
    // 然后输出长度信息
    SEGGER_RTT_printf(0, "len %d\n", len);
    
    // 输出16进制数据
    for (uint16_t i = 0; i < len; i++) {
        SEGGER_RTT_printf(0, "%02X ", pCmd[i]);
    }
    SEGGER_RTT_printf(0, "\n");
    #endif // ENABLE_MOUBIS_DEBUG
}
