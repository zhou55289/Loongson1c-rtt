/*
 * Copyright (c) 2006-2019, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-4-30     Sanjay_Wu  the first version
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#define DBG_ENABLE
#define DBG_SECTION_NAME "bh1750"
#define DBG_LEVEL DBG_LOG
#define DBG_COLOR
#include <rtdbg.h>
#include "bh1750.h"


#ifdef PKG_USING_BH1750_LATEST_VERSION

static rt_err_t bh1750_read_regs(struct rt_i2c_bus_device *bus, rt_uint8_t len, rt_uint8_t *buf)
{
    struct rt_i2c_msg msgs;

    msgs.addr = BH1750_ADDR;
    msgs.flags = RT_I2C_RD;
    msgs.buf = buf;
    msgs.len = len;

    if (1 == rt_i2c_transfer(bus, &msgs, 1))
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t bh1750_write_cmd(struct rt_i2c_bus_device *bus, rt_uint8_t cmd)
{
    struct rt_i2c_msg msgs;

    msgs.addr = BH1750_ADDR;
    msgs.flags = RT_I2C_WR;
    msgs.buf = &cmd;
    msgs.len = 1;

    if (1 == rt_i2c_transfer(bus, &msgs, 1))
    {
        return RT_EOK;
    }
    else
    {
        return -RT_ERROR;
    }
}

static rt_err_t bh1750_set_measure_mode(bh1750_device_t hdev, rt_uint8_t mode, rt_uint8_t m_time)
{
    RT_ASSERT(hdev);

    if (RT_EOK == bh1750_write_cmd(hdev->bus, BH1750_RESET) &&
        RT_EOK == bh1750_write_cmd(hdev->bus, mode))
    {
        rt_thread_mdelay(m_time);
        return RT_EOK;
    }
    else
    {
        LOG_D("bh1750 set measure mode failed!");
        return -RT_ERROR;
    }
}

rt_err_t bh1750_power_on(bh1750_device_t hdev)
{
    RT_ASSERT(hdev);

    if (RT_EOK == bh1750_write_cmd(hdev->bus, BH1750_POWER_ON))
    {
        return RT_EOK;
    }
    else
    {
        LOG_D("bh1750 power on failed!");
        return -RT_ERROR;
    }
}

rt_err_t bh1750_power_down(bh1750_device_t hdev)
{
    RT_ASSERT(hdev);

    if (RT_EOK == bh1750_write_cmd(hdev->bus, BH1750_POWER_DOWN))
    {
        return RT_EOK;
    }
    else
    {
        LOG_D("bh1750 power down failed!");
        return -RT_ERROR;
    }
}

rt_err_t bh1750_init(bh1750_device_t hdev, const char *i2c_bus_name)
{
    hdev->bus = rt_i2c_bus_device_find(i2c_bus_name);
    if (RT_NULL == hdev->bus)
    {
        LOG_E("Can't find bh1750 device on '%s' ", i2c_bus_name);
        rt_free(hdev->bus);
        return -RT_ERROR;
    }

    return RT_EOK;
}

float bh1750_read_light(bh1750_device_t hdev)
{
    rt_uint8_t temp[2];
    float current_light = 0;

    RT_ASSERT(hdev);

    bh1750_set_measure_mode(hdev, BH1750_CON_H_RES_MODE2, 120);
    bh1750_read_regs(hdev->bus, 2, temp);
    current_light = ((float)((temp[0] << 8) + temp[1]) / 1.2);

    return current_light;
}

#endif /* PKG_USING_BH1750_LATEST_VERSION */

