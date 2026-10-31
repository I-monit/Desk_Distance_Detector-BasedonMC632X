/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-01     songyh    the first version
 *
 */

#include <rtdevice.h>
#include <rthw.h>
#include "fh_def.h"
#include "board_info.h"
#include "pwm.h"
#include "fh_pwm.h"
#include "fh_clock.h"

#if defined(FH_PWM_DEBUG) && defined(RT_DEBUG)
#define PRINT_PWM_DBG(fmt, args...)   \
    do                                \
    {                                 \
        rt_kprintf("FH_PWM_DEBUG: "); \
        rt_kprintf(fmt, ##args);      \
    } while (0)
#else
#define PRINT_PWM_DBG(fmt, args...) \
    do                              \
    {                               \
    } while (0)
#endif

#define STATUS_INT                (1<<31)
#define STATUS_FINALL0            (1<<0)
#define STATUS_FINALL1            (1<<1)
#define STATUS_FINALL2            (1<<2)
#define STATUS_FINALL3            (1<<3)
#define STATUS_FINALL4            (1<<4)
#define STATUS_FINALL5            (1<<5)
#define STATUS_FINALL6            (1<<6)
#define STATUS_FINALL7            (1<<7)
#define STATUS_FINONCE0           (1<<8)
#define STATUS_FINONCE1           (1<<9)
#define STATUS_FINONCE2           (1<<10)
#define STATUS_FINONCE3           (1<<11)
#define STATUS_FINONCE4           (1<<12)
#define STATUS_FINONCE5           (1<<13)
#define STATUS_FINONCE6           (1<<14)
#define STATUS_FINONCE7           (1<<15)

#define OFFSET_PWM_BASE(n)        (0x100 + 0x100 * n)

#define OFFSET_PWM_GLOBAL_CTRL0        (0x000)
#define OFFSET_PWM_GLOBAL_CTRL1        (0x004)
#define OFFSET_PWM_GLOBAL_CTRL2        (0x008)
#define OFFSET_PWM_INT_ENABLE          (0x010)
#define OFFSET_PWM_INT_STATUS          (0x014)

#define OFFSET_PWM_CTRL0(n)          (0x000 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG0(n)           (0x004 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG1(n)           (0x008 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG2(n)           (0x00c + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG3(n)           (0x010 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_CFG4(n)           (0x014 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_STATUS0(n)        (0x020 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_STATUS1(n)        (0x024 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_STATUS2(n)        (0x028 + OFFSET_PWM_BASE(n))
#define OFFSET_PWM_STOPTIME_BIT(n)   (8 + 16 * ((n) / 8) + ((n) % 8))
#define OFFSET_PWM_FINSHALL_BIT(n)   (0 + 16 * ((n) / 8) + ((n) % 8))
#define OFFSET_PWM_FINSHONCE_BIT(n)  (8 + 16 * ((n) / 8) + ((n) % 8))



static struct pwm_driver *fh_pwm_drv = RT_NULL;

/****************************************************
 函数名： fh_pwm_output_mask
 函数功能：同时使能多个PWM模块
 输入参数： n:表示输入PWM使能比特位；
 输出参数：无
 返回值定义：无
 ****************************************************/
static void fh_pwm_output_mask(unsigned int mask)
{
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2, mask);
}

/****************************************************
 函数名： fh_pwm_output_enable
 函数功能：使能PWM模块
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_output_enable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2);
    reg |= (RT_UINT1_MAX << n);
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2, reg);
    return 0;
}
/****************************************************
 函数名： fh_pwm_output_disable
 函数功能：去使能PWM模块
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_output_disable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2);
    reg &= ~(RT_UINT1_MAX << n);
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL2, reg);
    return 0;
}
/****************************************************
 函数名： fh_pwm_stop_ctrl
 函数功能：PWM模块输出停止控制
 输入参数： n:表示输入PWM通道号；value：0表示立即停止，1表示等待当前脉冲结束后停止
 输出参数：无
  返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_stop_ctrl(unsigned int n, unsigned int value)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);
    if (value)
        reg |= RT_UINT1_MAX << OFFSET_PWM_STOPTIME_BIT(n);
    else
        reg &= ~(RT_UINT1_MAX << OFFSET_PWM_STOPTIME_BIT(n));
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1, reg);

    return 0;

}

/****************************************************
 函数名： fh_pwm_ouput_module
 函数功能：PWM模块输出模式控制
 输入参数： n:表示输入PWM通道号；keep：0表示输出固定数目的方波，1表示一直输出方波
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_ouput_module(unsigned int n, unsigned int keep)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_CTRL0(n));
    reg &= ~(RT_UINT1_MAX << 0);
    reg |= (keep << 0);
    SET_REG(fh_pwm_drv->base +  OFFSET_PWM_CTRL0(n), reg);

    return 0;
}

/****************************************************
 函数名： fh_pwm_ouput_pulse_num
 函数功能：PWM模块输出模式控制
 输入参数： n:表示输入PWM通道号；value：表示输出固定方波数目
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_ouput_pulse_num(unsigned int n, unsigned int value)
{
    SET_REG(fh_pwm_drv->base +  OFFSET_PWM_CFG4(n), value);

    return 0;
}
/****************************************************
 函数名： fh_pwm_delay_output
 函数功能：PWM模块输出波形延时使能控制
 输入参数： n:表示输入PWM通道号；delay：第一个脉冲延迟输出的拍数
 输出参数：无
  返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_delay_output(unsigned int n, unsigned int delay)
{
    unsigned int reg;

    if (!delay)
    {
        reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_CTRL0(n));
        reg &= ~(RT_UINT1_MAX << 3);
        SET_REG(fh_pwm_drv->base + OFFSET_PWM_CTRL0(n), reg);
    }
    else
    {
        reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_CTRL0(n));
        reg |= (RT_UINT1_MAX << 3);
        SET_REG(fh_pwm_drv->base + OFFSET_PWM_CFG3(n), delay);
        SET_REG(fh_pwm_drv->base + OFFSET_PWM_CTRL0(n), reg);
    }
    return 0;
}
/****************************************************
 函数名： fh_pwm_stop_level
 函数功能：PWM模块停止输出时通道电平状态
 输入参数： n:表示输入PWM通道号；stoplevel：00：低电平，11：高电平，01
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_stop_level(unsigned int n, unsigned int stoplevel)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_CTRL0(n));
    reg &= ~(0x3 << 1);
    reg |= (stoplevel << 1);
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_CTRL0(n), reg);
    return 0;

}
/****************************************************
 函数名： fh_pwm_config_enable
 函数功能：PWM模块配置有效
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_config_enable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL0);
    reg |= (RT_UINT1_MAX << n);
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL0, reg);
    return 0;
}
/****************************************************
 函数名： fh_pwm_config_enable
 函数功能：PWM模块配置无效
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_config_disable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL0);
    reg &= ~(RT_UINT1_MAX << n);
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL0, reg);
    return 0;
}

/****************************************************
 函数名： fh_pwm_shadow_enable
 函数功能：PWM模块shadow使能控制
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_shadow_enable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);
    reg |= (RT_UINT1_MAX << n);
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1, reg);
    return 0;
}

/****************************************************
 函数名： fh_pwm_shadow_disable
 函数功能：PWM模块shadow去使能控制
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_shadow_disable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1);
    reg &= ~(RT_UINT1_MAX << n);
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_GLOBAL_CTRL1, reg);
    return 0;
}

/****************************************************
 函数名： fh_pwm_interrupt_finishall_enable
 函数功能：PWM模块FinishAll中断使能
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_interrupt_finishall_enable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
    reg |= (RT_UINT1_MAX << OFFSET_PWM_FINSHALL_BIT(n));
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE, reg);
    return 0;
}

/****************************************************
 函数名： fh_pwm_interrupt_finishall_disable
 函数功能：PWM模块FinishAll中断去使能
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_interrupt_finishall_disable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
    reg &= ~(RT_UINT1_MAX << OFFSET_PWM_FINSHALL_BIT(n));
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE, reg);
    return 0;
}

/****************************************************
 函数名： fh_pwm_interrupt_finishonce_enable
 函数功能：PWM模块FinishOnce中断使能
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_interrupt_finishonce_enable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
    reg |= (RT_UINT1_MAX << OFFSET_PWM_FINSHONCE_BIT(n));
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE, reg);
    return 0;
}

/****************************************************
 函数名： fh_pwm_interrupt_finishonce_enable
 函数功能：PWM模块FinishOnce中断去使能
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_interrupt_finishonce_disable(unsigned int n)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);
    reg &= ~(RT_UINT1_MAX << OFFSET_PWM_FINSHONCE_BIT(n));
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE, reg);
    return 0;
}

/****************************************************
 函数名： fh_pwm_interrupt_get_status
 函数功能：获取pwm中断状态寄存器
 输入参数： 无
 输出参数：无
 返回值定义：
 ****************************************************/
static unsigned int fh_pwm_interrupt_get_status(void)
{
    unsigned int reg;

    reg = GET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_STATUS);
    reg &= GET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_ENABLE);

    return reg;
}

/****************************************************
 函数名： fh_pwm_interrupt_finishonce_clear
 函数功能：PWM模块清FinishOnce中断
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_interrupt_finishonce_clear(unsigned int n)
{
    unsigned int reg = 0x0;

    reg |= (RT_UINT1_MAX << OFFSET_PWM_FINSHONCE_BIT(n));

    SET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_STATUS, reg);

    return 0;
}

/****************************************************
 函数名： fh_pwm_interrupt_finishall_clear
 函数功能：PWM模块清FinishAll中断
 输入参数： n:表示输入PWM通道号；
 输出参数：无
 返回值定义：0：配置成功；非0：配置失败
 ****************************************************/
static unsigned int fh_pwm_interrupt_finishall_clear(unsigned int n)
{
    unsigned int reg = 0x0;

    reg |= (RT_UINT1_MAX << OFFSET_PWM_FINSHALL_BIT(n));

    SET_REG(fh_pwm_drv->base + OFFSET_PWM_INT_STATUS, reg);

    return 0;
}

static rt_err_t fh_pwm_set_config(struct fh_pwm_chip_data *chip_data)
{
    struct clk *clk_rate = RT_NULL;
    unsigned int period, duty, delay, phase;

    clk_rate = clk_get(NULL, "pwm_clk");
    if (clk_rate == RT_NULL)
    {
        rt_kprintf("PWM: get clk error\n");
        return -RT_EINVAL;
    }

    fh_pwm_config_disable(chip_data->id);

    period = chip_data->config.period_ns / (NSEC_PER_SEC / clk_rate->clk_out_rate);
    duty   = chip_data->config.duty_ns / (NSEC_PER_SEC / clk_rate->clk_out_rate);
    delay  = chip_data->config.delay_ns / (NSEC_PER_SEC / clk_rate->clk_out_rate);
    phase  = chip_data->config.phase_ns / (NSEC_PER_SEC / clk_rate->clk_out_rate);

    if (period > 0x1ffffff)
    {
        rt_kprintf("PWM: period exceed 24-bit\n");
        return -RT_EINVAL;
    }

    if (period == 0x0)
    {
        rt_kprintf("PWM: period too low\n");
        return -RT_EINVAL;
    }

    if (duty > 0x1ffffff)
    {
        rt_kprintf("PWM: duty exceed 24-bit\n");
        return -RT_EINVAL;
    }

    if (chip_data->config.duty_ns > chip_data->config.period_ns)
    {
        rt_kprintf("PWM: duty is over period\n");
        return -RT_EINVAL;
    }

    PRINT_PWM_DBG("clk rate: 0x%x\n", clk_rate->clk_out_rate);
    PRINT_PWM_DBG("set period: 0x%x\n", period);
    PRINT_PWM_DBG("set duty: 0x%x\n", duty);
    PRINT_PWM_DBG("set phase: 0x%x\n", phase);
    PRINT_PWM_DBG("set delay: 0x%x\n", delay);

    SET_REG(fh_pwm_drv->base + OFFSET_PWM_CFG0(chip_data->id), period);
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_CFG1(chip_data->id), duty);
    SET_REG(fh_pwm_drv->base + OFFSET_PWM_CFG2(chip_data->id), phase);

    /* config out delay */
    fh_pwm_delay_output(chip_data->id, delay);

    /* config stop module */
    fh_pwm_stop_level(chip_data->id, chip_data->config.stop_module);

    /* config stop ctrl */
    fh_pwm_stop_ctrl(chip_data->id, chip_data->config.stop_ctrl);

    /* config output module */
    fh_pwm_ouput_module(chip_data->id, chip_data->config.pulses);

    /* config output pulse num */
    if (chip_data->config.pulses == FH_PWM_PULSE_LIMIT)
        fh_pwm_ouput_pulse_num(chip_data->id, chip_data->config.pulse_num);

    /* config_interrupt */
    if (chip_data->config.finish_once)
        fh_pwm_interrupt_finishonce_enable(chip_data->id);
    else
        fh_pwm_interrupt_finishonce_disable(chip_data->id);

    if (chip_data->config.finish_all)
        fh_pwm_interrupt_finishall_enable(chip_data->id);
    else
        fh_pwm_interrupt_finishall_disable(chip_data->id);

    if (chip_data->config.shadow_enable)
        fh_pwm_shadow_enable(chip_data->id);
    else
        fh_pwm_shadow_disable(chip_data->id);

    rt_memcpy(fh_pwm_drv->pwm+chip_data->id, chip_data, sizeof(struct fh_pwm_chip_data));

    return fh_pwm_config_enable(chip_data->id);
}

static void fh_pwm_get_config(struct fh_pwm_chip_data *chip_data)
{
    struct clk *clk_rate = RT_NULL;
    unsigned int ctrl = 0, period, duty, delay, phase, pulses,
            status0, status1, status2;

    clk_rate = clk_get(NULL, "pwm_clk");
    if (clk_rate == RT_NULL)
    {
        rt_kprintf("PWM: get clk error\n");
        return;
    }
    period = GET_REG(fh_pwm_drv->base + OFFSET_PWM_CFG0(chip_data->id));
    duty = GET_REG(fh_pwm_drv->base + OFFSET_PWM_CFG1(chip_data->id));
    phase = GET_REG(fh_pwm_drv->base + OFFSET_PWM_CFG2(chip_data->id));
    delay = GET_REG(fh_pwm_drv->base + OFFSET_PWM_CFG3(chip_data->id));
    pulses = GET_REG(fh_pwm_drv->base + OFFSET_PWM_CFG4(chip_data->id));
    ctrl = GET_REG(fh_pwm_drv->base + OFFSET_PWM_CTRL0(chip_data->id));
    status0 = GET_REG(fh_pwm_drv->base + OFFSET_PWM_STATUS0(chip_data->id));
    status1 = GET_REG(fh_pwm_drv->base + OFFSET_PWM_STATUS1(chip_data->id));
    status2 = GET_REG(fh_pwm_drv->base + OFFSET_PWM_STATUS2(chip_data->id));

    PRINT_PWM_DBG("==============================\n");
    PRINT_PWM_DBG("pwm%d register config:\n", chip_data->id);
    PRINT_PWM_DBG("\t\tperiod: 0x%x\n", period);
    PRINT_PWM_DBG("\t\tduty: 0x%x\n", duty);
    PRINT_PWM_DBG("\t\tphase: 0x%x\n", phase);
    PRINT_PWM_DBG("\t\tdelay: 0x%x\n", delay);
    PRINT_PWM_DBG("\t\tpulses: 0x%x\n", pulses);
    PRINT_PWM_DBG("\t\tctrl: 0x%x\n", ctrl);
    PRINT_PWM_DBG("\t\tstatus0: 0x%x\n", status0);
    PRINT_PWM_DBG("\t\tstatus1: 0x%x\n", status1);
    PRINT_PWM_DBG("\t\tstatus2: 0x%x\n", status2);

    chip_data->config.period_ns = period * (NSEC_PER_SEC / clk_rate->clk_out_rate);
    chip_data->config.duty_ns = duty * (NSEC_PER_SEC / clk_rate->clk_out_rate);

    PRINT_PWM_DBG("\t\tclk_rate: %d\n", clk_rate->clk_out_rate);
    PRINT_PWM_DBG("\t\tconfig.period_ns: %d\n", chip_data->config.period_ns);
    PRINT_PWM_DBG("\t\tconfig.duty_ns: %d\n", chip_data->config.duty_ns);
    PRINT_PWM_DBG("==============================\n\n");

    chip_data->config.phase_ns = phase * (NSEC_PER_SEC / clk_rate->clk_out_rate);
    chip_data->config.delay_ns = delay * (NSEC_PER_SEC / clk_rate->clk_out_rate);
    chip_data->config.pulse_num = pulses;
    chip_data->config.stop_module = (ctrl >> 1) & 0x3;
    chip_data->config.percent = chip_data->config.duty_ns /
            (chip_data->config.period_ns / 100);

    chip_data->status.busy = (status2 >> 4) & 0x1;
    chip_data->status.error = (status2 >> 3) & 0x1;
    chip_data->status.total_cnt = status1;
    chip_data->status.done_cnt = status0;
}

static rt_err_t fh_pwm_ioctl(rt_device_t dev, int cmd, void *arg)
{
    int ret = 0;
    struct fh_pwm_chip_data *chip_data = RT_NULL;
    /* struct rt_pwm_configuration *configuration = RT_NULL;    */
    /* struct fh_pwm_chip_data cd = {0};    */
    unsigned int val;

    if ((dev == RT_NULL) || (arg == RT_NULL))
    {
        rt_kprintf("PWM: input point is null\n");
        return -RT_EINVAL;
    }

    switch (cmd)
    {
    case SET_PWM_ENABLE:
        val = *(unsigned int *)arg;

        if (val > fh_pwm_drv->npwm)
        {
            ret = -RT_EIO;
            break;
        }

        fh_pwm_output_enable(val);
        break;
    case ENABLE_PWM:
        chip_data = (struct fh_pwm_chip_data *)arg;

        if (chip_data->id >= fh_pwm_drv->npwm)
        {
            ret = -RT_EIO;
            break;
        }

        fh_pwm_output_enable(chip_data->id);
        break;
    case ENABLE_MUL_PWM:
        val = *(unsigned int *)arg;

        fh_pwm_output_mask(val);
        break;
    case DISABLE_PWM:
        chip_data = (struct fh_pwm_chip_data *)arg;

        if (chip_data->id >= fh_pwm_drv->npwm)
        {
            ret = -RT_EIO;
            break;
        }

        fh_pwm_output_disable(chip_data->id);
        break;
    case SET_PWM_CONFIG:
        chip_data = (struct fh_pwm_chip_data *)arg;

        if (chip_data->id >= fh_pwm_drv->npwm)
        {
            ret = -RT_EIO;
            break;
        }

        rt_kprintf("ioctl: SET_PWM_CONFIG, pwm->id: %d, pwm->counter: %d, pwm->period: %d ns\n",
                chip_data->id, chip_data->config.duty_ns,
                chip_data->config.period_ns);
        ret = fh_pwm_set_config(chip_data);
        break;
    case GET_PWM_CONFIG:
        chip_data = (struct fh_pwm_chip_data *)arg;

        if (chip_data->id >= fh_pwm_drv->npwm)
        {
            ret = -RT_EIO;
            break;
        }

        rt_kprintf("ioctl: GET_PWM_CONFIG, pwm->id: %d, pwm->counter: %d, pwm->period: %d ns\n",
                chip_data->id, chip_data->config.duty_ns,
                chip_data->config.period_ns);
        fh_pwm_get_config(chip_data);
        break;
    case SET_PWM_DUTY_CYCLE_PERCENT:
        chip_data = (struct fh_pwm_chip_data *)arg;

        if (chip_data->id >= fh_pwm_drv->npwm)
        {
            ret = -RT_EIO;
            break;
        }
        if (chip_data->config.percent > 100)
        {
            rt_kprintf("ERROR: percentage is over 100\n");
            return -RT_EINVAL;
        }

        chip_data->config.duty_ns = chip_data->config.period_ns *
                chip_data->config.percent / 100;
        rt_kprintf("ioctl: SET_PWM_DUTY_CYCLE_PERCENT, pwm->id: %d, pwm->counter: %d, pwm->period: %d ns\n",
                chip_data->id, chip_data->config.duty_ns,
                chip_data->config.period_ns);
        ret = fh_pwm_set_config(chip_data);
        break;
    case SET_PWM_STOP_CTRL:
        chip_data = (struct fh_pwm_chip_data *)arg;

        if (chip_data->id >= fh_pwm_drv->npwm)
        {
            val = -RT_EIO;
            break;
        }

        fh_pwm_stop_ctrl(chip_data->id, chip_data->config.stop_ctrl);
        break;
    case ENABLE_FINSHALL_INTR:
        val = *(unsigned int *)arg;

        if (val > fh_pwm_drv->npwm)
        {
            val = -RT_EIO;
            break;
        }

        fh_pwm_interrupt_finishall_enable(val);
        break;
    case ENABLE_FINSHONCE_INTR:
        val = *(unsigned int *)arg;

        if (val > fh_pwm_drv->npwm)
        {
            val = -RT_EIO;
            break;
        }

        fh_pwm_interrupt_finishonce_enable(val);
        break;
    case DISABLE_FINSHALL_INTR:
        val = *(unsigned int *)arg;

        if (val > fh_pwm_drv->npwm)
        {
            val = -RT_EIO;
            break;
        }

        fh_pwm_interrupt_finishall_disable(val);
        break;
    case DISABLE_FINSHONCE_INTR:
        val = *(unsigned int *)arg;

        if (val > fh_pwm_drv->npwm)
        {
            val = -RT_EIO;
            break;
        }

        fh_pwm_interrupt_finishonce_disable(val);
        break;
    default:
        rt_kprintf("ERROR: cmd not exist\n");
        return -RT_EINVAL;
    }

    return ret;
}


void fh_pwm_interrupt(int vector, void *param)
{
    unsigned int status, stat_once_l, stat_once_h, stat_all_l, stat_all_h;
    struct fh_pwm_chip_data *chip_data;
    unsigned int irq_once_l, irq_once_h, irq_all_l, irq_all_h;

    status = fh_pwm_interrupt_get_status();
    status &= 0xffffffff;
    stat_once_l = (status >> 8) & 0xff;
    stat_all_l  = status & 0xff;
    stat_once_h = (status >> 24) & 0x3f;
    stat_all_h  = (status >> 16) & 0x3f;

    while (stat_once_l)
    {
        irq_once_l = fls(stat_once_l);
        chip_data = (&fh_pwm_drv->pwm[irq_once_l - 1]);
        if (chip_data && chip_data->finishonce_callback)
            chip_data->finishonce_callback(chip_data);
        fh_pwm_interrupt_finishonce_clear(irq_once_l - 1);
        stat_once_l &= ~(1 << (irq_once_l - 1));
    }

    while (stat_once_h)
    {
        irq_once_h = fls(stat_once_h);
        chip_data = (&fh_pwm_drv->pwm[(irq_once_h + 8) - 1]);
        if (chip_data && chip_data->finishonce_callback)
            chip_data->finishonce_callback(chip_data);
        fh_pwm_interrupt_finishonce_clear((irq_once_h + 8) - 1);
        stat_once_h &= ~(1 << (irq_once_h - 1));
    }

    while (stat_all_l)
    {
        irq_all_l = fls(stat_all_l);
        chip_data = (&fh_pwm_drv->pwm[irq_all_l - 1]);
        if (chip_data && chip_data->finishall_callback)
            chip_data->finishall_callback(chip_data);
        fh_pwm_interrupt_finishall_clear(irq_all_l - 1);
        stat_all_l &= ~(1 << (irq_all_l - 1));
    }

    while (stat_all_h)
    {
        irq_all_h = fls(stat_all_h);
        chip_data = (&fh_pwm_drv->pwm[(irq_all_h + 8) - 1]);
        if (chip_data && chip_data->finishall_callback)
            chip_data->finishall_callback(chip_data);
        fh_pwm_interrupt_finishall_clear((irq_all_h + 8) - 1);
        stat_all_h &= ~(1 << (irq_all_h - 1));
    }
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops pwm_ops = {
    .control = fh_pwm_ioctl
};
#endif

int fh_pwm_probe(void *priv_data)
{
    struct clk *clk_rate = RT_NULL;
    struct fh_pwm_obj *pwm_obj = (struct fh_pwm_obj *)priv_data;

    clk_rate = clk_get(NULL, "pwm_clk");
    if (clk_rate == RT_NULL)
    {
        rt_kprintf("ERROR: pwm get clk failed\n");
    }
    else
    {
        clk_enable(clk_rate);
    }

    if (priv_data == RT_NULL)
    {
        rt_kprintf("%s:input para error", __func__);
        return RT_EINVAL;
    }

    fh_pwm_drv = rt_malloc(sizeof(struct pwm_driver));
    if (fh_pwm_drv == RT_NULL)
    {
        rt_kprintf("%s:rt malloc error", __func__);
        return -RT_ENOMEM;
    }
    rt_memset(fh_pwm_drv, 0, sizeof(struct pwm_driver));
    fh_pwm_drv->base = pwm_obj->base;
    fh_pwm_drv->irq = pwm_obj->irq;
    fh_pwm_drv->npwm = pwm_obj->npwm;

    rt_hw_interrupt_install(fh_pwm_drv->irq, fh_pwm_interrupt,
                                (void *)fh_pwm_drv, "PWM");
    rt_hw_interrupt_umask(fh_pwm_drv->irq);

    rt_device_t pwm_dev = rt_malloc(sizeof(struct rt_device));
    if (pwm_dev == RT_NULL)
    {
        rt_kprintf("ERROR: %s rt_device malloc failed\n", __func__);
        return -RT_ENOMEM;
    }
    rt_memset(pwm_dev, 0, sizeof(struct rt_device));

    pwm_dev->user_data = pwm_obj;
#ifdef RT_USING_DEVICE_OPS
    pwm_dev->ops       = &pwm_ops;
#else
    pwm_dev->control   = fh_pwm_ioctl;
#endif
    pwm_dev->type      = RT_Device_Class_Miscellaneous;

    rt_device_register(pwm_dev, "pwm", RT_DEVICE_FLAG_RDWR);

    return 0;
}

int fh_pwm_exit(void *priv_data) { return 0; }
struct fh_board_ops pwm_driver_ops = {
    .probe = fh_pwm_probe, .exit = fh_pwm_exit,
};

void rt_hw_pwm_init(void)
{
    PRINT_PWM_DBG("%s start\n", __func__);
    fh_board_driver_register("pwm", &pwm_driver_ops);
    PRINT_PWM_DBG("%s end\n", __func__);
}
