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
#include "qos_applications/app_init/unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* KY-032 避障模块默认引脚、轮询周期和任务参数。 */
#define KY032_PIN_NUM             23
#define KY032_POLL_INTERVAL_MS    200
#define KY032_TASK_STACK_SIZE     2048
#define KY032_TASK_PRIORITY       QOSA_PRIORITY_NORMAL

#ifndef KY032_USE_INTERRUPT_MODE
#define KY032_USE_INTERRUPT_MODE  0
#endif

typedef struct
{
	/* 模块连接的 PIN 编号。 */
	qosa_uint8_t     pin_num;
	/* PIN 映射后的 GPIO 编号。 */
	qosa_gpio_num_e  gpio_num;
	/* 中断模式下记录是否检测到障碍。 */
	volatile qosa_uint8_t obstacle_flag;
} ky032_sensor_t;

static ky032_sensor_t g_ky032_sensor = {
	.pin_num = KY032_PIN_NUM,
	.gpio_num = QOSA_GPIO_31,
	.obstacle_flag = 0,
};

static qosa_task_t g_ky032_task = QOSA_NULL;

/* 初始化 KY-032 输入 GPIO。 */
static int ky032_sensor_init(ky032_sensor_t *sensor)
{
	qosa_pin_cfg_t pin_cfg = {0};
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pin_ret;

	gpio_ret = qosa_get_pin_default_cfg(sensor->pin_num, &pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-032 get pin cfg failed, pin=%u, ret=%d", sensor->pin_num, gpio_ret);
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

	sensor->gpio_num = pin_cfg.gpio_num;
	sensor->obstacle_flag = 0;
	QLOGI("KY-032 init ok, pin=%u, gpio=%d", sensor->pin_num, sensor->gpio_num);
	return 0;
}

/* 读取 KY-032 当前输出电平。 */
static qosa_gpio_level_e ky032_read_state(const ky032_sensor_t *sensor)
{
	qosa_gpio_level_e level = QOSA_GPIO_LEVEL_HIGH;
	qosa_gpio_error_e ret;

	ret = qosa_gpio_get_level(sensor->gpio_num, &level);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-032 read gpio failed, gpio=%d, ret=%d", sensor->gpio_num, ret);
		return QOSA_GPIO_LEVEL_HIGH;
	}

	return level;
}

/* 判断当前电平是否表示检测到障碍物。 */
static qosa_uint8_t ky032_is_obstacle(const ky032_sensor_t *sensor)
{
	return ky032_read_state(sensor) == QOSA_GPIO_LEVEL_LOW;
}

/* 轮询模式监控任务，周期性读取传感器状态。 */
static void ky032_monitor_polling(void *argv)
{
	ky032_sensor_t *sensor = (ky032_sensor_t *)argv;

	if (ky032_sensor_init(sensor) != 0)
	{
		QLOGE("KY-032 polling mode start failed");
		qosa_task_delete(g_ky032_task);
		return;
	}

	QLOGI("KY-032 polling mode started");
	while (1)
	{
		if (ky032_is_obstacle(sensor))
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
	ky032_sensor_t *sensor = (ky032_sensor_t *)argv;

	if (ky032_is_obstacle(sensor))
	{
		sensor->obstacle_flag = 1;
	}
}

/* 初始化 KY-032 中断检测模式。 */
static int ky032_interrupt_init(ky032_sensor_t *sensor)
{
	qosa_int_cfg_t int_cfg = {0};
	qosa_gpio_error_e ret;

	int_cfg.gpio_num = sensor->gpio_num;
	int_cfg.gpio_debounce = QOSA_GPIO_DEBOUNCE_EN;
	int_cfg.gpio_pull = QOSA_GPIO_PULL_UP;
	int_cfg.interrupt_cb = ky032_irq_handler;
	int_cfg.options = 0;
	int_cfg.user_ctx = sensor;

	ret = qosa_interrupt_register(&int_cfg);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-032 interrupt register failed, gpio=%d, ret=%d", sensor->gpio_num, ret);
		return -1;
	}

	ret = qosa_interrupt_enable(sensor->gpio_num, QOSA_GPIO_TRIGGER_FALLING_EDGE);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-032 interrupt enable failed, gpio=%d, ret=%d", sensor->gpio_num, ret);
		return -1;
	}

	return 0;
}

/* 中断模式监控任务，消费中断标志并输出状态。 */
static void ky032_monitor_interrupt(void *argv)
{
	ky032_sensor_t *sensor = (ky032_sensor_t *)argv;

	if (ky032_sensor_init(sensor) != 0 || ky032_interrupt_init(sensor) != 0)
	{
		QLOGE("KY-032 interrupt mode start failed");
		qosa_task_delete(g_ky032_task);
		return;
	}

	QLOGI("KY-032 interrupt mode started");
	while (1)
	{
		if (sensor->obstacle_flag)
		{
			QLOGI("KY-032 obstacle detected");
			sensor->obstacle_flag = 0;
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
	ret = qosa_task_create(&g_ky032_task, KY032_TASK_STACK_SIZE, KY032_TASK_PRIORITY, "ky032_int", ky032_monitor_interrupt, &g_ky032_sensor);
#else
	ret = qosa_task_create(&g_ky032_task, KY032_TASK_STACK_SIZE, KY032_TASK_PRIORITY, "ky032_poll", ky032_monitor_polling, &g_ky032_sensor);
#endif
	if (ret != QOSA_ERROR_OK)
	{
		QLOGE("KY-032 task create failed, ret=%d", ret);
	}
}

/* 将 KY-032 避障示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(200, "ky032_demo", ky032_demo_init);
