# Ultrasonic Module

## **Module Introduction**

The HC-SR04 operating process is initiated by a "trigger signal" and provides distance feedback through an "echo signal". The specific steps are as follows:

Trigger distance measurement: STM32 outputs a high level signal of at least 10μs on the Trig pin (requires high precision delay, which has been implemented in timer notes and can be reviewed);

Module automatically sends/receives ultrasonic waves: After receiving the trigger signal on Trig, the module automatically sends 8 square waves at 40kHz and begins detecting reflected ultrasonic waves;

Echo signal feedback: If ultrasonic waves are reflected back, the module outputs a high level on the Echo pin — the duration of the high level = total time from "emission to return" of the ultrasonic wave;

Distance calculation: Based on "time - distance" formula derivation, final distance = (Echo high level duration × sound speed) / 2

(Note: Sound speed is 340m/s. Division by 2 is because ultrasound travels "emission→reflection→return", covering twice the distance).

**1. Core Parameters**

- Operating Voltage: **3.3V–5V**
- Measurement Range: **2cm–450cm**
- Resolution: 1mm
- Measurement Angle: Approximately 15°
- Output Method: **GPIO / I2C / UART**
- Characteristics: Non-contact, high accuracy, fast response, unaffected by light color

**2. Schematic Diagram**

![](../../media/hc1.png)

**3. Timing Diagram**

![](../../media/hc2.png)

## **Connection Example**

Connect peripherals to the development board according to the table:

| **Peripheral** | **Module** |
| ------------------ | -------- |
| Ultrasonic (+) | VCC(5V)  |
| Ultrasonic (Trig) | Pin30    |
| Ultrasonic (Echo) | Pin31    |
| Ultrasonic (-) | GND      |

## Quick Start

### 1. Development Environment Setup

Refer to [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Repository Cloning

```
# Clone the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/14-Ultrasonic_module(HC-SR04)
```

### 3. Project Structure

```text
14-Ultrasonic_module(HC-SR04)/
├── CMakeLists.txt      # HC-SR04 Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── hcsr04_demo.c       # HC-SR04 ultrasonic distance measurement example source code
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
[V/LOG_TAG_DEMO] [HC-SR04] enter demo init
[V/LOG_TAG_DEMO] [HC-SR04] init done, trig pin=30 gpio=30, echo pin=31 gpio=31
[V/LOG_TAG_DEMO] [HC-SR04] demo task started
```

During operation, the example continuously triggers the HC-SR04 for single distance measurement in the background task, combined with sliding window average filtering, outputting current distance with a default 200 ms period. Typical logs are as follows:

```text
[V/LOG_TAG_DEMO] [HC-SR04] distance: 23.84 cm
[V/LOG_TAG_DEMO] [HC-SR04] distance: 23.91 cm
[V/LOG_TAG_DEMO] [HC-SR04] distance: 24.03 cm
```

When echo wait timeout, pulse width anomaly, or measurement result exceeds default range, failure log is output, for example:

```text
[V/LOG_TAG_DEMO] [HC-SR04] wait echo start timeout
[V/LOG_TAG_DEMO] [HC-SR04] measurement failed
```

Under default configuration, the distance measurement rules are as follows:

- Trigger pin pulled low for 2 us, then pulled high for 10 us to trigger module wave emission
- Echo high level pulse width converted to distance by `distance_cm = duration_us / 58.0`
- Only retain valid measurement results between 2 cm and 800 cm
- Perform sliding window average filtering on 5 consecutive valid measurements

## Code Overview

### Example Workflow

```text
Program starts
    ↓
Call hcsr04_demo_init()
    ↓
Create background task named "hcsr04_demo"
    ↓
Enter task main function hcsr04_demo_process()
    ↓
Call hcsr04_gpio_init()
    ↓
Configure Trig pin as GPIO output, Echo pin as GPIO input
    ↓
Enter periodic loop:
  ├─ Call hcsr04_read_filtered_distance()
  ├─ Internally call hcsr04_read_distance() for single measurement
  ├─ Call hcsr04_trigger() to send Trig trigger pulse
  ├─ Poll Echo high/low level and count high level duration
  ├─ Convert to centimeter value by formula and validate range
  └─ Apply sliding average on recent valid result and output distance log
```

### Main API Interfaces

#### hcsr04_demo_init

Module startup entry function.

- Check if ultrasonic distance measurement task has already been created
- Create background task and set stack size, priority, and task name
- Output startup log during initialization phase

#### hcsr04_demo_process

Background task handler function.

- Call GPIO initialization function to complete Trig and Echo pin configuration
- Enter long-term loop and execute distance measurement at fixed intervals
- Read filtered distance value and output log
- Output exception prompt log when measurement fails

#### hcsr04_gpio_init

GPIO initialization function.

- Call `qosa_get_pin_default_cfg()` to get Trig and Echo default pin configuration
- Call `qosa_pin_set_func()` to switch target pins to GPIO function
- Call `qosa_gpio_init()` to initialize Trig as output type and Echo as input type
- Output pin number and GPIO number log after successful initialization

#### hcsr04_trigger

Trigger pulse send function.

- Pull Trig low first to ensure stable bus before triggering
- Delay 2 us then pull high Trig
- Maintain 10 us high level then pull low to complete one trigger

#### hcsr04_read_distance

Single measurement function.

- Call `hcsr04_trigger()` to send trigger pulse
- Poll Echo pin waiting for high level start and add start timeout protection
- Continuously count Echo high level duration and add end timeout protection
- Convert distance by `duration_us / 58.0` formula, unit in cm

#### hcsr04_read_filtered_distance

Sliding window filter function.

- Call `hcsr04_read_distance()` to get single raw distance
- Filter out invalid results outside 2 cm to 800 cm range
- Write recent valid distance to history window of length 5
- Calculate window average and output filtered distance result

## Configuration

Default HC-SR04 example configuration is defined in `hcsr04_demo.c` and can be overridden at compile time via macros:

- `HC_SR04_TRIG_PIN_NUM`: Default Trig pin is 30
- `HC_SR04_ECHO_PIN_NUM`: Default Echo pin is 31
- `HCSR04_TRIGGER_PRE_DELAY_US`: Trigger pre-low level hold time is 2 us
- `HCSR04_TRIGGER_PULSE_WIDTH_US`: Trig high level pulse width is 10 us
- `HCSR04_ECHO_START_TIMEOUT_US`: Wait Echo pull-high timeout count is 30000
- `HCSR04_ECHO_END_TIMEOUT_US`: Wait Echo pull-low timeout count is 500000
- `HCSR04_FILTER_SIZE`: Sliding window filter length is 5
- `HCSR04_VALID_MIN_CM`: Default minimum valid distance is 2 cm
- `HCSR04_VALID_MAX_CM`: Default maximum valid distance is 800 cm
- `HCSR04_DISTANCE_DIVISOR`: Distance conversion coefficient is 58.0
- `HCSR04_MEASUREMENT_INTERVAL_MS`: Distance measurement log output period is 200 ms
- `HCSR04_TASK_STACK_SIZE`: Background task stack size is 2048

If the actual hardware Trig/Echo pins differ from default values, or the sensor installation environment causes echo time offset or range limit change, adjust the above macros based on schematic diagram and measured results. The current example uses busy-wait method to count Echo high level width, suitable for quick verification of HC-SR04 basic distance measurement capability. For higher precision or CPU usage requirements, it can be further improved to implement based on hardware timer or interrupt capture.

## Community Forum

[Click here](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

You are welcome to contribute. We recommend submitting changes as follows:

- Run basic validation before submitting: env-setup, build, and clean.
- Use a clear commit message that describes the purpose of the change, its impact scope, and validation results.
- When adding new features or changing behavior, update the README and related documentation at the same time.
- Submit bug fixes and feature improvements through Issues or Pull Requests.
