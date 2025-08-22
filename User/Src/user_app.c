#include "user_app.h"
#include "modbus_app.h"
#include "stm32f4xx_hal.h"


/*
1. 使用RTU屏幕传递参数时，本机MODBUS做主机（485），使用RTU开关控制运行
2. 使用控制板传递参数时，本机MODBUS做从机（485），使用TRG1/TRG2控制运行
   多个电磁铁板子接到控制板时，需要区别不同的MB_ID

开发板TIM1 CH1-CH4 IO未引出，使用新板的时候需要变换IO
*/

void app_entry(void) {
    // Modbus初始化
    Modbus_Init();
    SEGGER_RTT_printf(0, "Modbus_Init PASS\n"); 

    // uint8_t message[] = "Hello DMA!";
    // uint16_t msg_len = 10; // "Hello DMA!" 的长度，不包括结尾的\0

    // while(1) {
    //     // 直接使用
    //     HAL_GPIO_WritePin(RS485_EN_GPIO_Port, RS485_EN_Pin, GPIO_PIN_SET) ;
    //     HAL_UART_Transmit_DMA(&huart2, message, msg_len);
    //     HAL_Delay(1000);
    // }
}
