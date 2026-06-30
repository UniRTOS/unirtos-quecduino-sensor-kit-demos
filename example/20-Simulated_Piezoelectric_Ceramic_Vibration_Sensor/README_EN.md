# Simulated Piezoelectric Ceramic Vibration Sensor

## **Module Introduction**

This sensor is a vibration sensor based on piezoelectric ceramic film simulation, utilizing the reverse transformation process of piezoelectric ceramics converting electrical signals into vibrations. When the piezoelectric ceramic sheet vibrates, the sensor signal terminal generates electrical signals. The module is compatible with various microcontroller control boards such as Arduino series. The module includes 2 interface types for your selection. One features 2.54 mm spacing white connectors with anti-reverse protection. When used, a sensor expansion board can be stacked on the microcontroller. The module and self-supplied wires are connected and then connected to the sensor expansion board, simple and convenient. The other features 2.54 mm spacing pin headers. Using male-to-female Dupont wires, it can be directly connected to the microcontroller.

**Working Principle:**

**As vibration output (inverse piezoelectric effect)**: The module has power, ground, and signal terminals. When pulse/square wave electrical signals are input to the signal terminal, the piezoelectric ceramic sheet deforms due to inverse piezoelectric effect, driving the substrate to vibrate, achieving vibration feedback.

**As vibration detection (direct piezoelectric effect)**: When the module experiences mechanical vibration/impact, the piezoelectric ceramic sheet generates weak electrical signals output from the signal terminal. The development board can detect vibration intensity through ADC sampling.

## Connection Example

Connect peripherals to the development board according to the table:

| Peripheral | Development Board |
| --------- | ---------- |
| Module (+) | 3.3V       |
| Module (-) | GND        |
| Module (S) | A1 (ADC1) |

## Quick Start

### 1. Development Environment Setup

Refer to [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Repository Cloning

```
# Clone the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/20-Simulated_Piezoelectric_Ceramic_Vibration_Sensor
```

### 3. Project Structure

```text
20-Simulated_Piezoelectric_Ceramic_Vibration_Sensor/
├── CMakeLists.txt      # Vibration Sensor Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── touch_demo.c        # Vibration sensor ADC polling alarm example source code
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

```text
[I/LOG_TAG_DEMO] touch demo task created
[I/LOG_TAG_DEMO] touch demo adc init ok, channel=1 threshold=1500mV poll=200ms
[I/LOG_TAG_DEMO] touch demo started
```

During operation, the example continuously reads ADC voltage values in the background task and outputs current vibration value or alarm status with a default 200 ms period. Typical logs are as follows:

```text
[I/LOG_TAG_DEMO] vibration value=842mV
[I/LOG_TAG_DEMO] vibration value=1165mV
[I/LOG_TAG_DEMO] vibration alert, value=1680mV
```

Under default configuration, the state judgment rules are as follows:

- `vibration value=xxxmV`: ADC voltage value less than 1500 mV, current vibration intensity has not reached alarm threshold
- `vibration alert, value=xxxmV`: ADC voltage value greater than or equal to 1500 mV, significant vibration or impact has occurred

## Code Overview

### Example Workflow

```text
Program starts
    ↓
Call touch_demo_init()
    ↓
Create background task named "touch_demo"
    ↓
Enter task main function touch_demo_task()
    ↓
Call touch_demo_open_adc()
    ↓
Set ADC range level
    ↓
Enter periodic loop:
  ├─ Call touch_demo_read_value() to read ADC voltage
  ├─ Call touch_demo_check_alert() to compare with threshold
  ├─ Output alarm log when exceeding threshold
  └─ Output current vibration value log when not exceeding threshold
```

### Main API Interfaces

#### touch_demo_init

Module startup entry function.

- Check if vibration monitoring task has already been created
- Create background task and set stack size, priority, and task name
- Output startup log after task creation succeeds

#### touch_demo_task

Background task handler function.

- Call ADC initialization function to complete range configuration
- Enter long-term loop and read ADC voltage at fixed intervals
- Determine current vibration alarm condition based on threshold
- Output current vibration value or alarm status log

#### touch_demo_open_adc

ADC initialization function.

- Set target ADC channel range level through `qosa_adc_ioctl()`
- Output current ADC channel, threshold, and polling period configuration after successful initialization
- Output error log and return failure status if configuration fails

#### touch_demo_read_value

ADC read function.

- Call `qosa_adc_get_volt()` to read current ADC voltage value
- Output error log and return failure status if read fails
- Return current sampling voltage value if read succeeds

#### touch_demo_check_alert

State judgment function.

- Compare current ADC voltage value with alarm threshold
- Return alarm status when voltage value is greater than or equal to threshold
- Return non-alarm status when voltage value is below threshold

## Configuration

Default vibration sensor example configuration is defined in `touch_demo.c` and can be overridden at compile time via macros:

- `TOUCH_DEMO_TASK_STACK_SIZE`: Background task stack size is 2048
- `TOUCH_DEMO_POLL_INTERVAL_MS`: Vibration monitoring log output period is 200 ms
- `TOUCH_DEMO_ALERT_THRESHOLD_MV`: Default vibration alarm threshold is 1500 mV
- `TOUCH_DEMO_ADC_CHANNEL`: Default ADC channel is `QOSA_ADC1_CHANNEL`
- `TOUCH_DEMO_ADC_SCALE`: Default ADC range level is `QOSA_ADC_SCALE_LEVEL_2`

If the actual hardware ADC input channel, sensor output amplitude, or alarm threshold differs from default values, adjust the above macros based on schematic diagram and measured data. The current example uses ADC voltage threshold judgment method suitable for simple monitoring of vibration sensor analog output, applicable to cabinet anti-tampering detection, equipment fall detection, door window vibration alarm scenarios.

## Community Forum

[Click here](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guide

Issues and Pull Requests are welcome.
