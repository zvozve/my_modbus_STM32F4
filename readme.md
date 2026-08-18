# Modbus 协议栈

基于 STM32 HAL 库的轻量级 Modbus RTU 协议栈，支持主机/从机模式，裸机/RTOS 双支持。

## 版本信息

| 版本 | 日期 | 说明 |
|------|------|------|
| v5.0.2 | 2026-08-18 | 主机轮询支持多段注册；统一引入队列；G4/F4统一平台判定 |
| v5.0.1 | 2026-08-14 | 修复主机轮询机制；修复串口DMA半中断 |
| v5.0.0 | 2026-08-05 | 初始版本 |

## 功能特性

### 协议支持
- Modbus RTU 模式
- CRC16 校验
- 广播地址 (0x00) 支持
- 异常响应处理

### 主机模式 (Master)
- 读取保持寄存器 (0x03)
- 读取线圈 (0x01)
- 写单个寄存器 (0x06)
- 写单个线圈 (0x05)
- 写多个寄存器 (0x10)
- **多段轮询**：支持同时轮询多个不连续的寄存器/线圈段
- **异步写入**：支持非阻塞写操作，带完成回调
- 数据变化回调通知
- 超时重试（可配置次数）
- 断线检测与自动重连

### 从机模式 (Slave)
- 响应读保持寄存器 (0x03)
- 响应读线圈 (0x01)
- 响应写单个寄存器 (0x06)
- 响应写单个线圈 (0x05)
- 响应写多个寄存器 (0x10)
- 数据变化回调通知
- 空闲超时断线检测

### 系统支持
- 裸机和 FreeRTOS 双支持
- 多实例（支持同时运行多个主机/从机）
- 非阻塞设计
- 静态内存分配

## 文件结构

```
User/Protocols/Modbus/
├── Inc/
│   ├── modbus_core.h          # 核心定义
│   ├── modbus_master.h        # 主机接口
│   ├── modbus_slave.h         # 从机接口
│   ├── modbus_rtu.h           # RTU 帧处理
│   └── modbus_uart_adapter.h  # UART适配器
├── Src/
│   ├── modbus_core.c          # 核心实现
│   ├── modbus_master.c        # 主机实现
│   ├── modbus_slave.c         # 从机实现
│   ├── modbus_rtu.c           # RTU 帧处理
│   └── modbus_uart_adapter.c  # UART适配器
└── readme.md
```

## 依赖关系

```
应用层 (task_modbus.c / task_modbus_m.c)
    ↓
Modbus 协议栈 (modbus_master/slave)
    ↓
UART 适配层 (modbus_uart_adapter)
    ↓
BSP UART 驱动 (bsp_uart_drv)
    ↓
HAL 库 + 硬件
```

## 移植说明

### 1. UART 驱动接口

协议栈需要底层 UART 驱动提供以下接口（`bsp_uart_drv.h`）：

```c
// 驱动初始化
void uart_drv_init(uart_drv_t *drv, UART_HandleTypeDef *huart, uart_rs485_t *rs485);

// 数据收发
int uart_drv_send(uart_drv_t *drv, const uint8_t *data, uint16_t len);
uint16_t uart_drv_available(uart_drv_t *drv);
uint8_t* uart_drv_get_packet(uart_drv_t *drv, uint16_t *len);
void uart_drv_release_packet(uart_drv_t *drv);

// 配置
int uart_drv_reconfig(uart_drv_t *drv, const uart_drv_cfg_t *cfg);

// 回调注册
void uart_drv_reg_cb(uart_drv_t *drv,
                     void (*on_recv)(uart_drv_t *, uint8_t *, uint16_t),
                     void (*on_sent)(uart_drv_t *),
                     void (*on_error)(uart_drv_t *));
```

### 2. 时间函数

在 `modbus_core.h` 中定义时间获取宏：

```c
// 裸机
#define MB_GET_TICK()      HAL_GetTick()

// 或 FreeRTOS
// #define MB_GET_TICK()   xTaskGetTickCount()
```

### 3. 中断处理

需要在 UART 中断中调用以下函数：

```c
// 发送完成中断
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    uart_drv_on_tx_done(huart);
}

// 接收完成中断 (空闲中断)
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    uart_drv_on_idle(huart);
}

// 错误中断
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    uart_drv_on_error(huart);
}
```

### 4. RS485 收发控制

若使用 RS485，需在 `uart_drv_init` 时传入控制结构：

```c
static uart_rs485_t g_rs485 = {
    .port = GPIOC,
    .pin = GPIO_PIN_3,
    .active_level = 1        // 高电平发送
};

uart_drv_init(&g_uart, &huart1, &g_rs485);
```

## 快速开始

### 主机模式

```c
#include "modbus_master.h"
#include "modbus_uart_adapter.h"
#include "bsp_uart_drv.h"

static uart_drv_t g_uart;
static modbus_t g_modbus;
static uint16_t g_reg_data[96];
static uint8_t g_coil_data[16];

static void on_reg_change(uint16_t addr, uint16_t old_val, uint16_t new_val) {
    // 处理寄存器变化
}

static void on_coil_change(uint16_t addr, bool old_val, bool new_val) {
    // 处理线圈变化
}

void TaskModbus_M_Init(void) {
    // 1. 初始化 UART
    uart_drv_init(&g_uart, &huart2, NULL);
    uart_drv_reg_cb(&g_uart, NULL, NULL, NULL);
    
    uart_drv_cfg_t cfg = {
        .baudrate = 115200,
        .word_length = UART_WORDLENGTH_8B,
        .stop_bits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE
    };
    uart_drv_reconfig(&g_uart, &cfg);

    // 2. 配置主机
    modbus_master_config_t mcfg = {
        .target_slave_addr = 1,
        .poll_interval_ms = 100,
        .min_frame_gap_ms = 100,
        .response_timeout_ms = 1000,
        .max_retries = 3,
        .reconnect_interval_ms = 2000,
    };
    
    // 3. 初始化 Modbus
    modbus_master_init(&g_modbus, &mcfg);
    modbus_master_set_reg_change_callback(&g_modbus, on_reg_change);
    modbus_master_set_coil_change_callback(&g_modbus, on_coil_change);
    modbus_uart_adapter_init(&g_modbus, &g_uart);

    // 4. 注册轮询段（多段支持）
    modbus_master_add_reg_range(&g_modbus, 1, 84, g_reg_data, 96, 200, 0);
    modbus_master_add_coil_range(&g_modbus, 1, 8, g_coil_data, 16, 200, 100);
}

void TaskModbus_M_Process(void) {
    modbus_process(&g_modbus);
}

// 异步写入示例
void write_config_to_slave(void) {
    uint16_t data[] = {100, 200, 300};
    modbus_master_write_regs_async(&g_modbus, 10, 3, data, on_write_done);
}
```

### 从机模式

```c
#include "modbus_slave.h"
#include "modbus_uart_adapter.h"
#include "bsp_uart_drv.h"

static uart_drv_t g_uart;
static modbus_t g_modbus;
static uint8_t g_coils[16];
static uint16_t g_holding_regs[128];

void TaskModbus_Init(void) {
    // 1. 初始化 UART
    uart_drv_init(&g_uart, &huart1, NULL);
    uart_drv_reg_cb(&g_uart, NULL, NULL, NULL);
    
    uart_drv_cfg_t cfg = {
        .baudrate = 115200,
        .word_length = UART_WORDLENGTH_8B,
        .stop_bits = UART_STOPBITS_1,
        .parity = UART_PARITY_NONE
    };
    uart_drv_reconfig(&g_uart, &cfg);

    // 2. 配置从机
    modbus_slave_config_t scfg = {
        .slave_addr = 1,
        .slave_timeout_ms = 5000,
        .coils = g_coils,
        .coil_count = sizeof(g_coils) * 8,
        .holding_regs = g_holding_regs,
        .holding_reg_count = sizeof(g_holding_regs) / sizeof(uint16_t),
    };
    
    // 3. 初始化 Modbus
    modbus_slave_init(&g_modbus, &scfg);
    modbus_slave_set_reg_change_callback(&g_modbus, on_reg_change);
    modbus_slave_set_coil_change_callback(&g_modbus, on_coil_change);
    modbus_uart_adapter_init(&g_modbus, &g_uart);
}

void TaskModbus_Process(void) {
    modbus_process(&g_modbus);
}
```

### 多实例运行

```c
static uart_drv_t g_uart1, g_uart2;
static modbus_t g_modbus1, g_modbus2;
static uint16_t g_reg1[64], g_reg2[64];

void TaskModbus_M_Init(void) {
    // 主机1 (UART1, 从机地址1)
    modbus_master_config_t cfg1 = { .target_slave_addr = 1, ... };
    modbus_master_init(&g_modbus1, &cfg1);
    modbus_uart_adapter_init(&g_modbus1, &g_uart1);
    modbus_master_add_reg_range(&g_modbus1, 0, 10, g_reg1, 64, 200, 0);

    // 主机2 (UART2, 从机地址2)
    modbus_master_config_t cfg2 = { .target_slave_addr = 2, ... };
    modbus_master_init(&g_modbus2, &cfg2);
    modbus_uart_adapter_init(&g_modbus2, &g_uart2);
    modbus_master_add_reg_range(&g_modbus2, 0, 10, g_reg2, 64, 500, 0);
}

void TaskModbus_M_Process(void) {
    modbus_process(&g_modbus1);
    modbus_process(&g_modbus2);
}
```

## API 参考

### 主机 API

| 函数                                       | 说明                 |
| ------------------------------------------ | -------------------- |
| `modbus_master_init()`                     | 初始化主机           |
| `modbus_master_add_reg_range()`            | 注册寄存器轮询段     |
| `modbus_master_add_coil_range()`           | 注册线圈轮询段       |
| `modbus_master_read_holding_regs()`        | 读保持寄存器（同步） |
| `modbus_master_read_coils()`               | 读线圈（同步）       |
| `modbus_master_write_single_reg()`         | 写单个寄存器（同步） |
| `modbus_master_write_single_coil()`        | 写单个线圈（同步）   |
| `modbus_master_write_regs_async()`         | 异步写多个寄存器     |
| `modbus_master_write_coil_async()`         | 异步写单个线圈       |
| `modbus_master_set_reg_change_callback()`  | 寄存器变化回调       |
| `modbus_master_set_coil_change_callback()` | 线圈变化回调         |
| `modbus_master_set_poll_interval()`        | 修改轮询间隔         |

### 从机 API

| 函数                                      | 说明           |
| ----------------------------------------- | -------------- |
| `modbus_slave_init()`                     | 初始化从机     |
| `modbus_slave_set_coil()`                 | 设置线圈值     |
| `modbus_slave_get_coil()`                 | 获取线圈值     |
| `modbus_slave_set_reg()`                  | 设置寄存器值   |
| `modbus_slave_get_reg()`                  | 获取寄存器值   |
| `modbus_slave_set_reg_change_callback()`  | 寄存器变化回调 |
| `modbus_slave_set_coil_change_callback()` | 线圈变化回调   |

### 核心 API

| 函数                          | 说明                   |
| ----------------------------- | ---------------------- |
| `modbus_init()`               | 初始化 Modbus 实例     |
| `modbus_process()`            | 协议处理（需频繁调用） |
| `modbus_set_line_callbacks()` | 断线/恢复回调          |
| `modbus_get_state()`          | 获取当前状态           |
| `modbus_get_line_state()`     | 获取线路状态           |
| `modbus_set_timeouts()`       | 设置超时参数           |
| `modbus_get_last_activity()`  | 获取最后通信时间       |

## 注意事项

| 项目       | 说明                                        |
| ---------- | ------------------------------------------- |
| 调用频率   | `modbus_process()` 需频繁调用（建议 ≤10ms） |
| 线圈存储   | 按位存储，1字节=8个线圈                     |
| 寄存器存储 | 16位存储                                    |
| 多实例     | 适配层支持最多 8 个实例                     |
| 非阻塞     | 所有 API 非阻塞                             |
| 内存分配   | 全部静态分配，无 malloc                     |
| 超时配置   | 响应超时需大于实际通信时间                  |
| 多段轮询   | 各段可独立配置周期和相位                    |

## 通信状态

```
         发送请求
             │
             ▼
    ┌─────────────────┐
    │ WAITING_RESPONSE │ ← 等待响应
    └─────────────────┘
         │        │
    收到响应    超时
         │        │
         ▼        ▼
       IDLE   重试/断线
```

## 线路状态

```
     LINE_OK      ← 正常通信
         │
    连续超时 3 次
         │
         ▼
LINE_DISCONNECTED  ← 断线
         │
   等待重连间隔
         │
         ▼
     LINE_OK      ← 自动恢复
```

## 配置结构说明

### 主机配置 (modbus_master_config_t)

```c
typedef struct {
    uint8_t  target_slave_addr;    // 目标从机地址
    uint32_t poll_interval_ms;     // 轮询基准周期
    uint32_t min_frame_gap_ms;     // 帧间隔
    uint32_t response_timeout_ms;  // 响应超时
    uint8_t  max_retries;          // 最大重试次数
    uint32_t reconnect_interval_ms;// 断线重连间隔
} modbus_master_config_t;
```

### 从机配置 (modbus_slave_config_t)

```c
typedef struct {
    uint8_t   slave_addr;           // 从机地址
    uint32_t  slave_timeout_ms;     // 空闲超时
    uint8_t   *coils;               // 线圈缓冲区
    uint16_t  coil_count;           // 线圈数量（位）
    uint16_t  *holding_regs;        // 保持寄存器缓冲区
    uint16_t  holding_reg_count;    // 寄存器数量
} modbus_slave_config_t;
```

### 轮询段注册

```c
// 寄存器段
int modbus_master_add_reg_range(modbus_t *ctx, uint16_t start_addr, 
                                 uint16_t count, uint16_t *buffer,
                                 uint16_t buffer_size, uint32_t interval_ms,
                                 uint32_t phase_ms);

// 线圈段
int modbus_master_add_coil_range(modbus_t *ctx, uint16_t start_addr,
                                  uint16_t count, uint8_t *buffer,
                                  uint16_t buffer_size, uint32_t interval_ms,
                                  uint32_t phase_ms);
```

**参数说明：**
- `start_addr`：起始 Modbus 地址
- `count`：寄存器/线圈数量
- `buffer`：数据缓冲区
- `buffer_size`：缓冲区大小（寄存器为 uint16_t 个数，线圈为字节数）
- `interval_ms`：轮询周期
- `phase_ms`：相位偏移（用于多段错开轮询）
