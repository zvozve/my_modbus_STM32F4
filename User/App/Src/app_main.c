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

// ============================================
// 任务开关（编辑此处 0/1 或用 CMake -D 覆盖）
// ============================================
#ifndef APP_TASK_MB_RTU_SLAVE
#define APP_TASK_MB_RTU_SLAVE     0   /* RTU 从机 (UART2) */
#endif
#ifndef APP_TASK_MB_RTU_MASTER
#define APP_TASK_MB_RTU_MASTER    0   /* RTU 主机 (UART1) */
#endif
#ifndef APP_TASK_MB_TCP_SERVER
#define APP_TASK_MB_TCP_SERVER    0   /* TCP 服务器 (:502) */
#endif
#ifndef APP_TASK_MB_TCP_CLIENT
#define APP_TASK_MB_TCP_CLIENT    1   /* TCP 客户端（多实例 master） */
#endif

// Tasks（按开关条件包含）
#if APP_TASK_MB_RTU_SLAVE
#include "task_mb_rtu_slave.h"
#endif
#if APP_TASK_MB_RTU_MASTER
#include "task_mb_rtu_master.h"
#endif
#if APP_TASK_MB_TCP_SERVER
#include "task_mb_tcp_server.h"
#endif
#if APP_TASK_MB_TCP_CLIENT
#include "task_mb_tcp_client.h"
#endif

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
#if APP_TASK_MB_RTU_SLAVE
    TaskModbus_Init();      // 从机 UART2
#endif
#if APP_TASK_MB_RTU_MASTER
    TaskModbus_M_Init();    // 主机 UART1
#endif
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
#if APP_TASK_MB_RTU_SLAVE
        TaskModbus_Process();
#endif
#if APP_TASK_MB_RTU_MASTER
        TaskModbus_M_Process();
#endif

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
#if APP_TASK_MB_RTU_SLAVE
    TaskModbus_Init();          // RTU 从机 UART2
#endif
#if APP_TASK_MB_RTU_MASTER
    TaskModbus_M_Init();        // RTU 主机 UART1
#endif
    heart_beat_init();
#if APP_TASK_MB_TCP_SERVER
    TaskModbus_TCP_Init();      // Modbus TCP 从机 :502
#endif
#if APP_TASK_MB_TCP_CLIENT
    TaskModbus_TCP_Client_Init();  // Modbus TCP 主机（client）多实例测试
#endif

    // RTOS循环
    while (1) {
#if APP_TASK_MB_RTU_SLAVE
        TaskModbus_Process();
#endif
#if APP_TASK_MB_RTU_MASTER
        TaskModbus_M_Process();
#endif
#if APP_TASK_MB_TCP_SERVER
        TaskModbus_TCP_Process();
#endif
#if APP_TASK_MB_TCP_CLIENT
        TaskModbus_TCP_Client_Process();
#endif

        static uint32_t last_feed_time = 0;
        uint32_t current_time = xTaskGetTickCount();
        if ((current_time - last_feed_time) >= pdMS_TO_TICKS(200)) {
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
        1024,                   // 任务栈大小（LwIP + RTT 调用链较深，加大防溢出）
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
