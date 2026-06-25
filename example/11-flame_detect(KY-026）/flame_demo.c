"""
@file      : flame_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on Flame Sensor Example
@version   : 0.1
@date      : 2026-06-01
@copyright : Copyright (c) 2026
"""
#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_adc.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"

#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* 火焰传感器默认接 ADC1。 */
#ifndef FLAME_DEMO_ADC_CHANNEL
#define FLAME_DEMO_ADC_CHANNEL          QOSA_ADC1_CHANNEL
#endif

/* Python 示例默认使用 GPIO31 作为报警 LED。 */
#ifndef FLAME_DEMO_LED_PIN_NUM
#define FLAME_DEMO_LED_PIN_NUM          QOSA_PIN_31
#endif

/* 火焰检测示例的任务参数和采样参数配置。 */
#define FLAME_DEMO_TASK_STACK_SIZE      2048U
/* 主监控循环固定每 1 秒采样一次。 */
#define FLAME_DEMO_SAMPLE_PERIOD_MS     1000U
/* 火灾报警时额外做 500ms 高低电平翻转，实现快闪。 */
#define FLAME_DEMO_BLINK_PERIOD_MS      500U
/* Python 版本使用 100/500 作为阈值；当前 C API 返回单位是 mV，因此这里按 mV 处理。 */
#define FLAME_DEMO_SAFE_THRESHOLD_MV    100
#define FLAME_DEMO_ALERT_THRESHOLD_MV   500
/* ADC 量程档位配置，需要和当前硬件接线匹配。 */
#define FLAME_DEMO_ADC_SCALE            QOSA_ADC_SCALE_LEVEL_2
/* 任务优先级，示例使用普通优先级即可。 */
#define FLAME_DEMO_TASK_PRIORITY        QOSA_PRIORITY_NORMAL

/* 枚举火焰状态：安全、隐患、火灾。 */
typedef enum
{
	FLAME_STATE_SAFE = 0,
	FLAME_STATE_WARNING = 1,
	FLAME_STATE_FIRE = 2,
} flame_demo_state_e;

/* 保存当前 demo 任务句柄，防止重复创建多个任务。 */
static qosa_task_t g_flame_demo_task = QOSA_NULL;
/* 保存报警 LED 对应的 GPIO 编号。 */
static qosa_gpio_num_e g_flame_demo_led_gpio_num = QOSA_GPIO_0;
/* 标记报警 LED 是否已初始化成功。 */
static qosa_bool_t g_flame_demo_led_ready = QOSA_FALSE;

/* 根据电压值返回当前火焰等级。 */
static flame_demo_state_e flame_demo_eval_state(int voltage_mv)
{
	if (voltage_mv < FLAME_DEMO_SAFE_THRESHOLD_MV)
	{
		return FLAME_STATE_SAFE;
	}

	if (voltage_mv < FLAME_DEMO_ALERT_THRESHOLD_MV)
	{
		return FLAME_STATE_WARNING;
	}

	return FLAME_STATE_FIRE;
}

/* 返回当前火焰等级对应的日志文本。 */
static const char *flame_demo_state_name(flame_demo_state_e state)
{
	switch (state)
	{
		case FLAME_STATE_SAFE:
			return "safe";
		case FLAME_STATE_WARNING:
			return "warning";
		case FLAME_STATE_FIRE:
			return "fire";
		default:
			return "unknown";
	}
}

/* 初始化 GPIO31 报警灯。 */
static int flame_demo_led_init(void)
{
	qosa_pin_cfg_t pin_cfg;
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pinctrl_ret;

	g_flame_demo_led_ready = QOSA_FALSE;
	gpio_ret = qosa_get_pin_default_cfg(FLAME_DEMO_LED_PIN_NUM, &pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("get led pin %d default cfg failed, ret=%d", FLAME_DEMO_LED_PIN_NUM, gpio_ret);
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

	g_flame_demo_led_gpio_num = pin_cfg.gpio_num;
	g_flame_demo_led_ready = QOSA_TRUE;
	QLOGI("flame led init ok, pin=%d gpio=%d", pin_cfg.pin_num, pin_cfg.gpio_num);
	return 0;
}

/* 初始化 ADC 量程。 */
static int flame_demo_adc_init(void)
{
	qosa_adc_aux_scale_e scale = FLAME_DEMO_ADC_SCALE;
	qosa_adc_errcode_e adc_ret = qosa_adc_ioctl(FLAME_DEMO_ADC_CHANNEL, QOSA_ADC_IOCTL_SET_SCALE, &scale);

	if (adc_ret != QOSA_ADC_SUCCESS)
	{
		QLOGE("adc channel %d scale config failed, ret=0x%x", FLAME_DEMO_ADC_CHANNEL, adc_ret);
		return -1;
	}

	QLOGI("flame adc init ok, channel=%d safe<%dmV fire>=%dmV", FLAME_DEMO_ADC_CHANNEL, FLAME_DEMO_SAFE_THRESHOLD_MV, FLAME_DEMO_ALERT_THRESHOLD_MV);
	return 0;
}

/* 设置 LED 输出电平。 */
static void flame_demo_set_led_level(qosa_gpio_level_e level)
{
	if (g_flame_demo_led_ready != QOSA_TRUE)
	{
		return;
	}

	(void)qosa_gpio_set_level(g_flame_demo_led_gpio_num, level);
}

/* 火灾报警时执行一次快闪。 */
static void flame_demo_blink_led(void)
{
	flame_demo_set_led_level(QOSA_GPIO_LEVEL_HIGH);
	qosa_task_sleep_ms(FLAME_DEMO_BLINK_PERIOD_MS);
	flame_demo_set_led_level(QOSA_GPIO_LEVEL_LOW);
	qosa_task_sleep_ms(FLAME_DEMO_BLINK_PERIOD_MS);
}

/*
 * 火焰 demo 的主任务函数。
 * 任务启动后先配置 ADC 与报警 LED，然后循环采样、分级并联动 LED。
 */
static void flame_demo_task(void *argv)
{
	flame_demo_state_e last_state = (flame_demo_state_e)-1;

	/* 当前任务函数没有入参，显式丢弃以避免未使用参数警告。 */
	(void)argv;

	if ((flame_demo_led_init() != 0) || (flame_demo_adc_init() != 0))
	{
		qosa_task_t current_task = QOSA_NULL;

		g_flame_demo_task = QOSA_NULL;
		(void)qosa_task_get_current_ref(&current_task);
		if (current_task != QOSA_NULL)
		{
			qosa_task_delete(current_task);
		}
		return;
	}

	QLOGI("flame demo started");

	while (1)
	{
		int voltage_mv = 0;
		flame_demo_state_e current_state;
		qosa_adc_errcode_e adc_ret;

		adc_ret = qosa_adc_get_volt(FLAME_DEMO_ADC_CHANNEL, &voltage_mv);
		if (adc_ret != QOSA_ADC_SUCCESS)
		{
			QLOGE("adc channel %d read failed, ret=0x%x", FLAME_DEMO_ADC_CHANNEL, adc_ret);
			qosa_task_sleep_ms(FLAME_DEMO_SAMPLE_PERIOD_MS);
			continue;
		}

		current_state = flame_demo_eval_state(voltage_mv);

		if (current_state == FLAME_STATE_SAFE)
		{
			flame_demo_set_led_level(QOSA_GPIO_LEVEL_LOW);
		}
		else if (current_state == FLAME_STATE_WARNING)
		{
			flame_demo_set_led_level(QOSA_GPIO_LEVEL_HIGH);
		}

		if (current_state != last_state)
		{
			QLOGI("flame state change: adc=%dmV | state=%s", voltage_mv, flame_demo_state_name(current_state));
			last_state = current_state;
		}
		else
		{
			QLOGI("flame monitor: adc=%dmV | state=%s", voltage_mv, flame_demo_state_name(current_state));
		}

		if (current_state == FLAME_STATE_FIRE)
		{
			flame_demo_blink_led();
		}
		else
		{
			qosa_task_sleep_ms(FLAME_DEMO_SAMPLE_PERIOD_MS);
		}
	}
}

/*
 * demo 的启动入口，由系统注册后在合适时机调用。
 * 它负责保证任务只会创建一次，并在创建成功后输出启动日志。
 */
static void flame_demo_bootstrap(void)
{
	/* 任务创建返回值。 */
	int ret;

	/* 如果任务已经存在，说明 demo 早已启动，直接返回即可。 */
	if (g_flame_demo_task != QOSA_NULL)
	{
		QLOGW("flame demo task already running");
		return;
	}

	/* 创建火焰检测任务，任务会在后台持续执行。 */
	ret = qosa_task_create(&g_flame_demo_task,
						   FLAME_DEMO_TASK_STACK_SIZE,
						   FLAME_DEMO_TASK_PRIORITY,
						   "flame_demo",
						   flame_demo_task,
						   QOSA_NULL);
	if (ret != 0)
	{
		QLOGE("flame demo task create failed, ret=%d", ret);
		/* 创建失败时，把句柄清空，避免后续误判为已启动。 */
		g_flame_demo_task = QOSA_NULL;
		return;
	}

	/* 任务创建成功，输出一条完成日志。 */
	QLOGI("flame demo bootstrap complete");
}

UNIRTOS_APP_EXPORT(220, "flame_demo", flame_demo_bootstrap);
