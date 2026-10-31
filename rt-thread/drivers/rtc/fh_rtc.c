/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-3-21      xuww         add license Apache-2.0
 */

#include <rtthread.h>
#include <time.h>
#include "board_info.h"
#include "fh_rtc.h"
#include "fh_clock.h"
#include "arch.h"
#include <rthw.h>

/* #define FH_RTC_DEBUG */


struct rt_mutex rtc_lock;
int core_int_count = 0;

#if defined(FH_RTC_DEBUG) && defined(RT_DEBUG)
#define PRINT_RTC_DBG(fmt, args...)   \
    do                                \
    {                                 \
        rt_kprintf("FH_RTC_DEBUG: "); \
        rt_kprintf(fmt, ##args);      \
    } while (0)
#else
#define PRINT_RTC_DBG(fmt, args...) \
    do                              \
    {                               \
    } while (0)
#endif

static struct rt_device rtc;
struct fh_rtc_controller fh_rtc;


static rt_err_t rt_rtc_open(rt_device_t dev, rt_uint16_t oflag)
{
    unsigned int sync;
    int status;

    struct fh_rtc_obj *rtc_obj;

    PRINT_RTC_DBG("%s start\n", __func__);

    rtc_obj = dev->user_data;
    status = fh_rtc_exam_magic(rtc_obj);
    if (status != 0)
        return -RT_ENOSYS;

    sync = fh_rtc_get_sync_status(rtc_obj);

    if (sync != 0x3)
    {
        rt_kprintf("ERROR: RTC is not ready\n");
        return -RT_EIO;
    }

    PRINT_RTC_DBG("%s end\n", __func__);

    return RT_EOK;
}

static rt_err_t rt_rtc_control(rt_device_t dev, int cmd, void *args)
{
    unsigned int *time;
    struct fh_rtc_obj *rtc_obj;
    int ret = RT_EOK;

    rtc_obj = dev->user_data;
    time = (unsigned int *)args;
    PRINT_RTC_DBG("%s start\n", __func__);
    RT_ASSERT(dev != RT_NULL);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_RTC_GET_TIME:
        ret = fh_rtc_gettime_sync(rtc_obj, time);
        break;

    case RT_DEVICE_CTRL_RTC_SET_TIME:
        ret = fh_rtc_settime(rtc_obj, time);
        break;
    case RT_DEVICE_CTRL_RTC_SET_ALARM:
        fh_rtc_set_alarm(rtc_obj, (struct rt_rtc_wkalarm *)args);
        break;
    case RTC_UIE_ON:
        fh_rtc_uie(rtc_obj, INT_ON);
        break;
    case RTC_UIE_OFF:
        fh_rtc_uie(rtc_obj, INT_OFF);
        break;
    case RTC_PIE_ON:
        fh_rtc_pie(rtc_obj, INT_ON);
        break;
    case RTC_PIE_OFF:
        fh_rtc_pie(rtc_obj, INT_OFF);
        break;
    }

    PRINT_RTC_DBG("%s end\n", __func__);
    return ret;
}
rt_size_t fh_rtc_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    return 0;
}
rt_err_t fh_rtc_poll(rt_device_t dev, rt_size_t size)
{
    return 0;
}
void fh_rtc_self_adjustment(struct rt_work *work, void *work_data)
{
    fh_adjust_rtc(RT_NULL);
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops fh_rtc_ops = {
    .init    = RT_NULL,
    .open    = rt_rtc_open,
    .close   = RT_NULL,
    .read    = fh_rtc_read,
    .write   = RT_NULL,
    .control = rt_rtc_control
};
#endif

#if defined(RT_USING_POSIX)
#include <dfs_file.h>
#include <dfs_posix.h>
#include <dfs_poll.h>

int rtc_fops_open(struct dfs_fd *fd)
{
    return 0;
}


static int rtc_fops_read(struct dfs_fd *fd, void *buf, size_t count)
{
    *(int *)buf = (core_int_count<<8) | 0x0;
    return 0;
}

static int rtc_fops_poll(struct dfs_fd *fd, rt_pollreq_t *req)
{
    int mask = 0;

    rt_completion_wait(&fh_rtc.rtc_uie_completion, RT_WAITING_FOREVER);
    return mask;
}

static int rtc_fops_ioctl(struct dfs_fd *fd, int cmd, void *args)
{
    unsigned int *time;
    struct fh_rtc_obj rtc_obj;
    int ret = RT_EOK;

    rtc_obj.base = RTC_REG_BASE;
    time = (unsigned int *)args;
    PRINT_RTC_DBG("%s start\n", __func__);

    switch (cmd)
    {
    case RT_DEVICE_CTRL_RTC_GET_TIME:

        ret = fh_rtc_gettime_sync(&rtc_obj, time);
        break;

    case RT_DEVICE_CTRL_RTC_SET_TIME:
        ret = fh_rtc_settime(&rtc_obj, time);
        break;
    case RT_DEVICE_CTRL_RTC_SET_ALARM:
        fh_rtc_set_alarm(&rtc_obj, (struct rt_rtc_wkalarm *)args);
        break;
    case RTC_UIE_ON:
        fh_rtc_uie(&rtc_obj, INT_ON);
        break;
    case RTC_UIE_OFF:
        fh_rtc_uie(&rtc_obj, INT_OFF);
        break;
    case RTC_PIE_ON:
        fh_rtc_pie(&rtc_obj, INT_ON);
        break;
    case RTC_PIE_OFF:
        fh_rtc_pie(&rtc_obj, INT_OFF);
        break;
    }
    PRINT_RTC_DBG("%s end\n", __func__);
    return ret;
}

static const struct dfs_file_ops rtc_fops = {
    rtc_fops_open,
    RT_NULL,
    rtc_fops_ioctl,
    rtc_fops_read,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    rtc_fops_poll,
};
#endif /* end of RT_USING_POSIX */

int fh_rtc_probe(void *priv_data)
{
    struct fh_rtc_obj *rtc_obj = (struct fh_rtc_obj *)priv_data;

    PRINT_RTC_DBG("%s start\n", __func__);

#ifdef RT_USING_DEVICE_OPS
    rtc.ops     = &fh_rtc_ops;
#else
    rtc.init    = RT_NULL;
    rtc.open    = rt_rtc_open;
    rtc.close   = RT_NULL;
    rtc.read    = RT_NULL;
    rtc.write   = RT_NULL;
    rtc.control = rt_rtc_control;
 #endif
    rtc.type    = RT_Device_Class_RTC;

    /* no private */
    rtc.user_data = rtc_obj;

    fh_rtc.regs = rtc_obj->base;
    fh_rtc.tsensor_delta_val = 0;
#ifndef RTC_USE_ONCHIP_CLK
    rt_device_register(&rtc, "rtc", RT_DEVICE_FLAG_RDWR);
#else
    rt_kprintf("rtc using onchip clk, rtc device will not be registered!\n");
#endif

    fh_rtc.adjust_parm = rtc_obj->adjust_parm;

#if defined(RT_USING_POSIX)
    rtc.fops = (void *)&rtc_fops;
#endif

#ifdef RT_USING_ALARM
    rt_thread_t thread_rtc_int;

    rt_completion_init(&fh_rtc.rtc_completion);
    rt_completion_init(&fh_rtc.rtc_uie_completion);

    rt_hw_interrupt_install(RTC_IRQn, fh_core_interrupt_handler,
            &fh_rtc, "rtc_isr");

    thread_rtc_int =
        rt_thread_create("rtc_int_thread", (void *)rtc_int_func, &fh_rtc, 10 * 1024, 12, 20);

    if (thread_rtc_int != RT_NULL)
        rt_thread_startup(thread_rtc_int);

#endif

    return 0;
}

int fh_rtc_exit(void *priv_data) { return 0; }
struct fh_board_ops rtc_driver_ops = {
    .probe = fh_rtc_probe,
    .exit = fh_rtc_exit,
};

void rt_hw_rtc_init(void)
{

    PRINT_RTC_DBG("%s start\n", __func__);
    fh_board_driver_register("rtc", &rtc_driver_ops);
    fh_rtc.regs = RTC_REG_BASE;

#ifdef CONFIG_CHIP_FH8626V100
    struct clk *rtc_clk = RT_NULL;

    rtc_clk = clk_get(NULL, "rtc_pclk_gate");
    if (rtc_clk == NULL)
    {
        PRINT_RTC_DBG("rtc clk get fail\n");
        return ;
    }
    clk_enable(rtc_clk);
#endif

    rt_mutex_init(&rtc_lock, "rtc_lock", RT_IPC_FLAG_FIFO);
    rt_thread_t thread_rtc_adjust;

    thread_rtc_adjust =
        rt_thread_create("rtc_adjust", (void *)fh_adjust_rtc, RT_NULL, 10 * 1024, 254, 20);

    if (thread_rtc_adjust != RT_NULL)
        rt_thread_startup(thread_rtc_adjust);
    PRINT_RTC_DBG("%s end\n", __func__);
}

int rtc_test(void)
{
    rt_device_t rtc_dev;
    unsigned int new_time = 0;
    unsigned int cnt = 0;
    unsigned int old_time = 0;
    int written = 0;
    unsigned int loop_cnt = 100000;

    do
    {
        rtc_dev = rt_device_find("rtc");
        if (rtc_dev == RT_NULL)
        {
            rt_kprintf("find rtc device error\n");
            return -1;
        }

        rt_device_open(rtc_dev, 0);
        rt_kprintf("-------- Test Count[%d]-------.\n", cnt++);
        rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_GET_TIME, &new_time);
        if (written)
        {
            if (new_time - old_time < 2 || new_time - old_time > 8)
            {
                rt_kprintf("error:%d,%d\n", new_time, old_time);

                rt_kprintf("-------- Test Failed[%d]-------.\n", cnt);
                rt_kprintf("error:%d,%d\n", new_time, old_time);
                return -1;
            }
            else
            {
                rt_kprintf("test ok\n");
            }
        }
        else
        {
            old_time = new_time;
        }

        rt_kprintf("\nCurrent RTC date/time is %d\n", old_time);

        old_time += 3;

        rt_device_control(rtc_dev, RT_DEVICE_CTRL_RTC_SET_TIME, &old_time);
        written = 1;

        rt_device_close(rtc_dev);
        rt_thread_delay(500);

    } while (cnt < loop_cnt);

    return 0;
}

#if 0       /* test code disabled by default */
#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(rtc_test, rtc_test);
#endif
#endif
