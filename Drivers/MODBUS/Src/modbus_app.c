#include "modbus_app.h"

// INIT
void Modbus_Init(void) {
    #if  ENABLE_MOUBUS_MASTER
    mbMaster_Init();
    mbMaster_Task.status  = MBT_IDLE;
    #endif // ENABLE_MOUBUS_MASTER

    #if  ENABLE_MOUBUS_SLAVE
    mbSlave.ID = MBS_SLAVE_ID;
    mbSlave_Init();
    #endif // ENABLE_MOUBUS_SLAVE

    // Æô¶¯¶¨Ê±Æ÷ÂÖÑ¯ 10us
    HAL_TIM_Base_Start_IT(&MBG_TIMx_Handle);
    
    #if  ENABLE_MOUBUS_MASTER
    mbMaster_Test();
    #endif // ENABLE_MOUBUS_MASTER

    #if  ENABLE_MOUBUS_SLAVE
    mbSlave_Test();
    #endif // ENABLE_MOUBUS_SLAVE
}

#if  ENABLE_MOUBUS_MASTER 
void mbMaster_Init(void){
    mbMemClr_Cmd(&mbMaster);
}

void mbMaster_Test(void){
    // ÂÖÑ¯¶Á0-9
    #if 1  
    uint16_t data[100] = {0};
    while(1) {
        mbMaster_Read_Holdings(1, 0, data, 100);

        HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
        HAL_Delay(500);

        SEGGER_RTT_printf(0, "mbMaster_Read_Holdings After\n");
        for(uint16_t i = 0;i < 20; i++){
            SEGGER_RTT_printf(0, "%5d ", data[i]);
        }
        SEGGER_RTT_printf(0, "\n");
    }
    #endif 
    
    // ÂÖÑ¯Ð´0-9
    #if 0
    uint16_t data[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    while(1) {
        SEGGER_RTT_printf(0, "mbMaster_Write_Multiple_Regs(1, 0, data, 10)\n");
        for(uint16_t i = 0;i < 10; i++){
            data[i] ++;
        }
        mbMaster_Write_Multiple_Regs(1, 0, data, 10);
        HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin);
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

#endif // ENABLE_MOUBUS_MASTER

#if  ENABLE_MOUBUS_SLAVE

void mbSlave_Init(void){
    mbMemClr_Cmd(&mbSlave);
    mbSlave.FxStatus  = MB_NO_ERR;
    mbSlave_Task.status = MBT_IDLE;
}

void mbSlave_Test(void){
    uint16_t j = 0;
//    bool coils[10];
    while (1) {
        j++;
        mbSlave_Write_Single_Reg(0, j);

//        mbSlave_Read_Coils(1, coils, 10);
//        for (uint16_t j = 0; j < 10; j++) {
//            SEGGER_RTT_printf(0, "%4d ", mbSlave_Coils_Buf[j]);
//        }
//        SEGGER_RTT_printf(0, "\n");
//        for (uint16_t j = 0; j < 10; j++) {
//            SEGGER_RTT_printf(0, "%4d ", coils[j]);
//        }
//        SEGGER_RTT_printf(0, "\n");
//        SEGGER_RTT_printf(0, "TEST FINISH\n");
        HAL_Delay(1000);
    }
}
#endif // ENABLE_MOUBUS_SLAVE
