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

#include "mol_wdt.h"
#include "fh_clock.h"

static void wdt_write_reg(struct mol_wdt_obj *wdt, unsigned int offset, unsigned int val, unsigned int mask)
{
    unsigned int reg = GET_REG(wdt->base + offset);

    reg &= ~mask;
    reg |= val;
    SET_REG(wdt->base + WDOG_OP_LOCK_OFFSET, WDOG_UNLOCK_KEY);
    SET_REG(wdt->base + offset, reg);
    SET_REG(wdt->base + WDOG_OP_LOCK_OFFSET, 0);
}

void WDT_Enable(struct mol_wdt_obj *wdt)
{
    unsigned long load_count, data, reg;

    data        = WDOG_RUN | WDOG_NEW;
    load_count  = wdt->timeout * wdt->clk->clk_out_rate;

    data |= WDOG_RST_EN;
    data &= ~WDOG_IRQ_EN;

    wdt_write_reg(wdt, WDOG_CTRL_OFFSET, data, 0xFFFFFFFF);
    wdt_write_reg(wdt, WDOG_LOAD_VALUE_OFFSET,
                    load_count, 0xFFFFFFFF);
    wdt_write_reg(wdt, WDOG_INT_CLEAR_OFFSET, 0x1, 0x1);
    wdt_write_reg(wdt, WDOG_IRQ_VALUE_OFFSET, 0, 0xFFFFFFFF);

    /*do
    {
        reg = GET_REG(wdt->base + WDOG_RAW_INT_OFFSET);
    }while (reg & WDOG_WAIT_SYNC);*/
}

void WDT_Disable(struct mol_wdt_obj *wdt)
{
    wdt_write_reg(wdt, WDOG_CTRL_OFFSET, 0, WDOG_RUN);
}

void WDT_Reset(struct mol_wdt_obj *wdt)
{
    wdt_write_reg(wdt, WDOG_LOAD_VALUE_OFFSET,
                    wdt->timeout * wdt->clk->clk_out_rate, 0xFFFFFFFF);
}

void WDT_Settimeout(struct mol_wdt_obj *wdt, unsigned int new_time)
{
    wdt->timeout = new_time;
    WDT_Reset(wdt);
}

unsigned int Wdt_Gettimeout(struct mol_wdt_obj *wdt)
{
    return wdt->timeout;
}

unsigned int Wdt_Gettimeleft(struct mol_wdt_obj *wdt)
{
    int time_left;

    time_left = GET_REG(wdt->base + WDOG_CNT_READ_OFFSET);
    return time_left / wdt->clk->clk_out_rate;
}