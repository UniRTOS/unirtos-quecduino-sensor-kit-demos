# Beep Demo

## Overview

Beep Demo is a buzzer driver example project based on UniRTOS. This project demonstrates how to query GPIO default configurations, complete PINMUX switching, initialize GPIO output direction, and periodically pull high or low control levels through background tasks to drive the buzzer on the UniRTOS platform. Through this example, developers can quickly understand the basic usage of UniRTOS GPIO, PINCTRL, and task APIs.

## **Module Introduction**

There are many interactive works that can be completed with Arduino. The most common and frequently used is sound and light display. Previously, experiments were mainly conducted using LED lights, but this experiment makes the circuit emit sound. The most common components that can emit sound are buzzers and speakers. Compared to speakers, buzzers are simpler and easier to use, so we use a buzzer in this experiment.

**Buzzer and Its Principles**

**(I) Buzzer Introduction**

1. Buzzer Function: A buzzer is an integrated electronic sound device powered by direct current voltage. It is widely used in computers, printers, copiers, alarm devices, electronic toys, automotive electronic equipment, telephones, timers, and other electronic products as a sound-emitting component.

2. Buzzer Classification: Buzzers are mainly divided into two types: piezoelectric buzzers and electromagnetic buzzers.

3. Buzzer Circuit Symbol: Buzzers are represented by the letters "H" or "HA" in circuits (old standards used "FM", "LB", "JD", etc.).

**(II) Buzzer Structure and Principles**

1. Piezoelectric Buzzer: A piezoelectric buzzer is mainly composed of an astable multivibrator, a piezoelectric buzzer element, an impedance matcher and resonance box, an outer shell, etc. Some piezoelectric buzzers also have an LED mounted on the shell. The astable multivibrator is composed of transistors or integrated circuits. When power is applied (1.5~15V DC operating voltage), the multivibrator oscillates and outputs a 1.5~2.5 kHz audio signal. The impedance matcher drives the piezoelectric buzzer element to produce sound. The piezoelectric buzzer element is made from lead zirconate titanate or lead magnesium niobate piezoelectric ceramic material. After the ceramic sheet is coated with silver electrodes on both sides and undergoes polarization and aging treatment, it is bonded to a brass or stainless steel sheet.

2. Electromagnetic Buzzer: An electromagnetic buzzer is composed of an oscillator, electromagnetic coil, magnet, vibration membrane, and outer shell, etc. When power is applied, the oscillator generates audio signal current that flows through the electromagnetic coil, causing the coil to produce a magnetic field. The vibration membrane oscillates periodically under the interaction of the electromagnetic coil and magnet, producing sound.

**Difference Between Active and Passive Buzzers**

"Source" here does not refer to the power source, but to the oscillation source. An active buzzer has a built-in oscillation source, so it will buzz as soon as power is supplied. A passive buzzer has no built-in oscillation source, so direct current cannot make it buzz. It requires a 2K~5K square wave to drive it. Active buzzers are often more expensive than passive ones because they have an additional oscillation circuit. Advantages of passive buzzers: 1. Cheap, 2. Controllable sound frequency, can produce "do-re-mi-fa-sol-la-ti" effects, 3. In some special cases, can share a control pin with an LED. Advantages of active buzzers: convenient program control.

**Sound Emission Principle:**

![](../../media/buzzer1.png)

The active buzzer module is triggered by low level. By configuring the I/O port and giving it a low level, it will emit sound. As shown in the circuit diagram above.

## Connection Example

Following the table and images, connect the peripherals to the development board one-to-one correspondingly.

| Peripheral       | Development Board |
| ---------------- | ----------------- |
| BUZZER (+)       | 3.3V              |
| BUZZER (-)       | GND               |
| BUZZER (S)       | PIN19             |

## Quick Start

### 1. Development Environment Setup

Refer to the [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Retrieval

```
# Retrieve the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/05-buzzer
```

### 3. Project Structure

```text
05-buzzer/
├── CMakeLists.txt      # Beep Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── beep_demo.c         # Buzzer example source code
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
[I/LOG_TAG_DEMO] beep demo init finished, pin=19
[I/LOG_TAG_DEMO] beep gpio ready: pin=19 gpio=19 active=1
[I/LOG_TAG_DEMO] beep alarm armed: initial_delay=10 sec, period=10 sec, repeat=3
```

With the default configuration, the example will wait approximately 10 seconds, then trigger a buzzer sequence every 10 seconds. Each sequence produces 3 consecutive beeps, with each beep lasting approximately 180 ms and an interval of approximately 120 ms between beeps. During execution you can see the following periodic log:

```text
[I/LOG_TAG_DEMO] beep alarm triggered, cycle=1
[I/LOG_TAG_DEMO] beep alarm triggered, cycle=2
[I/LOG_TAG_DEMO] beep alarm triggered, cycle=3
...
```

## Code Overview

### Example Workflow

```text
Program Startup
    ↓
Call beep_demo_bootstrap()
    ↓
Create background task named "beep_alarm"
    ↓
Enter task main function beep_demo_alarm_task()
    ↓
Call beep_demo_prepare_gpio()
    ↓
Read default pin configuration
    ↓
Switch pin multiplexing to GPIO functionality
    ↓
Initialize GPIO output and set to idle level
    ↓
Wait for initial delay
    ↓
Enter periodic loop:
  ├─ Record current alarm cycle log
  ├─ Call beep_demo_ring_alarm() for multiple consecutive beeps
  └─ Wait for next cycle
```

### Main API Interfaces

#### beep_demo_bootstrap

Module startup entry function.

- Check if buzzer task is already created
- Create background task with stack size, priority, and task name
- Output initialization complete log after successful task creation

#### beep_demo_alarm_task

Background task processing function.

- Call GPIO preparation function to complete hardware initialization
- Output current alarm cycle configuration log
- Wait for initial delay, then enter long-term loop
- Periodically trigger buzzer beeping task

#### beep_demo_prepare_gpio

GPIO initialization function.

- Query default GPIO configuration for target pin
- Switch PINMUX to GPIO functionality
- Initialize GPIO output direction, pull-up/down, and default level
- Cache initialization state to avoid repeated hardware configuration

#### beep_demo_ring_alarm

Beep execution function.

- Output buzzer pulses according to set count
- Control beep duration and pause time between beeps
- Call low-level level setting function to implement buzzer on/off

#### beep_demo_set_level

GPIO level setting wrapper function.

- Call `qosa_gpio_set_level()` to set output level
- Output error log on failure and return failure status

## Configuration Description

Default buzzer configuration is defined in `beep_demo.c` and can be overridden at compile time through macros:

- `BEEP_DEMO_PIN_NUM`: Default buzzer control pin is `QOSA_PIN_19`
- `BEEP_DEMO_ACTIVE_LEVEL`: Default active level is `QOSA_GPIO_LEVEL_HIGH`
- `BEEP_DEMO_INITIAL_DELAY_SEC`: Wait 10 seconds before first beep
- `BEEP_DEMO_PERIOD_SEC`: 10-second interval between buzzer cycles
- `BEEP_DEMO_BEEP_ON_MS`: Each beep lasts 180 ms
- `BEEP_DEMO_BEEP_OFF_MS`: 120 ms pause between beeps in same cycle
- `BEEP_DEMO_RING_REPEAT_COUNT`: 3 consecutive beeps per cycle
- `BEEP_DEMO_TASK_STACK_SIZE`: Background task stack size is 2048
- `BEEP_DEMO_TASK_NAME`: Background task name is `beep_alarm`

If your hardware uses low-level triggering for the buzzer or the buzzer is connected to a different GPIO, adjust the pin number and active level configuration according to your actual schematic. Different platforms may have different PINMUX default configurations, so when porting, please verify that the corresponding function number is correct according to your platform PIN table.

## Community Forum

[Click here to enter](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guidelines

We welcome Issue submissions and Pull Requests.
