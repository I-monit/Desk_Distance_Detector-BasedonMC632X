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

#ifndef __DMA_H__
#define __DMA_H__
#include <rtthread.h>

#define RT_DEVICE_CTRL_DMA_OPEN (1)
#define RT_DEVICE_CTRL_DMA_CLOSE (2)
#define RT_DEVICE_CTRL_DMA_REQUEST_CHANNEL (3)
#define RT_DEVICE_CTRL_DMA_RELEASE_CHANNEL (4)
#define RT_DEVICE_CTRL_DMA_SINGLE_TRANSFER (5)

/* cyclic add func below.... */

#define RT_DEVICE_CTRL_DMA_CYCLIC_PREPARE (6)
#define RT_DEVICE_CTRL_DMA_CYCLIC_START (7)
#define RT_DEVICE_CTRL_DMA_CYCLIC_STOP (8)
#define RT_DEVICE_CTRL_DMA_CYCLIC_FREE (9)
#define RT_DEVICE_CTRL_DMA_PAUSE (10)
#define RT_DEVICE_CTRL_DMA_RESUME (11)
#define RT_DEVICE_CTRL_DMA_GET_DAR (12)
#define RT_DEVICE_CTRL_DMA_GET_SAR (13)
#define RT_DEVICE_CTRL_DMA_USR_DEF (14)


struct rt_dma_ops;
struct rt_dma_device
{
    /* the parent must be the fitst para.. */
    struct rt_device parent;
    struct rt_dma_ops *ops;
};

struct rt_dma_ops
{
    rt_err_t (*init)(struct rt_dma_device *dma);
    rt_err_t (*control)(struct rt_dma_device *dma, int cmd, void *arg);
};

rt_err_t rt_hw_dma_register(struct rt_dma_device *dma, const char *name,
                            rt_uint32_t flag, void *data);

#endif
