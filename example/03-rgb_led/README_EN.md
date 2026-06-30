# RGB Demo

## Overview

RGB Demo is an RGB LED control example project based on UNIRTOS. This project demonstrates how to obtain GPIO default configurations, configure PINMUX as GPIO functionality, initialize GPIO output direction, and periodically switch the lighting status of red, green, and blue LED channels through background tasks on the UNIRTOS platform. Through this example, developers can quickly understand the basic usage of UNIRTOS GPIO and task APIs.

## **Module Introduction**

The tri-color RGB LED is a **full-color light-emitting diode module** composed of red, green, and blue chips packaged together. By adjusting brightness through PWM, arbitrary colors can be mixed. It is widely used in ambient lighting, status indication, interactive prompts, and maker DIY scenarios. It can achieve rainbow gradient, breathing, and flashing effects, with advantages including small size, high brightness, 3.3V/5V compatibility, simple driving, and long lifespan.

**Light Emission Principle:**

LED pins share a common ground. When there is a voltage difference between positive and negative terminals, the LED lights up, so high level indicates LED on.

## Connection Example

Following the table and images, connect the peripherals to the development board one-to-one correspondingly.

| Peripheral | Development Board |
| ---------- | ----------------- |
| LED (-)    | GND               |
| LED (R)    | PIN19             |
| LED (G)    | PIN20             |
| LED (B)    | PIN21             |

## Quick Start

### 1. Development Environment Setup

Refer to the [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Retrieval

```
# Retrieve the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/03-rgb_led
```

### 3. Project Structure

```text
03-rgb_led/
├── CMakeLists.txt      # RGB Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── unir_rgb_demo.c     # RGB LED example source code
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

During the initialization phase, you can see the following output in the logs:

```text
[V/DEMO] enter rgb led demo !!!
```

After successful execution, the example creates an RGB control task and, by default, switches the lighting color approximately every 1 second, with the effect as follows:

```text
Red LED on
Green LED on
Blue LED on
Red LED on
Green LED on
Blue LED on
...
```

If the development board is connected to a common anode RGB LED, low level means on and high level means off in the code; if it is common cathode, please adjust the high/low level logic according to your actual hardware.

## Code Overview

### Example Workflow

```text
Program Startup
    ↓
Call unir_rgb_init()
    ↓
Check if RGB control task is already created
    ↓
Create task named "unir_rgb_demo"
    ↓
Enter task main function unir_rgb_demo_process()
    ↓
Obtain default GPIO configuration for red, green, and blue pins respectively
    ↓
Switch three pins to GPIO functionality and initialize as high-level output
    ↓
Enter infinite loop, switch colors on 1-second intervals:
  ├─ Red LED on
  ├─ Green LED on
  └─ Blue LED on
```

### Main API Interfaces

#### unir_rgb_init

Task initialization function.

- Output RGB Demo startup log
- Check if task is already created
- Create RGB control task with stack size and priority
- Set task name and entry function

#### unir_rgb_demo_process

Task processing function.

- Obtain default configuration for red, green, and blue pins
- Configure PINMUX as GPIO functionality
- Initialize three GPIO channels as output mode with default high-level output
- In a loop, sequentially switch red, green, and blue colors
- Delay approximately 1 second after each color change

#### qosa_get_pin_default_cfg

GPIO default configuration retrieval interface.

- Parse platform default GPIO number and multiplexing function according to pin number
- Provide configuration parameters for subsequent PINMUX switching and GPIO initialization

#### qosa_gpio_init

GPIO initialization interface.

- Configure target GPIO as output mode
- Set pull-up property
- Set initial state level

## Configuration Description

The default RGB configuration is defined in unir_rgb_demo.c:

- RGB_RED_PIN: Default red LED pin is 19
- RGB_GREEN_PIN: Default green LED pin is 20
- RGB_BLUE_PIN: Default blue LED pin is 21
- UNIR_RGB_DEMO_TASK_STACK_SIZE: Task stack size is 1024
- UNIR_RGB_DEMO_TASK_PRIO: Task priority is QOSA_PRIORITY_NORMAL

The current example is designed by default for common anode RGB LED, meaning low level lights up and high level turns off. If your hardware is common cathode RGB LED, please swap QOSA_GPIO_LEVEL_LOW and QOSA_GPIO_LEVEL_HIGH in the code.

Different platforms may have different PINMUX definitions and RGB LED wiring methods. Please adjust RGB_RED_PIN, RGB_GREEN_PIN, RGB_BLUE_PIN, and output level logic according to your platform's PINMUX table and actual hardware connections.

## Community Forum

[Click here to enter](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

We welcome Issue submissions and Pull Requests!
