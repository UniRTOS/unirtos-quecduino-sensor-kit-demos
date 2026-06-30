# MIC Demo

## 概述

MIC Demo 是一个基于 UniRTOS 的麦克风阈值检测示例。该示例通过读取 ADC1 的电压值模拟麦克风声音强度检测，当采样值超过设定阈值时，点亮指定 GPIO 上的 LED 一段时间。通过此示例，开发者可以快速了解 UniRTOS 中 ADC 采样、GPIO 输出控制、PinMux 配置以及应用任务注册的基本使用方法。

## **模块介绍**

麦克风是**声电转换器件**的简称，也被称为声音检测传感器模块。它可以检测周围环境中的声音强度，并转换为电信号输出。它内部包含一个麦克风，可以捕捉声音信号。通过调节模块上的感敏度电位器，可以调节模块对声音的敏感度。它支持模拟输出模式，满足大部分应用及设计需求。

##  连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设       | 开发板   |
| ---------- | -------- |
| MIC（+）   | 3.3V     |
| MIC（-）   | GND      |
| MIC（S）   | A1(ADC1) |
| LED   (-)  | GND      |
| LED   (S） | PIN31    |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/04-mic
```

### 3. 项目结构

```text
04-mic/
├── CMakeLists.txt      # MIC Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── mic_demo.c          # MIC 阈值检测示例源代码
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

初始化成功后，可以在日志中看到类似输出：

```text
[I/DEMO] MIC demo init done, adc=ADC1 led_pin=31
[I/DEMO] MIC demo started: adc=ADC1 threshold=200mV sample=500ms led_pin=31 led_on=2s
```

运行过程中，任务会周期采集 ADC1 电压值，并输出类似日志：

```text
[I/DEMO] adc1 value=138mV
[I/DEMO] adc1 value=215mV
[I/DEMO] adc1 value=176mV
```

当电压值超过阈值 200mV 时，程序会将 LED 拉到有效电平并保持 2 秒，随后恢复为无效电平。

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 mic_demo_init()
    ↓
创建名为 "mic_demo" 的后台任务
    ↓
进入任务主函数 mic_demo_task()
    ↓
配置 ADC1 采样缩放参数
    ↓
配置 LED 对应 GPIO 和 PinMux
    ↓
循环读取 ADC1 电压值
    ↓
判断采样值是否超过阈值
    ├─ 未超过阈值：等待下一个采样周期
    └─ 超过阈值：点亮 LED 2 秒后熄灭
```

### 主要 API 接口

#### mic_demo_init

任务初始化函数。

- 检查 MIC Demo 任务是否已经创建
- 创建名为 mic_demo 的后台任务
- 设置任务栈大小与优先级
- 输出初始化完成日志

#### mic_demo_task

任务处理函数。

- 配置 ADC1 缩放档位
- 初始化 LED 所在 GPIO 及 PinMux
- 周期性读取 ADC1 电压值
- 输出采样日志
- 在电压超过阈值时触发 LED 指示

#### mic_configure_adc

ADC 配置函数。

- 设置 ADC1 的缩放档位为 QOSA_ADC_SCALE_LEVEL_2
- 在配置失败时输出告警日志

#### mic_prepare_led_gpio

LED GPIO 初始化函数。

- 获取 LED 引脚默认配置
- 设置引脚复用为 GPIO 功能
- 初始化 GPIO 输出方向、上下拉和默认电平
- 使用全局状态避免重复初始化

#### mic_handle_sound

声音阈值处理函数。

- 判断当前采样值是否大于阈值
- 若超过阈值，则点亮 LED
- 保持设定时长后熄灭 LED

## 配置说明

MIC Demo 的默认参数直接定义在 mic_demo.c 中：

- MIC_APP_ORDER：应用初始化顺序，默认值为 210
- MIC_TASK_STACK_SIZE：任务栈大小，默认值为 2048
- MIC_TASK_PRIORITY：任务优先级，默认值为 QOSA_PRIORITY_NORMAL
- MIC_LED_PIN_NUM：LED 引脚号，默认值为 QOSA_PIN_31
- MIC_THRESHOLD_MV：声音触发阈值，默认值为 200mV
- MIC_SAMPLE_INTERVAL_MS：采样周期，默认值为 500ms
- MIC_LED_ON_SEC：LED 点亮保持时间，默认值为 2 秒
- MIC_LED_ACTIVE_LEVEL：LED 有效电平，默认值为 QOSA_GPIO_LEVEL_HIGH

不同平台的 ADC 通道映射、GPIO 编号和 PinMux 复用定义可能不同，移植时请根据实际硬件原理图和平台 PINMUX 表调整。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎提交 Issue 和 Pull Request。