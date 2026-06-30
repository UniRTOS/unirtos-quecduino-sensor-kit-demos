"""
@file      : led_demo.c
@author    : Lionel Zhang (lionel.zhang@example.com)
@brief     : UniRTOS Based on led Example
@version   : 0.1
@date      : 2026-06-25
@copyright : Copyright (c) 2026
"""
#include "qosa_sys.h"
#include "qosa_gpio.h"
#include "qosa_dev_eigen.h"
#include "qosa_def.h"
#include "qosa_log.h"
#include "qosa_pinctrl.h"
#include <stdlib.h>
#include <string.h>
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG   LOG_TAG_DEMO

#define UniRTOS_TEST_DEMO_TASK_STACK_SIZE 1024  // 任务栈大小 1KB

#define UniRTOS_TEST_DEMO_TASK_PRIO QOSA_PRIORITY_NORMAL // 普通任务优先级

static qosa_task_t g_quec_test_demo_task = QOSA_NULL;

#define LED_PIN_NUM 19

qosa_pin_cfg_t pin_cfg; // 保存 LED 引脚配置，初始化和设置电平均会使用

/*
    名称: unir_led_init
    说明: 初始化 LED 对应的 GPIO 引脚。
    @return 0 表示成功，1 表示失败
*/
static qosa_uint8_t unir_led_init(void)
{
    qosa_memset(&pin_cfg, 0, sizeof(qosa_pin_cfg_t));
    qosa_get_pin_default_cfg(LED_PIN_NUM, &pin_cfg);
    qosa_pin_set_func(LED_PIN_NUM, pin_cfg.gpio_func);
    // 将 LED GPIO 初始化为输出模式，上拉，默认高电平熄灭
    if (qosa_gpio_init(pin_cfg.gpio_num, QOSA_GPIO_DIRECTION_OUTPUT, QOSA_GPIO_PULL_UP, QOSA_GPIO_LEVEL_HIGH) != QOSA_GPIO_SUCCESS)
    {
        QLOGD("[TEST Demo]Failed to initialize LED GPIO");
        return 1; // 初始化失败
    }
    QLOGI("[TEST Demo]LED GPIO initialized successfully, pin_num: %d, gpio_num: %d, level: %d", LED_PIN_NUM, pin_cfg.gpio_num, QOSA_GPIO_LEVEL_HIGH);
    return 0;
}

/*
    名称: unir_led_set
    说明: 设置 LED GPIO 电平，用于控制亮灭。
    @param gpio_level: 目标电平，QOSA_GPIO_LEVEL_LOW 点亮 LED，QOSA_GPIO_LEVEL_HIGH 熄灭 LED。
    @return 0 表示成功，1 表示失败
*/
static qosa_uint8_t unir_led_set(qosa_gpio_level_e gpio_level)
{ 
    if(qosa_gpio_set_level(pin_cfg.gpio_num, gpio_level) != QOSA_GPIO_SUCCESS)
    {
        QLOGD("[TEST Demo]Failed to set LED GPIO level");
        return 1; // 设置失败
    }
    return 0;
}

/*
    名称: unir_test_demo_process
    说明: LED 示例任务入口，初始化 LED 后循环闪烁。
    @param ctx: 任务上下文指针，当前未使用，预留扩展。
    @return 无
*/
static void unir_test_demo_process(void *ctx)
{
    unir_led_init();
    while (1)
    {
        unir_led_set(QOSA_GPIO_LEVEL_LOW);
        QLOGI("[TEST Demo]LED ON");
        qosa_task_sleep_ms(1000);
        unir_led_set(QOSA_GPIO_LEVEL_HIGH);
        QLOGI("[TEST Demo]LED OFF");
        qosa_task_sleep_ms(1000);
    }
    
}

/*
    名称: unir_test_demo_init
    说明: 初始化 LED 示例，并创建后台任务。
    @param 无
*/
void unir_test_demo_init(void)
{
    // 打印 LED 示例初始化入口日志
    QLOGV("[TEST Demo]enter TEST DEMO !!!");

    // 创建 LED 示例任务，避免重复创建
    if (g_quec_test_demo_task == QOSA_NULL) // 检查任务是否已经创建
    {       
        
        qosa_task_create(
            &g_quec_test_demo_task,
            UniRTOS_TEST_DEMO_TASK_STACK_SIZE,     // 任务栈大小
            UniRTOS_TEST_DEMO_TASK_PRIO,           // 任务优先级
            "test_demo",                           // 任务名称
            unir_test_demo_process,                // 任务入口函数
            QOSA_NULL                             // 任务上下文，当前未使用
        );
    }
}
UNIRTOS_APP_EXPORT(700, "unir_led_demo", unir_test_demo_init);