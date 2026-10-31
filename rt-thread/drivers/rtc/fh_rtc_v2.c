/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-3-22     xuww         add license Apache-2.0
 */

#include "fh_rtc.h"
#include <fh_aes.h>
#include <rtthread.h>
#include <fh_def.h>
#include <delay.h>
#include <rthw.h>
#include <fh_pmu.h>
#include <fh_chip.h>

#if defined(FH_RTC_DEBUG) && defined(RT_DEBUG)
#define RTC_PRINT_DBG(fmt, args...)   \
    do                                \
    {                                 \
        rt_kprintf("FH_RTC_DEBUG: "); \
        rt_kprintf(fmt, ##args);      \
    } while (0)
#else
#define RTC_PRINT_DBG(fmt, args...) \
    do                              \
    {                               \
    } while (0)
#endif

extern int core_int_count;
extern struct fh_rtc_controller fh_rtc;

static int fh_rtc_set_core(unsigned int base_addr, unsigned int reg_num,
        unsigned int value);
static int fh_rtc_get_core(unsigned int base_addr, unsigned int reg_num);
static int fh_rtc_get_temp(unsigned int base_addr);

extern struct rt_mutex rtc_lock;
#ifdef FH_USE_TSENSOR_OFFSET
static int fh_rtc_get_const_temp_offset(void)
{
    unsigned int tsensor_init_data;
    int tsensor_out_value_diff;
    unsigned int tsensor_12bit;
    int temp_diff;
#ifdef REG_PMU_RTC_PARAM
    tsensor_init_data = fh_pmu_get_tsensor_init_data();
#else
    unsigned char buf[4] = {0};
    EFUSE_INFO efuse_info;

    efuse_info.efuse_entry_no = 60;
    efuse_info.key_buff = (unsigned char *)buf;
    efuse_info.key_size = 2;
    efuse_info.status.error = 0;
    fh_efuse_ioctl(&efuse_info, IOCTL_EFUSE_READ_ENTRY, 0);
    tsensor_init_data = (buf[1]<<8)|buf[0];
#endif
    tsensor_12bit = tsensor_init_data&0xfff;
    tsensor_out_value_diff = tsensor_12bit - fh_rtc.adjust_parm->tsensor_cp_default_out;
    fh_rtc.tsensor_delta_val = tsensor_out_value_diff;
    temp_diff = (tsensor_out_value_diff*fh_rtc.adjust_parm->lut_coef)/4096;
    RTC_PRINT_DBG("temp diff is  %x\n",temp_diff);

    return temp_diff;
}
#endif

#ifdef FH_USE_TSENSOR
static int fh_rtc_temp_cfg_coef_offset(unsigned int coef, unsigned int offset)
{
    unsigned int temp_cfg;
    int status;

    if (coef > 0xff)
    {
        RTC_PRINT_DBG("coef invalid\n");
        return -1;
    }
    if (offset > 0xffff)
    {
        RTC_PRINT_DBG("offset invalid\n");
        return -1;
    }
    temp_cfg = fh_rtc_get_core(fh_rtc.regs,
            FH_RTC_CMD_TEMP_CFG);
    temp_cfg &= 0xffff0000;

    temp_cfg |= coef;
    temp_cfg |= (offset<<8);

    status = fh_rtc_set_core(fh_rtc.regs,
        FH_RTC_CMD_TEMP_CFG, temp_cfg);
    return status;
}

static int fh_rtc_temp_cfg_thl_thh(unsigned int thl, unsigned int thh)
{
    unsigned int temp_cfg;
    int status;

    if (thl > 0x3f)
    {
        RTC_PRINT_DBG("thl invalid\n");
        return -1;
    }
    if (thh > 0x3f)
    {
        RTC_PRINT_DBG("thh invalid\n");
        return -1;
    }

    temp_cfg = fh_rtc_get_core(fh_rtc.regs,
            FH_RTC_CMD_TEMP_CFG);
    temp_cfg &= 0xf000ffff;
    temp_cfg |= (thl<<16);
    temp_cfg |= (thh<<22);
    status = fh_rtc_set_core(fh_rtc.regs,
            FH_RTC_CMD_TEMP_CFG,
            temp_cfg);
    return status;
}

#ifdef RTC_SUPPORT_CLK_SRC_SWITCH
static void fh_rtc_set_clk_source(int osc_clk)
{
    int offset = fh_rtc_get_core(fh_rtc.regs, FH_RTC_CMD_OFFSET);

    if (osc_clk == RTC_OSC_CLK)
        offset |= CLK_OSC_SEL;
    else
        offset &= ~CLK_OSC_SEL;
    fh_rtc_set_core(fh_rtc.regs, FH_RTC_CMD_OFFSET, offset);
    rt_kprintf("rtc using %s clk\n", osc_clk?"osc":"scu");
}
#endif

void rtc_adjust(void)
{
    int i;
    int offset_index;
    char offset_lut[48];
    unsigned int tsensor_lut;
    int reg_offset;

#ifdef RTC_SUPPORT_CLK_SRC_SWITCH
#ifdef RTC_USE_OSC_CLK
    fh_rtc_set_clk_source(RTC_OSC_CLK);
#elif defined(RTC_USE_ONCHIP_CLK)
    fh_rtc_set_clk_source(RTC_SCU_CLK);
#endif
#endif

#ifdef FH_USE_TSENSOR_OFFSET
    int offset;


    offset = fh_rtc_get_const_temp_offset();
    if ((offset < -2) || (offset > 2))
        offset = 0;
    fh_rtc_temp_cfg_coef_offset(fh_rtc.adjust_parm->lut_coef, fh_rtc.adjust_parm->lut_offset-offset);
    RTC_PRINT_DBG("tsensor diff value : %d\n", offset);
#endif
    for (i = 0; i < 12; i++)
        fh_rtc_set_core(fh_rtc.regs, FH_RTC_CMD_OFFSET_LUT+(i<<4),
            *(int *)(fh_rtc.adjust_parm->tsensor_lut + i));
    fh_rtc_temp_cfg_coef_offset(fh_rtc.adjust_parm->lut_coef, fh_rtc.adjust_parm->lut_offset);
    fh_rtc_temp_cfg_thl_thh(0, 47);
    for (i = 0; i < 12; i++)
    {
        tsensor_lut = *(int *)(fh_rtc.adjust_parm->tsensor_lut + i);
        offset_lut[i*4] = tsensor_lut&0xff;
        offset_lut[i*4+1] = (tsensor_lut>>4);
        offset_lut[i*4+2] = (tsensor_lut>>8);
        offset_lut[i*4+3] = (tsensor_lut>>12);
    }
    offset_index = 0;
    for (i = 0; i < 46; i++)
    {
        if (offset_lut[i] > offset_lut[i+1])
            offset_index = i + 1;
        else
            offset_lut[i+1] = offset_lut[i];
    }

    reg_offset = fh_rtc_get_core(fh_rtc.regs, FH_RTC_CMD_OFFSET);
    reg_offset &= 0xff000000; /* clear offset configs */
    fh_rtc_set_core(fh_rtc.regs, FH_RTC_CMD_OFFSET,
           reg_offset|OFFSET_BK_EN|OFFSET_BK_AUTO|
           (offset_index<<2)|(offset_index<<10));
    fh_rtc_get_temp(fh_rtc.regs);
}

int rtc_get_temp(void)
{
    int tsensor, temp;
    int offset = (signed char)fh_rtc.adjust_parm->lut_offset;

    fh_rtc_get_temp(fh_rtc.regs);
    tsensor = fh_rtc_get_core(fh_rtc.regs, FH_RTC_CMD_TEMP_INFO);
    if (tsensor < 0)
        return -1;
    tsensor += fh_rtc.tsensor_delta_val;

    temp = (tsensor * fh_rtc.adjust_parm->lut_coef * 10 / 4096 + offset * 10) * 170 / 47 - 500;
    return (temp+5)/10;
}
#endif

struct fh_rtc_core_int_status
{
    unsigned int core_int_en;
    unsigned int core_int_status;
};

struct fh_rtc_core_int_status fh_core_int;
enum
{

    TIME_FUNC = 0, ALARM_FUNC,

};


static int fh_rtc_core_idle(unsigned int base_addr)
{
    unsigned int status;

    status = GET_REG(base_addr+FH_RTC_INT_STATUS);
    if (status & FH_RTC_INT_STATUS_CORE_IDLE)
        return 0;
    else
        return -1;
}

static int fh_rtc_core_wr(unsigned int base_addr)
{
    int reg;

    reg = GET_REG(base_addr+FH_RTC_INT_STATUS);
    reg &= (~FH_RTC_INT_STATUS_CORE_IDLE);
    SET_REG(base_addr+FH_RTC_INT_STATUS, reg);
    return 0;
}

static int fh_rtc_set_core(unsigned int base_addr, unsigned int reg_num,
        unsigned int value)
{
    int count, status;

    rt_mutex_take(&rtc_lock, RT_WAITING_FOREVER);
    if (fh_rtc_core_idle(base_addr) < 0)
        goto err;
    fh_rtc_core_wr(base_addr);
    SET_REG(base_addr+FH_RTC_WR_DATA, value);
    SET_REG(base_addr+FH_RTC_CMD, reg_num|RTC_WRITE);
    for (count = 0; count < 25; count++)
    {
        status = fh_rtc_core_idle(base_addr);
        if (status == 0)
        {
            rt_mutex_release(&rtc_lock);
            return 0;
        }
        rt_thread_delay(1);
    }
err:
    RTC_PRINT_DBG("rtc SET CORE REG TIMEOUT\n");
    rt_mutex_release(&rtc_lock);
    return -1;
}

static int fh_rtc_set_core_udelay(unsigned int base_addr, unsigned int reg_num,
        unsigned int value)
{
    int count, status;

    rt_mutex_take(&rtc_lock, RT_WAITING_FOREVER);
    if (fh_rtc_core_idle(base_addr) < 0)
        goto err;
    fh_rtc_core_wr(base_addr);
    SET_REG(base_addr+FH_RTC_WR_DATA, value);
    SET_REG(base_addr+FH_RTC_CMD, reg_num|RTC_WRITE);
    for (count = 0; count < 25; count++)
    {
        status = fh_rtc_core_idle(base_addr);
        if (status == 0)
        {
            rt_mutex_release(&rtc_lock);
            return 0;
        }
        rt_udelay(1000);
    }
err:
    RTC_PRINT_DBG("rtc SET CORE REG TIMEOUT\n");
    rt_mutex_release(&rtc_lock);
    return -1;
}

 static int fh_rtc_get_core(unsigned int base_addr, unsigned int reg_num)
{
    int reg, count, status;

    rt_mutex_take(&rtc_lock, RT_WAITING_FOREVER);
    if (fh_rtc_core_idle(base_addr) < 0)
        goto err;
    fh_rtc_core_wr(base_addr);
    SET_REG(base_addr+FH_RTC_CMD, reg_num|RTC_READ);
    for (count = 0; count < 15; count++)
    {
        status = fh_rtc_core_idle(base_addr);
        if (status == 0)
        {
            reg = GET_REG(base_addr+FH_RTC_RD_DATA);
            rt_mutex_release(&rtc_lock);
            return reg;
        }
        rt_thread_delay(10);
    }
err:
    RTC_PRINT_DBG("rtc GET CORE REG TIMEOUT line %d\n", __LINE__);
    rt_mutex_release(&rtc_lock);
    return -1;
}

 static int fh_rtc_get_core_udelay(unsigned int base_addr, unsigned int reg_num)
{
    int reg, count, status;

    rt_mutex_take(&rtc_lock, RT_WAITING_FOREVER);
    if (fh_rtc_core_idle(base_addr) < 0)
        goto err;
    fh_rtc_core_wr(base_addr);
    SET_REG(base_addr+FH_RTC_CMD, reg_num|RTC_READ);
    for (count = 0; count < 15; count++)
    {
        status = fh_rtc_core_idle(base_addr);
        if (status == 0)
        {
            reg = GET_REG(base_addr+FH_RTC_RD_DATA);
            rt_mutex_release(&rtc_lock);
            return reg;
        }
        rt_udelay(1000);
    }
err:
    RTC_PRINT_DBG("rtc GET CORE REG TIMEOUT line %d\n", __LINE__);
    rt_mutex_release(&rtc_lock);
    return -1;
}

static int fh_rtc_set_counter(struct fh_rtc_obj *rtc, unsigned int secs)
{
    unsigned int seconds, minutes, hours, days, tmp;
    int count;

    rt_mutex_take(&rtc_lock, RT_WAITING_FOREVER);
    days = secs / 86400;
    tmp = secs % 86400;

    hours = tmp / 3600;
    tmp = tmp % 3600;

    minutes = tmp / 60;
    seconds = tmp % 60;

    if (fh_rtc_core_idle(rtc->base) < 0)
        goto err;
    fh_rtc_core_wr(rtc->base);
    SET_REG(rtc->base+FH_RTC_WR_DATA, (days << 17) | (hours << 12) | (minutes << 6) | seconds);
    SET_REG(rtc->base+FH_RTC_CMD, RTC_WRITE);
    for (count = 0; count < 15; count++)
    {
        if (fh_rtc_core_idle(rtc->base) == 0)
        {
            rt_mutex_release(&rtc_lock);
            return 0;
        }

        rt_thread_delay(1);
    }
err:
    RTC_PRINT_DBG("rtc core busy can't set time\n");
    rt_mutex_release(&rtc_lock);
    return -1;

}

static unsigned  int fh_rtc_get_counter(struct fh_rtc_obj *rtc)
{
    unsigned int seconds, minutes, hours, days, tmp;
    int count, status;

    rt_mutex_take(&rtc_lock, RT_WAITING_FOREVER);
    if (fh_rtc_core_idle(rtc->base) < 0)
        goto err;
    fh_rtc_core_wr(rtc->base);
    SET_REG(rtc->base+FH_RTC_CMD, RTC_READ);
    for (count = 0; count < 25; count++)
    {
        status = fh_rtc_core_idle(rtc->base);
        if (status == 0)
        {
            tmp = GET_REG(rtc->base+FH_RTC_RD_DATA);
            seconds = tmp & 0x3f;
            minutes = (tmp >> 6) & 0x3f;
            hours = (tmp >> 12) & 0x1f;
            days = (tmp >> 17) & 0x7fff;
            rt_mutex_release(&rtc_lock);
            return (seconds + minutes * 60 + hours * 3600 + days * 86400);

        }
        rt_thread_delay(1);

    }
err:
    RTC_PRINT_DBG("rtc core busy can't get time\n");
    rt_mutex_release(&rtc_lock);
    return -1;
}

static int fh_rtc_get_temp(unsigned int base_addr)
{
    int reg, count, status;

    rt_mutex_take(&rtc_lock, RT_WAITING_FOREVER);
    if (fh_rtc_core_idle(base_addr) < 0)
        goto err;
    fh_rtc_core_wr(base_addr);
    SET_REG(base_addr+FH_RTC_CMD, RTC_TEMP);
    for (count = 0; count < 15; count++)
    {
        status = fh_rtc_core_idle(base_addr);
        if (status == 0)
        {
            reg = GET_REG(base_addr+FH_RTC_RD_DATA);
            rt_mutex_release(&rtc_lock);
            return reg;
        }
        rt_thread_delay(1);
    }
err:
    RTC_PRINT_DBG("rtc GET CORE REG TIMEOUT line %d\n", __LINE__);
    rt_mutex_release(&rtc_lock);
    return -1;
}

int fh_rtc_set_alarm_time(struct fh_rtc_obj *rtc, struct rt_rtc_wkalarm *args)
{
    int reg, hour, min, sec;

    hour = (args->tm_hour & 0x1f)<<12;
    min = (args->tm_min & 0x3f)<<6;
    sec = (args->tm_sec & 0x3f);
    reg = fh_rtc_get_core(rtc->base, FH_RTC_CMD_COUNTER);
    reg &= 0xfffe0000;
    reg = reg | hour |(min) |sec;
    fh_rtc_set_core(rtc->base, FH_RTC_CMD_ALARM_CFG, reg);
    return 0;
}

int fh_rtc_alarm_int_cotl(struct fh_rtc_obj *rtc, int enable)
{
    int reg;

    if (enable)
    {
        reg = GET_REG(fh_rtc.regs+FH_RTC_INT_STATUS);
        reg &= (~FH_RTC_INT_STATUS_CORE);
        SET_REG(fh_rtc.regs+FH_RTC_INT_STATUS, reg);

        reg = GET_REG(fh_rtc.regs+FH_RTC_INT_EN);
        reg |= FH_RTC_INT_CORE_INT_ERR_EN;
        reg &= (~FH_RTC_INT_CORE_INT_ERR_MASK);
        SET_REG(fh_rtc.regs+FH_RTC_INT_EN, reg);

        reg = fh_rtc_get_core(rtc->base, FH_RTC_CMD_INT_EN);
        reg |= FH_RTC_CORE_INT_EN_ALM_INT;
        reg &= (~FH_RTC_INT_CORE_INT_ERR_MASK);
        fh_rtc_set_core(rtc->base, FH_RTC_CMD_INT_EN, reg);
        rt_hw_interrupt_umask(rtc->irq);
    }
    else
    {
        reg = fh_rtc_get_core(rtc->base, FH_RTC_CMD_INT_EN);
        reg &= (~FH_RTC_CORE_INT_EN_ALM_INT);
        fh_rtc_set_core(rtc->base, FH_RTC_CMD_INT_EN, reg);
    }

    return 0;
}

int fh_rtc_core_int_cotl(struct fh_rtc_obj *rtc, int enable)
{
    int reg;

    if (enable)
    {
        reg = fh_rtc_get_core(rtc->base, FH_RTC_CMD_INT_EN);
        reg |= (1<<0);
        fh_rtc_set_core(rtc->base, FH_RTC_CMD_INT_EN, reg);
    }
    else
    {
        reg = fh_rtc_get_core(rtc->base, FH_RTC_CMD_INT_EN);
        reg &= (~(1<<0));
        fh_rtc_set_core(rtc->base, FH_RTC_CMD_INT_EN, reg);
    }

    return 0;
}
int fh_rtc_set_alarm(struct fh_rtc_obj *rtc, struct rt_rtc_wkalarm *args)
{
    if (args->enable)
    {
        fh_rtc_set_alarm_time(rtc, args);
        fh_rtc_alarm_int_cotl(rtc, INT_ON);
    }

    else
        fh_rtc_alarm_int_cotl(rtc, INT_OFF);
    return 0;
}

int fh_rtc_clear_alarm_int(struct fh_rtc_obj *rtc)
{
    int reg;

    reg = fh_rtc_get_core_udelay(rtc->base, FH_RTC_CMD_INT_STATUS);
    reg &= (~(FH_RTC_ISR_ALARM_POS)) & (~FH_RTC_ISR_ALM_OUT_INT);
    fh_rtc_set_core_udelay(rtc->base, FH_RTC_CMD_INT_STATUS, reg);

    return 0;
}

int fh_rtc_clear_power_failed_int(struct fh_rtc_obj *rtc)
{
    int reg;

    reg = fh_rtc_get_core_udelay(rtc->base, FH_RTC_CMD_INT_STATUS);
    reg &= (~FH_RTC_ISR_POWERFAIL_POS);
    fh_rtc_set_core_udelay(rtc->base, FH_RTC_CMD_INT_STATUS, reg);
    return 0;
}


int fh_rtc_get_core_int_status(struct fh_rtc_obj *rtc)
{
    int status;

    status = fh_rtc_get_core(rtc->base, FH_RTC_CMD_INT_STATUS);

    return status;
}

int fh_rtc_clear_uie_int(struct fh_rtc_obj *rtc)
{
    int reg;

    reg = fh_rtc_get_core_udelay(rtc->base, FH_RTC_CMD_INT_STATUS);
    reg &= (~FH_RTC_ISR_SEC_POS);
    fh_rtc_set_core_udelay(rtc->base, FH_RTC_CMD_INT_STATUS, reg);

    return 0;
}

int fh_rtc_clear_sec_int(struct fh_rtc_obj *rtc)
{
    int reg;

    reg = fh_rtc_get_core_udelay(rtc->base, FH_RTC_CMD_INT_STATUS);
    reg &= (~FH_RTC_ISR_SEC_POS);
    fh_rtc_set_core_udelay(rtc->base, FH_RTC_CMD_INT_STATUS, reg);

    return 0;
}

int fh_rtc_config_core_int(struct fh_rtc_obj *rtc, int enable)
{
    int reg;

    if (enable)
    {
        reg = GET_REG(fh_rtc.regs+FH_RTC_INT_EN);
        reg |= FH_RTC_INT_CORE_INT_ERR_EN;
        reg &= (~FH_RTC_INT_CORE_INT_ERR_MASK);
        SET_REG(fh_rtc.regs + FH_RTC_INT_EN, reg);
    } else
    {

        reg = GET_REG(fh_rtc.regs + FH_RTC_INT_EN);
        reg |= FH_RTC_INT_CORE_INT_ERR_MASK;
        SET_REG(fh_rtc.regs + FH_RTC_INT_EN, reg);

        reg = GET_REG(fh_rtc.regs + FH_RTC_INT_STATUS);
        reg &= (~FH_RTC_INT_STATUS_CORE);
        SET_REG(fh_rtc.regs + FH_RTC_INT_STATUS, reg);
    }

    return 0;
}

void fh_rtc_uie(struct fh_rtc_obj *rtc, int enable)
{

    if (enable)
    {
        fh_rtc_clear_sec_int(rtc);
        fh_rtc_core_int_cotl(rtc, INT_ON);
        fh_rtc_config_core_int(rtc, 1);
        rt_hw_interrupt_umask(rtc->irq);
     }
    else
        fh_rtc_core_int_cotl(rtc, INT_OFF);
}
void fh_rtc_pie(struct fh_rtc_obj *rtc, int enable)
{

}

void rtc_int_func(void *param)
{
    struct fh_rtc_obj rtc;
    struct fh_rtc_controller *fh_rtc_ctlr;
    unsigned int status;

    fh_rtc_ctlr = (struct fh_rtc_controller *)param;
    rtc.base = fh_rtc_ctlr->regs;
    while (1)
    {
        rt_completion_wait(&fh_rtc_ctlr->rtc_completion, RT_WAITING_FOREVER);

        status = fh_rtc_get_core_int_status(&rtc);
        if (status & FH_RTC_CORE_INT_EN_SEC_INT)
        {
            rt_kprintf("rtc sec int\n");
            core_int_count++;
            fh_rtc_clear_sec_int(&rtc);
            rt_completion_done(&fh_rtc_ctlr->rtc_uie_completion);
        }
        if (status & FH_RTC_CORE_INT_EN_ALM_INT)
        {
            rt_kprintf("rtc sec alarm\n");
            fh_rtc_clear_alarm_int(&rtc);
            rt_alarm_update(RT_NULL, 1);
        }

        if (status & FH_RTC_ISR_POWERFAIL_POS)
        {
            rt_kprintf("rtc power failed\n");
            fh_rtc_clear_power_failed_int(&rtc);
        }
        fh_rtc_config_core_int(&rtc, INT_ON);
    }

}
void fh_core_interrupt_handler(int irq, void *param)
{
    struct fh_rtc_obj *fh_rtc_ctlr = NULL;

    fh_rtc_config_core_int(fh_rtc_ctlr, INT_OFF);

    rt_completion_done(&fh_rtc.rtc_completion);
}
int fh_rtc_gettime_nosync(struct fh_rtc_obj *rtc, unsigned int *time)
{
    int t = fh_rtc_get_counter(rtc);
    RTC_PRINT_DBG("rtc read date:0x%x\n", t);

    if (t < 0)
        return t;
    *time = t;
    return 0;
}

int fh_rtc_gettime_sync(struct fh_rtc_obj *rtc, unsigned int *time)
{
    return fh_rtc_gettime_nosync(rtc, time);
}

int fh_rtc_settime(struct fh_rtc_obj *rtc, unsigned int *time)
{
    return fh_rtc_set_counter(rtc, *time);
}

unsigned int fh_rtc_get_sync_status(struct fh_rtc_obj *rtc)
{
    return 0x3;
}

int fh_adjust_rtc(void *param)
{
#ifdef FH_USE_TSENSOR
    rt_thread_delay(100);
    rtc_adjust();
#endif
    return 0;
}

int fh_rtc_exam_magic(struct fh_rtc_obj *rtc)
{
    return 0;
}
