# KY-008 Laser Emission Demo

## Overview

The KY-008 Laser Emission Demo is a GPIO output control example project built on UNIRTOS. This project demonstrates how to initialize laser module control pins and periodically turn the laser on and off through background tasks. Through this example, developers can quickly learn how to use UNIRTOS GPIO output, task creation, periodic delay, and basic log printing.

## Module Introduction

The KY-008 laser emission module emits laser when the signal pin is active, commonly used for optical triggering, indication, alignment, and experimental demonstration. The laser module features strong directionality and concentrated brightness. When using it, avoid direct viewing of the beam or illuminating human eyes.

This example defaults to high level to turn on the laser and low level to turn it off, flashing at fixed intervals.

## Connection Example

| Peripheral | Development Board |
| ---- | ------ |
| KY-008 (VCC) | 3.3V |
| KY-008 (GND) | GND |
| KY-008 (S) | PIN31 |

The current example uses PIN31 to control the laser emission module by default, with high level on and low level off.

## Quick Start

### 1. Development Environment Setup

Refer to [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Repository Cloning

```
# Clone the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/17-Laser_emission_module(KY-008)
```

### 3. Project Structure

```text
17-Laser_emission_module(KY-008)/
├── CMakeLists.txt      # KY-008 Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── ky008_demo.c        # Laser emission control example source code
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

```text
[I/LOG_TAG_DEMO] KY-008 init ok, pin=31, gpio=31
```

The background task will switch laser state every 2 seconds by default:

```text
[I/LOG_TAG_DEMO] KY-008 laser on
[I/LOG_TAG_DEMO] KY-008 laser off
```

Default status control rules are as follows:

- GPIO outputs high level: Laser on
- GPIO outputs low level: Laser off

## Code Overview

### Example Workflow

```text
Program starts
    ↓
Execute ky008_demo_init()
    ↓
Create background task named "ky008"
    ↓
Call ky008_laser_init() to initialize laser control GPIO
        ↓
Enter periodic loop:
    ├─ Call ky008_laser_on() to turn on laser
    ├─ Delay 2000 ms
    ├─ Call ky008_laser_off() to turn off laser
    └─ Delay 2000 ms
```

### Main API Interfaces

#### ky008_demo_init

- Create KY-008 laser control task
- Set task stack, priority, task name, and entry function

#### ky008_laser_init

- Get PIN31 default configuration and switch to GPIO function
- Initialize as GPIO output mode
- Default laser is off

#### ky008_laser_on

- Call `qosa_gpio_set_level()` to output high level
- Output error log if setting fails

#### ky008_laser_off

- Call `qosa_gpio_set_level()` to output low level
- Output error log if setting fails

#### ky008_laser_blink

- Periodically call on and off interfaces
- Output laser state log

#### UNIRTOS_APP_EXPORT

- Register `ky008_demo_init()` with name `ky008_demo`
- Startup priority is `200`

## Configuration

Default configuration in this example is defined in `ky008_demo.c`:

- Laser Control Pin: `QOSA_PIN_31`
- Valid Level: High level is valid
- Flash Interval: `2000 ms`
- Task Stack Size: `2048`
- Task Priority: `QOSA_PRIORITY_NORMAL`

The laser module poses potential safety risks. During debugging, avoid direct viewing or aiming at the human body or mirror-reflective objects.

## Community Forum

[Click here](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guide

Issues and Pull Requests are welcome.
