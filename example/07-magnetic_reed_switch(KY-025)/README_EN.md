# KY-025 Magnetic Reed Switch (Dry Reed Switch) Sensor Module Introduction

KY-025 is a magnetic control sensor module based on the principle of **magnetic reed switches (Reed Switch, also called dry reed switches)**. It is essentially a micro-electric switch controlled by a magnetic field. When a magnet approaches, the internal metal reed blades attract and close the circuit; when the magnet moves away, the blades automatically spring back and open the circuit.

Due to its simple structure, high sensitivity, and non-contact trigger capability, KY-025 is commonly used in IoT projects as a non-contact proximity detection or position limit switch.

![](../../media/reed1.png)

## Core Features

- **Dual Signal Output**: The module provides both digital (DO) and analog (AO) output interfaces simultaneously, enabling both simple on/off judgment and sensing of relative magnetic field strength changes.
- **Adjustable Sensitivity**: On-board precision potentiometer (fine-tuning knob) allows rotation to adjust sensor detection distance and trigger sensitivity according to actual application scenarios.
- **Intuitive Work Indicator**: Equipped with power indicator and work status LED. When magnetic field is detected and triggers, the on-board LED lights up, facilitating debugging and observation.
- **Wide Voltage Compatibility**: Usually supports wide voltage supply from 3.3V to 5V, perfectly compatible with Arduino, STM32, QuecDuino, and other mainstream single-chip microcontroller development boards.

## **Pin Description and Wiring**

KY-025 module typically provides 4 standard pins with the following definitions:

| Pin Name    | Function Description | Wiring Suggestion             |
| :---------- | :------------------- | :---------------------------- |
| **+ (VCC)** | Power Positive       | Connect to board's 3.3V or 5V |
| **G (GND)** | Power Negative       | Connect to board's GND        |
| **A0**      | Analog Signal Output | Connect to board's ADC pin (e.g., A1) |

## Working Principle Detailed Explanation

**Analog Output (A0)**: The voltage value output on this pin changes linearly with magnetic field strength. Normally, without magnetic field, the output is a higher value. As the magnet gradually approaches, the output voltage gradually decreases. By reading this analog value, you can roughly determine the distance between the magnet and sensor.

## Common Application Scenarios

- **Door and Window Anti-theft Alarm**: Install the module on the door frame and magnet on the door leaf. Opening the door triggers the alarm.
- **Smart Counting and Speed Measurement**: Install a magnet on a fan blade or rotating object, triggering once per rotation to calculate RPM or cumulative count.
- **Position Limit Detection**: On robotic arms or mobile platforms, detect whether preset physical boundaries have been reached.
- **Contactless Switch**: Used as a hidden, durable trigger for jewel boxes and gift boxes to light up when opened.

## Quick Start

### 1. Development Environment Setup

Refer to the [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Retrieval

```
# Retrieve the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/07-magnetic_reed_switch(KY-025)
```

### 3. Project Structure

```text
07-magnetic_reed_switch(KY-025)/
├── CMakeLists.txt      # CMake build configuration
├── env_config.json     # UniRTOS project environment configuration
├── adc.c               # KY-025 magnetic reed switch ADC example source code
├── Kconfig             # Example configuration switch
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
[I/LOG_TAG_DEMO] magnetic reed demo started
[I/LOG_TAG_DEMO] led init ok, pin=19 gpio=19
[I/LOG_TAG_DEMO] magnetic reed adc init ok, channel=1 threshold=100mV poll=500ms
```

During execution, the example continuously reads ADC voltage values in background tasks and outputs current voltage and magnetic field detection status on the default 500 ms cycle. Typical logs are as follows:

```text
[I/LOG_TAG_DEMO] magnetic reed: voltage=42mV | status=idle
[I/LOG_TAG_DEMO] magnetic reed: voltage=118mV | status=detected
[I/LOG_TAG_DEMO] magnetic reed: voltage=135mV | status=detected
```

With the default configuration, the status determination rules are as follows:

- `idle`: ADC voltage value <= 100 mV, indicating no obvious magnetic field detected
- `detected`: ADC voltage value > 100 mV, indicating magnetic field detected, with LED lit simultaneously

## Code Overview

### Example Workflow

```text
Program Startup
    ↓
Call magnetic_reed_demo_init()
    ↓
Create background task named "mag_reed"
    ↓
Enter task main function magnetic_reed_demo_task()
    ↓
Call magnetic_reed_led_init()
    ↓
Configure LED corresponding pin as GPIO output mode
    ↓
Call magnetic_reed_adc_init()
    ↓
Set ADC range level
    ↓
Enter periodic loop:
  ├─ Call qosa_adc_get_volt() to read ADC voltage
  ├─ Compare with threshold to determine magnetic field detection
  ├─ Call magnetic_reed_set_led() to control LED
  └─ Output current voltage and status log
```

### Main API Interfaces

#### magnetic_reed_demo_init

Module startup entry function.

- Check if magnetic reed switch monitoring task is already created
- Create background task with stack size, priority, and task name
- Output startup log after successful task creation

#### magnetic_reed_demo_task

Background task processing function.

- Call LED initialization function to complete indicator LED GPIO configuration
- Call ADC initialization function to complete range configuration
- Enter long-term loop, periodically read ADC voltage on fixed cycles
- Determine current magnetic field status based on threshold and update LED
- Output current voltage and status logs

#### magnetic_reed_led_init

LED initialization function.

- Call `qosa_get_pin_default_cfg()` to obtain LED pin default configuration
- Call `qosa_pin_set_func()` to switch pin to GPIO functionality
- Call `qosa_gpio_init()` to initialize corresponding GPIO as output mode
- Record GPIO number and output log after successful initialization

#### magnetic_reed_adc_init

ADC initialization function.

- Set target ADC channel range level through `qosa_adc_ioctl()`
- Output current ADC channel, threshold, and polling cycle configuration after successful initialization
- Output error log on configuration failure and return failure status

#### magnetic_reed_set_led

LED control function.

- Set LED high/low level based on magnetic field detection result
- Light LED when status is `detected`
- Turn off LED when status is `idle`

#### magnetic_reed_status_name

Status determination function.

- Compare current ADC voltage value with threshold
- Return `idle` or `detected` status string

## Configuration Description

Default magnetic reed switch example configuration is defined in `adc.c` and can be overridden at compile time through macros:

- `MAGNETIC_REED_ADC_CHANNEL`: Default ADC channel is `QOSA_ADC1_CHANNEL`
- `MAGNETIC_REED_THRESHOLD_MV`: Default magnetic field detection threshold is 100 mV
- `MAGNETIC_REED_LED_PIN_NUM`: Default LED pin is `QOSA_PIN_19`
- `MAGNETIC_REED_ADC_SCALE`: Default ADC range level is `QOSA_ADC_SCALE_LEVEL_2`
- `MAGNETIC_REED_POLL_MS`: Magnetic field monitoring log output cycle is 500 ms
- `MAGNETIC_REED_TASK_STACK_SIZE`: Background task stack size is 2048

If actual hardware ADC input channel, LED pin, or sensor output threshold differs from defaults, adjust the above macros according to schematic and measured data. The current example uses ADC voltage threshold determination, suitable for simple detection of KY-025 module analog output changes. If your board design is better suited for high/low level determination using digital output, you can further modify it to GPIO input version.

## Community Forum

[Click here to enter](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

We welcome Issue submissions and Pull Requests.
