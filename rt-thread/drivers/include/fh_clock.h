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

#ifndef __FH_CLOCK_H__
#define __FH_CLOCK_H__

#define   DIVVCO_ONE_DEVISION          0x0
#define   DIVVCO_TWO_DEVISION          0x8
#define   DIVVCO_FOUR_DEVISION         0xc
#define   DIVVCO_EIGHT_DEVISION        0xd
#define   DIVVCO_SIXTEEN_DEVISION      0xe
#define   DIVVCO_THIRTYTWO_DEVISION    0xf

#define   CLK_OK                0
#define   PARA_ERROR            1      /*parameter error*/
#define   CLK_UNDEFINED         2     /*can not find the clk module in list*/
#define   CLK_PARENT_UNCONFIG   3     /*the parent node of clk module unconfig*/
#define   CLK_NOT_SUPPORT_DIV   4
#define   CLK_PARENT_NOFIND     5
#define   CLK_DIV_PARA_ERROR    6
#define   CLK_DIV_VALUE_EXCEED_MUX    7

#ifdef CONFIG_ARCH_MC632X
enum gate_switch
{
        CLOSE,
        OPEN,
        DEFAULT
};
#else
enum gate_switch
{
        OPEN,
        CLOSE,
        DEFAULT
};
#endif

enum clk_config_flag
{
        PREDIV = 0x1,
        DIV = 0x2,
        GATE = 0x4,
        MUX = 0x8,
        CLOCK_PLL_P = 0x10,
        CLOCK_PLL_R = 0x20,
        CLOCK_FIXED = 0x40,
        HIDE = 0x80,
        CLOCK_CIS = 0x100,
        CLOCK_PLL = 0x400,
        RESET = 0x200,
        PHASE = 0x800,
        CLOCK_PLL2 = 0x1000,
        CLOCK_DDR = 0x2000,
};

enum clk_config
{
        UNCONFIG,
        CONFIG
};

struct clk_mux
{
        unsigned int reg_offset;
        unsigned int num;
        unsigned int mask;

};

struct clk_div
{
        unsigned int reg_offset;
        unsigned int div;
        unsigned int mask;
};

struct clk_gate
{
        unsigned int      reg_offset;
        enum gate_switch value;
        unsigned int      mask;
};
struct clk_reset
{
        unsigned int      reg_offset;
        unsigned int      mask;
};


#define CLOCK_MAX_PARENT    8
struct clk
{
        char *name;
        unsigned int     flag;
        unsigned int    clk_refsrc;
        unsigned int    is_aon_clk;
        unsigned int    aon_temp_type;
        unsigned int    granularity;
        unsigned int    pll_value;
        unsigned int    phase;
        struct clk_mux  clk_mux;
        struct clk_div    clk_div;
        struct clk_div    clk_div1;
        struct clk_gate clk_gate;
        struct clk_gate clk_gate1;
        struct clk_reset clk_reset;
        unsigned long     clk_out_rate;
        unsigned int     pre_div;
        enum clk_config clk_config;
        char *mult_parent[CLOCK_MAX_PARENT];
        unsigned int def_rate;
};
extern struct clk *clk_get(void *dev, const char *name);
extern int clk_set_rate(struct clk *clk, unsigned long rate);
extern unsigned long clk_get_rate(struct clk *clk);
extern int clk_enable(struct clk *clk);
extern void clk_disable(struct clk *clk);
extern int fh_clk_set_phase(struct clk *clk, int phase);
extern void fh_clk_reset(struct clk *clk);
extern int clk_is_enabled(struct clk *p_clk);
int rt_clk_dev_init(void);
int fh_clk_get_phase(struct clk *clk);

#endif
