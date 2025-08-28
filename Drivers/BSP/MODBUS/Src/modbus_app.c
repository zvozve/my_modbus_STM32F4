#include "modbus_app.h"

// INIT
void Modbus_Init(void) {
    #if  ENABLE_MB_RTU_MASTER
    mbMaster_Init();
    mbMaster_Task.status  = MBT_IDLE;
    #endif // ENABLE_MB_RTU_MASTER

    #if  ENABLE_MB_RTU_SLAVE
    mbSlave.ID = MBS_SLAVE_ID;
    mbSlave_Init();
    #endif // ENABLE_MB_RTU_SLAVE

    #if ENABLE_MOUBUS_DMACPY
    HAL_DMA_RegisterCallback(&hdma_mb_cpy, HAL_DMA_XFER_CPLT_CB_ID, mbSlave_FIFO_Write_Regs_DMA_Callback);
    #endif // ENABLE_MOUBUS_DMACPY
    
    // 启动定时器轮询 10us
    HAL_TIM_Base_Start_IT(&htim_mb);
    
    #if  ENABLE_MB_RTU_MASTER
    mbMaster_Test();
    #endif // ENABLE_MB_RTU_MASTER

    #if  ENABLE_MB_RTU_SLAVE
//    mbSlave_Test();
    #endif // ENABLE_MB_RTU_SLAVE
}

#if  ENABLE_MB_RTU_MASTER 

void mbMaster_Init(void){
    mbMaster_cmd_clr();
}

void mbMaster_Test(void){
    // 轮询读0-9
    #if 1  
    uint16_t data[100] = {0};
    while(1) {
        mbMaster_Read_Holdings(1, 0, data, 100);

//        HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
        HAL_Delay(1000);

        SEGGER_RTT_printf(0, "mbMaster_Read_Holdings After\n");
        for(uint16_t i = 0;i < 20; i++){
            SEGGER_RTT_printf(0, "%5d ", data[i]);
        }
        SEGGER_RTT_printf(0, "\n");
    }
    #endif 
    
    // 轮询写0-9
    #if 0
    uint16_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    while(1) {
        SEGGER_RTT_printf(0, "mbMaster_Write_Multiple_Regs(1, 0, data, 10)\n");
        for(uint16_t i = 0;i < 10; i++){
            data[i] ++;
        }
        mbMaster_Write_Multiple_Regs(1, 0, data, 10);
//        HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
        HAL_Delay(100);
    }
    #endif 

//    bool aaa = false;
//    bool bbb[10] = {1,0,1,0,1,0,1,0,1,0};
//    uint16_t ccc = 0;
//    uint16_t ddd[10] = {1,2,3,4,5,6,7,8,8,10};
    
//    while (1) {
//        mbMaster_Write_Single_Coil(1, 1, aaa%2);
//        HAL_Delay(250);
        
//        for(uint16_t i = 0;i < 10; i++){
//            bbb[i] = !bbb[i];
//        }
//        mbMaster_Write_Multiple_Coils(1, 0, bbb, 10);
//        HAL_Delay(250);
        
//        ccc++;
//        mbMaster_Write_Single_Reg(1, 0, ccc);
//        HAL_Delay(250);
        
//        for(uint16_t i = 0;i < 10; i++){
//            ddd[i] ++;
//        }
//        mbMaster_Write_Multiple_Regs(1, 0, ddd, 10);
//        HAL_Delay(1000);
        
//        mbMaster_Read_Holdings(1, 1, ddd, 10);
//        
//        HAL_Delay(1000);
//        
//        SEGGER_RTT_printf(0, "TEST FINISH\n");
//    }
}

void mbMaster_Poll(void) {
    MBT_UART_Print_STA("mbMaster_Poll ", mbMaster_Task.status, mbMaster.FxStatus);
    switch (mbMaster_Task.status) {
        // 静态
        case MBT_IDLE:
        break;
        // 构建请求，发送
        case MBT_REQUESTING:
            mbMaster_Structuring_Request();
            mbMemCpy_Buf(mbMaster_Tx_Buf, mbMaster.Tx.cmdBuf, mbMaster.Tx.cmdByte);
            mbMaster_Tx_Byte = mbMaster.Tx.cmdByte;
            MBT_UART_Transmit_DMA(&huart_mbm);
        break;
        // 发送完成: 打开接收，开始计时
        case MBT_TX_CPLT:
            MBT_UART_Receive_DMA(&huart_mbm);
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
                MBT_UART_Receive_DMA(&huart_mbm);
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
            HAL_UART_Abort(&huart_mbm);  
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

#endif // ENABLE_MB_RTU_MASTER

#if  ENABLE_MB_RTU_SLAVE

void mbSlave_Init(void){
    mbSlave_cmd_clr();
    mbSlave.FxStatus  = MB_NO_ERR;
    mbSlave_Task.status = MBT_IDLE;
}

void mbSlave_Test(void){
    // uint16_t j = 0;
    // bool coils[10];
    // while (1) {
    // j++;
    // mbSlave_Write_Single_Reg(0, j);

    // mbSlave_Read_Coils(1, coils, 10);
    // for (uint16_t j = 0; j < 10; j++) {
    //     SEGGER_RTT_printf(0, "%4d ", mbSlave_Coils_Buf[j]);
    // }
    // SEGGER_RTT_printf(0, "\n");
    // for (uint16_t j = 0; j < 10; j++) {
    //     SEGGER_RTT_printf(0, "%4d ", coils[j]);
    // }
    // SEGGER_RTT_printf(0, "\n");
    // SEGGER_RTT_printf(0, "TEST FINISH\n");
    //     HAL_Delay(1000);
    // }
}

void mbSlave_Poll(void) {
    MBT_UART_Print_STA("mbSlave_Poll ", mbSlave_Task.status, mbSlave.FxStatus);
    switch (mbSlave_Task.status) {
        // 初始化并打开串口空闲接收
        case MBT_IDLE:
            MBT_UART_Receive_DMA(&huart_mbs);
        break;

        case MBT_RX_ING:
            if (MBT_UART_Unstable(&huart_mbs)) {
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
                MBT_UART_Receive_DMA(&huart_mbs);
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
            mbMemCpy_Buf(mbSlave_Tx_Buf, mbSlave.Tx.cmdBuf, mbSlave.Tx.cmdByte);
            mbSlave_Tx_Byte = mbSlave.Tx.cmdByte;
            MBT_UART_Transmit_DMA(&huart_mbs);
        break;

        // 发送完成进行回调处理
        case MBT_TX_CPLT:
            mbSlave_Callback();
            mbSlave_Init();
        break;

        case MBT_RX_ERR:
            // 清空状态，重新进入空闲
            MBT_UART_Repair(&huart_mbs);
            mbSlave_Task.status = MBT_IDLE;
        break;
        
        default:
        break;            
    }
}

void mbSlave_Callback(void){
    // uint8_t  addr   = mbSlave.Tx.addr;
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
            // USR_TASK

        }
        break;

        case WRITE_SINGLE_REGISTER:{
            // USR_TASK


        }
        break;
        
        case WRITE_MULTIPLE_COILS:
        break;
        
        case WRITE_MULTIPLE_REGISTERS:    
        break;
        
        default:
        return;
    }
}

#endif // ENABLE_MB_RTU_SLAVE

void MBT_UART_Print_STA(const char *sFormat, mbTask_Status_t mbtStatus, mbFxStatus_t fxStatus) {
    #if ENABLE_MOUBUS_DEBUG
    // 先输出格式化的前缀信息
    SEGGER_RTT_WriteString(0, sFormat);

    SEGGER_RTT_printf(0, "mbTask_Status_t: %02X, mbFxStatus_t %02X\n", mbtStatus, fxStatus);    
    #endif // ENABLE_MOUBUS_DEBUG                       
}

// 中断处理函数
void TIM6_DAC_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim_mb);
    us_count ++;
    // SEGGER_RTT_printf(0, "TIM6_DAC_IRQHandler %d\n", us_count); 
    #if ENABLE_MB_RTU_MASTER  
    mbMaster_Poll();
    #endif // ENABLE_MB_RTU_MASTER
    
    #if  ENABLE_MB_RTU_SLAVE
    mbSlave_Poll();
    #endif // ENABLE_MB_RTU_SLAVE
    
    #if USE_EM_SIMU

    #endif // USE_EM_SIMU
}

void USART2_IRQHandler(void) {
    #if ENABLE_MB_RTU_MASTER
    HAL_UART_IRQHandler(&huart_mbm);
    #endif // ENABLE_MB_RTU_MASTER
}

void USART3_IRQHandler(void) {
    #if ENABLE_MB_RTU_SLAVE  
    HAL_UART_IRQHandler(&huart_mbs);
    #endif // ENABLE_MB_RTU_SLAVE
}

// 回调函数
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    MBT_UART_TxCpltCallback(huart);
}

// 接收完成回调
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    USR_UARTEx_RxEventCallback(huart, Size);
}

