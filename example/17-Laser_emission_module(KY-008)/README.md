# KY-008 Laser Emission Demo

## 概述

KY-008 Laser Emission Demo 是一个基于 UNIRTOS 的 GPIO 输出控制示例项目。该项目演示了如何初始化激光发射模块控制引脚，并通过后台任务周期性打开和关闭激光输出。通过此示例，开发者可以快速了解 UNIRTOS GPIO 输出、任务创建、周期延时和基础日志打印的使用方法。

## 模块介绍

KY-008 激光发射模块可在信号脚有效时发射激光，常用于光电触发、指示、对准和实验演示。激光模块具有方向性强、亮度集中的特点，使用时应避免直视光束或照射人眼。

本示例默认高电平打开激光，低电平关闭激光，并按固定周期闪烁。

## 连接示例

| 外设 | 开发板 |
| ---- | ------ |
| KY-008（VCC） | 3.3V |
| KY-008（GND） | GND |
| KY-008（S） | PIN31 |

当前示例默认使用 PIN31 控制激光发射模块，高电平打开，低电平关闭。

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/17-Laser_emission_module(KY-008)
```

### 3. 项目结构

```text
17-Laser_emission_module(KY-008)/
├── CMakeLists.txt      # KY-008 Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── ky008_demo.c        # 激光发射控制示例源代码
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

```text
[I/LOG_TAG_DEMO] KY-008 init ok, pin=31, gpio=31
```

后台任务会默认每隔 2 秒切换一次激光状态：

```text
[I/LOG_TAG_DEMO] KY-008 laser on
[I/LOG_TAG_DEMO] KY-008 laser off
```

默认状态控制规则如下：

- GPIO 输出高电平：激光打开
- GPIO 输出低电平：激光关闭

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
执行 ky008_demo_init()
    ↓
创建名为 "ky008" 的后台任务
    ↓
调用 ky008_laser_init() 初始化激光控制 GPIO
        ↓
进入周期循环：
    ├─ 调用 ky008_laser_on() 打开激光
    ├─ 延时 2000 ms
    ├─ 调用 ky008_laser_off() 关闭激光
    └─ 延时 2000 ms
```

### 主要 API 接口

#### ky008_demo_init

- 创建 KY-008 激光控制任务
- 设置任务栈、优先级、任务名称和入口函数

#### ky008_laser_init

- 获取 PIN31 默认配置并切换到 GPIO 功能
- 初始化为 GPIO 输出模式
- 默认关闭激光

#### ky008_laser_on

- 调用 `qosa_gpio_set_level()` 输出高电平
- 设置失败时输出错误日志

#### ky008_laser_off

- 调用 `qosa_gpio_set_level()` 输出低电平
- 设置失败时输出错误日志

#### ky008_laser_blink

- 周期调用打开和关闭接口
- 输出激光状态日志

#### UNIRTOS_APP_EXPORT

- 以名称 `ky008_demo` 注册 `ky008_demo_init()`
- 启动优先级为 `200`

## 配置说明

当前示例中的默认配置定义在 `ky008_demo.c` 中：

- 激光控制引脚：`QOSA_PIN_31`
- 有效电平：高电平有效
- 闪烁间隔：`2000 ms`
- 任务栈大小：`2048`
- 任务优先级：`QOSA_PRIORITY_NORMAL`

激光模块具有潜在安全风险，调试时请避免直视或对准人体、镜面反射物体。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎参与共建，建议按以下方式提交：

- 提交前先执行一次基础验证：env-setup、build、clean。
- 使用清晰的提交说明，描述改动目的、影响范围和验证结果。
- 新增功能或行为变化时，同步更新 README 与相关文档。
- 通过 Issue 或 Pull Request 提交问题修复与功能改进。
