# 迷你磁簧模块

## **模块介绍**

迷你磁簧，全称**迷你磁簧开关（干簧管模块）**，是一种利用磁场控制通断的无源开关组件，这类磁性感应器件一般作为门磁检测、位置检测、限位触发使用，目前已经广泛用于嵌入式设备、智能硬件、创客 DIY 场景；它能够在磁场靠近时导通、磁场远离时断开，拥有体积小、响应快、无机械触点磨损、低功耗、即插即用、适配 3.3V/5V 低压环境、可直接接 GPIO 检测、使用寿命长等优点。

迷你磁环组成：

![](../../media/mini1.png)

**工作原理：**

模块本质是一个受磁场控制的开关。当磁铁靠近模块时，玻璃管内的磁簧片被磁化并相互吸引接触，电路导通；当磁铁远离时，簧片失去磁性并依靠弹性分离，电路断开，以此实现磁场触发的开关信号输出。

## 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设       | 开发板 |
| ---------- | ------ |
| 磁簧（+）  | 3.3V   |
| 磁簧（-）  | GND    |
| 磁簧（S）  | PIN23  |
| LED等（S） | PIN22  |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/09-Mini_Magnetic(KY-021)
```

### 3. 项目结构

```text
09-Mini_Magnetic(KY-021)/
├── CMakeLists.txt      # KY-021 Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── ky021_demo.c        # KY-021 迷你磁簧传感器 GPIO 轮询示例源代码
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
[I/LOG_TAG_DEMO] KY-021 demo initializing...
[I/LOG_TAG_DEMO] KY-021 demo initialized successfully, sensor_pin=31 output_pin=30
[I/LOG_TAG_DEMO] KY-021 mini magnetic controller started
```

运行期间，示例会在后台任务中持续读取 GPIO 输入电平，并按默认 500 ms 周期输出当前磁场检测状态，同时联动控制输出引脚。典型日志如下：

```text
[I/LOG_TAG_DEMO] 检测到磁场变化
[I/LOG_TAG_DEMO] [MiniMagnetic] 触发事件
[I/LOG_TAG_DEMO] 未检测到磁场变化
[I/LOG_TAG_DEMO] [MiniMagnetic] 释放事件
```

在默认配置下，状态判定规则如下：

- `triggered`：输入引脚检测到低电平，认为当前检测到磁场，同时将输出引脚拉高
- `released`：输入引脚为高电平，认为当前未检测到磁场，同时将输出引脚拉低

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 ky021_demo_init()
    ↓
配置传感器输入引脚 GPIO31 为上拉输入模式
    ↓
配置联动输出引脚 GPIO30 为推挽输出模式
    ↓
创建名为 "ky021_task" 的后台任务
    ↓
进入任务主函数 ky021_task_entry()
    ↓
读取传感器当前电平并保存初始状态
    ↓
进入周期循环：
  ├─ 调用 qosa_gpio_get_level() 读取传感器输入电平
  ├─ 调用 ky021_is_triggered() 判断是否命中触发电平
  ├─ 调用 ky021_set_output() 联动控制输出引脚
  ├─ 输出“检测到磁场变化”或“未检测到磁场变化”
  └─ 当状态变化时输出触发事件或释放事件日志
```

### 主要 API 接口

#### ky021_demo_init

模块启动入口函数。

- 检查 KY-021 监控任务是否已经创建
- 初始化传感器输入 GPIO 和联动输出 GPIO
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出初始化完成日志

#### ky021_task_entry

后台任务处理函数。

- 读取当前传感器输入电平作为初始状态
- 进入长期循环，按固定周期轮询 GPIO 输入状态
- 根据触发电平判断当前是否检测到磁场
- 更新输出引脚电平，实现输入事件联动控制
- 输出当前状态日志，并在状态变化时输出事件日志

#### ky021_gpio_init

GPIO 初始化辅助函数。

- 调用 `qosa_get_pin_default_cfg()` 获取目标引脚默认配置
- 调用 `qosa_pin_set_func()` 将引脚切换到 GPIO 功能
- 调用 `qosa_gpio_init()` 完成输入或输出模式初始化
- 在初始化失败时输出错误日志并返回失败状态

#### ky021_set_output

输出控制函数。

- 根据当前传感器触发状态设置输出引脚电平
- 当状态为触发时输出激活电平
- 当状态为未触发时输出非激活电平

#### ky021_is_triggered

状态判定函数。

- 将当前输入电平与触发电平进行比较
- 返回当前是否处于触发状态

#### ky021_get_inactive_level

输出电平换算函数。

- 根据配置的输出激活电平推导非激活电平
- 用于初始化输出默认状态和运行时输出控制

## 配置说明

默认 KY-021 磁簧传感器示例配置定义在 `ky021_demo.c` 中，可通过宏进行编译期覆盖：

- `KY021_SENSOR_PIN`：默认传感器输入引脚为 `QOSA_PIN_23`
- `KY021_OUTPUT_PIN`：默认联动输出引脚为 `QOSA_PIN_22`
- `KY021_TRIGGER_LEVEL`：默认触发电平为 `QOSA_GPIO_LEVEL_LOW`
- `KY021_OUTPUT_ACTIVE_LEVEL`：默认输出激活电平为 `QOSA_GPIO_LEVEL_HIGH`
- `KY021_POLL_INTERVAL_MS`：状态轮询周期为 500 ms
- `KY021_TASK_STACK_SIZE`：后台任务栈大小为 4096
- `KY021_TASK_PRIORITY`：后台任务优先级为 100
- `KY021_TASK_NAME`：后台任务名称为 `"ky021_task"`

如果实际硬件连接的传感器输入引脚、联动输出引脚或触发电平与默认值不同，需要根据原理图和实测结果调整上述宏配置。当前示例采用 GPIO 数字输入判定方式，适合 KY-021 数字输出场景；如果你的板级设计更适合读取模拟量变化，可参考 ADC 版本的磁簧开关示例进行扩展。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎参与共建，建议按以下方式提交：

- 提交前先执行一次基础验证：env-setup、build、clean。
- 使用清晰的提交说明，描述改动目的、影响范围和验证结果。
- 新增功能或行为变化时，同步更新 README 与相关文档。
- 通过 Issue 或 Pull Request 提交问题修复与功能改进。
