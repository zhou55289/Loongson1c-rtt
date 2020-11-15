/*
 * Copyright (c) 2020, RudyLo <luhuadong@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-06-21     luhuadong    the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include "ccs811.h"

#define DBG_TAG                        "sensor.ams.ccs811"
#ifdef PKG_USING_CCS811_DEBUG
#define DBG_LVL                        DBG_LOG
#else
#define DBG_LVL                        DBG_ERROR
#endif
#include <rtdbg.h>


static void write_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, const void* pbuf, rt_size_t size)
{
    rt_uint8_t *_pbuf = (rt_uint8_t *)pbuf;

    rt_i2c_master_send(bus, CCS811_I2C_ADDRESS, RT_I2C_WR, &reg, 1);
    rt_i2c_master_send(bus, CCS811_I2C_ADDRESS, RT_I2C_WR, _pbuf, size);
}

static rt_size_t read_reg(struct rt_i2c_bus_device *bus, rt_uint8_t reg, const void* pbuf, rt_size_t size)
{
    rt_uint8_t *_pbuf = (rt_uint8_t *)pbuf;

    rt_i2c_master_send(bus, CCS811_I2C_ADDRESS, RT_I2C_WR, &reg, 1);

    return rt_i2c_master_recv(bus, CCS811_I2C_ADDRESS, RT_I2C_RD, _pbuf, size);
}

/*!
 *  @brief  I2C low level interfacing
 */
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

    if (delayms > 0)
        rt_thread_mdelay(delayms);

    /* If not need reply */
    if (readlen == 0)
        return RT_TRUE;

    /* Response */
    if (rt_i2c_master_recv(bus, CCS811_I2C_ADDRESS, RT_I2C_RD, readdata, readlen) != readlen)
        return RT_FALSE;

    return RT_TRUE;
}

rt_bool_t ccs811_check_ready(ccs811_device_t dev)
{
    rt_uint8_t status = 0;
    rt_uint8_t cmd[1] = {CCS811_REG_STATUS};

    if (!read_word_from_command(dev->i2c, cmd, 1, 10, &status, 1))
        return RT_FALSE;

    LOG_D("sensor status: 0x%x", status);
    if (!((status >> 3) & 0x01))
        return RT_FALSE;
    else 
        return RT_TRUE;
}

rt_bool_t ccs811_set_measure_cycle(ccs811_device_t dev, ccs811_cycle_t cycle)
{
    RT_ASSERT(dev);

    rt_uint8_t cmd[2] = {0};
    rt_uint8_t measurement = cycle << 4;

    cmd[0] = CCS811_REG_MEAS_MODE;
    cmd[1] = measurement;

    return read_word_from_command(dev->i2c, cmd, 2, 10, RT_NULL, 0);
}

rt_bool_t ccs811_set_measure_mode(ccs811_device_t dev, rt_uint8_t thresh, rt_uint8_t interrupt, ccs811_mode_t mode)
{
    RT_ASSERT(dev);

    rt_uint8_t cmd[2] = {0};
    rt_uint8_t measurement = (thresh << 2) | (interrupt << 3) | (mode << 4);

    cmd[0] = CCS811_REG_MEAS_MODE;
    cmd[1] = measurement;

    return read_word_from_command(dev->i2c, cmd, 2, 10, RT_NULL, 0);
}

rt_uint8_t ccs811_get_measure_mode(ccs811_device_t dev)
{
    RT_ASSERT(dev);

    rt_uint8_t cmd[1] = {0};
    rt_uint8_t measurement = 0;

    cmd[0] = CCS811_REG_MEAS_MODE;

    if (!read_word_from_command(dev->i2c, cmd, 1, 10, &measurement, 1))
        return 0xFF;

    return measurement;
}

rt_bool_t  ccs811_set_thresholds(ccs811_device_t dev, rt_uint16_t low_to_med, rt_uint16_t med_to_high)
{
    RT_ASSERT(dev);

    rt_uint8_t cmd[5] = {0};

    cmd[0] = CCS811_REG_THRESHOLDS;
    cmd[1] = (rt_uint8_t)((low_to_med >> 8) & 0xF);
    cmd[2] = (rt_uint8_t)(low_to_med & 0xF);
    cmd[3] = (rt_uint8_t)((med_to_high >> 8) & 0xF);
    cmd[4] = (rt_uint8_t)(med_to_high & 0xF);

    return read_word_from_command(dev->i2c, cmd, 5, 10, RT_NULL, 0);
}

rt_uint16_t ccs811_get_co2_ppm(ccs811_device_t dev)
{
    RT_ASSERT(dev);

    rt_uint8_t cmd[1] = {CCS811_REG_ALG_RESULT_DATA};
    rt_uint8_t buffer[8] = {0};

    read_word_from_command(dev->i2c, cmd, 1, 10, buffer, 8);
    dev->eCO2 = (((rt_uint16_t)buffer[0] << 8) | (rt_uint16_t)buffer[1]);
    return dev->eCO2;
}

rt_uint16_t ccs811_get_tvoc_ppb(ccs811_device_t dev)
{
    RT_ASSERT(dev);

    rt_uint8_t cmd[1] = {CCS811_REG_ALG_RESULT_DATA};
    rt_uint8_t buffer[8] = {0};

    read_word_from_command(dev->i2c, cmd, 1, 10, buffer, 8);
    dev->TVOC = (((rt_uint16_t)buffer[2] << 8) | (rt_uint16_t)buffer[3]);
    return dev->TVOC;
}

/*!
 *  @brief  Commands the sensor to take a single eCO2/VOC measurement. Places
 *          results in {@link TVOC} and {@link eCO2}
 *  @return True if command completed successfully, false if something went
 *          wrong!
 */
rt_bool_t ccs811_measure(ccs811_device_t dev)
{
    RT_ASSERT(dev);

    rt_uint8_t cmd[1] = {CCS811_REG_ALG_RESULT_DATA};
    rt_uint8_t buffer[8] = {0};

    if (!read_word_from_command(dev->i2c, cmd, 1, 10, buffer, 8))
        return RT_FALSE;
    
    dev->eCO2 = (((rt_uint16_t)buffer[0] << 8) | (rt_uint16_t)buffer[1]);
    dev->TVOC = (((rt_uint16_t)buffer[2] << 8) | (rt_uint16_t)buffer[3]);

    return RT_TRUE;
}

/*!
 *  @brief  Set the absolute humidity value [mg/m^3] for compensation to increase
 *          precision of TVOC and eCO2.
 *  @param  absolute_humidity 
 *          A uint32_t [mg/m^3] which we will be used for compensation.
 *          If the absolute humidity is set to zero, humidity compensation
 *          will be disabled.
 *  @return True if command completed successfully, false if something went
 *          wrong!
 */
rt_bool_t ccs811_set_envdata(ccs811_device_t dev, float temperature, float humidity)
{
    RT_ASSERT(dev);

    rt_uint8_t cmd[5] = {0};
    int _temp, _rh;

    if (temperature > 0)
        _temp = (int)temperature + 0.5;  // this will round off the floating point to the nearest integer value
    else if (temperature < 0) // account for negative temperatures
        _temp = (int)temperature - 0.5;
    
    _temp = _temp + 25;  // temperature high byte is stored as T+25°C in the sensor's memory so the value of byte is positive
    _rh = (int)humidity + 0.5;  // this will round off the floating point to the nearest integer value
    
    cmd[0] = CCS811_REG_ENV_DATA;
    cmd[1] = _rh << 1;  // shift the binary number to left by 1. This is stored as a 7-bit value
    cmd[2] = 0;  // most significant fractional bit. Using 0 here - gives us accuracy of +/-1%. Current firmware (2016) only supports fractional increments of 0.5
    cmd[3] = _temp << 1;
    cmd[4] = 0;

    if (!read_word_from_command(dev->i2c, cmd, 5, 10, RT_NULL, 0))
        return RT_FALSE;
    return RT_TRUE;
}

/*!
 *   @brief  Request baseline calibration values for both CO2 and TVOC IAQ
 *           calculations. Places results in parameter memory locaitons.
 *   @param  eco2_base 
 *           A pointer to a uint16_t which we will save the calibration
 *           value to
 *   @param  tvoc_base 
 *           A pointer to a uint16_t which we will save the calibration value to
 *   @return True if command completed successfully, false if something went
 *           wrong!
 */
rt_uint16_t ccs811_get_baseline(ccs811_device_t dev)
{
	RT_ASSERT(dev);

    rt_uint8_t cmd[1] = {CCS811_REG_BASELINE};
    rt_uint8_t reply[2];
    
    if (!read_word_from_command(dev->i2c, cmd, 1, 10, reply, 2))
        return 0;

    return reply[0] << 8 | reply[1];
}

/*!
 *  @brief  Assign baseline calibration values for both CO2 and TVOC IAQ
 *          calculations.
 *  @param  eco2_base 
 *          A uint16_t which we will save the calibration value from
 *  @param  tvoc_base 
 *          A uint16_t which we will save the calibration value from
 *  @return True if command completed successfully, false if something went
 *          wrong!
 */
rt_bool_t ccs811_set_baseline(ccs811_device_t dev, rt_uint16_t baseline)
{
	RT_ASSERT(dev);

    rt_uint8_t cmd[3];
    cmd[0] = CCS811_REG_BASELINE;
    cmd[1] = baseline >> 8;
    cmd[2] = baseline;

    return read_word_from_command(dev->i2c, cmd, 3, 10, RT_NULL, 0);
}

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
static rt_err_t sensor_init(ccs811_device_t dev)
{
    rt_uint8_t cmd[5] = {0};
    rt_uint8_t hardware_id = 0;
    //rt_uint8_t hardware_id = 0x81;
    /* Soft reset */
    cmd[0] = CCS811_REG_SW_RESET;
    cmd[1] = 0x11;
    cmd[2] = 0xE5;
    cmd[3] = 0x72;
    cmd[4] = 0x8A;
    //if (!read_word_from_command(dev->i2c, cmd, 5, 100, RT_NULL, 0))
    if (!read_word_from_command(dev->i2c, cmd, 5, 1000, RT_NULL, 0))
        return -RT_ERROR;
    
    /* Get sensor id */
    cmd[0] = CCS811_REG_HW_ID;
    //if (!read_word_from_command(dev->i2c, cmd, 1, 0, &hardware_id, 1))
    if (!read_word_from_command(dev->i2c, cmd, 1, 1000, &hardware_id, 1))
        return -RT_ERROR;

    if (hardware_id != CCS811_HW_ID)
    {
        LOG_E("sensor hardware id not 0x%x (0x%02x)", CCS811_HW_ID, hardware_id);
        return -RT_ERROR;
    }

    //rt_thread_mdelay(20);
    rt_thread_mdelay(50);

    /* Start app */
    cmd[0] = CCS811_BOOTLOADER_APP_START;
    if (!read_word_from_command(dev->i2c, cmd, 1, 10, RT_NULL, 0))
        return -RT_ERROR;
    
    /* Set measurement mode */
    ccs811_set_measure_mode(dev, 0, 0, CCS811_MODE_4);

    /* Set env data */
    ccs811_set_envdata(dev, 25, 50);

    return RT_EOK;
}

rt_err_t ccs811_init(struct ccs811_device *dev, const char *i2c_bus_name)
{
    RT_ASSERT(i2c_bus_name);

    dev->is_ready = RT_FALSE;

    dev->i2c = rt_i2c_bus_device_find(i2c_bus_name);
    if (dev->i2c == RT_NULL)
    {
        LOG_E("Can't find ccs811 device on '%s' ", i2c_bus_name);
        return -RT_ERROR;
    }

    dev->lock = rt_mutex_create("ccs811", RT_IPC_FLAG_FIFO);
    if (dev->lock == RT_NULL)
    {
        LOG_E("Can't create mutex for ccs811 device on '%s' ", i2c_bus_name);
        return -RT_ERROR;
    }

    return sensor_init(dev);
}

/**
 * This function initializes ccs811 registered device driver
 *
 * @param dev the name of ccs811 device
 *
 * @return the ccs811 device.
 */
ccs811_device_t ccs811_create(const char *i2c_bus_name)
{
    RT_ASSERT(i2c_bus_name);

    ccs811_device_t dev = rt_calloc(1, sizeof(struct ccs811_device));
    if (dev == RT_NULL)
    {
        LOG_E("Can't allocate memory for ccs811 device on '%s' ", i2c_bus_name);
        return RT_NULL;
    }

    dev->is_ready = RT_FALSE;

    dev->i2c = rt_i2c_bus_device_find(i2c_bus_name);
    if (dev->i2c == RT_NULL)
    {
        LOG_E("Can't find ccs811 device on '%s' ", i2c_bus_name);
        rt_free(dev);
        return RT_NULL;
    }

    dev->lock = rt_mutex_create("ccs811", RT_IPC_FLAG_FIFO);
    if (dev->lock == RT_NULL)
    {
        LOG_E("Can't create mutex for ccs811 device on '%s' ", i2c_bus_name);
        rt_free(dev);
        return RT_NULL;
    }

    if (sensor_init(dev) != RT_EOK)
        return RT_NULL;
    else
        return dev;
}

/**
 * This function releases memory and deletes mutex lock
 *
 * @param dev the pointer of device driver structure
 */
void ccs811_delete(ccs811_device_t dev)
{
    if (dev)
    {
        rt_mutex_delete(dev->lock);
        rt_free(dev);
    }
}
