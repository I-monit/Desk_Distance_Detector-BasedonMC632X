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
#include <rthw.h>
#include "fh_chip.h"
#include "fh_def.h"
#include "fh_pmu.h"
#include "fh_clk.h"
#include "fh_clock.h"

extern int __rt_ffs(int value);

struct fh_clock_reg_cfg
{
    rt_uint32_t offset;
    rt_uint32_t value;
    rt_uint32_t mask;
};

signed int rt_fh_clock_config(struct fh_clock_reg_cfg *clk)
{
    rt_uint32_t read_value, write_value;
    rt_uint32_t mask_shift_num, value_max;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    /*rt_enter_critical();*/

    mask_shift_num = __rt_ffs(clk->mask)-1;
    value_max = clk->mask>>mask_shift_num;
    if (clk->value > value_max)
    {
        rt_kprintf("rt_fh_clock_config exseed max vlue:%x\n", value_max);
        rt_hw_interrupt_enable(level);
        /*rt_exit_critical();*/
        return CLK_DIV_VALUE_EXCEED_MUX;
    }

    read_value = fh_pmu_get_reg(clk->offset);
    write_value = (read_value&(~clk->mask))|(clk->value<<mask_shift_num);
    fh_pmu_set_reg(clk->offset, write_value);
    rt_hw_interrupt_enable(level);
    /*rt_exit_critical();*/
    return CLK_OK;
}


void fh_clk_sync(struct clk *p)
{
    rt_int32_t div, gate, mux;
    rt_int32_t shift_num, reg_value;

    if (!!(p->flag&DIV))
    {
        reg_value = fh_pmu_get_reg(p->clk_div.reg_offset);
        shift_num = __rt_ffs(p->clk_div.mask)-1;
        div = (reg_value&p->clk_div.mask)>>shift_num;
        p->clk_div.div = div;
    }

    if (!!(p->flag&GATE))
    {
        reg_value = fh_pmu_get_reg(p->clk_gate.reg_offset);
        shift_num = __rt_ffs(p->clk_gate.mask)-1;
        gate = (reg_value&p->clk_gate.mask)>>shift_num;
        p->clk_gate.value = gate;
    }

    if (!!(p->flag&MUX))
    {
        reg_value = fh_pmu_get_reg(p->clk_mux.reg_offset);
        shift_num = __rt_ffs(p->clk_mux.mask)-1;
        mux = (reg_value&p->clk_mux.mask)>>shift_num;
        p->clk_mux.num = mux;
    }
    return;
}

rt_int32_t rt_hw_clock_config(struct clk *clk_config)
{
    struct clk *cur_config = RT_NULL;
    struct fh_clock_reg_cfg local_reg_cfg;

    cur_config = clk_config;
    if (cur_config == RT_NULL)
    {
        rt_kprintf("input para pointer is RT_NULL");
        return PARA_ERROR;
    }

    /*config*/
    if (cur_config->flag&MUX)
    {
        local_reg_cfg.offset = cur_config->clk_mux.reg_offset;
        local_reg_cfg.value = cur_config->clk_mux.num;
        local_reg_cfg.mask = cur_config->clk_mux.mask;
        rt_fh_clock_config(&local_reg_cfg);
    }

    if (!!(cur_config->flag&DIV))
    {
        local_reg_cfg.offset = cur_config->clk_div.reg_offset;
        local_reg_cfg.value = cur_config->clk_div.div;
        local_reg_cfg.mask = cur_config->clk_div.mask;
        rt_fh_clock_config(&local_reg_cfg);

    }

    if (!!(cur_config->flag&GATE))
    {
        local_reg_cfg.offset = cur_config->clk_gate.reg_offset;
        local_reg_cfg.value = cur_config->clk_gate.value;
        local_reg_cfg.mask = cur_config->clk_gate.mask;
        rt_fh_clock_config(&local_reg_cfg);
    }
    return CLK_OK;
}
static unsigned long fh_clk_get_pll_p_rate(struct clk *clk)
{
    unsigned int reg, m, n, p;
    unsigned int clk_vco, divvcop = 1, shift;

    reg = fh_pmu_get_reg(clk->clk_div.reg_offset);

    m = reg & 0x7f;
    n = (reg >> 8) & 0x1f;
    p = (reg >> 16) & 0x3f;

    /*pll databook*/
    if (m < 4)
        m = 128+m;

    if (m == 0xb)
        m = 0xa;

    shift = __rt_ffs(clk->clk_gate.mask)-1;
    reg = fh_pmu_get_reg(clk->clk_gate.reg_offset);

    switch ((reg&clk->clk_gate.mask)>>shift)
    {
    case DIVVCO_ONE_DEVISION:
        divvcop = 1;
        break;

    case DIVVCO_TWO_DEVISION:
        divvcop = 2;
        break;

    case DIVVCO_FOUR_DEVISION:
        divvcop = 4;
        break;

    case DIVVCO_EIGHT_DEVISION:
        divvcop = 8;
        break;

    case DIVVCO_SIXTEEN_DEVISION:
        divvcop = 16;
        break;

    case DIVVCO_THIRTYTWO_DEVISION:
        divvcop = 32;
        break;
    default:
        rt_kprintf("divvcop error:%x\n", divvcop);
    }

    clk_vco = OSC_FREQUENCY * m / (n+1);
    clk->clk_out_rate = clk_vco/ (p+1)/divvcop;
    return CLK_OK;
}

static unsigned long fh_clk_get_pll_r_rate(struct clk *clk)
{
    unsigned int reg, m, n, r = 1;
    unsigned int clk_vco, divvcor = 1, shift;

    reg = fh_pmu_get_reg(clk->clk_div.reg_offset);

    m = reg & 0x7f;
    n = (reg >> 8) & 0x1f;
    r = (reg >> 24) & 0x3f;

    /*pll databook*/
        if (m < 4)
            m = 128+m;

        if (m == 0xb)
            m = 0xa;

    shift = __rt_ffs(clk->clk_gate.mask)-1;
    reg = fh_pmu_get_reg(clk->clk_gate.reg_offset);

    switch ((reg&clk->clk_gate.mask)>>shift)
    {
    case DIVVCO_ONE_DEVISION:
        divvcor = 1;
        break;

    case DIVVCO_TWO_DEVISION:
        divvcor = 2;
        break;

    case DIVVCO_FOUR_DEVISION:
        divvcor = 4;
        break;

    case DIVVCO_EIGHT_DEVISION:
        divvcor = 8;
        break;

    case DIVVCO_SIXTEEN_DEVISION:
        divvcor = 16;
        break;

    case DIVVCO_THIRTYTWO_DEVISION:
        divvcor = 32;
        break;
    default:
        rt_kprintf("divvcop error:%x\n", divvcor);
    }


    clk_vco = OSC_FREQUENCY * m / (n+1);
    clk->clk_out_rate = clk_vco/ (r+1)/divvcor;
    return CLK_OK;
}

void rt_fh_clock_pll_sync(struct clk *clk)
{
    rt_uint32_t m, n, no, ret;

        ret = fh_pmu_get_reg(clk->clk_div.reg_offset);
        m = ret&0xff;
        n = ((ret&(0xf00))>>8);
        no = ((ret&(0x30000))>>16);
        clk->clk_out_rate = (OSC_FREQUENCY*m/n)>>no;
}

rt_int32_t rt_fh_clk_outrate_cal(struct clk *clk_config)
{

    rt_uint32_t div_value = 1;
    struct clk *cur_config = RT_NULL;
    struct clk *parent_config = RT_NULL;
    rt_uint32_t i = 0;

    cur_config = clk_config;

    if (cur_config == RT_NULL)
    {
        rt_kprintf("input para pointer is RT_NULL");
        return PARA_ERROR;
    }

    if (cur_config->flag & CLOCK_FIXED)
    {
        return CLK_OK;
    }

    if (cur_config->flag & CLOCK_PLL_P)
    {
        fh_clk_get_pll_p_rate(cur_config);
        return CLK_OK;
    }
    else if (cur_config->flag & CLOCK_PLL_R)
    {
        fh_clk_get_pll_r_rate(cur_config);
        return CLK_OK;
    }
    else if (cur_config->flag & CLOCK_PLL)
    {
        rt_fh_clock_pll_sync(cur_config);
        return CLK_OK;
    } else
        ;

    switch (cur_config->flag&RT_UINT2_MAX)
    {
    case 0:
        div_value = 1;
        break;
    case PREDIV:
        div_value = cur_config->pre_div;
        break;
    case DIV:
        div_value = cur_config->clk_div.div+1;
        break;
    case PREDIV|DIV:
        div_value = cur_config->pre_div*(cur_config->clk_div.div+1);
    }

    if (!(cur_config->flag & MUX))
        cur_config->clk_mux.num = 0;

    /*zt*/
    if ((cur_config->flag & CLOCK_CIS) && (cur_config->flag & MUX))
    {
        if (cur_config->clk_mux.num == 0)
        {
            cur_config->clk_out_rate = OSC_FREQUENCY;
            return CLK_OK;
        }
    }

    for (i = 0; i < fh_clock_cfg_count; i++)
    {
        parent_config = &fh_clk_list[i];
        if (cur_config->mult_parent[cur_config->clk_mux.num] &&
            !rt_strcmp(parent_config->name, cur_config->mult_parent[cur_config->clk_mux.num]))
        {
            cur_config->clk_out_rate = parent_config->clk_out_rate/div_value;
            break;
        }
        else
            parent_config = RT_NULL;
    }

    if (parent_config == RT_NULL)
    {
        cur_config->clk_out_rate = 0;
        return CLK_OK;
    }

    cur_config->clk_out_rate = parent_config->clk_out_rate/div_value;

    return CLK_OK;
}

void rt_hw_clock_init(void)
{
    struct clk *p;
    rt_int32_t i;

    for (i = 0; i < fh_clock_cfg_count; i++)
    {
        p = &fh_clk_list[i];

        if (p->clk_config == CONFIG)
            rt_hw_clock_config(p);
        else
            fh_clk_sync(p);
    }

    for (i = 0; i < fh_clock_cfg_count; i++)
    {
        p = &fh_clk_list[i];
        rt_fh_clk_outrate_cal(p);
    }

    return;
}

int clk_enable(struct clk *p_clk)
{
    struct fh_clock_reg_cfg local_reg_cfg;

    if ((p_clk == RT_NULL) || (p_clk->name == RT_NULL))
    {
        rt_kprintf("input para error\r\n");
        return PARA_ERROR;
    }

    if ((p_clk->flag&GATE) == RT_NULL)
    {
#ifndef FH_FAST_BOOT
        rt_kprintf("%s, %s has no gate register\n", __func__, p_clk->name);
#endif
        return PARA_ERROR;
    }
   local_reg_cfg.offset = p_clk->clk_gate.reg_offset;
   local_reg_cfg.value = p_clk->clk_gate.value = OPEN;
   local_reg_cfg.mask = p_clk->clk_gate.mask;
   rt_fh_clock_config(&local_reg_cfg);

   return CLK_OK;
}

void clk_disable(struct clk *p_clk)
{
    struct fh_clock_reg_cfg local_reg_cfg;

    if ((p_clk == RT_NULL) || (p_clk->name == RT_NULL))
    {
        rt_kprintf("input para error\r\n");
        return;
    }

    if ((p_clk->flag&GATE) == RT_NULL)
    {
        rt_kprintf("%s, %s has no gate register\n", __func__, p_clk->name);
        return;
    }

   local_reg_cfg.offset = p_clk->clk_gate.reg_offset;
   local_reg_cfg.value = p_clk->clk_gate.value = CLOSE;
   local_reg_cfg.mask = p_clk->clk_gate.mask;
   rt_fh_clock_config(&local_reg_cfg);

}

int clk_is_enabled(struct clk *p_clk)
{
    int enable = 0;
    rt_base_t level;

    if ((p_clk == RT_NULL) || (p_clk->name == RT_NULL))
    {
        rt_kprintf("input para error\r\n");
        return 0;
    }

    if ((p_clk->flag&GATE) == RT_NULL)
    {
        return 1;
    }

    level = rt_hw_interrupt_disable();

    enable = ((fh_pmu_get_reg(p_clk->clk_gate.reg_offset) & p_clk->clk_gate.mask) == (OPEN?p_clk->clk_gate.mask:0));

    rt_hw_interrupt_enable(level);

    return enable;
}

int clk_set_rate(struct clk *p_clk, unsigned long rate)
{
    rt_uint32_t i, j;
    struct clk *p = RT_NULL;
    struct clk *cur_config = RT_NULL;
    rt_uint32_t div_value = 1;
    rt_uint32_t cur_refsrc = 0;
    rt_uint32_t next_refsrc = 0;

    if ((p_clk == RT_NULL) || (p_clk->name == RT_NULL))
    {
        rt_kprintf("input para error\r\n");
        return PARA_ERROR;
    }

    if (rate == 0)
    {
        rt_kprintf("%s:input para error\r\n",p_clk->name);
        return PARA_ERROR;

    }

    if (!(p_clk->flag&DIV))
    {
        rt_kprintf("%s:current clk not support div \r\n",p_clk->name);
        return PARA_ERROR;
    }
    /*zt*/
    if ((p_clk->flag & CLOCK_CIS) && (p_clk->flag & MUX))
    {
        if (rate == OSC_FREQUENCY)
        {
            p_clk->clk_out_rate = OSC_FREQUENCY;
            p_clk->clk_mux.num = 0;
            rt_fh_clock_config((struct fh_clock_reg_cfg *)&p_clk->clk_mux);
            return CLK_OK;
        }
        else
            p_clk->clk_mux.num = 1;
    }

    for (i = 0; i < fh_clock_cfg_count; i++)
    {
        p = &fh_clk_list[i];

        if (!!rt_strcmp(p->name, p_clk->name))
            continue;

        cur_config = p;
        for (j = 0; j < fh_clock_cfg_count; j++)
        {
            p = &fh_clk_list[j];
            if (!rt_strcmp(p->name, cur_config->mult_parent[p_clk->clk_mux.num]))
            {
                cur_refsrc = p->clk_out_rate;
                break;
            }
        }

        if (p_clk->flag & MUX)
        {
            for (j = 0; j < fh_clock_cfg_count; j++)
            {
                p = &fh_clk_list[j];
                if (!rt_strcmp(p->name, cur_config->mult_parent[!cur_config->clk_mux.num]))
                {
                    next_refsrc = p->clk_out_rate;
                    break;
                }
            }
        }
        if (cur_refsrc == 0 && (next_refsrc == 0))
        {
            rt_kprintf("clk %s parent can not find in list\r\n", cur_config->name);
            return CLK_PARENT_NOFIND;
        }

        if (!(cur_config->flag&PREDIV))
            cur_config->pre_div = 1;

        div_value = cur_refsrc / (cur_config->pre_div * rate) - 1;
        cur_config->clk_out_rate = rate;
        if (cur_refsrc%(cur_config->pre_div*rate))
        {
            rt_kprintf("%s warning:  div failed %d\r\n", cur_config->name, div_value);
            if ((p_clk->flag & MUX) && (next_refsrc != 0))
            {
                if (next_refsrc%(cur_config->pre_div * rate) == 0)
                {
                    div_value = next_refsrc / (cur_config->pre_div * rate) - 1;
                    cur_config->clk_out_rate = next_refsrc / (div_value + 1) / cur_config->pre_div;
                    cur_config->clk_mux.num = !cur_config->clk_mux.num;
                }
             }

        }

        cur_config->clk_div.div = div_value;
        rt_hw_clock_config(cur_config);
        return CLK_OK;
    }

    rt_kprintf("the clock %s device can not find\r\n", p_clk->name);
    return CLK_DIV_PARA_ERROR;
}

struct clk *clk_get(void *dev, const char *name)
{
    rt_uint32_t i;
    struct clk *p = RT_NULL;

    if (name == RT_NULL)
    {
        return RT_NULL;
    }

    for (i = 0; i < fh_clock_cfg_count; i++)
    {
        p = &fh_clk_list[i];
        if (rt_strcmp(p->name, name))
            continue;
        else
        {
            fh_clk_sync(p);
            rt_fh_clk_outrate_cal(p);
            return p;
        }
    }
    return RT_NULL;
}

unsigned long clk_get_rate(struct clk *p_clk)
{
    if (p_clk == RT_NULL)
    {
        return -1;
    }
    return p_clk->clk_out_rate;

}

void clk_change_parent(struct clk *clk, int select)
{
    struct fh_clock_reg_cfg local_reg_cfg;

    if (clk == RT_NULL)
        return;

    if (clk->flag&MUX)
    {
        local_reg_cfg.offset = clk->clk_mux.reg_offset;
        local_reg_cfg.value = select;
        local_reg_cfg.mask = clk->clk_mux.mask;
        rt_fh_clock_config(&local_reg_cfg);
    }
    else
        rt_kprintf("clk have none parents\n");
}

void fh_clk_reset(struct clk *clk)
{
    rt_uint32_t reg;

    if (clk == RT_NULL)
        return;
    if (!(clk->flag&RESET))
        rt_kprintf("%s clk can not support reset\n", clk->name);

    reg = 0xffffffff & ~(clk->clk_reset.mask);
    fh_pmu_set_reg(clk->clk_reset.reg_offset, reg);
    reg = fh_pmu_get_reg(clk->clk_reset.reg_offset);
    while (reg != 0xffffffff)
        reg = fh_pmu_get_reg(clk->clk_reset.reg_offset);
}

int fh_clk_set_phase(struct clk *clk, int phase)
{
    rt_uint32_t reg;
    rt_uint32_t mask_shift_num, value_max;

    if (clk == RT_NULL)
        return;
    if (!(clk->flag&PHASE))
        rt_kprintf("%s clk can not support phase set\n", clk->name);
    mask_shift_num = __rt_ffs(clk->clk_div.mask)-1;
    value_max = clk->clk_div.mask>>mask_shift_num;
    if (phase > value_max)
    {
        rt_kprintf("rt_fh_clock_config exseed max phase:%x\n", value_max);
        return;
    }
    reg = fh_pmu_get_reg(clk->clk_div.reg_offset);
    reg = (reg&(~clk->clk_div.mask))|(phase<<mask_shift_num);
    fh_pmu_set_reg(clk->clk_div.reg_offset, reg);
    return;
}


/*debug module*/


void test_clk_rate(void)
{
    struct clk *p = RT_NULL;
    struct clk *get = RT_NULL;
    unsigned long rate_value = 396000000;
    rt_uint32_t i;
    rt_uint32_t ret = 0;

    for (i = 0; i < fh_clock_cfg_count; i++)
    {
        p = &fh_clk_list[i];
        if (!rt_strcmp(p->name, "uart0_clk"))
            continue;


        ret = clk_set_rate(p, rate_value);
        if (ret == 0)
        {
            get = clk_get(NULL, p->name);
            if ((get != RT_NULL) && (get->clk_out_rate == rate_value))
                rt_kprintf("set %s clk rate :%d, success!\r\n", get->name, get->clk_out_rate);
            else
                rt_kprintf("set %s clk rate fail:%d!=%d!\r\n", p->name, p->clk_out_rate, rate_value);
        }
    }
}

void test_clk_get(void)
{
    struct clk *p = RT_NULL;
    rt_uint32_t i;

    for (i = 0; i < fh_clock_cfg_count; i++)
    {
        p = clk_get(NULL, fh_clk_list[i].name);
        clk_enable(p);
        if (p != RT_NULL)
            rt_kprintf("[%-16.15s]\t\t[baud]:%d\t\t[mux]:%5d\t\t[div]:%5d\t\t[gate]:%5d\t\n",
            p->name, (p->clk_out_rate)/1000000, p->clk_mux.num, p->clk_div.div, p->clk_gate.value);
        else
            rt_kprintf("can not find %s clock\r\n", fh_clk_list[i].name);

        clk_disable(p);
        rt_kprintf("[%-16.15s]\t\t[baud]:%d\t\t[mux]:%5d\t\t[div]:%5d\t\t[gate]:%5d\t\n",
        p->name, (p->clk_out_rate)/1000000, p->clk_mux.num, p->clk_div.div, p->clk_gate.value);

    }

}

void fh_clk_debug(void)
{
    struct clk *p;
    int i;
    rt_int32_t mux = -1, div, gate;
    rt_int32_t shift_num, reg_value;

    for (i = 0; i < fh_clock_cfg_count; i++)
    {
        p = &fh_clk_list[i];

        if (!!(p->flag&HIDE))
            continue;

        if (!!(p->flag&MUX))
        {
            reg_value = fh_pmu_get_reg(p->clk_mux.reg_offset);
            shift_num = __rt_ffs(p->clk_mux.mask)-1;
            mux = (reg_value&p->clk_mux.mask)>>shift_num;
            p->clk_mux.num = mux;
        }
        else
            mux =  -1;

        if (!!(p->flag&DIV))
        {
            reg_value = fh_pmu_get_reg(p->clk_div.reg_offset);
            shift_num = __rt_ffs(p->clk_div.mask)-1;
            div = (reg_value&p->clk_div.mask)>>shift_num;
            p->clk_div.div = div;
        }
        else
            div =  -1;

        if (!!(p->flag&GATE))
        {
            reg_value = fh_pmu_get_reg(p->clk_gate.reg_offset);
            shift_num = __rt_ffs(p->clk_gate.mask)-1;
            gate = (reg_value&p->clk_gate.mask)>>shift_num;
            p->clk_gate.value = gate;
        }
        else
            gate =  -1;
        rt_fh_clk_outrate_cal(p);
        rt_kprintf("[%-16.15s]\t\t[baud]:%d\t\t[mux]:%5d\t\t[div]:%5d\t\t[gate]:%5d\t\n",
        p->name, (p->clk_out_rate)/1000000, mux, div, gate);

    }

}


#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(fh_clk_debug, fh_clk_debug..);
/*FINSH_FUNCTION_EXPORT(test_clk_rate, test_clk_rate..);*/
/*FINSH_FUNCTION_EXPORT(test_clk_get, test_clk_get..);*/
#endif
