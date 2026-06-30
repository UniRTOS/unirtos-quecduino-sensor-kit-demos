# Water Level Detection Module

## **Module Introduction**

The water level monitoring module is a **resistive liquid detection sensor** used to detect water level height, water presence/absence, and water leak alarms. Through conductive probes that detect liquid surface changes, it outputs analog signals with advantages including **fast response, small size, 3.3V compatibility, direct ADC connection, and long service life**.

**Working Principle:**

The Water Sensor water level sensor can monitor water levels. The module mainly utilizes the current amplification principle of transistors: when the liquid level causes the transistor base to conduct with the power supply positive terminal, a current of certain magnitude flows between the transistor base and emitter. This current is then amplified by a certain factor between the transistor collector and emitter. This amplified current produces a characteristic voltage across the emitter resistor, which is sampled by an AD converter.

## Connection Example

Following the table and images, connect the peripherals to the development board one-to-one correspondingly.

| Peripheral      | Development Board |
| --------------- | ----------------- |
| Module (+)      | 3.3V              |
| Module (-)      | GND               |
| Module (S)      | A1 (ADC1)         |

## Quick Start

### 1. Development Environment Setup

Refer to the [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Retrieval

```
# Retrieve the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/06-water_level_detect
```

### 3. Project Structure

```text
06-water_level_detect/
├── CMakeLists.txt      # Water Level Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── water_demo.c        # Water level sensor example source code
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
[I/LOG_TAG_DEMO] water level demo started
[I/LOG_TAG_DEMO] water level adc init ok, channel=1 ref=3300mV max=60mm warn=15mm alert=35mm samples=10
```

During execution, the example continuously performs multiple ADC samples and averages in background tasks, outputting current water level, voltage, and status on the default 1-second cycle. Typical logs are as follows:

```text
[I/LOG_TAG_DEMO] water level: 8.36 mm | voltage: 459.8 mV | status: normal
[I/LOG_TAG_DEMO] water level: 17.42 mm | voltage: 958.0 mV | status: warning
[I/LOG_TAG_DEMO] water level: 39.31 mm | voltage: 2162.0 mV | status: alert
```

With the default configuration, the status classification rules are as follows:

- `normal`: Water level below 15 mm
- `warning`: Water level >= 15 mm and < 35 mm
- `alert`: Water level >= 35 mm

## Code Overview

### Example Workflow

```text
Program Startup
    ↓
Call water_level_demo_init()
    ↓
Create background task named "water_level"
    ↓
Enter task main function water_level_demo_task()
    ↓
Call water_level_adc_init()
    ↓
Set ADC range level
    ↓
Enter periodic loop:
  ├─ Call water_level_read_voltage_avg_tenth_mv()
  ├─ Continuously sample multiple times and calculate average voltage
  ├─ Call water_level_convert_to_hundredths_mm() to convert to water level
  ├─ Call water_level_status_name() to determine status level
  └─ Output current water level, voltage, and status log
```

### Main API Interfaces

#### water_level_demo_init

Module startup entry function.

- Check if water level monitoring task is already created
- Create background task with stack size, priority, and task name
- Output startup log after successful task creation

#### water_level_demo_task

Background task processing function.

- Call ADC initialization function to complete range configuration
- Enter long-term loop, perform sampling on fixed cycles
- Read average voltage and convert to current water level
- Output current water level, voltage, and status logs

#### water_level_adc_init

ADC initialization function.

- Set target ADC channel range level through `qosa_adc_ioctl()`
- Output current reference voltage, range, and threshold configuration after successful initialization
- Output error log on configuration failure and return failure status

#### water_level_read_voltage_avg_tenth_mv

ADC average sampling function.

- Call `qosa_adc_get_volt()` to read target ADC channel voltage value
- Loop sampling according to set count and accumulate millivolt values
- Sleep briefly between samples according to set interval
- Result is average voltage in units of 0.1 mV

#### water_level_convert_to_hundredths_mm

Water level conversion function.

- Map average voltage to water level height according to linear formula
- Calculate result based on reference voltage and full-scale configuration
- Result maintains two decimal places in units of 0.01 mm

#### water_level_status_name

Status classification function.

- Compare converted water level with alarm thresholds
- Return `normal`, `warning`, or `alert` status string

## Configuration Description

Default water level sensor configuration is defined in `water_demo.c` and can be overridden at compile time through macros:

- `WATER_LEVEL_ADC_CHANNEL`: Default ADC channel is `QOSA_ADC1_CHANNEL`
- `WATER_LEVEL_ADC_REF_MV`: Default reference voltage is 3300 mV
- `WATER_LEVEL_MAX_MM`: Default water level full-scale is 60 mm
- `WATER_LEVEL_WARN_MM`: Default warning threshold is 15 mm
- `WATER_LEVEL_ALERT_MM`: Default alert threshold is 35 mm
- `WATER_LEVEL_SAMPLE_COUNT`: Samples per averaging cycle is 10
- `WATER_LEVEL_SAMPLE_INTERVAL_MS`: Interval between samples is 5 ms
- `WATER_LEVEL_ADC_SCALE`: Default ADC range level is `QOSA_ADC_SCALE_LEVEL_2`
- `WATER_LEVEL_DEMO_POLL_MS`: Water level monitoring log output cycle is 1000 ms
- `WATER_LEVEL_DEMO_TASK_STACK_SIZE`: Background task stack size is 2048

If the actual water level sensor output voltage range, ADC input channel, or calibrated range differs from defaults, adjust the above macros according to your hardware schematic and sensor specifications. The current example uses linear conversion, suitable for scenarios where sensor output voltage is approximately proportional to water level height. If the sensor has nonlinear regions, further calibration of the conversion formula based on measured data is recommended.

## Community Forum

[Click here to enter](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

We welcome Issue submissions and Pull Requests.
