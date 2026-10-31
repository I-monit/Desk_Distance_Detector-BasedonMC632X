#include <rtthread.h>
#ifdef RT_USING_POSIX
#include <dfs_posix.h>
#include <dfs_poll.h>

#ifdef RT_USING_POSIX_TERMIOS
#include <posix_termios.h>
#endif

/* it's possible the 'getc/putc' is defined by stdio.h in gcc/newlib. */
#ifdef getc
#undef getc
#endif

#ifdef putc
#undef putc
#endif

extern struct rt_semaphore sem_null;

static rt_err_t null_fops_rx_ind(rt_device_t dev, rt_size_t size)
{
    rt_sem_release(&sem_null);

    return RT_EOK;
}

/* fops for nulldev */
static int null_fops_open(struct dfs_fd *fd)
{
    rt_err_t ret = 0;
    rt_uint16_t flags = 0;
    rt_device_t device;

    device = (rt_device_t)fd->data;
    RT_ASSERT(device != RT_NULL);

    switch (fd->flags & O_ACCMODE)
    {
    case O_RDONLY:
        flags = RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RDONLY;
        break;
    case O_WRONLY:
        flags = RT_DEVICE_FLAG_WRONLY;
        break;
    case O_RDWR:
        flags = RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_RDWR;
        break;
    default:
        break;
    }

    if ((fd->flags & O_ACCMODE) != O_WRONLY)
        rt_device_set_rx_indicate(device, null_fops_rx_ind);
    ret = rt_device_open(device, flags);
    if (ret == RT_EOK)
        return 0;

    return ret;
}

static int null_fops_close(struct dfs_fd *fd)
{
    rt_device_t device;

    device = (rt_device_t)fd->data;

    rt_device_set_rx_indicate(device, RT_NULL);
    rt_device_close(device);
    return 0;
}

static int null_fops_ioctl(struct dfs_fd *fd, int cmd, void *args)
{
    return 0;
}

static int null_fops_read(struct dfs_fd *fd, void *buf, size_t count)
{
    while (1)
    {
        if (rt_sem_take(&sem_null, RT_WAITING_FOREVER) != RT_EOK)
            continue;
        else
        {
            rt_sem_detach(&sem_null);
        }

        break;
    }
    return 0;
}

static int null_fops_write(struct dfs_fd *fd, const void *buf, size_t count)
{
    return 0;
}

static int null_fops_poll(struct dfs_fd *fd, struct rt_pollreq *req)
{
    return 0;
}

const static struct dfs_file_ops _null_dev_fops = {
    null_fops_open,
    null_fops_close,
    null_fops_ioctl,
    null_fops_read,
    null_fops_write,
    RT_NULL, /* flush */
    RT_NULL, /* lseek */
    RT_NULL, /* getdents */
    null_fops_poll,
};
#endif

static rt_err_t null_open(rt_device_t dev, rt_uint16_t oflag)
{
    return RT_EOK;
}

static rt_err_t null_close(rt_device_t dev)
{
    return RT_EOK;
}

static rt_size_t null_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t count)
{
    return 0;
}

static rt_size_t null_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    return 0;
}

static rt_err_t null_control(rt_device_t dev, int cmd, void *args)
{
    return 0;
}

static struct rt_device _null_dev;

rt_err_t rt_nulldev_register(void)
{
    rt_err_t err;

    _null_dev.type      = RT_Device_Class_Char;
    _null_dev.open      = null_open;
    _null_dev.close     = null_close;
    _null_dev.read      = null_read;
    _null_dev.write     = null_write;
    _null_dev.control   = null_control;

    _null_dev.user_data = RT_NULL;

    err = rt_device_register(&_null_dev, "null_dev", RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STREAM);
    if (err != RT_EOK) return err;

#if defined(RT_USING_POSIX)
    /* set fops */
    _null_dev.fops        = &_null_dev_fops;
#endif

    return RT_EOK;
}
