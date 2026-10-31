/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-24     tangyh    the first version
 *
 */

#include <rtthread.h>
#include "fh_clock.h"
#include <clock.h>
#include "board_info.h"
#include <rtdef.h>

static rt_err_t fh_clock_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t fh_clock_open(rt_device_t dev, rt_uint16_t oflag)
{

    return RT_EOK;
}

static rt_err_t fh_clock_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t fh_clock_ioctl(rt_device_t dev, int cmd, void *arg)
{

    struct clk_usr *uclk = RT_NULL;
    struct clk *get_clk = RT_NULL;
    rt_int32_t ret;

    switch (cmd)
    {
    case SET_CLK_RATE:
        uclk = (struct clk_usr *)arg;
        get_clk = clk_get(NULL, uclk->name);
        if (get_clk == RT_NULL)
        {
            rt_kprintf("can not get clk rate");
            return -RT_EIO;
        }
        ret = clk_set_rate(get_clk, uclk->frequency);
        if (ret != RT_EOK)
        return -RT_EIO;
        break;
    case GET_CLK_RATE:
        uclk = (struct clk_usr  *)arg;
        get_clk = clk_get(NULL, uclk->name);
        if (get_clk == RT_NULL)
        {
            rt_kprintf("can not get clk rate");
            return -RT_EIO;

        }
        uclk->frequency = get_clk->clk_out_rate;
        rt_kprintf("get clk: %s, rate: %lu\n", uclk->name, uclk->frequency);
        break;
    case CLK_ENABLE:
        uclk = (struct clk_usr  *)arg;
        get_clk = clk_get(NULL, uclk->name);
        if (get_clk == RT_NULL)
        {
            rt_kprintf("can not enable clk %s", uclk->name);
            return -RT_EIO;

        }
        ret = clk_enable(get_clk);
        if (ret != RT_EOK)
            return -RT_EIO;
        break;
    case CLK_DISABLE:
        uclk = (struct clk_usr  *)arg;
        get_clk = clk_get(NULL, uclk->name);
        if (get_clk == RT_NULL)
        {
            rt_kprintf("can not disable clk %s", uclk->name);
            return -RT_EIO;

        }
        clk_disable(get_clk);
        break;
    case SET_CLK_PHASE:
        uclk = (struct clk_usr *)arg;
        get_clk = clk_get(NULL, uclk->name);
        if (get_clk == RT_NULL)
        {
            rt_kprintf("can not set clk phase");
            return -RT_EIO;
        }
        ret = fh_clk_set_phase(get_clk, uclk->frequency);
        if (ret != RT_EOK)
            return -RT_EIO;
        break;
    case GET_CLK_PHASE:
        uclk = (struct clk_usr  *)arg;
        get_clk = clk_get(NULL, uclk->name);
        if (get_clk == RT_NULL)
        {
            rt_kprintf("can not get clk phase");
            return -RT_EIO;

        }
        uclk->frequency = get_clk->clk_out_rate;
        rt_kprintf("get clk: %s, phase: %lu\n", uclk->name, uclk->frequency);
        break;
    default:
        rt_kprintf("wrong para...\n");
        return -RT_EIO;
    }

    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops fh_clock_ops = {
    fh_clock_init,
    fh_clock_open,
    fh_clock_close,
    NULL,
    NULL,
    fh_clock_ioctl
};
#endif

int rt_clk_dev_init(void)
{
    rt_device_t clk_dev;

    /* malloc a rt device.. */
    clk_dev = RT_KERNEL_MALLOC(sizeof(struct rt_device));
    if (!clk_dev)
    {
        return -RT_ENOMEM;
    }
    rt_memset(clk_dev, 0, sizeof(struct rt_device));
#ifdef RT_USING_DEVICE_OPS
    clk_dev->ops  = &fh_clock_ops;
#else
    clk_dev->open      = fh_clock_open;
    clk_dev->close     = fh_clock_close;
    clk_dev->control   = fh_clock_ioctl;
    clk_dev->init      = fh_clock_init;
#endif
    clk_dev->type      = RT_Device_Class_Miscellaneous;

    rt_device_register(clk_dev, "fh_clock", RT_DEVICE_FLAG_RDWR);
    return RT_EOK;
}

