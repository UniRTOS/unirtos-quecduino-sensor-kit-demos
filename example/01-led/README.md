# LED模块

## **一、** **模块介绍**

LED原理及产业分类LED是发光二极体( Light EmitTIng Diode, LED)的简称，也被称作发光二极管，这种半导体组件发展以来一般是作为指示灯、显示板，但目前随着技术增加，已经能作为光源使用，它不但能够高效率地直接将电能转化为光能，而且拥有最长达数万小时～10 万小时的使用寿命，同时具备不若传统灯泡易碎，并能省电，同时拥有环保无汞、体积小、可应用在低温环境、光源具方向性、造成光害少与色域丰富等优点。

**LED组成：**

![](../../media/led1.png)

**发光原理：**

![](../../media/led2.png)

左为正极，右为负极。当正负极形成电压差时，LED点亮。

## 二、 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设     | 开发板 |
| -------- | ------ |
| LED（+） | 3.3V   |
| LED（-） | GND    |
| LED（S） | PIN19  |

## 快速上手

### 1. 开发环境搭建

参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境和完成基础开发流程。

### 2. 项目结构

```text
led_demos/
├── CMakeLists.txt      # CMake 构建配置
├── led_demo.c          # LED GPIO 闪烁示例源代码
└── README.md           # 本文件
```

### 3. 构建项目

当前目录中的示例源码和子目录 CMakeLists 已就绪，但当前仓库顶层入口尚未将 led_demos 接入统一菜单配置和条件构建。

如需作为内置应用参与整仓构建，建议补齐以下接入项：

- 在 qos_applications/Kconfig 中新增 led demo 对应配置项，并通过 orsource 引入目录级 Kconfig
- 在 qos_applications/CMakeLists.txt 中新增对应的 add_subdirectory_if_exist 条件编译入口
- 在菜单配置中使能新增的 led demo 选项

完成接入并使能后，可在 UniRTOS 根目录执行类似命令进行构建：

```bash
unirtos make EG800ZCN_LA EG800ZCNLAR01A01M04_BETA_OCPU_20260511
```

### 4. 日志展示

初始化成功后，可在日志中看到类似输出：

```text
[I/LOG_TAG_DEMO] [TEST Demo]LED GPIO initialized successfully, pin_num: 19, gpio_num: 19, level: 1
```

运行期间，示例会在后台任务中持续控制 LED 亮灭，并按默认 1 s 周期交替输出开关状态日志。典型日志如下：

```text
[I/LOG_TAG_DEMO] [TEST Demo]LED ON
[I/LOG_TAG_DEMO] [TEST Demo]LED OFF
[I/LOG_TAG_DEMO] [TEST Demo]LED ON
```

在默认配置下，状态控制规则如下：

- `LED ON`：输出 GPIO 低电平，点亮 LED
- `LED OFF`：输出 GPIO 高电平，熄灭 LED

## 代码概览

### 示例工作流程

```text
程序启动
    ↓
调用 unir_test_demo_init()
    ↓
创建名为 "test_demo" 的后台任务
    ↓
进入任务主函数 unir_test_demo_process()
    ↓
调用 unir_led_init()
    ↓
获取 LED 引脚默认配置并切换到 GPIO 功能
    ↓
将 LED 对应 GPIO 初始化为输出模式
    ↓
进入周期循环：
  ├─ 调用 unir_led_set() 输出低电平点亮 LED
  ├─ 输出 LED ON 日志并延时 1000 ms
  ├─ 调用 unir_led_set() 输出高电平熄灭 LED
  └─ 输出 LED OFF 日志并延时 1000 ms
```

### 主要 API 接口

#### unir_test_demo_init

模块启动入口函数。

- 检查 LED 演示任务是否已经创建
- 创建后台任务并设置任务栈大小、优先级和任务名
- 由后台任务驱动 LED 周期闪烁

#### unir_test_demo_process

后台任务处理函数。

- 调用 LED 初始化函数完成 GPIO 配置
- 进入长期循环，按固定周期控制 LED 亮灭
- 在每次状态切换后输出对应日志

#### unir_led_init

LED 初始化函数。

- 调用 `qosa_get_pin_default_cfg()` 获取 LED 引脚默认配置
- 调用 `qosa_pin_set_func()` 将引脚切换到 GPIO 功能
- 调用 `qosa_gpio_init()` 将对应 GPIO 初始化为输出模式
- 在初始化成功后输出当前 pin 和 gpio 信息

#### unir_led_set

LED 控制函数。

- 调用 `qosa_gpio_set_level()` 设置 GPIO 输出电平
- 当输出低电平时点亮 LED
- 当输出高电平时熄灭 LED

## 配置说明

默认 LED 示例配置定义在 `led_demo.c` 中，可通过宏进行编译期覆盖：

- `UniRTOS_TEST_DEMO_TASK_STACK_SIZE`：后台任务栈大小为 1024
- `UniRTOS_TEST_DEMO_TASK_PRIO`：后台任务优先级为 `QOSA_PRIORITY_NORMAL`
- `LED_PIN_NUM`：默认 LED 引脚为 `19`

如果实际硬件连接的 LED 引脚与默认值不同，需要根据原理图调整上述宏配置。当前示例使用 GPIO 输出高低电平控制 LED 闪烁，适合作为 GPIO 输出、任务创建和基础日志打印的入门参考。

## 论坛社区

[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南

欢迎提交 Issue 和 Pull Request。