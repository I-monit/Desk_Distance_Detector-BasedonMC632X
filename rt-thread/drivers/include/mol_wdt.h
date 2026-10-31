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

#ifndef __MOL_WDT_H__
#define __MOL_WDT_H__

#include "fh_def.h"

#define WDT_DEFAULT_TIMEOUT        10
#define WDT_MIN_TIMEOUT            1

#define WDOG_LOAD_VALUE_OFFSET            0x00
#define WDOG_CTRL_OFFSET                0x04
#define WDOG_INT_CLEAR_OFFSET            0x08
#define WDOG_RAW_INT_OFFSET                0x0C
#define WDOG_INT_STATUS_OFFSET            0x10
#define WDOG_CNT_VALUE_OFFSET            0x14
#define WDOG_OP_LOCK_OFFSET                0x18
#define WDOG_CNT_READ_OFFSET            0x1C
#define WDOG_IRQ_VALUE_OFFSET            0x20
#define WDOG_UNLOCK_KEY                    (0x1ACCE551)

//debug
#define WDT_CLK 30000

/*
 * WDT_CTRL Register
 */
#define WDOG_IRQ_EN                     BIT(0)
#define WDOG_RUN                        BIT(1)
#define WDOG_NEW                        BIT(2)
#define WDOG_RST_EN                     BIT(3)
#define WDOG_WAIT_SYNC                  BIT(4)

struct mol_wdt_obj
{
    int id;
    int irq;
    int rst;
    unsigned int base;
    struct clk *clk;
    unsigned int timeout;
    int is_enabled;
};

void WDT_Enable(struct mol_wdt_obj *wdt);
void WDT_Disable(struct mol_wdt_obj *wdt);
void WDT_Reset(struct mol_wdt_obj *wdt);
void WDT_Settimeout(struct mol_wdt_obj *wdt, unsigned int new_time);
unsigned int Wdt_Gettimeout(struct mol_wdt_obj *wdt);
unsigned int Wdt_Gettimeleft(struct mol_wdt_obj *wdt);


#endif