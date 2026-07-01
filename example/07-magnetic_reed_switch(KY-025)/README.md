# KY-025 磁簧开关（干簧管）传感器模块介绍

KY-025是一款基于**磁簧开关（Reed Switch，又称干簧管）**原理的磁控传感器模块。它本质上是一个受磁场控制的微型电气开关，当有磁铁靠近时，内部的金属簧片会吸合导通电路；当磁铁远离时，簧片会自动弹开断开电路。

由于其结构简单、灵敏度高且无需直接接触即可触发，KY-025常被用于各种物联网项目中作为非接触式的接近检测或位置限位装置。

![](../../media/reed1.png)

## 核心特点

- **双重信号输出**：模块同时提供数字量（DO）和模拟量（AO）两种输出接口，既能做简单的开关判断，也能感知磁场强度的相对变化。
- **灵敏度可调**：板载精密电位器（微调旋钮），可以根据实际应用场景旋转调节传感器的探测距离和触发灵敏度。
- **直观的工作指示**：配有电源指示灯和工作状态LED，当检测到磁场触发时，板载LED会亮起，方便调试与观察。
- **宽电压兼容**：通常支持3.3V至5V的宽电压供电，能够完美适配Arduino、STM32以及你手中的QuecDuino等各类主流单片机开发板。

## **引脚说明与接线**

KY-025模块通常引出4个标准引脚，具体的定义如下：

| 引脚名称    | 功能说明     | 接线建议                  |
| :---------- | :----------- | :------------------------ |
| **+ (VCC)** | 电源正极     | 接开发板的 3.3V 或 5V     |
| **G (GND)** | 电源负极     | 接开发板的 GND            |
| **A0**      | 模拟信号输出 | 接开发板的ADC引脚（如A1） |

## 工作原理详解

**模拟输出（A0）**：该引脚输出的电压值会随着磁场强度的变化而线性改变。通常情况下，没有磁场时输出较高数值，随着磁铁逐渐靠近，输出电压会逐渐降低。通过读取这个模拟值，可以大致判断出磁铁与传感器之间的距离远近。

##  常见应用场景

- **门窗防盗报警**：将模块安装在门框，磁铁安装在门扇上，开门即触发警报。
- **智能计数与测速**：在风扇叶片或旋转物体上安装磁铁，每转一圈触发一次，从而计算转速或累计次数。
- **位置限位检测**：在机械臂或移动小车上，用于检测是否到达了预设的物理边界。
- **无触点开关**：作为珠宝盒、礼品盒的开盖亮灯触发器，既隐蔽又耐用。

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/07-magnetic_reed_switch(KY-025)
```

### 3. 项目结构

```text
07-magnetic_reed_switch(KY-025)/
├── CMakeLists.txt      # CMake 构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── adc.c               # KY-025 磁簧开关 ADC 示例源代码
├── Kconfig             # 示例配置开关
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
[I/LOG_TAG_DEMO] magnetic reed demo started
[I/LOG_TAG_DEMO] led init ok, pin=19 gpio=19
[I/LOG_TAG_DEMO] magnetic reed adc init ok, channel=1 threshold=100mV poll=500ms
```

运行期间，示例会在后台任务中持续读取 ADC 电压值，并按默认 500 ms 周期输出当前电压和磁场检测状态。典型日志如下：

```text
[I/LOG_TAG_DEMO] magnetic reed: voltage=42mV | status=idle
[I/LOG_TAG_DEMO] magnetic reed: voltage=118mV | status=detected
[I/LOG_TAG_DEMO] magnetic reed: voltage=135mV | status=detected
```

在默认配置下，状态判定规则如下：

- `idle`：ADC 电压值小于等于 100 mV，认为当前未检测到明显磁场
- `detected`：ADC 电压值大于 100 mV，认为当前检测到磁场，同时点亮 LED

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 magnetic_reed_demo_init()
    ↓
创建名为 "mag_reed" 的后台任务
    ↓
进入任务主函数 magnetic_reed_demo_task()
    ↓
调用 magnetic_reed_led_init()
    ↓
配置 LED 对应引脚为 GPIO 输出模式
    ↓
调用 magnetic_reed_adc_init()
    ↓
设置 ADC 量程档位
    ↓
进入周期循环：
  ├─ 调用 qosa_adc_get_volt() 读取 ADC 电压
  ├─ 与阈值比较判断是否检测到磁场
  ├─ 调用 magnetic_reed_set_led() 控制 LED 亮灭
  └─ 输出当前电压和状态日志
```

### 主要 API 接口

#### magnetic_reed_demo_init

模块启动入口函数。

- 检查磁簧开关监控任务是否已经创建
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出启动日志

#### magnetic_reed_demo_task

后台任务处理函数。

- 调用 LED 初始化函数完成指示灯 GPIO 配置
- 调用 ADC 初始化函数完成量程配置
- 进入长期循环，按固定周期读取 ADC 电压
- 根据阈值判断当前磁场状态并更新 LED
- 输出当前电压和状态日志

#### magnetic_reed_led_init

LED 初始化函数。

- 调用 `qosa_get_pin_default_cfg()` 获取 LED 引脚默认配置
- 调用 `qosa_pin_set_func()` 将引脚切换到 GPIO 功能
- 调用 `qosa_gpio_init()` 将对应 GPIO 初始化为输出模式
- 在初始化成功后记录 GPIO 编号并输出日志

#### magnetic_reed_adc_init

ADC 初始化函数。

- 通过 `qosa_adc_ioctl()` 设置目标 ADC 通道的量程档位
- 在初始化成功后输出当前 ADC 通道、阈值和轮询周期配置
- 在配置失败时输出错误日志并返回失败状态

#### magnetic_reed_set_led

LED 控制函数。

- 根据磁场检测结果设置 LED 高低电平
- 当状态为 `detected` 时点亮 LED
- 当状态为 `idle` 时熄灭 LED

#### magnetic_reed_status_name

状态判定函数。

- 将当前 ADC 电压值与阈值进行比较
- 返回 `idle` 或 `detected` 状态字符串

## 配置说明

默认磁簧开关示例配置定义在 `adc.c` 中，可通过宏进行编译期覆盖：

- `MAGNETIC_REED_ADC_CHANNEL`：默认 ADC 通道为 `QOSA_ADC1_CHANNEL`
- `MAGNETIC_REED_THRESHOLD_MV`：默认磁场检测阈值为 100 mV
- `MAGNETIC_REED_LED_PIN_NUM`：默认 LED 引脚为 `QOSA_PIN_19`
- `MAGNETIC_REED_ADC_SCALE`：默认 ADC 量程档位为 `QOSA_ADC_SCALE_LEVEL_2`
- `MAGNETIC_REED_POLL_MS`：磁场监控日志输出周期为 500 ms
- `MAGNETIC_REED_TASK_STACK_SIZE`：后台任务栈大小为 2048

如果实际硬件连接的 ADC 输入通道、LED 引脚或传感器输出阈值与默认值不同，需要根据原理图和实测数据调整上述宏配置。当前示例采用 ADC 电压阈值判断方式，适合对 KY-025 模块模拟输出变化进行简单检测；如果你的板级设计更适合使用数字输出口进行高低电平判定，可进一步改为 GPIO 输入版实现。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎参与共建，建议按以下方式提交：

- 提交前先执行一次基础验证：env-setup、build、clean。
- 使用清晰的提交说明，描述改动目的、影响范围和验证结果。
- 新增功能或行为变化时，同步更新 README 与相关文档。
- 通过 Issue 或 Pull Request 提交问题修复与功能改进。
