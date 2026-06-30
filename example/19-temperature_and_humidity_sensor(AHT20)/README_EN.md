# Temperature and Humidity Sensor

## **Module Introduction**

The temperature and humidity sensor, as one of common sensors, is a sensor device equipped with humidity-sensitive and temperature-sensitive elements that can measure temperature and humidity. Its working principle is mainly based on the characteristics of thermal resistors and humidity-sensitive resistors, which measure resistance values and convert them into voltage signal output, achieving accurate monitoring of environmental temperature and humidity.

**Working Principle:**

The module collects environmental data through internal thermal sensing elements and humidity-sensing elements. After chip calibration, it outputs **I2C digital signals**. The development board reads temperature and humidity values through the I2C bus.

## Connection Example

Connect peripherals to the development board according to the table and diagram:

| Peripheral | Development Board |
| ------------ | ------ |
| AHT20 (+) | 3.3V   |
| AHT20 (-) | GND    |
| AHT20 (SCL) | PIN67  |
| AHT20 (SDA) | PIN66  |

## Quick Start

### 1. Development Environment Setup

Refer to [UNIRTOS Quick Start](https://docs.quectel.com/zh/UniRTOS/UniRTOS%E6%96%87%E6%A1%A3/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B/%E5%BF%AB%E9%80%9F%E4%B8%8A%E6%89%8B.html) documentation to learn how to set up the development environment and complete the basic development workflow.

### 2. Code Repository Cloning

```
# Clone the example repository
unirtos-cli new -r unirtos-quecduino-sensor-kit-demos
# Enter the project
cd unirtos-quecduino-sensor-kit-demos-1.0.0/example/19-temperature_and_humidity_sensor(AHT20)
```

### 3. Project Structure

```text
19-temperature_and_humidity_sensor(AHT20)/
├── CMakeLists.txt      # AHT20 Demo local build configuration
├── env_config.json     # UniRTOS project environment configuration
├── Kconfig             # Example configuration switch
├── aht20_demo.c        # AHT20 temperature and humidity sensor example source code
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
[I/LOG_TAG_DEMO] AHT20 demo thread created
[I/LOG_TAG_DEMO] AHT20 demo started on I2C channel 1 slave 0x38
```

During operation, the example continuously reads AHT20 sensor data in the background task and outputs current temperature, humidity, and environmental status with a default 1 s period. Typical logs are as follows:

```text
[I/LOG_TAG_DEMO] Temperature: 24.6C | Humidity: 45.3% | Status: Comfortable
[I/LOG_TAG_DEMO] Temperature: 16.8C | Humidity: 41.2% | Status: Cold
[I/LOG_TAG_DEMO] Temperature: 27.1C | Humidity: 75.6% | Status: Humid
```

If measurement triggering or sample reading fails, alarm logs are also displayed:

```text
[W/LOG_TAG_DEMO] Read failed
```

Under default configuration, the environmental status judgment rules are as follows:

- `Cold`: Temperature below 18.0°C
- `Hot`: Temperature above 28.0°C
- `Dry`: Temperature between 18.0°C and 28.0°C, and humidity below 30.0%
- `Humid`: Temperature between 18.0°C and 28.0°C, and humidity above 70.0%
- `Comfortable`: Both temperature and humidity are within default comfort range

## Code Overview

### Example Workflow

```text
Program starts
    ↓
Call aht20_demo_init()
    ↓
Create background task named "aht20_demo"
    ↓
Enter task main function aht20_demo_thread()
    ↓
Call aht20_configure_bus()
    ↓
Configure SDA/SCL pin multiplexing and initialize I2C bus
    ↓
Call aht20_reset_sensor()
    ↓
Send soft reset command to AHT20
    ↓
Call aht20_initialize_sensor()
    ↓
Send initialization command and wait for sensor ready
    ↓
Enter periodic loop:
  ├─ Call aht20_read_measurement() to trigger one measurement and read raw data
  ├─ Convert raw data to temperature and humidity
  ├─ Call aht20_check_comfort() to determine environmental status
  └─ Output current temperature, humidity, and status log
```

### Main API Interfaces

#### aht20_demo_init

Module startup entry function.

- Check if AHT20 monitoring task has already been created
- Create background task and set stack size, priority, and task name
- Output startup log after task creation succeeds

#### aht20_demo_thread

Background task handler function.

- Call I2C bus configuration function to complete pin multiplexing and bus initialization
- Execute sensor reset and initialization process sequentially
- Enter long-term loop and trigger measurement at fixed intervals then read data
- Determine current environmental status based on temperature and humidity results
- Output current temperature, humidity, and status log

#### aht20_configure_bus

I2C bus initialization function.

- Call `qosa_pin_set_func()` to configure SDA and SCL pin multiplexing function
- Call `qosa_i2c_init()` to initialize target I2C channel
- Output error log and return failure status if configuration fails

#### aht20_reset_sensor

Sensor reset function.

- Call `qosa_i2c_write()` to send soft reset command to AHT20
- Wait default 20 ms after successful reset
- Output error log and return failure status if reset fails

#### aht20_initialize_sensor

Sensor initialization function.

- Call `qosa_i2c_write()` to send initialization command to AHT20
- Output error log and return failure status if initialization fails

#### aht20_read_measurement

Measurement read function.

- Call `qosa_i2c_write()` to trigger one temperature and humidity measurement
- After waiting default 80 ms, call `qosa_i2c_read()` to read 6 bytes of measurement result
- Check status bits to confirm sensor is not currently in busy state
- Convert raw data to temperature and humidity decimal values

#### aht20_check_comfort

Environmental status judgment function.

- First determine if `Cold` or `Hot` based on temperature
- When temperature is in comfort range, then determine if `Dry` or `Humid` based on humidity
- Return `Comfortable` if both temperature and humidity are within default threshold ranges

## Configuration

Default AHT20 example configuration is defined in `aht20_demo.c` and is currently fixed via macros, which can be adjusted directly based on actual hardware connection and application requirements:

- `AHT20_I2C_ADDR_7BIT`: Default slave device 7-bit address is `0x38`
- `AHT20_CMD_RESET`: Soft reset command byte is `0xBA`
- `AHT20_CMD_INIT`: Initialization command byte is `0xE1`
- `AHT20_CMD_MEASURE`: Measurement trigger command byte is `0xAC`
- `AHT20_RESET_DELAY_MS`: Wait time after reset is 20 ms
- `AHT20_INIT_DELAY_MS`: Wait time after initialization is 1000 ms
- `AHT20_MEASURE_DELAY_MS`: Wait time after measurement trigger is 80 ms
- `AHT20_POLL_INTERVAL_MS`: Log output period is 1000 ms
- `AHT20_THREAD_STACK_SIZE`: Background task stack size is 2048
- `AHT20_I2C_SDA_PIN`: Default SDA pin is `QOSA_PIN_66`
- `AHT20_I2C_SCL_PIN`: Default SCL pin is `QOSA_PIN_67`
- `AHT20_I2C_FUNC`: Default pin multiplexing function number is `2`
- `AHT20_I2C_CHANNEL`: Default I2C channel is `QOSA_I2C_1`

If the actual hardware I2C channel, pin multiplexing function, sensor address, or sampling period differs from default values, adjust the above macros based on schematic diagram and measured situation. The current example uses polling method to periodically read AHT20 data and completes basic comfort level judgment at application layer, suitable as entry reference for temperature and humidity sampling and I2C peripheral integration.

## Community Forum

[Click here](https://forumschinese.quectel.com/c/66-category/66)

## Contribution Guide

Issues and Pull Requests are welcome.
