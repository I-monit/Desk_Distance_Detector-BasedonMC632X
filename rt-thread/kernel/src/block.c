/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
   2019-09-02     songyh        first version

 */

#include <rtthread.h>
#include <rtdevice.h>
#include <rtdef.h>
#include <linux/fs.h>
#include <dfs_file.h>

int DeviceIsBlock(rt_device_t dev)
{
    return dev->type & RT_Device_Class_Block;
}

int BlockDevice_Read(struct dfs_fd *file, const void *buf, rt_size_t count)
{
    rt_err_t ret;
    struct rt_device_blk_geometry geometry;
    char *temp_buf     = RT_NULL;
    char *p_buf        = (char *)buf;
    rt_size_t ret_size = 0;
    int read_size      = 0;
    rt_device_t device = file->data;

    ret = rt_device_control(device, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry);
    if (ret)
    {
        rt_kprintf("[Block Device]:%s get geometry failed\n", __FUNCTION__);
        return ret;
    }

    rt_uint32_t start_blk = file->pos64 / geometry.bytes_per_sector;
    rt_uint32_t start_off = file->pos64 % geometry.bytes_per_sector;
    rt_uint32_t end_blk   = (file->pos64 + count) / geometry.bytes_per_sector;
    rt_uint32_t end_off   = (file->pos64 + count) % geometry.bytes_per_sector;
    if (end_blk > geometry.sector_count || (end_blk == geometry.sector_count && end_off > 0))
    {
        rt_kprintf("[Block Device]: read size(%d) > dev size(%d)\n", end_blk, geometry.sector_count);
        return -RT_ENOMEM;
    }

    if (start_off == 0 && end_off == 0)
    {
        ret_size = rt_device_read(device, start_blk, (void *)buf, count / geometry.bytes_per_sector);

        if (ret_size < 0)
            return ret_size;

        ret_size = ret_size * geometry.bytes_per_sector;
        file->pos += ret_size;
        file->pos64 += ret_size;
        return ret_size;
    }

    temp_buf = rt_malloc(geometry.bytes_per_sector);
    if (temp_buf == RT_NULL)
    {
        rt_kprintf("[Block Device]: malloc fail\n");
        return -RT_ENOMEM;
    }

    ret = rt_device_read(device, start_blk, temp_buf, 1);
    if (ret <= 0)
    {
        rt_kprintf("[Block Device]: read block fail: %d\n", start_blk);
        rt_free(temp_buf);
        return -RT_ERROR;
    }

    if (start_blk == end_blk)
    {
        ret_size = end_off - start_off;
        rt_memcpy(p_buf, temp_buf + start_off, ret_size);
        rt_free(temp_buf);
        file->pos += ret_size;
        file->pos64 += ret_size;
        return ret_size;
    }

    read_size = geometry.bytes_per_sector - start_off;
    rt_memcpy(p_buf, temp_buf + start_off, read_size);
    p_buf += read_size;
    ret_size += read_size;
    if (end_blk - start_blk > 1)
    {
        ret = rt_device_read(device, start_blk + 1, p_buf, end_blk - start_blk - 1);
        if (ret <= 0)
        {
            rt_free(temp_buf);
            return ret_size;
        }
        read_size = (end_blk - start_blk - 1) * geometry.bytes_per_sector;
        p_buf += read_size;
        ret_size += read_size;
    }
    if (end_off > 0)
    {
        ret = rt_device_read(device, end_blk, temp_buf, 1);
        if (ret <= 0)
        {
            rt_kprintf("[Block Device]: read block fail: %d\n", end_blk);
            rt_free(temp_buf);
            return ret_size;
        }
        rt_memcpy(p_buf, temp_buf, end_off);
        ret_size += end_off;
    }

    file->pos += ret_size;
    file->pos64 += ret_size;
    rt_free(temp_buf);

    return ret_size;
}

int BlockDevice_Write(struct dfs_fd *file, const void *buf, rt_size_t count)
{
    rt_err_t ret;
    struct rt_device_blk_geometry geometry;
    char *temp_buf     = RT_NULL;
    char *p_buf        = (char *)buf;
    rt_size_t ret_size = 0;
    int write_size     = 0;
    rt_device_t device = file->data;

    ret = rt_device_control(device, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry);
    if (ret)
    {
        rt_kprintf("[Block Device]: %s, get geometry failed\n", __FUNCTION__);
        return -RT_EIO;
    }

    rt_uint32_t start_blk = file->pos64 / geometry.bytes_per_sector;
    rt_uint32_t start_off = file->pos64 % geometry.bytes_per_sector;
    rt_uint32_t end_blk   = (file->pos64 + count) / geometry.bytes_per_sector;
    rt_uint32_t end_off   = (file->pos64 + count) % geometry.bytes_per_sector;

    if (end_blk > geometry.sector_count || (end_blk == geometry.sector_count && end_off > 0))
    {
        rt_kprintf("[Block Device]: write size(%d) > dev size(%d)\n", end_blk, geometry.sector_count);
        return -RT_ENOMEM;
    }

    if (start_off == 0 && end_off == 0)
    {
        ret_size = rt_device_write(device, start_blk, buf, count / geometry.bytes_per_sector);
        if (ret_size < 0)
            return ret_size;

        ret_size = ret_size * geometry.bytes_per_sector;
        file->pos += ret_size;
        file->pos64 += ret_size;
        return ret_size;
    }

    temp_buf = rt_malloc(geometry.bytes_per_sector);
    if (temp_buf == RT_NULL)
    {
        rt_kprintf("[Block Device]: malloc fail\n");
        return -RT_ENOMEM;
    }
    ret = rt_device_read(device, start_blk, temp_buf, 1);
    if (ret <= 0)
    {
        rt_kprintf("[Block Device:%s]: read block fail: %d\n", __FUNCTION__, start_blk);
        rt_free(temp_buf);
        return -RT_ERROR;
    }

    if (start_blk == end_blk)
    {
        ret_size = end_off - start_off;

        rt_memcpy(temp_buf + start_off, p_buf, ret_size);

        ret = rt_device_write(device, start_blk, temp_buf, 1);
        if (ret <= 0)
        {
            rt_kprintf("[Block Device:%s]: write block fail: %d\n", __FUNCTION__, start_blk);
            ret_size = -RT_ERROR;
        }
        file->pos += ret_size;
        file->pos64 += ret_size;
        rt_free(temp_buf);
        return ret_size;
    }

    write_size = geometry.bytes_per_sector - start_off;
    rt_memcpy(temp_buf + start_off, p_buf, write_size);

    ret = rt_device_write(device, start_blk, temp_buf, 1);
    if (ret <= 0)
    {
        rt_kprintf("[Block Device:%s]: write block fail: %d\n", __FUNCTION__, start_blk);
        rt_free(temp_buf);
        return -RT_ERROR;
    }

    p_buf += write_size;
    ret_size += write_size;
    if (end_blk - start_blk > 1)
    {
        ret = rt_device_write(device, start_blk + 1, p_buf, end_blk - start_blk - 1);
        if (ret <= 0)
        {
            rt_kprintf("[Block Device:%s]: continue write block fail: %d\n", __FUNCTION__, (start_blk + 1));
            rt_free(temp_buf);
            return ret_size;
        }
        write_size = (end_blk - start_blk - 1) * geometry.bytes_per_sector;
        p_buf += write_size;
        ret_size += write_size;
    }
    if (end_off > 0)
    {
        ret = rt_device_read(device, end_blk, temp_buf, 1);
        if (ret <= 0)
        {
            rt_kprintf("[Block Device:%s]: read block fail: %d\n", __FUNCTION__, end_blk);
            rt_free(temp_buf);
            return ret_size;
        }
        rt_memcpy(temp_buf, p_buf, end_off);

        ret = rt_device_write(device, end_blk, temp_buf, 1);
        if (ret <= 0)
        {
            rt_kprintf("[Block Device:%s]: write block fail: %d\n", __FUNCTION__, end_blk);
            rt_free(temp_buf);
            return ret_size;
        }
        ret_size += end_off;
    }

    file->pos += ret_size;
    file->pos64 += ret_size;
    rt_free(temp_buf);

    return ret_size;
}

int BlockDevice_Open(struct dfs_fd *file)
{
    rt_err_t ret;
    struct rt_device_blk_geometry geometry;
    rt_uint32_t size;
    rt_device_t device    = file->data;

    ret = rt_device_control(device, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry);
    if (ret)
    {
        rt_kprintf("[Block Device]:%s get geometry failed\n", __FUNCTION__);
        return -RT_EIO;
    }

    ret = rt_device_control(device, BLKGETSIZE, &size);
    if (ret)
    {
        rt_kprintf("[Block Device]: %s, get blksize failed\n", __FUNCTION__);
        return -RT_EIO;
    }
    file->size64 = (rt_off64_t)(geometry.bytes_per_sector) * size;
    file->size   = (geometry.bytes_per_sector) * size;

    return 0;
}

#ifdef RT_USING_POSIX
int blk_fops_open(struct dfs_fd *file)
{
    rt_err_t ret;
    rt_device_t device;

    device = (rt_device_t)file->data;
    RT_ASSERT(device != RT_NULL);

    ret = rt_device_open(device, RT_DEVICE_OFLAG_RDWR);
    /* rt_device_open always returns RT_EOK */
    if (ret == RT_EOK)
    {
        /* extern int BlockDevice_Open(struct dfs_fd *file);    */

        return BlockDevice_Open(file);
    }

    return ret;
}
int blk_fops_read(struct dfs_fd *file, const void *buf, rt_size_t count)
{
    return BlockDevice_Read(file, buf, count);
}
int blk_fops_write(struct dfs_fd *file, const void *buf, rt_size_t count)
{
    return BlockDevice_Write(file, buf, count);
}
#endif
