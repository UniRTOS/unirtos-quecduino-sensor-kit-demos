/*
@file      : unir_rgb_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on RGB LED Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
*/
// QOSA 核心定义头文件
#include "qosa_def.h"
// QOSA 系统 API 头文件
#include "qosa_sys.h"
// QOSA GPIO 和引脚复用控制头文件
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
// QOSA 日志系统头文件
#include "qosa_log.h"

// 定义日志标签
#define QOS_LOG_TAG   LOG_TAG_DEMO

#include "unirtos_app_init_registry.h"

// 定义 RGB 控制任务栈大小
#define UNIR_RGB_DEMO_TASK_STACK_SIZE 1024
// 定义 RGB 控制任务优先级为普通优先级
#define UNIR_RGB_DEMO_TASK_PRIO QOSA_PRIORITY_NORMAL

// RGB 示例任务句柄，初始为空
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
    // 分别保存红、绿、蓝三个通道的引脚配置
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
        // 创建 RGB 控制任务，在任务中循环显示红、绿、蓝三色
        qosa_task_create(
            &g_unir_rgb_demo_task,
            UNIR_RGB_DEMO_TASK_STACK_SIZE,
            UNIR_RGB_DEMO_TASK_PRIO,
            "unir_rgb_demo",
            unir_rgb_demo_process,
            QOSA_NULL);
    }
}

// 将 RGB LED 示例注册到 UniRTOS 应用启动流程
UNIRTOS_APP_EXPORT(700, "unir_rgb_demo", unir_rgb_init);