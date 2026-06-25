#include <stddef.h>

#include "qcm_proj_config.h"
#include "qosa_gpio.h"
#include "qosa_log.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

#define KY032_TASK_STACK_SIZE      2048
#define KY032_TASK_PRIORITY        10
#define KY032_INIT_ORDER           320
#define KY032_DEFAULT_ACTIVE_LEVEL 0
#define KY032_DEFAULT_USE_INTERRUPT 0

static qosa_task_t g_ky032_task = NULL;
static qosa_pin_cfg_t g_ky032_pin_cfg;
static qosa_uint8_t g_ky032_has_obstacle = 0;
static qosa_uint8_t g_ky032_obstacle_flag = 0;

static qosa_uint8_t ky032_level_to_state(qosa_gpio_level_e level);

static void ky032_gpio_irq_handler(void *argv)
{
	qosa_gpio_basic_isr_info_t *isr_info = (qosa_gpio_basic_isr_info_t *)argv;

	if ((isr_info != NULL) && (ky032_level_to_state(isr_info->trigger_level) != 0U))
	{
		g_ky032_obstacle_flag = 1U;
	}
	else if (isr_info == NULL)
	{
		g_ky032_obstacle_flag = 1U;
	}
	(void)argv;
}

static qosa_uint8_t ky032_get_active_level(void)
{
#ifdef CONFIG_QAPP_KY032_ACTIVE_LEVEL
	return (CONFIG_QAPP_KY032_ACTIVE_LEVEL != 0U) ? 1U : 0U;
#else
	return KY032_DEFAULT_ACTIVE_LEVEL;
#endif
}

static qosa_uint32_t ky032_get_sample_period_ms(void)
{
#ifdef CONFIG_QAPP_KY032_SAMPLE_PERIOD_MS
	return (qosa_uint32_t)CONFIG_QAPP_KY032_SAMPLE_PERIOD_MS;
#else
	return 100U;
#endif
}

static qosa_uint8_t ky032_get_use_interrupt(void)
{
#ifdef CONFIG_QAPP_KY032_USE_INTERRUPT
	return (CONFIG_QAPP_KY032_USE_INTERRUPT != 0U) ? 1U : 0U;
#else
	return KY032_DEFAULT_USE_INTERRUPT;
#endif
}

static qosa_gpio_pull_e ky032_get_input_pull(void)
{
	return QOSA_GPIO_PULL_UP;
}

static qosa_gpio_trigger_e ky032_get_interrupt_trigger(void)
{
	return (ky032_get_active_level() != 0U) ? QOSA_GPIO_TRIGGER_RISING_EDGE : QOSA_GPIO_TRIGGER_FALLING_EDGE;
}

static qosa_uint8_t ky032_level_to_state(qosa_gpio_level_e level)
{
	return ((qosa_uint8_t)level == ky032_get_active_level()) ? 1U : 0U;
}

static qosa_uint8_t ky032_read_state(void)
{
	qosa_gpio_level_e level = QOSA_GPIO_LEVEL_LOW;

	if (qosa_gpio_get_level(g_ky032_pin_cfg.gpio_num, &level) != QOSA_GPIO_SUCCESS)
	{
		QLOGE("ky032 read gpio level failed, gpio=%d", g_ky032_pin_cfg.gpio_num);
		return 0U;
	}

	return (qosa_uint8_t)level;
}

static qosa_uint8_t ky032_is_obstacle(void)
{
	return ky032_level_to_state((qosa_gpio_level_e)ky032_read_state());
}

static int ky032_prepare_input_pin(void)
{
	qosa_gpio_error_e ret;

	ret = qosa_get_pin_default_cfg((qosa_uint8_t)CONFIG_QAPP_KY032_PIN_NUM, &g_ky032_pin_cfg);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("ky032 get pin cfg failed, pin=%u, ret=%d", (unsigned int)CONFIG_QAPP_KY032_PIN_NUM, ret);
		return -1;
	}

	if (qosa_pin_set_func((qosa_pin_num_e)g_ky032_pin_cfg.pin_num, g_ky032_pin_cfg.gpio_func) != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("ky032 set pin func failed, pin=%u", (unsigned int)g_ky032_pin_cfg.pin_num);
		return -1;
	}

	ret = qosa_gpio_init(
		g_ky032_pin_cfg.gpio_num,
		QOSA_GPIO_DIRECTION_INPUT,
		ky032_get_input_pull(),
		QOSA_GPIO_LEVEL_LOW);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("ky032 gpio init failed, gpio=%d, ret=%d", g_ky032_pin_cfg.gpio_num, ret);
		return -1;
	}

	QLOGI(
		"ky032 ready, pin=%u gpio=%d active_level=%u sample=%ums mode=%s",
		(unsigned int)g_ky032_pin_cfg.pin_num,
		g_ky032_pin_cfg.gpio_num,
		(unsigned int)ky032_get_active_level(),
		(unsigned int)ky032_get_sample_period_ms(),
		(ky032_get_use_interrupt() != 0U) ? "interrupt" : "polling");
	return 0;
}

static void ky032_report_state(qosa_uint8_t has_obstacle)
{
	if (has_obstacle != 0U)
	{
		QLOGW("ky032 detect obstacle");
	}
	else
	{
		QLOGI("ky032 no obstacle");
	}
}

static int ky032_enable_interrupt_mode(void)
{
	qosa_int_cfg_t int_cfg;
	qosa_gpio_error_e ret;

	int_cfg.gpio_num = g_ky032_pin_cfg.gpio_num;
	int_cfg.gpio_debounce = QOSA_GPIO_DEBOUNCE_DIS;
	int_cfg.gpio_pull = ky032_get_input_pull();
	int_cfg.interrupt_cb = ky032_gpio_irq_handler;
	int_cfg.options = 1U;
	int_cfg.user_ctx = NULL;

	ret = qosa_interrupt_register(&int_cfg);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("ky032 irq register failed, gpio=%d, ret=%d", g_ky032_pin_cfg.gpio_num, ret);
		return -1;
	}

	ret = qosa_interrupt_enable(g_ky032_pin_cfg.gpio_num, ky032_get_interrupt_trigger());
	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("ky032 irq enable failed, gpio=%d, ret=%d", g_ky032_pin_cfg.gpio_num, ret);
		(void)qosa_interrupt_unregister(g_ky032_pin_cfg.gpio_num);
		return -1;
	}

	return 0;
}

static void ky032_monitor_polling(void)
{
	while (1)
	{
		g_ky032_has_obstacle = ky032_is_obstacle();
		ky032_report_state(g_ky032_has_obstacle);
		qosa_task_sleep_ms(ky032_get_sample_period_ms());
	}
}

static void ky032_monitor_interrupt(void)
{
	if (ky032_enable_interrupt_mode() != 0)
	{
		return;
	}

	while (1)
	{
		if (g_ky032_obstacle_flag != 0U)
		{
			g_ky032_obstacle_flag = 0U;
			g_ky032_has_obstacle = 1U;
			ky032_report_state(1U);
		}
		else
		{
			g_ky032_has_obstacle = ky032_is_obstacle();
			ky032_report_state(g_ky032_has_obstacle);
		}

		qosa_task_sleep_ms(ky032_get_sample_period_ms());
	}
}

static void ky032_demo_task(void *argv)
{
	(void)argv;

	if (ky032_prepare_input_pin() != 0)
	{
		return;
	}

	if (ky032_get_use_interrupt() != 0U)
	{
		QLOGI("ky032 interrupt mode start");
		ky032_monitor_interrupt();
		return;
	}

	QLOGI("ky032 polling mode start");
	ky032_monitor_polling();
}

static void ky032_demo_init(void)
{
	if (g_ky032_task != NULL)
	{
		QLOGW("ky032 demo already started");
		return;
	}

	if (qosa_task_create(
			&g_ky032_task,
			KY032_TASK_STACK_SIZE,
			KY032_TASK_PRIORITY,
			"ky032_demo",
			ky032_demo_task,
			NULL) != QOSA_ERROR_OK)
	{
		QLOGE("ky032 task create failed");
		g_ky032_task = NULL;
		return;
	}

	QLOGI("ky032 demo started");
}

UNIRTOS_APP_EXPORT(KY032_INIT_ORDER, "ky032_demo", ky032_demo_init);
