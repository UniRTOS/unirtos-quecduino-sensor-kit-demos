# Inclination Switch Module

## **Module Introduction**

The inclination switch is an **attitude-sensing digital switch device**, also called a ball switch or tilt sensor, commonly used for tilt detection, anti-topple protection, attitude triggering, and smart alarm scenarios. It automatically switches electrical level signals when the module tilts to a certain angle, featuring small size, contactless design, low power consumption, 3.3V/5V compatibility, direct GPIO detection, fast response, and long lifespan.

**Working Principle:**

The module has positive, negative, and signal terminals. When tilted, internal ball/conductive fluid moves, causing internal contacts to conduct or disconnect, outputting high/low levels. The development board can read the state directly to determine if tilting has occurred.

## Connection Example

Connect peripherals to the development board according to the table and diagram:

| Peripheral | Development Board |
| ------------- | ------ |
| Inclination Switch (+) | 3.3V   |
| Inclination Switch (-) | GND    |
| Inclination Switch (S) | PIN31  |
| LED (S) | PIN32  |

## Quick Start

### 1. Development Environment Setup

Refer to [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Repository Cloning

```
# Clone the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/13-Inclination_switch_module(KY-020)
```

### 3. Project Structure

```text
13-Inclination_switch_module(KY-020)/
├── CMakeLists.txt      # KY-020 Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── ky020_demo.c        # KY-020 inclination switch example source code
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
[I/LOG_TAG_DEMO] ky020 demo init done
[I/LOG_TAG_DEMO] ky020 demo started: sensor pin=31 gpio=31, led pin=32 gpio=32
```

During operation, the example continuously reads GPIO input level in the background task and outputs the current tilt state with a default 1000 ms period. Typical logs are as follows:

```text
[I/LOG_TAG_DEMO] Horizontal state
[I/LOG_TAG_DEMO] Tilt detected
[I/LOG_TAG_DEMO] Tilt detected
[I/LOG_TAG_DEMO] Horizontal state
```

Under default configuration, the state judgment rules are as follows:

- `Horizontal state`: GPIO input level is not trigger level, current tilt has not occurred, LED is off
- `Tilt detected`: GPIO input level equals trigger level, current tilt has occurred, LED is on

## Code Overview

### Example Workflow

```text
Program starts
    ↓
Call ky020_demo_init()
    ↓
Initialize sensor input GPIO and LED output GPIO
    ↓
Create background task named "ky020_demo"
    ↓
Enter task main function ky020_demo_task()
    ↓
Enter periodic loop:
  ├─ Call qosa_gpio_get_level() to read inclination switch input level
  ├─ Call ky020_is_tilted() to determine if trigger level is matched
  ├─ Call ky020_set_led() to control LED on/off
  └─ Output current status log
```

### Main API Interfaces

#### ky020_demo_init

Module startup entry function.

- Check if inclination switch monitoring task has already been created
- Call `ky020_prepare_gpio()` to initialize input and LED output pins
- Create background task and set stack size, priority, and task name
- Output initialization completion log after task creation succeeds

#### ky020_demo_task

Background task handler function.

- Periodically read inclination switch input GPIO level
- Determine current tilt state based on trigger level
- Update LED output level based on judgment result
- Output "Tilt detected" or "Horizontal state" log

#### ky020_prepare_gpio

GPIO initialization function.

- Call `qosa_get_pin_default_cfg()` to get target pin default configuration
- Call `qosa_pin_set_func()` to switch pin to GPIO function
- Call `qosa_gpio_init()` to complete GPIO initialization in input or output mode
- Output error log and return failure status if configuration fails

#### ky020_is_tilted

State judgment function.

- Compare current input level with `KY020_TRIGGER_LEVEL`
- Return current tilt trigger state

#### ky020_set_led

LED control function.

- Set LED high/low level based on tilt state
- Light LED when tilt is detected
- Turn off LED when in horizontal state

#### ky020_get_inactive_level

Output level helper function.

- Derive corresponding invalid level from LED valid level
- Reused during LED initialization and state switching

## Configuration

Default KY-020 inclination switch example configuration is defined in `ky020_demo.c` and can be overridden at compile time via macros:

- `KY020_SENSOR_PIN`: Default sensor input pin is `QOSA_PIN_31`
- `KY020_LED_PIN`: Default LED output pin is `QOSA_PIN_32`
- `KY020_TRIGGER_LEVEL`: Default trigger level is `QOSA_GPIO_LEVEL_LOW`
- `KY020_SENSOR_PULL`: Default input pull-up/pull-down configuration is `QOSA_GPIO_PULL_UP`
- `KY020_POLL_INTERVAL_MS`: Tilt detection and log output period is 1000 ms
- `KY020_TASK_STACK_SIZE`: Background task stack size is 2048

If the actual hardware input pin, LED pin, or trigger level differs from default values, adjust the above macros based on schematic diagram and measured logic. The current example uses GPIO digital input polling method suitable for quick state detection and indicator light linkage control of binary inclination switches like the KY-020.

## Community Forum

[Click here](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

You are welcome to contribute. We recommend submitting changes as follows:

- Run basic validation before submitting: env-setup, build, and clean.
- Use a clear commit message that describes the purpose of the change, its impact scope, and validation results.
- When adding new features or changing behavior, update the README and related documentation at the same time.
- Submit bug fixes and feature improvements through Issues or Pull Requests.
