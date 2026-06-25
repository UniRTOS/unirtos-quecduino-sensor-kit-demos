# 激光发射模块

## **一、** **模块介绍**

**激光发射模块（Laser Emitter Module）** 的核心原理是：**通过半导体激光二极管（LD），将电能高效转化为高亮度、高方向性、单色性的相干光（激光），再经光学系统准直 / 整形后发射出去**。它广泛用于激光测距、激光雷达、光纤通信、激光指示、红外夜视等场景。

## 二、 连接示例

根据表格和图片指导，将外设与开发板一一对应连接

| 外设        | 开发板 |
| ----------- | ------ |
| Module（+） | 3.3V   |
| Module（-） | GND    |
| Module（S） | PIN31  |

## 快速上手

### 1. 开发环境搭建
参考 [UNIRTOS 快速入门](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) 文档，了解如何搭建开发环境和完成基础开发流程。

### 2. 项目结构

```
ky008_demos/
├── CMakeLists.txt      # CMake 构建配置
├── Kconfig             # 示例配置开关
├── ky008_demo.c        # KY008 激光发射器示例源代码
└── README.md           # 本文件
```

### 3. 构建项目
当前目录中的示例源码和子目录 CMakeLists 已就绪；如需作为内置应用参与整仓构建，需要在菜单配置中使能 `QAPP_KY008_DEMO`。完成接入并使能后，可在 UniRTOS 根目录执行类似命令进行构建：

```
unirtos make EG800ZCN_LA EG800ZCNLAR01A01M04_BETA_OCPU_20260511
```

### 4. 日志展示
初始化成功后，可在日志中看到类似输出：

```
[I/LOG_TAG_DEMO] ky008 demo task created
[I/LOG_TAG_DEMO] ky008 gpio init ok, pin=31 gpio=31 active=1 interval=2000ms
[I/LOG_TAG_DEMO] ky008 demo started
```

运行期间，示例会在后台任务中持续执行激光开关循环，并按默认 2000 ms 间隔输出当前状态日志。典型日志如下：

```
[I/LOG_TAG_DEMO] laser on
[I/LOG_TAG_DEMO] laser off
[I/LOG_TAG_DEMO] laser on
```

在默认配置下，状态控制规则如下：

- `active=1`：GPIO 输出高电平时开启激光，输出低电平时关闭激光
- `interval=2000ms`：激光开启后保持 2 秒，再关闭 2 秒，持续循环闪烁

## 代码概览

### 示例工作流程

```
程序启动
    ↓
调用 ky008_demo_start()
    ↓
创建名为 "ky008" 的后台任务
    ↓
进入任务主函数 ky008_demo_task()
    ↓
调用 ky008_gpio_init()
    ↓
查询目标 PIN 默认配置并切换为 GPIO 输出功能
    ↓
根据 active_level 计算激光开启/关闭电平
    ↓
将 GPIO 初始化为无效电平，避免上电误亮
    ↓
进入周期循环：
  ├─ 调用 ky008_on() 开启激光
  ├─ 调用 qosa_task_sleep_ms() 延时一个 interval 周期
  ├─ 调用 ky008_off() 关闭激光
  └─ 再延时一个 interval 周期后继续循环
```

### 主要 API 接口

#### ky008_demo_start
模块启动入口函数。

- 检查 KY008 后台任务是否已经创建
- 创建后台任务并设置任务栈大小、优先级和任务名
- 在任务创建成功后输出启动日志

#### ky008_demo_task
后台任务处理函数。

- 调用 GPIO 初始化函数完成激光控制引脚配置
- 在初始化失败时删除当前任务并退出
- 初始化成功后进入长期循环，持续执行激光闪烁

#### ky008_gpio_init
激光控制 GPIO 初始化函数。

- 调用 `qosa_get_pin_default_cfg()` 获取激光控制引脚默认配置
- 调用 `qosa_pin_set_func()` 将引脚切换到 GPIO 功能
- 根据配置的 `active_level` 计算有效电平和无效电平
- 调用 `qosa_gpio_init()` 将对应 GPIO 初始化为输出模式
- 在初始化成功后输出当前 pin、gpio、有效电平和闪烁间隔日志

#### ky008_set_level
GPIO 电平设置函数。

- 调用 `qosa_gpio_set_level()` 输出指定电平
- 在设置失败时输出错误日志并返回错误码

#### ky008_on
激光开启函数。

- 将 GPIO 输出为有效电平
- 在设置成功后输出 `laser on` 日志

#### ky008_off
激光关闭函数。

- 将 GPIO 输出为无效电平
- 在设置成功后输出 `laser off` 日志

#### ky008_blink_once
单次闪烁函数。

- 调用 `ky008_on()` 开启激光
- 延时一个 interval 周期
- 调用 `ky008_off()` 关闭激光
- 再延时一个 interval 周期

## 配置说明
默认 KY008 示例配置定义在 [qos_applications/ky008_demos/Kconfig](qos_applications/ky008_demos/Kconfig) 和 [qos_applications/ky008_demos/ky008_demo.c](qos_applications/ky008_demos/ky008_demo.c) 中，可通过配置项和宏进行编译期覆盖：

- `QAPP_KY008_DEMO`：KY008 激光发射器示例总开关
- `QAPP_KY008_PIN_NUM`：默认控制引脚为 `31`
- `QAPP_KY008_ACTIVE_LEVEL`：默认有效电平为 `1`，即高电平开启激光
- `QAPP_KY008_INTERVAL_MS`：默认闪烁间隔为 `2000 ms`
- `KY008_TASK_STACK_SIZE`：后台任务栈大小为 `2048`
- `KY008_TASK_PRIORITY`：后台任务优先级为 `QOSA_PRIORITY_NORMAL`

如果实际硬件连接的激光控制引脚或触发电平与默认值不同，需要根据原理图和实物接线调整上述配置。当前示例采用后台任务持续闪烁方式，适合用于激光指示、对准辅助、安防警示等场景；如果你希望激光常亮或按外部事件触发开关，可在 `ky008_demo_task()` 中进一步改为事件驱动版实现。

## 论坛社区
[点此进入](https://forumschinese.quectel.com/c/66-category/66)

## 贡献指南
欢迎提交 Issue 和 Pull Request。