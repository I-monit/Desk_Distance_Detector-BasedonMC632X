/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2020-5-11       tangyh        the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_fs.h>
#include <dfs_file.h>
#include "dfs_yaffs2.h"
#include <string.h>
#include <yaffsfs.h>
#include <yaffs_osglue.h>
#include <yaffs_guts.h>
#include <yaffs_mtdif.h>

#include <drivers/mtd_nand.h>


static struct rt_mutex yaffs2_lock;

/*
 * config for yaffs2
 */
struct yaffs_options {
	int yaffs_version;
    int inband_tags;
    int skip_checkpoint_read;
    int skip_checkpoint_write;
    int no_cache;
    int tags_ecc_on;
    int tags_ecc_overridden;
    int lazy_loading_enabled;
    int lazy_loading_overridden;
    int empty_lost_and_found;
    int empty_lost_and_found_overridden;
    int disable_summary;
};
struct yaffs_options options =
{
    2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
static int dfs_yaffs2_mount(struct dfs_filesystem* fs,
                    unsigned long rwflag,
                    const void* data)
{
    int result;
    struct yaffs_dev *dev = 0;
    struct yaffs_param *param;
    struct rt_mtd_nand_device *mtd_dev;
    unsigned int skip_checkpt = 0;
    int n_blocks;

    mtd_dev = RT_MTD_NAND_DEVICE(fs->dev_id);
    dev = rt_malloc(sizeof(struct yaffs_dev));
    if(!dev)
        return -ENOMEM;

    rt_memset(dev, 0, sizeof(struct yaffs_dev));
    param = &(dev->param);

    dev->read_only = rwflag;
    dev->driver_context = mtd_dev;
    param->name = fs->path;//mtd_dev->parent.parent.name;
    fs->data = dev;
    param->n_reserved_blocks = 5;
    param->n_caches = (options.no_cache) ? 0 : 10;
    param->inband_tags = options.inband_tags;
    param->enable_xattr = 1;
    if (options.lazy_loading_overridden)
        param->disable_lazy_load = !options.lazy_loading_enabled;

    param->defered_dir_update = 1;

    if (options.tags_ecc_overridden)
        param->no_tags_ecc = !options.tags_ecc_on;

    param->empty_lost_n_found = 1;
    param->refresh_period = 500;
    param->disable_summary = options.disable_summary;

    if (options.empty_lost_and_found_overridden)
        param->empty_lost_n_found = options.empty_lost_and_found;

    /* ... and the functions. */
    if (options.yaffs_version == 2) {
        param->is_yaffs2 = 1;
        param->total_bytes_per_chunk = mtd_dev->page_size;
        param->chunks_per_block = mtd_dev->pages_per_block;
        n_blocks = mtd_dev->block_total;
        
        param->start_block = 0;
        param->end_block = n_blocks - 1;
    } else {
		param->is_yaffs2 = 0;
		n_blocks = mtd_dev->block_total * mtd_dev->pages_per_block
			* mtd_dev->page_size /
			YAFFS_CHUNKS_PER_BLOCK / YAFFS_BYTES_PER_CHUNK;

		param->chunks_per_block = YAFFS_CHUNKS_PER_BLOCK;
		param->total_bytes_per_chunk = YAFFS_BYTES_PER_CHUNK;
    }
    	
    param->start_block = 0;
    param->end_block = n_blocks - 1;
    
    yaffs_mtd_drv_install(dev);
    param->use_nand_ecc = 1;
	param->skip_checkpt_rd = options.skip_checkpoint_read;
	param->skip_checkpt_wr = options.skip_checkpoint_write;
	yaffs_add_device(dev);
	rt_kprintf("yaffs2:%s\n",fs->path);
    result = yaffs_mount(fs->path);
    return result;
}

static int dfs_yaffs2_unmount(struct dfs_filesystem* fs)
{
    int result;
	struct yaffs_dev *dev = 0;

    dev = fs->data;
	if(!dev)
	{
	    rt_kprintf("device has even umount\n\r");
	    return 0;
	}
	result = yaffs_unmount(fs->path);
	yaffs_remove_device(dev);
	rt_free(dev);
	fs->data = NULL;
	
    return result;
}

static int dfs_yaffs2_mkfs(rt_device_t dev_id)
{
    /* just erase all blocks on this nand partition */
    return -ENOSYS;
}

static int dfs_yaffs2_statfs(struct dfs_filesystem* fs,
                     struct statfs *buf)
{

    struct yaffs_dev *dev;
    int result;

    dev = fs->data;
    if(!dev)
        return -ENODEV;

    buf->f_bsize = dev->param.total_bytes_per_chunk * dev->param.chunks_per_block ;
    buf->f_blocks = dev->param.end_block - dev->param.start_block + 1;
    buf->f_bfree = yaffs_get_n_free_chunks(dev) * dev->param.total_bytes_per_chunk
		/buf->f_bsize;
    return 0;
}
static int dfs_yaffs2_open(struct dfs_fd* file)
{
    int result = 0;
    int oflag;
    const char * name;
    struct dfs_filesystem *fs;
	struct yaffs_dev *dev;
	struct yaffs_obj *root;
	int *  handle ;
	yaffs_DIR * dir = NULL;

    oflag = file->flags;
    fs = (struct dfs_filesystem *)file->fs;
    RT_ASSERT(fs != RT_NULL);

    dev = fs->data;
	root = dev->root_dir;
    name = file->path;
	if(oflag & O_DIRECTORY)
	{
		if (oflag & O_CREAT) /* create a dir*/
        {
            /* fixme, should test file->path can end with '/' */
            result = yaffs_mkdir(name, 0);
            if (result)
            {
                return result;
            }
        }
		dir = yaffs_opendir(name);
        if (!dir)
        {
            return -1;
        }
		file->data = dir;
		return 0;
	}
	handle = rt_malloc(sizeof(int));
    *handle = yaffs_open(name, oflag, 0);
	if(*handle < 0)
		result = *handle;
	file->data = handle;
    return (result);
}
static int dfs_yaffs2_close(struct dfs_fd* file)
{
    int result;
	int handle;
    yaffs_DIR *dir = NULL;
    if(file->data == NULL)
		return 0;
	if (file->flags & O_DIRECTORY) /* operations about dir */
    {
       dir = file->data;
       result = yaffs_closedir(dir);
	   return result;
	}
	handle = *(int*)(file->data);
    result = yaffs_close(handle);
	if(!result)
	{
		rt_free(file->data);
		file->data = NULL;
	}
	return result;
}

static int dfs_yaffs2_ioctl(struct dfs_fd* file, int cmd, void* args)
{
    return -ENOSYS;
}

static int dfs_yaffs2_read(struct dfs_fd* file, void* buf, size_t len)
{
    int result;
	int handle;

    RT_ASSERT(file->data != NULL);
    handle = *(int*)(file->data);
    result = yaffs_read(handle, buf, len);
    if (result < 0)
        return result;

    /* update position */
    file->pos += result;
    return result;
}

static int dfs_yaffs2_write(struct dfs_fd* file,
                    const void* buf,
                    size_t len)
{
    int handle;
    int result;

    RT_ASSERT(file->data != NULL);
    handle = *(int*)(file->data);

    result = yaffs_write(handle, buf, len);
    if (result < 0)
        return result;

    /* update position */
    file->pos += result;
    return result;
}

static int dfs_yaffs2_flush(struct dfs_fd* file)
{
    int handle;
	int result;

    RT_ASSERT(file->data != NULL);
    handle = *(int*)(file->data);

    result = yaffs_flush(handle);
    return result;
}

/* fixme warning: the offset is rt_off_t, so maybe the size of a file is must <= 2G*/
static int dfs_yaffs2_lseek(struct dfs_fd* file,
                    rt_off_t offset)
{
    int handle;
    int result;

    RT_ASSERT(file->data != NULL);
    handle = *(int*)(file->data);

    /* set offset as current offset */
    result = yaffs_lseek(handle, offset, SEEK_SET);
    if (result < 0)
        return result;
    /* update file position */
    file->pos = offset;
    return offset;
}

/* return the size of  struct dirent*/
static int dfs_yaffs2_getdents(struct dfs_fd* file,
                       struct dirent* dirp,
                       rt_uint32_t count)
{
    struct dirent * d;
    rt_uint32_t index;
    struct yaffs_dirent *result;
	yaffs_DIR *dir;

    RT_ASSERT(file->data != RT_NULL);
    dir = (yaffs_DIR *)(file->data);

    /* make integer count, usually count is 1 */
    count = (count / sizeof(struct dirent)) * sizeof(struct dirent);
    if (count == 0) return -EINVAL;

    index = 0;
    /* usually, the while loop should only be looped only once! */
    while (1)
    {
        d = dirp + index;
        result = yaffs_readdir(dir);
        /* if met a error or all entry are read over, break while*/
        if (!result)
            break;
		

        /* convert to dfs stat structure */
        if (S_ISREG(dir->de.d_type))
            d->d_type = DT_REG;
        else if (S_ISDIR(dir->de.d_type))
            d->d_type = DT_DIR;
        else
            d->d_type = DT_UNKNOWN;
        /* write the rest fields of struct dirent* dirp  */
        d->d_namlen = rt_strlen(dir->de.d_name);
        d->d_reclen = (rt_uint16_t)sizeof(struct dirent);
        rt_strncpy(d->d_name, dir->de.d_name, d->d_namlen + 1);

        index ++;
        if (index * sizeof(struct dirent) >= count)
            break;
    }
    if (!result)
        return -1;
    return index * sizeof(struct dirent);
}

static int dfs_yaffs2_unlink(struct dfs_filesystem* fs, const char* path)
{
    int result;

    result = yaffs_unlink(path);
    return result;
}

static int dfs_yaffs2_rename(struct dfs_filesystem* fs,
                     const char* oldpath,
                     const char* newpath)
{
    int result;
    result = yaffs_rename(oldpath, newpath);
    return result;
}

static int dfs_yaffs2_stat(struct dfs_filesystem* fs, const char *path, struct stat *st)
{
    int result;
    struct yaffs_stat s;

    result = yaffs_stat(path, &s);
    if (result < 0)
        return result;
    /* convert to dfs stat structure */
    if (S_ISREG(s.st_mode))
        st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH |
        S_IWUSR | S_IWGRP | S_IWOTH;
    else if (S_ISDIR(s.st_mode))
        st->st_mode = S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
    else
        st->st_mode = DT_UNKNOWN; /*fixme*/

    st->st_dev  = 0;
    st->st_size = s.st_size;
    st->st_mtime = s.yst_mtime;

    return 0;
}

static const struct dfs_file_ops yaffs2_fops =
{
    .open = dfs_yaffs2_open,
    .close = dfs_yaffs2_close,
    .ioctl = dfs_yaffs2_ioctl,
    .read = dfs_yaffs2_read,
    .write = dfs_yaffs2_write,
    .flush = dfs_yaffs2_flush,
    .lseek = dfs_yaffs2_lseek,
    .getdents = dfs_yaffs2_getdents,
};

static const struct dfs_filesystem_ops yaffs2_ops =
{
    .name = "yaffs2",
    .flags = DFS_FS_FLAG_DEFAULT,
    .fops = &yaffs2_fops,
    .mount = dfs_yaffs2_mount,
    .unmount = dfs_yaffs2_unmount,
    .mkfs = dfs_yaffs2_mkfs,
    .statfs = dfs_yaffs2_statfs,
    .unlink = dfs_yaffs2_unlink,
    .stat = dfs_yaffs2_stat,
    .rename = dfs_yaffs2_rename,
};
#define DBG_SECTION_NAME               "YAFFS"
#ifdef RT_YAFFS_DEBUG
#define DBG_LEVEL                      DBG_LOG
#else
#define DBG_LEVEL                      DBG_INFO
#endif /* RT_SDIO_DEBUG */
#include <rtdbg.h>
#include <yaffs_trace.h>
unsigned int yaffs_trace_mask = YAFFS_TRACE_ALWAYS;

void yaffs_bug_fn(const char *file_name, int line_no)
{
    LOG_E("%s %d\r\n", file_name, line_no);
}

void yaffsfs_Lock(void)
{
    rt_mutex_take(&yaffs2_lock, RT_WAITING_FOREVER);
}
void yaffsfs_Unlock(void)
{
    rt_mutex_release(&yaffs2_lock);
}

u32 yaffsfs_CurrentTime(void)
{
    return 0;
}

void yaffsfs_SetError(int err)
{
   if (err)
       LOG_E("yaffs error:%d\r\n", err);
}

void *yaffsfs_malloc(size_t size)
{
    return rt_malloc(size);
}
void yaffsfs_free(void *ptr)
{
    rt_free(ptr);
}
int yaffsfs_CheckMemRegion(const void *addr, size_t size, int write_request)
{
    return 0;
}

int dfs_yaffs2_init(void)
{
    /* register fatfs file system */
    dfs_register(&yaffs2_ops);

    /* initialize mutex */
    if (rt_mutex_init(&yaffs2_lock, "yaffs2lock", RT_IPC_FLAG_FIFO) != RT_EOK)
    {
        rt_kprintf("init yaffs2 lock mutex failed\n");
    }
    /* rt_kprintf("init jffs2 lock mutex okay\n");  */
    return 0;
}
