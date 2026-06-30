# 温湿度传感器

## **模块介绍**

温湿度传感器作为常见的传感器之一，是一种装有湿敏和热敏元件，能够用来测量温度和湿度的传感器装置。其工作原理主要基于热敏电阻和湿敏电阻的特性，通过测量电阻值并转换成电压信号输出，实现对环境温湿度的准确监测。

**发光原理：**

模块通过内部热敏元件与湿敏元件采集环境数据，经芯片校准后以**I2C 数字信号**输出，开发板通过 I2C 总线读取温度和湿度数值。

## 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设         | 开发板 |
| ------------ | ------ |
| AHT20（+）   | 3.3V   |
| AHT20（-）   | GND    |
| AHT20（SCL） | PIN67  |
| AHT20（SDA） | PIN66  |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/19-temperature_and_humidity_sensor(AHT20)
```

### 3. 项目结构

```text
19-temperature_and_humidity_sensor(AHT20)/
├── CMakeLists.txt      # AHT20 Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── Kconfig             # 示例配置开关
├── aht20_demo.c        # AHT20 温湿度传感器示例源代码
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
[I/LOG_TAG_DEMO] AHT20 demo thread created
[I/LOG_TAG_DEMO] AHT20 demo started on I2C channel 1 slave 0x38
```

运行期间，示例会在后台任务中持续读取 AHT20 传感器数据，并按默认 1 s 周期输出当前温度、湿度和环境状态。典型日志如下：

```text
[I/LOG_TAG_DEMO] 温度: 24.6C | 湿度: 45.3% | 状态: 舒适
[I/LOG_TAG_DEMO] 温度: 16.8C | 湿度: 41.2% | 状态: 偏冷
[I/LOG_TAG_DEMO] 温度: 27.1C | 湿度: 75.6% | 状态: 偏潮湿
```

如果测量触发或采样读取失败，还会看到类似告警日志：

```text
[W/LOG_TAG_DEMO] 读取失败
```

在默认配置下，环境状态判定规则如下：

- `偏冷`：温度低于 18.0°C
- `偏热`：温度高于 28.0°C
- `偏干燥`：温度处于 18.0°C 到 28.0°C 之间，且湿度低于 30.0%
- `偏潮湿`：温度处于 18.0°C 到 28.0°C 之间，且湿度高于 70.0%
- `舒适`：温度与湿度都处于默认舒适区间内

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 aht20_demo_init()
    ↓
创建名为 "aht20_demo" 的后台任务
    ↓
进入任务主函数 aht20_demo_thread()
    ↓
调用 aht20_configure_bus()
    ↓
配置 SDA/SCL 引脚复用并初始化 I2C 总线
    ↓
调用 aht20_reset_sensor()
    ↓
向 AHT20 发送软复位命令
    ↓
调用 aht20_initialize_sensor()
    ↓
发送初始化命令并等待传感器就绪
    ↓
进入周期循环：
  ├─ 调用 aht20_read_measurement() 触发一次测量并读取原始数据
  ├─ 将原始数据换算为温度和湿度
  ├─ 调用 aht20_check_comfort() 判断环境状态
  └─ 输出当前温度、湿度和状态日志
```

### 主要 API 接口

#### aht20_demo_init

模块启动入口函数。

- 检查 AHT20 监控任务是否已经创建
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出启动日志

#### aht20_demo_thread

后台任务处理函数。

- 调用 I2C 总线配置函数完成引脚复用和总线初始化
- 依次执行传感器复位和初始化流程
- 进入长期循环，按固定周期触发测量并读取数据
- 根据温湿度结果判断当前环境状态
- 输出当前温度、湿度和状态日志

#### aht20_configure_bus

I2C 总线初始化函数。

- 调用 `qosa_pin_set_func()` 配置 SDA 和 SCL 引脚复用功能
- 调用 `qosa_i2c_init()` 初始化目标 I2C 通道
- 在配置失败时输出错误日志并返回失败状态

#### aht20_reset_sensor

传感器复位函数。

- 调用 `qosa_i2c_write()` 向 AHT20 发送软复位命令
- 复位成功后等待默认 20 ms
- 在复位失败时输出错误日志并返回失败状态

#### aht20_initialize_sensor

传感器初始化函数。

- 调用 `qosa_i2c_write()` 向 AHT20 发送初始化命令
- 在初始化失败时输出错误日志并返回失败状态

#### aht20_read_measurement

测量读取函数。

- 调用 `qosa_i2c_write()` 触发一次温湿度测量
- 等待默认 80 ms 后调用 `qosa_i2c_read()` 读取 6 字节测量结果
- 检查状态位，确认传感器当前不处于 busy 状态
- 将原始数据换算为温度和湿度的十分位数值

#### aht20_check_comfort

环境状态判定函数。

- 优先根据温度判断是否 `偏冷` 或 `偏热`
- 当温度处于舒适区间时，再根据湿度判断是否 `偏干燥` 或 `偏潮湿`
- 若温湿度均在默认阈值范围内，则返回 `舒适`

## 配置说明

默认 AHT20 示例配置定义在 `aht20_demo.c` 中，当前通过宏固定，可根据实际硬件连接和应用需求直接调整：

- `AHT20_I2C_ADDR_7BIT`：默认从设备 7 位地址为 `0x38`
- `AHT20_CMD_RESET`：软复位命令字为 `0xBA`
- `AHT20_CMD_INIT`：初始化命令字为 `0xE1`
- `AHT20_CMD_MEASURE`：测量触发命令字为 `0xAC`
- `AHT20_RESET_DELAY_MS`：复位后等待时间为 20 ms
- `AHT20_INIT_DELAY_MS`：初始化后等待时间为 1000 ms
- `AHT20_MEASURE_DELAY_MS`：触发测量后等待时间为 80 ms
- `AHT20_POLL_INTERVAL_MS`：日志输出周期为 1000 ms
- `AHT20_THREAD_STACK_SIZE`：后台任务栈大小为 2048
- `AHT20_I2C_SDA_PIN`：默认 SDA 引脚为 `QOSA_PIN_66`
- `AHT20_I2C_SCL_PIN`：默认 SCL 引脚为 `QOSA_PIN_67`
- `AHT20_I2C_FUNC`：默认引脚复用功能号为 `2`
- `AHT20_I2C_CHANNEL`：默认 I2C 通道为 `QOSA_I2C_1`

如果实际硬件连接的 I2C 通道、引脚复用功能、传感器地址或采样周期与默认值不同，需要根据原理图和实测情况调整上述宏配置。当前示例采用轮询方式定期读取 AHT20 数据，并在应用层完成基础舒适度判断，适合作为温湿度采样和 I2C 外设接入的入门参考。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎提交 Issue 和 Pull Request。