#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
#include "qosa_sys.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* Default controller pins: GPIO31 input, GPIO30 linked output */
#define KY021_SENSOR_PIN QOSA_PIN_23
#define KY021_OUTPUT_PIN QOSA_PIN_22

/* LOW means magnetic field detected, HIGH means released */
#define KY021_TRIGGER_LEVEL       QOSA_GPIO_LEVEL_LOW
#define KY021_OUTPUT_ACTIVE_LEVEL QOSA_GPIO_LEVEL_HIGH

/* Polling interval in milliseconds */
#define KY021_POLL_INTERVAL_MS 500

/* Task parameters */
#define KY021_TASK_STACK_SIZE 4096
#define KY021_TASK_PRIORITY   100
#define KY021_TASK_NAME       "ky021_task"

static qosa_task_t ky021_task_ref = NULL;
static qosa_pin_cfg_t ky021_sensor_pin_cfg;
static qosa_pin_cfg_t ky021_output_pin_cfg;

/**
 * @brief Convert active output level to its inactive counterpart
 */
static qosa_gpio_level_e ky021_get_inactive_level(qosa_gpio_level_e active_level)
{
    return (active_level == QOSA_GPIO_LEVEL_HIGH) ? QOSA_GPIO_LEVEL_LOW : QOSA_GPIO_LEVEL_HIGH;
}

/**
 * @brief Return whether current sensor level matches trigger level
 */
static qosa_bool_t ky021_is_triggered(qosa_gpio_level_e level)
{
    return (level == KY021_TRIGGER_LEVEL) ? QOSA_TRUE : QOSA_FALSE;
}

/**
 * @brief Drive linked output according to trigger state
 */
static void ky021_set_output(qosa_bool_t active)
{
    qosa_gpio_level_e output_level = ky021_get_inactive_level(KY021_OUTPUT_ACTIVE_LEVEL);

    if (active == QOSA_TRUE)
    {
        output_level = KY021_OUTPUT_ACTIVE_LEVEL;
    }

    (void)qosa_gpio_set_level(ky021_output_pin_cfg.gpio_num, output_level);
}

/**
 * @brief Initialize one GPIO pin using its default mux configuration
 */
static int ky021_gpio_init(qosa_pin_num_e pin_num,
                           qosa_gpio_direction_e direction,
                           qosa_gpio_pull_e pull,
                           qosa_gpio_level_e init_level,
                           qosa_pin_cfg_t *pin_cfg)
{
    qosa_gpio_error_e gpio_ret;
    qosa_pinctrl_error_e pinctrl_ret;

    gpio_ret = qosa_get_pin_default_cfg(pin_num, pin_cfg);
    if (gpio_ret != QOSA_GPIO_SUCCESS)
    {
        QLOGE("get pin %d default cfg failed, ret=%d", pin_num, gpio_ret);
        return -1;
    }

    pinctrl_ret = qosa_pin_set_func(pin_cfg->pin_num, pin_cfg->gpio_func);
    if (pinctrl_ret != QOSA_PINCTRL_SUCCESS)
    {
        QLOGE("set pin %d gpio func failed, ret=%d", pin_cfg->pin_num, pinctrl_ret);
        return -1;
    }

    gpio_ret = qosa_gpio_init(pin_cfg->gpio_num, direction, pull, init_level);
    if (gpio_ret != QOSA_GPIO_SUCCESS)
    {
        QLOGE("init gpio %d failed, ret=%d", pin_cfg->gpio_num, gpio_ret);
        return -1;
    }

    return 0;
}

/**
 * @brief KY-021 sensor polling task with linked output control
 */
static void ky021_task_entry(void *argv)
{
    qosa_gpio_level_e level = QOSA_GPIO_LEVEL_HIGH;
    qosa_gpio_level_e last_level = QOSA_GPIO_LEVEL_HIGH;
    qosa_bool_t triggered;

    (void)argv;

    qosa_gpio_get_level(ky021_sensor_pin_cfg.gpio_num, &last_level);
    QLOGI("KY-021 mini magnetic controller started");

    while (1)
    {
        qosa_gpio_get_level(ky021_sensor_pin_cfg.gpio_num, &level);
        triggered = ky021_is_triggered(level);

        ky021_set_output(triggered);

        if (triggered == QOSA_TRUE)
        {
            QLOGI("检测到磁场变化");
        }
        else
        {
            QLOGI("未检测到磁场变化");
        }

        if (level != last_level)
        {
            if (triggered == QOSA_TRUE)
            {
                QLOGI("[MiniMagnetic] 触发事件");
            }
            else
            {
                QLOGI("[MiniMagnetic] 释放事件");
            }
            last_level = level;
        }

        qosa_task_sleep_ms(KY021_POLL_INTERVAL_MS);
    }
}

/**
 * @brief Initialize sensor/input GPIO and linked output GPIO
 */
static void ky021_demo_init(void)
{
    int task_ret;
    qosa_gpio_level_e output_inactive_level = ky021_get_inactive_level(KY021_OUTPUT_ACTIVE_LEVEL);

    QLOGI("KY-021 demo initializing...");

    if (ky021_task_ref != NULL)
    {
        QLOGW("KY-021 demo already started");
        return;
    }

    if (ky021_gpio_init(KY021_SENSOR_PIN,
                        QOSA_GPIO_DIRECTION_INPUT,
                        QOSA_GPIO_PULL_UP,
                        QOSA_GPIO_LEVEL_LOW,
                        &ky021_sensor_pin_cfg) != 0)
    {
        return;
    }

    if (ky021_gpio_init(KY021_OUTPUT_PIN,
                        QOSA_GPIO_DIRECTION_OUTPUT,
                        QOSA_GPIO_PULL_NONE,
                        output_inactive_level,
                        &ky021_output_pin_cfg) != 0)
    {
        return;
    }

    task_ret = qosa_task_create(&ky021_task_ref,
                                KY021_TASK_STACK_SIZE,
                                KY021_TASK_PRIORITY,
                                KY021_TASK_NAME,
                                ky021_task_entry,
                                NULL);
    if (task_ret != 0)
    {
        QLOGE("Failed to create KY-021 task, error=%d", task_ret);
        return;
    }

    QLOGI("KY-021 demo initialized successfully, sensor_pin=%d output_pin=%d",
          ky021_sensor_pin_cfg.pin_num,
          ky021_output_pin_cfg.pin_num);
}

UNIRTOS_APP_EXPORT(200, "ky021_demo", ky021_demo_init);
