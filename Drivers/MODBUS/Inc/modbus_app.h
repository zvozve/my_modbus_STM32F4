#ifndef __MODBUS_APP_H__
#define __MODBUS_APP_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include <math.h>

#include "user_app.h"

#define ENABLE_MOUBUS_MASTER 0
#define ENABLE_MOUBUS_SLAVE  1

#define ENABLE_MOUBUS_DEBUG  1

#define MB_BUF_LEN 1024

#include "modbus_gpio.h"
#include "modbus_uart.h"
#include "modbus_tim.h"
#include "modbus_func.h"
#include "modbus_task.h"

void Modbus_Init(void);

#if  ENABLE_MOUBUS_MASTER
void mbMaster_Init(void);
void mbMaster_Test(void);
#endif // ENABLE_MOUBUS_MASTER

#if  ENABLE_MOUBUS_SLAVE
void mbSlave_Init(void);
void mbSlave_Test(void);
#endif // ENABLE_MOUBUS_SLAVE

#endif // __MODBUS_APP_H__
