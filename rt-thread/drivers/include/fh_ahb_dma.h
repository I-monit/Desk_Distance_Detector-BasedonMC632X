/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-01-31     wangyl307    add license Apache-2.0
 */

#ifndef __FH_AHB_DMA_H__
#define __FH_AHB_DMA_H__

#include <rtthread.h>
#include "fh_arch.h"


struct dw_lli
{
    /* values that are not changed by hardware */
    rt_uint32_t sar;
    rt_uint32_t dar;
    rt_uint32_t llp; /* chain to next lli */
    rt_uint32_t ctllo;
    /* values that may get written back: */
    rt_uint32_t ctlhi;
    /* sstat and dstat can snapshot peripheral register state.
     * silicon config may discard either or both...
     */
    rt_uint32_t sstat;
    rt_uint32_t dstat;
    rt_uint32_t reserve;
};

int fh_dma_init(void);

#endif /* FH81_DMA_H_ */
