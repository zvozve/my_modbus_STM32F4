#include "modbus_task.h"

#if ENABLE_MOUBUS_MASTER  
Modbus_Task_t mbMaster_Task;

void mbMaster_Poll(void) {
//    USR_UART_Print_STA();
    switch (mbMaster_Task.status) {
        // 静态
        case MBT_IDLE:
        break;
        // 构建请求，发送
        case MBT_REQUESTING:
            mbMaster_Structuring_Request();
            USR_UART_Transmit_DMA(&MBM_UARTx_Handle);
        break;
        // 发送完成: 打开接收，开始计时
        case MBT_TX_CPLT:
            USR_UART_Receive_DMA(&MBM_UARTx_Handle);
            mbMaster_Task.timeout = MB_TIM_MAX_DELAY;
            mbMaster_Task.retry   = MB_TIM_MAX_RETRY;
        break;
        // 接收中：计时
        case MBT_RX_ING:
            if(mbMaster_Task.timeout > 0) {
                mbMaster_Task.timeout--;
            } else {
                if(mbMaster_Task.retry > 0) {
                    mbMaster_Task.retry--;
                    mbMaster_Task.status = MBT_REQUESTING; // 重试请求
                } else {
                    mbMaster_Task.status = MBT_RX_TIMEOUT;
                }
            }
        break;
        // 接收完成
        case MBT_RX_CPLT:
            // 第一步检查ID是否是本机
            if (mbMaster_Rx_Buf[0] == mbMaster.Tx.id) {
                // 拷贝数据
                mbMemCpy_Buf(mbMaster.Rx.cmdBuf, mbMaster_Rx_Buf, mbMaster_Rx_Byte);
                mbMaster.Rx.cmdByte = mbMaster_Rx_Byte;
                // 进入解析
                mbMaster_Task.status = MBT_PARSING; 
            } else {
                // 重新接收
                USR_UART_Receive_DMA(&MBM_UARTx_Handle);
            }          
        break;
        // 解析
        case MBT_PARSING:
            mbMaster_Processing_Response();
            mbMaster_Task.status = MBT_CALLBACK;
        break;
        // 处理回调任务
        case MBT_CALLBACK:
            mbMaster_Callback();
            mbMaster_Task.status = MBT_IDLE;
        break;

        // 异常处理
        case MBT_TX_ERR:
        case MBT_RX_ERR:
        case MBT_TX_TIMEOUT:
        case MBT_RX_TIMEOUT:
            // 终止当前DMA操作
            HAL_UART_Abort(&MBM_UARTx_Handle);  
            mbMaster_Task.status = MBT_IDLE;
        break;
        
        default:
        break;            
    }
}

void mbMaster_Callback(void){
//    uint8_t  addr   = mbMaster.Tx.addr;
    uint16_t fxcode = mbMaster.Tx.fxcode;

    switch(fxcode){
        case READ_COILS:
        case READ_DISCRETE_INPUTS:
        case READ_HOLDING_REGISTERS:
        case READ_INPUT_REGISTERS:{
        }
        break;

        case WRITE_SINGLE_COIL:{
        }
        break;

        case WRITE_SINGLE_REGISTER:{
        }
        break;
        case WRITE_MULTIPLE_COILS:
            
        break;
        case WRITE_MULTIPLE_REGISTERS:    
        
        break;
    }
}

#endif // ENABLE_MOUBUS_MASTER

#if  ENABLE_MOUBUS_SLAVE
Modbus_Task_t mbSlave_Task;

void mbSlave_Poll(void) {
//    USR_UART_Print_STA();
    switch (mbSlave_Task.status) {
        // 初始化并打开串口空闲接收
        case MBT_IDLE:
            USR_UART_Receive_DMA(&MBS_UARTx_Handle);
        break;

        case MBT_RX_ING:
            if (USR_UART_Unstable(&MBS_UARTx_Handle)) {
                mbSlave_Task.status = MBT_RX_ERR;
            }
        break;

        // 接收完成
        case MBT_RX_CPLT:
            // 第一步检查ID是否是本机
            if (mbSlave_Rx_Buf[0] == mbSlave.ID) {
                // 拷贝数据
                mbMemCpy_Buf(mbSlave.Rx.cmdBuf, mbSlave_Rx_Buf, mbSlave_Rx_Byte);
                mbSlave.Rx.cmdByte = mbSlave_Rx_Byte;
                // 打开接收
                USR_UART_Receive_DMA(&MBS_UARTx_Handle);
                // 进入解析
                mbSlave_Task.status = MBT_PARSING; 
            } else {
                // 重新接收
                mbSlave_Task.status = MBT_IDLE; 
            }
        break;

        // 解析
        case MBT_PARSING:
            mbSlave.FxStatus = mbSlave_Processing_Request();
            mbSlave_Task.status = MBT_RESPONDING;
        break;

        // 响应
        case MBT_RESPONDING:
            mbSlave_Structuring_Response();
            mbSlave_Task.status = MBT_TX_ENTRY;
        break;

        case MBT_TX_ENTRY:
            USR_UART_Transmit_DMA(&MBS_UARTx_Handle);
        break;

        // 发送完成进行回调处理
        case MBT_TX_CPLT:
            mbSlave_Callback();
            mbSlave_Init();
        break;

        case MBT_RX_ERR:
            // 清空状态，重新进入空闲
            USR_UART_Repair(&MBS_UARTx_Handle);
            mbSlave_Task.status = MBT_IDLE;
        break;
        
        default:
        break;            
    }
}

void mbSlave_Callback(void){
    uint8_t  addr   = mbSlave.Tx.addr;
    uint16_t fxcode = mbSlave.Tx.fxcode;

    switch(fxcode){
        case READ_COILS:
        case READ_DISCRETE_INPUTS:
        case READ_HOLDING_REGISTERS:
        case READ_INPUT_REGISTERS:{
            // USR_TASK
        }
        break;

        case WRITE_SINGLE_COIL:{
            bool bit = 0;
            mbSlave_Read_Coils(addr, &bit, 1);
            // USR_TASK
//            EM_mbSlave_Write_Coil_Callback(addr, bit);
//            POT_mbSlave_Write_Coil_Callback(addr, bit);
        }
        break;

        case WRITE_SINGLE_REGISTER:{
            uint16_t value = 0;
            mbSlave_Read_Holdings(addr, &value, 1);
//            SEGGER_RTT_printf(0, "MBT_CALLBACK addr %04d, value %d\n", addr, value);
            // USR_TASK
//            EM_mbSlave_Write_Reg_Callback(addr, value);
//            POT_mbSlave_Write_Reg_Callback(addr, value);
        }
        break;
        case WRITE_MULTIPLE_COILS:
            
        break;
        case WRITE_MULTIPLE_REGISTERS:    
        
        break;
    }
}

#endif // ENABLE_MOUBUS_SLAVE
