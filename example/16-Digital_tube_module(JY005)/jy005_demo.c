/*
@file      : jy005_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on JY005 Digital Tube Example
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

/* JY005 一位数码管示例参数：段数、任务配置和显示间隔。 */
#define JY005_SEGMENT_COUNT 8U
#define JY005_TASK_STACK_SIZE 2048U
#define JY005_TASK_PRIORITY QOSA_PRIORITY_NORMAL
#define JY005_DISPLAY_INTERVAL_MS 1000U

#ifndef JY005_SEG_A_PIN
#define JY005_SEG_A_PIN QOSA_PIN_32
#endif

#ifndef JY005_SEG_B_PIN
#define JY005_SEG_B_PIN QOSA_PIN_31
#endif

#ifndef JY005_SEG_C_PIN
#define JY005_SEG_C_PIN QOSA_PIN_30
#endif

#ifndef JY005_SEG_D_PIN
#define JY005_SEG_D_PIN QOSA_PIN_33
#endif

#ifndef JY005_SEG_E_PIN
#define JY005_SEG_E_PIN QOSA_PIN_80
#endif

#ifndef JY005_SEG_F_PIN
#define JY005_SEG_F_PIN QOSA_PIN_58
#endif

#ifndef JY005_SEG_G_PIN
#define JY005_SEG_G_PIN QOSA_PIN_29
#endif

#ifndef JY005_SEG_DP_PIN
#define JY005_SEG_DP_PIN QOSA_PIN_28
#endif

typedef struct
{
	/* 数码管段对应的 PIN 编号。 */
	qosa_pin_num_e pin_num;
	/* PIN 映射后的 GPIO 编号。 */
	qosa_gpio_num_e gpio_num;
} jy005_segment_gpio_t;

/* A、B、C、D、E、F、G、DP 八个段对应的默认引脚。 */
static const qosa_pin_num_e g_jy005_segment_pins[JY005_SEGMENT_COUNT] = {
	JY005_SEG_A_PIN,
	JY005_SEG_B_PIN,
	JY005_SEG_C_PIN,
	JY005_SEG_D_PIN,
	JY005_SEG_E_PIN,
	JY005_SEG_F_PIN,
	JY005_SEG_G_PIN,
	JY005_SEG_DP_PIN,
};

/* 数码管 0-9 字形表，0 表示点亮该段，1 表示熄灭该段。 */
static const qosa_uint8_t g_jy005_num_table[10][JY005_SEGMENT_COUNT] = {
	{0, 0, 0, 0, 1, 0, 0, 0},
	{0, 1, 0, 1, 1, 0, 1, 1},
	{1, 0, 0, 0, 0, 0, 0, 1},
	{0, 0, 0, 0, 0, 0, 1, 1},
	{0, 1, 0, 1, 0, 0, 1, 0},
	{0, 0, 1, 0, 0, 0, 1, 0},
	{0, 0, 1, 0, 0, 0, 0, 0},
	{0, 0, 0, 1, 1, 0, 1, 1},
	{0, 0, 0, 0, 0, 0, 0, 0},
	{0, 0, 0, 0, 0, 0, 1, 0},
};

static jy005_segment_gpio_t g_jy005_segments[JY005_SEGMENT_COUNT];
static qosa_task_t g_jy005_task = NULL;

/* 初始化单个数码管段对应的 GPIO。 */
static int jy005_segment_gpio_init(qosa_pin_num_e pin_num, jy005_segment_gpio_t *segment)
{
	qosa_pin_cfg_t pin_cfg;
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pinctrl_ret;

	gpio_ret = qosa_get_pin_default_cfg(pin_num, &pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("get pin %d default cfg failed, ret=%d", pin_num, gpio_ret);
		return -1;
	}

	pinctrl_ret = qosa_pin_set_func(pin_cfg.pin_num, pin_cfg.gpio_func);
	if (pinctrl_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("set pin %d gpio func failed, ret=%d", pin_cfg.pin_num, pinctrl_ret);
		return -1;
	}

	gpio_ret = qosa_gpio_init(pin_cfg.gpio_num,
				 QOSA_GPIO_DIRECTION_OUTPUT,
				 QOSA_GPIO_PULL_NONE,
				 QOSA_GPIO_LEVEL_HIGH);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("init gpio %d failed, ret=%d", pin_cfg.gpio_num, gpio_ret);
		return -1;
	}

	segment->pin_num = pin_cfg.pin_num;
	segment->gpio_num = pin_cfg.gpio_num;
	return 0;
}

/* 初始化数码管所有段 GPIO。 */
static int jy005_gpio_init(void)
{
	qosa_uint8_t segment_index;

	for (segment_index = 0; segment_index < JY005_SEGMENT_COUNT; ++segment_index)
	{
		if (jy005_segment_gpio_init(g_jy005_segment_pins[segment_index], &g_jy005_segments[segment_index]) != 0)
		{
			return -1;
		}
	}

	return 0;
}

/* 清空显示，关闭所有段。 */
static void jy005_clear_display(void)
{
	qosa_uint8_t segment_index;

	for (segment_index = 0; segment_index < JY005_SEGMENT_COUNT; ++segment_index)
	{
		(void)qosa_gpio_set_level(g_jy005_segments[segment_index].gpio_num, QOSA_GPIO_LEVEL_HIGH);
	}
}

/* 根据字形表显示一个 0-9 的数字。 */
static void jy005_display_num(int number)
{
	qosa_uint8_t segment_index;

	if ((number < 0) || (number > 9))
	{
		return;
	}

	for (segment_index = 0; segment_index < JY005_SEGMENT_COUNT; ++segment_index)
	{
		qosa_gpio_level_e level = (g_jy005_num_table[number][segment_index] == 0U) ? QOSA_GPIO_LEVEL_LOW : QOSA_GPIO_LEVEL_HIGH;

		(void)qosa_gpio_set_level(g_jy005_segments[segment_index].gpio_num, level);
	}
}

/* 数码管演示任务，循环显示 0 到 9。 */
static void jy005_demo_task(void *argv)
{
	(void)argv;

	if (jy005_gpio_init() != 0)
	{
		qosa_task_t current_task = NULL;

		QLOGE("jy005 gpio init failed");
		g_jy005_task = NULL;
		(void)qosa_task_get_current_ref(&current_task);
		if (current_task != NULL)
		{
			qosa_task_delete(current_task);
		}
		return;
	}

	jy005_clear_display();
	QLOGI("jy005 demo start: a=%d b=%d c=%d d=%d e=%d f=%d g=%d dp=%d",
		  JY005_SEG_A_PIN,
		  JY005_SEG_B_PIN,
		  JY005_SEG_C_PIN,
		  JY005_SEG_D_PIN,
		  JY005_SEG_E_PIN,
		  JY005_SEG_F_PIN,
		  JY005_SEG_G_PIN,
		  JY005_SEG_DP_PIN);

	while (1)
	{
		int number;

		for (number = 0; number < 10; ++number)
		{
			jy005_display_num(number);
			qosa_task_sleep_ms(JY005_DISPLAY_INTERVAL_MS);
		}
	}
}

/* JY005 数码管示例初始化入口，负责创建显示任务。 */
static void jy005_demo_init(void)
{
	int ret;

	if (g_jy005_task != NULL)
	{
		QLOGW("jy005 demo already started");
		return;
	}

	ret = qosa_task_create(&g_jy005_task,
			       JY005_TASK_STACK_SIZE,
			       JY005_TASK_PRIORITY,
			       "jy005",
			       jy005_demo_task,
			       NULL);
	if (ret != QOSA_OK)
	{
		QLOGE("create jy005 task failed: %d", ret);
		g_jy005_task = NULL;
		return;
	}

	QLOGI("jy005 demo task created");
}

/* 将 JY005 数码管示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(200, "jy005_demo", jy005_demo_init);
