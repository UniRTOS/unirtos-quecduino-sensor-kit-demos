"""
@file      : water_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on Water Level Detection Example
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

/* 水位检测示例参数：ADC 通道、参考电压、量程、报警阈值和采样配置。 */
#ifndef WATER_LEVEL_ADC_CHANNEL
#define WATER_LEVEL_ADC_CHANNEL QOSA_ADC1_CHANNEL
#endif

#ifndef WATER_LEVEL_ADC_REF_MV
#define WATER_LEVEL_ADC_REF_MV 3300
#endif

#ifndef WATER_LEVEL_MAX_MM
#define WATER_LEVEL_MAX_MM 60
#endif

#ifndef WATER_LEVEL_WARN_MM
#define WATER_LEVEL_WARN_MM 15
#endif

#ifndef WATER_LEVEL_ALERT_MM
#define WATER_LEVEL_ALERT_MM 35
#endif

#ifndef WATER_LEVEL_SAMPLE_COUNT
#define WATER_LEVEL_SAMPLE_COUNT 10
#endif

#ifndef WATER_LEVEL_SAMPLE_INTERVAL_MS
#define WATER_LEVEL_SAMPLE_INTERVAL_MS 5
#endif

#ifndef WATER_LEVEL_ADC_SCALE
#define WATER_LEVEL_ADC_SCALE QOSA_ADC_SCALE_LEVEL_2
#endif

#ifndef WATER_LEVEL_DEMO_TASK_STACK_SIZE
#define WATER_LEVEL_DEMO_TASK_STACK_SIZE 2048
#endif

#ifndef WATER_LEVEL_DEMO_POLL_MS
#define WATER_LEVEL_DEMO_POLL_MS 1000
#endif

static qosa_task_t g_water_level_task = QOSA_NULL;

/* 根据换算后的水位高度返回状态名称。 */
static const char *water_level_status_name(int level_hundredths_mm)
{
	if (level_hundredths_mm < (WATER_LEVEL_WARN_MM * 100))
	{
		return "normal";
	}

	if (level_hundredths_mm < (WATER_LEVEL_ALERT_MM * 100))
	{
		return "warning";
	}

	return "alert";
}

/* 初始化水位传感器使用的 ADC 量程。 */
static int water_level_adc_init(void)
{
	qosa_adc_aux_scale_e scale = WATER_LEVEL_ADC_SCALE;
	qosa_adc_errcode_e adc_ret = qosa_adc_ioctl(WATER_LEVEL_ADC_CHANNEL, QOSA_ADC_IOCTL_SET_SCALE, &scale);

	if (adc_ret != QOSA_ADC_SUCCESS)
	{
		QLOGE("set adc channel %d scale failed, ret=%d", WATER_LEVEL_ADC_CHANNEL, adc_ret);
		return -1;
	}

	QLOGI("water level adc init ok, channel=%d ref=%dmV max=%dmm warn=%dmm alert=%dmm samples=%d",
		  WATER_LEVEL_ADC_CHANNEL,
		  WATER_LEVEL_ADC_REF_MV,
		  WATER_LEVEL_MAX_MM,
		  WATER_LEVEL_WARN_MM,
		  WATER_LEVEL_ALERT_MM,
		  WATER_LEVEL_SAMPLE_COUNT);
	return 0;
}

/* 多次采样 ADC 电压并计算平均值，单位为 0.1mV。 */
static int water_level_read_voltage_avg_tenth_mv(int *avg_tenth_mv)
{
	int sample_index;
	int voltage_mv;
	qosa_int32_t voltage_sum_mv = 0;

	if (avg_tenth_mv == QOSA_NULL)
	{
		return -1;
	}

	for (sample_index = 0; sample_index < WATER_LEVEL_SAMPLE_COUNT; sample_index++)
	{
		qosa_adc_errcode_e adc_ret = qosa_adc_get_volt(WATER_LEVEL_ADC_CHANNEL, &voltage_mv);

		if (adc_ret != QOSA_ADC_SUCCESS)
		{
			QLOGE("read adc channel %d failed, ret=%d", WATER_LEVEL_ADC_CHANNEL, adc_ret);
			return -1;
		}

		voltage_sum_mv += voltage_mv;
		qosa_task_sleep_ms(WATER_LEVEL_SAMPLE_INTERVAL_MS);
	}

	*avg_tenth_mv = (int)(((voltage_sum_mv * 10) + (WATER_LEVEL_SAMPLE_COUNT / 2)) / WATER_LEVEL_SAMPLE_COUNT);
	return 0;
}

/* 将平均电压按线性关系换算为水位高度，单位为 0.01mm。 */
static int water_level_convert_to_hundredths_mm(int voltage_tenth_mv)
{
	qosa_int32_t numerator = (qosa_int32_t)voltage_tenth_mv * WATER_LEVEL_MAX_MM * 100;
	qosa_int32_t denominator = WATER_LEVEL_ADC_REF_MV * 10;

	if (voltage_tenth_mv <= 0)
	{
		return 0;
	}

	return (int)((numerator + (denominator / 2)) / denominator);
}

/* 水位检测后台任务，周期读取电压并输出水位状态。 */
static void water_level_demo_task(void *argv)
{
	int avg_tenth_mv;
	int level_hundredths_mm;
	const char *status;

	(void)argv;

	if (water_level_adc_init() != 0)
	{
		qosa_task_t current_task = QOSA_NULL;

		g_water_level_task = QOSA_NULL;
		(void)qosa_task_get_current_ref(&current_task);
		if (current_task != QOSA_NULL)
		{
			qosa_task_delete(current_task);
		}
		return;
	}

	while (1)
	{
		if (water_level_read_voltage_avg_tenth_mv(&avg_tenth_mv) != 0)
		{
			qosa_task_sleep_ms(WATER_LEVEL_DEMO_POLL_MS);
			continue;
		}

		level_hundredths_mm = water_level_convert_to_hundredths_mm(avg_tenth_mv);
		status = water_level_status_name(level_hundredths_mm);

		QLOGI("water level: %d.%02d mm | voltage: %d.%01d mV | status: %s",
			  level_hundredths_mm / 100,
			  level_hundredths_mm % 100,
			  avg_tenth_mv / 10,
			  avg_tenth_mv % 10,
			  status);

		qosa_task_sleep_ms(WATER_LEVEL_DEMO_POLL_MS);
	}
}

/* 水位检测示例初始化入口，负责创建后台任务。 */
static void water_level_demo_init(void)
{
	int ret;

	if (g_water_level_task != QOSA_NULL)
	{
		QLOGW("water level demo already started");
		return;
	}

	ret = qosa_task_create(&g_water_level_task,
						   WATER_LEVEL_DEMO_TASK_STACK_SIZE,
						   QOSA_PRIORITY_NORMAL,
						   "water_level",
						   water_level_demo_task,
						   QOSA_NULL);
	if (ret != QOSA_OK)
	{
		QLOGE("create water level task failed, ret=%d", ret);
		g_water_level_task = QOSA_NULL;
		return;
	}

	QLOGI("water level demo started");
}

/* 将水位检测示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(200, "water_level_demo", water_level_demo_init);
