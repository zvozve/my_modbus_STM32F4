
#include "heart_beat.h"
#include "gpio.h"
#include "iwdg.h"
#include "SEGGER_RTT_Log.h"

void heart_beat_init(void)
{
    MX_IWDG_Init();
    SYS_LOG("heart_beat_init(iwdg) done");
}

void heart_beat_run(void)
{
    HAL_GPIO_TogglePin(CPU_STA_GPIO_Port, CPU_STA_Pin);
    HAL_IWDG_Refresh(&hiwdg);
    SYS_LOG("heart_beat");
}
