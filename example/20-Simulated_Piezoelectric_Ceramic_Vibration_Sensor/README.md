# 模拟压电陶瓷震动模块

## **模块介绍**

这个传感器是基于压电陶瓷片模拟震动的传感器，它利用压电陶瓷给电信号产生震动的反变换过程。当压电陶瓷片震动时，该传感器信号端就会产生电信号。该模块兼容各种单片机控制板，如arduino系列单片机。模块包含2种接口，任你选择。一种是间距为2.54mm的防反接白色端子，使用时，我们可以在单片机上堆叠一个传感器扩展板。模块和自带导线连接，然后连接在传感器扩展板上，简单方便。另一种是间距为2.54mm的排针接口，利用公对母杜邦线，可直接连接在单片机上。

**工作原理：**

 **作为震动输出（逆压电效应）**：模块有供电、接地、信号端。当信号端输入脉冲 / 方波电信号时，压电陶瓷片因逆压电效应产生形变，带动基底振动，实现震动反馈。

**作为震动检测（正压电效应）**：当模块受到机械震动 / 敲击时，压电陶瓷片产生微弱电信号，由信号端输出，开发板通过 ADC 采集即可检测震动强度。

## 连接示例

根据表格指导，将外设与开发板一一对应连接

| 外设      | 开发板     |
| --------- | ---------- |
| 模块（+） | 3.3V       |
| 模块（-） | GND        |
| 模块（S） | A1（ADC1） |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/20-Simulated_Piezoelectric_Ceramic_Vibration_Sensor
```

### 3. 项目结构

```text
20-Simulated_Piezoelectric_Ceramic_Vibration_Sensor/
├── CMakeLists.txt      # Vibration Sensor Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── touch_demo.c        # 振动传感器 ADC 轮询报警示例源码
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

初始化成功后，可在日志中看到类似输出：

```text
[I/LOG_TAG_DEMO] touch demo task created
[I/LOG_TAG_DEMO] touch demo adc init ok, channel=1 threshold=1500mV poll=200ms
[I/LOG_TAG_DEMO] touch demo started
```

运行期间，示例会在后台任务中持续读取 ADC 电压值，并按默认 200 ms 周期输出当前振动数值或报警状态。典型日志如下：

```text
[I/LOG_TAG_DEMO] vibration value=842mV
[I/LOG_TAG_DEMO] vibration value=1165mV
[I/LOG_TAG_DEMO] vibration alert, value=1680mV
```

在默认配置下，状态判定规则如下：

- `vibration value=xxxmV`：ADC 电压值小于 1500 mV，认为当前振动强度未达到报警阈值
- `vibration alert, value=xxxmV`：ADC 电压值大于等于 1500 mV，认为当前发生了较强振动或冲击

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 touch_demo_init()
    ↓
创建名为 "touch_demo" 的后台任务
    ↓
进入任务主函数 touch_demo_task()
    ↓
调用 touch_demo_open_adc()
    ↓
设置 ADC 量程档位
    ↓
进入周期循环：
  ├─ 调用 touch_demo_read_value() 读取 ADC 电压
  ├─ 调用 touch_demo_check_alert() 与阈值比较
  ├─ 超过阈值时输出报警日志
  └─ 未超过阈值时输出当前振动数值日志
```

### 主要 API 接口

#### touch_demo_init

模块启动入口函数。

- 检查振动监控任务是否已经创建
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出启动日志

#### touch_demo_task

后台任务处理函数。

- 调用 ADC 初始化函数完成量程配置
- 进入长期循环，按固定周期读取 ADC 电压
- 根据阈值判断当前振动是否达到报警条件
- 输出当前振动数值或报警状态日志

#### touch_demo_open_adc

ADC 初始化函数。

- 通过 `qosa_adc_ioctl()` 设置目标 ADC 通道的量程档位
- 在初始化成功后输出当前 ADC 通道、阈值和轮询周期配置
- 在配置失败时输出错误日志并返回失败状态

#### touch_demo_read_value

ADC 读取函数。

- 调用 `qosa_adc_get_volt()` 读取当前 ADC 电压值
- 在读取失败时输出错误日志并返回失败状态
- 在读取成功时返回当前采样电压值

#### touch_demo_check_alert

状态判定函数。

- 将当前 ADC 电压值与报警阈值进行比较
- 当电压值大于等于阈值时返回报警状态
- 当电压值小于阈值时返回非报警状态

## 配置说明

默认振动传感器示例配置定义在 `touch_demo.c` 中，可通过宏进行编译期覆盖：

- `TOUCH_DEMO_TASK_STACK_SIZE`：后台任务栈大小为 2048
- `TOUCH_DEMO_POLL_INTERVAL_MS`：振动监控日志输出周期为 200 ms
- `TOUCH_DEMO_ALERT_THRESHOLD_MV`：默认振动报警阈值为 1500 mV
- `TOUCH_DEMO_ADC_CHANNEL`：默认 ADC 通道为 `QOSA_ADC1_CHANNEL`
- `TOUCH_DEMO_ADC_SCALE`：默认 ADC 量程档位为 `QOSA_ADC_SCALE_LEVEL_2`

如果实际硬件连接的 ADC 输入通道、传感器输出幅值或报警阈值与默认值不同，需要根据原理图和实测数据调整上述宏配置。当前示例采用 ADC 电压阈值判断方式，适合对振动传感器模拟输出进行简单监测，可用于机柜防拆检测、设备跌落检测、门窗振动报警等场景。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎提交 Issue 和 Pull Request。