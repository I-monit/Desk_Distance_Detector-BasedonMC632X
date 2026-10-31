/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2009-05-27     Yi.qiu       The first version.
 * 2010-07-18     Bernard      add stat and statfs structure definitions.
 * 2011-05-16     Yi.qiu       Change parameter name of rename, "new" is C++ key word.
 * 2017-12-27     Bernard      Add fcntl API.
 * 2018-02-07     Bernard      Change the 3rd parameter of open/fcntl/ioctl to '...'
 */

#ifndef __DFS_POSIX_H__
#define __DFS_POSIX_H__

#include <dfs_file.h>
#include <dirent.h>     /* import DIR definition */

#ifdef __cplusplus
extern "C" {
#endif


/**********************************************************************
 *
 *   NOTE:
 *
 *     1.dfs_posix.h为dfs模块内部使用，对于应用代码，不会导出该头文件
 *
 *     2.这里定义的所有接口均可以在unistd.h/fcntl.h等头文件中找到定义
 *
 *     3.由于这里仅是声明，验证与POSIX标准接口定义一致即可，此处暂时不
 *       作修改
 *     4.结构体定义除外，DIR移到dirent.h中定义
 *
 *********************************************************************/

#define fdatasync fsync

#if 0
typedef struct      /* defined in dirent.h */
{
    int fd;     /* directory file */
    char buf[512];
    int num;
    int cur;
} DIR;
#endif

/* directory api*/
int mkdir(const char *path, mode_t mode);                   /* sys/stat.h */
DIR *opendir(const char *name);                             /* dirent.h */
struct dirent *readdir(DIR *d);
long telldir(DIR *d);
void seekdir(DIR *d, off_t offset);                         /* offset should be long */
void rewinddir(DIR *d);
int closedir(DIR* d);

/* file api*/
int open(const char *file, int flags, ...);                 /* sys/stat.h & fcntl.h */
int close(int d);

#if defined(RT_USING_NEWLIB) && defined(_EXFUN)
_READ_WRITE_RETURN_TYPE _EXFUN(read, (int __fd, void *__buf, size_t __nbyte));
_READ_WRITE_RETURN_TYPE _EXFUN(write, (int __fd, const void *__buf, size_t __nbyte));
#else
int read(int fd, void *buf, size_t len);                    /* unistd.h */
int write(int fd, const void *buf, size_t len);
#endif

int ftruncate(int fd, off_t len);                           /* unistd.h */
int fallocate(int fd, int mode, rt_off64_t offset, rt_off64_t len);
int fhide(int fd, int enable);                              /* not defined */
off_t lseek(int fd, off_t offset, int whence);              /* unistd.h */
rt_off64_t lseek64(int fd, rt_off64_t offset, int whence);  /* unistd.h */
int rename(const char *from, const char *to);               /* defined in stdio.h, shall we include stdio.h directly? */
int unlink(const char *pathname);                           /* unistd.h */
int stat(const char *file, struct stat *buf);               /* sys/stat.h */
int fstat(int fildes, struct stat *buf);                    /* sys/stat.h */
int fsync(int fildes);                                      /* unistd.h */
int fcntl(int fildes, int cmd, ...);                        /* unistd.h/fcntl.h */
int ioctl(int fildes, int cmd, ...);                        /* sys/ioctl.h */

int mount(const char   *device_name,                        /* sys/mount.h */
            const char   *path,
            const char   *filesystemtype,
            unsigned long rwflag,
            const void   *data);
int umount(const char *specialfile);

/* directory api*/
int rmdir(const char *path);                                /* unistd.h */
int chdir(const char *path);                                /* unistd.h */
char *getcwd(char *buf, size_t size);                       /* unistd.h */

/* file system api */
int statfs(const char *path, struct statfs *buf);           /* sys/vfs.h or sys/statfs.h */

int access(const char *path, int amode);                    /* unistd.h */
int pipe(int fildes[2]);                                    /* unistd.h */
int mkfifo(const char *path, mode_t mode);                  /* sys/stat.h */

#ifdef __cplusplus
}
#endif

#endif
