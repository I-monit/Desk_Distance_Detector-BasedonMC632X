/**
 * Copyright (c) 2015-2024 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2024-02-27     luoyj532     the first version
 *
 */

#include <rtthread.h>
#include <rthw.h>
#include <rtdef.h>
#include "fh_def.h"
#include "mol_wdt.h"
#include "wdt.h"
#include "board_info.h"
#include <drivers/watchdog.h>
#include "fh_clock.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif

static void wdt_write_reg(struct mol_wdt_obj *wdt, unsigned int offset, unsigned int val, unsigned int mask)
{
    unsigned int reg = GET_REG(wdt->base + offset);

    reg &= ~mask;
    reg |= val;
    SET_REG(wdt->base + WDOG_OP_LOCK_OFFSET, WDOG_UNLOCK_KEY);
    SET_REG(wdt->base + offset, reg);
    SET_REG(wdt->base + WDOG_OP_LOCK_OFFSET, 0);
}

static void fh_wdt_keepalive(struct mol_wdt_obj *wdt_obj)
{
    WDT_Reset(wdt_obj);
}

rt_err_t fh_watchdog_init(rt_watchdog_t *wdt)
{
    struct mol_wdt_obj *wdt_obj = wdt->parent.user_data;
    rt_uint32_t flag;

    flag = rt_hw_interrupt_disable();
    WDT_Settimeout(wdt_obj, WDT_DEFAULT_TIMEOUT);
    rt_hw_interrupt_enable(flag);

    return RT_EOK;
}

rt_err_t fh_watchdog_ctrl(rt_watchdog_t *wdt, int cmd, void *arg)
{
    int heartbeat;
    struct mol_wdt_obj *wdt_obj = wdt->parent.user_data;
    rt_uint32_t val;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_WDT_START:
        WDT_Enable(wdt_obj);
        wdt_obj->is_enabled = 1;
        break;

    case RT_DEVICE_CTRL_WDT_STOP:
        WDT_Disable(wdt_obj);
        wdt_obj->is_enabled = 0;
        break;

    case RT_DEVICE_CTRL_WDT_KEEPALIVE:
        fh_wdt_keepalive(wdt_obj);
        break;

    case RT_DEVICE_CTRL_WDT_SET_TIMEOUT:
        heartbeat = *((int *)(arg));
        rt_kprintf("[wdt] settime value %lu\n", heartbeat);
        WDT_Settimeout(wdt_obj, (unsigned int)heartbeat);
        fh_wdt_keepalive(wdt_obj);
        break;

    case RT_DEVICE_CTRL_WDT_GET_TIMEOUT:
        *((int *)arg) = (int)Wdt_Gettimeout(wdt_obj);
        break;

    case RT_DEVICE_CTRL_WDT_GET_TIMELEFT:
        val = Wdt_Gettimeleft(wdt_obj);
        *((int *)arg) = val;
        break;
    default:
        return -RT_EIO;
    }

    return RT_EOK;
}

static void fh_wdt_interrupt(int irq, void *param)
{
    struct mol_wdt_obj *wdt_obj = (struct mol_wdt_obj *)param;

    WDT_Disable(wdt_obj);
    wdt_write_reg(wdt_obj, WDOG_INT_CLEAR_OFFSET, 0x1, 0x1);
    wdt_obj->rst = 1;
    WDT_Settimeout(wdt_obj, 1);
}

struct rt_watchdog_ops fh_watchdog_ops = {
    .init = &fh_watchdog_init, .control = &fh_watchdog_ctrl,
};

#ifdef RT_USING_PM
int wdt_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct rt_watchdog_device *wdt_dev = rt_container_of(device,
                    struct rt_watchdog_device, parent);
    struct mol_wdt_obj *wdt_obj = wdt_dev->parent.user_data;

    if (wdt_obj->is_enabled)
        WDT_Disable(wdt_obj);

    return 0;
}

void wdt_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct rt_watchdog_device *wdt_dev = rt_container_of(device,
                    struct rt_watchdog_device, parent);
    struct mol_wdt_obj *wdt_obj = wdt_dev->parent.user_data;

    WDT_Settimeout(wdt_obj, wdt_obj->timeout);

    if (wdt_obj->is_enabled)
        WDT_Enable(wdt_obj);
}

struct rt_device_pm_ops wdt_pm_ops = {
    .suspend_prepare = wdt_suspend,
    .resume_prepare = wdt_resume
};
#endif

int fh_wdt_probe(void *priv_data)
{
    rt_watchdog_t *wdt_dev;
    struct mol_wdt_obj *wdt_obj = (struct mol_wdt_obj *)priv_data;
    char wdt_dev_name[8] = {0};
    char wdt_clk_name[16] = {0};

    rt_sprintf(wdt_clk_name, "wdt_clk");
    wdt_obj->clk = clk_get(NULL, wdt_clk_name);
    if (wdt_obj->clk == RT_NULL)
    {
        rt_kprintf("ERROR: %s rt_watchdog_t get clk failed\n", __func__);
        return -RT_EIO;
    }
    clk_enable(wdt_obj->clk);

    rt_hw_interrupt_install(wdt_obj->irq, fh_wdt_interrupt, (void *)wdt_obj,
                            "wdt_irq");
    rt_hw_interrupt_umask(wdt_obj->irq);


    wdt_dev = (rt_watchdog_t *)rt_malloc(sizeof(rt_watchdog_t));
    if (wdt_dev == RT_NULL)
    {
        rt_kprintf("ERROR: %s rt_watchdog_t malloc failed\n", __func__);
        return -RT_ENOMEM;
    }

    wdt_dev->ops = &fh_watchdog_ops;
    rt_sprintf(wdt_dev_name, "%s%d", "fh_wdt", wdt_obj->id);
    rt_hw_watchdog_register(wdt_dev, wdt_dev_name, RT_DEVICE_OFLAG_RDWR, wdt_obj);

#ifdef RT_USING_PM
    rt_pm_device_register(&wdt_dev->parent, &wdt_pm_ops);
#endif

    return 0;

}

int fh_wdt_exit(void *priv_data) { return 0; }
struct fh_board_ops wdt_driver_ops = {
    .probe = fh_wdt_probe, .exit = fh_wdt_exit,
};

void rt_hw_wdt_init(void)
{
    fh_board_driver_register("wdt", &wdt_driver_ops);
}

#if 0
#include <rtthread.h>
#include <rtdevice.h>
#include <delay.h>
#include <wdt.h>

#define FH_WDT_NAME "fh_wdt0"

void wdt_test(void)
{
    int feed_times = 0;
    int timeout = 5;
    int ret = 0;
    rt_device_t wdt_dev;

    wdt_dev = rt_device_find(FH_WDT_NAME);
    if (wdt_dev == RT_NULL)
        rt_kprintf("find dev failed\n");

    ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &timeout);
    if (ret != RT_EOK)
        rt_kprintf("set wdt timeout failed\n");

    ret = rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_START, RT_NULL);
    if (ret != RT_EOK)
        rt_kprintf("enable wdt failed\n");

    while (feed_times < 10)
    {
        rt_device_control(wdt_dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, NULL);
        rt_kprintf("feed wdt %d", feed_times);
        feed_times++;
        rt_thread_mdelay(1000);
    }
    feed_times = 0;
    while (feed_times < 10)
    {
        rt_kprintf("stop feed wdt %d", feed_times);
        feed_times++;
        rt_thread_mdelay(1000);
    }
}
MSH_CMD_EXPORT(wdt_test, test fullhan watchdog);
#endif
