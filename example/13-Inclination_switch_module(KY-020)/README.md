# 倾斜开关模块

## **模块介绍**

倾斜开关是**姿态感应数字开关器件**，也被称作滚珠开关、倾倒传感器，常用于倾斜检测、防倒保护、姿态触发、智能报警场景；它能在模块倾斜到一定角度时自动切换电平信号，具备体积小、无触点、低功耗、3.3V/5V 兼容、直接 GPIO 检测、响应灵敏、寿命长等优点。

**工作原理：**

模块有正极、负极、信号端。倾斜时内部滚珠 / 导电液移动，使内部触点导通或断开，输出高低电平，开发板可直接读取状态判断是否倾斜。

## 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设          | 开发板 |
| ------------- | ------ |
| 倾斜开关（+） | 3.3V   |
| 倾斜开关（-） | GND    |
| 倾斜开关（S） | PIN31  |
| LED（S）      | PIN32  |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/13-Inclination_switch_module(KY-020)
```

### 3. 项目结构

```text
13-Inclination_switch_module(KY-020)/
├── CMakeLists.txt      # KY-020 Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── ky020_demo.c        # KY-020 倾斜开关示例源代码
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
[I/LOG_TAG_DEMO] ky020 demo init done
[I/LOG_TAG_DEMO] ky020 demo started: sensor pin=31 gpio=31, led pin=32 gpio=32
```

运行期间，示例会在后台任务中持续读取 GPIO 输入电平，并按默认 1000 ms 周期输出当前倾斜状态。典型日志如下：

```text
[I/LOG_TAG_DEMO] 水平状态
[I/LOG_TAG_DEMO] 检测到倾斜
[I/LOG_TAG_DEMO] 检测到倾斜
[I/LOG_TAG_DEMO] 水平状态
```

在默认配置下，状态判定规则如下：

- `水平状态`：GPIO 输入电平不是触发电平，认为当前未发生倾斜，同时关闭 LED
- `检测到倾斜`：GPIO 输入电平等于触发电平，认为当前发生倾斜，同时点亮 LED

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 ky020_demo_init()
    ↓
初始化传感器输入 GPIO 和 LED 输出 GPIO
    ↓
创建名为 "ky020_demo" 的后台任务
    ↓
进入任务主函数 ky020_demo_task()
    ↓
进入周期循环：
  ├─ 调用 qosa_gpio_get_level() 读取倾斜开关输入电平
  ├─ 调用 ky020_is_tilted() 判断当前是否命中触发电平
  ├─ 调用 ky020_set_led() 控制 LED 亮灭
  └─ 输出当前状态日志
```

### 主要 API 接口

#### ky020_demo_init

模块启动入口函数。

- 检查倾斜开关监控任务是否已经创建
- 调用 `ky020_prepare_gpio()` 初始化输入脚和 LED 输出脚
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出初始化完成日志

#### ky020_demo_task

后台任务处理函数。

- 周期读取倾斜开关输入 GPIO 电平
- 根据触发电平判断当前是否处于倾斜状态
- 根据判断结果更新 LED 输出电平
- 输出“检测到倾斜”或“水平状态”日志

#### ky020_prepare_gpio

GPIO 初始化函数。

- 调用 `qosa_get_pin_default_cfg()` 获取目标引脚默认配置
- 调用 `qosa_pin_set_func()` 将引脚切换到 GPIO 功能
- 调用 `qosa_gpio_init()` 按输入或输出模式完成 GPIO 初始化
- 在配置失败时输出错误日志并返回失败状态

#### ky020_is_tilted

状态判定函数。

- 将当前输入电平与 `KY020_TRIGGER_LEVEL` 比较
- 返回当前是否处于倾斜触发状态

#### ky020_set_led

LED 控制函数。

- 根据倾斜状态设置 LED 高低电平
- 当检测到倾斜时点亮 LED
- 当处于水平状态时熄灭 LED

#### ky020_get_inactive_level

输出电平辅助函数。

- 根据 LED 有效电平推导对应的无效电平
- 供 LED 初始化和状态切换时复用

## 配置说明

默认 KY-020 倾斜开关示例配置定义在 `ky020_demo.c` 中，可通过宏进行编译期覆盖：

- `KY020_SENSOR_PIN`：默认传感器输入引脚为 `QOSA_PIN_31`
- `KY020_LED_PIN`：默认 LED 输出引脚为 `QOSA_PIN_32`
- `KY020_TRIGGER_LEVEL`：默认触发电平为 `QOSA_GPIO_LEVEL_LOW`
- `KY020_SENSOR_PULL`：默认输入上下拉配置为 `QOSA_GPIO_PULL_UP`
- `KY020_POLL_INTERVAL_MS`：倾斜检测和日志输出周期为 1000 ms
- `KY020_TASK_STACK_SIZE`：后台任务栈大小为 2048

如果实际硬件连接的输入引脚、LED 引脚或触发电平与默认值不同，需要根据原理图和实测逻辑调整上述宏配置。当前示例采用 GPIO 数字输入轮询方式，适合对 KY-020 这类二值倾斜开关做快速状态检测和指示灯联动控制。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎参与共建，建议按以下方式提交：

- 提交前先执行一次基础验证：env-setup、build、clean。
- 使用清晰的提交说明，描述改动目的、影响范围和验证结果。
- 新增功能或行为变化时，同步更新 README 与相关文档。
- 通过 Issue 或 Pull Request 提交问题修复与功能改进。
