#include "app_main.h"
#include "task_modbus.h"
#include "task_modbus_m.h"
#include "heart_beat.h"
#include "bsp_dwt.h"
#include "SEGGER_RTT_Log.h"

void App_Init(void) {
    bsp_InitDWT();
    heart_beat_init();

    // TaskModbus_Init();      // 从机 UART2
    TaskModbus_M_Init();    // 主机 UART1

    App_Loop();
}

void App_Loop(void) {
    static uint32_t last_heartbeat = 0;

    while (1) {
        uint32_t now = bsp_GetCycleCount();

        if (bsp_IsTimeout(last_heartbeat, 500000)) {
            last_heartbeat = now;
            heart_beat_run();
        }

        // TaskModbus_Process();
        TaskModbus_M_Process();

        bsp_DelayUS(1000);
    }
}