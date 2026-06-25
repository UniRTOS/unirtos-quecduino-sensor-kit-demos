#include "qcm_proj_config.h"
#include "qosa_log.h"
#include "qosa_iic.h"
#include "qosa_pinctrl.h"
#include "unirtos_app_init_registry.h"

#define QOS_LOG_TAG LOG_TAG_DEMO

#define AHT20_I2C_ADDR_7BIT 0x38U
#define AHT20_STATUS_BUSY_MASK 0x80U

#define AHT20_CMD_RESET 0xBAU
#define AHT20_CMD_INIT 0xE1U
#define AHT20_CMD_MEASURE 0xACU

#define AHT20_RESET_DELAY_MS 20U
#define AHT20_INIT_DELAY_MS 1000U
#define AHT20_MEASURE_DELAY_MS 80U
#define AHT20_POLL_INTERVAL_MS 1000U
#define AHT20_THREAD_STACK_SIZE 2048U

#define AHT20_I2C_SDA_PIN QOSA_PIN_66
#define AHT20_I2C_SCL_PIN QOSA_PIN_67
#define AHT20_I2C_FUNC 2U
#define AHT20_I2C_CHANNEL QOSA_I2C_1


static qosa_task_t aht20_demo_thread_id;
static qosa_uint8_t aht20_slave_addr = AHT20_I2C_ADDR_7BIT;

static qosa_bool_t aht20_reset_sensor(void)
{
	qosa_uint8_t unused = 0U;
	qosa_i2c_error_e err;

	err = qosa_i2c_write(AHT20_I2C_CHANNEL, aht20_slave_addr, AHT20_CMD_RESET, &unused, 0U);
	if (err != QOSA_I2C_SUCCESS)
	{
		QLOGE("failed to reset AHT20, err=0x%x", err);
		return QOSA_FALSE;
	}

	qosa_task_sleep_ms(AHT20_RESET_DELAY_MS);
	return QOSA_TRUE;
}

static qosa_bool_t aht20_initialize_sensor(void)
{
	qosa_uint8_t unused = 0U;
	qosa_i2c_error_e err;

	err = qosa_i2c_write(AHT20_I2C_CHANNEL, aht20_slave_addr, AHT20_CMD_INIT, &unused, 0U);
	if (err != QOSA_I2C_SUCCESS)
	{
		QLOGE("failed to initialize AHT20, err=0x%x", err);
		return QOSA_FALSE;
	}

	return QOSA_TRUE;
}

static qosa_bool_t aht20_configure_bus(void)
{
	qosa_pinctrl_error_e pin_err;
	qosa_i2c_error_e i2c_err;

	pin_err = qosa_pin_set_func(AHT20_I2C_SDA_PIN, AHT20_I2C_FUNC);
	if (pin_err != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("failed to configure AHT20 SDA pin, err=0x%x", pin_err);
		return QOSA_FALSE;
	}

	pin_err = qosa_pin_set_func(AHT20_I2C_SCL_PIN, AHT20_I2C_FUNC);
	if (pin_err != QOSA_PINCTRL_SUCCESS)
	{
		QLOGE("failed to configure AHT20 SCL pin, err=0x%x", pin_err);
		return QOSA_FALSE;
	}

	i2c_err = qosa_i2c_init(AHT20_I2C_CHANNEL, QOSA_IIC_STANDARD_MODE);
	if (i2c_err != QOSA_I2C_SUCCESS)
	{
		QLOGE("failed to init AHT20 I2C bus, err=0x%x", i2c_err);
		return QOSA_FALSE;
	}

	return QOSA_TRUE;
}

static const char *aht20_check_comfort(qosa_int32_t temperature_tenths, qosa_uint32_t humidity_tenths)
{
	if (temperature_tenths < 180)
	{
		return "偏冷";
	}

	if (temperature_tenths > 280)
	{
		return "偏热";
	}

	if (humidity_tenths < 300U)
	{
		return "偏干燥";
	}

	if (humidity_tenths > 700U)
	{
		return "偏潮湿";
	}

	return "舒适";
}

static qosa_bool_t aht20_read_measurement(qosa_int32_t *temperature_tenths, qosa_uint32_t *humidity_tenths)
{
	qosa_uint8_t measure_payload[2] = {0x33U, 0x00U};
	qosa_uint8_t rx_buf[6] = {0};
	qosa_uint32_t humidity_raw;
	qosa_uint32_t temperature_raw;
	qosa_i2c_error_e err;

	err = qosa_i2c_write(AHT20_I2C_CHANNEL, aht20_slave_addr, AHT20_CMD_MEASURE, measure_payload, 2U);
	if (err != QOSA_I2C_SUCCESS)
	{
		QLOGE("failed to trigger AHT20 measurement, err=0x%x", err);
		return QOSA_FALSE;
	}

	qosa_task_sleep_ms(AHT20_MEASURE_DELAY_MS);

	err = qosa_i2c_read(AHT20_I2C_CHANNEL, aht20_slave_addr, 0x00U, rx_buf, sizeof(rx_buf));
	if (err != QOSA_I2C_SUCCESS)
	{
		QLOGE("failed to read AHT20 sample, err=0x%x", err);
		return QOSA_FALSE;
	}

	if ((rx_buf[0] & AHT20_STATUS_BUSY_MASK) != 0U)
	{
		QLOGW("AHT20 sample not ready, status=0x%02x", rx_buf[0]);
		return QOSA_FALSE;
	}

	humidity_raw = ((qosa_uint32_t)rx_buf[1] << 12) | ((qosa_uint32_t)rx_buf[2] << 4) | ((qosa_uint32_t)rx_buf[3] >> 4);
	temperature_raw = (((qosa_uint32_t)rx_buf[3] & 0x0FU) << 16) | ((qosa_uint32_t)rx_buf[4] << 8) | (qosa_uint32_t)rx_buf[5];

	*humidity_tenths = (humidity_raw * 1000U) >> 20;
	*temperature_tenths = (qosa_int32_t)(((temperature_raw * 2000U) >> 20) - 500);

	return QOSA_TRUE;
}

static void aht20_demo_thread(void *argument)
{
	qosa_int32_t temperature_tenths = 0;
	qosa_uint32_t humidity_tenths = 0;
	const char *comfort;

	(void)argument;

	if (!aht20_configure_bus())
	{
		return;
	}

	if (!aht20_reset_sensor())
	{
		(void)qosa_i2c_deinit(AHT20_I2C_CHANNEL);
		return;
	}

	if (!aht20_initialize_sensor())
	{
		(void)qosa_i2c_deinit(AHT20_I2C_CHANNEL);
		return;
	}

	qosa_task_sleep_ms(AHT20_INIT_DELAY_MS);

	QLOGI("AHT20 demo started on I2C channel %d slave 0x%02x", AHT20_I2C_CHANNEL, aht20_slave_addr);

	for (;;)
	{
		if (aht20_read_measurement(&temperature_tenths, &humidity_tenths))
		{
			comfort = aht20_check_comfort(temperature_tenths, humidity_tenths);
			QLOGI("温度: %d.%dC | 湿度: %u.%u%% | 状态: %s",
				  (int)(temperature_tenths / 10),
				  (int)((temperature_tenths < 0 ? -temperature_tenths : temperature_tenths) % 10),
				  (unsigned int)(humidity_tenths / 10),
				  (unsigned int)(humidity_tenths % 10),
				  comfort);
		}
		else
		{
			QLOGW("读取失败");
		}

		qosa_task_sleep_ms(AHT20_POLL_INTERVAL_MS);
	}
}

static void aht20_demo_init(void)
{
	int ret;

	if (aht20_demo_thread_id != NULL)
	{
		return;
	}

	ret = qosa_task_create(&aht20_demo_thread_id,
				       AHT20_THREAD_STACK_SIZE,
				       QOSA_PRIORITY_NORMAL,
				       "aht20_demo",
				       aht20_demo_thread,
				       NULL);
	if (ret != QOSA_ERROR_OK)
	{
		QLOGE("failed to create AHT20 demo thread, err=%d", ret);
		return;
	}

	QLOGI("AHT20 demo thread created");
}

UNIRTOS_APP_EXPORT(700, "aht20_demo", aht20_demo_init);
