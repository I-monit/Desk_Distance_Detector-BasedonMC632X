
#include <rtthread.h>
#include "mstorage.h"

#ifdef RT_USING_RAM_BLKDEV


#define RAMDEV_MIN_SIZE 0x100000

struct ram_blk_device
{
    struct rt_device dev;
    struct rt_device_blk_geometry geometry;

    void *ram_addr;
    rt_size_t ram_size;
};

static struct ram_blk_device   ram_dev;

static rt_err_t rt_ramdev_init(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_ramdev_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t rt_ramdev_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_err_t rt_ramdev_control(rt_device_t dev, int cmd, void *args)
{
    struct ram_blk_device *blk_dev = (struct ram_blk_device *)dev->user_data;

    switch (cmd)
    {
    case RT_DEVICE_CTRL_BLK_GETGEOME:
        rt_memcpy(args, &blk_dev->geometry, sizeof(struct rt_device_blk_geometry));
        break;

    default:
        break;
    }
    return RT_EOK;
}


#define DFS_STATUS_OK            0           /* no error */
#define DFS_STATUS_ENOENT        ENOENT      /* No such file or directory */
#define DFS_STATUS_EIO           EIO         /* I/O error */
#define DFS_STATUS_ENXIO         ENXIO       /* No such device or address */
#define DFS_STATUS_EBADF         EBADF       /* Bad file number */
#define DFS_STATUS_EAGAIN        EAGAIN      /* Try again */
#define DFS_STATUS_ENOMEM        ENOMEM      /* no memory */
#define DFS_STATUS_EBUSY         EBUSY       /* Device or resource busy */
#define DFS_STATUS_EEXIST        EEXIST      /* File exists */
#define DFS_STATUS_EXDEV         EXDEV       /* Cross-device link */
#define DFS_STATUS_ENODEV        ENODEV      /* No such device */
#define DFS_STATUS_ENOTDIR       ENOTDIR     /* Not a directory */
#define DFS_STATUS_EISDIR        EISDIR      /* Is a directory */
#define DFS_STATUS_EINVAL        EINVAL      /* Invalid argument */
#define DFS_STATUS_ENOSPC        ENOSPC      /* No space left on device */
#define DFS_STATUS_EROFS         EROFS       /* Read-only file system */
#define DFS_STATUS_ENOSYS        ENOSYS      /* Function not implemented */
#define DFS_STATUS_ENOTEMPTY     ENOTEMPTY   /* Directory not empty */


static rt_size_t rt_ramdev_read(rt_device_t dev,
                               rt_off_t    pos,
                               void       *buffer,
                               rt_size_t   size)
{
    struct ram_blk_device *blk_dev = (struct ram_blk_device *)dev->user_data;

    if (dev == RT_NULL)
    {
        rt_set_errno(-DFS_STATUS_EINVAL);
        return 0;
    }
    if(pos + size > blk_dev->geometry.sector_count)
    {
        rt_set_errno(-DFS_STATUS_EIO);
        return 0;
    }
    rt_memcpy(buffer, blk_dev->ram_addr + pos * blk_dev->geometry.bytes_per_sector, size * blk_dev->geometry.bytes_per_sector);

    return size;
}

static rt_size_t rt_ramdev_write(rt_device_t dev,
                                rt_off_t    pos,
                                const void *buffer,
                                rt_size_t   size)
{
    struct ram_blk_device *blk_dev = (struct ram_blk_device *)dev->user_data;

    if (dev == RT_NULL)
    {
        rt_set_errno(-DFS_STATUS_EINVAL);
        return 0;
    }
    // rt_kprintf("%s:%d\n", __func__, __LINE__);
    if(pos + size > blk_dev->geometry.sector_count)
    {
        rt_set_errno(-DFS_STATUS_EIO);
        return 0;
    }
    rt_memcpy(blk_dev->ram_addr + pos * blk_dev->geometry.bytes_per_sector, buffer, size * blk_dev->geometry.bytes_per_sector);
    return size;
}

rt_err_t rt_ramdev_create(const char *name, void *ram_buf, unsigned int ram_size)
{
    struct rt_device *device = NULL;

    if((NULL == ram_buf) || (ram_size % 512 != 0) || ram_size < RAMDEV_MIN_SIZE)
    {
        rt_kprintf("rt_ramdev_create fail! ram = %08x, size = %d\n", ram_buf, ram_size);
        return -1;
    }

    device = &ram_dev.dev;

    device->type        = RT_Device_Class_Block;
    device->rx_indicate = RT_NULL;
    device->tx_complete = RT_NULL;

    device->init        = rt_ramdev_init;
    device->open        = rt_ramdev_open;
    device->close       = rt_ramdev_close;
    device->read        = rt_ramdev_read;
    device->write       = rt_ramdev_write;
    device->control     = rt_ramdev_control;
    device->user_data   = &ram_dev;

    ram_dev.ram_addr = ram_buf;
    ram_dev.ram_size = ram_size;
    ram_dev.geometry.bytes_per_sector = 512;
    ram_dev.geometry.block_size = 512;
    ram_dev.geometry.sector_count = ram_size / ram_dev.geometry.bytes_per_sector - 1;
    /* register a character device */
    return rt_device_register(device, name, 0);
}

#endif

