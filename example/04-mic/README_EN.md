# MIC Demo

## Overview

MIC Demo is a microphone threshold detection example based on UniRTOS. This example detects sound intensity by reading voltage values from ADC1 to simulate microphone sound detection. When the sampled value exceeds the set threshold, it lights up an LED on a specified GPIO for a period of time. Through this example, developers can quickly learn the basic usage of ADC sampling, GPIO output control, PinMux configuration, and application task registration in UniRTOS.

## **Module Introduction**

A microphone is a short name for a **sound-to-electric transducer**, also called a sound detection sensor module. It can detect the sound intensity in the surrounding environment and convert it to an electrical signal output. It includes an internal microphone that can capture sound signals. By adjusting the sensitivity potentiometer on the module, you can adjust the module's sensitivity to sound. It supports analog output mode, satisfying most application and design needs.

## Connection Example

Following the table and images, connect the peripherals to the development board one-to-one correspondingly.

| Peripheral   | Development Board |
| ------------ | ----------------- |
| MIC (+)      | 3.3V              |
| MIC (-)      | GND               |
| MIC (S)      | A1(ADC1)          |
| LED (-)      | GND               |
| LED (S)      | PIN31             |

## Quick Start

### 1. Development Environment Setup

Refer to the [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Retrieval

```
# Retrieve the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/04-mic
```

### 3. Project Structure

```text
04-mic/
├── CMakeLists.txt      # MIC Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── mic_demo.c          # MIC threshold detection example source code
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

After successful initialization, you can see similar output in the logs:

```text
[I/DEMO] MIC demo init done, adc=ADC1 led_pin=31
[I/DEMO] MIC demo started: adc=ADC1 threshold=200mV sample=500ms led_pin=31 led_on=2s
```

During execution, the task periodically samples ADC1 voltage values and outputs similar logs:

```text
[I/DEMO] adc1 value=138mV
[I/DEMO] adc1 value=215mV
[I/DEMO] adc1 value=176mV
```

When the voltage value exceeds the 200mV threshold, the program pulls the LED to the active level and maintains it for 2 seconds, then returns to the inactive level.

## Code Overview

### Example Workflow

```text
Program Startup
    ↓
Call mic_demo_init()
    ↓
Create background task named "mic_demo"
    ↓
Enter task main function mic_demo_task()
    ↓
Configure ADC1 sampling scaling parameters
    ↓
Configure LED corresponding GPIO and PinMux
    ↓
Loop reading ADC1 voltage values
    ↓
Determine if sampled value exceeds threshold
    ├─ Below threshold: Wait for next sampling cycle
    └─ Exceeds threshold: Light LED for 2 seconds then turn off
```

### Main API Interfaces

#### mic_demo_init

Task initialization function.

- Check if MIC Demo task is already created
- Create background task named mic_demo
- Set task stack size and priority
- Output initialization complete log

#### mic_demo_task

Task processing function.

- Configure ADC1 scaling level
- Initialize LED GPIO and PinMux
- Periodically read ADC1 voltage values
- Output sampling logs
- Trigger LED indication when voltage exceeds threshold

#### mic_configure_adc

ADC configuration function.

- Set ADC1 scaling level to QOSA_ADC_SCALE_LEVEL_2
- Output warning log on configuration failure

#### mic_prepare_led_gpio

LED GPIO initialization function.

- Obtain LED pin default configuration
- Set pin multiplexing to GPIO functionality
- Initialize GPIO output direction, pull-up/down, and default level
- Use global state to avoid repeated hardware initialization

#### mic_handle_sound

Sound threshold processing function.

- Determine if current sampled value exceeds threshold
- If threshold exceeded, light the LED
- Turn off LED after set duration

## Configuration Description

MIC Demo default parameters are directly defined in mic_demo.c:

- MIC_APP_ORDER: Application initialization order, default value 210
- MIC_TASK_STACK_SIZE: Task stack size, default value 2048
- MIC_TASK_PRIORITY: Task priority, default value QOSA_PRIORITY_NORMAL
- MIC_LED_PIN_NUM: LED pin number, default value QOSA_PIN_31
- MIC_THRESHOLD_MV: Sound trigger threshold, default value 200mV
- MIC_SAMPLE_INTERVAL_MS: Sampling period, default value 500ms
- MIC_LED_ON_SEC: LED on duration, default value 2 seconds
- MIC_LED_ACTIVE_LEVEL: LED active level, default value QOSA_GPIO_LEVEL_HIGH

Different platforms may have different ADC channel mappings, GPIO numbering, and PinMux multiplexing definitions. When porting, please adjust according to your actual hardware schematic and platform PINMUX table.

## Community Forum

[Click here to enter](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

We welcome Issue submissions and Pull Requests.
