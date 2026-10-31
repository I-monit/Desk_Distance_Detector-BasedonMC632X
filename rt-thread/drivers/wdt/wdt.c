/*
* Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-12     songyh         add license Apache-2.0
 */

#include <rtthread.h>
#include <rthw.h>
#include <rtdef.h>
#include <fh_pmu.h>
#include "fh_def.h"
#include "fh_wdt.h"
#include "wdt.h"
#include "board_info.h"
#include <drivers/watchdog.h>
#include "fh_clock.h"
#ifdef RT_USING_PM
#include <pm.h>
#endif

#if defined(FH_WDT_DEBUG) && defined(RT_DEBUG)
#define PRINT_WDT_DBG(fmt, args...)   \
    do                                \
    {                                 \
        rt_kprintf("FH_WDT_DEBUG: "); \
        rt_kprintf(fmt, ##args);      \
    } while (0)
#else
#define PRINT_WDT_DBG(fmt, args...) \
    do                              \
    {                               \
    } while (0)
#endif

static rt_uint32_t fh_wdt_time_left(struct fh_wdt_obj *wdt_obj)
{
    return WDT_GetCurrCount(wdt_obj) / wdt_obj->clk->clk_out_rate;
}

static void fh_wdt_keepalive(struct fh_wdt_obj *wdt_obj)
{
    WDT_Kick(wdt_obj);
}

static int fh_wdt_top_in_seconds(struct fh_wdt_obj *wdt_obj,
                                        unsigned int top)
{
    /*
     * There are 16 possible timeout values in 0..15 where the number of
     * cycles is 2 ^ (16 + i) and the watchdog counts down.
     */
    return (1 << (16 + top)) / wdt_obj->clk->clk_out_rate;
}

static int fh_wdt_get_countdown_in_seconds(struct fh_wdt_obj *wdt_obj)
{
    return (1 << (16 + WDT_GetTopValue(wdt_obj))) / wdt_obj->clk->clk_out_rate;
}

static int fh_wdt_set_top(struct fh_wdt_obj *wdt_obj, unsigned int top_s)
{
    int i, top_val = FH_WDT_MAX_TOP;

    /*
     * Iterate over the timeout values until we find the closest match. We
     * always look for >=.
     */
    for (i = 0; i <= FH_WDT_MAX_TOP; ++i)
    {
        if (fh_wdt_top_in_seconds(wdt_obj, i) >= top_s)
        {
            top_val = i;
            break;
        }
    }

    /* Set the new value in the watchdog. */
    PRINT_WDT_DBG("[wdt] set topval: %d, top_s: %d\n", top_val, top_s);
#ifdef RT_USING_PM
    wdt_obj->top_val = top_val;
#endif
    WDT_SetTopValue(wdt_obj, top_val);

    return fh_wdt_top_in_seconds(wdt_obj, top_val);
}

rt_err_t fh_watchdog_init(rt_watchdog_t *wdt)
{
    struct fh_wdt_obj *wdt_obj = wdt->parent.user_data;
    rt_uint32_t flag;

    flag = rt_hw_interrupt_disable();
    fh_wdt_set_top(wdt_obj, WDT_HW_TIMEOUT);
    if (!WDT_IsEnable(wdt_obj))
    {
        /*
         * The watchdog is not currently enabled. Set the timeout to
         * the maximum and then start it.
         */
        rt_uint32_t value;

        value = WDOG_CONTROL_REG_WDT_EN_MASK | WDOG_CONTROL_REG_RMOD_MASK;
        WDT_SetCtrl(wdt_obj, value);
        fh_wdt_keepalive(wdt_obj);
    }
    rt_hw_interrupt_enable(flag);

    return RT_EOK;
}

rt_err_t fh_watchdog_ctrl(rt_watchdog_t *wdt, int cmd, void *arg)
{
    int heartbeat;
    struct fh_wdt_obj *wdt_obj = wdt->parent.user_data;
    rt_uint32_t val;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_WDT_START:
        WDT_Enable(wdt_obj, RT_TRUE);
        break;

    case RT_DEVICE_CTRL_WDT_STOP:
        WDT_Enable(wdt_obj, RT_FALSE);
        break;

    case RT_DEVICE_CTRL_WDT_KEEPALIVE:
        fh_wdt_keepalive(wdt_obj);
        break;

    case RT_DEVICE_CTRL_WDT_SET_TIMEOUT:
        heartbeat = *((int *)(arg));
        PRINT_WDT_DBG("[wdt] settime value %lu\n", heartbeat);
        fh_wdt_set_top(wdt_obj, heartbeat);
        fh_wdt_keepalive(wdt_obj);
        break;

    case RT_DEVICE_CTRL_WDT_GET_TIMEOUT:
        *((int *)arg) = fh_wdt_get_countdown_in_seconds(wdt_obj);
        break;

    case RT_DEVICE_CTRL_WDT_GET_TIMELEFT:
        val = fh_wdt_time_left(wdt_obj);
        *((int *)arg) = val;
        break;
    default:
        return -RT_EIO;
    }

    return RT_EOK;
}

static void fh_wdt_interrupt(int irq, void *param)
{
    struct fh_wdt_obj *wdt_obj = (struct fh_wdt_obj *)param;

    if (wdt_obj && wdt_obj->interrupt)
        return wdt_obj->interrupt(irq, RT_NULL);
}

struct rt_watchdog_ops fh_watchdog_ops = {
    .init = &fh_watchdog_init, .control = &fh_watchdog_ctrl,
};

#ifdef RT_USING_PM
static int fh_wdt_suspend(const struct rt_device *device, rt_uint8_t mode)
{
    struct fh_wdt_obj *wdt_obj = (struct fh_wdt_obj *)device->user_data;

    wdt_obj->is_enabled = WDT_IsEnable(wdt_obj);
    if (wdt_obj->is_enabled)
    {
        if (wdt_obj->rst_base)
            fh_pmu_set_reg_m(wdt_obj->rst_base + wdt_obj->rst_offset,
                      wdt_obj->rst_val, wdt_obj->rst_bit);
    }
    return 0;
}

static void fh_wdt_resume(const struct rt_device *device, rt_uint8_t mode)
{
    struct fh_wdt_obj *wdt_obj = (struct fh_wdt_obj *)device->user_data;

    WDT_SetTopValue(wdt_obj, wdt_obj->top_val);

    if (wdt_obj->is_enabled)
    {
        WDT_Enable(wdt_obj, RT_TRUE);
        fh_wdt_keepalive(wdt_obj);
    }

    return;
}

struct rt_device_pm_ops wdt_pm_ops = {
    .suspend_prepare = fh_wdt_suspend,
    .resume_prepare = fh_wdt_resume
};

#endif

int fh_wdt_probe(void *priv_data)
{
    rt_watchdog_t *wdt_dev;
    struct fh_wdt_obj *wdt_obj = (struct fh_wdt_obj *)priv_data;
    char wdt_dev_name[8] = {0};
    char wdt_clk_name[16] = {0};

    rt_sprintf(wdt_clk_name, "wdt_clk");
    wdt_obj->clk = clk_get(NULL, wdt_clk_name);
    if (wdt_obj->clk == RT_NULL)
    {
        rt_kprintf("ERROR: %s rt_watchdog_t get clk failed\n", __func__);
        return -RT_EIO;
    }

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
    rt_pm_device_register(&(wdt_dev->parent), &wdt_pm_ops);
#endif

    return 0;

}


int fh_wdt_exit(void *priv_data) { return 0; }
struct fh_board_ops wdt_driver_ops = {
    .probe = fh_wdt_probe, .exit = fh_wdt_exit,
};

void rt_hw_wdt_init(void)
{
    PRINT_WDT_DBG("%s start\n", __func__);
    fh_board_driver_register("wdt", &wdt_driver_ops);
    PRINT_WDT_DBG("%s end\n", __func__);
}
