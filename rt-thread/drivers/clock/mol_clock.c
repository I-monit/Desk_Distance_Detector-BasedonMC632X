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
extern void clk_change_parent(struct clk *clk, int select);

static struct clk *fh_clk_lists = fh_clk_list_low;
static unsigned int is_high_volt_mode;
static unsigned int fh_clk_cfg_count;

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

    read_value = GET_REG(clk->offset);
    write_value = (read_value&(~clk->mask))|(clk->value<<mask_shift_num);
    SET_REG(clk->offset, write_value);
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
        reg_value = GET_REG(p->clk_div.reg_offset);
        shift_num = __rt_ffs(p->clk_div.mask)-1;
        div = (reg_value&p->clk_div.mask)>>shift_num;
        p->clk_div.div = div;
    }

    if (!!(p->flag&GATE))
    {
        reg_value = GET_REG(p->clk_gate.reg_offset);
        shift_num = __rt_ffs(p->clk_gate.mask)-1;
        gate = (reg_value&p->clk_gate.mask)>>shift_num;
        p->clk_gate.value = gate;
    }

    if (!!(p->flag&MUX))
    {
        reg_value = GET_REG(p->clk_mux.reg_offset);
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

    reg = GET_REG(clk->clk_div.reg_offset);

    m = (reg >> 20) & 0x7f;
    n = (reg >> 8) & 0x1f;
    p = (reg >> 13) & 0x3f;

    /*pll databook*/
    if (m < 4)
        m = 128+m;

    if (m == 0xb)
        m = 0xa;

    shift = __rt_ffs(clk->clk_gate.mask)-1;
    reg = GET_REG(clk->clk_gate.reg_offset);

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

    reg = GET_REG(clk->clk_div.reg_offset);

    m = (reg >> 20) & 0x7f;
    n = (reg >> 8) & 0x1f;
    r = (reg >> 1) & 0x3f;

    /*pll databook*/
        if (m < 4)
            m = 128+m;

        if (m == 0xb)
            m = 0xa;

    shift = __rt_ffs(clk->clk_gate.mask)-1;
    reg = GET_REG(clk->clk_gate.reg_offset);

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

        ret = GET_REG(clk->clk_div.reg_offset);
        m = ret&0xff;
        n = ((ret&(0xf00))>>8);
        no = ((ret&(0x30000))>>16);
        clk->clk_out_rate = (OSC_FREQUENCY*m/n)>>no;
}

void fh_clk_get_pll2_rate(struct clk *clk)
{
    rt_uint32_t prediv, fbdiv;

    prediv = GET_REG(clk->clk_div.reg_offset) & clk->clk_div.mask;
    fbdiv = GET_REG(clk->clk_div1.reg_offset) & clk->clk_div1.mask;
    clk->clk_out_rate = OSC_FREQUENCY * fbdiv / (prediv + 1);
}

#define PREDIV_MASK  (0x7)
#define POSTDIV_MASK (0x7000000)
#define DIV_S_MASK   (0x8000000)

void fh_clk_get_ddr_rate(struct clk *clk)
{
    rt_uint32_t div_reg, div_reg1, prediv, postdiv, div_s, fbdiv;
    rt_uint64_t temp_rate;

    div_reg = GET_REG(clk->clk_div.reg_offset) & clk->clk_div.mask;
    div_reg1 = GET_REG(clk->clk_div1.reg_offset) & clk->clk_div1.mask;

    prediv = div_reg & PREDIV_MASK;
    postdiv = (div_reg & POSTDIV_MASK) >> 24;
    div_s = (div_reg & DIV_S_MASK) >> 27;
    fbdiv = div_reg1;

    if (div_s)
    {
        rt_kprintf("ddr_clk: div_s = 0x%x, it should be 0\n");
        clk->clk_out_rate = 0;
        return;
    }

    temp_rate = ((rt_uint64_t)OSC_FREQUENCY * fbdiv) / (prediv + 1);

    if (((postdiv & 0x6) >> 1) == 0x0)
        postdiv = 2;
    else if (((postdiv & 0x6) >> 1) == 0x1)
        postdiv = 4;
    else if (((postdiv & 0x4) >> 2) == 0x1)
        postdiv = 8;
    else
    {
        rt_kprintf("error ddr postdiv\n");
        clk->clk_out_rate = 0;
        return;
    }
    clk->clk_out_rate = (rt_uint32_t)temp_rate / postdiv;
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
    }
    else if (cur_config->flag & CLOCK_PLL2)
    {
        fh_clk_get_pll2_rate(cur_config);
        return CLK_OK;
    }
    else if (cur_config->flag & CLOCK_DDR)
    {
        fh_clk_get_ddr_rate(cur_config);
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

    for (i = 0; i < fh_clk_cfg_count; i++)
    {
        parent_config = &fh_clk_lists[i];
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

    is_high_volt_mode = GET_REG(CEN_GLB_APB_REG_BASE + 0x514)
                    & 0x10000000;

    if (is_high_volt_mode)
    {
        rt_kprintf("clk in vol high mode\n");
        fh_clk_lists = fh_clk_list_high;
        fh_clk_cfg_count = fh_clock_cfg_count_high;
    } else
    {
        rt_kprintf("clk in vol low mode\n");
        fh_clk_lists = fh_clk_list_low;
        fh_clk_cfg_count = fh_clock_cfg_count_low;
    }

    for (i = 0; i < fh_clk_cfg_count; i++)
    {
        p = &fh_clk_lists[i];

        if (p->clk_config == CONFIG)
            rt_hw_clock_config(p);
        else
            fh_clk_sync(p);
    }

    for (i = 0; i < fh_clk_cfg_count; i++)
    {
        p = &fh_clk_lists[i];
        rt_fh_clk_outrate_cal(p);
        if (p->def_rate)
        {
            if (clk_set_rate(p, p->def_rate))
                rt_kprintf("clk_set_rate(%s, %d) error\n", p->name, p->def_rate);
        }
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
    if (p_clk->clk_gate1.reg_offset != 0)
    {
        local_reg_cfg.offset = p_clk->clk_gate1.reg_offset;
        local_reg_cfg.value = p_clk->clk_gate1.value = OPEN;
        local_reg_cfg.mask = p_clk->clk_gate1.mask;
        rt_fh_clock_config(&local_reg_cfg);
    }

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
    if (p_clk->clk_gate1.reg_offset != 0)
    {
        local_reg_cfg.offset = p_clk->clk_gate1.reg_offset;
        local_reg_cfg.value = p_clk->clk_gate1.value = CLOSE;
        local_reg_cfg.mask = p_clk->clk_gate1.mask;
        rt_fh_clock_config(&local_reg_cfg);
    }

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

    enable = ((GET_REG(p_clk->clk_gate.reg_offset) & p_clk->clk_gate.mask) == (OPEN?p_clk->clk_gate.mask:0));
    if (enable && (p_clk->clk_gate1.reg_offset != 0))
        enable = ((GET_REG(p_clk->clk_gate1.reg_offset) & p_clk->clk_gate1.mask) == (OPEN?p_clk->clk_gate1.mask:0));
    rt_hw_interrupt_enable(level);

    return enable;
}

int clk_set_rate(struct clk *p_clk, unsigned long rate)
{
    rt_uint32_t i, j;
    struct clk *p = RT_NULL;
    struct clk *cur_config = RT_NULL;
    rt_uint32_t parent_rate[CLOCK_MAX_PARENT] = {0};
    rt_uint32_t div_value = 1;
    rt_uint32_t div_flag;

    cur_config = p_clk;
    if ((cur_config == RT_NULL) || (cur_config->name == RT_NULL))
    {
        rt_kprintf("input para error\r\n");
        return PARA_ERROR;
    }

    if (rate == 0)
    {
        rt_kprintf("%s:input para error\r\n",cur_config->name);
        return PARA_ERROR;

    }
    /*zt*/
    if ((cur_config->flag & CLOCK_CIS) && (cur_config->flag & MUX))
    {
        if (rate == OSC_FREQUENCY)
        {
            cur_config->clk_out_rate = OSC_FREQUENCY;
            cur_config->clk_mux.num = 0;
            rt_fh_clock_config((struct fh_clock_reg_cfg *)&cur_config->clk_mux);
            return CLK_OK;
        }
        else
            cur_config->clk_mux.num = 1;
    }
    for (i = 0; i < CLOCK_MAX_PARENT; i++)
    {
        if (cur_config->mult_parent[i] == 0)
            break;

        for (j = 0; j < fh_clk_cfg_count; j++)
        {
            p = &fh_clk_lists[j];
            if (!rt_strcmp(p->name, cur_config->mult_parent[i]))
            {
                parent_rate[i] = p->clk_out_rate;
                break;
            }
        }
    }
    if (cur_config->mult_parent[0] == 0)
        return CLK_OK;

    if ((cur_config->flag & PREDIV) == 0)
        cur_config->pre_div = 1;

    switch (cur_config->flag&(DIV|MUX))
    {
    case DIV:
        div_value = parent_rate[cur_config->clk_mux.num]
            / (cur_config->pre_div * rate) - 1;
        cur_config->clk_out_rate = rate;
        cur_config->clk_div.div = div_value;
        rt_hw_clock_config(cur_config);
        break;
    case MUX:
        for (i = 0; i < CLOCK_MAX_PARENT; i++)
        {
            if (parent_rate[i] == rate)
            {
                cur_config->clk_out_rate = rate;
                cur_config->clk_mux.num = i;
                rt_hw_clock_config(cur_config);
                break;
            }
        }
        break;
    case DIV|MUX:
        div_flag = parent_rate[cur_config->clk_mux.num]%
            (cur_config->pre_div*rate);
        if (div_flag == 0)
        {
            cur_config->clk_out_rate = rate;
            div_value = parent_rate[cur_config->clk_mux.num]
                / (cur_config->pre_div * rate) - 1;
            cur_config->clk_div.div = div_value;
            rt_hw_clock_config(cur_config);
            return CLK_OK;
        }
        else
        {
            for (i = 0; i < CLOCK_MAX_PARENT; i++)
            {
                if (parent_rate[i] != 0 &&
                    parent_rate[i]%(cur_config->pre_div * rate) == 0)
                {
                    div_value = parent_rate[i] /
                        (cur_config->pre_div * rate) - 1;
                    cur_config->clk_div.div = div_value;
                    cur_config->clk_out_rate = parent_rate[i] /
                        (div_value + 1) / cur_config->pre_div;
                    cur_config->clk_mux.num = i;
                    rt_hw_clock_config(cur_config);
                    return CLK_OK;
                }
            }
        }
        rt_kprintf("clock tree could not set the accurate rate\n ");
        break;
    default:
        return CLK_OK;
    }
    return CLK_OK;
}

struct clk *clk_get(void *dev, const char *name)
{
    rt_uint32_t i;
    struct clk *p = RT_NULL;

    if (name == RT_NULL)
    {
        return RT_NULL;
    }

    for (i = 0; i < fh_clk_cfg_count; i++)
    {
        p = &fh_clk_lists[i];
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
    SET_REG(clk->clk_reset.reg_offset, reg);
    reg = GET_REG(clk->clk_reset.reg_offset);
    while (reg != 0xffffffff)
        reg = GET_REG(clk->clk_reset.reg_offset);
}

int fh_clk_set_phase(struct clk *clk, int phase)
{
    return 0;
}


/*debug module*/


void test_clk_rate(void)
{
    struct clk *p = RT_NULL;
    struct clk *get = RT_NULL;
    unsigned long rate_value = 396000000;
    rt_uint32_t i;
    rt_uint32_t ret = 0;

    for (i = 0; i < fh_clk_cfg_count; i++)
    {
        p = &fh_clk_lists[i];
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
MSH_CMD_EXPORT(test_clk_rate, test fullhan test_clk_rate);

void test_clk_get(void)
{
    struct clk *p = RT_NULL;
    rt_uint32_t i;

    for (i = 0; i < fh_clk_cfg_count; i++)
    {
        p = clk_get(NULL, fh_clk_lists[i].name);
        clk_enable(p);
        if (p != RT_NULL)
            rt_kprintf("[%-16.15s]\t\t[baud]:%d\t\t[mux]:%5d\t\t[div]:%5d\t\t[gate]:%5d\t\n",
            p->name, (p->clk_out_rate)/1000000, p->clk_mux.num, p->clk_div.div, p->clk_gate.value);
        else
            rt_kprintf("can not find %s clock\r\n", fh_clk_lists[i].name);

        clk_disable(p);
        rt_kprintf("[%-16.15s]\t\t[baud]:%d\t\t[mux]:%5d\t\t[div]:%5d\t\t[gate]:%5d\t\n",
        p->name, (p->clk_out_rate)/1000000, p->clk_mux.num, p->clk_div.div, p->clk_gate.value);

    }

}

void fh_clk_debug(void)
{
    struct clk *p;
    int i;
    rt_int32_t mux = -1, div, gate0, gate1;
    rt_int32_t gate;
    rt_int32_t shift_num, reg_value;

    for (i = 0; i < fh_clk_cfg_count; i++)
    {
        p = &fh_clk_lists[i];

        if (!!(p->flag&HIDE))
            continue;

        if (!!(p->flag&MUX))
        {
            reg_value = GET_REG(p->clk_mux.reg_offset);
            shift_num = __rt_ffs(p->clk_mux.mask)-1;
            mux = (reg_value&p->clk_mux.mask)>>shift_num;
            p->clk_mux.num = mux;
        }
        else
            mux =  -1;

        if (!!(p->flag&DIV))
        {
            reg_value = GET_REG(p->clk_div.reg_offset);
            shift_num = __rt_ffs(p->clk_div.mask)-1;
            div = (reg_value&p->clk_div.mask)>>shift_num;
            p->clk_div.div = div;
        }
        else
            div =  -1;

        if (!!(p->flag&GATE))
        {
            reg_value = GET_REG(p->clk_gate.reg_offset);
            shift_num = __rt_ffs(p->clk_gate.mask)-1;
            gate0 = (reg_value&p->clk_gate.mask)>>shift_num;
            p->clk_gate.value = gate0;
            gate = gate0;
            if (p->clk_gate1.reg_offset)
            {
                reg_value = GET_REG(p->clk_gate1.reg_offset);
                shift_num = __rt_ffs(p->clk_gate1.mask)-1;
                gate1 = (reg_value&p->clk_gate1.mask)>>shift_num;
                p->clk_gate1.value = gate1;
#ifdef CONFIG_ARCH_MC632X
                gate = gate0 & gate1;
#else
                gate = gate0 | gate1;
#endif
            }
        }
        else
            gate =  -1;
        rt_fh_clk_outrate_cal(p);
        rt_kprintf("[%-16.15s]\t\t[baud]:%d\t\t[mux]:%5d\t\t[div]:%5d\t\t[gate]:%5d\t\n",
        p->name, (p->clk_out_rate)/1000000, mux, div, gate);

    }

}
MSH_CMD_EXPORT(fh_clk_debug, test fullhan clk);

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(fh_clk_debug, fh_clk_debug..);
/*FINSH_FUNCTION_EXPORT(test_clk_rate, test_clk_rate..);*/
/*FINSH_FUNCTION_EXPORT(test_clk_get, test_clk_get..);*/
#endif
