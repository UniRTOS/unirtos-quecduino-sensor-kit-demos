# Button Demo

## 概述

Button Demo 是一个基于 UNIRTOS 的按键中断示例项目。该项目演示了如何在 UNIRTOS 平台上解析 GPIO 默认配置、切换 PINMUX 到 GPIO 功能、注册 GPIO 中断回调，并在检测到按键按下时输出日志计数。通过此示例，开发者可以快速了解 UNIRTOS GPIO 中断相关 API 的基本使用方法。

**适用平台**：所有支持 UNIRTOS GPIO、PINCTRL 与 GPIO 中断能力的平台

##  **模块介绍**

按键模块是**最基础的数字输入模块**，通过轻触开关实现通断控制，输出高低电平信号，用于实现**人机交互、开关控制、触发指令、计数、模式切换**等功能，是嵌入式 / 物联网项目必备模块。

**1、核心参数**

- 类型：轻触按键（机械式）
- 供电：3.3V–5V
- 输出：**数字信号（高 / 低电平）**
- 引脚：3 针（VCC、GND、SIG）
- 默认状态：**高电平（未按下）**
- 触发状态：**低电平（按下）**
- 自带：上拉电阻、信号指示灯

**2、原理图**

![](../../media/key1.png)

vcc和电阻都在芯片内部，当按键断开时，流过电阻的电流称为灌电流，大概几十毫安，因此此时引脚为高电平。按下时与地接通为低电平

## **二、** **连接示例**

根据表格和图片指导，将外设与开发板一一对应连接

| **外设**     | **模块** |
| ------------ | -------- |
| **KEY（+）** | 3.3V     |
| **KEY（-）** | GND      |
| **KEY（S）** | PIN29    |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境及完成基本开发流程。

### 2. 获取项目

Button Demo 已位于 UniRTOS SDK 源码树中，无需单独克隆仓库。进入以下目录即可查看示例代码：

```text
unirtos/qos_applications/button/button_demos/
```

### 3. 项目结构

```text
button_demos/
├── CMakeLists.txt          # Button Demo 局部构建配置
├── README.md               # 本文件
└── button_demo.c           # 按键中断示例源代码
```

### 4. 构建项目

当前目录中已经提供 Button Demo 的局部 CMakeLists.txt，用于生成静态库目标。

如果需要将该示例纳入 UniRTOS 应用统一构建流程，请先在以下入口文件中补充接线：

- qos_applications/Kconfig
- qos_applications/CMakeLists.txt

接入完成后，可在 unirtos 根目录下使用类似如下命令进行构建：

```text
unirtos make EG800ZCN_LA EG800ZCNLAR01A01M04_BETA_OCPU_20260511
```

### 5. 日志展示

初始化成功后，可在日志中看到类似输出：

```text
[I/DEMO] button irq demo ready: pin=29 gpio=xx trigger=falling-edge pull-up=on debounce=on
```

按键被按下后，示例会在每次检测到有效低电平按下事件时输出以下日志：

```text
[I/DEMO] button pressed count=1 pin=29 gpio=xx
[I/DEMO] button pressed count=2 pin=29 gpio=xx
[I/DEMO] button pressed count=3 pin=29 gpio=xx
...
```

其中 gpio 编号由平台默认 PIN 配置解析得到，不同平台可能不同。

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 button_demo_init()
    ↓
清零按键运行时状态
    ↓
调用 qosa_get_pin_default_cfg() 解析默认 PIN/GPIO 配置
    ↓
调用 qosa_pin_set_func() 切换 PINMUX 到 GPIO 功能
    ↓
配置中断参数：上拉、去抖、用户上下文、回调函数
    ↓
调用 qosa_interrupt_register() 注册中断回调
    ↓
调用 qosa_interrupt_enable() 使能下降沿触发
    ↓
输出 ready 日志，等待按键事件
    ↓
按键触发中断后进入 button_demo_irq_callback()
    ↓
读取 GPIO 电平并过滤无效触发
    ↓
检测到低电平按下事件后累加计数并打印日志
```

### 主要 API 接口

#### button_demo_init

按键示例初始化函数。

- 重置中断就绪标志和按键计数
- 根据 BUTTON_DEMO_PIN_NUM 获取平台默认 PIN/GPIO 配置
- 将目标引脚切换到 GPIO 功能
- 配置 GPIO 中断参数，包括去抖、上拉和用户上下文
- 注册中断回调并使能下降沿触发
- 初始化完成后输出 ready 日志

#### button_demo_irq_callback

GPIO 中断回调函数。

- 从用户上下文中获取当前引脚配置
- 在模块未完全初始化前忽略中断
- 读取 GPIO 当前电平，避免误判无效触发
- 仅将低电平视为有效按下事件
- 统计有效按键次数并输出日志

#### UNIRTOS_APP_EXPORT

应用启动注册宏。

- 以名称 button_irq_demo 将 button_demo_init 注册到应用启动流程
- 系统启动后自动执行初始化逻辑，无需手动创建任务

## 配置说明

当前示例中的默认配置定义在 button_demo.c 中：

- BUTTON_DEMO_PIN_NUM：默认测试按键引脚为 QOSA_PIN_29
- 触发方式：QOSA_GPIO_TRIGGER_FALLING_EDGE
- 上拉配置：QOSA_GPIO_PULL_UP
- 去抖配置：QOSA_GPIO_DEBOUNCE_EN
- 启动顺序：UNIRTOS_APP_EXPORT(200, "button_irq_demo", button_demo_init)

该示例假设按键硬件在按下时将 GPIO 拉低，因此使用下降沿触发并在回调中将低电平判定为有效按下。

不同平台的 PINMUX、GPIO 编号和外部按键连接方式可能存在差异，请根据实际平台的 PINMUX 表与硬件设计调整 BUTTON_DEMO_PIN_NUM 及相关触发逻辑。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎提交 Issue 和 Pull Request。