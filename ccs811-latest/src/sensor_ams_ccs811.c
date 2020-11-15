/*
 * Copyright (c) 2020, RudyLo <luhuadong@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-06-21     luhuadong    the first version
 */
#include "rtthread.h"
#include "ls1c.h"
#include <stdlib.h>  
#include "../libraries/ls1c_delay.h"
#include "../libraries/ls1c_clock.h"



#include <board.h>
#include "ccs811.h"

#define DBG_TAG                        "sensor.ams.ccs811"
#ifdef PKG_USING_CCS811_DEBUG
#define DBG_LVL                        DBG_LOG
#else
#define DBG_LVL                        DBG_ERROR
#endif
#include <rtdbg.h>

/* range */
#define SENSOR_ECO2_RANGE_MIN          (400)
#define SENSOR_ECO2_RANGE_MAX          (29206)
#define SENSOR_TVOC_RANGE_MIN          (0)
#define SENSOR_TVOC_RANGE_MAX          (32768)

/* minial period (ms) */
#define SENSOR_PERIOD_MIN              (250)
#define SENSOR_ECO2_PERIOD_MIN         SENSOR_PERIOD_MIN
#define SENSOR_TVOC_PERIOD_MIN         SENSOR_PERIOD_MIN

/* fifo max length */
#define SENSOR_FIFO_MAX                (1)
#define SENSOR_ECO2_FIFO_MAX           SENSOR_FIFO_MAX
#define SENSOR_TVOC_FIFO_MAX           SENSOR_FIFO_MAX

static rt_bool_t 
read_word_from_command(struct rt_i2c_bus_device *bus,
                       rt_uint8_t                cmd[], 
                       rt_uint8_t                cmdlen, 
                       rt_uint16_t               delayms, 
                       rt_uint8_t               *readdata, 
                       rt_uint8_t                readlen)
{
    /* Request */
    rt_i2c_master_send(bus, CCS811_I2C_ADDRESS, RT_I2C_WR, cmd, cmdlen);

    rt_thread_mdelay(delayms);

    /* If not need reply */
    if (readlen == 0) return RT_TRUE;

    /* Response */
    if (rt_i2c_master_recv(bus, CCS811_I2C_ADDRESS, RT_I2C_RD, readdata, readlen) != readlen)
        return RT_FALSE;

    return RT_TRUE;
}

static rt_err_t _ccs811_measure(struct rt_i2c_bus_device *i2c_bus, rt_uint16_t reply[], const rt_size_t len)
{
    RT_ASSERT(reply);

    if (len < 2)
        return -RT_ERROR;

    rt_uint8_t cmd[1] = {CCS811_REG_ALG_RESULT_DATA};
    rt_uint8_t buffer[8] = {0};
    
    if (!read_word_from_command(i2c_bus, cmd, 1, 10, buffer, 8))
        return -RT_ERROR;

    reply[0] = (((rt_uint16_t)buffer[0] << 8) | (rt_uint16_t)buffer[1]);  /* eCO2 */
    reply[1] = (((rt_uint16_t)buffer[2] << 8) | (rt_uint16_t)buffer[3]);  /* TVOC */

    return RT_EOK;
}

static rt_err_t _ccs811_get_baseline(struct rt_i2c_bus_device *i2c_bus, void *args)
{
    rt_uint16_t *baseline = (rt_uint16_t *)args;

    rt_uint8_t cmd[1] = {CCS811_REG_BASELINE};
    rt_uint8_t reply[2];
    
    if (!read_word_from_command(i2c_bus, cmd, 1, 10, reply, 2))
        return -RT_ERROR;

    *baseline = reply[0] << 8 | reply[1];

    return RT_EOK;
}

static rt_err_t _ccs811_set_baseline(struct rt_i2c_bus_device *i2c_bus, void *args)
{
    rt_uint16_t *baseline = (rt_uint16_t *)args;
    rt_uint8_t cmd[3];

    cmd[0] = CCS811_REG_BASELINE;
    cmd[1] = *baseline >> 8;
    cmd[2] = *baseline;

    if (!read_word_from_command(i2c_bus, cmd, 3, 10, RT_NULL, 0))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t _ccs811_set_envdata(struct rt_i2c_bus_device *i2c_bus, void *args)
{
    struct ccs811_envdata *envdata = (struct ccs811_envdata *)args;

    rt_uint8_t cmd[5] = {0};
    int _temp, _rh;

    if (envdata->temperature > 0)
        _temp = (int)envdata->temperature + 0.5;  // this will round off the floating point to the nearest integer value
    else if (envdata->temperature < 0) // account for negative temperatures
        _temp = (int)envdata->temperature - 0.5;
    
    _temp = _temp + 25;  // temperature high byte is stored as T+25°C in the sensor's memory so the value of byte is positive
    _rh = (int)envdata->humidity + 0.5;  // this will round off the floating point to the nearest integer value
    
    cmd[0] = CCS811_REG_ENV_DATA;
    cmd[1] = _rh << 1;  // shift the binary number to left by 1. This is stored as a 7-bit value
    cmd[2] = 0;  // most significant fractional bit. Using 0 here - gives us accuracy of +/-1%. Current firmware (2016) only supports fractional increments of 0.5
    cmd[3] = _temp << 1;
    cmd[4] = 0;

    if (!read_word_from_command(i2c_bus, cmd, 5, 10, RT_NULL, 0))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t _ccs811_set_measure_cycle(struct rt_i2c_bus_device *i2c_bus, void *args)
{
    ccs811_cycle_t *cycle = (ccs811_cycle_t *)args;
    rt_uint8_t cmd[2] = {0};

    cmd[0] = CCS811_REG_MEAS_MODE;
    cmd[1] = *cycle << 4;

    if (!read_word_from_command(i2c_bus, cmd, 2, 10, RT_NULL, 0))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t _ccs811_set_measure_mode(struct rt_i2c_bus_device *i2c_bus, void *args)
{
    struct ccs811_meas_mode *meas = (struct ccs811_meas_mode *)args;
    rt_uint8_t cmd[2] = {0};

    cmd[0] = CCS811_REG_MEAS_MODE;
    cmd[1] = (meas->thresh << 2) | (meas->interrupt << 3) | (meas->mode << 4);

    if (!read_word_from_command(i2c_bus, cmd, 2, 10, RT_NULL, 0))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_err_t _ccs811_get_measure_mode(struct rt_i2c_bus_device *i2c_bus, void *args)
{
    struct ccs811_meas_mode *meas = (struct ccs811_meas_mode *)args;
    rt_uint8_t cmd[1] = {0};
    rt_uint8_t measurement = 0;

    cmd[0] = CCS811_REG_MEAS_MODE;

    if (!read_word_from_command(i2c_bus, cmd, 1, 0, &measurement, 1))
        return -RT_ERROR;

    meas->thresh = (measurement & 0x04) >> 2;
    meas->thresh = (measurement & 0x08) >> 3;
    meas->thresh = (measurement & 0x70) >> 4;

    return RT_EOK;
}

static rt_err_t _ccs811_set_thresholds(struct rt_i2c_bus_device *i2c_bus, void *args)
{
    struct ccs811_thresholds *thresholds = (struct ccs811_thresholds *)args;
    rt_uint8_t cmd[5] = {0};

    cmd[0] = CCS811_REG_THRESHOLDS;
    cmd[1] = (rt_uint8_t)((thresholds->low_to_med >> 8) & 0xF);
    cmd[2] = (rt_uint8_t)( thresholds->low_to_med & 0xF);
    cmd[3] = (rt_uint8_t)((thresholds->med_to_high >> 8) & 0xF);
    cmd[4] = (rt_uint8_t)( thresholds->med_to_high & 0xF);

    if (!read_word_from_command(i2c_bus, cmd, 5, 10, RT_NULL, 0))
        return -RT_ERROR;

    return RT_EOK;
}

static rt_size_t _ccs811_polling_get_data(struct rt_sensor_device *sensor, void *buf)
{
    struct rt_sensor_data *sensor_data = buf;
    struct rt_i2c_bus_device *i2c_bus = (struct rt_i2c_bus_device *)sensor->config.intf.user_data;

    rt_uint16_t measure_data[2] = {0};
    if (RT_EOK != _ccs811_measure(i2c_bus, measure_data, 2))
    {
        LOG_E("Can not read from %s", sensor->info.model);
        return 0;
    }
    rt_uint32_t timestamp = rt_sensor_get_ts();
    rt_uint32_t eco2 = measure_data[0];
    rt_uint32_t tvoc = measure_data[1];

    if (eco2 < SENSOR_ECO2_RANGE_MIN || eco2 > SENSOR_ECO2_RANGE_MAX || 
        tvoc < SENSOR_TVOC_RANGE_MIN || tvoc > SENSOR_TVOC_RANGE_MAX )
    {
        LOG_D("Data out of range");
        return 0;
    }

    if (sensor->info.type == RT_SENSOR_CLASS_ECO2)
    {
        sensor_data->type = RT_SENSOR_CLASS_ECO2;
        sensor_data->data.eco2 = eco2;
        sensor_data->timestamp = timestamp;

        struct rt_sensor_data *partner_data = (struct rt_sensor_data *)sensor->module->sen[1]->data_buf;
        if (partner_data)
        {
            partner_data->type = RT_SENSOR_CLASS_TVOC;
            partner_data->data.tvoc = tvoc;
            partner_data->timestamp = timestamp;
            sensor->module->sen[1]->data_len = sizeof(struct rt_sensor_data);
        }
    }
    else if (sensor->info.type == RT_SENSOR_CLASS_TVOC)
    {
        sensor_data->type = RT_SENSOR_CLASS_TVOC;
        sensor_data->data.tvoc = tvoc;
        sensor_data->timestamp = timestamp;

        struct rt_sensor_data *partner_data = (struct rt_sensor_data *)sensor->module->sen[0]->data_buf;
        if (partner_data)
        {
            partner_data->type = RT_SENSOR_CLASS_ECO2;
            partner_data->data.eco2 = eco2;
            partner_data->timestamp = timestamp;
            sensor->module->sen[0]->data_len = sizeof(struct rt_sensor_data);
        }
    }

    return 1;
}

static rt_size_t ccs811_fetch_data(struct rt_sensor_device *sensor, void *buf, rt_size_t len)
{
    if (sensor->config.mode == RT_SENSOR_MODE_POLLING)
    {
        return _ccs811_polling_get_data(sensor, buf);
    }
    else
        return 0;
}

static rt_err_t ccs811_control(struct rt_sensor_device *sensor, int cmd, void *args)
{
    rt_err_t result = RT_EOK;
    struct rt_i2c_bus_device *i2c_bus = (struct rt_i2c_bus_device *)sensor->config.intf.user_data;

    switch (cmd)
    {
    case RT_SENSOR_CTRL_GET_ID:
        if (args)
        {
            rt_uint8_t *hwid = (rt_uint8_t *)args;
            *hwid = CCS811_HW_ID;
            result = RT_EOK;
        }
        break;
    case RT_SENSOR_CTRL_SET_MODE:
        sensor->config.mode = (rt_uint32_t)args & 0xFF;
        break;
    case RT_SENSOR_CTRL_SET_RANGE:
        break;
    case RT_SENSOR_CTRL_SET_ODR:
        break;
    case RT_SENSOR_CTRL_SET_POWER:
        break;
    case RT_SENSOR_CTRL_SELF_TEST:
        break;
    case RT_SENSOR_CTRL_GET_BASELINE:
        LOG_D("Custom command : Get baseline");
        if (args)
        {
            result = _ccs811_get_baseline(i2c_bus, args);
        }
        break;
    case RT_SENSOR_CTRL_SET_BASELINE:
        LOG_D("Custom command : Set baseline");
        if (args)
        {
            result = _ccs811_set_baseline(i2c_bus, args);
        }
        break;
    case RT_SENSOR_CTRL_SET_ENVDATA:
        LOG_D("Custom command : Set env data");
        if (args)
        {
            result = _ccs811_set_envdata(i2c_bus, args);
        }
        break;
    case RT_SENSOR_CTRL_GET_MEAS_MODE:
        LOG_D("Custom command : Get measure mode");
        if (args)
        {
            result = _ccs811_get_measure_mode(i2c_bus, args);
        }
        break;
    case RT_SENSOR_CTRL_SET_MEAS_MODE:
        LOG_D("Custom command : Set measure mode");
        if (args)
        {
            result = _ccs811_set_measure_mode(i2c_bus, args);
        }
        break;
    case RT_SENSOR_CTRL_SET_MEAS_CYCLE:
        LOG_D("Custom command : Set measure cycle");
        if (args)
        {
            result = _ccs811_set_measure_cycle(i2c_bus, args);
        }
        break;
    case RT_SENSOR_CTRL_SET_THRESHOLDS:
        LOG_D("Custom command : Set thresholds");
        if (args)
        {
            result = _ccs811_set_thresholds(i2c_bus, args);
        }
        break;
    default:
        return -RT_ERROR;
        break;
    }

    return result;
}

static struct rt_sensor_ops sensor_ops =
{
    ccs811_fetch_data,
    ccs811_control
};

/*!
 *  @brief  Setups the hardware and detects a valid CCS811. Initializes I2C
 *          then reads the serialnumber and checks that we are talking to an
 *          CCS811. Commands the sensor to begin the IAQ algorithm. Must be 
 *          called after startup.
 *  @param  dev
 *          The pointer to I2C device
 *  @return RT_EOK if CCS811 found on I2C and command completed successfully, 
 *          -RT_ERROR if something went wrong!
 */
static rt_err_t _sensor_init(struct rt_i2c_bus_device *i2c_bus)
{
    rt_uint8_t cmd[5] = {0};
    //rt_uint8_t hardware_id = 0x81;
    rt_uint8_t hardware_id = 0;

    /* Soft reset */
    cmd[0] = CCS811_REG_SW_RESET;
    cmd[1] = 0x11;
    cmd[2] = 0xE5;
    cmd[3] = 0x72;
    cmd[4] = 0x8A;
    if (!read_word_from_command(i2c_bus, cmd, 5, 10, RT_NULL, 0))
    {
        return -RT_ERROR;
    }

    /* Get sensor id */
    cmd[0] = CCS811_REG_HW_ID;
    if (!read_word_from_command(i2c_bus, cmd, 1, 10, &hardware_id, 1))
        return -RT_ERROR;

    if (hardware_id != CCS811_HW_ID)
    {
        LOG_E("sensor hardware id not 0x%x", CCS811_HW_ID);
        return -RT_ERROR;
    }

    /* Start app */
    //add delay
    delay_us(200);

    cmd[0] = CCS811_BOOTLOADER_APP_START;
    if (!read_word_from_command(i2c_bus, cmd, 1, 10, RT_NULL, 0))
        return -RT_ERROR;
    
    /* Set measurement mode */
    //setMeasurementMode(0,0,eMode4);
    // struct ccs811_meas_mode meas = {0, 0, CCS811_MODE_4};
    struct ccs811_meas_mode meas = { 0, 0, CCS811_MODE_1 };
    _ccs811_set_measure_mode(i2c_bus, &meas);

    /* Set env data */
    //setInTempHum(25, 50);
    struct ccs811_envdata envdata = {23, 50};
   // struct ccs811_envdata envdata = { 25, 50 };
    _ccs811_set_envdata(i2c_bus, &envdata);

    return RT_EOK;
}

/**
 * This function will init dhtxx sensor device.
 *
 * @param intf  interface 
 *
 * @return RT_EOK
 */
static rt_err_t _ccs811_init(struct rt_sensor_intf *intf)
{
    if (intf->type == RT_SENSOR_INTF_I2C)
    {
        intf->user_data = (void *)rt_i2c_bus_device_find(intf->dev_name);
        if (intf->user_data == RT_NULL)
        {
            LOG_E("Can't find ccs811 device on '%s' ", intf->dev_name);
            return -RT_ERROR;
        }
    }
    return _sensor_init((struct rt_i2c_bus_device *)intf->user_data);
}

/**
 * Call function rt_hw_ccs811_init for initial and register a dhtxx sensor.
 *
 * @param name  the name will be register into device framework
 * @param cfg   sensor config
 *
 * @return the result
 */
rt_err_t rt_hw_ccs811_init(const char *name, struct rt_sensor_config *cfg)
{
    int result;
    rt_sensor_t sensor_tvoc = RT_NULL;
    rt_sensor_t sensor_eco2 = RT_NULL;
    struct rt_sensor_module *module = RT_NULL;

    if (_ccs811_init(&cfg->intf) != RT_EOK)
    {
        return -RT_ERROR;
    }
    
    module = rt_calloc(1, sizeof(struct rt_sensor_module));
    if (module == RT_NULL)
    {
        return -RT_ENOMEM;
    }

    /* eCO2 sensor register */
    {
        sensor_eco2 = rt_calloc(1, sizeof(struct rt_sensor_device));
        if (sensor_eco2 == RT_NULL)
            goto __exit;

        sensor_eco2->info.type       = RT_SENSOR_CLASS_ECO2;
        sensor_eco2->info.vendor     = RT_SENSOR_VENDOR_AMS;
        sensor_eco2->info.model      = "ccs811";
        sensor_eco2->info.unit       = RT_SENSOR_UNIT_PPM;
        sensor_eco2->info.intf_type  = RT_SENSOR_INTF_I2C;
        sensor_eco2->info.range_max  = SENSOR_ECO2_RANGE_MAX;
        sensor_eco2->info.range_min  = SENSOR_ECO2_RANGE_MIN;
        sensor_eco2->info.period_min = SENSOR_ECO2_PERIOD_MIN;
        sensor_eco2->info.fifo_max   = SENSOR_ECO2_FIFO_MAX;
        sensor_eco2->data_len        = 0;

        rt_memcpy(&sensor_eco2->config, cfg, sizeof(struct rt_sensor_config));
        sensor_eco2->ops = &sensor_ops;
        sensor_eco2->module = module;

        result = rt_hw_sensor_register(sensor_eco2, name, RT_DEVICE_FLAG_RDWR, RT_NULL);
        if (result != RT_EOK)
        {
            LOG_E("device register err code: %d", result);
            goto __exit;
        }
    }

    /* TVOC sensor register */
    {
        sensor_tvoc = rt_calloc(1, sizeof(struct rt_sensor_device));
        if (sensor_tvoc == RT_NULL)
            goto __exit;

        sensor_tvoc->info.type       = RT_SENSOR_CLASS_TVOC;
        sensor_tvoc->info.vendor     = RT_SENSOR_VENDOR_AMS;
        sensor_tvoc->info.model      = "ccs811";
        sensor_tvoc->info.unit       = RT_SENSOR_UNIT_PPB;
        sensor_tvoc->info.intf_type  = RT_SENSOR_INTF_I2C;
        sensor_tvoc->info.range_max  = SENSOR_TVOC_RANGE_MAX;
        sensor_tvoc->info.range_min  = SENSOR_TVOC_RANGE_MIN;
        sensor_tvoc->info.period_min = SENSOR_TVOC_PERIOD_MIN;
        sensor_tvoc->info.fifo_max   = SENSOR_TVOC_FIFO_MAX;
        sensor_tvoc->data_len        = 0;

        rt_memcpy(&sensor_tvoc->config, cfg, sizeof(struct rt_sensor_config));
        sensor_tvoc->ops = &sensor_ops;
        sensor_tvoc->module = module;
        
        result = rt_hw_sensor_register(sensor_tvoc, name, RT_DEVICE_FLAG_RDWR, RT_NULL);
        if (result != RT_EOK)
        {
            LOG_E("device register err code: %d", result);
            goto __exit;
        }
    }

    module->sen[0] = sensor_eco2;
    module->sen[1] = sensor_tvoc;
    module->sen_num = 2;
    
    LOG_I("sensor init success");
    
    return RT_EOK;
    
__exit:
    if(sensor_tvoc) 
    {
        if(sensor_tvoc->data_buf)
            rt_free(sensor_tvoc->data_buf);

        rt_free(sensor_tvoc);
    }
    if(sensor_eco2) 
    {
        if(sensor_eco2->data_buf)
            rt_free(sensor_eco2->data_buf);

        rt_free(sensor_eco2);
    }
    if (module)
        rt_free(module);

    return -RT_ERROR;
}
