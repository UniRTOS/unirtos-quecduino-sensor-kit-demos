# Human Body Touch Module

## **Module Introduction**

This module is a capacitive momentary touch switch module based on touch detection. The metal touch module responds through human body capacitance. Since it detects capacitance, non-metallic materials like wood, paper, plastic, and other insulating materials can be placed on the module surface to detect human touch, enabling creation of hidden keys in walls, desktops, and other locations.

**Module Composition:**

![](../../media/finger1.png)

**Lighting Principle:**

The module has positive, negative, and signal terminals. When a person touches the sensing pad, the capacitance value changes. The module's internal circuit recognizes this and outputs high/low level signals. The development board can read the state directly to determine if it has been touched.

## Connection Example

Connect peripherals to the development board according to the table and diagram:

| Peripheral | Development Board |
| --------- | ------ |
| Module (+) | 3.3V   |
| Module (-) | GND    |
| Module (S) | PIN31  |

## Quick Start

### 1. Development Environment Setup

Refer to [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Repository Cloning

```
# Clone the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/15-Human_body_touch_module(KY-036)
```

### 3. Project Structure

```
15-Human_body_touch_module(KY-036)/
├── CMakeLists.txt      # KY-036 Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── ky036_demo.c        # KY-036 human body touch sensor example source code
└── README.md           # This file
```

### 4. Build Project

Fetch SDK and dependencies:

```
unirtos-cli env-setup
```

Execute the firmware compilation command in the PowerShell window:

```
unirtos-cli build -m EG800ZCN_LA -v EG800ZCNLAR01A01_OCPU_20260626
```

After compilation completes, the PowerShell window will display the build result:

```
SUCCESS: Unirtos project built successfully!
```

### 5. Log Display

After successful initialization, you can see similar output in the logs:

```
[I/LOG_TAG_DEMO] KY036: Touch sensor demo initializing...
[I/LOG_TAG_DEMO] KY036: DO pin = PIN_31
[I/LOG_TAG_DEMO] KY036: gpio_num=31, gpio_func=0
[I/LOG_TAG_DEMO] KY036: Touch sensor demo initialized successfully!
[I/LOG_TAG_DEMO] KY036: trigger_level=1, pull=2
```

During operation, the example continuously reads GPIO input level in the background task and outputs the current touch state with a default 1000 ms period. Typical logs are as follows:

```
[I/LOG_TAG_DEMO] KY036: Poll task started, interval=1000ms
[I/LOG_TAG_DEMO] Touch not detected
[I/LOG_TAG_DEMO] Touch detected
[I/LOG_TAG_DEMO] Touch detected
```

Under default configuration, the state judgment rules are as follows:

- `Touch not detected`: GPIO current level does not equal trigger level, current touch input is not present
- `Touch detected`: GPIO current level equals trigger level, current human touch is detected

## Code Overview

### Example Workflow

```
Program starts
    ↓
Call ky036_demo_init()
    ↓
Get DO pin default configuration and switch to GPIO function
    ↓
Call qosa_gpio_init() to configure input mode and pull-down
    ↓
Create background task named "ky036_poll"
    ↓
Enter task main function ky036_poll_task()
    ↓
Enter periodic loop:
  ├─ Call ky036_read_state() to read GPIO level
  ├─ Call ky036_is_touched() to determine if touch state
  ├─ Output "Touch detected" or "Touch not detected" log
  └─ Call qosa_task_sleep_ms() to delay 1000 ms
```

### Main API Interfaces

#### ky036_demo_init

Module startup entry function.

- Get KY-036 DO pin default pinmux and GPIO mapping configuration
- Call `qosa_pin_set_func()` to switch pin to GPIO function
- Call `qosa_gpio_init()` to initialize corresponding GPIO to input mode
- Create background polling task and output initialization log

#### ky036_poll_task

Background task handler function.

- Periodically read sensor digital output level
- Call touch state judgment function to confirm current touch status
- Output current status log for each poll
- Control polling period through `qosa_task_sleep_ms()`

#### ky036_read_state

GPIO level read function.

- Call `qosa_gpio_get_level()` to get current GPIO input level
- Output error log and return failure status if read fails

#### ky036_is_touched

Touch state judgment function.

- Compare current GPIO level with trigger level
- Return `QOSA_TRUE` or `QOSA_FALSE` indicating whether touch is detected

## Configuration

Default human body touch sensor example configuration is defined in `ky036_demo.c` and can be overridden at compile time via macros:

- `KY036_DO_PIN_NUM`: Default input pin is `QOSA_PIN_31`
- `KY036_TRIGGER_LEVEL`: Default trigger level is `QOSA_GPIO_LEVEL_HIGH`
- `KY036_GPIO_PULL`: Default pull-up/pull-down configuration is `QOSA_GPIO_PULL_DOWN`
- `KY036_POLL_INTERVAL_MS`: Default polling and log output period is 1000 ms
- `KY036_TASK_STACK_SIZE`: Background task stack size is 4 KB
- `KY036_TASK_PRIORITY`: Background task priority is 100

If the actual hardware touch sensor pin, trigger level, or pull-up/pull-down method differs from default values, adjust the above macros based on schematic diagram and module characteristics. The current example uses digital input polling method suitable for simple touch detection of KY-036 module DO output. If your board design is more suitable for event-driven processing, it can be further extended to GPIO interrupt implementation.

## Community Forum

[Click here](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guide

Issues and Pull Requests are welcome.
