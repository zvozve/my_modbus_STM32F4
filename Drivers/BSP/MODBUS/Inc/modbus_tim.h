#ifndef __MODBUS_TIM_H__
#define __MODBUS_TIM_H__

#include "modbus_app.h"

#define TIM_MB              TIM6
#define htim_mb             htim6
extern TIM_HandleTypeDef    htim_mb;

#define MB_TIM_PERIOD_US    1000
#define MB_TIM_MAX_DELAY    200 * 1000 / MB_TIM_PERIOD_US
#define MB_TIM_MAX_RETRY    5

void MB_TIM_Delay_us(uint16_t us);
void MB_TIM_Delay_ms(uint16_t ms);

#endif // __MODBUS_TIM_H__
