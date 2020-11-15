/*
 * Copyright (c) 2020, RudyLo <luhuadong@163.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-06-21     luhuadong    the first version
 */

#ifndef __CCS811_H__
#define __CCS811_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <sensor.h>
#include <board.h>

#define CCS811_PACKAGE_VERSION                   "0.0.1"

/* CCS811 i2c address */
#define CCS811_I2C_ADDRESS1                      0x5A
#define CCS811_I2C_ADDRESS2                      0x5B
#define CCS811_I2C_ADDRESS                       PKG_USING_CCS811_I2C_ADDRESS

#define CCS811_REG_STATUS                        0x00
#define CCS811_REG_MEAS_MODE                     0x01
#define CCS811_REG_ALG_RESULT_DATA               0x02
#define CCS811_REG_RAW_DATA                      0x03
#define CCS811_REG_ENV_DATA                      0x05
#define CCS811_REG_THRESHOLDS                    0x10
#define CCS811_REG_BASELINE                      0x11
#define CCS811_REG_HW_ID                         0x20
#define CCS811_REG_HW_VERSION                    0x21
#define CCS811_REG_FW_BOOT_VERSION               0x23
#define CCS811_REG_FW_APP_VERSION                0x24
#define CCS811_REG_INTERNAL_STATE                0xA0
#define CCS811_REG_ERROR_ID                      0xE0
#define CCS811_REG_SW_RESET                      0xFF

#define CCS811_BOOTLOADER_APP_ERASE              0xF1
#define CCS811_BOOTLOADER_APP_DATA               0xF2
#define CCS811_BOOTLOADER_APP_VERIFY             0xF3
#define CCS811_BOOTLOADER_APP_START              0xF4

#define CCS811_HW_ID                             0x81

/* Custom sensor control cmd types */
#define  RT_SENSOR_CTRL_GET_BASELINE             (0x110)   /* Get device id */
#define  RT_SENSOR_CTRL_SET_BASELINE             (0x111)   /* Set the measure range of sensor. unit is info of sensor */
#define  RT_SENSOR_CTRL_SET_ENVDATA              (0x112)   /* Set output date rate. unit is HZ */
#define  RT_SENSOR_CTRL_GET_MEAS_MODE            (0x113)
#define  RT_SENSOR_CTRL_SET_MEAS_MODE            (0x114)
#define  RT_SENSOR_CTRL_SET_MEAS_CYCLE           (0x115)
#define  RT_SENSOR_CTRL_SET_THRESHOLDS           (0x116)

typedef enum
{
    CCS811_CLOSED,      /* Idle (Measurements are disabled in this mode) */
    CCS811_CYCLE_1S,    /* Constant power mode, IAQ measurement every second */
    CCS811_CYCLE_10S,   /* Pulse heating mode IAQ measurement every 10 seconds */
    CCS811_CYCLE_60S,   /* Low power pulse heating mode IAQ measurement every 60 seconds */
    CCS811_CYCLE_250MS  /* Constant power mode, sensor measurement every 250ms */

} ccs811_cycle_t;

typedef enum
{
    CCS811_MODE_0,      /* Idle (Measurements are disabled in this mode) */
    CCS811_MODE_1,      /* Constant power mode, IAQ measurement every second */
    CCS811_MODE_2,      /* Pulse heating mode IAQ measurement every 10 seconds */
    CCS811_MODE_3,      /* Low power pulse heating mode IAQ measurement every 60 seconds */
    CCS811_MODE_4       /* Constant power mode, sensor measurement every 250ms */

} ccs811_mode_t;

struct ccs811_envdata
{
    float temperature;
    float humidity;
};

struct ccs811_meas_mode
{
    rt_uint8_t    thresh;
    rt_uint8_t    interrupt;
    ccs811_mode_t mode;
};

struct ccs811_thresholds
{
    rt_uint16_t low_to_med;
    rt_uint16_t med_to_high;
};

struct ccs811_device
{
	struct rt_i2c_bus_device *i2c;

	rt_uint16_t TVOC;
	rt_uint16_t eCO2;

	rt_bool_t   is_ready;
	rt_mutex_t  lock;
};
typedef struct ccs811_device *ccs811_device_t;

/* Device APIs */
rt_err_t        ccs811_init(struct ccs811_device *dev, const char *i2c_bus_name);
ccs811_device_t ccs811_create(const char *i2c_bus_name);
void            ccs811_delete(ccs811_device_t dev);

rt_bool_t   ccs811_check_ready(ccs811_device_t dev);
rt_uint16_t ccs811_get_co2_ppm(ccs811_device_t dev);
rt_uint16_t ccs811_get_tvoc_ppb(ccs811_device_t dev);

rt_bool_t  ccs811_measure(ccs811_device_t dev);
rt_bool_t  ccs811_set_measure_cycle(ccs811_device_t dev, ccs811_cycle_t cycle);
rt_bool_t  ccs811_set_measure_mode(ccs811_device_t dev, rt_uint8_t thresh, rt_uint8_t interrupt, ccs811_mode_t mode);
rt_uint8_t ccs811_get_measure_mode(ccs811_device_t dev);
rt_bool_t  ccs811_set_thresholds(ccs811_device_t dev, rt_uint16_t low_to_med, rt_uint16_t med_to_high);

rt_uint16_t ccs811_get_baseline(ccs811_device_t dev);
rt_bool_t   ccs811_set_baseline(ccs811_device_t dev, rt_uint16_t baseline);
rt_bool_t   ccs811_set_envdata(ccs811_device_t dev, float temperature, float humidity);

/* Sensor APIs */
rt_err_t rt_hw_ccs811_init(const char *name, struct rt_sensor_config *cfg);

#endif /* __CCS811_H__ */