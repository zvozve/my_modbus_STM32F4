/**
 * @file    SEGGER_RTT_Log.h
 * @brief   SEGGER RTT 日志中间件 - 带时间戳的彩色日志输出
 * @version V2.0
 * @date    2026-08-04
 * 
 * @note    功能说明：
 *          1. 基于 SEGGER RTT 的日志输出中间件
 *          2. 自动添加时间戳 [HH:MM:SS.mmm]
 *          3. 支持多标签分类日志（SYS/ERR/WARN/INFO/DBG/APP/UART/MODBUS等）
 *          4. 支持 HEX 格式数据打印
 *          5. 支持裸机和 RTOS 两种模式（通过 RTT_USE_RTOS 切换）
 *          6. 每个标签独立开关控制（xxx_LOG_ENABLE）
 *          7. 总开关 RTT_LOG_ENABLE 可彻底禁用所有日志
 * 
 * @note    使用示例：
 *          SYS_LOG("System init done\n");
 *          ERR_LOG("Error: %d\n", err_code);
 *          HEX_LOG("TX: ", tx_buf, tx_len);
 * 
 * @note    配置说明：
 *          - RTT_USE_RTOS : 0=裸机, 1=RTOS
 *          - RTT_LOG_ENABLE : 0=禁用所有日志, 1=启用
 *          - RTT_LOG_MAX_HOURS : 时间戳小时归零阈值（默认24小时）
 *          - xxx_LOG_ENABLE : 各标签独立开关
 */
 
#ifndef __SEGGER_RTT_LOG_H
#define __SEGGER_RTT_LOG_H

#define RTT_USE_RTOS       0    // 0: 裸机, 1: RTOS

#include <stdio.h>
#include <stdint.h>

#if RTT_USE_RTOS
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#endif

#if RTT_USE_RTOS
    #define RTT_GET_TICK()      xTaskGetTickCount()
    #define RTT_Delay_ms(ms)    vTaskDelay(pdMS_TO_TICKS(ms))
#else
    #define RTT_GET_TICK()      HAL_GetTick()
    #define RTT_Delay_ms(ms)    HAL_Delay(ms)
#endif

// ===========================
// 总开关：设为 0 则彻底禁用所有日志
// ===========================
#ifndef RTT_LOG_ENABLE
    #define RTT_LOG_ENABLE     1
#endif

// ===========================
// 时间戳配置
// ===========================
#ifndef RTT_LOG_MAX_HOURS
    #define RTT_LOG_MAX_HOURS  24    // 最大小时数，超过后归零
#endif

// ===========================
// 日志引擎
// ===========================
#if RTT_LOG_ENABLE
    #include "SEGGER_RTT.h"

    // 时间戳格式化函数（支持小时溢出归零）
    static inline void RTT_log_format_time(uint32_t tick, char *buf, size_t buf_size) {
        uint32_t ms = tick;
        uint32_t hours = ms / 3600000;
        ms -= hours * 3600000;
        
        // 小时超过最大值时归零
        if (hours >= RTT_LOG_MAX_HOURS) {
            hours = hours % RTT_LOG_MAX_HOURS;
        }
        
        uint32_t minutes = ms / 60000;
        ms -= minutes * 60000;
        uint32_t seconds = ms / 1000;
        ms -= seconds * 1000;

        snprintf(buf, buf_size, "%02lu:%02lu:%02lu.%03lu", hours, minutes, seconds, ms);
    }

    // 带标签和时间戳的日志输出（自动换行）
    #define RTT_LOG_EMIT(tag, fmt, ...)                                          \
        do {                                                                     \
            char _time_buf[16];                                                  \
            RTT_log_format_time(RTT_GET_TICK(), _time_buf, sizeof(_time_buf));   \
            SEGGER_RTT_printf(0, "[%s][%s] " fmt "\r\n", _time_buf, tag, ##__VA_ARGS__); \
        } while (0)

    // 标签日志引擎
    #define RTT_LOG_TAG(enable, tag, fmt, ...)                     \
        do {                                                       \
            if (enable) { RTT_LOG_EMIT(tag, fmt, ##__VA_ARGS__); } \
        } while (0)

    // HEX日志内部实现（带时间戳和[HEX]标签，支持自定义前缀，一行显示，自动换行）
    #define HEX_PRINT(enable, prefix, data, len)                                   \
        do {                                                                       \
            if (enable) {                                                          \
                char _time_buf[16];                                                \
                RTT_log_format_time(RTT_GET_TICK(), _time_buf, sizeof(_time_buf)); \
                SEGGER_RTT_printf(0, "[%s][HEX] %s", _time_buf, prefix);           \
                for (size_t _i = 0; _i < (len); _i++) {                            \
                    SEGGER_RTT_printf(0, "%02X ", ((uint8_t*)(data))[_i]);         \
                }                                                                  \
                SEGGER_RTT_printf(0, "\r\n");                                      \
            }                                                                      \
        } while (0)

#else
    #define RTT_LOG_TAG(enable, tag, fmt, ...)   ((void)0)
    #define HEX_PRINT(enable, prefix, data, len) ((void)0)
#endif

// ===========================
// 标签日志定义（带时间戳，自动换行）
// ===========================
#define RTT_LOG(fmt, ...)     RTT_LOG_TAG(1, "RTT", fmt, ##__VA_ARGS__)

#ifndef SYS_LOG_ENABLE
    #define SYS_LOG_ENABLE     1
    #define SYS_LOG(fmt, ...)     RTT_LOG_TAG(SYS_LOG_ENABLE,    "SYS",    fmt, ##__VA_ARGS__)
#endif
#ifndef ERR_LOG_ENABLE
    #define ERR_LOG_ENABLE     1
    #define ERR_LOG(fmt, ...)     RTT_LOG_TAG(ERR_LOG_ENABLE,    "ERR",    fmt, ##__VA_ARGS__)
#endif
#ifndef WARN_LOG_ENABLE
    #define WARN_LOG_ENABLE    1
    #define WARN_LOG(fmt, ...)    RTT_LOG_TAG(WARN_LOG_ENABLE,   "WARN",   fmt, ##__VA_ARGS__)
#endif
#ifndef INFO_LOG_ENABLE
    #define INFO_LOG_ENABLE    1
    #define INFO_LOG(fmt, ...)    RTT_LOG_TAG(INFO_LOG_ENABLE,   "INFO",   fmt, ##__VA_ARGS__)
#endif
#ifndef DBG_LOG_ENABLE
    #define DBG_LOG_ENABLE     1
    #define DBG_LOG(fmt, ...)     RTT_LOG_TAG(DBG_LOG_ENABLE,    "DBG",    fmt, ##__VA_ARGS__)
#endif

#ifndef APP_LOG_ENABLE
    #define APP_LOG_ENABLE     1
    #define APP_LOG(fmt, ...)     RTT_LOG_TAG(APP_LOG_ENABLE,    "APP",    fmt, ##__VA_ARGS__)
#endif
#ifndef MODBUS_LOG_ENABLE
    #define MODBUS_LOG_ENABLE  1
    #define MODBUS_LOG(fmt, ...)  RTT_LOG_TAG(MODBUS_LOG_ENABLE, "MODBUS", fmt, ##__VA_ARGS__)
#endif
#ifndef LAN_LOG_ENABLE
    #define LAN_LOG_ENABLE     1
    #define LAN_LOG(fmt, ...)     RTT_LOG_TAG(LAN_LOG_ENABLE,    "LAN",    fmt, ##__VA_ARGS__)
#endif
#ifndef UART_LOG_ENABLE
    #define UART_LOG_ENABLE     1
    #define UART_LOG(fmt, ...)     RTT_LOG_TAG(UART_LOG_ENABLE,    "UART",    fmt, ##__VA_ARGS__)
#endif
#ifndef EUSB_LOG_ENABLE
    #define EUSB_LOG_ENABLE     1
    #define EUSB_LOG(fmt, ...)     RTT_LOG_TAG(EUSB_LOG_ENABLE,    "EUSB",    fmt, ##__VA_ARGS__)
#endif
#ifndef EUART_LOG_ENABLE
    #define EUART_LOG_ENABLE     1
    #define EUART_LOG(fmt, ...)     RTT_LOG_TAG(EUART_LOG_ENABLE,    "EUART",    fmt, ##__VA_ARGS__)
#endif

// ===========================
// HEX日志定义（默认启用）
// ===========================
#ifndef HEX_LOG_ENABLE
    #define HEX_LOG_ENABLE      1
    #define HEX_LOG(prefix, data, len) HEX_PRINT(HEX_LOG_ENABLE, prefix, data, len)
#endif

#endif // __SEGGER_RTT_LOG_H