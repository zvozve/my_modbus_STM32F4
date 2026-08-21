/**
 * @file    bsp_gpio_drv.h
 * @brief   BSP GPIO 抽象层 + 中断回调注册
 * @version V1.1
 * @date    2026-08-12
 */

#ifndef __BSP_GPIO_DRV_H
#define __BSP_GPIO_DRV_H

#include <stdint.h>
#include <stdbool.h>
#include "hal_platform.h"   /* STM32 系列 HAL 统一入口（移植层） */

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 原有 GPIO 设备结构 ========== */

typedef struct {
    GPIO_TypeDef *port;
    uint16_t      pin;
    bool          active_high;
} gpio_pin_t;

typedef struct {
    void (*init)(gpio_pin_t *pin);
    void (*write)(gpio_pin_t *pin, bool state);
    void (*toggle)(gpio_pin_t *pin);
    bool (*read)(gpio_pin_t *pin);
    void (*set_high)(gpio_pin_t *pin);
    void (*set_low)(gpio_pin_t *pin);
} gpio_ops_t;

typedef struct {
    gpio_pin_t  pin;
    gpio_ops_t  ops;
    bool        is_initialized;
} gpio_dev_t;

/* ========== 中断回调类型 ========== */

/**
 * @brief   GPIO 中断回调函数类型
 * @param   pin     触发中断的引脚号
 * @param   user_data 用户数据
 */
typedef void (*gpio_irq_callback_t)(uint16_t pin, void *user_data);

/**
 * @brief   电平门忽略哨兵：注册时传此值表示不检查有效电平（每个边沿都触发）
 * @note    GPIO_PIN_RESET=0 / GPIO_PIN_SET=1，故用 0xFF 作哨兵
 */
#define BSP_GPIO_IRQ_LEVEL_ANY    0xFF

/**
 * @brief   中断回调注册项
 * @note    防抖窗口 + 有效电平门在 bsp_gpio_irq_dispatch 内统一处理，
 *          调用方只需在注册时传入 debounce_us / active_level。
 */
typedef struct {
    GPIO_TypeDef           *port;          // 引脚所在端口（电平门回读用；NULL=跳过电平门）
    uint16_t                pin;           // 引脚号
    gpio_irq_callback_t     callback;      // 回调函数
    void                   *user_data;     // 用户数据
    bool                    enabled;       // 是否启用
    uint32_t                debounce_us;   // 防抖窗口(us)，0=不防抖
    uint8_t                 active_level;  // 有效电平(GPIO_PIN_SET/RESET)；BSP_GPIO_IRQ_LEVEL_ANY=不检查
    uint32_t                last_active;   // 上次有效触发时的 DWT 计数值（防抖基准）
} gpio_irq_reg_t;

/* ========== GPIO 设备 API ========== */

bool bsp_gpio_init(gpio_dev_t *dev, GPIO_TypeDef *port, uint16_t pin, bool active_high);
bool bsp_gpio_init_default(gpio_dev_t *dev, GPIO_TypeDef *port, uint16_t pin);

/**
 * @brief   输入模式初始化：GPIO_MODE_INPUT + GPIO_PULLUP（不调用 set_low，
 *          避免把输入脚拉成输出）。读沿用 bsp_gpio_read 的 active_high 反相。
 * @note    RCC 时钟由调用方使能（与输出路径一致：bsp_gpio 不负责 RCC）。
 * @retval  true: 成功  false: 失败
 */
bool bsp_gpio_init_input(gpio_dev_t *dev, GPIO_TypeDef *port, uint16_t pin, bool active_high);

void bsp_gpio_register_ops(gpio_dev_t *dev, gpio_ops_t *ops);

void bsp_gpio_set_high(gpio_dev_t *dev);
void bsp_gpio_set_low(gpio_dev_t *dev);
void bsp_gpio_toggle(gpio_dev_t *dev);
void bsp_gpio_write(gpio_dev_t *dev, bool state);
bool bsp_gpio_read(gpio_dev_t *dev);
void bsp_gpio_set_high_raw(gpio_dev_t *dev);
void bsp_gpio_set_low_raw(gpio_dev_t *dev);
bool bsp_gpio_is_initialized(gpio_dev_t *dev);

/* ========== 中断回调注册 API ========== */

/**
 * @brief   注册 GPIO 中断回调（内置有效电平门 + 防抖）
 * @param   port          引脚所在端口（电平门回读用；传 NULL 则跳过电平门）
 * @param   pin           引脚号
 * @param   callback      回调函数
 * @param   user_data     用户数据
 * @param   debounce_us   防抖窗口(us)，0=不防抖
 * @param   active_level  有效电平(GPIO_PIN_SET/RESET)；BSP_GPIO_IRQ_LEVEL_ANY=不检查电平
 * @retval  true: 成功  false: 失败
 * @note    电平门把“双沿 EXTI”收敛成“单沿逻辑触发”：硬件配成 RISING_FALLING 时，
 *          只有引脚回读电平 == active_level 的那次边沿才会触发回调，避免一次脉冲两次触发。
 *          防抖在 ISR 内用 DWT 周期计数器计时（分辨率≈6ns），仅对越过窗口的触发更新基准。
 */
bool bsp_gpio_irq_register(GPIO_TypeDef *port, uint16_t pin,
                           gpio_irq_callback_t callback, void *user_data,
                           uint32_t debounce_us, uint8_t active_level);

/**
 * @brief   注销 GPIO 中断回调
 * @param   pin  GPIO 引脚号
 */
void bsp_gpio_irq_unregister(uint16_t pin);

/**
 * @brief   启用/禁用 GPIO 中断回调
 * @param   pin      GPIO 引脚号
 * @param   enable   true: 启用, false: 禁用
 */
void bsp_gpio_irq_enable(uint16_t pin, bool enable);

/* 中断回调分发 (供 HAL 层调用) */
void bsp_gpio_irq_dispatch(uint16_t GPIO_Pin);

#ifdef __cplusplus
}
#endif

#endif /* __BSP_GPIO_DRV_H */