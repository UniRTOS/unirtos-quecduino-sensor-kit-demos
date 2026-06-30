# Photoresistor Module

## **Module Introduction**

A photoresistor sensor is a sensor capable of converting light signals to electrical signals. Its resistance changes as light intensity varies. In many practical applications such as automatic lighting systems and ambient light detection, photoresistor sensors play an important role. The EG800Z Duino development board has abundant peripheral resources and can conveniently be combined with photoresistor sensors to detect and process light intensity.

Photoresistors are typically made from semiconductor materials and operate based on the internal photoelectric effect. When light shines on a photoresistor, electrons in the semiconductor material absorb photon energy, transitioning from the valence band to the conduction band, increasing the material's conductivity and reducing resistance. Conversely, when light intensity decreases, resistance increases.

The characteristic curve of photoresistors typically exhibits nonlinear relationships, meaning light intensity and resistance are not in simple linear proportion. In practical applications, calibration and processing according to specific requirements and characteristic curves are necessary.

**Photoresistor Composition:**

![](../../media/light1.png)

**Working Principle:**

![](../../media/light2.png)

**The stronger the light, the smaller the resistance and the lower the voltage; the weaker the light, the larger the resistance and the higher the voltage.**

## Connection Example

Following the table and images, connect the peripherals to the development board one-to-one correspondingly.

| Peripheral | Development Board |
| ---------- | ----------------- |
| LDR (+)    | 3.3V              |
| LDR (-)    | GND               |
| LDR (S)    | A1 (ADC1)         |
| LED (S)    | PIN 31(10)        |

## Quick Start

### 1. Development Environment Setup

Refer to the [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Retrieval

```
# Retrieve the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/10-photoresistor(KY-018)
```

### 3. Project Structure

```text
10-photoresistor(KY-018)/
├── CMakeLists.txt      # KY-018 Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── light.c             # KY-018 light-sensitive sensor example source code
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
[I/LOG_TAG_DEMO] KY-018 light sensor demo started
[I/LOG_TAG_DEMO] light sensor led init ok, pin=31 gpio=31
[I/LOG_TAG_DEMO] light sensor adc init ok, channel=1 threshold=500mV poll=500ms
[I/LOG_TAG_DEMO] KY-018 light sensor task started
```

During execution, the example continuously reads ADC voltage values in background tasks and outputs current voltage, light status, and LED status on the default 500 ms cycle. Typical logs are as follows:

```text
[I/LOG_TAG_DEMO] light sensor: voltage=320mV | status=strong_light | led=on
[I/LOG_TAG_DEMO] light sensor: voltage=485mV | status=strong_light | led=on
[I/LOG_TAG_DEMO] light sensor: voltage=760mV | status=weak_light | led=off
```

With the default configuration, the status determination rules are as follows:

- `strong_light`: ADC voltage value < 500 mV, indicating strong current light, LED is lit
- `weak_light`: ADC voltage value >= 500 mV, indicating weak current light, LED is off

## Code Overview

### Example Workflow

```text
Program Startup
    ↓
Call light_sensor_demo_init()
    ↓
Create background task named "ky018_light"
    ↓
Enter task main function light_sensor_demo_task()
    ↓
Call light_sensor_led_init()
    ↓
Configure LED corresponding pin as GPIO output mode
    ↓
Call light_sensor_adc_init()
    ↓
Set ADC range level
    ↓
Enter periodic loop:
  ├─ Call qosa_adc_get_volt() to read ADC voltage
  ├─ Compare with threshold to determine light intensity
  ├─ Call light_sensor_set_led() to control LED
  └─ Output current voltage, status, and LED log
```

### Main API Interfaces

#### light_sensor_demo_init

Module startup entry function.

- Check if photoresistor monitoring task is already created
- Create background task with stack size, priority, and task name
- Output startup log after successful task creation

#### light_sensor_demo_task

Background task processing function.

- Call LED initialization function to complete indicator LED GPIO configuration
- Call ADC initialization function to complete range configuration
- Enter long-term loop, periodically read ADC voltage on fixed cycles
- Determine current light intensity based on threshold and update LED
- Output current voltage, light status, and LED status logs

#### light_sensor_led_init

LED initialization function.

- Call `qosa_get_pin_default_cfg()` to obtain LED pin default configuration
- Call `qosa_pin_set_func()` to switch pin to GPIO functionality
- Call `qosa_gpio_init()` to initialize corresponding GPIO as output mode
- Record GPIO number and output log after successful initialization

#### light_sensor_adc_init

ADC initialization function.

- Set target ADC channel range level through `qosa_adc_ioctl()`
- Output current ADC channel, threshold, and polling cycle configuration after successful initialization
- Output error log on configuration failure and return failure status

#### light_sensor_set_led

LED control function.

- Set LED high/low level based on light intensity determination result
- Light LED when status is `strong_light`
- Turn off LED when status is `weak_light`

#### light_sensor_status_name

Status determination function.

- Compare current ADC voltage value with threshold
- Return `strong_light` or `weak_light` status string

## Configuration Description

Default photoresistor example configuration is defined in `light.c` and can be overridden at compile time through macros:

- `LIGHT_SENSOR_ADC_CHANNEL`: Default ADC channel is `QOSA_ADC1_CHANNEL`
- `LIGHT_SENSOR_LED_PIN_NUM`: Default LED pin is `QOSA_PIN_31`
- `LIGHT_SENSOR_THRESHOLD_MV`: Default light determination threshold is 500 mV
- `LIGHT_SENSOR_ADC_SCALE`: Default ADC range level is `QOSA_ADC_SCALE_LEVEL_2`
- `LIGHT_SENSOR_POLL_MS`: Light monitoring log output cycle is 500 ms
- `LIGHT_SENSOR_TASK_STACK_SIZE`: Background task stack size is 2048

If actual hardware ADC input channel, LED pin, or light threshold differs from defaults, adjust the above macros according to schematic and measured data.

The current example adopts the common KY-018 module characteristic of "lower voltage indicates strong light, higher voltage indicates weak light" and maintains demonstration logic as "strong light lights LED, weak light turns off LED". If your application scenario is an automatic street light, you can modify the LED control logic to "weak light lights LED, strong light turns off LED".

## Community Forum

[Click here to enter](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

We welcome Issue submissions and Pull Requests.
