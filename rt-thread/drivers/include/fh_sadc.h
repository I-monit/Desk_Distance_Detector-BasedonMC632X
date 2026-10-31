/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-07-01     tangyh    the first version
 *
 */

#ifndef __FH_SADC_H__
#define __FH_SADC_H__

#include <rtdef.h>

struct wrap_sadc_obj
{
    rt_uint32_t id;
    void *regs;
    rt_uint32_t irq_no;
	rt_uint32_t frequency;
	rt_uint32_t ref_vol;
	rt_uint32_t max_value;
	rt_uint32_t max_chan_no;
	
};

/****************************************************************************
 *  extern variable declaration section
 ***************************************************************************/

/****************************************************************************
 *  section
 *    add function prototype here if any
 ***************************************************************************/
void rt_hw_sadc_init(void);
#endif /* FH_SADC_H_ */
