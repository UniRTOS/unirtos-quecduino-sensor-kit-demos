# RGB Demo

## 概述

RGB Demo 是一个基于 UNIRTOS 的 RGB LED 控制示例项目。该项目演示了如何在 UNIRTOS 平台上获取 GPIO 默认配置、配置 PINMUX 为 GPIO 功能、初始化 GPIO 输出方向，并通过任务周期性切换红、绿、蓝三路 LED 的点亮状态。通过此示例，开发者可以快速了解 UNIRTOS GPIO 与任务 API 的基本使用方法。

**适用平台**：所有支持 UNIRTOS GPIO、PINCTRL 和任务调度能力的平台

## **模块介绍**

三色 RGBLED 是**全彩发光二极管模块**，由红、绿、蓝三颗芯片封装在一起，可通过 PWM 调节亮度混合出任意颜色，广泛用于氛围灯、状态指示、交互提示、创客 DIY 场景；它能实现七彩渐变、呼吸、闪烁等效果，具备体积小、亮度高、3.3V/5V 兼容、驱动简单、寿命长等优点。

**发光原理：** 

LED引脚共地，当正负极形成电压差时，LED点亮，所以高电平LED亮灯。

## 二、 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设     | 开发板 |
| -------- | ------ |
| LED（-） | GND    |
| LED（R） | PIN19  |
| LED（G） | PIN20  |
| LED（B） | PIN21  |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境，了解开发过程。

### 2. 获取项目

RGB Demo 已位于 UniRTOS SDK 源码树中，无需单独克隆仓库。进入以下目录即可查看示例代码：

```text
unirtos/qos_applications/rgb_demos/
```

### 3. 项目结构

```text
rgb_demos/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 本文件
└── unir_rgb_demo.c         # RGB LED 示例源代码
```

### 4. 构建项目

当前目录中已经提供 RGB Demo 的局部 CMakeLists.txt，用于生成静态库目标。

如果需要将该示例纳入 UniRTOS 应用统一构建流程，请先在以下入口文件中补充接线：

- qos_applications/Kconfig
- qos_applications/CMakeLists.txt

接入完成后，可在 unirtos 目录下运行以下命令进行构建：

```text
unirtos make EG800ZCN_LA EG800ZCNLAR01A01M04_BETA_OCPU_20260511
```

### 5. 日志展示

初始化阶段可在日志中看到以下输出：

```text
[V/DEMO] enter rgb led demo !!!
```

成功运行后，示例会创建 RGB 控制任务，并默认每隔约 1 秒切换一次点亮颜色，效果如下：

```text
红灯亮
绿灯亮
蓝灯亮
红灯亮
绿灯亮
蓝灯亮
...
```

如果开发板连接了共阳极 RGB LED，则代码中的低电平表示点亮、高电平表示熄灭；如果是共阴极，请按实际硬件调整高低电平逻辑。

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 unir_rgb_init()
    ↓
检查 RGB 控制任务是否已创建
    ↓
创建名为 "unir_rgb_demo" 的任务
    ↓
进入任务主函数 unir_rgb_demo_process()
    ↓
分别获取红、绿、蓝三路引脚默认 GPIO 配置
    ↓
将三个引脚切换为 GPIO 功能并初始化为输出高电平
    ↓
进入无限循环，按 1 秒周期切换颜色：
  ├─ 红灯亮
  ├─ 绿灯亮
  └─ 蓝灯亮
```

### 主要 API 接口

#### unir_rgb_init

任务初始化函数。

- 输出 RGB Demo 启动日志
- 检查任务是否已创建
- 创建 RGB 控制任务，设置堆栈大小和优先级
- 设置任务名称和入口函数

#### unir_rgb_demo_process

任务处理函数。

- 获取红、绿、蓝三路引脚的默认配置
- 配置 PINMUX 为 GPIO 功能
- 将三路 GPIO 初始化为输出模式，并默认输出高电平
- 在循环中依次切换红、绿、蓝三种颜色
- 每次颜色切换后延时约 1 秒

#### qosa_get_pin_default_cfg

GPIO 默认配置获取接口。

- 根据引脚号解析平台默认 GPIO 编号与复用功能
- 为后续 PINMUX 切换和 GPIO 初始化提供配置参数

#### qosa_gpio_init

GPIO 初始化接口。

- 将目标 GPIO 配置为输出模式
- 设置上拉属性
- 设置初始化电平状态

## 配置说明

默认 RGB 配置定义在 unir_rgb_demo.c 中：

- RGB_RED_PIN：默认红灯引脚为 19
- RGB_GREEN_PIN：默认绿灯引脚为 20
- RGB_BLUE_PIN：默认蓝灯引脚为 21
- UNIR_RGB_DEMO_TASK_STACK_SIZE：任务栈大小为 1024
- UNIR_RGB_DEMO_TASK_PRIO：任务优先级为 QOSA_PRIORITY_NORMAL

当前示例默认按共阳极 RGB LED 设计，即低电平点亮、高电平熄灭。如果硬件为共阴极 RGB LED，请将代码中的 QOSA_GPIO_LEVEL_LOW 与 QOSA_GPIO_LEVEL_HIGH 对调。

不同平台的 PINMUX 定义和 RGB LED 接线方式可能存在差异，请根据实际平台的 PINMUX 表与硬件连接关系调整 RGB_RED_PIN、RGB_GREEN_PIN、RGB_BLUE_PIN 以及输出电平逻辑。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎提交 Issue 和 Pull Request！