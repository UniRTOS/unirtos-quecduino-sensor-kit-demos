#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"

#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/*
 * 当前平台默认管脚映射使用 QOSA_PIN_23 / QOSA_PIN_22。
 */
#define TILT_SWITCH_SENSOR_PIN         QOSA_PIN_23
#define TILT_SWITCH_OUTPUT_PIN         QOSA_PIN_22
#define TILT_SWITCH_TRIGGER_LEVEL      QOSA_GPIO_LEVEL_HIGH
#define TILT_SWITCH_OUTPUT_ACTIVE      QOSA_GPIO_LEVEL_HIGH

#define TILT_SWITCH_TASK_STACK_SIZE    2048
#define TILT_SWITCH_POLL_INTERVAL_MS   1000

static qosa_task_t g_tilt_switch_demo_task = QOSA_NULL;
static qosa_pin_cfg_t g_tilt_switch_sensor_pin_cfg;
static qosa_pin_cfg_t g_tilt_switch_output_pin_cfg;

static qosa_gpio_level_e tilt_switch_get_inactive_level(qosa_gpio_level_e active_level)
{
	return (active_level == QOSA_GPIO_LEVEL_HIGH) ? QOSA_GPIO_LEVEL_LOW : QOSA_GPIO_LEVEL_HIGH;
}

static qosa_bool_t tilt_switch_is_tilted(qosa_gpio_level_e level)
{
	return (level == TILT_SWITCH_TRIGGER_LEVEL) ? QOSA_TRUE : QOSA_FALSE;
}

static void tilt_switch_set_output(qosa_bool_t active)
{
	qosa_gpio_level_e output_level = tilt_switch_get_inactive_level(TILT_SWITCH_OUTPUT_ACTIVE);

	if (active == QOSA_TRUE)
	{
		output_level = TILT_SWITCH_OUTPUT_ACTIVE;
	}

	(void)qosa_gpio_set_level(g_tilt_switch_output_pin_cfg.gpio_num, output_level);
}

static int tilt_switch_prepare_gpio(qosa_pin_num_e pin_num,
								qosa_gpio_direction_e direction,
								qosa_gpio_pull_e pull,
								qosa_gpio_level_e level,
								qosa_pin_cfg_t *pin_cfg)
{
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pinctrl_ret;

	gpio_ret = qosa_get_pin_default_cfg(pin_num, pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("get pin cfg failed, pin=%d ret=%d", pin_num, gpio_ret);
		return -1;
	}

	pinctrl_ret = qosa_pin_set_func(pin_cfg->pin_num, pin_cfg->gpio_func);
	if (pinctrl_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("set pin func failed, pin=%d gpio_func=%d ret=%d",
			  pin_cfg->pin_num,
			  pin_cfg->gpio_func,
			  pinctrl_ret);
		return -1;
	}

	gpio_ret = qosa_gpio_init(pin_cfg->gpio_num, direction, pull, level);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("gpio init failed, pin=%d gpio=%d ret=%d", pin_num, pin_cfg->gpio_num, gpio_ret);
		return -1;
	}

	return 0;
}

static void tilt_switch_demo_task(void *argv)
{
	qosa_gpio_level_e sensor_level = QOSA_GPIO_LEVEL_LOW;
	qosa_bool_t tilted;

	(void)argv;

	QLOGI("tilt switch demo started: sensor pin=%d gpio=%d, output pin=%d gpio=%d",
		  g_tilt_switch_sensor_pin_cfg.pin_num,
		  g_tilt_switch_sensor_pin_cfg.gpio_num,
		  g_tilt_switch_output_pin_cfg.pin_num,
		  g_tilt_switch_output_pin_cfg.gpio_num);

	for (;;)
	{
		if (qosa_gpio_get_level(g_tilt_switch_sensor_pin_cfg.gpio_num, &sensor_level) != QOSA_GPIO_SUCCESS)
		{
			QLOGW("read sensor level failed, gpio=%d", g_tilt_switch_sensor_pin_cfg.gpio_num);
			qosa_task_sleep_ms(TILT_SWITCH_POLL_INTERVAL_MS);
			continue;
		}

		tilted = tilt_switch_is_tilted(sensor_level);
		tilt_switch_set_output(tilted);

		if (tilted == QOSA_TRUE)
		{
			QLOGI("检测到倾斜");
		}
		else
		{
			QLOGI("位置正常");
		}

		qosa_task_sleep_ms(TILT_SWITCH_POLL_INTERVAL_MS);
	}
}

static void magic_halo_demo_init(void)
{
	int ret;
	qosa_gpio_level_e output_inactive_level = tilt_switch_get_inactive_level(TILT_SWITCH_OUTPUT_ACTIVE);

	if (g_tilt_switch_demo_task != QOSA_NULL)
	{
		QLOGW("tilt switch demo already started");
		return;
	}

	if (tilt_switch_prepare_gpio(TILT_SWITCH_SENSOR_PIN,
							 QOSA_GPIO_DIRECTION_INPUT,
							 QOSA_GPIO_PULL_DOWN,
							 QOSA_GPIO_LEVEL_LOW,
							 &g_tilt_switch_sensor_pin_cfg) != 0)
	{
		return;
	}

	if (tilt_switch_prepare_gpio(TILT_SWITCH_OUTPUT_PIN,
							 QOSA_GPIO_DIRECTION_OUTPUT,
							 QOSA_GPIO_PULL_NONE,
							 output_inactive_level,
							 &g_tilt_switch_output_pin_cfg) != 0)
	{
		(void)qosa_gpio_deinit(g_tilt_switch_sensor_pin_cfg.gpio_num);
		return;
	}

	ret = qosa_task_create(&g_tilt_switch_demo_task,
						   TILT_SWITCH_TASK_STACK_SIZE,
						   QOSA_PRIORITY_NORMAL,
						   "tilt_switch_demo",
						   tilt_switch_demo_task,
						   QOSA_NULL);
	if (ret != QOSA_ERROR_OK)
	{
		QLOGE("create tilt switch demo task failed, ret=%d", ret);
		(void)qosa_gpio_deinit(g_tilt_switch_sensor_pin_cfg.gpio_num);
		(void)qosa_gpio_deinit(g_tilt_switch_output_pin_cfg.gpio_num);
		return;
	}

	QLOGI("tilt switch demo init done");
}

UNIRTOS_APP_EXPORT(260, "magic_halo_demo", magic_halo_demo_init);
