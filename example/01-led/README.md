# LED Demo

## 概述

LED Demo 是一个基于 UNIRTOS 的 GPIO 输出控制示例项目。该项目演示了如何在 UNIRTOS 平台上获取 PIN 默认配置、切换 PINMUX 到 GPIO 功能、初始化 GPIO 输出方向，并通过后台任务周期性控制 LED 亮灭。通过此示例，开发者可以快速了解 UNIRTOS GPIO 输出、任务创建和基础日志打印的使用方法。

## 模块介绍

LED原理及产业分类LED是发光二极体( Light EmitTIng Diode, LED)的简称，也被称作发光二极管，这种半导体组件发展以来一般是作为指示灯、显示板，但目前随着技术增加，已经能作为光源使用，它不但能够高效率地直接将电能转化为光能，而且拥有最长达数万小时～10 万小时的使用寿命，同时具备不若传统灯泡易碎，并能省电，同时拥有环保无汞、体积小、可应用在低温环境、光源具方向性、造成光害少与色域丰富等优点

**LED 组成：**

![](../../media/led1.png)

**发光原理：**

![](../../media/led2.png)

LED 具有单向导通特性。左侧为正极，右侧为负极，当正负极之间形成合适的电压差时，LED 点亮；反向连接或电压不足时，LED 不亮。

## 连接示例

根据下表将 LED 模块与开发板连接：

| 外设 | 开发板 |
| ---- | ------ |
| LED（+） | 3.3V |
| LED（-） | GND |
| LED（S） | PIN19 |

当前示例默认使用 PIN19 控制 LED 信号脚，代码中低电平点亮 LED，高电平熄灭 LED。

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/01-led
```

### 3. 项目结构

```text
01-led/
├── CMakeLists.txt      # LED Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── led_demo.c          # LED GPIO 闪烁示例源代码
└── README.md           # 本文件
```

### 4. 构建项目

拉取SDK与依赖库

```
unirtos-cli env-setup
```

在 PowerShell 窗口执行固件编译命令：

```bash
unirtos-cli build -m EG800ZCN_LA -v EG800ZCNLAR01A01_OCPU_20260626
```

等待编译结束后，PowerShell 窗口末尾会提示固件编译结果：

```text
SUCCESS: Unirtos project built successfully!
```

### 5. 日志展示

固件烧录后开机启动，可在日志中看到类似输出：

```text
[V/LOG_TAG_DEMO] [TEST Demo]enter TEST DEMO !!!
[I/LOG_TAG_DEMO] [TEST Demo]LED GPIO initialized successfully, pin_num: 19, gpio_num: 19, level: 1
```
初始化成功后，后台任务会默认每隔 1 秒切换一次 LED 状态，并持续输出开关状态日志：

```text
[I/LOG_TAG_DEMO] [TEST Demo]LED ON
[I/LOG_TAG_DEMO] [TEST Demo]LED OFF
[I/LOG_TAG_DEMO] [TEST Demo]LED ON
```

默认状态控制规则如下：

- `LED ON`：GPIO 输出低电平，LED 点亮
- `LED OFF`：GPIO 输出高电平，LED 熄灭

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
执行 UNIRTOS_APP_EXPORT 注册的 unir_test_demo_init()
    ↓
检查 LED 控制任务是否已经创建
    ↓
创建名为 "test_demo" 的后台任务
    ↓
进入任务主函数 unir_test_demo_process()
    ↓
调用 unir_led_init() 初始化 LED GPIO
    ↓
获取 PIN19 默认配置并切换到 GPIO 功能
    ↓
将 LED 对应 GPIO 初始化为输出模式，默认输出高电平
        ↓
进入周期循环：
    ├─ 调用 unir_led_set(QOSA_GPIO_LEVEL_LOW) 点亮 LED
    ├─ 输出 LED ON 日志并延时 1000 ms
    ├─ 调用 unir_led_set(QOSA_GPIO_LEVEL_HIGH) 熄灭 LED
    └─ 输出 LED OFF 日志并延时 1000 ms
```

### 主要 API 接口

#### unir_test_demo_init

LED Demo 启动入口函数。

- 输出示例启动日志
- 检查 LED 控制任务是否已经创建
- 调用 `qosa_task_create()` 创建后台任务
- 设置任务栈大小、任务优先级、任务名称和任务入口函数

#### unir_test_demo_process

LED 后台任务处理函数。

- 调用 `unir_led_init()` 完成 GPIO 初始化
- 进入长期循环，按固定周期控制 LED 亮灭
- 在每次状态切换后输出对应日志

#### unir_led_init

LED GPIO 初始化函数。

- 调用 `qosa_get_pin_default_cfg()` 获取 PIN19 的默认配置
- 调用 `qosa_pin_set_func()` 将目标引脚切换为 GPIO 功能
- 调用 `qosa_gpio_init()` 将 GPIO 初始化为输出模式
- 默认输出高电平，使 LED 初始状态为熄灭
- 初始化成功后输出当前 pin、gpio 和默认电平信息

#### unir_led_set

LED 电平控制函数。

- 调用 `qosa_gpio_set_level()` 设置 GPIO 输出电平
- 输出低电平时点亮 LED
- 输出高电平时熄灭 LED
- 设置失败时输出错误日志并返回失败状态

#### UNIRTOS_APP_EXPORT

应用启动注册宏。

- 以名称 `unir_led_demo` 注册 `unir_test_demo_init()`
- 启动优先级为 `700`
- 系统启动后自动执行 LED Demo 初始化逻辑

## 配置说明

当前示例中的默认配置定义在 `led_demo.c` 中：

- `LED_PIN_NUM`：默认 LED 控制引脚为 `19`
- `UniRTOS_TEST_DEMO_TASK_STACK_SIZE`：LED 控制任务栈大小为 `1024`
- `UniRTOS_TEST_DEMO_TASK_PRIO`：LED 控制任务优先级为 `QOSA_PRIORITY_NORMAL`
- `UNIRTOS_APP_EXPORT(700, "unir_led_demo", unir_test_demo_init)`：注册 LED Demo 启动入口

该示例默认按低电平点亮 LED 的接线方式设计。如果实际硬件为高电平点亮，请将 `unir_led_set()` 调用中的 `QOSA_GPIO_LEVEL_LOW` 与 `QOSA_GPIO_LEVEL_HIGH` 控制逻辑对调。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎参与共建，建议按以下方式提交：

- 提交前先执行一次基础验证：env-setup、build、clean。
- 使用清晰的提交说明，描述改动目的、影响范围和验证结果。
- 新增功能或行为变化时，同步更新 README 与相关文档。
- 通过 Issue 或 Pull Request 提交问题修复与功能改进。
