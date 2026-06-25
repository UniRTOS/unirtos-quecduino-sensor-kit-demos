// QOSA core definition header
#include "qosa_def.h"
// Include QOSA system API header
#include "qosa_sys.h"
// Include QOSA GPIO and pinctrl headers
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
// Include QOSA log system header file
#include "qosa_log.h"

// Define log information
#define QOS_LOG_TAG   LOG_TAG_DEMO

#include "unirtos_app_init_registry.h"

// Define stack size of RGB control task
#define UNIR_RGB_DEMO_TASK_STACK_SIZE 1024
// Define RGB control task priority as normal
#define UNIR_RGB_DEMO_TASK_PRIO QOSA_PRIORITY_NORMAL

// Declare global task handle and initialize as NULL
static qosa_task_t g_unir_rgb_demo_task = QOSA_NULL;

// ========== 请根据实际电路修改下面的引脚号 ==========
#define RGB_RED_PIN   19    // 红色 LED 所接 GPIO 引脚
#define RGB_GREEN_PIN 20    // 绿色 LED 所接 GPIO 引脚
#define RGB_BLUE_PIN  21    // 蓝色 LED 所接 GPIO 引脚
// ====================================================

// 共阳极 RGB LED：低电平点亮，高电平熄灭
// 如果是共阴极，请将下面代码中的 QOSA_GPIO_LEVEL_LOW 与 HIGH 互换

static void unir_rgb_demo_process(void *ctx)
{
    qosa_pin_cfg_t red_pin_cfg;
    qosa_pin_cfg_t green_pin_cfg;
    qosa_pin_cfg_t blue_pin_cfg;

    (void)ctx;

    // 配置红引脚
    qosa_memset(&red_pin_cfg, 0, sizeof(red_pin_cfg));
    qosa_get_pin_default_cfg(RGB_RED_PIN, &red_pin_cfg);
    qosa_pin_set_func(RGB_RED_PIN, red_pin_cfg.gpio_func);
    qosa_gpio_init(red_pin_cfg.gpio_num, QOSA_GPIO_DIRECTION_OUTPUT, QOSA_GPIO_PULL_UP, QOSA_GPIO_LEVEL_HIGH);

    // 配置绿引脚
    qosa_memset(&green_pin_cfg, 0, sizeof(green_pin_cfg));
    qosa_get_pin_default_cfg(RGB_GREEN_PIN, &green_pin_cfg);
    qosa_pin_set_func(RGB_GREEN_PIN, green_pin_cfg.gpio_func);
    qosa_gpio_init(green_pin_cfg.gpio_num, QOSA_GPIO_DIRECTION_OUTPUT, QOSA_GPIO_PULL_UP, QOSA_GPIO_LEVEL_HIGH);

    // 配置蓝引脚
    qosa_memset(&blue_pin_cfg, 0, sizeof(blue_pin_cfg));
    qosa_get_pin_default_cfg(RGB_BLUE_PIN, &blue_pin_cfg);
    qosa_pin_set_func(RGB_BLUE_PIN, blue_pin_cfg.gpio_func);
    qosa_gpio_init(blue_pin_cfg.gpio_num, QOSA_GPIO_DIRECTION_OUTPUT, QOSA_GPIO_PULL_UP, QOSA_GPIO_LEVEL_HIGH);

    while (1)
    {
        // 红灯亮
        qosa_gpio_set_level(red_pin_cfg.gpio_num,   QOSA_GPIO_LEVEL_LOW);
        qosa_gpio_set_level(green_pin_cfg.gpio_num, QOSA_GPIO_LEVEL_HIGH);
        qosa_gpio_set_level(blue_pin_cfg.gpio_num,  QOSA_GPIO_LEVEL_HIGH);
        qosa_task_sleep_ms(1000);

        // 绿灯亮
        qosa_gpio_set_level(red_pin_cfg.gpio_num,   QOSA_GPIO_LEVEL_HIGH);
        qosa_gpio_set_level(green_pin_cfg.gpio_num, QOSA_GPIO_LEVEL_LOW);
        qosa_gpio_set_level(blue_pin_cfg.gpio_num,  QOSA_GPIO_LEVEL_HIGH);
        qosa_task_sleep_ms(1000);

        // 蓝灯亮
        qosa_gpio_set_level(red_pin_cfg.gpio_num,   QOSA_GPIO_LEVEL_HIGH);
        qosa_gpio_set_level(green_pin_cfg.gpio_num, QOSA_GPIO_LEVEL_HIGH);
        qosa_gpio_set_level(blue_pin_cfg.gpio_num,  QOSA_GPIO_LEVEL_LOW);
        qosa_task_sleep_ms(1000);
    }
}

void unir_rgb_init(void)
{
    QLOGV("enter rgb led demo !!!");
    if (g_unir_rgb_demo_task == QOSA_NULL)
    {
        qosa_task_create(
            &g_unir_rgb_demo_task,
            UNIR_RGB_DEMO_TASK_STACK_SIZE,
            UNIR_RGB_DEMO_TASK_PRIO,
            "unir_rgb_demo",
            unir_rgb_demo_process,
            QOSA_NULL);
    }
}

UNIRTOS_APP_EXPORT(700, "unir_rgb_demo", unir_rgb_init);