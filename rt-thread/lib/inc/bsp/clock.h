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

#ifndef __CLOCK_H__
#define __CLOCK_H__

struct clk_usr
{
    char            *name;
    unsigned long           frequency;
};

#define SET_CLK_RATE            2
#define GET_CLK_RATE            3
#define CLK_ENABLE              4
#define CLK_DISABLE             5
#define SET_CLK_PHASE           6
#define GET_CLK_PHASE           7


#endif /* CLOCK_H_ */
