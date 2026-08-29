#ifndef __BOARD_CFG_H
#define __BOARD_CFG_H

/*
 * board_cfg.h —— 工程侧硬件绑定（类 devicetree）
 *
 * 规则：
 *   - 全工程唯一允许 include CubeMX 生成头（main.h / usart.h / tim.h）的地方
 *     就是本文件；SDK（library/）不做任何绑定。
 *   - app / tasks 只引用本文件的绑定宏，不直接引用 MX 符号
 *     （CPU_STA_GPIO_Port / ETH_RST_Pin / &huart1 / &hiwdg ...）。
 *   - 换板只改本文件（引脚、句柄、时钟），SDK 与业务代码不动。
 */

#include "main.h"
#include "usart.h"
#include "tim.h"

/* ========== 功能开关（驱动读取，决定编译哪些传输/外设） ========== */
#define BOARD_MODBUS_RTU_ENABLE   1      /* RTU 串行传输（仅依赖 UART，默认开） */
#define BOARD_MODBUS_TCP_ENABLE   1      /* TCP 传输：需要 LwIP 栈；无网口板设 0 */
#define BOARD_HEART_IWDG_ENABLE   1      /* 心跳喂狗：无独立看门狗设 0 */

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 心跳 LED + 看门狗 ========== */
#define BOARD_HEART_LED_PORT   CPU_STA_GPIO_Port
#define BOARD_HEART_LED_PIN    CPU_STA_Pin
#define BOARD_HEART_IWDG       (&hiwdg)        /* 无 IWDG 传 NULL */

/* ========== LAN8720A PHY 复位 ========== */
#define BOARD_ETH_RST_PORT     ETH_RST_GPIO_Port
#define BOARD_ETH_RST_PIN      ETH_RST_Pin

/* ========== Modbus RTU UART 句柄 ========== */
#define BOARD_UART1            (&huart1)       /* 按任务分配：RTU 主机/从机 */
#define BOARD_UART2            (&huart2)

#ifdef __cplusplus
}
#endif

#endif /* __BOARD_CFG_H */
