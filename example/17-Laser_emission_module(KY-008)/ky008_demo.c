/*
@file      : ky008_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on KY-008 Laser Emission Example
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

/* KY-008 激光发射模块默认引脚、电平和闪烁任务参数。 */
#define KY008_PIN_NUM             31
#define KY008_ACTIVE_LEVEL        QOSA_GPIO_LEVEL_HIGH
#define KY008_BLINK_INTERVAL_MS   2000
#define KY008_TASK_STACK_SIZE     2048
#define KY008_TASK_PRIORITY       QOSA_PRIORITY_NORMAL

typedef struct
{
	/* 激光模块控制 PIN 编号。 */
	qosa_uint8_t      pin_num;
	/* PIN 映射后的 GPIO 编号。 */
	qosa_gpio_num_e   gpio_num;
	/* 打开激光的有效电平。 */
	qosa_gpio_level_e active_level;
	/* 关闭激光的无效电平。 */
	qosa_gpio_level_e inactive_level;
} ky008_laser_t;

static ky008_laser_t g_ky008_laser = {
	.pin_num = KY008_PIN_NUM,
	.gpio_num = QOSA_GPIO_31,
	.active_level = KY008_ACTIVE_LEVEL,
	.inactive_level = QOSA_GPIO_LEVEL_LOW,
};

static qosa_task_t g_ky008_task = QOSA_NULL;

/* 初始化 KY-008 激光模块控制 GPIO。 */
static int ky008_laser_init(ky008_laser_t *laser)
{
	qosa_pin_cfg_t pin_cfg = {0};
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pin_ret;

	laser->inactive_level = (laser->active_level == QOSA_GPIO_LEVEL_HIGH) ? QOSA_GPIO_LEVEL_LOW : QOSA_GPIO_LEVEL_HIGH;

	gpio_ret = qosa_get_pin_default_cfg(laser->pin_num, &pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-008 get pin cfg failed, pin=%u, ret=%d", laser->pin_num, gpio_ret);
		return -1;
	}

	pin_ret = qosa_pin_set_func((qosa_pin_num_e)pin_cfg.pin_num, pin_cfg.gpio_func);
	if (pin_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("KY-008 set pin func failed, pin=%u, func=%u, ret=%d", pin_cfg.pin_num, pin_cfg.gpio_func, pin_ret);
		return -1;
	}

	gpio_ret = qosa_gpio_init(pin_cfg.gpio_num, QOSA_GPIO_DIRECTION_OUTPUT, QOSA_GPIO_PULL_NONE, laser->inactive_level);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-008 gpio init failed, gpio=%d, ret=%d", pin_cfg.gpio_num, gpio_ret);
		return -1;
	}

	laser->gpio_num = pin_cfg.gpio_num;
	QLOGI("KY-008 init ok, pin=%u, gpio=%d, active_level=%d", laser->pin_num, laser->gpio_num, laser->active_level);
	return 0;
}

/* 写入激光模块 GPIO 电平。 */
static void ky008_laser_write(const ky008_laser_t *laser, qosa_gpio_level_e level)
{
	qosa_gpio_error_e ret;

	ret = qosa_gpio_set_level(laser->gpio_num, level);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("KY-008 set level failed, gpio=%d, level=%d, ret=%d", laser->gpio_num, level, ret);
	}
}

/* 打开激光输出。 */
static void ky008_laser_on(const ky008_laser_t *laser)
{
	ky008_laser_write(laser, laser->active_level);
	QLOGI("KY-008 laser on");
}

/* 关闭激光输出。 */
static void ky008_laser_off(const ky008_laser_t *laser)
{
	ky008_laser_write(laser, laser->inactive_level);
	QLOGI("KY-008 laser off");
}

/* 按指定间隔闪烁一次激光。 */
static void ky008_laser_blink(const ky008_laser_t *laser, qosa_uint32_t interval_ms)
{
	ky008_laser_on(laser);
	qosa_task_sleep_ms(interval_ms);
	ky008_laser_off(laser);
	qosa_task_sleep_ms(interval_ms);
}

/* KY-008 后台任务，循环闪烁激光模块。 */
static void ky008_demo_task(void *argv)
{
	ky008_laser_t *laser = (ky008_laser_t *)argv;

	if (ky008_laser_init(laser) != 0)
	{
		QLOGE("KY-008 demo start failed");
		qosa_task_delete(g_ky008_task);
		return;
	}

	QLOGI("KY-008 demo started");
	while (1)
	{
		ky008_laser_blink(laser, KY008_BLINK_INTERVAL_MS);
	}
}

/* KY-008 激光模块示例初始化入口，负责创建任务。 */
static void ky008_demo_init(void)
{
	int ret;

	ret = qosa_task_create(&g_ky008_task, KY008_TASK_STACK_SIZE, KY008_TASK_PRIORITY, "ky008", ky008_demo_task, &g_ky008_laser);
	if (ret != QOSA_ERROR_OK)
	{
		QLOGE("KY-008 task create failed, ret=%d", ret);
	}
}

/* 将 KY-008 激光发射示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(200, "ky008_demo", ky008_demo_init);
