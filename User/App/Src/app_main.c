#include "app_main.h"

// Common

// BSP
#include "bsp_dwt.h"

// Hardware
#include "heart_beat.h"
#include "lan8720a_reset.h"

// Middleware
#include "SEGGER_RTT_Log.h"

// Protocols

// Tasks
#include "task_modbus.h"
#include "task_modbus_m.h"

// ============================================
// RTOS支持
// ============================================
#define APP_USE_RTOS    1

#ifdef APP_USE_RTOS
    #include "FreeRTOS.h"
    #include "task.h"
    #include "cmsis_os.h"
    
    // 入口任务句柄
    static TaskHandle_t xAppTaskHandle = NULL;
    
    // 入口任务函数
    static void vAppTask(void *pvParameters);
#endif

// ============================================
// 裸机模式初始化
// ============================================
#ifndef APP_USE_RTOS
static void App_BareMetal_Init(void) {
    // TaskModbus_Init();      // 从机 UART2
    TaskModbus_M_Init();    // 主机 UART1
    heart_beat_init();
    App_BareMetal_Loop();
}

static void App_BareMetal_Loop(void) {
    static uint32_t last_heartbeat = 0;

    while (1) {
        uint32_t now = bsp_GetCycleCount();

        // 心跳处理（500ms）
        if (bsp_IsTimeout(last_heartbeat, 500000)) {
            last_heartbeat = now;
            heart_beat_run();
        }

        // Modbus处理（每个循环都执行）
        // TaskModbus_Process();
        TaskModbus_M_Process();

        bsp_DelayUS(1000);
    }
}
#endif

// ============================================
// RTOS入口任务
// ============================================
#ifdef APP_USE_RTOS

static void vAppTask(void *pvParameters) {
    // 初始化外设
    // TaskModbus_Init();      // 从机 UART2
    TaskModbus_M_Init();    // 主机 UART1
    heart_beat_init();
    
    // RTOS循环
    while (1) {
        // TaskModbus_Process();
        TaskModbus_M_Process();

        static uint32_t last_feed_time = 0;
        uint32_t current_time = xTaskGetTickCount();
        if ((current_time - last_feed_time) >= pdMS_TO_TICKS(500)) {
            last_feed_time = current_time;
            heart_beat_run();
        }
        
        vTaskDelay(pdMS_TO_TICKS(1));  // 1ms周期
    }
}

static void App_RTOS_CreateTask(void) {
    xTaskCreate(
        vAppTask,               // 任务函数
        "AppTask",              // 任务名称
        512,                    // 任务栈大小
        NULL,                   // 任务参数
        osPriorityNormal,       // 任务优先级
        &xAppTaskHandle        // 任务句柄
    );
}

#endif

void App_Init(void) {
    // 基础硬件初始化
    ETH_RST_Init();
    bsp_InitDWT();
    ETH_RST_Execute();
    
#ifdef APP_USE_RTOS
    // RTOS模式
    App_RTOS_CreateTask();
#else
    // 裸机模式
    App_BareMetal_Init();
    
#endif
}

// ============================================
// 应用循环（兼容接口）
// ============================================
void App_Loop(void) {
#ifdef APP_USE_RTOS
    // RTOS模式：由调度器管理，不应到达
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
#else
    // 裸机模式：由App_Init调用，此处不会执行
    // 保留此函数作为兼容接口
    App_BareMetal_Loop();
#endif
}