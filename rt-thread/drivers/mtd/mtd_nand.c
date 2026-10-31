/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2011-12-05     Bernard      the first version
 */

/*
 * COPYRIGHT (C) 2012, Shanghai Real Thread
 */

#include <drivers/mtd_nand.h>
#include <spi_flash.h>
#include <fal.h>
#ifdef RT_USING_POSIX
#include <drivers/block.h>
#endif

#ifdef FH_USING_NAND_FLASH
#include <spi-nand.h>
#endif

#ifdef RT_USING_MTD_NAND

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
    struct rt_mtd_nand_device *mtd_device = RT_MTD_NAND_DEVICE(dev);
    struct fal_partition *fal_part = (struct fal_partition *)mtd_device->user_data;
    const struct fal_flash_dev *flash_dev = NULL;
    struct mtd_info *mtd = NULL;
    rt_off_t fal_pos = 0;
    size_t retlen = 0;
    int result;

    flash_dev = (struct fal_flash_dev *)fal_part->user_data;
    mtd = (struct mtd_info *)flash_dev->user_data;

    fal_pos = pos / mtd_device->page_size * mtd_device->page_size;
    result = mtd->_read(mtd,fal_part->offset + fal_pos,size,&retlen,buffer);
    if (result < 0)
    {
        return result;
    }
    return retlen;
}

static rt_size_t _mtd_block_read(rt_device_t dev,
                           rt_off_t    pos,
                           void       *buffer,
                           rt_size_t   size)
{
    struct rt_mtd_nand_device *mtd_device = RT_MTD_NAND_DEVICE(dev);
    struct fal_partition *fal_part = (struct fal_partition *)mtd_device->user_data;
    const struct fal_flash_dev *flash_dev = NULL;
    struct mtd_info *mtd = NULL;
    rt_off_t fal_pos = 0;
    size_t retlen = 0;
    int result;

    flash_dev = (struct fal_flash_dev *)fal_part->user_data;
    mtd = (struct mtd_info *)flash_dev->user_data;

    fal_pos = pos * mtd_device->block_size;
    size = size * mtd_device->block_size;

    result = mtd->_read(mtd, fal_part->offset + fal_pos, size, &retlen, buffer);
    if (result < 0)
    {
        return result;
    }
    return (rt_size_t)(retlen/mtd_device->block_size);
}

static rt_size_t _mtd_write(rt_device_t dev,
                            rt_off_t    pos,
                            const void *buffer,
                            rt_size_t   size)
{
    struct rt_mtd_nand_device *mtd_device = RT_MTD_NAND_DEVICE(dev);
    struct fal_partition *fal_part = (struct fal_partition *)mtd_device->user_data;
    const struct fal_flash_dev *flash_dev = NULL;
    struct mtd_info *mtd = NULL;
    rt_off_t fal_pos = 0;
    size_t retlen = 0;
    int result;

    flash_dev = (struct fal_flash_dev *)fal_part->user_data;
    mtd = (struct mtd_info *)flash_dev->user_data;

    fal_pos = pos / mtd_device->page_size * mtd_device->page_size;

    result = mtd->_write(mtd, fal_part->offset + fal_pos, size, &retlen, buffer);
    if (result < 0)
    {
        return result;
    }
    return retlen;

}

static rt_size_t _mtd_block_write(rt_device_t dev,
                            rt_off_t    pos,
                            const void *buffer,
                            rt_size_t   size)
{
    struct rt_mtd_nand_device *mtd_device = RT_MTD_NAND_DEVICE(dev);
    struct fal_partition *fal_part = (struct fal_partition *)mtd_device->user_data;
    const struct fal_flash_dev *flash_dev = NULL;
    struct mtd_info *mtd = NULL;
    rt_off_t fal_pos = 0;
    size_t retlen = 0;
    int result;

    flash_dev = (struct fal_flash_dev *)fal_part->user_data;
    mtd = (struct mtd_info *)flash_dev->user_data;

    fal_pos = pos * mtd_device->block_size;
    size = size * mtd_device->block_size;

    result = mtd->_write(mtd, fal_part->offset + fal_pos, size, &retlen, buffer);
    if (result < 0)
    {
        return result;
    }
    return (rt_size_t)(retlen/mtd_device->block_size);

}

static rt_err_t _mtd_control(rt_device_t dev, int cmd, void *args)
{
    struct mtd_info_user info;
    struct rt_device_blk_geometry *geometry;
    struct rt_mtd_nand_device *mtd_device = RT_MTD_NAND_DEVICE(dev);
    struct fal_partition *fal_part = (struct fal_partition *)mtd_device->user_data;
    const struct fal_flash_dev *flash_dev = NULL;
    struct mtd_info *mtd = NULL;
    struct erase_info_user *erase_info_user = (struct erase_info_user *)args;
    struct erase_info einfo = {0};
    int ret = 0;

    flash_dev = (struct fal_flash_dev *)fal_part->user_data;
    mtd = (struct mtd_info *)flash_dev->user_data;

    switch (cmd)
    {
        case RT_DEVICE_CTRL_BLK_GETGEOME:
            geometry = (struct rt_device_blk_geometry *)args;
            geometry->bytes_per_sector = mtd_device->block_size;
            geometry->block_size = mtd_device->block_size;
            geometry->sector_count = mtd_device->block_end;

            break;
        case MEMGETINFO:
            rt_memset(&info, 0, sizeof(struct mtd_info_user));

            info.type  = MTD_NANDFLASH;
            info.flags = MTD_CAP_NANDFLASH;
            info.size  = (mtd_device->block_end - mtd_device->block_start) * mtd_device->block_size;
            info.erasesize  = mtd_device->block_size;
            info.writesize  = mtd->sector_size;
            info.oobsize    = mtd->oobsize;
            info.ecctype    = -1;
            rt_memcpy(args, &info, sizeof(struct mtd_info_user));
            break;
        case MEMERASE:
            einfo.mtd = mtd;
            einfo.addr = fal_part->offset + erase_info_user->start;
            einfo.len = erase_info_user->length;
            ret = mtd->_erase(mtd, &einfo);
            break;
    }

    return ret;
}

rt_size_t rt_mtd_nand_read(
    struct rt_mtd_nand_device *device,
    rt_off_t offset, rt_uint8_t *data, rt_uint32_t length)
{
    rt_device_t dev;

    dev = &device->parent;
    return _mtd_read(dev, offset, data, length);
}

rt_size_t rt_mtd_nand_write(
    struct rt_mtd_nand_device *device,
    rt_off_t offset, rt_uint8_t *data, rt_uint32_t length)
{
    rt_device_t dev;

    dev = &device->parent;
    return _mtd_write(dev, offset, data, length);
}

rt_err_t rt_mtd_nand_register_device(const char                *name,
                                     struct rt_mtd_nand_device *device)
{
    rt_device_t dev;
    int ret = 0;

    dev = &device->parent;
    dev = RT_DEVICE(device);
    RT_ASSERT(dev != RT_NULL);

    /* set device class and generic device interface */
    dev->type        = RT_Device_Class_Block;
    dev->init        = _mtd_init;
    dev->open        = _mtd_open;
    dev->read        = _mtd_block_read;
    dev->write       = _mtd_block_write;
    dev->close       = _mtd_close;
    dev->control     = _mtd_control;

    dev->rx_indicate = RT_NULL;
    dev->tx_complete = RT_NULL;

    /* register to RT-Thread device system */
    ret = rt_device_register(dev, name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);

#ifdef RT_USING_POSIX
    if (ret == RT_EOK)
    {
        dev->fops = &_blk_fops;
    }
#endif

    return ret;
}

#if defined(RT_MTD_NAND_DEBUG) && defined(RT_USING_FINSH)
#include <finsh.h>
#define __is_print(ch) ((unsigned int)((ch) - ' ') < 127u - ' ')

static void mtd_dump_hex(const rt_uint8_t *ptr, rt_size_t buflen)
{
    unsigned char *buf = (unsigned char *)ptr;
    int i, j;

    for (i = 0; i < buflen; i += 16)
    {
        rt_kprintf("%06x: ", i);
        for (j = 0; j < 16; j++)
            if (i+j < buflen)
                rt_kprintf("%02x ", buf[i+j]);
            else
                rt_kprintf("   ");
        rt_kprintf(" ");
        for (j = 0; j < 16; j++)
            if (i+j < buflen)
                rt_kprintf("%c", __is_print(buf[i+j]) ? buf[i+j] : '.');
        rt_kprintf("\n");
    }
}

int mtd_nandid(const char *name)
{
    struct rt_mtd_nand_device *nand;
    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    return rt_mtd_nand_read_id(nand);
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nandid, nand_id, read ID - nandid(name));

int mtd_nand_read(const char* name, int block, int page)
{
    rt_err_t result;
    rt_uint8_t *page_ptr;
    rt_uint8_t *oob_ptr;
    struct rt_mtd_nand_device *nand;

    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    page_ptr = rt_malloc(nand->page_size + nand->oob_size);
    if (page_ptr == RT_NULL)
    {
        rt_kprintf("out of memory!\n");
        return -RT_ENOMEM;
    }

    oob_ptr = page_ptr + nand->page_size;
    rt_memset(page_ptr, 0xff, nand->page_size + nand->oob_size);

    /* calculate the page number */
    page = block * nand->pages_per_block + page;
    result = rt_mtd_nand_read(nand, page, page_ptr, nand->page_size,
        oob_ptr, nand->oob_size);

    rt_kprintf("read page, rc=%d\n", result);
    mtd_dump_hex(page_ptr, nand->page_size);
    mtd_dump_hex(oob_ptr, nand->oob_size);

    rt_free(page_ptr);
    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_read, nand_read, read page in nand - nand_read(name, block, page));

int mtd_nand_readoob(const char* name, int block, int page)
{
    struct rt_mtd_nand_device *nand;
    rt_uint8_t *oob_ptr;

    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    oob_ptr = rt_malloc(nand->oob_size);
    if (oob_ptr == RT_NULL)
    {
        rt_kprintf("out of memory!\n");
        return -RT_ENOMEM;
    }

    /* calculate the page number */
    page = block * nand->pages_per_block + page;
    rt_mtd_nand_read(nand, page, RT_NULL, nand->page_size,
        oob_ptr, nand->oob_size);
    mtd_dump_hex(oob_ptr, nand->oob_size);

    rt_free(oob_ptr);
    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_readoob, nand_readoob, read spare data in nand - nand_readoob(name, block, page));

int mtd_nand_write(const char* name, int block, int page)
{
    rt_err_t result;
    rt_uint8_t *page_ptr;
    rt_uint8_t *oob_ptr;
    rt_uint32_t index;
    struct rt_mtd_nand_device *nand;

    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    page_ptr = rt_malloc(nand->page_size + nand->oob_size);
    if (page_ptr == RT_NULL)
    {
        rt_kprintf("out of memory!\n");
        return -RT_ENOMEM;
    }

    oob_ptr = page_ptr + nand->page_size;
    /* prepare page data */
    for (index = 0; index < nand->page_size; index++)
    {
        page_ptr[index] = index & 0xff;
    }
    /* prepare oob data */
    for (index = 0; index < nand->oob_size; index++)
    {
        oob_ptr[index] = index & 0xff;
    }

    /* calculate the page number */
    page = block * nand->pages_per_block + page;
    result = rt_mtd_nand_write(nand, page, page_ptr, nand->page_size,
        oob_ptr, nand->oob_size);
    if (result != RT_MTD_EOK)
    {
        rt_kprintf("write page failed!, rc=%d\n", result);
    }

    rt_free(page_ptr);
    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_write, nand_write, write dump data to nand - nand_write(name, block, page));

int mtd_nand_erase(const char* name, int block)
{
    struct rt_mtd_nand_device *nand;

    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    return rt_mtd_nand_erase_block(nand, block);
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_erase, nand_erase, nand_erase(name, block));

int mtd_nand_erase_all(const char* name)
{
    rt_uint32_t index = 0;
    struct rt_mtd_nand_device *nand;

    nand = RT_MTD_NAND_DEVICE(rt_device_find(name));
    if (nand == RT_NULL)
    {
        rt_kprintf("no nand device found!\n");
        return -RT_ERROR;
    }

    for (index = 0; index < (nand->block_end - nand->block_start); index++)
    {
        rt_mtd_nand_erase_block(nand, index);
    }

    return 0;
}
FINSH_FUNCTION_EXPORT_ALIAS(mtd_nand_erase_all, nand_erase_all, erase all of nand device - nand_erase_all(name, block));
#endif

#endif
