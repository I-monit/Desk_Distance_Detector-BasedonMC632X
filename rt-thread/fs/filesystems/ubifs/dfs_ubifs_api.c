#include <rtthread.h>
#include <rtdevice.h>

#undef mode_t

#include <dfs_fs.h>
#include <dfs_file.h>
#include "ubi_glue.h"
#include "ubifs_glue.h"
#include <dfs_posix.h>
#include <string.h>
#include <rt_ubi_api.h>

static struct rt_mutex ubifs_lock;

static char *getDeviceNamebyID(rt_device_t dev_id)
{
    struct rt_object *object;
    struct rt_list_node *node;
    struct rt_object_information *information;

    /* enter critical */
    if (rt_thread_self() != RT_NULL)
        rt_enter_critical();

    /* try to find device object */
    information = rt_object_get_information(RT_Object_Class_Device);
    RT_ASSERT(information != RT_NULL);
    for (node = information->object_list.next;
         node != &(information->object_list);
         node = node->next)
    {
        object = rt_list_entry(node, struct rt_object, list);
        if (object == (struct rt_object *)dev_id)
        {
            if (rt_thread_self() != RT_NULL)
                rt_exit_critical();
            printf("get device.name:%s\n", object->name);
            return object->name;
        }
    }
    /* leave critical */
    if (rt_thread_self() != RT_NULL)
        rt_exit_critical();
    return NULL;
}

/*
 * RT-Thread Device Interface for ubifs
 */
/*
 * RT-Thread DFS Interface for ubifs
 */
static int dfs_ubifs_mount(struct dfs_filesystem *fs,
                           unsigned long rwflag,
                           const void *data)
{
    char *volName = (char *)data;
    char mVolName[256] = {0};
    char *devName = getDeviceNamebyID(fs->dev_id);
    rt_kprintf("dfs_ubifs_mount.%s,%s,%p,id:%d\n", fs->path, volName, fs->dev_id, fs->dev_id->device_id);
    if (volName == NULL)
    {
        rt_kprintf("not set vol name.use param data to set\n");
        snprintf(mVolName, sizeof(mVolName), "ubi:%s", rt_ubi_get_defautlVolName());
        volName = mVolName;
        /* return -1; */
    }
    return rt_ubifs_mount(rwflag, devName, volName, fs->dev_id); /* char *vol_name); */
}

static int dfs_ubifs_unmount(struct dfs_filesystem *fs)
{
    printf("%s,%d\n", __FUNCTION__, __LINE__);
    rt_ubifs_unmount(fs->dev_id);
    return 0;
}

static int dfs_ubifs_mkfs(rt_device_t dev_id)
{
    char *devName = getDeviceNamebyID(dev_id);
    if (devName != NULL)
    {
        glue_ubi_detach_mtd(devName); /* must detach before erase. */
        glue_ubi_mtd_erase(devName);

        glue_ubi_setMtd((const char **)&devName, 1); /* ubi init error will reset mtd num. */
        rt_ubi_create_defaultVolume(devName);
        /* rt_ubifs_mkfs(devName); */
    }
    else
    {
        rt_kprintf("no find device!\n");
        return -1;
    }
    /* struct rt_mtd_nand_device *pNand; */
    /* ubi_exit(); */
    /*rt_ubi_create_vol("default", 10 * 1024 * 1024, 1, 0);*/
    /* pNand = RT_MTD_NAND_DEVICE(dev_id); */
    /* rt_ubifs_mkfs(dev_id); */
    /* ubi_init(); */
    /* ubifs_init(); */
    /* int ret = dfs_mount("inand0", "/", "ubifs", 0, "ubi:ui1");*/
    /* just erase all blocks on this nand partition */
    return 0; /* -ENOSYS; */
}

int dfs_ubifs_statfs(struct dfs_filesystem *fs,
                     struct statfs *buf)
{
    struct rt_ubifs_statfs ubifs_statfs;
    rt_ubifs_statfs(&ubifs_statfs, fs->dev_id);
    buf->f_bsize = ubifs_statfs.f_bsize;
    buf->f_blocks = ubifs_statfs.f_blocks;
    buf->f_bfree = ubifs_statfs.f_bfree;
    return 0;
}

static int dfs_ubifs_open(struct dfs_fd *file)
{
    int ret = 0;
    struct rt_ubifs_fd *ubifs_fd = rt_malloc(sizeof(struct rt_ubifs_fd));
    if (!ubifs_fd)
    {
        rt_kprintf("%s: Error, no memory for malloc!\n", __func__);
        return -ENOMEM;
    }
    rt_mutex_take(&ubifs_lock, RT_WAITING_FOREVER);
    memset(ubifs_fd, 0, sizeof(struct rt_ubifs_fd));
    if (file->flags & O_RDWR)
        ubifs_fd->flags |= RT_UBIFS_O_RDWR;
    if (file->flags & O_WRONLY)
        ubifs_fd->flags |= RT_UBIFS_O_WRONLY;
    if (file->flags & O_APPEND)
        ubifs_fd->flags |= RT_UBIFS_O_APPEND;
    if (file->flags & O_CREAT)
        ubifs_fd->flags |= RT_UBIFS_O_CREAT;
    if (file->flags & O_TRUNC)
        ubifs_fd->flags |= RT_UBIFS_O_TRUNC;
    if (file->flags & O_EXCL)
        ubifs_fd->flags |= RT_UBIFS_O_EXCL;
    if (file->flags & O_DIRECTORY)
        ubifs_fd->flags |= RT_UBIFS_O_DIRECTORY;
    /* ubifs_fd->flags = file->flags; */
    ubifs_fd->pDevHandle = file->fs->dev_id;
    ubifs_fd->path = file->path;
    ret = rt_ubifs_open(ubifs_fd);
    file->data = ubifs_fd;
    file->size = ubifs_fd->size;
    file->pos = ubifs_fd->pos;
    rt_mutex_release(&ubifs_lock);
    return ret;
}

static int dfs_ubifs_close(struct dfs_fd *file)
{
    struct rt_ubifs_fd *ubifs_fd = (struct rt_ubifs_fd *)file->data;
    rt_mutex_take(&ubifs_lock, RT_WAITING_FOREVER);
    if (ubifs_fd)
    {
        rt_ubifs_close(ubifs_fd);
        if (ubifs_fd)
            rt_free(ubifs_fd);
    }
    rt_mutex_release(&ubifs_lock);
    return 0;
}

static int dfs_ubifs_ioctl(struct dfs_fd *file, int cmd, void *args)
{
    printf("%s,%d not support\n", __FUNCTION__, __LINE__);
    return -ENOSYS;
}

static int dfs_ubifs_read(struct dfs_fd *file, void *buf, size_t len)
{
    int read_length = 0;
    struct rt_ubifs_fd *ubifs_fd = (struct rt_ubifs_fd *)file->data;
    rt_mutex_take(&ubifs_lock, RT_WAITING_FOREVER);
    read_length = rt_ubifs_read(ubifs_fd, buf, len);
    file->pos = ubifs_fd->pos;
    rt_mutex_release(&ubifs_lock);
    return read_length;
}

static int dfs_ubifs_write(struct dfs_fd *file,
                           const void *buf,
                           size_t len)
{
    int writedSize = 0;
    struct rt_ubifs_fd *ubifs_fd = (struct rt_ubifs_fd *)file->data;
    rt_mutex_take(&ubifs_lock, RT_WAITING_FOREVER);
    writedSize = rt_ubifs_write(ubifs_fd, buf, len);
    file->size = ubifs_fd->size;
    file->pos = ubifs_fd->pos;
    rt_mutex_release(&ubifs_lock);
    return writedSize;
}

static int dfs_ubifs_flush(struct dfs_fd *file)
{
    struct rt_ubifs_fd *ubifs_fd = (struct rt_ubifs_fd *)file->data;
    rt_mutex_take(&ubifs_lock, RT_WAITING_FOREVER);
    if (ubifs_fd)
    {
        rt_ubifs_flush(ubifs_fd);
    }
    rt_mutex_release(&ubifs_lock);
    return 0;
}

/* fixme warning: the offset is rt_off_t, so maybe the size of a file is must <= 2G*/
/* return the current position after seek */
static int dfs_ubifs_lseek(struct dfs_fd *file,
                           rt_off_t offset)
{
    /* update file position */
    if (offset > file->size)
    {
        rt_kprintf("offset:%d>size:%d\n", offset, file->size);
        return -1;
    }
    rt_mutex_take(&ubifs_lock, RT_WAITING_FOREVER);
    struct rt_ubifs_fd *ubifs_fd = (struct rt_ubifs_fd *)file->data;
    file->pos = rt_ubifs_lseek(ubifs_fd, offset);
    rt_mutex_release(&ubifs_lock);
    return file->pos;
}
static int dfs_ubifs_getdents(struct dfs_fd *file,
                              struct dirent *dirp,
                              rt_uint32_t count)
{
    /* count = (count / sizeof(struct rt_ubifs_dirent)) * sizeof(struct rt_ubifs_dirent); */
    if (count == 0)
        return -EINVAL;
    uint32_t retNum = (count / sizeof(struct rt_ubifs_dirent));
    struct rt_ubifs_fd *ubifs_fd = (struct rt_ubifs_fd *)file->data;
    int index = 0;
    struct rt_ubifs_dirent ubifs_dirp; /* = rt_malloc(count * sizeof(struct rt_ubifs_dirent)); */
    rt_mutex_take(&ubifs_lock, RT_WAITING_FOREVER);
    struct dirent *d = dirp;
    while (index < retNum)
    {
        d = dirp + index;
        if (rt_ubifs_getdents(ubifs_fd, &ubifs_dirp, 1) == 0)
        {
            break;
        }
        d->d_namlen = ubifs_dirp.d_namlen;
        d->d_reclen = (rt_uint16_t)sizeof(struct dirent); /* ubifs_dirp.d_reclen; */
        d->d_type = ubifs_dirp.d_type;
        strncpy(d->d_name, ubifs_dirp.d_name, sizeof(d->d_name));
        index++;
    }
    rt_mutex_release(&ubifs_lock);
    return index * sizeof(struct dirent);
}
/* delete */
static int dfs_ubifs_unlink(struct dfs_filesystem *fs, const char *path)
{
    int ret = 0;
    rt_mutex_take(&ubifs_lock, RT_WAITING_FOREVER);
    ret = rt_ubifs_unlink(path, fs->dev_id);
    rt_mutex_release(&ubifs_lock);
    return ret;
}

static int dfs_ubifs_rename(struct dfs_filesystem *fs,
                            const char *oldpath,
                            const char *newpath)
{
    int ret = 0;
    rt_mutex_take(&ubifs_lock, RT_WAITING_FOREVER);
    ret = rt_ubifs_rename(oldpath, newpath, fs->dev_id);
    rt_mutex_release(&ubifs_lock);
    return ret;
}

static int dfs_ubifs_stat(struct dfs_filesystem *fs, const char *path, struct stat *st)
{
    struct rt_ubifs_stat ubifsStat;
    rt_mutex_take(&ubifs_lock, RT_WAITING_FOREVER);
    rt_ubifs_stat(path, &ubifsStat, fs->dev_id);
    st->st_dev = 0;
    st->st_size = ubifsStat.st_size;
    st->st_mtime = ubifsStat.st_mtime;
    st->st_mode = ubifsStat.st_mode;
    st->st_ino = ubifsStat.st_ino;
    st->st_nlink = ubifsStat.st_nlink;
    st->st_blocks = ubifsStat.st_blocks;
    st->st_blksize = ubifsStat.st_blksize;
    rt_mutex_release(&ubifs_lock);
    return 0;
}

static const struct dfs_file_ops _ubifs_fops = {
        dfs_ubifs_open,
        dfs_ubifs_close,
        dfs_ubifs_ioctl,
        dfs_ubifs_read,
        dfs_ubifs_write,
        dfs_ubifs_flush,
        dfs_ubifs_lseek,
        dfs_ubifs_getdents,
};

static const struct dfs_filesystem_ops _ubifs_ops = {
        "ubifs",
        DFS_FS_FLAG_DEFAULT,
        &_ubifs_fops,

        dfs_ubifs_mount,
        dfs_ubifs_unmount,
        dfs_ubifs_mkfs,
        dfs_ubifs_statfs,

        dfs_ubifs_unlink,
        dfs_ubifs_stat,
        dfs_ubifs_rename,
};
void dfs_ubifs_register(void)
{
    dfs_register(&_ubifs_ops);
}
int dfs_ubifs_init(void)
{
    rt_ubifs_init();

    /* initialize mutex */
    if (rt_mutex_init(&ubifs_lock, "ubifslock", RT_IPC_FLAG_FIFO) != RT_EOK)
    {
        rt_kprintf("init ubifs lock mutex failed\n");
    }
    rt_kprintf("init ubifs lock mutex okay\n");
    return 0;
}
INIT_COMPONENT_EXPORT(dfs_ubifs_init);
/* FINSH_FUNCTION_EXPORT(dfs_ubifs_init, ubifs init); */
/* MSH_CMD_EXPORT(dfs_ubifs_init, ubifs init); */
