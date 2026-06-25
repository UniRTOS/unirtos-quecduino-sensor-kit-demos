# 火焰检测模块

## **一、** **模块介绍**

火焰检测模块是用于**探测火焰 / 明火**的传感器模块，通过接收火焰产生的红外光，输出高低电平信号，实现火灾报警、火源检测。

**核心参数**

- 工作电压：3.3V–5V
- 输出：数字信号（无高 /有低）
- 输出：模拟信号（近高 / 远低）
- 检测角度：约 60°
- 接口：**MX1.25-2P**
- 用途：火焰检测、火灾报警、火源判断

## 二、连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设         | 开发板      |
| ------------ | ----------- |
| Flame（+）   | 3.3V        |
| Flame（-）   | GND         |
| Flame（A/D） | A1（ADC1）  |
| LED（S）     | PIN31（10） |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境和完成基础开发流程。

### 2. 项目结构

```text
flame_demos/
├── CMakeLists.txt      # CMake 构建配置
├── flame_demo.c        # 火焰传感器 ADC 分级报警示例源码
└── README.md           # 本文件
```

### 3. 构建项目

当前目录中的示例源码和子目录 CMakeLists 已就绪，但当前仓库顶层入口尚未将 flame_demos 接入统一菜单配置和条件构建。

如需作为内置应用参与整仓构建，建议补齐以下接入项：

- 在 qos_applications/Kconfig 中新增 flame demo 对应配置项，并通过 orsource 引入目录级 Kconfig
- 在 qos_applications/CMakeLists.txt 中新增对应的 add_subdirectory_if_exist 条件编译入口
- 在菜单配置中使能新增的 flame demo 选项

完成接入并使能后，可在 UniRTOS 根目录执行类似命令进行构建：

```bash
unirtos make EG800ZCN_LA EG800ZCNLAR01A01M04_BETA_OCPU_20260511
```

### 4. 日志展示

初始化成功后，可在日志中看到类似输出：

```text
[I/LOG_TAG_DEMO] flame led init ok, pin=31 gpio=31
[I/LOG_TAG_DEMO] flame adc init ok, channel=1 safe<100mV fire>=500mV
[I/LOG_TAG_DEMO] flame demo started
```

运行期间，示例会在后台任务中持续读取 ADC 电压值，并按默认 1 s 周期执行一次监控判断；当进入火灾报警状态时，LED 会按 500 ms 高低电平交替进行快闪。典型日志如下：

```text
[I/LOG_TAG_DEMO] flame state change: adc=56mV | state=safe
[I/LOG_TAG_DEMO] flame state change: adc=286mV | state=warning
[I/LOG_TAG_DEMO] flame state change: adc=712mV | state=fire
[I/LOG_TAG_DEMO] flame monitor: adc=745mV | state=fire
```

在默认配置下，状态判定规则如下：

- safe：ADC 电压值小于 100 mV，认为当前无火焰，LED 熄灭
- warning：ADC 电压值大于等于 100 mV 且小于 500 mV，认为存在火险隐患，LED 常亮
- fire：ADC 电压值大于等于 500 mV，认为进入火灾报警状态，LED 快闪

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 flame_demo_bootstrap()
    ↓
创建名为 "flame_demo" 的后台任务
    ↓
进入任务主函数 flame_demo_task()
    ↓
调用 flame_demo_led_init()
    ↓
配置 GPIO31 对应引脚为 GPIO 输出模式
    ↓
调用 flame_demo_adc_init()
    ↓
设置 ADC 量程档位
    ↓
进入周期循环：
  ├─ 调用 qosa_adc_get_volt() 读取 ADC 电压
  ├─ 调用 flame_demo_eval_state() 判断安全/隐患/火灾状态
  ├─ 调用 flame_demo_set_led_level() 控制 LED 亮灭
  ├─ 处于 fire 状态时调用 flame_demo_blink_led() 执行快闪
  └─ 输出当前电压和状态日志
```

### 主要 API 接口

#### flame_demo_bootstrap

模块启动入口函数。

- 检查火焰监控任务是否已经创建
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出启动日志

#### flame_demo_task

后台任务处理函数。

- 调用 LED 初始化函数完成报警灯 GPIO 配置
- 调用 ADC 初始化函数完成量程配置
- 进入长期循环，按固定周期读取 ADC 电压
- 根据阈值判断当前火焰等级并联动更新 LED
- 输出当前电压和状态日志

#### flame_demo_led_init

LED 初始化函数。

- 调用 qosa_get_pin_default_cfg() 获取 LED 引脚默认配置
- 调用 qosa_pin_set_func() 将引脚切换到 GPIO 功能
- 调用 qosa_gpio_init() 将对应 GPIO 初始化为输出模式
- 在初始化成功后记录 GPIO 编号并输出日志

#### flame_demo_adc_init

ADC 初始化函数。

- 通过 qosa_adc_ioctl() 设置目标 ADC 通道的量程档位
- 在初始化成功后输出当前 ADC 通道和状态阈值配置
- 在配置失败时输出错误日志并返回失败状态

#### flame_demo_eval_state

状态判定函数。

- 将当前 ADC 电压值与 100 mV 和 500 mV 阈值进行比较
- 返回 safe、warning 或 fire 三档状态

#### flame_demo_set_led_level

LED 电平控制函数。

- 向已初始化的报警灯 GPIO 输出高低电平
- safe 状态下拉低输出
- warning 状态下拉高输出

#### flame_demo_blink_led

火灾报警快闪函数。

- 先将 LED 置高
- 延时 500 ms 后将 LED 置低
- 再延时 500 ms，完成一次快闪周期

#### flame_demo_state_name

状态文本映射函数。

- 将枚举态转换为日志字符串
- 供状态变化日志和监控日志复用

## 配置说明

默认火焰传感器示例配置定义在 flame_demo.c 中，可通过宏进行编译期覆盖：

- FLAME_DEMO_ADC_CHANNEL：默认 ADC 通道为 QOSA_ADC1_CHANNEL
- FLAME_DEMO_LED_PIN_NUM：默认 LED 引脚为 QOSA_PIN_31
- FLAME_DEMO_TASK_STACK_SIZE：后台任务栈大小为 2048
- FLAME_DEMO_SAMPLE_PERIOD_MS：常规监控循环周期为 1000 ms
- FLAME_DEMO_BLINK_PERIOD_MS：火灾报警快闪周期的单相位时长为 500 ms
- FLAME_DEMO_SAFE_THRESHOLD_MV：安全阈值为 100 mV
- FLAME_DEMO_ALERT_THRESHOLD_MV：火灾报警阈值为 500 mV
- FLAME_DEMO_ADC_SCALE：默认 ADC 量程档位为 QOSA_ADC_SCALE_LEVEL_2
- FLAME_DEMO_TASK_PRIORITY：后台任务优先级为 QOSA_PRIORITY_NORMAL

如果实际硬件连接的 ADC 输入通道、LED 引脚或传感器输出阈值与默认值不同，需要根据原理图和实测数据调整上述宏配置。当前示例采用 ADC 电压阈值判断方式，适合对火焰传感器模拟输出进行简单分级报警；如果你的模块输出特性并非电压越高火焰越强，或者更适合使用数字输出口进行高低电平判定，可在此基础上继续调整判定逻辑。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎提交 Issue 和 Pull Request。