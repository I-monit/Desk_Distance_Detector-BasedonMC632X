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

#ifndef __FH_AXI_DMA_H__
#define __FH_AXI_DMA_H__



#include <rtthread.h>
#include "fh_arch.h"


struct  axi_dma_lli {
	rt_uint32_t		sar_lo;
	rt_uint32_t		sar_hi;
	rt_uint32_t		dar_lo;
	rt_uint32_t		dar_hi;
	rt_uint32_t		block_ts_lo;
	rt_uint32_t		block_ts_hi;
	rt_uint32_t		llp_lo;
	rt_uint32_t		llp_hi;
	rt_uint32_t		ctl_lo;
	rt_uint32_t		ctl_hi;
	rt_uint32_t		sstat;
	rt_uint32_t		dstat;
	rt_uint32_t		status_lo;
	rt_uint32_t		status_hi;
	rt_uint32_t		reserved_lo;
	rt_uint32_t		reserved_hi;
};

#endif /* FH81_DMA_H_ */
