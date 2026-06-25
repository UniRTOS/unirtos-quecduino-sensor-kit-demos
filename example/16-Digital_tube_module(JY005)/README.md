# 数码管模块

## **一、** **模块介绍**

单位数码管模块是**数字显示器件**，由 7 段发光二极管组成，用于显示 0-9 数字及简单符号，广泛用于计数、计时、状态显示、创客 DIY 场景；它亮度高、显示清晰、3.3V/5V 兼容、驱动简单、使用寿命长。

**LED组成：**

7 段 LED 发光段、公共端、小数点、限流电阻、PCB 板、接线端子

**发光原理：**

模块有正极、负极、段选信号端。通过控制不同段的亮灭，组合显示 0-9 数字，开发板通过 GPIO 输出电平控制对应段点亮。

## 二、 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设     | 开发板 |
| -------- | ------ |
| LED（+） | 3.3V   |
| LED（-） | GND    |
| LED（S） | 自选   |

## 快速上手

### 1. 开发环境搭建
参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境和完成基础开发流程。

### 2. 项目结构

```
JY005_demos/
├── CMakeLists.txt      # CMake 构建配置
├── jy005_demo.c        # JY005 数码管示例源代码
└── README.md           # 本文件
```

### 3. 构建项目
```
unirtos make EG800ZCN_LA EG800ZCNLAR01A01M04_BETA_OCPU_20260511
```

### 4. 日志展示
初始化成功后，可在日志中看到类似输出：

```
[I/LOG_TAG_DEMO] jy005 demo task created
[I/LOG_TAG_DEMO] jy005 demo start: a=32 b=31 c=30 d=33 e=80 f=58 g=29 dp=28
```

运行期间，示例会在后台任务中每隔 1 秒切换一次段码，按 0 到 9 的顺序循环显示。当前实现默认不输出每个数字切换时的单独日志，显示效果直接体现在数码管硬件上。

默认配置下，示例使用共阳极反逻辑段码表：

- 段码值 `0`：对应段点亮
- 段码值 `1`：对应段熄灭
- `dp` 段默认保持熄灭状态，不参与数字轮显

## 代码概览

### 示例工作流程

```
程序启动
    ↓
调用 jy005_demo_init()
    ↓
创建名为 "jy005" 的后台任务
    ↓
进入任务主函数 jy005_demo_task()
    ↓
调用 jy005_gpio_init()
    ↓
依次初始化 a、b、c、d、e、f、g、dp 8 个段引脚为 GPIO 输出模式
    ↓
调用 jy005_clear_display()
    ↓
输出当前段引脚映射日志
    ↓
进入周期循环：
  ├─ 调用 jy005_display_num() 输出当前数字对应段码
  ├─ 调用 qosa_task_sleep_ms() 延时 1 秒
  └─ 数字按 0 到 9 顺序循环显示
```

### 主要 API 接口

#### jy005_demo_init
模块启动入口函数。

- 检查 JY005 显示任务是否已经创建
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出启动日志

#### jy005_demo_task
后台任务处理函数。

- 调用 GPIO 初始化函数完成 8 个段引脚配置
- 调用清屏函数将所有段默认拉高熄灭
- 输出当前 a、b、c、d、e、f、g、dp 段的引脚映射
- 进入长期循环，按固定周期轮流显示 0 到 9

#### jy005_segment_gpio_init
单个段引脚初始化函数。

- 调用 `qosa_get_pin_default_cfg()` 获取段引脚默认配置
- 调用 `qosa_pin_set_func()` 将引脚切换到 GPIO 功能
- 调用 `qosa_gpio_init()` 将对应 GPIO 初始化为输出模式
- 默认将 GPIO 输出为高电平，使对应段熄灭

#### jy005_gpio_init
批量 GPIO 初始化函数。

- 遍历 a、b、c、d、e、f、g、dp 8 个段引脚配置
- 逐个调用 `jy005_segment_gpio_init()` 完成初始化
- 任一段初始化失败时立即返回失败状态

#### jy005_clear_display
数码管清屏函数。

- 遍历全部段引脚并输出高电平
- 将所有段恢复为熄灭状态

#### jy005_display_num
数字显示函数。

- 检查输入数字是否位于 0 到 9 范围内
- 按段码表读取当前数字对应的 8 段状态
- 将段码值转换为 GPIO 电平并输出到对应段引脚

## 配置说明
默认 JY005 示例配置定义在 [qos_applications/JY005_demos/jy005_demo.c](qos_applications/JY005_demos/jy005_demo.c) 中，可通过宏进行编译期覆盖：

- `JY005_SEG_A_PIN`：默认 a 段引脚为 `QOSA_PIN_32`
- `JY005_SEG_B_PIN`：默认 b 段引脚为 `QOSA_PIN_31`
- `JY005_SEG_C_PIN`：默认 c 段引脚为 `QOSA_PIN_30`
- `JY005_SEG_D_PIN`：默认 d 段引脚为 `QOSA_PIN_33`
- `JY005_SEG_E_PIN`：默认 e 段引脚为 `QOSA_PIN_80`
- `JY005_SEG_F_PIN`：默认 f 段引脚为 `QOSA_PIN_58`
- `JY005_SEG_G_PIN`：默认 g 段引脚为 `QOSA_PIN_29`
- `JY005_SEG_DP_PIN`：默认 dp 段引脚为 `QOSA_PIN_28`
- `JY005_DISPLAY_INTERVAL_MS`：默认每个数字显示时长为 1000 ms
- `JY005_TASK_STACK_SIZE`：后台任务栈大小为 2048
- `JY005_TASK_PRIORITY`：后台任务优先级为 `QOSA_PRIORITY_NORMAL`

如果实际硬件连接的段引脚与默认映射不同，需要根据原理图和实物接线调整上述宏配置。当前示例按共阳极数码管逻辑实现；如果你使用的是共阴极数码管，则需要同步调整 `jy005_display_num()` 中的电平极性或直接修改段码表。

## 论坛社区
[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南
欢迎提交 Issue 和 Pull Request。