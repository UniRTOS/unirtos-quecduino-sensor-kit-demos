#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* KY008 的 S 引脚接到此物理 PIN，默认值可在 menuconfig 中修改。 */
#define KY008_SIGNAL_PIN ((qosa_uint8_t)CONFIG_QAPP_KY008_PIN_NUM)

#define KY008_ACTIVE_LEVEL   ((qosa_uint8_t)CONFIG_QAPP_KY008_ACTIVE_LEVEL)
#define KY008_BLINK_INTERVAL_MS ((qosa_uint32_t)CONFIG_QAPP_KY008_INTERVAL_MS)
#define KY008_TASK_STACK_SIZE 2048U
#define KY008_TASK_PRIORITY   QOSA_PRIORITY_NORMAL

static qosa_task_t g_ky008_task = QOSA_NULL;
static qosa_pin_cfg_t g_ky008_pin_cfg;
static qosa_gpio_level_e g_ky008_active_level = QOSA_GPIO_LEVEL_HIGH;
static qosa_gpio_level_e g_ky008_inactive_level = QOSA_GPIO_LEVEL_LOW;

static qosa_gpio_level_e ky008_to_level(qosa_uint8_t level)
{
	return (level != 0U) ? QOSA_GPIO_LEVEL_HIGH : QOSA_GPIO_LEVEL_LOW;
}

static qosa_gpio_error_e ky008_set_level(qosa_gpio_level_e level)
{
	qosa_gpio_error_e ret = qosa_gpio_set_level(g_ky008_pin_cfg.gpio_num, level);

	if (ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("ky008 set gpio %d level %d failed, ret=%d", g_ky008_pin_cfg.gpio_num, level, ret);
	}

	return ret;
}

static void ky008_on(void)
{
	if (ky008_set_level(g_ky008_active_level) == QOSA_GPIO_SUCCESS)
	{
		QLOGI("laser on");
	}
}

static void ky008_off(void)
{
	if (ky008_set_level(g_ky008_inactive_level) == QOSA_GPIO_SUCCESS)
	{
		QLOGI("laser off");
	}
}

static void ky008_blink_once(void)
{
	ky008_on();
	qosa_task_sleep_ms(KY008_BLINK_INTERVAL_MS);
	ky008_off();
	qosa_task_sleep_ms(KY008_BLINK_INTERVAL_MS);
}

static int ky008_gpio_init(void)
{
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pin_ret;

	gpio_ret = qosa_get_pin_default_cfg(KY008_SIGNAL_PIN, &g_ky008_pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("ky008 get pin %d default cfg failed, ret=%d", KY008_SIGNAL_PIN, gpio_ret);
		return -1;
	}

	pin_ret = qosa_pin_set_func(g_ky008_pin_cfg.pin_num, g_ky008_pin_cfg.gpio_func);
	if (pin_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("ky008 set pin %d gpio func %d failed, ret=%d", g_ky008_pin_cfg.pin_num, g_ky008_pin_cfg.gpio_func, pin_ret);
		return -1;
	}

	g_ky008_active_level = ky008_to_level(KY008_ACTIVE_LEVEL);
	g_ky008_inactive_level = (g_ky008_active_level == QOSA_GPIO_LEVEL_HIGH) ? QOSA_GPIO_LEVEL_LOW : QOSA_GPIO_LEVEL_HIGH;

	gpio_ret = qosa_gpio_init(g_ky008_pin_cfg.gpio_num,
				 QOSA_GPIO_DIRECTION_OUTPUT,
				 QOSA_GPIO_PULL_NONE,
				 g_ky008_inactive_level);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("ky008 init gpio %d failed, ret=%d", g_ky008_pin_cfg.gpio_num, gpio_ret);
		return -1;
	}

	QLOGI("ky008 gpio init ok, pin=%d gpio=%d active=%d interval=%dms",
		  g_ky008_pin_cfg.pin_num,
		  g_ky008_pin_cfg.gpio_num,
		  g_ky008_active_level,
		  KY008_BLINK_INTERVAL_MS);
	return 0;
}

static void ky008_demo_task(void *argv)
{
	(void)argv;

	if (ky008_gpio_init() != 0)
	{
		qosa_task_t current_task = QOSA_NULL;

		g_ky008_task = QOSA_NULL;
		(void)qosa_task_get_current_ref(&current_task);
		if (current_task != QOSA_NULL)
		{
			qosa_task_delete(current_task);
		}
		return;
	}

	QLOGI("ky008 demo started");

	while (1)
	{
		ky008_blink_once();
	}
}

static void ky008_demo_start(void)
{
	int ret;

	if (g_ky008_task != QOSA_NULL)
	{
		QLOGW("ky008 demo already started");
		return;
	}

	ret = qosa_task_create(&g_ky008_task,
			       KY008_TASK_STACK_SIZE,
			       KY008_TASK_PRIORITY,
			       "ky008",
			       ky008_demo_task,
			       NULL);
	if (ret != QOSA_OK)
	{
		QLOGE("create ky008 task failed: %d", ret);
		g_ky008_task = QOSA_NULL;
		return;
	}

	QLOGI("ky008 demo task created");
}

UNIRTOS_APP_EXPORT(200, "ky008_demo", ky008_demo_start);
