# 水银开关模块

## **模块介绍**

水银开关模块是**重力感应式倾斜 / 倾倒检测数字开关器件**，也叫倾侧开关、角度传感器，常用于倾斜报警、防倒保护、姿态检测、触发控制场景；依靠水银流动导通 / 断开电路，输出稳定高低电平，具有**灵敏度高、导通可靠、无机械触点噪音、3.3V/5V 兼容、GPIO 直读、体积小巧**等优点。

**发光原理：**

模块有正极、负极、信号端。利用水银的导电性与流动性，倾斜到一定角度时，水银流动接通电极，电路导通；复位后水银离开电极，电路断开，开发板通过读取电平判断倾斜状态。

## 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设          | 开发板 |
| ------------- | ------ |
| 水银开关（+） | 3.3V   |
| 水银开关（-） | GND    |
| 水银开关（S） | PIN31  |
| LED（S）      | PIN30  |

## 快速上手

### 1. 开发环境搭建
参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境并完成基本开发流程。

### 2. 代码拉取

```
# 拉取示例仓库
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# 进入该项目
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/18-Mercury_switch_module(KY-017)
```

### 3. 项目结构

```
18-Mercury_switch_module(KY-017)/
├── CMakeLists.txt      # KY-017 Demo 局部构建配置
├── env_config.json     # UniRTOS 工程环境配置
├── ky017_demo.c        # KY017 水银开关示例源代码
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
[I/LOG_TAG_DEMO] ky017 demo init ok, sensor pin=31 output pin=30 trigger=1 pull=1
[I/LOG_TAG_DEMO] ky017 demo started: sensor pin=31 gpio=31, output pin=30 gpio=30
```

运行期间，示例会在后台任务中持续轮询水银开关输入状态，并按默认 1000 ms 周期输出当前倾斜检测结果。典型日志如下：

```
[I/LOG_TAG_DEMO] 水银开关检测到倾斜
[I/LOG_TAG_DEMO] 水银开关未检测到倾斜
[I/LOG_TAG_DEMO] 水银开关检测到倾斜
```

在默认配置下，状态判定规则如下：

- `未检测到倾斜`：输入引脚电平不等于触发电平，联动输出保持低电平
- `检测到倾斜`：输入引脚电平等于触发电平，联动输出拉高

## 代码概览

### 示例工作流程

```
程序启动
    ↓
调用 ky017_demo_init()
    ↓
初始化传感器输入 GPIO 和联动输出 GPIO
    ↓
创建名为 "ky017" 的后台任务
    ↓
进入任务主函数 ky017_demo_task()
    ↓
读取当前传感器输入电平
    ↓
调用 ky017_is_triggered() 判断当前是否触发
    ↓
调用 ky017_set_output() 更新联动输出状态
    ↓
输出当前倾斜检测日志
    ↓
延时 1000 ms 后继续下一轮轮询
```

### 主要 API 接口

#### ky017_demo_init
模块启动入口函数。

- 检查水银开关监控任务是否已经创建
- 初始化输入 GPIO 和输出 GPIO
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在初始化成功后输出当前引脚、触发电平和上下拉配置日志

#### ky017_demo_task
后台任务处理函数。

- 周期性读取传感器输入 GPIO 电平
- 根据触发电平判断当前是否处于倾斜状态
- 调用联动输出函数更新输出引脚状态
- 输出当前检测结果日志

#### ky017_gpio_init
GPIO 初始化函数。

- 调用 `qosa_get_pin_default_cfg()` 获取目标引脚默认配置
- 调用 `qosa_pin_set_func()` 将引脚切换到 GPIO 功能
- 调用 `qosa_gpio_init()` 将引脚初始化为输入或输出模式
- 在初始化失败时返回错误状态

#### ky017_is_triggered
状态判定函数。

- 将当前输入电平与触发电平进行比较
- 返回当前是否处于触发状态

#### ky017_set_output
联动输出控制函数。

- 根据触发状态决定输出高低电平
- 触发时输出有效电平，未触发时输出无效电平

#### ky017_get_inactive_level
无效电平计算函数。

- 根据输出有效电平推导相反的无效电平
- 用于输出 GPIO 上电默认关闭和非触发态控制

## 配置说明
默认 KY017 示例配置定义在 [qos_applications/ky017_demos/ky017_demo.c](qos_applications/ky017_demos/ky017_demo.c) 中，可通过宏进行编译期覆盖：

- `KY017_SENSOR_PIN`：默认传感器输入引脚为 `QOSA_PIN_31`
- `KY017_OUTPUT_PIN`：默认联动输出引脚为 `QOSA_PIN_30`
- `KY017_TRIGGER_LEVEL`：默认触发电平为 `QOSA_GPIO_LEVEL_HIGH`
- `KY017_SENSOR_PULL`：默认输入上下拉配置为 `QOSA_GPIO_PULL_UP`
- `KY017_OUTPUT_ACTIVE_LEVEL`：默认输出有效电平为 `QOSA_GPIO_LEVEL_HIGH`
- `KY017_POLL_INTERVAL_MS`：默认轮询周期为 `1000 ms`
- `KY017_TASK_STACK_SIZE`：后台任务栈大小为 `2048`
- `KY017_TASK_PRIORITY`：后台任务优先级为 `QOSA_PRIORITY_NORMAL`

如果实际硬件连接的输入引脚、输出引脚、触发电平或上下拉方式与默认值不同，需要根据原理图和实物接线调整上述宏配置。当前示例采用 GPIO 轮询方式实现，适合倾覆报警、跌落检测、防盗装置等场景；如果你需要降低轮询开销，也可以进一步改为 GPIO 中断版实现。

## 论坛社区
[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎参与共建，建议按以下方式提交：

- 提交前先执行一次基础验证：env-setup、build、clean。
- 使用清晰的提交说明，描述改动目的、影响范围和验证结果。
- 新增功能或行为变化时，同步更新 README 与相关文档。
- 通过 Issue 或 Pull Request 提交问题修复与功能改进。
