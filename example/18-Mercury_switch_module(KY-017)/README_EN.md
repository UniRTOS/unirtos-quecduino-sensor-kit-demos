# Mercury Switch Module

## **Module Introduction**

The mercury switch module is a **gravity-sensing tilt/topple detection digital switch device**, also called tilt switch or inclination sensor, commonly used for tilt alarms, anti-topple protection, attitude detection, and trigger control scenarios. It relies on mercury flow to conduct/disconnect circuits and outputs stable high/low levels, featuring **high sensitivity, reliable conduction, no mechanical contact noise, 3.3V/5V compatibility, GPIO direct reading, compact size**.

**Lighting Principle:**

The module has positive, negative, and signal terminals. Utilizing the electrical conductivity and fluidity of mercury, when tilted to a certain angle, mercury flows to connect the electrode, circuit conducts. After reset, mercury leaves the electrode, circuit disconnects. The development board reads the level to determine tilt status.

## Connection Example

Connect peripherals to the development board according to the table and diagram:

| Peripheral | Development Board |
| ------------- | ------ |
| Mercury Switch (+) | 3.3V   |
| Mercury Switch (-) | GND    |
| Mercury Switch (S) | PIN31  |
| LED (S) | PIN30  |

## Quick Start

### 1. Development Environment Setup

Refer to [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Repository Cloning

```
# Clone the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/18-Mercury_switch_module(KY-017)
```

### 3. Project Structure

```
18-Mercury_switch_module(KY-017)/
├── CMakeLists.txt      # KY-017 Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── ky017_demo.c        # KY017 mercury switch example source code
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
[I/LOG_TAG_DEMO] ky017 demo init ok, sensor pin=31 output pin=30 trigger=1 pull=1
[I/LOG_TAG_DEMO] ky017 demo started: sensor pin=31 gpio=31, output pin=30 gpio=30
```

During operation, the example continuously polls mercury switch input status in the background task and outputs current tilt detection result with a default 1000 ms period. Typical logs are as follows:

```
[I/LOG_TAG_DEMO] Mercury switch tilt detected
[I/LOG_TAG_DEMO] Mercury switch tilt not detected
[I/LOG_TAG_DEMO] Mercury switch tilt detected
```

Under default configuration, the state judgment rules are as follows:

- `Tilt not detected`: Input pin level does not equal trigger level, linkage output keeps low level
- `Tilt detected`: Input pin level equals trigger level, linkage output pulled high

## Code Overview

### Example Workflow

```
Program starts
    ↓
Call ky017_demo_init()
    ↓
Initialize sensor input GPIO and linkage output GPIO
    ↓
Create background task named "ky017"
    ↓
Enter task main function ky017_demo_task()
    ↓
Read current sensor input level
    ↓
Call ky017_is_triggered() to determine if currently triggered
    ↓
Call ky017_set_output() to update linkage output status
    ↓
Output current tilt detection log
    ↓
Delay 1000 ms then continue next polling round
```

### Main API Interfaces

#### ky017_demo_init

Module startup entry function.

- Check if mercury switch monitoring task has already been created
- Initialize input GPIO and output GPIO
- Create background task and set stack size, priority, and task name
- Output current pin, trigger level, and pull-up/pull-down configuration log after successful initialization

#### ky017_demo_task

Background task handler function.

- Periodically read sensor input GPIO level
- Determine current tilt state based on trigger level
- Call linkage output function to update output pin status
- Output current detection result log

#### ky017_gpio_init

GPIO initialization function.

- Call `qosa_get_pin_default_cfg()` to get target pin default configuration
- Call `qosa_pin_set_func()` to switch pin to GPIO function
- Call `qosa_gpio_init()` to initialize pin as input or output mode
- Return error status if initialization fails

#### ky017_is_triggered

State judgment function.

- Compare current input level with trigger level
- Return current trigger state

#### ky017_set_output

Linkage output control function.

- Decide output high/low level based on trigger state
- Output valid level when triggered, invalid level when not triggered

#### ky017_get_inactive_level

Invalid level calculation function.

- Derive opposite invalid level from output valid level
- Used for output GPIO power-up default off and non-trigger state control

## Configuration

Default KY017 example configuration is defined in [qos_applications/ky017_demos/ky017_demo.c](qos_applications/ky017_demos/ky017_demo.c) and can be overridden at compile time via macros:

- `KY017_SENSOR_PIN`: Default sensor input pin is `QOSA_PIN_31`
- `KY017_OUTPUT_PIN`: Default linkage output pin is `QOSA_PIN_30`
- `KY017_TRIGGER_LEVEL`: Default trigger level is `QOSA_GPIO_LEVEL_HIGH`
- `KY017_SENSOR_PULL`: Default input pull-up/pull-down configuration is `QOSA_GPIO_PULL_UP`
- `KY017_OUTPUT_ACTIVE_LEVEL`: Default output valid level is `QOSA_GPIO_LEVEL_HIGH`
- `KY017_POLL_INTERVAL_MS`: Default polling period is `1000 ms`
- `KY017_TASK_STACK_SIZE`: Background task stack size is `2048`
- `KY017_TASK_PRIORITY`: Background task priority is `QOSA_PRIORITY_NORMAL`

If the actual hardware input pin, output pin, trigger level, or pull-up/pull-down method differs from default values, adjust the above macros based on schematic diagram and physical wiring. The current example implements GPIO polling method suitable for rollover alarm, fall detection, anti-theft device scenarios. If you need to reduce polling overhead, it can be further improved to GPIO interrupt implementation.

## Community Forum

[Click here](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

You are welcome to contribute. We recommend submitting changes as follows:

- Run basic validation before submitting: env-setup, build, and clean.
- Use a clear commit message that describes the purpose of the change, its impact scope, and validation results.
- When adding new features or changing behavior, update the README and related documentation at the same time.
- Submit bug fixes and feature improvements through Issues or Pull Requests.
