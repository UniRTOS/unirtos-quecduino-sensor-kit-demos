"""
@file      : beep_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on Buzzer Example
@version   : 0.1
@date      : 2026-06-01
@copyright : Copyright (c) 2026
"""
#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_sys.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"

#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/*
 * This demo drives a buzzer through a GPIO output.
 * The macros below are compile-time overrides for different wiring and beep timing.
 */
#ifndef BEEP_DEMO_PIN_NUM
/* Use pin 19 as the default buzzer control pin. */
#define BEEP_DEMO_PIN_NUM QOSA_PIN_19
#endif

#ifndef BEEP_DEMO_ACTIVE_LEVEL
/* Active level for the buzzer. High level is the default, but this can be overridden for active-low hardware. */
#define BEEP_DEMO_ACTIVE_LEVEL QOSA_GPIO_LEVEL_HIGH
#endif

#ifndef BEEP_DEMO_INITIAL_DELAY_SEC
/* Wait for a while after startup before the first beep cycle begins. */
#define BEEP_DEMO_INITIAL_DELAY_SEC 10U
#endif

#ifndef BEEP_DEMO_PERIOD_SEC
/* Interval between two complete beep cycles. */
#define BEEP_DEMO_PERIOD_SEC 10U
#endif

#ifndef BEEP_DEMO_BEEP_ON_MS
/* Duration of each beep pulse. */
#define BEEP_DEMO_BEEP_ON_MS 180U
#endif

#ifndef BEEP_DEMO_BEEP_OFF_MS
/* Short pause between consecutive beeps in the same cycle. */
#define BEEP_DEMO_BEEP_OFF_MS 120U
#endif

#ifndef BEEP_DEMO_RING_REPEAT_COUNT
/* Number of consecutive beeps in one alarm cycle. */
#define BEEP_DEMO_RING_REPEAT_COUNT 3U
#endif

/* Task stack size and task name used for the background loop task. */
#define BEEP_DEMO_TASK_STACK_SIZE 2048U
#define BEEP_DEMO_TASK_NAME "beep_alarm"

/*
 * Module-level static state:
 * g_beep_alarm_task tracks whether the task has already been created.
 * g_beep_pin_cfg stores the resolved pin configuration for the buzzer.
 * g_beep_gpio_ready prevents repeated GPIO initialization.
 */
static qosa_task_t g_beep_alarm_task = QOSA_NULL;
static qosa_pin_cfg_t g_beep_pin_cfg;
static int g_beep_gpio_ready;

/* Derive the inactive level from the buzzer's active level.
 * For example, if high level is active, silence requires low level, and vice versa.
 */
static qosa_gpio_level_e beep_demo_get_inactive_level(void)
{
	return (BEEP_DEMO_ACTIVE_LEVEL == QOSA_GPIO_LEVEL_HIGH) ? QOSA_GPIO_LEVEL_LOW : QOSA_GPIO_LEVEL_HIGH;
}

/* Wrap a single GPIO level change.
 * Centralizing error handling and logging avoids repeated return-value checks in the business logic.
 */
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

/* Complete all preparation steps for the buzzer GPIO:
 * 1. Read the default pin configuration.
 * 2. Switch the pin mux to GPIO mode.
 * 3. Initialize the GPIO as an output and keep it at the inactive level first.
 */
static int beep_demo_prepare_gpio(void)
{
	qosa_gpio_level_e inactive_level = beep_demo_get_inactive_level();
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pin_ret;

	/* Return early if initialization has already completed to avoid reconfiguring hardware. */
	if (g_beep_gpio_ready)
	{
		return QOSA_ERROR_OK;
	}

	/* Query the platform default GPIO and pin-mux settings from the configured pin number. */
	gpio_ret = qosa_get_pin_default_cfg((qosa_uint8_t)BEEP_DEMO_PIN_NUM, &g_beep_pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("beep pin default cfg failed, pin=%u ret=%d", (unsigned int)BEEP_DEMO_PIN_NUM, (int)gpio_ret);
		return -1;
	}

	/* Switch the physical pin to GPIO function, otherwise later level changes will not take effect. */
	pin_ret = qosa_pin_set_func((qosa_pin_num_e)g_beep_pin_cfg.pin_num, g_beep_pin_cfg.gpio_func);
	if (pin_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("beep pin func config failed, pin=%u func=%u ret=%d", (unsigned int)g_beep_pin_cfg.pin_num, (unsigned int)g_beep_pin_cfg.gpio_func, (int)pin_ret);
		return -1;
	}

	/* Initialize the GPIO as an output and set the initial value to the inactive level to avoid unwanted noise at power-up. */
	gpio_ret = qosa_gpio_init(g_beep_pin_cfg.gpio_num, QOSA_GPIO_DIRECTION_OUTPUT, g_beep_pin_cfg.gpio_pull, inactive_level);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("beep gpio init failed, gpio=%u ret=%d", (unsigned int)g_beep_pin_cfg.gpio_num, (int)gpio_ret);
		return -1;
	}

	/* Mark the GPIO as ready so the task loop can reuse it directly. */
	g_beep_gpio_ready = 1;
	QLOGI("beep gpio ready: pin=%u gpio=%u active=%u", (unsigned int)g_beep_pin_cfg.pin_num, (unsigned int)g_beep_pin_cfg.gpio_num, (unsigned int)BEEP_DEMO_ACTIVE_LEVEL);
	return QOSA_ERROR_OK;
}

/* Run one alarm beep sequence.
 * The logic is to beep BEEP_DEMO_RING_REPEAT_COUNT times in a row,
 * with the on/off durations controlled by the corresponding timing macros.
 */
static void beep_demo_ring_alarm(void)
{
	qosa_uint32_t index;

	for (index = 0; index < BEEP_DEMO_RING_REPEAT_COUNT; ++index)
	{
		/* Drive the active level so the buzzer starts sounding. */
		if (beep_demo_set_level(BEEP_DEMO_ACTIVE_LEVEL) != QOSA_ERROR_OK)
		{
			/* If a level change fails, exit this beep sequence immediately. */
			return;
		}

		/* Keep the buzzer on long enough to create an audible pulse. */
		qosa_task_sleep_ms(BEEP_DEMO_BEEP_ON_MS);

		/* Return to the inactive level so the buzzer stops sounding. */
		if (beep_demo_set_level(beep_demo_get_inactive_level()) != QOSA_ERROR_OK)
		{
			/* Likewise, if switching back to silence fails, stop this sequence. */
			return;
		}

		/* Insert a short pause only when this is not the last beep in the cycle. */
		if ((index + 1U) < BEEP_DEMO_RING_REPEAT_COUNT)
		{
			qosa_task_sleep_ms(BEEP_DEMO_BEEP_OFF_MS);
		}
	}
}

/* Background task entry point.
 * After startup, it initializes the GPIO, waits for the initial delay,
 * and then enters an infinite loop that beeps once every fixed period.
 */
static void beep_demo_alarm_task(void *argv)
{
	qosa_uint32_t cycle = 0;

	/* This demo does not use external arguments, so discard the parameter to avoid warnings. */
	(void)argv;

	/* Ensure the GPIO is ready first; if initialization fails, exit the task. */
	if (beep_demo_prepare_gpio() != QOSA_ERROR_OK)
	{
		return;
	}

	/* Log the timing parameters so the beep pattern can be verified from logs. */
	QLOGI("beep alarm armed: initial_delay=%u sec, period=%u sec, repeat=%u", (unsigned int)BEEP_DEMO_INITIAL_DELAY_SEC, (unsigned int)BEEP_DEMO_PERIOD_SEC, (unsigned int)BEEP_DEMO_RING_REPEAT_COUNT);

	/* Wait before the first alarm to avoid beeping immediately after system startup. */
	qosa_task_sleep_sec(BEEP_DEMO_INITIAL_DELAY_SEC);

	/* Enter the long-running alarm loop. */
	while (1)
	{
		/* Count the current cycle to help with runtime diagnostics. */
		++cycle;
		QLOGI("beep alarm triggered, cycle=%u", (unsigned int)cycle);

		/* Run one complete beep sequence. */
		beep_demo_ring_alarm();

		/* Wait until the next cycle before beeping again. */
		qosa_task_sleep_sec(BEEP_DEMO_PERIOD_SEC);
	}
}

/* Module bootstrap entry point.
 * Its job is to ensure the demo creates the background task only once and avoids duplicate startup conflicts.
 */
static void beep_demo_bootstrap(void)
{
	int ret;

	/* A non-null task handle means the demo has already been started, so ignore repeated calls. */
	if (g_beep_alarm_task != QOSA_NULL)
	{
		QLOGW("beep demo already started");
		return;
	}

	/* Create the background task; the actual beep logic runs inside that task. */
	ret = qosa_task_create(&g_beep_alarm_task, BEEP_DEMO_TASK_STACK_SIZE, QOSA_PRIORITY_NORMAL, BEEP_DEMO_TASK_NAME, beep_demo_alarm_task, QOSA_NULL);
	if (ret != QOSA_ERROR_OK)
	{
		/* If task creation fails, reset the handle so later retries are possible. */
		QLOGE("create beep alarm task failed, ret=%d", ret);
		g_beep_alarm_task = QOSA_NULL;
		return;
	}

	/* Log initialization completion after the task has been created successfully. */
	QLOGI("beep demo init finished, pin=%u", (unsigned int)BEEP_DEMO_PIN_NUM);
}

/* Register this demo into the system startup flow; it will be invoked by priority during boot. */
UNIRTOS_APP_EXPORT(300, "beep_demo", beep_demo_bootstrap);
