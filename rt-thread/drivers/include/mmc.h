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

#ifndef __MMC_H__
#define __MMC_H__

// #include "Libraries/inc/fh_driverlib.h"
#include <rtthread.h>
#include <fh_mmc.h>
#include "rtdevice.h"
#define MMC_FEQ_MIN 100000
#define MMC_FEQ_MAX 50000000

#define CARD_UNPLUGED 1
#define CARD_PLUGED 0

struct mmc_driver
{
    MMC_DMA_Descriptors *dma_descriptors;
    rt_uint32_t max_desc;
    struct rt_mmcsd_host *host;
    struct rt_mmcsd_req *req;
    struct rt_mmcsd_data *data;
    struct rt_mmcsd_cmd *cmd;
    struct rt_completion transfer_completion;
#ifdef SD1_MULTI_SLOT
    rt_uint32_t slotid;
#endif
    void *priv;
};

void rt_hw_mmc_init(void);

#endif /* MMC_H_ */
