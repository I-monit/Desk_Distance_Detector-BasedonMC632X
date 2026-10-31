#include <rtthread.h>
#include <sys/resource.h>
#include <errno.h>

#include <dfs.h>
#include <dfs_file.h>
#include <poll.h>

int dup(int oldfd)
{
    int newfd;
    struct dfs_fd *new_d, *old_d;

    old_d = fd_get(oldfd);
    if (old_d == NULL)
    {
        errno = EBADF;
        return -1;
    }

    newfd = fd_new();
    if (newfd < 0)
    {
        errno = EMFILE;
        return -1;
    }
    new_d = fd_get(newfd);
    new_d->type = old_d->type;
    new_d->path = old_d->path;
    new_d->fops = old_d->fops;
    new_d->flags = old_d->flags;
    new_d->size = old_d->size;
    new_d->pos = old_d->pos;
    new_d->data = old_d->data;

    fd_put(new_d);
    fd_put(old_d);

    return newfd;
}

/* RT_USING_DFS_DEVFS depends on RT_USING_DFS */
#if defined(RT_USING_DFS_DEVFS) && defined(RT_USING_CONSOLE)
#define SYS_FD_MAX  (DFS_FD_MAX - DFS_FD_OFFSET)
#else
#define SYS_FD_MAX  (DFS_FD_MAX)
#endif

int getrlimit(int resource, struct rlimit *rlim)
{
    struct dfs_fdtable *fdt;

    fdt = dfs_fdtable_get();
    if (rlim == NULL)
    {
        errno = EFAULT;
        return -1;
    }
    if (resource != RLIMIT_NOFILE)
    {
        errno = EINVAL;
        return -1;
    }

    rlim->rlim_cur = fdt->maxfd;
    rlim->rlim_max = SYS_FD_MAX;

    return 0;
}

int setrlimit(int resource, const struct rlimit *rlim)
{
    struct dfs_fdtable *fdt;

    fdt = dfs_fdtable_get();
    if (rlim == NULL)
    {
        errno = EFAULT;
        return -1;
    }
    if (resource != RLIMIT_NOFILE)
    {
        errno = EINVAL;
        return -1;
    }

    if (rlim->rlim_cur > rlim->rlim_max || rlim->rlim_cur > SYS_FD_MAX)
    {
        errno = EPERM;
        return -1;
    }

    fdt->maxfd = rlim->rlim_cur;

    return 0;
}

void sync(void)
{
    struct dfs_fd *fd;
    int index;
    struct dfs_fdtable *fdt;

    fdt = dfs_fdtable_get();

    dfs_lock();
    for (index = 0; index < fdt->maxfd; index++)
    {
        fd = fdt->fds[index];
        if (fd == RT_NULL)
            continue;
        if (fd->fops == RT_NULL)
            continue;
        if (!(fd->flags & DFS_F_OPEN))
            continue;

        if (fd->fops->flush)
            fd->fops->flush(fd);
    }
    dfs_unlock();
}

void cmd_sync(int argc, char *argv[])
{
    sync();
}

#ifdef RT_USING_FINSH
#include <finsh.h>
FINSH_FUNCTION_EXPORT(sync, sync);
FINSH_FUNCTION_EXPORT_ALIAS(cmd_sync, __cmd_sync, flush file system buffers)
#endif
