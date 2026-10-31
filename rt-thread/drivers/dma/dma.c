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

#include "dma.h"

static rt_err_t rt_dma_init(struct rt_device *dev);
static rt_err_t rt_dma_open(struct rt_device *dev, rt_uint16_t oflag);
static rt_err_t rt_dma_close(struct rt_device *dev);
static rt_err_t rt_dma_control(struct rt_device *dev, int cmd, void *args);

static rt_err_t rt_dma_init(struct rt_device *dev)
{
    struct rt_dma_device *dma;

    RT_ASSERT(dev != RT_NULL);
    dma = (struct rt_dma_device *)dev;
    if (dma->ops->init)
    {
        return dma->ops->init(dma);
    }

    return (-RT_ENOSYS);
}

static rt_err_t rt_dma_open(struct rt_device *dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_dma_close(struct rt_device *dev)
{
    struct rt_dma_device *dma;

    RT_ASSERT(dev != RT_NULL);
    dma = (struct rt_dma_device *)dev;

    if (dma->ops->control(dma, RT_DEVICE_CTRL_DMA_CLOSE, RT_NULL) != RT_EOK)
    {
        return (-RT_ERROR);
    }

    return (RT_EOK);
}

static rt_err_t rt_dma_control(struct rt_device *dev, int cmd, void *args)
{
    struct rt_dma_device *dma;

    RT_ASSERT(dev != RT_NULL);
    dma = (struct rt_dma_device *)dev;

    /* args is the private data for the soc!! */
    return (dma->ops->control(dma, cmd, args));
}


#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops dma_ops = {
    .init      = rt_dma_init,
    .open      = rt_dma_open,
    .close     = rt_dma_close,
    .read      = RT_NULL,
    .write     = RT_NULL,
    .control   = rt_dma_control,
};
#endif

/**
 * This function register a dma device
 */
rt_err_t rt_hw_dma_register(struct rt_dma_device *dma, const char *name,
                            rt_uint32_t flag, void *data)
{
    int ret;
    struct rt_device *device;

    RT_ASSERT(dma != RT_NULL);

    device = &(dma->parent);

    device->type        = RT_Device_Class_Miscellaneous;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;

#ifdef RT_USING_DEVICE_OPS
    device->ops = &dma_ops;
#else
    device->init      = rt_dma_init;
    device->open      = rt_dma_open;
    device->close     = rt_dma_close;
    device->read      = RT_NULL;
    device->write     = RT_NULL;
    device->control   = rt_dma_control;
#endif

    device->user_data = data;

    /* register a character device */
    ret = rt_device_register(device, name, flag);
    if (ret)
        rt_kprintf("dma register failed :%x\n", ret);
    return ret;
}
