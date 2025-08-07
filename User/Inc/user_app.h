#ifndef __USER_APP_H__
#define __USER_APP_H__

// st_lib
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

// project_driver
#include "main.h"
#include "dma.h"
#include "tim.h"
#include "gpio.h"
#include "usart.h"

#include "modbus_app.h"
#include "SEGGER_RTT.h"

// user_driver
//#include "user_gpio.h"

void app_entry(void);

#endif // __USER_APP_H__
