"""
@file      : ky020_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on KY-020 Inclination Switch Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
"""
#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* KY-020 倾斜开关输入引脚，默认可在编译时重定义。 */
#ifndef KY020_SENSOR_PIN
#define KY020_SENSOR_PIN QOSA_PIN_31
#endif

#ifndef KY020_LED_PIN
#define KY020_LED_PIN QOSA_PIN_32
#endif

#ifndef KY020_TRIGGER_LEVEL
#define KY020_TRIGGER_LEVEL QOSA_GPIO_LEVEL_LOW
#endif

#ifndef KY020_SENSOR_PULL
#define KY020_SENSOR_PULL QOSA_GPIO_PULL_UP
#endif

#define KY020_OUTPUT_ACTIVE_LEVEL QOSA_GPIO_LEVEL_HIGH
#define KY020_POLL_INTERVAL_MS    1000U
#define KY020_TASK_STACK_SIZE     2048U

static qosa_task_t g_ky020_demo_task = QOSA_NULL;
static qosa_pin_cfg_t g_ky020_sensor_pin_cfg;
static qosa_pin_cfg_t g_ky020_led_pin_cfg;

/* 根据 LED 有效电平计算熄灭电平。 */
static qosa_gpio_level_e ky020_get_inactive_level(qosa_gpio_level_e active_level)
{
	return (active_level == QOSA_GPIO_LEVEL_HIGH) ? QOSA_GPIO_LEVEL_LOW : QOSA_GPIO_LEVEL_HIGH;
}

/* 判断当前输入电平是否表示倾斜。 */
static qosa_bool_t ky020_is_tilted(qosa_gpio_level_e level)
{
	return (level == KY020_TRIGGER_LEVEL) ? QOSA_TRUE : QOSA_FALSE;
}

/* 根据倾斜状态控制 LED。 */
static void ky020_set_led(qosa_bool_t active)
{
	qosa_gpio_level_e led_level = ky020_get_inactive_level(KY020_OUTPUT_ACTIVE_LEVEL);

	if (active == QOSA_TRUE)
	{
		led_level = KY020_OUTPUT_ACTIVE_LEVEL;
	}

	(void)qosa_gpio_set_level(g_ky020_led_pin_cfg.gpio_num, led_level);
}

/* 初始化 KY-020 示例使用的 GPIO。 */
static int ky020_prepare_gpio(qosa_pin_num_e pin_num,
						   qosa_gpio_direction_e direction,
						   qosa_gpio_pull_e pull,
						   qosa_gpio_level_e init_level,
						   qosa_pin_cfg_t *pin_cfg)
{
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pin_ret;

	gpio_ret = qosa_get_pin_default_cfg(pin_num, pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("ky020 get pin cfg failed, pin=%d ret=%d", pin_num, gpio_ret);
		return -1;
	}

	pin_ret = qosa_pin_set_func(pin_cfg->pin_num, pin_cfg->gpio_func);
	if (pin_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("ky020 set pin func failed, pin=%d ret=%d", pin_cfg->pin_num, pin_ret);
		return -1;
	}

	gpio_ret = qosa_gpio_init(pin_cfg->gpio_num, direction, pull, init_level);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("ky020 gpio init failed, gpio=%d ret=%d", pin_cfg->gpio_num, gpio_ret);
		return -1;
	}

	return 0;
}

/* KY-020 后台任务，周期读取倾斜状态并控制 LED。 */
static void ky020_demo_task(void *argv)
{
	qosa_gpio_level_e sensor_level = QOSA_GPIO_LEVEL_HIGH;
	qosa_bool_t tilted;

	(void)argv;

	QLOGI("ky020 demo started: sensor pin=%d gpio=%d, led pin=%d gpio=%d",
		  g_ky020_sensor_pin_cfg.pin_num,
		  g_ky020_sensor_pin_cfg.gpio_num,
		  g_ky020_led_pin_cfg.pin_num,
		  g_ky020_led_pin_cfg.gpio_num);

	while (1)
	{
		if (qosa_gpio_get_level(g_ky020_sensor_pin_cfg.gpio_num, &sensor_level) != QOSA_GPIO_SUCCESS)
		{
			QLOGW("ky020 read gpio %d failed", g_ky020_sensor_pin_cfg.gpio_num);
			qosa_task_sleep_ms(KY020_POLL_INTERVAL_MS);
			continue;
		}

		tilted = ky020_is_tilted(sensor_level);
		ky020_set_led(tilted);

		if (tilted == QOSA_TRUE)
		{
			QLOGI("检测到倾斜");
		}
		else
		{
			QLOGI("水平状态");
		}

		qosa_task_sleep_ms(KY020_POLL_INTERVAL_MS);
	}
}

/* KY-020 示例初始化入口，配置输入和 LED 后创建任务。 */
static void ky020_demo_init(void)
{
	int ret;
	qosa_gpio_level_e led_inactive_level = ky020_get_inactive_level(KY020_OUTPUT_ACTIVE_LEVEL);

	if (g_ky020_demo_task != QOSA_NULL)
	{
		QLOGW("ky020 demo task already started");
		return;
	}

	if (ky020_prepare_gpio(KY020_SENSOR_PIN,
					  QOSA_GPIO_DIRECTION_INPUT,
					  KY020_SENSOR_PULL,
					  QOSA_GPIO_LEVEL_LOW,
					  &g_ky020_sensor_pin_cfg) != 0)
	{
		return;
	}

	if (ky020_prepare_gpio(KY020_LED_PIN,
					  QOSA_GPIO_DIRECTION_OUTPUT,
					  QOSA_GPIO_PULL_NONE,
					  led_inactive_level,
					  &g_ky020_led_pin_cfg) != 0)
	{
		(void)qosa_gpio_deinit(g_ky020_sensor_pin_cfg.gpio_num);
		return;
	}

	ret = qosa_task_create(&g_ky020_demo_task,
						   KY020_TASK_STACK_SIZE,
						   QOSA_PRIORITY_NORMAL,
						   "ky020_demo",
						   ky020_demo_task,
						   QOSA_NULL);
	if (ret != QOSA_ERROR_OK)
	{
		g_ky020_demo_task = QOSA_NULL;
		(void)qosa_gpio_deinit(g_ky020_sensor_pin_cfg.gpio_num);
		(void)qosa_gpio_deinit(g_ky020_led_pin_cfg.gpio_num);
		QLOGE("create ky020 demo task failed ret=%d", ret);
		return;
	}

	QLOGI("ky020 demo init done");
}

/* 将 KY-020 倾斜开关示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(220, "ky020_demo", ky020_demo_init);
