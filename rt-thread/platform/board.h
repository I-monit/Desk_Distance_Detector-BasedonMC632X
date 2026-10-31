/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-04-09     songyh    the first version
 *
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <mmu.h>

void rt_hw_board_init(void);
void reset_timer(void);

unsigned int get_mmu_table(struct mem_desc **mtbl);

#endif
