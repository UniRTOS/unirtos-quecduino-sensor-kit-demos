# KY-026 Flame Detection Demo

## Overview

The KY-026 Flame Detection Demo is an ADC-based flame detection example project built on UNIRTOS. This project demonstrates how to read the analog output of a flame sensor, classify states (safe, risk, fire alarm) based on voltage thresholds, and provide LED alarm output. Through this example, developers can quickly learn how to use UNIRTOS ADC sampling, threshold classification, GPIO output, and task creation.

## Module Introduction

The KY-026 flame detection module senses flame light signals in specific wavelengths, commonly used for flame proximity detection and alarm demonstration experiments. The module's analog output voltage varies with the intensity of detected flame. The development board reads this through ADC and makes classification judgments.

This example uses ADC1 to read the flame sensor voltage and PIN31 to control the alarm LED. Higher voltage indicates higher risk level as determined by the code.

## Connection Example

| Peripheral | Development Board |
| ---- | ------ |
| KY-026 (VCC) | 3.3V |
| KY-026 (GND) | GND |
| KY-026 (AO) | ADC1 |
| LED (S) | PIN31 |

The current example uses ADC1 to read the flame sensor analog output by default and PIN31 to control the alarm LED.

## Quick Start

### 1. Development Environment Setup

Refer to [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Repository Cloning

```
# Clone the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/11-flame_detect(KY-026)
```

### 3. Project Structure

```text
11-flame_detect(KY-026)/
├── CMakeLists.txt      # KY-026 Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── flame_demo.c        # Flame detection ADC example source code
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

After firmware flashing and startup, you can see similar output in the logs:

```text
[I/LOG_TAG_DEMO] flame led init ok, pin=31, gpio=31
[I/LOG_TAG_DEMO] flame demo started
```

The background task will output status by default every 1 second:

```text
[I/LOG_TAG_DEMO] ADC: 80 mV, status: safe
[I/LOG_TAG_DEMO] ADC: 260 mV, status: flame risk
[I/LOG_TAG_DEMO] ADC: 650 mV, status: fire alarm
```

Default status control rules are as follows:

- ADC voltage below `100 mV`: Safe state
- ADC voltage from `100 mV` to below `500 mV`: Flame risk state
- ADC voltage at or above `500 mV`: Fire alarm state, LED flashing alarm

## Code Overview

### Example Workflow

```text
Program starts
    ↓
Execute flame_demo_init()
    ↓
Create background task named "flame"
    ↓
Initialize LED GPIO and ADC1
        ↓
Enter periodic loop:
    ├─ Call flame_read_value() to read ADC voltage
    ├─ Determine safe, risk, or fire alarm state based on threshold
    ├─ Flash LED in fire alarm state
    └─ Output current ADC value and status log
```

### Main API Interfaces

#### flame_demo_init

- Call `qosa_task_create()` to create flame detection task
- Set task stack, priority, task name, and entry function

#### flame_led_init

- Get PIN31 default configuration and switch to GPIO function
- Initialize alarm LED as GPIO output mode

#### flame_adc_init

- Configure ADC1 channel and range
- Output error log if initialization fails

#### flame_read_value

- Call ADC read interface to get current voltage
- Output error log if read fails

#### flame_monitor_task

- Periodically read flame sensor voltage
- Classify by threshold and control LED

#### UNIRTOS_APP_EXPORT

- Register `flame_demo_init()` with name `flame_demo`
- Startup priority is `200`

## Configuration

Default configuration in this example is defined in `flame_demo.c`:

- ADC Channel: `QOSA_ADC1_CHANNEL`
- LED Control Pin: `QOSA_PIN_31`
- Safe Threshold: `100 mV`
- Fire Alarm Threshold: `500 mV`
- Monitoring Period: `1000 ms`
- Alarm Flash Interval: `500 ms`
- Task Stack Size: `2048`
- Task Priority: `QOSA_PRIORITY_NORMAL`

Flame detection demonstration should be performed in a safe environment. Adjust thresholds based on actual sensor output.

## Community Forum

[Click here](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

You are welcome to contribute. We recommend submitting changes as follows:

- Run basic validation before submitting: env-setup, build, and clean.
- Use a clear commit message that describes the purpose of the change, its impact scope, and validation results.
- When adding new features or changing behavior, update the README and related documentation at the same time.
- Submit bug fixes and feature improvements through Issues or Pull Requests.
