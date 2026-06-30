"""
@file      : light.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on KY-018 Photoresistor Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
"""
#include "qcm_proj_config.h"
#include "qosa_adc.h"
#include "qosa_gpio.h"
#include "qosa_log.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/*
 * KY-018 光敏传感器默认接 ADC1。
 * 如果你的硬件接到了其他 ADC 通道，可在编译时重定义这个宏。
 */
#ifndef LIGHT_SENSOR_ADC_CHANNEL
#define LIGHT_SENSOR_ADC_CHANNEL QOSA_ADC1_CHANNEL
#endif

/*
 * 若开发板 LED 接在其他引脚，可按实际接线修改。
 */
#ifndef LIGHT_SENSOR_LED_PIN_NUM
#define LIGHT_SENSOR_LED_PIN_NUM QOSA_PIN_31
#endif

/*
 * ADC 接口返回单位为 mV。
 * KY-018 分压电路在不同板卡上的输出电压范围会有差异，阈值需要按实测调整。
 */
#ifndef LIGHT_SENSOR_THRESHOLD_MV
#define LIGHT_SENSOR_THRESHOLD_MV 500
#endif

#ifndef LIGHT_SENSOR_ADC_SCALE
#define LIGHT_SENSOR_ADC_SCALE QOSA_ADC_SCALE_LEVEL_2
#endif

/* 默认每 500ms 采样一次。 */
#ifndef LIGHT_SENSOR_POLL_MS
#define LIGHT_SENSOR_POLL_MS 500
#endif

#ifndef LIGHT_SENSOR_TASK_STACK_SIZE
#define LIGHT_SENSOR_TASK_STACK_SIZE 2048
#endif

static qosa_task_t g_light_sensor_task = QOSA_NULL;
static qosa_gpio_num_e g_light_sensor_led_gpio_num = QOSA_GPIO_0;
static qosa_bool_t g_light_sensor_led_ready = QOSA_FALSE;

/**
 * @brief 返回当前电压对应的光照状态描述。
 *
 * KY-018 常见特性为：光照越强，输出电压越低；光照越弱，输出电压越高。
 */
static const char *light_sensor_status_name(int voltage_mv)
{
	if (voltage_mv < LIGHT_SENSOR_THRESHOLD_MV)
	{
		return "strong_light";
	}

	return "weak_light";
}

/**
 * @brief 初始化 LED GPIO，供后台任务联动控制。
 */
static int light_sensor_led_init(void)
{
	qosa_pin_cfg_t pin_cfg;
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pinctrl_ret;

	g_light_sensor_led_ready = QOSA_FALSE;
	gpio_ret = qosa_get_pin_default_cfg(LIGHT_SENSOR_LED_PIN_NUM, &pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("get led pin %d default cfg failed, ret=%d", LIGHT_SENSOR_LED_PIN_NUM, gpio_ret);
		return -1;
	}

	pinctrl_ret = qosa_pin_set_func(pin_cfg.pin_num, pin_cfg.gpio_func);
	if (pinctrl_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("set led pin %d gpio func failed, ret=%d", pin_cfg.pin_num, pinctrl_ret);
		return -1;
	}

	gpio_ret = qosa_gpio_init(pin_cfg.gpio_num,
					 QOSA_GPIO_DIRECTION_OUTPUT,
					 QOSA_GPIO_PULL_NONE,
					 QOSA_GPIO_LEVEL_LOW);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("init led gpio %d failed, ret=%d", pin_cfg.gpio_num, gpio_ret);
		return -1;
	}

	g_light_sensor_led_gpio_num = pin_cfg.gpio_num;
	g_light_sensor_led_ready = QOSA_TRUE;
	QLOGI("light sensor led init ok, pin=%d gpio=%d", pin_cfg.pin_num, pin_cfg.gpio_num);
	return 0;
}

/**
 * @brief 初始化 ADC 量程。
 */
static int light_sensor_adc_init(void)
{
	qosa_adc_aux_scale_e scale = LIGHT_SENSOR_ADC_SCALE;
	qosa_adc_errcode_e adc_ret = qosa_adc_ioctl(LIGHT_SENSOR_ADC_CHANNEL, QOSA_ADC_IOCTL_SET_SCALE, &scale);

	if (adc_ret != QOSA_ADC_SUCCESS)
	{
		QLOGE("set adc channel %d scale failed, ret=%d", LIGHT_SENSOR_ADC_CHANNEL, adc_ret);
		return -1;
	}

	QLOGI("light sensor adc init ok, channel=%d threshold=%dmV poll=%dms",
		  LIGHT_SENSOR_ADC_CHANNEL,
		  LIGHT_SENSOR_THRESHOLD_MV,
		  LIGHT_SENSOR_POLL_MS);
	return 0;
}

/**
 * @brief 控制 LED 状态。
 *
 * - 电压低于阈值，认为当前光线较强，点亮 LED
 * - 电压高于等于阈值，认为当前光线较弱，熄灭 LED
 *
 * 如果你想做自动路灯，可将这里的高低电平逻辑反转。
 */
static void light_sensor_set_led(qosa_bool_t turn_on)
{
	if (g_light_sensor_led_ready != QOSA_TRUE)
	{
		return;
	}

	(void)qosa_gpio_set_level(g_light_sensor_led_gpio_num,
					  turn_on == QOSA_TRUE ? QOSA_GPIO_LEVEL_HIGH : QOSA_GPIO_LEVEL_LOW);
}

/**
 * @brief KY-018 后台监控任务。
 *
 * 任务持续读取 ADC 电压，并根据阈值判断当前光照强弱，随后联动控制 LED。
 */
static void light_sensor_demo_task(void *argv)
{
	int voltage_mv;
	qosa_bool_t led_on;

	(void)argv;

	if ((light_sensor_led_init() != 0) || (light_sensor_adc_init() != 0))
	{
		qosa_task_t current_task = QOSA_NULL;

		g_light_sensor_task = QOSA_NULL;
		(void)qosa_task_get_current_ref(&current_task);
		if (current_task != QOSA_NULL)
		{
			qosa_task_delete(current_task);
		}
		return;
	}

	QLOGI("KY-018 light sensor task started");

	while (1)
	{
		qosa_adc_errcode_e adc_ret = qosa_adc_get_volt(LIGHT_SENSOR_ADC_CHANNEL, &voltage_mv);

		if (adc_ret != QOSA_ADC_SUCCESS)
		{
			QLOGE("read adc channel %d failed, ret=%d", LIGHT_SENSOR_ADC_CHANNEL, adc_ret);
			qosa_task_sleep_ms(LIGHT_SENSOR_POLL_MS);
			continue;
		}

		/*
		 * 这里按传感器常见特性判断：电压越低，表示环境越亮。
		 * 强光亮灯，弱光灭灯。
		 */
		led_on = (voltage_mv < LIGHT_SENSOR_THRESHOLD_MV) ? QOSA_TRUE : QOSA_FALSE;
		light_sensor_set_led(led_on);

		QLOGI("light sensor: voltage=%dmV | status=%s | led=%s",
			  voltage_mv,
			  light_sensor_status_name(voltage_mv),
			  led_on == QOSA_TRUE ? "on" : "off");

		qosa_task_sleep_ms(LIGHT_SENSOR_POLL_MS);
	}
}

/**
 * @brief KY-018 应用初始化入口。
 *
 * 只在启动时创建一次后台任务，避免阻塞系统初始化流程。
 */
static void light_sensor_demo_init(void)
{
	int ret;

	if (g_light_sensor_task != QOSA_NULL)
	{
		QLOGW("KY-018 light sensor demo already started");
		return;
	}

	ret = qosa_task_create(&g_light_sensor_task,
				       LIGHT_SENSOR_TASK_STACK_SIZE,
				       QOSA_PRIORITY_NORMAL,
				       "ky018_light",
				       light_sensor_demo_task,
				       QOSA_NULL);
	if (ret != QOSA_OK)
	{
		QLOGE("create KY-018 task failed, ret=%d", ret);
		g_light_sensor_task = QOSA_NULL;
		return;
	}

	QLOGI("KY-018 light sensor demo started");
}

/* 将 KY-018 光敏传感器示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(200, "ky018_light_demo", light_sensor_demo_init);
