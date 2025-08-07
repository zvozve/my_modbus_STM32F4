#include "modbus_tim.h"

volatile uint32_t us_count = 0; // ���ж��е����ļ�����

void MB_TIM_Delay_us(uint16_t us) {
    uint32_t start = us_count;
    int32_t ticks = us / MB_TIM_PERIOD_US; // ��Ҫ�ȴ���ticks��
    while ((int32_t)(us_count - start) < ticks); // ��ȷ�������
}

// ���뼶��ʱ
void MB_TIM_Delay_ms(uint16_t ms)
{
    for (uint16_t i = 0; i < ms; i++)
    {
        MB_TIM_Delay_us(1000);  // 1ms = 1000us
    }
}

void TIM6_DAC_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&MBG_TIMx_Handle);
    us_count ++;
    #if ENABLE_MOUBUS_MASTER  
    mbMaster_Poll();
    #endif // ENABLE_MOUBUS_MASTER
    
    #if  ENABLE_MOUBUS_SLAVE
    mbSlave_Poll();
    #endif // ENABLE_MOUBUS_SLAVE
}

