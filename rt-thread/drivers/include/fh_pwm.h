/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-07-13     songyh    the first version
 *
 */

#ifndef __FH_PWM_H__
#define __FH_PWM_H__

#include "fh_clock.h"
#include "pwm.h"

#define RT_UINT1_MAX                    0x1

rt_inline rt_uint32_t fls(rt_uint32_t val)
{
    rt_uint32_t  bit = 32;

    if (!val)
        return 0;
    if (!(val & 0xffff0000u))
    {
        val <<= 16;
        bit -= 16;
    }
    if (!(val & 0xff000000u))
    {
        val <<= 8;
        bit -= 8;
    }
    if (!(val & 0xf0000000u))
    {
        val <<= 4;
        bit -= 4;
    }
    if (!(val & 0xc0000000u))
    {
        val <<= 2;
        bit -= 2;
    }
    if (!(val & 0x80000000u))
    {
        val <<= 1;
        bit -= 1;
    }

    return bit;
}

struct pwm_driver
{
    struct clk *clk;
    struct fh_pwm_chip_data pwm[16];
    unsigned int base;
    int    irq;
    unsigned int npwm;
};

struct fh_pwm_obj
{
    unsigned int base;
    int irq;
    int id;
    unsigned int npwm;
};

#endif
