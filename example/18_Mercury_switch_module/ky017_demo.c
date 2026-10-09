/*
@file      : ky017_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on KY-017 Mercury Switch Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
*/
#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* KY-017 水银开关输入和联动输出引脚，支持编译时重定义。 */
#ifndef KY017_SENSOR_PIN
#define KY017_SENSOR_PIN QOSA_PIN_31
#endif

#ifndef KY017_OUTPUT_PIN
#define KY017_OUTPUT_PIN QOSA_PIN_30
#endif

#ifndef KY017_TRIGGER_LEVEL
#define KY017_TRIGGER_LEVEL QOSA_GPIO_LEVEL_HIGH
#endif

#ifndef KY017_SENSOR_PULL
#define KY017_SENSOR_PULL QOSA_GPIO_PULL_UP
#endif

#define KY017_OUTPUT_ACTIVE_LEVEL QOSA_GPIO_LEVEL_HIGH
#define KY017_POLL_INTERVAL_MS 1000U
#define KY017_TASK_STACK_SIZE 2048U
#define KY017_TASK_PRIORITY QOSA_PRIORITY_NORMAL

static qosa_task_t g_ky017_task = QOSA_NULL;
static qosa_pin_cfg_t g_ky017_sensor_pin_cfg;
static qosa_pin_cfg_t g_ky017_output_pin_cfg;

/* 根据输出有效电平计算无效电平。 */
static qosa_gpio_level_e ky017_get_inactive_level(qosa_gpio_level_e active_level)
{
	return (active_level == QOSA_GPIO_LEVEL_HIGH) ? QOSA_GPIO_LEVEL_LOW : QOSA_GPIO_LEVEL_HIGH;
}

/* 判断当前传感器电平是否达到触发条件。 */
static qosa_bool_t ky017_is_triggered(qosa_gpio_level_e level)
{
	return (level == KY017_TRIGGER_LEVEL) ? QOSA_TRUE : QOSA_FALSE;
}

/* 初始化 KY-017 示例使用的输入或输出 GPIO。 */
static int ky017_gpio_init(qosa_pin_num_e pin_num,
			   qosa_gpio_direction_e direction,
			   qosa_gpio_pull_e pull,
			   qosa_gpio_level_e init_level,
			   qosa_pin_cfg_t *pin_cfg)
{
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pinctrl_ret;

	gpio_ret = qosa_get_pin_default_cfg(pin_num, pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("get pin %d default cfg failed, ret=%d", pin_num, gpio_ret);
		return -1;
	}

	pinctrl_ret = qosa_pin_set_func(pin_cfg->pin_num, pin_cfg->gpio_func);
	if (pinctrl_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("set pin %d gpio func failed, ret=%d", pin_cfg->pin_num, pinctrl_ret);
		return -1;
	}

	gpio_ret = qosa_gpio_init(pin_cfg->gpio_num, direction, pull, init_level);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("init gpio %d failed, ret=%d", pin_cfg->gpio_num, gpio_ret);
		return -1;
	}

	return 0;
}

/* 根据触发状态控制联动输出。 */
static void ky017_set_output(qosa_bool_t active)
{
	qosa_gpio_level_e level = ky017_get_inactive_level(KY017_OUTPUT_ACTIVE_LEVEL);

	if (active == QOSA_TRUE)
	{
		level = KY017_OUTPUT_ACTIVE_LEVEL;
	}

	(void)qosa_gpio_set_level(g_ky017_output_pin_cfg.gpio_num, level);
}

/* 水银开关轮询任务，周期读取倾斜状态并控制输出。 */
static void ky017_demo_task(void *argv)
{
	qosa_gpio_level_e sensor_level = QOSA_GPIO_LEVEL_LOW;
	qosa_bool_t triggered;

	(void)argv;

	QLOGI("ky017 demo started: sensor pin=%d gpio=%d, output pin=%d gpio=%d",
		  g_ky017_sensor_pin_cfg.pin_num,
		  g_ky017_sensor_pin_cfg.gpio_num,
		  g_ky017_output_pin_cfg.pin_num,
		  g_ky017_output_pin_cfg.gpio_num);

	while (1)
	{
		if (qosa_gpio_get_level(g_ky017_sensor_pin_cfg.gpio_num, &sensor_level) != QOSA_GPIO_SUCCESS)
		{
			QLOGW("read sensor level failed, gpio=%d", g_ky017_sensor_pin_cfg.gpio_num);
			qosa_task_sleep_ms(KY017_POLL_INTERVAL_MS);
			continue;
		}

		triggered = ky017_is_triggered(sensor_level);
		ky017_set_output(triggered);

		if (triggered == QOSA_TRUE)
		{
			QLOGI("水银开关检测到倾斜");
		}
		else
		{
			QLOGI("水银开关未检测到倾斜");
		}

		qosa_task_sleep_ms(KY017_POLL_INTERVAL_MS);
	}
}

/* KY-017 示例初始化入口，配置 GPIO 并创建任务。 */
static void ky017_demo_init(void)
{
	int ret;
	qosa_gpio_level_e output_inactive_level = ky017_get_inactive_level(KY017_OUTPUT_ACTIVE_LEVEL);

	if (g_ky017_task != QOSA_NULL)
	{
		QLOGW("ky017 demo already started");
		return;
	}

	if (ky017_gpio_init(KY017_SENSOR_PIN,
			   QOSA_GPIO_DIRECTION_INPUT,
			   KY017_SENSOR_PULL,
			   QOSA_GPIO_LEVEL_LOW,
			   &g_ky017_sensor_pin_cfg) != 0)
	{
		return;
	}

	if (ky017_gpio_init(KY017_OUTPUT_PIN,
			   QOSA_GPIO_DIRECTION_OUTPUT,
			   QOSA_GPIO_PULL_NONE,
			   output_inactive_level,
			   &g_ky017_output_pin_cfg) != 0)
	{
		(void)qosa_gpio_deinit(g_ky017_sensor_pin_cfg.gpio_num);
		return;
	}

	ret = qosa_task_create(&g_ky017_task,
			       KY017_TASK_STACK_SIZE,
			       KY017_TASK_PRIORITY,
			       "ky017",
			       ky017_demo_task,
			       QOSA_NULL);
	if (ret != QOSA_OK)
	{
		QLOGE("create ky017 task failed: %d", ret);
		(void)qosa_gpio_deinit(g_ky017_sensor_pin_cfg.gpio_num);
		(void)qosa_gpio_deinit(g_ky017_output_pin_cfg.gpio_num);
		return;
	}

	QLOGI("ky017 demo init ok, sensor pin=%d output pin=%d trigger=%d pull=%d",
		  g_ky017_sensor_pin_cfg.pin_num,
		  g_ky017_output_pin_cfg.pin_num,
		  KY017_TRIGGER_LEVEL,
		  KY017_SENSOR_PULL);
}

/* 将 KY-017 水银开关示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(200, "ky017_demo", ky017_demo_init);
