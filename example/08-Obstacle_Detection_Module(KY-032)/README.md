# KY-032 Obstacle Detection Demo

## 概述
KY-032 Obstacle Detection Demo 是一个基于 UNIRTOS 的 GPIO 输入检测示例项目。该项目演示了如何在 UNIRTOS 平台上获取 PIN 默认配置、切换 PINMUX 到 GPIO 功能、初始化 GPIO 输入方向，并通过后台任务周期性读取 KY-032 红外避障模块输出电平。通过此示例，开发者可以快速了解 UNIRTOS GPIO 输入、任务创建、轮询检测和基础日志打印的使用方法。

## 模块介绍
KY-032 障碍物检测模块是红外反射式数字检测器件，也叫红外避障模块，常用于近距离障碍物检测、循迹、避障和限位触发等场景。模块通过红外发射管发射红外光，并由接收管检测反射光，从而判断前方是否存在障碍物。

**模块组成：**

![](../../media/obstacle1.png)

**工作原理：**

当没有检测到反射回来的红外光线时，KY-032 OUT 引脚通常输出高电平；当检测到障碍物反射红外光线时，OUT 引脚输出低电平。代码中通过读取 GPIO 输入电平判断当前是否检测到障碍物。

## 连接示例
根据下表将 KY-032 模块与开发板连接：

| 外设 | 开发板 |
| ---- | ------ |
| KY-032（+） | 3.3V |
| KY-032（-） | GND |
| KY-032（S/OUT） | PIN23 |

当前示例默认使用 PIN23 读取 KY-032 信号脚，代码中低电平表示检测到障碍物，高电平表示未检测到障碍物。

## 快速上手

### 1. 开发环境搭建
参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/08-Obstacle_Detection_Module(KY-032)
```

### 3. 项目结构

```
08-Obstacle_Detection_Module(KY-032)/
├── CMakeLists.txt      # KY-032 Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── ky032_demo.c        # KY-032 障碍物检测示例源代码
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
固件烧录后开机启动，默认轮询模式下可在日志中看到类似输出：

```
[I/LOG_TAG_DEMO] KY-032 init ok, pin=23, gpio=31
[I/LOG_TAG_DEMO] KY-032 polling mode started
```
初始化成功后，后台任务会默认每隔 200 ms 读取一次传感器状态，并持续输出障碍物检测日志：

```
[I/LOG_TAG_DEMO] KY-032 no obstacle
[I/LOG_TAG_DEMO] KY-032 obstacle detected
[I/LOG_TAG_DEMO] KY-032 no obstacle
```
默认状态控制规则如下：

- `KY-032 no obstacle`：GPIO 输入高电平，未检测到障碍物
- `KY-032 obstacle detected`：GPIO 输入低电平，检测到障碍物

如果将 `KY032_USE_INTERRUPT_MODE` 编译开关设置为 `1`，示例会创建中断模式任务，并在初始化成功后输出类似日志：

```
[I/LOG_TAG_DEMO] KY-032 interrupt mode started
```

## 代码概览

### 示例工作流程

```
程序启动
    ↓
执行 UNIRTOS_APP_EXPORT 注册的 ky032_demo_init()
    ↓
根据 KY032_USE_INTERRUPT_MODE 编译开关选择轮询模式或中断模式
    ↓
创建名为 "ky032_poll" 或 "ky032_int" 的后台任务
    ↓
进入任务主函数 ky032_monitor_polling() 或 ky032_monitor_interrupt()
    ↓
调用 ky032_sensor_init() 初始化 KY-032 输入 GPIO
    ↓
获取 PIN23 默认配置并切换到 GPIO 功能
    ↓
将 KY-032 对应 GPIO 初始化为上拉输入模式
        ↓
默认轮询模式进入周期循环：
    ├─ 调用 ky032_is_obstacle() 判断是否检测到障碍物
    ├─ 内部调用 ky032_read_state() 读取 GPIO 输入电平
    ├─ 低电平时输出 KY-032 obstacle detected 日志
    ├─ 高电平时输出 KY-032 no obstacle 日志
    └─ 延时 200 ms 后继续下一轮检测
```

### 主要 API 接口

#### ky032_demo_init
KY-032 Demo 启动入口函数。

- 根据 `KY032_USE_INTERRUPT_MODE` 编译开关选择任务入口函数
- 默认创建轮询检测任务 `ky032_poll`
- 中断模式下创建检测任务 `ky032_int`
- 调用 `qosa_task_create()` 创建后台任务
- 设置任务栈大小、任务优先级、任务名称和任务入口函数
- 任务创建失败时输出错误日志

#### ky032_sensor_init
KY-032 GPIO 初始化函数。

- 调用 `qosa_get_pin_default_cfg()` 获取 PIN23 的默认配置
- 调用 `qosa_pin_set_func()` 将目标引脚切换为 GPIO 功能
- 调用 `qosa_gpio_init()` 将 GPIO 初始化为输入模式
- 输入上下拉配置为 `QOSA_GPIO_PULL_UP`
- 初始化成功后清零障碍物标志并输出当前 pin、gpio 信息

#### ky032_read_state
KY-032 电平读取函数。

- 调用 `qosa_gpio_get_level()` 读取当前 GPIO 输入电平
- 读取成功时返回当前电平
- 读取失败时输出错误日志，并按高电平作为默认返回值处理

#### ky032_is_obstacle
障碍物状态判断函数。

- 调用 `ky032_read_state()` 获取当前 GPIO 输入电平
- 当输入电平为 `QOSA_GPIO_LEVEL_LOW` 时返回检测到障碍物
- 当输入电平为高电平时返回未检测到障碍物

#### ky032_monitor_polling
默认轮询模式任务函数。

- 调用 `ky032_sensor_init()` 完成 GPIO 初始化
- 初始化失败时输出错误日志并删除当前任务
- 初始化成功后输出轮询模式启动日志
- 进入长期循环，按 200 ms 周期读取障碍物状态并输出日志

#### ky032_interrupt_init
KY-032 中断模式初始化函数，仅在 `KY032_USE_INTERRUPT_MODE` 为 `1` 时编译。

- 配置 GPIO 中断参数，包括去抖、上拉、回调函数和用户上下文
- 调用 `qosa_interrupt_register()` 注册中断回调
- 调用 `qosa_interrupt_enable()` 使能下降沿触发中断
- 注册或使能失败时输出错误日志并返回失败状态

#### ky032_irq_handler
KY-032 中断回调函数，仅在中断模式下使用。

- 中断触发后读取当前传感器电平
- 当电平表示检测到障碍物时，将 `obstacle_flag` 置为 `1`

#### ky032_monitor_interrupt
中断模式任务函数，仅在 `KY032_USE_INTERRUPT_MODE` 为 `1` 时编译。

- 初始化 KY-032 输入 GPIO
- 注册并使能 GPIO 下降沿中断
- 周期检查 `obstacle_flag` 标志位
- 检测到障碍物后输出日志并清零标志位
- 未检测到新事件时输出无障碍物日志

#### UNIRTOS_APP_EXPORT
应用启动注册宏。

- 以名称 `ky032_demo` 注册 `ky032_demo_init()`
- 启动优先级为 `200`
- 系统启动后自动执行 KY-032 Demo 初始化逻辑

## 配置说明
当前示例中的默认配置定义在 `ky032_demo.c` 中：

- `KY032_PIN_NUM`：默认 KY-032 信号输入引脚为 `23`
- `KY032_POLL_INTERVAL_MS`：默认检测与日志输出周期为 `200 ms`
- `KY032_TASK_STACK_SIZE`：KY-032 检测任务栈大小为 `2048`
- `KY032_TASK_PRIORITY`：KY-032 检测任务优先级为 `QOSA_PRIORITY_NORMAL`
- `KY032_USE_INTERRUPT_MODE`：默认值为 `0`，使用轮询模式；设置为 `1` 时启用中断模式
- `UNIRTOS_APP_EXPORT(200, "ky032_demo", ky032_demo_init)`：注册 KY-032 Demo 启动入口

该示例默认按低电平触发的 KY-032 模块设计。如果实际模块输出逻辑与默认规则相反，请调整 `ky032_is_obstacle()` 中的电平判断逻辑。

## 论坛社区
[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南
欢迎提交 Issue 和 Pull Request。
