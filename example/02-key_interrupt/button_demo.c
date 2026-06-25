#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"

#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/*
 * Button interrupt demo:
 * This example configures one GPIO as a button input, registers an interrupt,
 * and prints a log message each time the button is pressed.
 */
#define BUTTON_DEMO_PIN_NUM QOSA_PIN_29

/* Resolved pin configuration for the selected button pin. */
static qosa_pin_cfg_t g_button_pin_cfg;
/* Counts how many valid button presses have been detected. */
static unsigned int   g_button_irq_count;
/* Marks whether interrupt handling has been fully initialized. */
static int            g_button_irq_ready;

/* Interrupt callback invoked by the GPIO framework.
 * The callback reads the current GPIO level and treats a low level as a button press.
 */
static void button_demo_irq_callback(void *argv)
{
	/* The interrupt registration passes the resolved pin configuration as user context. */
	qosa_pin_cfg_t      *pin_cfg = (qosa_pin_cfg_t *)argv;
	/* Default to high so an unexpected read does not look like a press. */
	qosa_gpio_level_e    level = QOSA_GPIO_LEVEL_HIGH;
	/* Stores the return value from the GPIO read API. */
	qosa_gpio_error_e    ret;

	/* If the callback context is missing, there is no safe way to continue. */
	if (pin_cfg == NULL)
	{
		QLOGW("button irq callback missing pin context");
		return;
	}

	/* Ignore interrupts until the module finishes initialization. */
	if (!g_button_irq_ready)
	{
		return;
	}

	/* Read the current level from the button GPIO so we can filter out spurious triggers. */
	ret = qosa_gpio_get_level(pin_cfg->gpio_num, &level);
	if (ret != QOSA_GPIO_SUCCESS)
	{
		/* A failed read means we cannot confirm whether a press happened. */
		QLOGW("button gpio=%d get level failed ret=%d",
			  (int)pin_cfg->gpio_num,
			  (int)ret);
		return;
	}

	/* This demo uses a falling-edge trigger with pull-up, so a low level means pressed. */
	if (level != QOSA_GPIO_LEVEL_LOW)
	{
		/* If the line is not low, the interrupt is not treated as a valid press event. */
		return;
	}

	/* Count only confirmed button presses. */
	++g_button_irq_count;

	/* Log the press event together with the pin and GPIO number for debugging. */
	QLOGI("button pressed count=%u pin=%u gpio=%d",
		  g_button_irq_count,
		  (unsigned int)pin_cfg->pin_num,
		  (int)pin_cfg->gpio_num);
}

/* Initialize the button demo.
 * This function resolves the pin configuration, sets the pin to GPIO mode,
 * registers the interrupt callback, and enables falling-edge detection.
 */
static void button_demo_init(void)
{
	/* Interrupt configuration passed to the GPIO framework. */
	qosa_int_cfg_t    int_cfg = {0};
	/* Stores the result of GPIO-related API calls. */
	qosa_gpio_error_e gpio_ret;
	/* Stores the result of pinctrl configuration. */
	qosa_pinctrl_error_e pinctrl_ret;

	/* Reset runtime state before registering the interrupt. */
	g_button_irq_ready = 0;
	g_button_irq_count = 0;

	/* Translate the compile-time pin number into platform-specific GPIO settings. */
	gpio_ret = qosa_get_pin_default_cfg((qosa_uint8_t)BUTTON_DEMO_PIN_NUM, &g_button_pin_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		/* Stop initialization if the pin configuration cannot be resolved. */
		QLOGE("button demo get pin %u default cfg failed ret=%d",
			  (unsigned int)BUTTON_DEMO_PIN_NUM,
			  (int)gpio_ret);
		return;
	}

	/* Switch the pin mux to GPIO mode so the pin can be used as a button input. */
	pinctrl_ret = qosa_pin_set_func((qosa_pin_num_e)g_button_pin_cfg.pin_num, g_button_pin_cfg.gpio_func);
	if (pinctrl_ret != QOSA_PINCTRL_SUCCESS)
	{
		/* Abort if the pin cannot be configured for GPIO operation. */
		QLOGE("button demo set pin %u gpio func failed ret=%d",
			  (unsigned int)g_button_pin_cfg.pin_num,
			  (int)pinctrl_ret);
		return;
	}

	/* Prepare the interrupt configuration:
	 * - debounce is enabled to reduce mechanical switch bounce;
	 * - pull-up is enabled because the demo uses a falling-edge trigger;
	 * - user_ctx carries the resolved pin configuration into the callback.
	 */
	int_cfg.gpio_num = g_button_pin_cfg.gpio_num;
	int_cfg.gpio_debounce = QOSA_GPIO_DEBOUNCE_EN;
	int_cfg.gpio_pull = QOSA_GPIO_PULL_UP;
	int_cfg.interrupt_cb = button_demo_irq_callback;
	int_cfg.options = 1;
	int_cfg.user_ctx = &g_button_pin_cfg;

	/* Register the interrupt handler with the GPIO subsystem. */
	gpio_ret = qosa_interrupt_register(&int_cfg);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		/* Stop if interrupt registration fails. */
		QLOGE("button demo register irq failed pin=%u gpio=%d ret=%d",
			  (unsigned int)g_button_pin_cfg.pin_num,
			  (int)g_button_pin_cfg.gpio_num,
			  (int)gpio_ret);
		return;
	}

	/* Enable the interrupt on the falling edge so a press generates an event. */
	gpio_ret = qosa_interrupt_enable(g_button_pin_cfg.gpio_num, QOSA_GPIO_TRIGGER_FALLING_EDGE);
	if (gpio_ret != QOSA_GPIO_SUCCESS)
	{
		/* If enabling fails, unregister to keep the system state clean. */
		QLOGE("button demo enable irq failed pin=%u gpio=%d ret=%d",
			  (unsigned int)g_button_pin_cfg.pin_num,
			  (int)g_button_pin_cfg.gpio_num,
			  (int)gpio_ret);
		(void)qosa_interrupt_unregister(g_button_pin_cfg.gpio_num);
		return;
	}

	/* Mark the demo as ready only after both registration and enable succeed. */
	g_button_irq_ready = 1;

	/* Emit a summary log so it is clear which pin and trigger mode are active. */
	QLOGI("button irq demo ready: pin=%u gpio=%d trigger=falling-edge pull-up=on debounce=on",
		  (unsigned int)g_button_pin_cfg.pin_num,
		  (int)g_button_pin_cfg.gpio_num);
}

/* Register the button interrupt demo with the application startup framework. */
UNIRTOS_APP_EXPORT(200, "button_irq_demo", button_demo_init);
