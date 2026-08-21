/**
 * @file    bsp_gpio_drv.c
 * @brief   BSP GPIO 抽象层 + 中断回调注册实现
 */

#include "bsp_gpio_drv.h"
#include "bsp_dwt.h"            /* bsp_GetCycleCount / bsp_GetElapsedUS：防抖窗口计时 */
#include "SEGGER_RTT_Log.h"

/* ========== 默认操作函数 ========== */

static void default_gpio_init(gpio_pin_t *pin) {
    if (pin == NULL || pin->port == NULL) return;
    
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin->pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(pin->port, &GPIO_InitStruct);
}

static void default_gpio_write(gpio_pin_t *pin, bool state) {
    if (pin == NULL || pin->port == NULL) return;
    
    GPIO_PinState hal_state;
    if (pin->active_high) {
        hal_state = state ? GPIO_PIN_SET : GPIO_PIN_RESET;
    } else {
        hal_state = state ? GPIO_PIN_RESET : GPIO_PIN_SET;
    }
    HAL_GPIO_WritePin(pin->port, pin->pin, hal_state);
}

static void default_gpio_toggle(gpio_pin_t *pin) {
    if (pin == NULL || pin->port == NULL) return;
    HAL_GPIO_TogglePin(pin->port, pin->pin);
}

static bool default_gpio_read(gpio_pin_t *pin) {
    if (pin == NULL || pin->port == NULL) return false;
    
    GPIO_PinState state = HAL_GPIO_ReadPin(pin->port, pin->pin);
    if (pin->active_high) {
        return (state == GPIO_PIN_SET);
    } else {
        return (state == GPIO_PIN_RESET);
    }
}

static void default_gpio_set_high(gpio_pin_t *pin) {
    default_gpio_write(pin, true);
}

static void default_gpio_set_low(gpio_pin_t *pin) {
    default_gpio_write(pin, false);
}

static gpio_ops_t g_default_ops = {
    .init = default_gpio_init,
    .write = default_gpio_write,
    .toggle = default_gpio_toggle,
    .read = default_gpio_read,
    .set_high = default_gpio_set_high,
    .set_low = default_gpio_set_low
};

/* ========== 中断回调注册表 ========== */

#define MAX_IRQ_REGISTERS  8

gpio_irq_reg_t g_irq_regs[MAX_IRQ_REGISTERS] = {0};

int bsp_gpio_irq_find_slot(uint16_t pin) {
    for (int i = 0; i < MAX_IRQ_REGISTERS; i++) {
        if (g_irq_regs[i].pin == pin && g_irq_regs[i].enabled) {
            return i;
        }
    }
    return -1;
}

static int bsp_gpio_irq_find_free_slot(void) {
    for (int i = 0; i < MAX_IRQ_REGISTERS; i++) {
        if (!g_irq_regs[i].enabled) {
            return i;
        }
    }
    return -1;
}

/* ========== GPIO 设备 API ========== */

bool bsp_gpio_init(gpio_dev_t *dev, GPIO_TypeDef *port, uint16_t pin, bool active_high) {
    if (dev == NULL || port == NULL) return false;
    
    dev->pin.port = port;
    dev->pin.pin = pin;
    dev->pin.active_high = active_high;
    
    if (dev->ops.init == NULL) {
        dev->ops = g_default_ops;
    }
    
    dev->ops.init(&dev->pin);
    dev->is_initialized = true;
    dev->ops.set_low(&dev->pin);
    
    DBG_LOG("GPIO initialized: port=%p, pin=0x%04X, active_high=%d", port, pin, active_high);
    return true;
}

bool bsp_gpio_init_default(gpio_dev_t *dev, GPIO_TypeDef *port, uint16_t pin) {
    return bsp_gpio_init(dev, port, pin, true);
}

/* 输入模式初始化：INPUT + PULLUP，不做 set_low（输入脚不能写） */
static void input_gpio_init(gpio_pin_t *pin) {
    if (pin == NULL || pin->port == NULL) return;

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin->pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(pin->port, &GPIO_InitStruct);
}

bool bsp_gpio_init_input(gpio_dev_t *dev, GPIO_TypeDef *port, uint16_t pin, bool active_high) {
    if (dev == NULL || port == NULL) return false;

    dev->pin.port = port;
    dev->pin.pin = pin;
    dev->pin.active_high = active_high;

    if (dev->ops.init == NULL) {
        dev->ops = g_default_ops;
    }
    dev->ops.init = input_gpio_init;   /* 覆盖为输入模式 */
    dev->ops.init(&dev->pin);
    dev->is_initialized = true;

    DBG_LOG("GPIO input initialized: port=%p, pin=0x%04X, active_high=%d",
            port, pin, active_high);
    return true;
}

void bsp_gpio_register_ops(gpio_dev_t *dev, gpio_ops_t *ops) {
    if (dev == NULL || ops == NULL) return;
    
    dev->ops.init = ops->init ? ops->init : g_default_ops.init;
    dev->ops.write = ops->write ? ops->write : g_default_ops.write;
    dev->ops.toggle = ops->toggle ? ops->toggle : g_default_ops.toggle;
    dev->ops.read = ops->read ? ops->read : g_default_ops.read;
    dev->ops.set_high = ops->set_high ? ops->set_high : g_default_ops.set_high;
    dev->ops.set_low = ops->set_low ? ops->set_low : g_default_ops.set_low;
}

void bsp_gpio_set_high(gpio_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized) return;
    dev->ops.set_high(&dev->pin);
}

void bsp_gpio_set_low(gpio_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized) return;
    dev->ops.set_low(&dev->pin);
}

void bsp_gpio_toggle(gpio_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized) return;
    dev->ops.toggle(&dev->pin);
}

void bsp_gpio_write(gpio_dev_t *dev, bool state) {
    if (dev == NULL || !dev->is_initialized) return;
    dev->ops.write(&dev->pin, state);
}

bool bsp_gpio_read(gpio_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized) return false;
    return dev->ops.read(&dev->pin);
}

void bsp_gpio_set_high_raw(gpio_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized || dev->pin.port == NULL) return;
    HAL_GPIO_WritePin(dev->pin.port, dev->pin.pin, GPIO_PIN_SET);
}

void bsp_gpio_set_low_raw(gpio_dev_t *dev) {
    if (dev == NULL || !dev->is_initialized || dev->pin.port == NULL) return;
    HAL_GPIO_WritePin(dev->pin.port, dev->pin.pin, GPIO_PIN_RESET);
}

bool bsp_gpio_is_initialized(gpio_dev_t *dev) {
    return (dev != NULL && dev->is_initialized);
}

/* ========== 中断回调注册 API ========== */

bool bsp_gpio_irq_register(GPIO_TypeDef *port, uint16_t pin,
                           gpio_irq_callback_t callback, void *user_data,
                           uint32_t debounce_us, uint8_t active_level) {
    if (callback == NULL) {
        DBG_LOG("IRQ register: callback NULL for pin 0x%04X", pin);
        return false;
    }

    int slot = bsp_gpio_irq_find_slot(pin);
    if (slot >= 0) {
        /* 已注册（enabled）的引脚：覆盖配置 */
        g_irq_regs[slot].port = port;
        g_irq_regs[slot].callback = callback;
        g_irq_regs[slot].user_data = user_data;
        g_irq_regs[slot].debounce_us = debounce_us;
        g_irq_regs[slot].active_level = active_level;
        g_irq_regs[slot].last_active = 0;
        DBG_LOG("IRQ register: updated pin 0x%04X (debounce=%uus, lvl=%d)",
                pin, (unsigned)debounce_us, active_level);
        return true;
    }

    slot = bsp_gpio_irq_find_free_slot();
    if (slot < 0) {
        DBG_LOG("IRQ register: no free slot for pin 0x%04X", pin);
        return false;
    }

    g_irq_regs[slot].port = port;
    g_irq_regs[slot].pin = pin;
    g_irq_regs[slot].callback = callback;
    g_irq_regs[slot].user_data = user_data;
    g_irq_regs[slot].enabled = true;
    g_irq_regs[slot].debounce_us = debounce_us;
    g_irq_regs[slot].active_level = active_level;
    g_irq_regs[slot].last_active = 0;

    DBG_LOG("IRQ register: pin 0x%04X registered (slot %d, debounce=%uus, lvl=%d)",
            pin, slot, (unsigned)debounce_us, active_level);
    return true;
}

void bsp_gpio_irq_unregister(uint16_t pin) {
    int slot = bsp_gpio_irq_find_slot(pin);
    if (slot < 0) return;
    
    g_irq_regs[slot].enabled = false;
    g_irq_regs[slot].pin = 0;
    g_irq_regs[slot].callback = NULL;
    g_irq_regs[slot].user_data = NULL;
    DBG_LOG("IRQ unregister: pin 0x%04X", pin);
}

void bsp_gpio_irq_enable(uint16_t pin, bool enable) {
    int slot = bsp_gpio_irq_find_slot(pin);
    if (slot < 0) return;
    g_irq_regs[slot].enabled = enable;
}

/* ========== 中断回调分发 ========== */

/**
 * @brief   分发 GPIO 中断到注册的回调
 * @param   GPIO_Pin  触发中断的引脚号
 */
void bsp_gpio_irq_dispatch(uint16_t GPIO_Pin) {
    int slot = bsp_gpio_irq_find_slot(GPIO_Pin);
    if (slot < 0) return;

    gpio_irq_reg_t *reg = &g_irq_regs[slot];
    if (!reg->enabled || reg->callback == NULL) return;

    /* 有效电平门：把“双沿 EXTI”收敛成“单沿逻辑触发”
     * 硬件 MX_GPIO_Init 配的是 RISING_FALLING，靠回读电平滤掉无效边沿，
     * 避免一次脉冲在上升/下降沿各触发一次。port==NULL 或 level==ANY 时跳过。 */
    if (reg->active_level != BSP_GPIO_IRQ_LEVEL_ANY && reg->port != NULL) {
        if (HAL_GPIO_ReadPin(reg->port, GPIO_Pin) != (GPIO_PinState)reg->active_level)
            return;
    }

    /* 防抖窗口：DWT 周期计数器（分辨率≈6ns）。仅对“越过窗口”的触发更新
     * last_active，与调用方原有语义一致——被窗口滤掉的边沿不刷新基准，
     * 以免一次抖动把后续有效触发也压住。 */
    if (reg->debounce_us > 0) {
        uint32_t now = bsp_GetCycleCount();
        if (bsp_GetElapsedUS(reg->last_active, now) < reg->debounce_us)
            return;
        reg->last_active = now;
    }

    reg->callback(GPIO_Pin, reg->user_data);
}
