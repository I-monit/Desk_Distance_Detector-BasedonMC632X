/*
 * Date           Author       Notes
 * 2020/07/10     wf_wangfeng@qq.com           glue for ubi and rt devices.
 */
#ifndef __DFS_UBIFS_H__
#define __DFS_UBIFS_H__
/* #include <rtdef.h> */
#include <stdint.h>
#include <time.h>

struct rt_ubifs_stat
{
    uint16_t st_ino;
    uint16_t st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    uint32_t st_size;
    time_t st_atime;
    long st_spare1;
    time_t st_mtime;
    long st_spare2;
    time_t st_ctime;
    long st_spare3;
    uint32_t st_blksize;
    uint32_t st_blocks;
    long st_spare4[2];
};

struct rt_ubifs_statfs
{
    uint32_t f_bsize;  /* block size */
    uint32_t f_blocks; /* total data blocks in file system */
    uint32_t f_bfree;  /* free blocks in file system */
};

struct rt_ubifs_fd
{
    char *path;     /* Name (below mount point) */
    uint32_t flags; /* Descriptor flags */
    uint32_t size;  /* Size in bytes */
    uint32_t pos;   /* Current file position */
    void *pDevHandle;
    void *data; /* Specific file system data */
};

#define RT_UBIFS_UNKNOWN 0x00
#define RT_UBIFS_REG 0x01
#define RT_UBIFS_DIR 0x02
#define RT_UBIFS_PATH_MAX 256

struct rt_ubifs_dirent
{
    uint8_t d_type;                 /* The type of the file */
    uint8_t d_namlen;               /* The length of the not including the terminating null file name */
    uint16_t d_reclen;              /* length of this record */
    char d_name[RT_UBIFS_PATH_MAX]; /* The null-terminated file name */
};

#define RT_UBIFS_O_RDONLY 00
#define RT_UBIFS_O_WRONLY 01
#define RT_UBIFS_O_RDWR 02

#if defined(__x86_64) || defined(__i386)
#define RT_UBIFS_O_CREAT 0100
#define RT_UBIFS_O_EXCL 0200
#define RT_UBIFS_O_NOCTTY 0400
#define RT_UBIFS_O_TRUNC 01000
#define RT_UBIFS_O_APPEND 02000
#define RT_UBIFS_O_DIRECTORY 0200000
#else
#define RT_UBIFS_O_CREAT 0x100
#define RT_UBIFS_O_EXCL 0x200
#define RT_UBIFS_O_NOCTTY 0x400
#define RT_UBIFS_O_TRUNC 0x1000
#define RT_UBIFS_O_APPEND 0x2000
#define RT_UBIFS_O_DIRECTORY 0x200000
#endif

void rt_ubifs_init(void);
int rt_ubifs_mount(unsigned long rwflag, char *mtdName, char *volName, void *pDevHandle);
int rt_ubifs_unmount(void *pDevHandle);
int rt_ubifs_mkfs(void *pDev);
int rt_ubifs_statfs(struct rt_ubifs_statfs *buf, void *pDevHandle);

int rt_ubifs_open(struct rt_ubifs_fd *file);
int rt_ubifs_close(struct rt_ubifs_fd *file);
int rt_ubifs_ioctl(struct rt_ubifs_fd *file, int cmd, void *args);
int rt_ubifs_read(struct rt_ubifs_fd *file, void *buf, size_t len);
int rt_ubifs_write(struct rt_ubifs_fd *file, const void *buf, size_t len);
int rt_ubifs_flush(struct rt_ubifs_fd *file);
int rt_ubifs_lseek(struct rt_ubifs_fd *file, uint32_t offset);
int rt_ubifs_getdents(struct rt_ubifs_fd *file, struct rt_ubifs_dirent *dirp, uint32_t count);
int rt_ubifs_unlink(const char *path, void *pDevHandle);
int rt_ubifs_rename(const char *oldpath, const char *newpath, void *pDevHandle);
int rt_ubifs_stat(const char *path, struct rt_ubifs_stat *st, void *pDevHandle);
#endif
