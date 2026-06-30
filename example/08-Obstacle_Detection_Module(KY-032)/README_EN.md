# KY-032 Obstacle Detection Demo

## Overview

KY-032 Obstacle Detection Demo is a GPIO input detection example project based on UNIRTOS. This project demonstrates how to obtain PIN default configurations, switch PINMUX to GPIO functionality, initialize GPIO input direction, and periodically read KY-032 infrared obstacle avoidance module output levels through background tasks on the UNIRTOS platform. Through this example, developers can quickly understand the basic usage of UNIRTOS GPIO input, task creation, polling detection, and basic log output.

## Module Introduction

KY-032 obstacle detection module is an infrared reflective digital detector, also called an infrared obstacle avoidance module, commonly used for near-range obstacle detection, line-following, obstacle avoidance, and limit triggering scenarios. The module detects obstacles by emitting infrared light through an emitter and detecting reflected light with a receiver.

**Module Composition:**

![](../../media/obstacle1.png)

**Working Principle:**

When no reflected infrared light is detected, the KY-032 OUT pin typically outputs high level; when an obstacle's reflected infrared light is detected, the OUT pin outputs low level. The code determines whether an obstacle is currently detected by reading the GPIO input level.

## Connection Example

Connect the KY-032 module to the development board according to the following table:

| Peripheral | Development Board |
| ---------- | ----------------- |
| KY-032 (+) | 3.3V              |
| KY-032 (-) | GND               |
| KY-032 (S/OUT) | PIN23             |

The current example defaults to using PIN23 to read the KY-032 signal pin. In the code, low level indicates obstacle detected and high level indicates no obstacle detected.

## Quick Start

### 1. Development Environment Setup

Refer to the [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Retrieval

```
# Retrieve the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/08-Obstacle_Detection_Module(KY-032)
```

### 3. Project Structure

```text
08-Obstacle_Detection_Module(KY-032)/
├── CMakeLists.txt      # KY-032 Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── ky032_demo.c        # KY-032 obstacle detection example source code
└── README.md           # This file
```

### 4. Build the Project

Retrieve SDK and dependency libraries

```
unirtos-cli env-setup
```

Execute the firmware compilation command in the PowerShell window:

```
unirtos-cli build -m EG800ZCN_LA -v EG800ZCNLAR01A01_OCPU_20260626
```

After compilation completes, the PowerShell window will display the firmware compilation result:

```
SUCCESS: Unirtos project built successfully!
```

### 5. Log Display

After firmware is flashed and started, you can see similar output in the logs in default polling mode:

```text
[I/LOG_TAG_DEMO] KY-032 init ok, pin=23, gpio=31
[I/LOG_TAG_DEMO] KY-032 polling mode started
```

After successful initialization, the background task defaults to reading sensor status every 200 ms and continuously outputs obstacle detection logs:

```text
[I/LOG_TAG_DEMO] KY-032 no obstacle
[I/LOG_TAG_DEMO] KY-032 obstacle detected
[I/LOG_TAG_DEMO] KY-032 no obstacle
```

Default status control rules are as follows:

- `KY-032 no obstacle`: GPIO input high level, no obstacle detected
- `KY-032 obstacle detected`: GPIO input low level, obstacle detected

If the `KY032_USE_INTERRUPT_MODE` compilation switch is set to `1`, the example will create an interrupt mode task and output similar log after successful initialization:

```text
[I/LOG_TAG_DEMO] KY-032 interrupt mode started
```

## Code Overview

### Example Workflow

```text
Program Startup
    ↓
Execute UNIRTOS_APP_EXPORT registered ky032_demo_init()
    ↓
Select polling or interrupt mode based on KY032_USE_INTERRUPT_MODE compilation switch
    ↓
Create background task named "ky032_poll" or "ky032_int"
    ↓
Enter task main function ky032_monitor_polling() or ky032_monitor_interrupt()
    ↓
Call ky032_sensor_init() to initialize KY-032 input GPIO
    ↓
Obtain PIN23 default configuration and switch to GPIO functionality
    ↓
Initialize KY-032 corresponding GPIO as pull-up input mode
    ↓
Default polling mode enters periodic loop:
  ├─ Call ky032_is_obstacle() to determine obstacle detection
  ├─ Internally call ky032_read_state() to read GPIO input level
  ├─ Output KY-032 obstacle detected log on low level
  ├─ Output KY-032 no obstacle log on high level
  └─ Delay 200 ms then continue to next detection
```

### Main API Interfaces

#### ky032_demo_init

KY-032 Demo startup entry function.

- Select task entry function based on `KY032_USE_INTERRUPT_MODE` compilation switch
- Create polling detection task `ky032_poll` by default
- Create detection task `ky032_int` in interrupt mode
- Call `qosa_task_create()` to create background task
- Set task stack size, task priority, task name, and task entry function
- Output error log on task creation failure

#### ky032_sensor_init

KY-032 GPIO initialization function.

- Call `qosa_get_pin_default_cfg()` to obtain PIN23 default configuration
- Call `qosa_pin_set_func()` to switch target pin to GPIO functionality
- Call `qosa_gpio_init()` to initialize GPIO as input mode
- Input pull-up/down configuration is `QOSA_GPIO_PULL_UP`
- Clear obstacle flag and output current pin and gpio information after successful initialization

#### ky032_read_state

KY-032 level reading function.

- Call `qosa_gpio_get_level()` to read current GPIO input level
- Return current level on successful read
- Output error log on read failure and default to high level as return value

#### ky032_is_obstacle

Obstacle status judgment function.

- Call `ky032_read_state()` to obtain current GPIO input level
- Return obstacle detected when input level is `QOSA_GPIO_LEVEL_LOW`
- Return no obstacle detected on high level

#### ky032_monitor_polling

Default polling mode task function.

- Call `ky032_sensor_init()` to complete GPIO initialization
- Output error log and delete current task on initialization failure
- Output polling mode startup log on successful initialization
- Enter long-term loop, read obstacle status and output log on 200 ms cycles

#### ky032_interrupt_init

KY-032 interrupt mode initialization function, only compiled when `KY032_USE_INTERRUPT_MODE` is `1`.

- Configure GPIO interrupt parameters including debounce, pull-up, callback function, and user context
- Call `qosa_interrupt_register()` to register interrupt callback
- Call `qosa_interrupt_enable()` to enable falling-edge trigger interrupt
- Output error log on registration or enabling failure and return failure status

#### ky032_irq_handler

KY-032 interrupt callback function, only used in interrupt mode.

- Read current sensor level after interrupt triggers
- Set `obstacle_flag` to `1` when level indicates obstacle detected

#### ky032_monitor_interrupt

Interrupt mode task function, only compiled when `KY032_USE_INTERRUPT_MODE` is `1`.

- Initialize KY-032 input GPIO
- Register and enable GPIO falling-edge interrupt
- Periodically check `obstacle_flag` flag
- Output log on obstacle detection and clear flag
- Output no obstacle log when no new event detected

#### UNIRTOS_APP_EXPORT

Application startup registration macro.

- Register `ky032_demo_init()` with name `ky032_demo`
- Startup priority is `200`
- Automatically execute KY-032 Demo initialization logic after system startup

## Configuration Description

Default configuration in the current example is defined in `ky032_demo.c`:

- `KY032_PIN_NUM`: Default KY-032 signal input pin is `23`
- `KY032_POLL_INTERVAL_MS`: Default detection and log output cycle is `200 ms`
- `KY032_TASK_STACK_SIZE`: KY-032 detection task stack size is `2048`
- `KY032_TASK_PRIORITY`: KY-032 detection task priority is `QOSA_PRIORITY_NORMAL`
- `KY032_USE_INTERRUPT_MODE`: Default value is `0`, using polling mode; set to `1` to enable interrupt mode
- `UNIRTOS_APP_EXPORT(200, "ky032_demo", ky032_demo_init)`: Register KY-032 Demo startup entry

This example is designed by default for KY-032 modules triggered by low level. If your actual module output logic is opposite to the default rule, adjust the level determination logic in `ky032_is_obstacle()`.

## Community Forum

[Click here to enter](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

We welcome Issue submissions and Pull Requests.
