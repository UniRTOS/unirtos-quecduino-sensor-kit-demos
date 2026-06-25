# 障碍物检测模块

## **一、** **模块介绍**

障碍物检测模块是红外反射式数字检测器件，也叫红外避障模块，用于近距离障碍物检测、循迹、避障、限位触发；通过红外发射与接收判断前方是否有障碍物，具备响应快、体积小、3.3V/5V 兼容、GPIO 直读、抗干扰强、寿命长等优点。

**模块组成：**

![](../../media/obstacle1.png)

**工作原理：**

工作原理是红外光 线发射管**发射红外光线**，红外光线接收管**接收红外光线**，当**没有接收到返回的红外光线**时，OUT引脚输出**高电平**，当**接收到返回的红外光线时**，OUT引脚输出**低电平**。

## 二、 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设      | 开发板 |
| --------- | ------ |
| 模块（+） | 3.3V   |
| 模块（-） | GND    |
| 模块（S） | PIN23  |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境和完成基础开发流程。

### 2. 项目结构

```text
ky032_demps/
├── ky032_demo.c        # KY-032 红外避障传感器示例源代码
├── CMakeLists.txt      # CMake 构建配置
├── Kconfig             # 示例配置开关
└── README.md           # 本文件
```

### 3. 构建项目

当前目录中的示例源码和子目录 CMakeLists 已就绪；如需作为内置应用参与整仓构建，需要在菜单配置中使能 `QAPP_KY032_DEMO`。完成接入并使能后，可在 UniRTOS 根目录执行类似命令进行构建：

```bash
unirtos make EG800ZCN_LA EG800ZCNLAR01A01M04_BETA_OCPU_20260511
```

### 4. 日志展示

初始化成功后，可在日志中看到类似输出：

```text
[I/LOG_TAG_DEMO] ky032 demo started
[I/LOG_TAG_DEMO] ky032 ready, pin=23 gpio=23 active_level=0 sample=100ms mode=polling
[I/LOG_TAG_DEMO] ky032 polling mode start
```

轮询模式下，示例会在后台任务中持续读取 KY-032 数字输出电平，并按默认 100 ms 周期输出当前检测状态。典型日志如下：

```text
[I/LOG_TAG_DEMO] ky032 no obstacle
[W/LOG_TAG_DEMO] ky032 detect obstacle
[I/LOG_TAG_DEMO] ky032 no obstacle
```

如果启用了中断模式 `QAPP_KY032_USE_INTERRUPT`，初始化后可看到类似输出：

```text
[I/LOG_TAG_DEMO] ky032 ready, pin=23 gpio=23 active_level=0 sample=100ms mode=interrupt
[I/LOG_TAG_DEMO] ky032 interrupt mode start
[W/LOG_TAG_DEMO] ky032 detect obstacle
```

在默认配置下，状态判定规则如下：

- `no obstacle`：传感器输出电平不等于有效电平，认为当前无障碍物
- `detect obstacle`：传感器输出电平等于有效电平，认为当前检测到障碍物

当前默认有效电平为低电平，即：

- 无障碍物时，OUT 输出高电平
- 检测到障碍物时，OUT 输出低电平

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 ky032_demo_init()
    ↓
创建名为 "ky032_demo" 的后台任务
    ↓
进入任务主函数 ky032_demo_task()
    ↓
调用 ky032_prepare_input_pin()
    ↓
配置传感器引脚为 GPIO 输入模式并开启上拉
    ↓
根据配置选择运行模式：
  ├─ 轮询模式
  │   ├─ 调用 ky032_read_state() 读取 GPIO 电平
  │   ├─ 调用 ky032_is_obstacle() 判断是否检测到障碍物
  │   └─ 调用 ky032_report_state() 输出状态日志
  └─ 中断模式
      ├─ 调用 ky032_enable_interrupt_mode() 注册 GPIO 中断
      ├─ 障碍物触发时由 ky032_gpio_irq_handler() 置位标志
      └─ 主循环检查标志位并输出检测结果
```

### 主要 API 接口

#### ky032_demo_init

模块启动入口函数。

- 检查 KY-032 监控任务是否已经创建
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出启动日志

#### ky032_demo_task

后台任务处理函数。

- 调用 `ky032_prepare_input_pin()` 完成传感器 GPIO 初始化
- 根据 `QAPP_KY032_USE_INTERRUPT` 配置选择轮询模式或中断模式
- 进入对应监控流程并持续输出检测状态

#### ky032_prepare_input_pin

输入引脚初始化函数。

- 调用 `qosa_get_pin_default_cfg()` 获取目标引脚默认映射关系
- 调用 `qosa_pin_set_func()` 将引脚切换到 GPIO 功能
- 调用 `qosa_gpio_init()` 将对应 GPIO 初始化为输入模式
- 配置输入上拉，并在初始化成功后输出引脚、GPIO、有效电平和模式信息

#### ky032_read_state

GPIO 电平读取函数。

- 调用 `qosa_gpio_get_level()` 读取当前传感器输出电平
- 在读取失败时输出错误日志
- 返回当前 GPIO 电平值

#### ky032_is_obstacle

障碍物判定函数。

- 调用 `ky032_read_state()` 获取当前电平
- 调用 `ky032_level_to_state()` 将电平转换为障碍物状态
- 返回当前是否检测到障碍物

#### ky032_monitor_polling

轮询监控函数。

- 按固定周期读取当前传感器状态
- 调用 `ky032_report_state()` 输出检测结果
- 适用于对实时性要求不高、优先验证接线的场景

#### ky032_enable_interrupt_mode

GPIO 中断初始化函数。

- 填充 `qosa_int_cfg_t` 中断配置结构体
- 调用 `qosa_interrupt_register()` 注册 GPIO 中断回调
- 调用 `qosa_interrupt_enable()` 使能中断触发
- 根据有效电平自动选择上升沿或下降沿触发方式

#### ky032_gpio_irq_handler

中断回调函数。

- 在检测到目标触发电平时置位障碍物标志
- 将传感器事件从中断上下文传递给后台任务处理

#### ky032_monitor_interrupt

中断监控函数。

- 调用 `ky032_enable_interrupt_mode()` 完成 GPIO 中断配置
- 在后台循环中检查中断标志位
- 检测到障碍物时输出日志，并在空闲周期补充当前状态判断

#### ky032_report_state

状态输出函数。

- 根据当前障碍物状态输出 `ky032 no obstacle` 或 `ky032 detect obstacle`
- 作为示例保留最小行为，便于后续扩展到停车、转向等动作控制

## 配置说明

默认 KY-032 示例配置定义在 `Kconfig` 和 `ky032_demo.c` 中，可通过配置项或宏进行编译期覆盖：

- `QAPP_KY032_PIN_NUM`：默认传感器数字输出引脚为 `23`
- `QAPP_KY032_ACTIVE_LEVEL`：默认障碍物有效电平为 `0`
- `QAPP_KY032_SAMPLE_PERIOD_MS`：默认采样周期为 `100 ms`
- `QAPP_KY032_USE_INTERRUPT`：默认关闭，中断模式需手动使能
- `KY032_TASK_STACK_SIZE`：后台任务栈大小为 `2048`
- `KY032_TASK_PRIORITY`：后台任务优先级为 `10`

如果实际硬件连接的传感器输出引脚、有效电平或采样周期与默认值不同，需要根据原理图和模块实测结果调整上述配置。当前示例同时提供轮询和中断两种检测方式：

- 轮询方式适合快速验证接线和基本功能
- 中断方式适合需要更快响应障碍事件的场景

如果你的板级设计已经把 KY-032 输出接到了其他 GPIO，只需修改 `QAPP_KY032_PIN_NUM` 即可；如果使用的模块输出逻辑和默认相反，可将 `QAPP_KY032_ACTIVE_LEVEL` 调整为 `1`。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎提交 Issue 和 Pull Request。
