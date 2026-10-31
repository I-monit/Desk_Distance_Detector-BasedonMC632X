/*
 * COPYRIGHT (C) 2018, Real-Thread Information Technology Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2012-5-30     Bernard      the first version
 */

#include <rtthread.h>
#include <drivers/mtd_nor.h>
#ifdef RT_USING_POSIX
#include <drivers/block.h>
#endif

#include "mtd/mtd-user.h"

#include "fal.h"

#ifdef PKG_USING_FCL
#include "fcl.h"
#endif

#ifdef RT_USING_MTD_NOR

/**
 * RT-Thread Generic Device Interface
 */
static rt_err_t _mtd_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t _mtd_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t _mtd_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t _mtd_read(rt_device_t dev,
                           rt_off_t    pos,
                           void       *buffer,
                           rt_size_t   size)
{
    struct rt_mtd_nor_device *mtd_device = RT_MTD_NOR_DEVICE(dev);
    struct fal_partition *fal_part = (struct fal_partition *)mtd_device->user_data;
    rt_off_t fal_pos = 0;
    rt_size_t fal_size = 0;
    int readsize = 0;

    fal_pos = pos * mtd_device->block_size;
    fal_size = size * mtd_device->block_size;

    readsize = fal_partition_read(fal_part, fal_pos, buffer, fal_size);
    if (readsize < 0)
        return 0;
    else
        return (rt_size_t)(readsize / mtd_device->block_size);
}

static rt_size_t _mtd_write(rt_device_t dev,
                            rt_off_t    pos,
                            const void *buffer,
                            rt_size_t   size)
{
    struct rt_mtd_nor_device *mtd_device = RT_MTD_NOR_DEVICE(dev);
    struct fal_partition *fal_part = (struct fal_partition *)mtd_device->user_data;
    rt_off_t fal_pos = 0;
    rt_size_t fal_size = 0;
    int writesize = 0;

    fal_pos   = pos * mtd_device->block_size;
    fal_size  = size * mtd_device->block_size;
    writesize = fal_partition_write(fal_part, fal_pos, buffer, fal_size);

    if (writesize < 0)
        return 0;
    else
        return (rt_size_t)(writesize / mtd_device->block_size);
}

static rt_err_t _mtd_control(rt_device_t dev, int cmd, void *args)
{
    struct mtd_info_user info;
    struct rt_device_blk_geometry mtd_geometry;
    struct rt_device_blk_geometry *geometry;
    struct erase_info_user einfo32;
    struct rt_mtd_nor_device *mtd_device = RT_MTD_NOR_DEVICE(dev);
    struct fal_partition *fal_part = (struct fal_partition *)mtd_device->user_data;
    switch (cmd)
    {
        case RT_DEVICE_CTRL_BLK_GETGEOME:
            geometry = (struct rt_device_blk_geometry *)args;

            mtd_geometry.bytes_per_sector = mtd_device->block_size;
            mtd_geometry.block_size = mtd_device->block_size;
            mtd_geometry.sector_count = mtd_device->block_end;

            rt_memcpy(geometry, &mtd_geometry, sizeof(struct rt_device_blk_geometry));
            break;
        case MEMGETINFO:
            rt_memset(&info, 0, sizeof(struct mtd_info_user));

            info.type  = MTD_NORFLASH;
            info.flags = MTD_CAP_NORFLASH;
            info.size  = (mtd_device->block_end - mtd_device->block_start) * mtd_device->block_size;
            info.erasesize  = mtd_device->block_size;
            info.writesize  = 1;
            info.oobsize    = 0;
            info.ecctype    = -1;

            rt_memcpy(args, &info, sizeof(struct mtd_info_user));
            break;
        case MEMERASE:
            rt_memcpy(&einfo32, args, sizeof(struct erase_info_user));
            int erasesize = fal_partition_erase(fal_part, einfo32.start, einfo32.length);

            if (erasesize != einfo32.length)
                return -RT_EIO;
            break;
    }

    return RT_EOK;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops mtd_nor_ops =
{
    .init = _mtd_init,
    .open = _mtd_open,
    .close = _mtd_close,
    .read = _mtd_read,
    .write = _mtd_write,
    .control = _mtd_control
};
#ifdef PKG_USING_FCL
const static struct rt_device_ops mtd_nor_cache_ops = {
    .init = mtd_device_cache_init,
    .open = mtd_device_cache_open,
    .close = mtd_device_cache_close,
    .read = mtd_device_cache_read,
    .write = mtd_device_cache_write,
    .control = mtd_device_cache_control
};
#endif
#endif

rt_err_t rt_mtd_nor_register_device(const char               *name,
                                    struct rt_mtd_nor_device *device)
{
    rt_err_t ret;
    rt_device_t dev;

    dev = &device->parent;
    RT_ASSERT(dev != RT_NULL);

    /* set device class and generic device interface */
    dev->type        = RT_Device_Class_Block;
#ifdef RT_USING_DEVICE_OPS
#ifndef PKG_USING_FCL
    dev->ops         = &mtd_nor_ops;
#else
    dev->ops         = &mtd_nor_cache_ops;
#endif
#else
#ifdef PKG_USING_FCL
    dev->init        = mtd_device_cache_init;
    dev->open        = mtd_device_cache_open;
    dev->read        = mtd_device_cache_read;
    dev->write       = mtd_device_cache_write;
    dev->close       = mtd_device_cache_close;
    dev->control     = mtd_device_cache_control;
#else
    dev->init        = _mtd_init;
    dev->open        = _mtd_open;
    dev->read        = _mtd_read;
    dev->write       = _mtd_write;
    dev->close       = _mtd_close;
    dev->control     = _mtd_control;
#endif
#endif

    dev->rx_indicate = RT_NULL;
    dev->tx_complete = RT_NULL;

    /* register to RT-Thread device system */
#ifdef MTD_MULTIPROCESS_ACCESS
    ret =  rt_device_register(dev, name, RT_DEVICE_FLAG_RDWR);
#else
    ret =  rt_device_register(dev, name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
#endif
#ifdef RT_USING_POSIX
    if (ret == RT_EOK)
    {
        dev->fops = &_blk_fops;
    }
#endif

    return ret;
}

#endif
