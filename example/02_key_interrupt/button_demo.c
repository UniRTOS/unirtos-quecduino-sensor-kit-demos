/*
@file      : button_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on Button Interrupt Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
*/
#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"

#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* 按键中断示例：配置一个 GPIO 作为按键输入，注册中断，并在按下时打印日志。 */
#define BUTTON_DEMO_PIN_NUM QOSA_PIN_29

/* 保存按键引脚解析后的平台配置。 */
static qosa_pin_cfg_t g_button_pin_cfg;
/* 记录已经确认的有效按键次数。 */
static unsigned int   g_button_irq_count;
/* 标记中断是否已经完成初始化。 */
static int            g_button_irq_ready;

/* GPIO 框架调用的中断回调，读取当前电平并将低电平视为按键按下。 */
static void button_demo_irq_callback(void *argv)
{
	/* 中断注册时将解析后的引脚配置作为用户上下文传入。 */
	qosa_pin_cfg_t      *pin_cfg = (qosa_pin_cfg_t *)argv;
	/* 默认设为高电平，避免异常读取被误判为按下。 */
	qosa_gpio_level_e    level = QOSA_GPIO_LEVEL_HIGH;
	/* 保存 GPIO 读取接口的返回值。 */
	qosa_gpio_error_e    ret;

	/* 若回调上下文为空，则无法安全继续处理。 */
	if (pin_cfg == NULL)
	{
		QLOGW("button irq callback missing pin context");
		return;
	}

	/* 初始化完成前忽略中断，避免状态未就绪时误处理。 */
	if (!g_button_irq_ready)
	{
		return;
	}

	/* 读取按键 GPIO 当前电平，用于过滤毛刺或误触发。 */
	ret = qosa_gpio_get_level(pin_cfg->gpio_num, &level);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		/* 读取失败时无法确认是否真的按下。 */
		QLOGW("button gpio=%d get level failed ret=%d",
			  (int)pin_cfg->gpio_num,
			  (int)ret);
		return;
	}

	/* 本示例使用上拉和下降沿触发，因此低电平表示按键按下。 */
	if (level != QOSA_GPIO_LEVEL_LOW)
	{
		/* 非低电平不作为有效按键事件。 */
		return;
	}

	/* 只统计确认后的按键事件。 */
	++g_button_irq_count;

	/* 打印按键次数、PIN 和 GPIO 编号，便于调试接线。 */
	QLOGI("button pressed count=%u pin=%u gpio=%d",
		  g_button_irq_count,
		  (unsigned int)pin_cfg->pin_num,
		  (int)pin_cfg->gpio_num);
}

/* 初始化按键示例：解析引脚、切换 GPIO 复用、注册中断并使能下降沿检测。 */
static void button_demo_init(void)
{
	/* 传递给 GPIO 框架的中断配置。 */
	qosa_int_cfg_t    int_cfg = {0};
	/* 保存 GPIO 相关接口的返回值。 */
	qosa_gpio_error_e gpio_ret;
	/* 保存引脚复用配置接口的返回值。 */
	qosa_pinctrl_error_e pinctrl_ret;

	/* 注册中断前重置运行状态。 */
	g_button_irq_ready = 0;
	g_button_irq_count = 0;

	/* 将编译期 PIN 编号转换成平台 GPIO 配置。 */
	gpio_ret = qosa_get_pin_default_cfg((qosa_uint8_t)BUTTON_DEMO_PIN_NUM, &g_button_pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		/* 无法解析引脚配置时停止初始化。 */
		QLOGE("button demo get pin %u default cfg failed ret=%d",
			  (unsigned int)BUTTON_DEMO_PIN_NUM,
			  (int)gpio_ret);
		return;
	}

	/* 切换引脚复用为 GPIO，作为按键输入使用。 */
	pinctrl_ret = qosa_pin_set_func((qosa_pin_num_e)g_button_pin_cfg.pin_num, g_button_pin_cfg.gpio_func);
	if (pinctrl_ret != QOSA_PINCTRL_SUCCESS)
	{
		/* 引脚无法配置为 GPIO 时终止初始化。 */
		QLOGE("button demo set pin %u gpio func failed ret=%d",
			  (unsigned int)g_button_pin_cfg.pin_num,
			  (int)pinctrl_ret);
		return;
	}

	/* 准备中断配置：
	 * - 启用消抖，降低机械按键抖动影响；
	 * - 启用上拉，配合下降沿触发；
	 * - user_ctx 将引脚配置传入回调函数。
	 */
	int_cfg.gpio_num = g_button_pin_cfg.gpio_num;
	int_cfg.gpio_debounce = QOSA_GPIO_DEBOUNCE_EN;
	int_cfg.gpio_pull = QOSA_GPIO_PULL_UP;
	int_cfg.interrupt_cb = button_demo_irq_callback;
	int_cfg.options = 1;
	int_cfg.user_ctx = &g_button_pin_cfg;

	/* 向 GPIO 子系统注册中断处理函数。 */
	gpio_ret = qosa_interrupt_register(&int_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		/* 中断注册失败时停止初始化。 */
		QLOGE("button demo register irq failed pin=%u gpio=%d ret=%d",
			  (unsigned int)g_button_pin_cfg.pin_num,
			  (int)g_button_pin_cfg.gpio_num,
			  (int)gpio_ret);
		return;
	}

	/* 使能下降沿中断，使按下动作产生事件。 */
	gpio_ret = qosa_interrupt_enable(g_button_pin_cfg.gpio_num, QOSA_GPIO_TRIGGER_FALLING_EDGE);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		/* 使能失败时注销中断，保持系统状态干净。 */
		QLOGE("button demo enable irq failed pin=%u gpio=%d ret=%d",
			  (unsigned int)g_button_pin_cfg.pin_num,
			  (int)g_button_pin_cfg.gpio_num,
			  (int)gpio_ret);
		(void)qosa_interrupt_unregister(g_button_pin_cfg.gpio_num);
		return;
	}

	/* 注册和使能都成功后，才标记示例就绪。 */
	g_button_irq_ready = 1;

	/* 输出摘要日志，确认当前使用的引脚和触发方式。 */
	QLOGI("button irq demo ready: pin=%u gpio=%d trigger=falling-edge pull-up=on debounce=on",
		  (unsigned int)g_button_pin_cfg.pin_num,
		  (int)g_button_pin_cfg.gpio_num);
}

/* 将按键中断示例注册到应用启动框架。 */
UNIRTOS_APP_EXPORT(200, "button_irq_demo", button_demo_init);
