# 水位检测模块

## **模块介绍**

水位监测模块是**电阻式液体检测传感器**，用于检测水位高度、有无水、漏水报警等场景；通过导电探针检测液面变化，输出模拟，具备**响应快、体积小、3.3V兼容、直接接 ADC、使用寿命长**等优点。

**工作原理：**

Water Sensor水位传感器能够监测水位。该模块主要是利用三极管的电流放大原理：当液位高度使三极管的基极与电源正极导通的时候，在三极管的基极和发射极之间就会产生一定大小的电流，此时在三极管的集电极和发射极之间就会产生一个一定放大倍数的电流，该电流经过发射极的电阻产生特点电压，被AD转换器采集。

## 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

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
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/06-water_level_detect
```

### 3. 项目结构

```text
06-water_level_detect/
├── CMakeLists.txt      # Water Level Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── water_demo.c        # 水位传感器示例源代码
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
[I/LOG_TAG_DEMO] water level demo started
[I/LOG_TAG_DEMO] water level adc init ok, channel=1 ref=3300mV max=60mm warn=15mm alert=35mm samples=10
```

运行期间，示例会在后台任务中持续执行 ADC 多次采样求均值，并按默认 1 秒周期输出当前水位、电压和状态。典型日志如下：

```text
[I/LOG_TAG_DEMO] water level: 8.36 mm | voltage: 459.8 mV | status: normal
[I/LOG_TAG_DEMO] water level: 17.42 mm | voltage: 958.0 mV | status: warning
[I/LOG_TAG_DEMO] water level: 39.31 mm | voltage: 2162.0 mV | status: alert
```

在默认配置下，状态分级规则如下：

- `normal`：水位低于 15 mm
- `warning`：水位大于等于 15 mm 且低于 35 mm
- `alert`：水位大于等于 35 mm

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 water_level_demo_init()
    ↓
创建名为 "water_level" 的后台任务
    ↓
进入任务主函数 water_level_demo_task()
    ↓
调用 water_level_adc_init()
    ↓
设置 ADC 量程档位
    ↓
进入周期循环：
  ├─ 调用 water_level_read_voltage_avg_tenth_mv()
  ├─ 连续采样多次并计算平均电压
  ├─ 调用 water_level_convert_to_hundredths_mm() 换算水位
  ├─ 调用 water_level_status_name() 判断状态等级
  └─ 输出当前水位、电压和状态日志
```

### 主要 API 接口

#### water_level_demo_init

模块启动入口函数。

- 检查水位监控任务是否已经创建
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出启动日志

#### water_level_demo_task

后台任务处理函数。

- 调用 ADC 初始化函数完成量程配置
- 进入长期循环，按固定周期执行采样
- 读取平均电压并换算当前水位高度
- 输出当前水位、电压和状态日志

#### water_level_adc_init

ADC 初始化函数。

- 通过 `qosa_adc_ioctl()` 设置目标 ADC 通道的量程档位
- 在初始化成功后输出当前参考电压、量程和阈值配置
- 在配置失败时输出错误日志并返回失败状态

#### water_level_read_voltage_avg_tenth_mv

ADC 均值采样函数。

- 调用 `qosa_adc_get_volt()` 读取目标 ADC 通道电压值
- 按设定次数循环采样并累加毫伏值
- 在每次采样之间按设定间隔短暂休眠
- 输出结果为平均电压，单位为 0.1 mV

#### water_level_convert_to_hundredths_mm

水位换算函数。

- 按线性公式将平均电压映射到水位高度
- 根据参考电压和满量程配置计算结果
- 输出结果保留两位小数，单位为 0.01 mm

#### water_level_status_name

状态分级函数。

- 将换算后的水位与告警阈值进行比较
- 返回 `normal`、`warning` 或 `alert` 状态字符串

## 配置说明

默认水位传感器配置定义在 `water_demo.c` 中，可通过宏进行编译期覆盖：

- `WATER_LEVEL_ADC_CHANNEL`：默认 ADC 通道为 `QOSA_ADC1_CHANNEL`
- `WATER_LEVEL_ADC_REF_MV`：默认参考电压为 3300 mV
- `WATER_LEVEL_MAX_MM`：默认水位满量程为 60 mm
- `WATER_LEVEL_WARN_MM`：默认预警阈值为 15 mm
- `WATER_LEVEL_ALERT_MM`：默认报警阈值为 35 mm
- `WATER_LEVEL_SAMPLE_COUNT`：每轮均值采样次数为 10 次
- `WATER_LEVEL_SAMPLE_INTERVAL_MS`：两次采样间隔为 5 ms
- `WATER_LEVEL_ADC_SCALE`：默认 ADC 量程档位为 `QOSA_ADC_SCALE_LEVEL_2`
- `WATER_LEVEL_DEMO_POLL_MS`：水位监控日志输出周期为 1000 ms
- `WATER_LEVEL_DEMO_TASK_STACK_SIZE`：后台任务栈大小为 2048

如果实际水位传感器的输出电压范围、ADC 输入通道或标定量程与默认值不同，需要根据硬件原理图和传感器特性调整上述宏配置。当前示例采用线性换算方式，适用于传感器输出电压与水位高度近似成比例的场景；如果传感器存在非线性区间，建议结合实测数据进一步标定换算公式。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎参与共建，建议按以下方式提交：

- 提交前先执行一次基础验证：env-setup、build、clean。
- 使用清晰的提交说明，描述改动目的、影响范围和验证结果。
- 新增功能或行为变化时，同步更新 README 与相关文档。
- 通过 Issue 或 Pull Request 提交问题修复与功能改进。
