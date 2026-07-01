# KY-026 Flame Detection Demo

## 概述

KY-026 Flame Detection Demo 是一个基于 UNIRTOS 的 ADC 火焰检测示例项目。该项目演示了如何读取火焰传感器模拟输出，并根据电压阈值划分安全、风险和火警状态，同时通过 LED 输出报警提示。通过此示例，开发者可以快速了解 UNIRTOS ADC 采样、阈值分级、GPIO 输出和任务创建的使用方法。

## 模块介绍

KY-026 火焰检测模块可感知特定波段的火焰光信号，常用于火源靠近检测和实验报警演示。模块的模拟输出电压会随检测到的火焰强度变化，开发板可通过 ADC 读取并进行分级判断。

本示例使用 ADC1 读取火焰传感器电压，使用 PIN31 控制报警 LED。电压越高，代码判定的风险等级越高。

## 连接示例

| 外设 | 开发板 |
| ---- | ------ |
| KY-026（VCC） | 3.3V |
| KY-026（GND） | GND |
| KY-026（AO） | ADC1 |
| LED（S） | PIN31 |

当前示例默认使用 ADC1 读取火焰传感器模拟输出，使用 PIN31 控制报警 LED。

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/11-flame_detect(KY-026）
```

### 3. 项目结构

```text
11-flame_detect(KY-026）/
├── CMakeLists.txt      # KY-026 Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── flame_demo.c        # 火焰检测 ADC 示例源代码
└── README.md           # 本文件
```

### 4. 构建项目

拉取SDK与依赖库

```
unirtos-cli env-setup
```
在 PowerShell 窗口执行固件编译命令：

```
unirtos-cli build -m EG800ZCN_LA -v EG800ZCNLAR01A01_OCPU_20260626
```
等待编译结束后，PowerShell 窗口末尾会提示固件编译结果：

```
SUCCESS: Unirtos project built successfully!
```

### 5. 日志展示

固件烧录后开机启动，可在日志中看到类似输出：

```text
[I/LOG_TAG_DEMO] flame led init ok, pin=31, gpio=31
[I/LOG_TAG_DEMO] flame demo started
```

后台任务会默认每隔 1 秒输出一次状态：

```text
[I/LOG_TAG_DEMO] ADC: 80 mV, status: safe
[I/LOG_TAG_DEMO] ADC: 260 mV, status: flame risk
[I/LOG_TAG_DEMO] ADC: 650 mV, status: fire alarm
```

默认状态控制规则如下：

- ADC 电压低于 `100 mV`：安全状态
- ADC 电压达到 `100 mV` 且低于 `500 mV`：火焰风险状态
- ADC 电压达到或超过 `500 mV`：火警状态，LED 闪烁报警

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
执行 flame_demo_init()
    ↓
创建名为 "flame" 的后台任务
    ↓
初始化 LED GPIO 与 ADC1
        ↓
进入周期循环：
    ├─ 调用 flame_read_value() 读取 ADC 电压
    ├─ 根据阈值判断安全、风险或火警状态
    ├─ 火警状态下闪烁 LED
    └─ 输出当前 ADC 值和状态日志
```

### 主要 API 接口

#### flame_demo_init

- 调用 `qosa_task_create()` 创建火焰检测任务
- 设置任务栈、优先级、任务名称和入口函数

#### flame_led_init

- 获取 PIN31 默认配置并切换到 GPIO 功能
- 初始化报警 LED 为 GPIO 输出模式

#### flame_adc_init

- 配置 ADC1 通道和量程
- 初始化失败时输出错误日志

#### flame_read_value

- 调用 ADC 读取接口获取当前电压
- 读取失败时输出错误日志

#### flame_monitor_task

- 周期读取火焰传感器电压
- 按阈值分级并控制 LED

#### UNIRTOS_APP_EXPORT

- 以名称 `flame_demo` 注册 `flame_demo_init()`
- 启动优先级为 `200`

## 配置说明

当前示例中的默认配置定义在 `flame_demo.c` 中：

- ADC 通道：`QOSA_ADC1_CHANNEL`
- LED 控制引脚：`QOSA_PIN_31`
- 安全阈值：`100 mV`
- 火警阈值：`500 mV`
- 监控周期：`1000 ms`
- 报警闪烁间隔：`500 ms`
- 任务栈大小：`2048`
- 任务优先级：`QOSA_PRIORITY_NORMAL`

火焰检测演示请在安全环境中进行，并根据传感器实际输出调整阈值。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎参与共建，建议按以下方式提交：

- 提交前先执行一次基础验证：env-setup、build、clean。
- 使用清晰的提交说明，描述改动目的、影响范围和验证结果。
- 新增功能或行为变化时，同步更新 README 与相关文档。
- 通过 Issue 或 Pull Request 提交问题修复与功能改进。
