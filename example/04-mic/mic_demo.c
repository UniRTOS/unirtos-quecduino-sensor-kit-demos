/*
@file      : mic_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on Microphone Sensor Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
*/
#include <stddef.h>

#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_adc.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"
#include "unirtos_apps_config.h"

#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* 麦克风示例参数：ADC 采样、LED 引脚、阈值和任务配置。 */
#define MIC_APP_ORDER             210
#define MIC_TASK_STACK_SIZE       2048
#define MIC_TASK_PRIORITY         QOSA_PRIORITY_NORMAL
#define MIC_LED_PIN_NUM           QOSA_PIN_31
#define MIC_THRESHOLD_MV          200
#define MIC_SAMPLE_INTERVAL_MS    500
#define MIC_LED_ON_SEC            2
#define MIC_LED_ACTIVE_LEVEL      QOSA_GPIO_LEVEL_HIGH

static qosa_task_t g_mic_task = NULL;
/* 保存声音触发提示 LED 的引脚配置。 */
static qosa_pin_cfg_t g_mic_led_pin_cfg;
/* 标记 LED GPIO 是否已经初始化完成。 */
static qosa_bool_t g_mic_led_ready = QOSA_FALSE;

/* 根据有效电平计算 LED 的无效电平。 */
static qosa_gpio_level_e mic_get_led_inactive_level(void)
{
	if (MIC_LED_ACTIVE_LEVEL == QOSA_GPIO_LEVEL_HIGH)
	{
		return QOSA_GPIO_LEVEL_LOW;
	}

	return QOSA_GPIO_LEVEL_HIGH;
}

/* 配置 ADC 量程，保证麦克风模拟电压读取范围正确。 */
static void mic_configure_adc(void)
{
	qosa_adc_aux_scale_e adc_scale = QOSA_ADC_SCALE_LEVEL_2;
	qosa_adc_errcode_e ret;

	ret = qosa_adc_ioctl(QOSA_ADC1_CHANNEL, QOSA_ADC_IOCTL_SET_SCALE, &adc_scale);
	if (ret != QOSA_ADC_SUCCESS)
	{
		QLOGW("adc1 scale config failed, ret=%d", ret);
	}
}

/* 初始化提示 LED 对应的 GPIO。 */
static int mic_prepare_led_gpio(void)
{
	qosa_gpio_error_e gpio_ret;
	qosa_pinctrl_error_e pin_ret;

	if (g_mic_led_ready)
	{
		return QOSA_ERROR_OK;
	}

	qosa_memset(&g_mic_led_pin_cfg, 0, sizeof(g_mic_led_pin_cfg));
	gpio_ret = qosa_get_pin_default_cfg((qosa_uint8_t)MIC_LED_PIN_NUM, &g_mic_led_pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("mic led pin default cfg failed, pin=%u ret=%d", (unsigned int)MIC_LED_PIN_NUM, (int)gpio_ret);
		return -1;
	}

	pin_ret = qosa_pin_set_func((qosa_pin_num_e)g_mic_led_pin_cfg.pin_num, g_mic_led_pin_cfg.gpio_func);
	if (pin_ret != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("mic led pin func config failed, pin=%u func=%u ret=%d",
			(unsigned int)g_mic_led_pin_cfg.pin_num,
			(unsigned int)g_mic_led_pin_cfg.gpio_func,
			(int)pin_ret);
		return -1;
	}

	gpio_ret = qosa_gpio_init(
		g_mic_led_pin_cfg.gpio_num,
		QOSA_GPIO_DIRECTION_OUTPUT,
		g_mic_led_pin_cfg.gpio_pull,
		mic_get_led_inactive_level());
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		QLOGE("mic led gpio init failed, gpio=%u ret=%d", (unsigned int)g_mic_led_pin_cfg.gpio_num, (int)gpio_ret);
		return -1;
	}

	g_mic_led_ready = QOSA_TRUE;
	return QOSA_ERROR_OK;
}

/* 当采样值超过阈值时点亮 LED 一段时间。 */
static void mic_handle_sound(int sample_mv)
{
	if (sample_mv <= MIC_THRESHOLD_MV)
	{
		return;
	}

	qosa_gpio_set_level(g_mic_led_pin_cfg.gpio_num, MIC_LED_ACTIVE_LEVEL);
	qosa_task_sleep_sec(MIC_LED_ON_SEC);
	qosa_gpio_set_level(g_mic_led_pin_cfg.gpio_num, mic_get_led_inactive_level());
}

/* 麦克风检测任务：周期读取 ADC，并根据声音强度控制 LED。 */
static void mic_demo_task(void *argv)
{
	qosa_adc_errcode_e ret;
	int sample_mv = 0;

	(void)argv;
	mic_configure_adc();
	if (mic_prepare_led_gpio() != QOSA_ERROR_OK)
	{
		QLOGE("mic led gpio prepare failed");
		return;
	}

	QLOGI("MIC demo started: adc=ADC1 threshold=%dmV sample=%dms led_pin=%u led_on=%ds",
		MIC_THRESHOLD_MV,
		MIC_SAMPLE_INTERVAL_MS,
		(unsigned int)MIC_LED_PIN_NUM,
		MIC_LED_ON_SEC);

	while (1)
	{
		ret = qosa_adc_get_volt(QOSA_ADC1_CHANNEL, &sample_mv);
		if (ret != QOSA_ADC_SUCCESS)
		{
			QLOGE("adc1 read failed, ret=%d", ret);
			qosa_task_sleep_sec(1);
			continue;
		}

		QLOGI("adc1 value=%dmV", sample_mv);
		mic_handle_sound(sample_mv);
		qosa_task_sleep_ms(MIC_SAMPLE_INTERVAL_MS);
	}
}

/* 麦克风示例初始化入口，负责创建后台检测任务。 */
static void mic_demo_init(void)
{
	int ret;

	if (g_mic_task != NULL)
	{
		QLOGW("MIC demo already running");
		return;
	}

	ret = qosa_task_create(&g_mic_task, MIC_TASK_STACK_SIZE, MIC_TASK_PRIORITY, "mic_demo", mic_demo_task, NULL);
	if (ret != QOSA_ERROR_OK)
	{
		g_mic_task = NULL;
		QLOGE("create MIC demo task failed, ret=%d", ret);
		return;
	}

	QLOGI("MIC demo init done, adc=ADC1 led_pin=%u", (unsigned int)MIC_LED_PIN_NUM);
}

/* 将麦克风示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(MIC_APP_ORDER, "mic_demo", mic_demo_init);
