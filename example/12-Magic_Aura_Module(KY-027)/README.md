# 魔术光环模块

## **模块介绍**

魔术光环模块（KY‑027）是**倾斜感应 + LED 发光**二合一数字模块，内置水银开关与高亮 LED，用于倾斜检测、姿态触发、状态指示、创客互动项目；模块体积小、响应快、数字电平输出、3.3V/5V 兼容、直接接 GPIO 驱动、寿命稳定。

**工作原理：**

![](../../media/magic1.png)

模块有供电、接地、信号输出、LED 控制端。倾斜到一定角度时，水银开关导通 / 断开，输出高低电平；可通过 GPIO 控制 LED 亮灭，实现倾斜亮灯、姿态报警等效果。

## 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设          | 开发板 |
| ------------- | ------ |
| 魔术光环（+） | 3.3V   |
| 魔术光环（-） | GND    |
| 魔术光环（S） | PIN23  |
| 魔术光环（L） | PIN22  |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/12-Magic_Aura_Module(KY-027)
```

### 3. 项目结构

```text
12-Magic_Aura_Module(KY-027)/
├── CMakeLists.txt      # KY-027 Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── magic_demo.c        # 倾斜开关联动输出示例源代码
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
[I/LOG_TAG_DEMO] tilt switch demo init done
[I/LOG_TAG_DEMO] tilt switch demo started: sensor pin=23 gpio=23, output pin=22 gpio=22
```

运行期间，示例会在后台任务中持续读取 GPIO 输入电平，并按默认 1000 ms 周期输出当前姿态状态。典型日志如下：

```text
[I/LOG_TAG_DEMO] 位置正常
[I/LOG_TAG_DEMO] 检测到倾斜
[I/LOG_TAG_DEMO] 检测到倾斜
[I/LOG_TAG_DEMO] 位置正常
```

在默认配置下，状态判定规则如下：

- `位置正常`：输入引脚读取到低电平，认为当前未触发倾斜，联动输出关闭
- `检测到倾斜`：输入引脚读取到高电平，认为当前发生倾斜，联动输出拉高

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 magic_halo_demo_init()
    ↓
初始化倾斜开关输入 GPIO 和联动输出 GPIO
    ↓
创建名为 "tilt_switch_demo" 的后台任务
    ↓
进入任务主函数 tilt_switch_demo_task()
    ↓
进入周期循环：
  ├─ 调用 qosa_gpio_get_level() 读取倾斜开关输入电平
  ├─ 调用 tilt_switch_is_tilted() 判断是否命中触发电平
  ├─ 调用 tilt_switch_set_output() 控制联动输出高低电平
  └─ 输出当前状态日志
```

### 主要 API 接口

#### magic_halo_demo_init

模块启动入口函数。

- 检查倾斜开关监控任务是否已经创建
- 调用 `tilt_switch_prepare_gpio()` 初始化输入脚和输出脚
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出启动完成日志

#### tilt_switch_demo_task

后台任务处理函数。

- 周期读取倾斜开关输入 GPIO 电平
- 根据触发电平判断当前是否处于倾斜状态
- 根据判断结果更新联动输出电平
- 输出“检测到倾斜”或“位置正常”日志

#### tilt_switch_prepare_gpio

GPIO 初始化函数。

- 调用 `qosa_get_pin_default_cfg()` 获取目标引脚默认配置
- 调用 `qosa_pin_set_func()` 将引脚切换到 GPIO 功能
- 调用 `qosa_gpio_init()` 按输入或输出模式完成 GPIO 初始化
- 在配置失败时输出错误日志并返回失败状态

#### tilt_switch_is_tilted

状态判定函数。

- 将当前输入电平与 `TILT_SWITCH_TRIGGER_LEVEL` 比较
- 返回当前是否处于倾斜触发状态

#### tilt_switch_set_output

联动输出控制函数。

- 根据倾斜状态设置输出引脚高低电平
- 当检测到倾斜时输出有效电平
- 当位置正常时输出无效电平

#### tilt_switch_get_inactive_level

输出电平辅助函数。

- 根据输出有效电平推导对应的无效电平
- 供输出初始化和状态切换时复用

## 配置说明

默认倾斜开关示例配置定义在 `magic_demo.c` 中，可通过宏进行编译期覆盖：

- `TILT_SWITCH_SENSOR_PIN`：默认输入引脚为 `QOSA_PIN_23`
- `TILT_SWITCH_OUTPUT_PIN`：默认联动输出引脚为 `QOSA_PIN_22`
- `TILT_SWITCH_TRIGGER_LEVEL`：默认触发电平为 `QOSA_GPIO_LEVEL_HIGH`
- `TILT_SWITCH_OUTPUT_ACTIVE`：默认输出有效电平为 `QOSA_GPIO_LEVEL_HIGH`
- `TILT_SWITCH_POLL_INTERVAL_MS`：姿态轮询和日志输出周期为 1000 ms
- `TILT_SWITCH_TASK_STACK_SIZE`：后台任务栈大小为 2048

如果实际硬件连接的输入引脚、联动输出引脚或有效电平与默认值不同，需要根据原理图和实测逻辑调整上述宏配置。当前示例采用 GPIO 数字输入轮询方式，适合对倾斜开关这类高低电平型传感器做快速联动控制。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎提交 Issue 和 Pull Request。