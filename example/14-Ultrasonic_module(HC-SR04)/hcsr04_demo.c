"""
@file      : hcsr04_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on HC-SR04 Ultrasonic Module Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
"""
#include "qosa_def.h"
#include "qosa_sys.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
#include "qosa_log.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

/* HC-SR04 超声波测距任务参数。 */
#define HCSR04_TASK_STACK_SIZE 2048
#define HCSR04_TASK_PRIO QOSA_PRIORITY_NORMAL
#define HCSR04_MEASUREMENT_INTERVAL_MS 200

#define HC_SR04_TRIG_PIN_NUM 30
#define HC_SR04_ECHO_PIN_NUM 31

#define HCSR04_TRIGGER_PRE_DELAY_US 2U
#define HCSR04_TRIGGER_PULSE_WIDTH_US 10U
#define HCSR04_ECHO_START_TIMEOUT_US 30000U
#define HCSR04_ECHO_END_TIMEOUT_US 500000U
#define HCSR04_FILTER_SIZE 5U
#define HCSR04_VALID_MIN_CM 2.0f
#define HCSR04_VALID_MAX_CM 800.0f
#define HCSR04_DISTANCE_DIVISOR 58.0f

static qosa_task_t g_hcsr04_demo_task = QOSA_NULL;
static qosa_pin_cfg_t g_trig_pin_cfg;
static qosa_pin_cfg_t g_echo_pin_cfg;
static float g_distance_history[HCSR04_FILTER_SIZE];
static qosa_uint32_t g_distance_history_count;
static qosa_uint32_t g_distance_history_index;

/* 简单忙等延时，用于产生微秒级触发脉冲和回波计时。 */
static void hcsr04_delay_us(qosa_uint32_t us)
{
    volatile qosa_uint32_t count;

    for (count = 0; count < us * 6U; ++count)
    {
        __NOP();
    }
}

/* 读取 ECHO 引脚当前电平。 */
static qosa_uint8_t hcsr04_read_echo_level(qosa_gpio_level_e *level)
{
    if (qosa_gpio_get_level(g_echo_pin_cfg.gpio_num, level) != QOSA_GPIO_SUCCESS)
    {
        QLOGV("[HC-SR04] failed to read echo level");
        return 1;
    }

    return 0;
}

/* 初始化 TRIG 输出和 ECHO 输入 GPIO。 */
static qosa_uint8_t hcsr04_gpio_init(void)
{
    qosa_memset(&g_trig_pin_cfg, 0, sizeof(g_trig_pin_cfg));
    qosa_get_pin_default_cfg(HC_SR04_TRIG_PIN_NUM, &g_trig_pin_cfg);
    qosa_pin_set_func(HC_SR04_TRIG_PIN_NUM, g_trig_pin_cfg.gpio_func);

    if (qosa_gpio_init(g_trig_pin_cfg.gpio_num,
                       QOSA_GPIO_DIRECTION_OUTPUT,
                       QOSA_GPIO_PULL_DOWN,
                       QOSA_GPIO_LEVEL_LOW) != QOSA_GPIO_SUCCESS)
    {
        QLOGV("[HC-SR04] failed to initialize TRIG pin");
        return 1;
    }

    qosa_memset(&g_echo_pin_cfg, 0, sizeof(g_echo_pin_cfg));
    qosa_get_pin_default_cfg(HC_SR04_ECHO_PIN_NUM, &g_echo_pin_cfg);
    qosa_pin_set_func(HC_SR04_ECHO_PIN_NUM, g_echo_pin_cfg.gpio_func);

    if (qosa_gpio_init(g_echo_pin_cfg.gpio_num,
                       QOSA_GPIO_DIRECTION_INPUT,
                       QOSA_GPIO_PULL_DOWN,
                       QOSA_GPIO_LEVEL_LOW) != QOSA_GPIO_SUCCESS)
    {
        QLOGV("[HC-SR04] failed to initialize ECHO pin");
        return 1;
    }

    QLOGV("[HC-SR04] init done, trig pin=%d gpio=%d, echo pin=%d gpio=%d",
          HC_SR04_TRIG_PIN_NUM,
          g_trig_pin_cfg.gpio_num,
          HC_SR04_ECHO_PIN_NUM,
          g_echo_pin_cfg.gpio_num);
    return 0;
}

/* 向 TRIG 引脚输出一次测距触发脉冲。 */
static void hcsr04_trigger(void)
{
    qosa_gpio_set_level(g_trig_pin_cfg.gpio_num, QOSA_GPIO_LEVEL_LOW);
    hcsr04_delay_us(HCSR04_TRIGGER_PRE_DELAY_US);
    qosa_gpio_set_level(g_trig_pin_cfg.gpio_num, QOSA_GPIO_LEVEL_HIGH);
    hcsr04_delay_us(HCSR04_TRIGGER_PULSE_WIDTH_US);
    qosa_gpio_set_level(g_trig_pin_cfg.gpio_num, QOSA_GPIO_LEVEL_LOW);
}

/* 完成一次超声波测距，并将回波高电平时间换算为厘米。 */
static qosa_uint8_t hcsr04_read_distance(float *distance_cm)
{
    qosa_gpio_level_e echo_level = QOSA_GPIO_LEVEL_LOW;
    qosa_uint32_t wait_us = 0;
    qosa_uint32_t duration_us = 0;

    if (distance_cm == QOSA_NULL)
    {
        return 1;
    }

    hcsr04_trigger();

    while (wait_us < HCSR04_ECHO_START_TIMEOUT_US)
    {
        if (hcsr04_read_echo_level(&echo_level) != 0)
        {
            return 1;
        }

        if (echo_level == QOSA_GPIO_LEVEL_HIGH)
        {
            break;
        }

        hcsr04_delay_us(1U);
        ++wait_us;
    }

    if (wait_us >= HCSR04_ECHO_START_TIMEOUT_US)
    {
        QLOGV("[HC-SR04] wait echo start timeout");
        return 1;
    }

    while (duration_us < HCSR04_ECHO_END_TIMEOUT_US)
    {
        if (hcsr04_read_echo_level(&echo_level) != 0)
        {
            return 1;
        }

        if (echo_level != QOSA_GPIO_LEVEL_HIGH)
        {
            break;
        }

        hcsr04_delay_us(1U);
        ++duration_us;
    }

    if (duration_us >= HCSR04_ECHO_END_TIMEOUT_US)
    {
        QLOGV("[HC-SR04] echo high timeout");
        return 1;
    }

    *distance_cm = (float)duration_us / HCSR04_DISTANCE_DIVISOR;
    return 0;
}

/* 读取并过滤距离值，使用滑动平均降低测量抖动。 */
static qosa_uint8_t hcsr04_read_filtered_distance(float *distance_cm)
{
    float raw_distance_cm = 0.0f;
    float distance_sum_cm = 0.0f;
    qosa_uint32_t sample_index;

    if (distance_cm == QOSA_NULL)
    {
        return 1;
    }

    if (hcsr04_read_distance(&raw_distance_cm) != 0)
    {
        return 1;
    }

    if (raw_distance_cm < HCSR04_VALID_MIN_CM || raw_distance_cm > HCSR04_VALID_MAX_CM)
    {
        QLOGV("[HC-SR04] invalid distance: %.2f cm", raw_distance_cm);
        return 1;
    }

    g_distance_history[g_distance_history_index] = raw_distance_cm;
    g_distance_history_index = (g_distance_history_index + 1U) % HCSR04_FILTER_SIZE;
    if (g_distance_history_count < HCSR04_FILTER_SIZE)
    {
        ++g_distance_history_count;
    }

    for (sample_index = 0; sample_index < g_distance_history_count; ++sample_index)
    {
        distance_sum_cm += g_distance_history[sample_index];
    }

    *distance_cm = distance_sum_cm / (float)g_distance_history_count;

    return 0;
}

/* HC-SR04 测距任务，周期输出过滤后的距离。 */
static void hcsr04_demo_process(void *ctx)
{
    float distance_cm = 0.0f;

    (void)ctx;

    if (hcsr04_gpio_init() != 0)
    {
        QLOGV("[HC-SR04] gpio init failed, task exit");
        return;
    }

    QLOGV("[HC-SR04] demo task started");

    while (1)
    {
        if (hcsr04_read_filtered_distance(&distance_cm) == 0)
        {
            QLOGV("[HC-SR04] distance: %.2f cm", distance_cm);
        }
        else
        {
            QLOGV("[HC-SR04] measurement failed");
        }

        qosa_task_sleep_ms(HCSR04_MEASUREMENT_INTERVAL_MS);
    }
}

/* HC-SR04 示例初始化入口，负责创建测距任务。 */
void hcsr04_demo_init(void)
{
    QLOGV("[HC-SR04] enter demo init");

    if (g_hcsr04_demo_task == QOSA_NULL)
    {
        qosa_task_create(
            &g_hcsr04_demo_task,
            HCSR04_TASK_STACK_SIZE,
            HCSR04_TASK_PRIO,
            "hcsr04_demo",
            hcsr04_demo_process,
            QOSA_NULL);
    }
}

/* 将 HC-SR04 超声波测距示例注册到 UniRTOS 应用启动流程。 */
UNIRTOS_APP_EXPORT(700, "hcsr04_demo", hcsr04_demo_init);