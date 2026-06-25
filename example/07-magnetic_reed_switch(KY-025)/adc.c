#include "qcm_proj_config.h"
#include "qosa_adc.h"
#include "qosa_gpio.h"
#include "qosa_log.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

#ifndef MAGNETIC_REED_ADC_CHANNEL
#define MAGNETIC_REED_ADC_CHANNEL QOSA_ADC1_CHANNEL
#endif

#ifndef MAGNETIC_REED_THRESHOLD_MV
#define MAGNETIC_REED_THRESHOLD_MV 100
#endif

#ifndef MAGNETIC_REED_LED_PIN_NUM
#define MAGNETIC_REED_LED_PIN_NUM QOSA_PIN_19
#endif

#ifndef MAGNETIC_REED_ADC_SCALE
#define MAGNETIC_REED_ADC_SCALE QOSA_ADC_SCALE_LEVEL_2
#endif

#ifndef MAGNETIC_REED_POLL_MS
#define MAGNETIC_REED_POLL_MS 500
#endif

#ifndef MAGNETIC_REED_TASK_STACK_SIZE
#define MAGNETIC_REED_TASK_STACK_SIZE 2048
#endif

static qosa_task_t g_magnetic_reed_task = QOSA_NULL;
static qosa_gpio_num_e g_led_gpio_num = QOSA_GPIO_0;
static qosa_bool_t g_led_ready = QOSA_FALSE;

static const char *magnetic_reed_status_name(int voltage_mv)
{
	if (voltage_mv > MAGNETIC_REED_THRESHOLD_MV)
	{
		return "detected";
	}

	return "idle";
}

static void magnetic_reed_set_led(qosa_bool_t detected)
{
	if (g_led_ready != QOSA_TRUE)
	{
		return;
	}

	(void)qosa_gpio_set_level(g_led_gpio_num,
						  detected == QOSA_TRUE ? QOSA_GPIO_LEVEL_HIGH : QOSA_GPIO_LEVEL_LOW);
}

static int magnetic_reed_led_init(void)
{
	qosa_pin_cfg_t pin_cfg;
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pinctrl_ret;

	g_led_ready = QOSA_FALSE;
	gpio_ret = qosa_get_pin_default_cfg(MAGNETIC_REED_LED_PIN_NUM, &pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("get led pin %d default cfg failed, ret=%d", MAGNETIC_REED_LED_PIN_NUM, gpio_ret);
		return -1;
	}

	pinctrl_ret = qosa_pin_set_func(pin_cfg.pin_num, pin_cfg.gpio_func);
	if (pinctrl_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("set led pin %d gpio func failed, ret=%d", pin_cfg.pin_num, pinctrl_ret);
		return -1;
	}

	gpio_ret = qosa_gpio_init(pin_cfg.gpio_num,
					 QOSA_GPIO_DIRECTION_OUTPUT,
					 QOSA_GPIO_PULL_NONE,
					 QOSA_GPIO_LEVEL_LOW);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("init led gpio %d failed, ret=%d", pin_cfg.gpio_num, gpio_ret);
		return -1;
	}

	g_led_gpio_num = pin_cfg.gpio_num;
	g_led_ready = QOSA_TRUE;
	QLOGI("led init ok, pin=%d gpio=%d", pin_cfg.pin_num, pin_cfg.gpio_num);
	return 0;
}

static int magnetic_reed_adc_init(void)
{
	qosa_adc_aux_scale_e scale = MAGNETIC_REED_ADC_SCALE;
	qosa_adc_errcode_e adc_ret = qosa_adc_ioctl(MAGNETIC_REED_ADC_CHANNEL, QOSA_ADC_IOCTL_SET_SCALE, &scale);

	if (adc_ret != QOSA_ADC_SUCCESS)
	{
		QLOGE("set adc channel %d scale failed, ret=%d", MAGNETIC_REED_ADC_CHANNEL, adc_ret);
		return -1;
	}

	QLOGI("magnetic reed adc init ok, channel=%d threshold=%dmV poll=%dms",
		  MAGNETIC_REED_ADC_CHANNEL,
		  MAGNETIC_REED_THRESHOLD_MV,
		  MAGNETIC_REED_POLL_MS);
	return 0;
}

static void magnetic_reed_demo_task(void *argv)
{
	int voltage_mv;
	qosa_bool_t detected;

	(void)argv;

	if ((magnetic_reed_led_init() != 0) || (magnetic_reed_adc_init() != 0))
	{
		qosa_task_t current_task = QOSA_NULL;

		g_magnetic_reed_task = QOSA_NULL;
		(void)qosa_task_get_current_ref(&current_task);
		if (current_task != QOSA_NULL)
		{
			qosa_task_delete(current_task);
		}
		return;
	}

	while (1)
	{
		qosa_adc_errcode_e adc_ret = qosa_adc_get_volt(MAGNETIC_REED_ADC_CHANNEL, &voltage_mv);

		if (adc_ret != QOSA_ADC_SUCCESS)
		{
			QLOGE("read adc channel %d failed, ret=%d", MAGNETIC_REED_ADC_CHANNEL, adc_ret);
			qosa_task_sleep_ms(MAGNETIC_REED_POLL_MS);
			continue;
		}

		detected = (voltage_mv > MAGNETIC_REED_THRESHOLD_MV) ? QOSA_TRUE : QOSA_FALSE;
		magnetic_reed_set_led(detected);

		QLOGI("magnetic reed: voltage=%dmV | status=%s",
			  voltage_mv,
			  magnetic_reed_status_name(voltage_mv));

		qosa_task_sleep_ms(MAGNETIC_REED_POLL_MS);
	}
}

static void magnetic_reed_demo_init(void)
{
	int ret;

	if (g_magnetic_reed_task != QOSA_NULL)
	{
		QLOGW("magnetic reed demo already started");
		return;
	}

	ret = qosa_task_create(&g_magnetic_reed_task,
				       MAGNETIC_REED_TASK_STACK_SIZE,
				       QOSA_PRIORITY_NORMAL,
				       "mag_reed",
				       magnetic_reed_demo_task,
				       QOSA_NULL);
	if (ret != QOSA_OK)
	{
		QLOGE("create magnetic reed task failed, ret=%d", ret);
		g_magnetic_reed_task = QOSA_NULL;
		return;
	}

	QLOGI("magnetic reed demo started");
}

UNIRTOS_APP_EXPORT(200, "magnetic_reed_demo", magnetic_reed_demo_init);
