/*
@file      : beep_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on Buzzer Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
*/
#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_sys.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"

#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* 本示例通过 GPIO 输出驱动蜂鸣器；下面的宏可在编译时按不同接线和蜂鸣节奏重定义。 */
#ifndef BEEP_DEMO_PIN_NUM
/* 默认使用 19 号 PIN 作为蜂鸣器控制引脚。 */
#define BEEP_DEMO_PIN_NUM QOSA_PIN_19
#endif

#ifndef BEEP_DEMO_ACTIVE_LEVEL
/* 蜂鸣器有效电平，默认高电平有效；若硬件为低电平有效可重定义。 */
#define BEEP_DEMO_ACTIVE_LEVEL QOSA_GPIO_LEVEL_HIGH
#endif

#ifndef BEEP_DEMO_INITIAL_DELAY_SEC
/* 系统启动后等待一段时间，再开始第一次蜂鸣。 */
#define BEEP_DEMO_INITIAL_DELAY_SEC 10U
#endif

#ifndef BEEP_DEMO_PERIOD_SEC
/* 两次完整蜂鸣周期之间的间隔。 */
#define BEEP_DEMO_PERIOD_SEC 10U
#endif

#ifndef BEEP_DEMO_BEEP_ON_MS
/* 每个蜂鸣脉冲的持续时间。 */
#define BEEP_DEMO_BEEP_ON_MS 180U
#endif

#ifndef BEEP_DEMO_BEEP_OFF_MS
/* 同一周期内连续蜂鸣之间的短暂停顿。 */
#define BEEP_DEMO_BEEP_OFF_MS 120U
#endif

#ifndef BEEP_DEMO_RING_REPEAT_COUNT
/* 一个报警周期内连续蜂鸣的次数。 */
#define BEEP_DEMO_RING_REPEAT_COUNT 3U
#endif

/* 后台循环任务使用的任务栈大小和任务名称。 */
#define BEEP_DEMO_TASK_STACK_SIZE 2048U
#define BEEP_DEMO_TASK_NAME "beep_alarm"

/*
 * 模块级静态状态：
 * g_beep_alarm_task 用于判断任务是否已经创建。
 * g_beep_pin_cfg 保存蜂鸣器解析后的引脚配置。
 * g_beep_gpio_ready 防止重复初始化 GPIO。
 */
static qosa_task_t g_beep_alarm_task = QOSA_NULL;
static qosa_pin_cfg_t g_beep_pin_cfg;
static int g_beep_gpio_ready;

/* 根据蜂鸣器有效电平推导无效电平，例如高电平有效时，静音需要低电平。 */
static qosa_gpio_level_e beep_demo_get_inactive_level(void)
{
	return (BEEP_DEMO_ACTIVE_LEVEL == QOSA_GPIO_LEVEL_HIGH) ? QOSA_GPIO_LEVEL_LOW : QOSA_GPIO_LEVEL_HIGH;
}

/* 封装一次 GPIO 电平切换，集中处理错误和日志。 */
static int beep_demo_set_level(qosa_gpio_level_e level)
{
	qosa_gpio_error_e ret = qosa_gpio_set_level(g_beep_pin_cfg.gpio_num, level);

	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("beep gpio set level failed, gpio=%u ret=%d", (unsigned int)g_beep_pin_cfg.gpio_num, (int)ret);
		return -1;
	}

	return QOSA_ERROR_OK;
}

/* 完成蜂鸣器 GPIO 的准备流程：
 * 1. 读取默认引脚配置。
 * 2. 将引脚复用切换到 GPIO 模式。
 * 3. 初始化为输出，并先保持无效电平。
 */
static int beep_demo_prepare_gpio(void)
{
	qosa_gpio_level_e inactive_level = beep_demo_get_inactive_level();
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pin_ret;

	/* 如果已经初始化完成，直接返回，避免重复配置硬件。 */
	if (g_beep_gpio_ready)
	{
		return QOSA_ERROR_OK;
	}

	/* 根据配置的 PIN 编号查询平台默认 GPIO 和复用配置。 */
	gpio_ret = qosa_get_pin_default_cfg((qosa_uint8_t)BEEP_DEMO_PIN_NUM, &g_beep_pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("beep pin default cfg failed, pin=%u ret=%d", (unsigned int)BEEP_DEMO_PIN_NUM, (int)gpio_ret);
		return -1;
	}

	/* 将物理引脚切换到 GPIO 功能，否则后续电平设置不会生效。 */
	pin_ret = qosa_pin_set_func((qosa_pin_num_e)g_beep_pin_cfg.pin_num, g_beep_pin_cfg.gpio_func);
	if (pin_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("beep pin func config failed, pin=%u func=%u ret=%d", (unsigned int)g_beep_pin_cfg.pin_num, (unsigned int)g_beep_pin_cfg.gpio_func, (int)pin_ret);
		return -1;
	}

	/* 初始化为输出并设为无效电平，避免上电时误响。 */
	gpio_ret = qosa_gpio_init(g_beep_pin_cfg.gpio_num, QOSA_GPIO_DIRECTION_OUTPUT, g_beep_pin_cfg.gpio_pull, inactive_level);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("beep gpio init failed, gpio=%u ret=%d", (unsigned int)g_beep_pin_cfg.gpio_num, (int)gpio_ret);
		return -1;
	}

	/* 标记 GPIO 已就绪，后续任务循环可直接使用。 */
	g_beep_gpio_ready = 1;
	QLOGI("beep gpio ready: pin=%u gpio=%u active=%u", (unsigned int)g_beep_pin_cfg.pin_num, (unsigned int)g_beep_pin_cfg.gpio_num, (unsigned int)BEEP_DEMO_ACTIVE_LEVEL);
	return QOSA_ERROR_OK;
}

/* 执行一次报警蜂鸣序列，按配置次数和时长连续鸣叫。 */
static void beep_demo_ring_alarm(void)
{
	qosa_uint32_t index;

	for (index = 0; index < BEEP_DEMO_RING_REPEAT_COUNT; ++index)
	{
		/* 输出有效电平，使蜂鸣器开始发声。 */
		if (beep_demo_set_level(BEEP_DEMO_ACTIVE_LEVEL) != QOSA_ERROR_OK)
		{
			/* 电平切换失败时立即退出本轮蜂鸣。 */
			return;
		}

		/* 保持一段时间，形成可听见的蜂鸣脉冲。 */
		qosa_task_sleep_ms(BEEP_DEMO_BEEP_ON_MS);

		/* 回到无效电平，使蜂鸣器停止发声。 */
		if (beep_demo_set_level(beep_demo_get_inactive_level()) != QOSA_ERROR_OK)
		{
			/* 如果切回静音失败，也停止本轮序列。 */
			return;
		}

		/* 非最后一次蜂鸣时插入短暂停顿。 */
		if ((index + 1U) < BEEP_DEMO_RING_REPEAT_COUNT)
		{
			qosa_task_sleep_ms(BEEP_DEMO_BEEP_OFF_MS);
		}
	}
}

/* 后台任务入口：初始化 GPIO，等待初始延时，然后按固定周期蜂鸣。 */
static void beep_demo_alarm_task(void *argv)
{
	qosa_uint32_t cycle = 0;

	/* 本示例不使用任务参数，显式丢弃以避免告警。 */
	(void)argv;

	/* 先确保 GPIO 就绪，初始化失败则退出任务。 */
	if (beep_demo_prepare_gpio() != QOSA_ERROR_OK)
	{
		return;
	}

	/* 打印时序参数，便于从日志确认蜂鸣模式。 */
	QLOGI("beep alarm armed: initial_delay=%u sec, period=%u sec, repeat=%u", (unsigned int)BEEP_DEMO_INITIAL_DELAY_SEC, (unsigned int)BEEP_DEMO_PERIOD_SEC, (unsigned int)BEEP_DEMO_RING_REPEAT_COUNT);

	/* 第一次报警前先等待，避免系统刚启动就立即发声。 */
	qosa_task_sleep_sec(BEEP_DEMO_INITIAL_DELAY_SEC);

	/* 进入长期运行的报警循环。 */
	while (1)
	{
		/* 统计当前周期，便于运行时诊断。 */
		++cycle;
		QLOGI("beep alarm triggered, cycle=%u", (unsigned int)cycle);

		/* 执行一轮完整蜂鸣序列。 */
		beep_demo_ring_alarm();

		/* 等待下一个周期再继续蜂鸣。 */
		qosa_task_sleep_sec(BEEP_DEMO_PERIOD_SEC);
	}
}

/* 模块启动入口：确保只创建一次后台任务，避免重复启动冲突。 */
static void beep_demo_bootstrap(void)
{
	int ret;

	/* 任务句柄非空表示示例已经启动，忽略重复调用。 */
	if (g_beep_alarm_task != QOSA_NULL)
	{
		QLOGW("beep demo already started");
		return;
	}

	/* 创建后台任务，实际蜂鸣逻辑在任务中运行。 */
	ret = qosa_task_create(&g_beep_alarm_task, BEEP_DEMO_TASK_STACK_SIZE, QOSA_PRIORITY_NORMAL, BEEP_DEMO_TASK_NAME, beep_demo_alarm_task, QOSA_NULL);
	if (ret != QOSA_ERROR_OK)
	{
		/* 创建失败时重置句柄，允许后续重试。 */
		QLOGE("create beep alarm task failed, ret=%d", ret);
		g_beep_alarm_task = QOSA_NULL;
		return;
	}

	/* 任务创建成功后打印初始化完成日志。 */
	QLOGI("beep demo init finished, pin=%u", (unsigned int)BEEP_DEMO_PIN_NUM);
}

/* 将蜂鸣器示例注册到系统启动流程，启动时按优先级调用。 */
UNIRTOS_APP_EXPORT(300, "beep_demo", beep_demo_bootstrap);
