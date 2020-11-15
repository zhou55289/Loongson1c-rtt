/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-4-30     Sanjay_Wu  the first version
 */

#include "sensor_rohm_bh1750.h"
#include "bh1750.h"
#include "../libraries/ls1c_delay.h"


/*bee*/
#include <rtthread.h>
#include <stdlib.h>
#include "../libraries/ls1c_public.h"
#include "../libraries/ls1c_gpio.h"
//yellow led
#define led4_gpio 60
//red led
#define led5_gpio 62




#define DBG_ENABLE
#define DBG_LEVEL DBG_LOG
#define DBG_SECTION_NAME  "sensor.rohm.bh1750"
#define DBG_COLOR
#include <rtdbg.h>

static bh1750_device_t bh1750_create(struct rt_sensor_intf *intf)
{
    bh1750_device_t hdev = rt_malloc(sizeof(bh1750_device_t));

    if (RT_NULL == hdev)
    {
        return RT_NULL;
    }

    bh1750_init(hdev, intf->dev_name);

    return hdev;
}

static rt_size_t bh1750_fetch_data(struct rt_sensor_device *sensor, void *buf, rt_size_t len)
{
    bh1750_device_t hdev = sensor->parent.user_data;
    struct rt_sensor_data *data = (struct rt_sensor_data *)buf;
    




    if (sensor->info.type == RT_SENSOR_CLASS_LIGHT)
    {
        //yellow led
        gpio_init(led4_gpio, gpio_mode_output);
        gpio_set(led4_gpio, gpio_level_high);
        //red led
        gpio_init(led5_gpio, gpio_mode_output);
        gpio_set(led5_gpio, gpio_level_low);
       // gpio_set(led5_gpio, gpio_level_high);

        float light_value;
        light_value = bh1750_read_light(hdev);
        data->type = RT_SENSOR_CLASS_LIGHT;
        data->data.light = (rt_int32_t)(light_value * 10);

        //red led alarm
        if (data->data.light < 1000)
        {
            gpio_set(led5_gpio, gpio_level_low);
            delay_ms(5000);
            gpio_set(led5_gpio, gpio_level_high);
            delay_ms(5000);
        }
        else
            gpio_set(led5_gpio, gpio_level_low);
        //yellow led normal
        if (data->data.light > 1000)
        {
            gpio_set(led4_gpio, gpio_level_high);
        }
        else
            gpio_set(led4_gpio, gpio_level_low);
      
        data->data.light = (rt_int32_t)(light_value);
        data->timestamp = rt_sensor_get_ts();
    }

    return 1;
}

rt_err_t bh1750_set_power(bh1750_device_t hdev, rt_uint8_t power)
{
    if (power == RT_SENSOR_POWER_NORMAL)
    {
        bh1750_power_on(hdev);
    }
    else if (power == RT_SENSOR_POWER_DOWN)
    {
        bh1750_power_down(hdev);
    }
    else
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t bh1750_control(struct rt_sensor_device *sensor, int cmd, void *args)
{
    rt_err_t result = RT_EOK;
    bh1750_device_t hdev = sensor->parent.user_data;

    switch (cmd)
    {
        case RT_SENSOR_CTRL_SET_POWER:
        {
            result = bh1750_set_power(hdev, (rt_uint32_t)args & 0xff);
            break;
        }
        case RT_SENSOR_CTRL_SELF_TEST:
        {
            result =  -RT_EINVAL;
            break;
        }
        default:
        {
            result = -RT_ERROR;
            break;
        }
    }

    return result;
}

static struct rt_sensor_ops sensor_ops =
{
    bh1750_fetch_data,
    bh1750_control
};

int rt_hw_bh1750_init(const char *name, struct rt_sensor_config *cfg)
{
    int result = -RT_ERROR;
    rt_sensor_t sensor = RT_NULL;
    bh1750_device_t hdev = bh1750_create(&cfg->intf);

    sensor = rt_calloc(1, sizeof(struct rt_sensor_device));
    if (RT_NULL == sensor)
    {
        LOG_E("calloc failed");
        return -RT_ERROR;
    }

    sensor->info.type       = RT_SENSOR_CLASS_LIGHT;
    sensor->info.vendor     = RT_SENSOR_VENDOR_UNKNOWN;
    sensor->info.model      = "bh1750_light";
    sensor->info.unit       = RT_SENSOR_UNIT_LUX;
    sensor->info.intf_type  = RT_SENSOR_INTF_I2C;
    sensor->info.range_max  = 65535;
    sensor->info.range_min  = 1;
    sensor->info.period_min = 120;

    rt_memcpy(&sensor->config, cfg, sizeof(struct rt_sensor_config));
    sensor->ops = &sensor_ops;

    result = rt_hw_sensor_register(sensor, name, RT_DEVICE_FLAG_RDWR, hdev);
    if (result != RT_EOK)
    {
        LOG_E("device register err code: %d", result);
        rt_free(sensor);
        return -RT_ERROR;
    }
    else
    {
        LOG_I("light sensor init success");
        return RT_EOK;
    }
}

int bh1750_port(void)
{
    struct rt_sensor_config cfg;

    cfg.intf.dev_name = "i2c2";
    cfg.intf.user_data = (void *)BH1750_ADDR;
    cfg.irq_pin.pin = RT_PIN_NONE;

    rt_hw_bh1750_init("bh1750", &cfg);

    return 0;
}
INIT_APP_EXPORT(bh1750_port);

