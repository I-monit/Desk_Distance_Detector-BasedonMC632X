/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
   2019-09-02     songyh        first version

 */

#ifndef __BLOCK_H__
#define __BLOCK_H__

#include <rtthread.h>
#include <rtdevice.h>
#include <rtdef.h>
#include <linux/fs.h>
#include <dfs_file.h>

#ifdef RT_USING_POSIX
#include <dfs_posix.h>
#include <dfs_poll.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
/*
 * int DeviceIsBlock(rt_device_t dev);
 * int BlockDevice_Read(struct dfs_fd *file, const void *buf, rt_size_t count);
 * int BlockDevice_Write(struct dfs_fd *file, const void *buf, rt_size_t count);
 * int BlockDevice_Open(struct dfs_fd *file);
 */

/*
 * block device special implementation
 */
int blk_fops_open(struct dfs_fd *file);
int blk_fops_read(struct dfs_fd *file, void *buf, size_t count);
int blk_fops_write(struct dfs_fd *file, const void *buf, size_t count);

/*
 * dfs device default op
 */
extern int dfs_device_fs_close(struct dfs_fd *file);
extern int dfs_device_fs_poll(struct dfs_fd *file, struct rt_pollreq *req);
extern int dfs_device_fs_lseek(struct dfs_fd *file, rt_off_t offset);
extern rt_off64_t dfs_device_fs_lseek64(struct dfs_fd *file, rt_off64_t offset);
extern int dfs_device_fs_ioctl(struct dfs_fd *file, int cmd, void *args);

static struct dfs_file_ops _blk_fops = {
    .open = blk_fops_open,
    .read = blk_fops_read,
    .write = blk_fops_write,

    .close = dfs_device_fs_close,
    .ioctl = dfs_device_fs_ioctl,
    .lseek = dfs_device_fs_lseek,
    .poll = dfs_device_fs_poll,
    .lseek64 = dfs_device_fs_lseek64
};
#ifdef __cplusplus
}
#endif

#endif
