# LED Demo

## Overview

LED Demo is a GPIO output control example project based on UNIRTOS. This project demonstrates how to obtain the default PIN configuration on the UNIRTOS platform, switch PINMUX to GPIO functionality, initialize the GPIO output direction, and periodically control LED on/off states through a background task. Through this example, developers can quickly understand the basic usage of UNIRTOS GPIO output, task creation, and basic log printing.

## Module Introduction

LED stands for Light Emitting Diode. It is also called a light-emitting diode. Since its development, this semiconductor component has generally been used as an indicator light or display panel. With technological improvements, it can now also be used as a light source. It can efficiently convert electrical energy directly into light energy, has a service life of tens of thousands to 100,000 hours, is less fragile than traditional bulbs, saves power, is environmentally friendly and mercury-free, has a small size, can be used in low-temperature environments, has directional light output, causes less light pollution, and supports rich color ranges.

**LED Composition:**

![](../../media/led1.png)

**Light-Emitting Principle:**

![](../../media/led2.png)

An LED has one-way conduction characteristics. The left side is the positive electrode, and the right side is the negative electrode. When a suitable voltage difference is formed between the positive and negative electrodes, the LED lights up. If connected in reverse or if the voltage is insufficient, the LED does not light up.

## Connection Example

Connect the LED module to the development board according to the table below:

| Peripheral | Development Board |
| ---- | ------ |
| LED (+) | 3.3V |
| LED (-) | GND |
| LED (S) | PIN19 |

The current example uses PIN19 to control the LED signal pin by default. In the code, low level turns the LED on, and high level turns the LED off.

## Quick Start

### 1. Development Environment Setup

Refer to the [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Retrieval

```
# Pull the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/01-led
```

### 3. Project Structure

```text
01-led/
├── CMakeLists.txt      # LED Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── led_demo.c          # LED GPIO blinking example source code
└── README.md           # This file
```

### 4. Build Project

Fetch SDK and dependencies:

```
unirtos-cli env-setup
```

Execute the firmware compilation command in the PowerShell window:

```bash
unirtos-cli build -m EG800ZCN_LA -v EG800ZCNLAR01A01_OCPU_20260626
```

After compilation completes, the PowerShell window will display the build result:

```text
SUCCESS: Unirtos project built successfully!
```

### 5. Log Display

After firmware flashing and startup, you can see similar output in the logs:

```text
[V/LOG_TAG_DEMO] [TEST Demo]enter TEST DEMO !!!
[I/LOG_TAG_DEMO] [TEST Demo]LED GPIO initialized successfully, pin_num: 19, gpio_num: 19, level: 1
```

After initialization succeeds, the background task switches the LED state every 1 second by default and continuously outputs on/off state logs:

```text
[I/LOG_TAG_DEMO] [TEST Demo]LED ON
[I/LOG_TAG_DEMO] [TEST Demo]LED OFF
[I/LOG_TAG_DEMO] [TEST Demo]LED ON
```

Default state control rules are as follows:

- `LED ON`: GPIO outputs low level, and the LED lights up
- `LED OFF`: GPIO outputs high level, and the LED turns off

## Code Overview

### Example Workflow

```text
Program starts
    ↓
Execute unir_test_demo_init() registered by UNIRTOS_APP_EXPORT
    ↓
Check whether the LED control task has already been created
    ↓
Create a background task named "test_demo"
    ↓
Enter task function unir_test_demo_process()
    ↓
Call unir_led_init() to initialize LED GPIO
    ↓
Get PIN19 default configuration and switch it to GPIO functionality
    ↓
Initialize the LED GPIO as output mode, with default high-level output
        ↓
Enter periodic loop:
    ├─ Call unir_led_set(QOSA_GPIO_LEVEL_LOW) to turn on the LED
    ├─ Output LED ON log and delay 1000 ms
    ├─ Call unir_led_set(QOSA_GPIO_LEVEL_HIGH) to turn off the LED
    └─ Output LED OFF log and delay 1000 ms
```

### Main API Interfaces

#### unir_test_demo_init

LED Demo startup entry function.

- Output demo startup log
- Check whether the LED control task has already been created
- Call `qosa_task_create()` to create a background task
- Set task stack size, task priority, task name, and task entry function

#### unir_test_demo_process

LED background task processing function.

- Call `unir_led_init()` to complete GPIO initialization
- Enter a long-running loop and control LED on/off states at a fixed period
- Output the corresponding log after each state switch

#### unir_led_init

LED GPIO initialization function.

- Call `qosa_get_pin_default_cfg()` to obtain the default configuration of PIN19
- Call `qosa_pin_set_func()` to switch the target pin to GPIO functionality
- Call `qosa_gpio_init()` to initialize GPIO as output mode
- Output high level by default so that the LED initial state is off
- Output current pin, gpio, and default level information after successful initialization

#### unir_led_set

LED level control function.

- Call `qosa_gpio_set_level()` to set GPIO output level
- Output low level to turn on the LED
- Output high level to turn off the LED
- Output error log and return failure status when setting fails

#### UNIRTOS_APP_EXPORT

Application startup registration macro.

- Register `unir_test_demo_init()` with the name `unir_led_demo`
- Startup priority is `700`
- Automatically execute LED Demo initialization logic after system startup

## Configuration Description

The default configuration in the current example is defined in `led_demo.c`:

- `LED_PIN_NUM`: Default LED control pin is `19`
- `UniRTOS_TEST_DEMO_TASK_STACK_SIZE`: LED control task stack size is `1024`
- `UniRTOS_TEST_DEMO_TASK_PRIO`: LED control task priority is `QOSA_PRIORITY_NORMAL`
- `UNIRTOS_APP_EXPORT(700, "unir_led_demo", unir_test_demo_init)`: Register LED Demo startup entry

This example is designed by default for wiring where low level turns on the LED. If the actual hardware uses high level to turn on the LED, swap the `QOSA_GPIO_LEVEL_LOW` and `QOSA_GPIO_LEVEL_HIGH` control logic in calls to `unir_led_set()`.

## Community Forum

[Click here to enter](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

You are welcome to contribute. We recommend submitting changes as follows:

- Run basic validation before submitting: env-setup, build, and clean.
- Use a clear commit message that describes the purpose of the change, its impact scope, and validation results.
- When adding new features or changing behavior, update the README and related documentation at the same time.
- Submit bug fixes and feature improvements through Issues or Pull Requests.
