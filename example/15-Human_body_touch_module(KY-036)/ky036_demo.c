#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_sys.h"
#include "qosa_gpio.h"
#include "qosa_pinctrl.h"
#include "unirtos_app_init_registry.h"

/* 日志 TAG 定义 */
#define QOS_LOG_TAG LOG_TAG_DEMO

/*===========================================================================
 * 宏定义 - 用户可根据实际硬件连接修改引脚号
 ===========================================================================*/
/* KY-036 DO 引脚连接的模组 PIN 编号（数字输出引脚） */
#define KY036_DO_PIN_NUM        QOSA_PIN_31

/* 触摸触发电平：高电平表示检测到触摸 */
#define KY036_TRIGGER_LEVEL     QOSA_GPIO_LEVEL_HIGH

/* GPIO 上下拉配置 */
#define KY036_GPIO_PULL         QOSA_GPIO_PULL_DOWN

/* 触摸检测轮询间隔（毫秒） */
#define KY036_POLL_INTERVAL_MS  1000

/* 触摸检测任务栈大小 */
#define KY036_TASK_STACK_SIZE   (4 * 1024)

/* 触摸检测任务优先级 */
#define KY036_TASK_PRIORITY     100

/* 存储 KY036 DO 引脚对应的 GPIO 编号 */
static qosa_gpio_num_e g_ky036_gpio_num = 0;

/* 轮询任务句柄 */
static qosa_task_t g_ky036_task_ref = NULL;

/*===========================================================================
 * 传感器状态读取
 ===========================================================================*/
/**
 * @brief 读取 KY-036 当前数字电平
 *
 * @param[out] level 读取到的电平值
 *
 * @return 0 表示成功，-1 表示失败
 */
static int ky036_read_state(qosa_gpio_level_e *level)
{
    qosa_gpio_error_e ret = qosa_gpio_get_level(g_ky036_gpio_num, level);
    if (ret != QOSA_GPIO_SUCCESS)
    {
        QLOGE("KY036: gpio_get_level failed, ret=%d", ret);
        return -1;
    }

    return 0;
}

/**
 * @brief 判断当前是否检测到触摸
 *
 * @param[in] level 当前 GPIO 电平
 *
 * @return QOSA_TRUE 表示检测到触摸
 */
static qosa_bool_t ky036_is_touched(qosa_gpio_level_e level)
{
    return (level == KY036_TRIGGER_LEVEL) ? QOSA_TRUE : QOSA_FALSE;
}

/*===========================================================================
 * 轮询检测任务
 ===========================================================================*/
/**
 * @brief KY-036 轮询检测任务入口
 *
 * 周期性读取 GPIO 电平来判断是否有人体触摸。
 *
 * @param[in] argv  任务参数（未使用）
 */
static void ky036_poll_task(void *argv)
{
    (void)argv;
    qosa_gpio_level_e level = QOSA_GPIO_LEVEL_LOW;

    QLOGI("KY036: Poll task started, interval=%dms", KY036_POLL_INTERVAL_MS);

    while (1)
    {
        if (ky036_read_state(&level) != 0)
        {
            qosa_task_sleep_ms(KY036_POLL_INTERVAL_MS);
            continue;
        }

        if (ky036_is_touched(level) == QOSA_TRUE)
        {
            QLOGI("检测到触摸");
        }
        else
        {
            QLOGI("未检测到触摸");
        }

        qosa_task_sleep_ms(KY036_POLL_INTERVAL_MS);
    }
}

/*===========================================================================
 * 初始化函数
 ===========================================================================*/
/**
 * @brief KY-036 触摸传感器初始化
 *
 * 初始化流程：
 *   1. 获取引脚默认配置（pin_num -> gpio_num 映射）
 *   2. 设置引脚复用为 GPIO 功能
 *   3. 初始化 GPIO 为输入模式和默认上下拉
 *   4. 创建轮询任务，持续输出当前触摸状态
 */
static void ky036_demo_init(void)
{
    qosa_pin_cfg_t pin_cfg = {0};
    qosa_gpio_error_e gpio_ret;

    QLOGI("KY036: Touch sensor demo initializing...");
    QLOGI("KY036: DO pin = PIN_%d", KY036_DO_PIN_NUM);

    /*--- 步骤1：获取引脚默认配置 ---*/
    gpio_ret = qosa_get_pin_default_cfg(KY036_DO_PIN_NUM, &pin_cfg);
    if (gpio_ret != QOSA_GPIO_SUCCESS)
    {
        QLOGE("KY036: get_pin_default_cfg failed, ret=%d", gpio_ret);
        return;
    }

    /* 保存 GPIO 编号供后续使用 */
    g_ky036_gpio_num = pin_cfg.gpio_num;
    QLOGI("KY036: gpio_num=%d, gpio_func=%d", pin_cfg.gpio_num, pin_cfg.gpio_func);

    /*--- 步骤2：设置引脚复用为 GPIO 功能 ---*/
    qosa_pinctrl_error_e pin_ret = qosa_pin_set_func(pin_cfg.pin_num, pin_cfg.gpio_func);
    if (pin_ret != QOSA_PINCTRL_SUCCESS)
    {
        QLOGE("KY036: pin_set_func failed, ret=%d", pin_ret);
        return;
    }

    /*--- 步骤3：初始化 GPIO 为输入模式 ---*/
    gpio_ret = qosa_gpio_init(pin_cfg.gpio_num,
                              QOSA_GPIO_DIRECTION_INPUT,
                              KY036_GPIO_PULL,
                              QOSA_GPIO_LEVEL_LOW);
    if (gpio_ret != QOSA_GPIO_SUCCESS)
    {
        QLOGE("KY036: gpio_init failed, ret=%d", gpio_ret);
        return;
    }

    /*--- 步骤4：创建轮询任务 ---*/
    int task_ret = qosa_task_create(&g_ky036_task_ref,
                                    KY036_TASK_STACK_SIZE,
                                    KY036_TASK_PRIORITY,
                                    "ky036_poll",
                                    ky036_poll_task,
                                    NULL);
    if (task_ret != 0)
    {
        QLOGE("KY036: task_create failed, ret=%d", task_ret);
        return;
    }

    QLOGI("KY036: Touch sensor demo initialized successfully!");
    QLOGI("KY036: trigger_level=%d, pull=%d", KY036_TRIGGER_LEVEL, KY036_GPIO_PULL);
}

/*===========================================================================
 * 应用注册
 ===========================================================================*/
/* 使用 UNIRTOS_APP_EXPORT 将 demo 注册到系统启动流程中
 * 参数说明：
 *   200     - 启动优先级（数值越小越早执行，100为标准应用，200为普通demo）
 *   "ky036" - 应用名称标识
 *   ky036_demo_init - 初始化入口函数
 */
UNIRTOS_APP_EXPORT(200, "ky036", ky036_demo_init);
