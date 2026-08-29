// ==================== app_main.h ====================
#ifndef __APP_MAIN_H
#define __APP_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "SEGGER_RTT_Log.h"   /* 复用 RTT_LOG_TAG；应用层专属标签在此定义 */
// ===========================
// 应用层日志标签
// ===========================
#ifndef APP_LOG_ENABLE
    #define APP_LOG_ENABLE     1
#endif
#define APP_LOG(fmt, ...)     RTT_LOG_TAG(APP_LOG_ENABLE,    "APP",    fmt, ##__VA_ARGS__)

void App_Init(void);
void App_Loop(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_MAIN_H */