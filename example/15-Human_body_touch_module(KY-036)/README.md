# 人体触摸模块

## **模块介绍**

该模块是一个基于触摸检测的电容式点动型触摸开关模块。·金属触摸模块是通过人体的电容来作出反应的。由于其是监测电容，还可以在模块表面覆盖非金属材料如木材、纸、塑料等等jue缘材料，来检测人的触摸可做成隐藏在墙壁、桌面等地方的按键。

**模块组成：**

![](../../media/finger1.png) 

**发光原理：**

模块有正极、负极、信号端。人体触摸感应片时，电容值发生变化，模块内部电路识别后输出高低电平信号，开发板可直接读取状态判断是否被触摸。

## 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设      | 开发板 |
| --------- | ------ |
| 模块（+） | 3.3V   |
| 模块（-） | GND    |
| 模块（S） | PIN31  |

## 快速上手

### 1. 开发环境搭建
参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/15-Human_body_touch_module(KY-036)
```

### 3. 项目结构

```
15-Human_body_touch_module(KY-036)/
├── CMakeLists.txt      # KY-036 Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── ky036_demo.c        # KY-036 人体触摸传感器示例源代码
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

```
[I/LOG_TAG_DEMO] KY036: Touch sensor demo initializing...
[I/LOG_TAG_DEMO] KY036: DO pin = PIN_31
[I/LOG_TAG_DEMO] KY036: gpio_num=31, gpio_func=0
[I/LOG_TAG_DEMO] KY036: Touch sensor demo initialized successfully!
[I/LOG_TAG_DEMO] KY036: trigger_level=1, pull=2
```

运行期间，示例会在后台任务中持续读取 GPIO 输入电平，并按默认 1000 ms 周期输出当前触摸状态。典型日志如下：

```
[I/LOG_TAG_DEMO] KY036: Poll task started, interval=1000ms
[I/LOG_TAG_DEMO] 未检测到触摸
[I/LOG_TAG_DEMO] 检测到触摸
[I/LOG_TAG_DEMO] 检测到触摸
```

在默认配置下，状态判定规则如下：

- `未检测到触摸`：GPIO 当前电平不等于触发电平，认为当前无触摸输入
- `检测到触摸`：GPIO 当前电平等于触发电平，认为当前检测到人体触摸

## 代码概览

### 示例工作流程

```
程序启动
    ↓
调用 ky036_demo_init()
    ↓
获取 DO 引脚默认配置并切换为 GPIO 功能
    ↓
调用 qosa_gpio_init() 配置输入模式和下拉
    ↓
创建名为 "ky036_poll" 的后台任务
    ↓
进入任务主函数 ky036_poll_task()
    ↓
进入周期循环：
  ├─ 调用 ky036_read_state() 读取 GPIO 电平
  ├─ 调用 ky036_is_touched() 判断是否为触摸状态
  ├─ 输出“检测到触摸”或“未检测到触摸”日志
  └─ 调用 qosa_task_sleep_ms() 延时 1000 ms
```

### 主要 API 接口

#### ky036_demo_init
模块启动入口函数。

- 获取 KY-036 DO 引脚的默认 pinmux 和 GPIO 映射配置
- 调用 `qosa_pin_set_func()` 将引脚切换到 GPIO 功能
- 调用 `qosa_gpio_init()` 将对应 GPIO 初始化为输入模式
- 创建后台轮询任务并输出初始化日志

#### ky036_poll_task
后台任务处理函数。

- 周期性读取传感器数字输出电平
- 调用触摸状态判断函数确认当前是否被触摸
- 每次轮询都输出当前状态日志
- 通过 `qosa_task_sleep_ms()` 控制轮询周期

#### ky036_read_state
GPIO 电平读取函数。

- 调用 `qosa_gpio_get_level()` 获取当前 GPIO 输入电平
- 在读取失败时输出错误日志并返回失败状态

#### ky036_is_touched
触摸状态判定函数。

- 将当前 GPIO 电平与触发电平进行比较
- 返回 `QOSA_TRUE` 或 `QOSA_FALSE` 表示是否检测到触摸

## 配置说明
默认人体触摸传感器示例配置定义在 `ky036_demo.c` 中，可通过宏进行编译期覆盖：

- `KY036_DO_PIN_NUM`：默认输入引脚为 `QOSA_PIN_31`
- `KY036_TRIGGER_LEVEL`：默认触发电平为 `QOSA_GPIO_LEVEL_HIGH`
- `KY036_GPIO_PULL`：默认上下拉配置为 `QOSA_GPIO_PULL_DOWN`
- `KY036_POLL_INTERVAL_MS`：默认轮询与日志输出周期为 1000 ms
- `KY036_TASK_STACK_SIZE`：后台任务栈大小为 4 KB
- `KY036_TASK_PRIORITY`：后台任务优先级为 100

如果实际硬件连接的触摸传感器引脚、触发电平或上下拉方式与默认值不同，需要根据原理图和模块特性调整上述宏配置。当前示例采用数字输入轮询方式，适合对 KY-036 模块的 DO 输出做简单触摸检测；如果你的板级设计更适合事件驱动处理，也可以进一步扩展为 GPIO 中断版实现。

## 论坛社区
[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南
欢迎提交 Issue 和 Pull Request。