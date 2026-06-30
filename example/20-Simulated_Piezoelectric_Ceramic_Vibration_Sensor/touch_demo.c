"""
@file      : touch_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on Piezoelectric Vibration Sensor Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
"""
#include "qcm_proj_config.h"
#include "qosa_adc.h"
#include "qosa_log.h"
#include "qosa_sys.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* 压电振动传感器示例参数：采样通道、阈值、轮询周期和任务栈。 */
#define TOUCH_DEMO_TASK_STACK_SIZE    2048
#define TOUCH_DEMO_POLL_INTERVAL_MS   200
#define TOUCH_DEMO_ALERT_THRESHOLD_MV 1500
#define TOUCH_DEMO_ADC_CHANNEL        QOSA_ADC1_CHANNEL
#define TOUCH_DEMO_ADC_SCALE          QOSA_ADC_SCALE_LEVEL_2

static qosa_task_t g_touch_demo_task = QOSA_NULL;

/* 打开并配置振动传感器使用的 ADC 通道。 */
static qosa_bool_t touch_demo_open_adc(void)
{
	qosa_adc_aux_scale_e scale = TOUCH_DEMO_ADC_SCALE;
	qosa_adc_errcode_e ret;

	ret = qosa_adc_ioctl(TOUCH_DEMO_ADC_CHANNEL, QOSA_ADC_IOCTL_SET_SCALE, &scale);
	if (ret != QOSA_ADC_SUCCESS)
	{
		QLOGE("touch demo adc init failed, channel=%d ret=%d", TOUCH_DEMO_ADC_CHANNEL, ret);
		return QOSA_FALSE;
	}

	QLOGI("touch demo adc init ok, channel=%d threshold=%dmV poll=%dms",
		  TOUCH_DEMO_ADC_CHANNEL,
		  TOUCH_DEMO_ALERT_THRESHOLD_MV,
		  TOUCH_DEMO_POLL_INTERVAL_MS);
	return QOSA_TRUE;
}

/* 读取当前振动传感器 ADC 电压值。 */
static qosa_bool_t touch_demo_read_value(int *value_mv)
{
	qosa_adc_errcode_e ret;

	ret = qosa_adc_get_volt(TOUCH_DEMO_ADC_CHANNEL, value_mv);
	if (ret != QOSA_ADC_SUCCESS)
	{
		QLOGE("touch demo adc read failed, channel=%d ret=%d", TOUCH_DEMO_ADC_CHANNEL, ret);
		return QOSA_FALSE;
	}

	return QOSA_TRUE;
}

/* 判断当前电压是否超过振动报警阈值。 */
static qosa_bool_t touch_demo_check_alert(int value_mv)
{
	return (value_mv >= TOUCH_DEMO_ALERT_THRESHOLD_MV) ? QOSA_TRUE : QOSA_FALSE;
}

/* 振动检测后台任务，周期采样并输出报警状态。 */
static void touch_demo_task(void *argv)
{
	int value_mv;

	(void)argv;

	if (touch_demo_open_adc() != QOSA_TRUE)
	{
		g_touch_demo_task = QOSA_NULL;
		return;
	}

	QLOGI("touch demo started");

	while (1)
	{
		if (touch_demo_read_value(&value_mv) == QOSA_TRUE)
		{
			if (touch_demo_check_alert(value_mv) == QOSA_TRUE)
			{
				QLOGI("vibration alert, value=%dmV", value_mv);
			}
			else
			{
				QLOGI("vibration value=%dmV", value_mv);
			}
		}

		qosa_task_sleep_ms(TOUCH_DEMO_POLL_INTERVAL_MS);
	}
}

/* 振动传感器示例初始化入口，负责创建采样任务。 */
static void touch_demo_init(void)
{
	int ret;

	if (g_touch_demo_task != QOSA_NULL)
	{
		return;
	}

	ret = qosa_task_create(&g_touch_demo_task,
				   TOUCH_DEMO_TASK_STACK_SIZE,
				   QOSA_PRIORITY_NORMAL,
				   "touch_demo",
				   touch_demo_task,
				   QOSA_NULL);
	if (ret != QOSA_ERROR_OK)
	{
		QLOGE("touch demo task create failed, ret=%d", ret);
		g_touch_demo_task = QOSA_NULL;
		return;
	}

	QLOGI("touch demo task created");
}

/* 将压电振动传感器示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(200, "touch_demo", touch_demo_init);
