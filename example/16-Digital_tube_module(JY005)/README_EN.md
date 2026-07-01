# Digital Tube Module

## **Module Introduction**

Single-digit digital tube module is a **digital display device** composed of 7 LED segments, used for displaying digits 0-9 and simple symbols, widely used in counting, timing, status display, and maker DIY scenarios. It features high brightness, clear display, 3.3V/5V compatibility, simple driving, and long service life.

**LED Composition:**

7-segment LED emitting sections, common terminal, decimal point, current-limiting resistor, PCB board, terminal connectors.

**Lighting Principle:**

The module has positive, negative, and segment selection signal terminals. By controlling the on/off of different segments, digits 0-9 can be displayed. The development board controls corresponding segments to light up via GPIO output levels.

## Connection Example

Connect peripherals to the development board according to the table and diagram:

| Peripheral | Development Board |
| -------- | ------ |
| LED (+) | 3.3V   |
| LED (-) | GND    |
| LED (S) | Select  |

## Quick Start

### 1. Development Environment Setup

Refer to [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Repository Cloning

```
# Clone the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/16-Digital_tube_module(JY005)
```

### 3. Project Structure

```
16-Digital_tube_module(JY005)/
├── CMakeLists.txt      # JY005 Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── jy005_demo.c        # JY005 digital tube example source code
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
[I/LOG_TAG_DEMO] jy005 demo task created
[I/LOG_TAG_DEMO] jy005 demo start: a=32 b=31 c=30 d=33 e=80 f=58 g=29 dp=28
```

During operation, the example switches segment codes in the background task every 1 second, cyclically displaying digits 0 through 9 in order. The current implementation does not output individual logs for each digit switch by default; the display effect is directly reflected in the digital tube hardware.

Under default configuration, the example uses common-anode reverse logic segment code table:

- Segment code value `0`: Corresponding segment lights up
- Segment code value `1`: Corresponding segment turns off
- `dp` segment remains off by default and does not participate in digit cycle display

## Code Overview

### Example Workflow

```
Program starts
    ↓
Call jy005_demo_init()
    ↓
Create background task named "jy005"
    ↓
Enter task main function jy005_demo_task()
    ↓
Call jy005_gpio_init()
    ↓
Sequentially initialize a, b, c, d, e, f, g, dp 8 segment pins as GPIO output mode
    ↓
Call jy005_clear_display()
    ↓
Output current segment pin mapping log
    ↓
Enter periodic loop:
  ├─ Call jy005_display_num() to output current digit's corresponding segment code
  ├─ Call qosa_task_sleep_ms() to delay 1 second
  └─ Digits cycle display from 0 to 9 in order
```

### Main API Interfaces

#### jy005_demo_init

Module startup entry function.

- Check if JY005 display task has already been created
- Create background task and set stack size, priority, and task name
- Output startup log after task creation succeeds

#### jy005_demo_task

Background task handler function.

- Call GPIO initialization function to complete configuration of 8 segment pins
- Call clear screen function to default pull high all segments to turn off
- Output current mapping of a, b, c, d, e, f, g, dp segment pins
- Enter long-term loop and cyclically display 0 through 9 at fixed intervals

#### jy005_segment_gpio_init

Single segment pin initialization function.

- Call `qosa_get_pin_default_cfg()` to get segment pin default configuration
- Call `qosa_pin_set_func()` to switch pin to GPIO function
- Call `qosa_gpio_init()` to initialize corresponding GPIO to output mode
- Default pull GPIO output high to turn off corresponding segment

#### jy005_gpio_init

Batch GPIO initialization function.

- Traverse a, b, c, d, e, f, g, dp 8 segment pin configurations
- Call `jy005_segment_gpio_init()` sequentially to complete initialization
- Return failure status immediately if any segment initialization fails

#### jy005_clear_display

Digital tube clear screen function.

- Traverse all segment pins and output high level
- Restore all segments to off state

#### jy005_display_num

Number display function.

- Check if input digit is within 0 to 9 range
- Read current digit's corresponding 8-segment state from segment code table
- Convert segment code value to GPIO level and output to corresponding segment pin

## Configuration

Default JY005 example configuration is defined in [qos_applications/JY005_demos/jy005_demo.c](qos_applications/JY005_demos/jy005_demo.c) and can be overridden at compile time via macros:

- `JY005_SEG_A_PIN`: Default a segment pin is `QOSA_PIN_32`
- `JY005_SEG_B_PIN`: Default b segment pin is `QOSA_PIN_31`
- `JY005_SEG_C_PIN`: Default c segment pin is `QOSA_PIN_30`
- `JY005_SEG_D_PIN`: Default d segment pin is `QOSA_PIN_33`
- `JY005_SEG_E_PIN`: Default e segment pin is `QOSA_PIN_80`
- `JY005_SEG_F_PIN`: Default f segment pin is `QOSA_PIN_58`
- `JY005_SEG_G_PIN`: Default g segment pin is `QOSA_PIN_29`
- `JY005_SEG_DP_PIN`: Default dp segment pin is `QOSA_PIN_28`
- `JY005_DISPLAY_INTERVAL_MS`: Default display duration for each digit is 1000 ms
- `JY005_TASK_STACK_SIZE`: Background task stack size is 2048
- `JY005_TASK_PRIORITY`: Background task priority is `QOSA_PRIORITY_NORMAL`

If the actual hardware segment pins differ from default mapping, adjust the above macros based on schematic diagram and physical wiring. The current example implements common-anode digital tube logic. If you are using a common-cathode digital tube, you need to synchronously adjust the polarity of levels in `jy005_display_num()` or directly modify the segment code table.

## Community Forum

[Click here](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

You are welcome to contribute. We recommend submitting changes as follows:

- Run basic validation before submitting: env-setup, build, and clean.
- Use a clear commit message that describes the purpose of the change, its impact scope, and validation results.
- When adding new features or changing behavior, update the README and related documentation at the same time.
- Submit bug fixes and feature improvements through Issues or Pull Requests.
