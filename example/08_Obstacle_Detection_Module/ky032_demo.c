/*
@file      : ky032_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on KY-032 Obstacle Detection Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
*/
#include "qcm_proj_config.h"
#include "qosa_gpio.h"
#include "qosa_log.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* KY-032 OUT 接 PIN23，低电平表示检测到障碍物。 */
#define KY032_OUT_PIN             QOSA_PIN_23
#define KY032_OBSTACLE_LEVEL      QOSA_GPIO_LEVEL_LOW
#define KY032_POLL_INTERVAL_MS    200
#define KY032_TASK_STACK_SIZE     2048
#define KY032_TASK_PRIORITY       QOSA_PRIORITY_NORMAL

#ifndef KY032_USE_INTERRUPT_MODE
#define KY032_USE_INTERRUPT_MODE  0
#endif

static qosa_pin_cfg_t g_ky032_out_pin_cfg;
static volatile qosa_uint8_t g_ky032_obstacle_flag = 0;
static qosa_task_t g_ky032_task = QOSA_NULL;

/* 初始化 KY-032 OUT 对应的输入 GPIO。 */
static int ky032_gpio_init(void)
{
	qosa_pin_cfg_t pin_cfg = {0};
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pin_ret;

	gpio_ret = qosa_get_pin_default_cfg((qosa_uint8_t)KY032_OUT_PIN, &pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-032 get pin cfg failed, pin=%u, ret=%d", (unsigned int)KY032_OUT_PIN, gpio_ret);
		return -1;
	}

	pin_ret = qosa_pin_set_func((qosa_pin_num_e)pin_cfg.pin_num, pin_cfg.gpio_func);
	if (pin_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("KY-032 set pin func failed, pin=%u, func=%u, ret=%d", pin_cfg.pin_num, pin_cfg.gpio_func, pin_ret);
		return -1;
	}

	gpio_ret = qosa_gpio_init(pin_cfg.gpio_num, QOSA_GPIO_DIRECTION_INPUT, QOSA_GPIO_PULL_UP, QOSA_GPIO_LEVEL_LOW);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-032 gpio init failed, gpio=%d, ret=%d", pin_cfg.gpio_num, gpio_ret);
		return -1;
	}

	g_ky032_out_pin_cfg = pin_cfg;
	g_ky032_obstacle_flag = 0;
	QLOGI("KY-032 init ok, pin=%u, gpio=%d", g_ky032_out_pin_cfg.pin_num, g_ky032_out_pin_cfg.gpio_num);
	return 0;
}

/* 读取 KY-032 当前输出电平。 */
static qosa_gpio_level_e ky032_read_state(void)
{
	qosa_gpio_level_e level = QOSA_GPIO_LEVEL_HIGH;
	qosa_gpio_error_e ret;

	ret = qosa_gpio_get_level(g_ky032_out_pin_cfg.gpio_num, &level);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-032 read gpio failed, gpio=%d, ret=%d", g_ky032_out_pin_cfg.gpio_num, ret);
		return QOSA_GPIO_LEVEL_HIGH;
	}

	return level;
}

/* 判断当前电平是否表示检测到障碍物。 */
static qosa_uint8_t ky032_is_obstacle(void)
{
	return ky032_read_state() == KY032_OBSTACLE_LEVEL;
}

/* 轮询模式监控任务，周期性读取传感器状态。 */
static void ky032_monitor_polling(void *argv)
{
	(void)argv;

	if (ky032_gpio_init() != 0)
	{
		QLOGE("KY-032 polling mode start failed");
		qosa_task_delete(g_ky032_task);
		return;
	}

	QLOGI("KY-032 polling mode started");
	while (1)
	{
		if (ky032_is_obstacle())
		{
			QLOGI("KY-032 obstacle detected");
		}
		else
		{
			QLOGI("KY-032 no obstacle");
		}

		qosa_task_sleep_ms(KY032_POLL_INTERVAL_MS);
	}
}

#if KY032_USE_INTERRUPT_MODE
/* 中断回调：检测到低电平时置位障碍标志。 */
static void ky032_irq_handler(void *argv)
{
	(void)argv;

	if (ky032_is_obstacle())
	{
		g_ky032_obstacle_flag = 1;
	}
}

/* 初始化 KY-032 中断检测模式。 */
static int ky032_interrupt_init(void)
{
	qosa_int_cfg_t int_cfg = {0};
	qosa_gpio_error_e ret;

	int_cfg.gpio_num = g_ky032_out_pin_cfg.gpio_num;
	int_cfg.gpio_debounce = QOSA_GPIO_DEBOUNCE_EN;
	int_cfg.gpio_pull = QOSA_GPIO_PULL_UP;
	int_cfg.interrupt_cb = ky032_irq_handler;
	int_cfg.options = 0;
	int_cfg.user_ctx = QOSA_NULL;

	ret = qosa_interrupt_register(&int_cfg);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-032 interrupt register failed, gpio=%d, ret=%d", g_ky032_out_pin_cfg.gpio_num, ret);
		return -1;
	}

	ret = qosa_interrupt_enable(g_ky032_out_pin_cfg.gpio_num, QOSA_GPIO_TRIGGER_FALLING_EDGE);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-032 interrupt enable failed, gpio=%d, ret=%d", g_ky032_out_pin_cfg.gpio_num, ret);
		return -1;
	}

	return 0;
}

/* 中断模式监控任务，消费中断标志并输出状态。 */
static void ky032_monitor_interrupt(void *argv)
{
	(void)argv;

	if (ky032_gpio_init() != 0 || ky032_interrupt_init() != 0)
	{
		QLOGE("KY-032 interrupt mode start failed");
		qosa_task_delete(g_ky032_task);
		return;
	}

	QLOGI("KY-032 interrupt mode started");
	while (1)
	{
		if (g_ky032_obstacle_flag)
		{
			QLOGI("KY-032 obstacle detected");
			g_ky032_obstacle_flag = 0;
		}
		else
		{
			QLOGI("KY-032 no obstacle");
		}

		qosa_task_sleep_ms(KY032_POLL_INTERVAL_MS);
	}
}
#endif

static void ky032_demo_init(void)
{
	int ret;

	/* 根据编译开关选择轮询模式或中断模式。 */
#if KY032_USE_INTERRUPT_MODE
	ret = qosa_task_create(&g_ky032_task, KY032_TASK_STACK_SIZE, KY032_TASK_PRIORITY, "ky032_int", ky032_monitor_interrupt, QOSA_NULL);
#else
	ret = qosa_task_create(&g_ky032_task, KY032_TASK_STACK_SIZE, KY032_TASK_PRIORITY, "ky032_poll", ky032_monitor_polling, QOSA_NULL);
#endif
	if (ret != QOSA_ERROR_OK)
	{
		QLOGE("KY-032 task create failed, ret=%d", ret);
	}
}

/* 将 KY-032 避障示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(200, "ky032_demo", ky032_demo_init);
