#include "modbus_tim.h"
#include "modbus_task.h"
#include "SEGGER_RTT.h"

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

void TIM6_DAC_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim_mb);
    us_count ++;
    // SEGGER_RTT_printf(0, "TIM6_DAC_IRQHandler %d\n", us_count); 
    #if ENABLE_MOUBUS_MASTER  
    mbMaster_Poll();
    #endif // ENABLE_MOUBUS_MASTER
    
    #if  ENABLE_MOUBUS_SLAVE
    mbSlave_Poll();
    #endif // ENABLE_MOUBUS_SLAVE
    
    #if USE_EM_SIMU

    #endif // USE_EM_SIMU
}

