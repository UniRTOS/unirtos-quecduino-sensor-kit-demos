/*
@file      : flame_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on KY-026 Flame Detection Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
*/
#include "qcm_proj_config.h"
#include "qosa_adc.h"
#include "qosa_gpio.h"
#include "qosa_log.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* 火焰传感器示例参数：ADC 通道、报警 LED 引脚、阈值和任务配置。 */
#define FLAME_ADC_CHANNEL              QOSA_ADC1_CHANNEL
#define FLAME_LED_PIN_NUM              31
#define FLAME_SAFE_THRESHOLD_MV        100
#define FLAME_WARNING_THRESHOLD_MV     500
#define FLAME_MONITOR_INTERVAL_MS      1000
#define FLAME_BLINK_INTERVAL_MS        500
#define FLAME_TASK_STACK_SIZE          2048
#define FLAME_TASK_PRIORITY            QOSA_PRIORITY_NORMAL

typedef struct
{
	/* 火焰传感器连接的 ADC 通道。 */
	qosa_adc_channel_e adc_channel;
	/* 报警 LED 使用的 PIN 编号。 */
	qosa_uint8_t       led_pin_num;
	/* PIN 映射后的 GPIO 编号。 */
	qosa_gpio_num_e    led_gpio_num;
} flame_sensor_t;

static flame_sensor_t g_flame_sensor = {
	.adc_channel = FLAME_ADC_CHANNEL,
	.led_pin_num = FLAME_LED_PIN_NUM,
	.led_gpio_num = QOSA_GPIO_31,
};

static qosa_task_t g_flame_task = QOSA_NULL;

/* 初始化火焰报警 LED 的 GPIO。 */
static int flame_led_init(flame_sensor_t *sensor)
{
	qosa_pin_cfg_t pin_cfg = {0};
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pin_ret;

	gpio_ret = qosa_get_pin_default_cfg(sensor->led_pin_num, &pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("flame get led pin cfg failed, pin=%u, ret=%d", sensor->led_pin_num, gpio_ret);
		return -1;
	}

	pin_ret = qosa_pin_set_func((qosa_pin_num_e)pin_cfg.pin_num, pin_cfg.gpio_func);
	if (pin_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("flame set led pin func failed, pin=%u, func=%u, ret=%d", pin_cfg.pin_num, pin_cfg.gpio_func, pin_ret);
		return -1;
	}

	gpio_ret = qosa_gpio_init(pin_cfg.gpio_num, QOSA_GPIO_DIRECTION_OUTPUT, QOSA_GPIO_PULL_NONE, QOSA_GPIO_LEVEL_LOW);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("flame led gpio init failed, gpio=%d, ret=%d", pin_cfg.gpio_num, gpio_ret);
		return -1;
	}

	sensor->led_gpio_num = pin_cfg.gpio_num;
	QLOGI("flame led init ok, pin=%u, gpio=%d", sensor->led_pin_num, sensor->led_gpio_num);
	return 0;
}

/* 初始化火焰传感器 ADC 量程。 */
static int flame_adc_init(const flame_sensor_t *sensor)
{
	qosa_adc_aux_scale_e scale = QOSA_ADC_SCALE_LEVEL_2;
	qosa_adc_errcode_e ret;

	ret = qosa_adc_ioctl(sensor->adc_channel, QOSA_ADC_IOCTL_SET_SCALE, &scale);
	if (ret != QOSA_ADC_SUCCESS)
	{
		QLOGE("flame adc set scale failed, channel=%d, ret=%d", sensor->adc_channel, ret);
		return -1;
	}

	QLOGI("flame adc init ok, channel=%d", sensor->adc_channel);
	return 0;
}

/* 设置火焰报警 LED 电平。 */
static void flame_led_set(const flame_sensor_t *sensor, qosa_gpio_level_e level)
{
	qosa_gpio_error_e ret;

	ret = qosa_gpio_set_level(sensor->led_gpio_num, level);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("flame led set failed, gpio=%d, level=%d, ret=%d", sensor->led_gpio_num, level, ret);
	}
}

/* 读取火焰传感器当前 ADC 电压值。 */
static int flame_read_value(const flame_sensor_t *sensor, int *value_mv)
{
	qosa_adc_errcode_e ret;

	ret = qosa_adc_get_volt(sensor->adc_channel, value_mv);
	if (ret != QOSA_ADC_SUCCESS)
	{
		QLOGE("flame adc read failed, channel=%d, ret=%d", sensor->adc_channel, ret);
		return -1;
	}

	return 0;
}

/* 火警状态下闪烁 LED。 */
static void flame_led_blink(const flame_sensor_t *sensor)
{
	flame_led_set(sensor, QOSA_GPIO_LEVEL_HIGH);
	qosa_task_sleep_ms(FLAME_BLINK_INTERVAL_MS);
	flame_led_set(sensor, QOSA_GPIO_LEVEL_LOW);
	qosa_task_sleep_ms(FLAME_BLINK_INTERVAL_MS);
}

/* 火焰监控任务，根据 ADC 阈值输出安全、风险或报警状态。 */
static void flame_monitor_task(void *argv)
{
	flame_sensor_t *sensor = (flame_sensor_t *)argv;

	if (flame_led_init(sensor) != 0 || flame_adc_init(sensor) != 0)
	{
		QLOGE("flame demo start failed");
		qosa_task_delete(g_flame_task);
		return;
	}

	QLOGI("flame demo started");
	while (1)
	{
		int value_mv = 0;

		if (flame_read_value(sensor, &value_mv) != 0)
		{
			flame_led_set(sensor, QOSA_GPIO_LEVEL_LOW);
			qosa_task_sleep_ms(FLAME_MONITOR_INTERVAL_MS);
			continue;
		}

		if (value_mv < FLAME_SAFE_THRESHOLD_MV)
		{
			flame_led_set(sensor, QOSA_GPIO_LEVEL_LOW);
			QLOGI("ADC: %d mV | status: safe", value_mv);
			qosa_task_sleep_ms(FLAME_MONITOR_INTERVAL_MS);
		}
		else if (value_mv < FLAME_WARNING_THRESHOLD_MV)
		{
			flame_led_set(sensor, QOSA_GPIO_LEVEL_HIGH);
			QLOGI("ADC: %d mV | status: flame risk", value_mv);
			qosa_task_sleep_ms(FLAME_MONITOR_INTERVAL_MS);
		}
		else
		{
			QLOGI("ADC: %d mV | status: fire alarm", value_mv);
			flame_led_blink(sensor);
		}
	}
}

/* 火焰检测示例初始化入口，负责创建监控任务。 */
static void flame_demo_init(void)
{
	int ret;

	ret = qosa_task_create(&g_flame_task, FLAME_TASK_STACK_SIZE, FLAME_TASK_PRIORITY, "flame", flame_monitor_task, &g_flame_sensor);
	if (ret != QOSA_ERROR_OK)
	{
		QLOGE("flame task create failed, ret=%d", ret);
	}
}

/* 将 KY-026 火焰检测示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(200, "flame_demo", flame_demo_init);
