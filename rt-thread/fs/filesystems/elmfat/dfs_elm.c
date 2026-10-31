/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2008-02-22     QiuYi        The first version.
 * 2011-10-08     Bernard      fixed the block size in statfs.
 * 2011-11-23     Bernard      fixed the rename issue.
 * 2012-07-26     aozima       implement ff_memalloc and ff_memfree.
 * 2012-12-19     Bernard      fixed the O_APPEND and lseek issue.
 * 2013-03-01     aozima       fixed the stat(st_mtime) issue.
 * 2014-01-26     Bernard      Check the sector size before mount.
 * 2017-02-13     Hichard      Update Fatfs version to 0.12b, support exFAT.
 * 2017-04-11     Bernard      fix the st_blksize issue.
 * 2017-05-26     Urey         fix f_mount error when mount more fats
 */

#include <rtthread.h>
#include "ffconf.h"
#include "ff.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#ifdef RT_DFS_ELM_BLK_CACHE
#include "dfs_block_cache.h"
#endif
/* ELM FatFs provide a DIR struct */
#define HAVE_DIR_STRUCTURE
#define FILEINFO_CACHE_NUM 1

#include <dfs_fs.h>
#include <dfs_file.h>

static rt_device_t disk[_VOLUMES] = {0};
extern unsigned int mkfs_fs_type;

typedef struct {
    int valid;
    unsigned int cid;
    FILINFO fno;
} st_fileinfo_cache;
st_fileinfo_cache fno_cache[FILEINFO_CACHE_NUM] = {{0}};

static const unsigned int crc32_tab[] = {
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
   };

static unsigned int  dfs_crc32(unsigned int crc32val, unsigned char *s, int len)
{
    int i;

    for (i = 0;  i < len;  i++)
        crc32val = crc32_tab[(crc32val ^ s[i]) & 0xff] ^ (crc32val >> 8);

    return crc32val;
}

static int find_finfo_cache(char *fullpath, FILINFO *fno)
{
    if (fullpath == NULL || fno == NULL)
        return 0;

    unsigned int cid = dfs_crc32(0, (unsigned char *)fullpath, strlen(fullpath));
    int i = 0;
    int found = 0;

    for (i = 0; i < FILEINFO_CACHE_NUM; i++)
    {
        if (fno_cache[i].valid != 0 && fno_cache[i].cid == cid)
        {
            rt_memcpy(fno, &fno_cache[i].fno, sizeof(FILINFO));
            found = 1;
            break;
        }
    }

    return found;
}

static int add_finfo_cache(char *fullpath, FILINFO *fno)
{
    if (fullpath == NULL || fno == NULL)
        return 0;

    unsigned int cid = dfs_crc32(0, (unsigned char *)fullpath, strlen(fullpath));
    int i = 0;
    int found = 0;

    for (i = 0; i < FILEINFO_CACHE_NUM; i++)
    {
        if (fno_cache[i].valid == 0)
        {
            fno_cache[i].valid = 1;
            rt_memcpy(&fno_cache[i].fno, fno, sizeof(FILINFO));
            fno_cache[i].cid = cid;
            found = 1;
            break;
        }
    }

    return found;
}

static void del_double_slash(char *buff)
{
    int i = 0;
    int len = strlen(buff) + 1;

    while (1)
    {
        if (buff[i] == '\0')
            break;

        if (buff[i] == '/' && buff[i] == buff[i+1])
        {
            rt_memmove(&buff[i], &buff[i+1], len-i);
            len--;
        }
        i++;
    }
}

static void invaildate_finfo_cache(int cid)
{
    if (cid == -1)
        rt_memset(&fno_cache, 0, sizeof(fno_cache));
    else if (cid < FILEINFO_CACHE_NUM)
        fno_cache[cid].valid = 0;
}

void createfullpath(struct dfs_filesystem *fs, char *fullpath, char *path, char *name)
{
    if (name == NULL || fs == NULL || fullpath == NULL)
        return;

#if (_VOLUMES > 1)
    int vol;

    extern int elm_get_vol(FATFS *fat);

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
        return;
    if (path != NULL)
        rt_sprintf(fullpath,"%d:%s/%s", vol, path, name);
    else
        rt_sprintf(fullpath,"%d:%s", vol, name);
#else
    if (path != NULL)
        rt_sprintf(fullpath,"%s/%s", path, name);
    else
        rt_sprintf(fullpath,"%s", name);
#endif
    del_double_slash(fullpath);
}



static int elm_result_to_dfs(FRESULT result)
{
    int status = RT_EOK;

    switch (result)
    {
    case FR_OK:
        break;

    case FR_NO_FILE:
    case FR_NO_PATH:
    case FR_NO_FILESYSTEM:
        status = -ENOENT;
        break;

    case FR_INVALID_NAME:
        status = -EINVAL;
        break;

    case FR_EXIST:
    case FR_INVALID_OBJECT:
        status = -EEXIST;
        break;

    case FR_DISK_ERR:
    case FR_NOT_READY:
    case FR_INT_ERR:
        status = -EIO;
        break;

    case FR_WRITE_PROTECTED:
    case FR_DENIED:
        status = -EROFS;
        break;

    case FR_MKFS_ABORTED:
        status = -EINVAL;
        break;

    default:
        status = -1;
        break;
    }

    return status;
}

/* results:
 *  -1, no space to install fatfs driver
 *  >= 0, there is an space to install fatfs driver
 */
static int get_disk(rt_device_t id)
{
    int index;

    for (index = 0; index < _VOLUMES; index ++)
    {
        if (disk[index] == id)
            return index;
    }

    return -1;
}

int dfs_elm_mount(struct dfs_filesystem *fs, unsigned long rwflag, const void *data)
{
    FATFS *fat;
    FRESULT result;
    int index;
    struct rt_device_blk_geometry geometry;
    char logic_nbr[2] = {'0',':'};

    invaildate_finfo_cache(-1);
    /* get an empty position */
    index = get_disk(RT_NULL);
    if (index == -1)
        return -ENOENT;
    logic_nbr[0] = '0' + index;
    /* save device */
    disk[index] = fs->dev_id;
#ifdef RT_DFS_ELM_BLK_CACHE
    api_dfs_bind_hw_io_handle(api_dfs_get_cache_handle(), fs->dev_id);
#endif
    /* check sector size */
    if (rt_device_control(fs->dev_id, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry) == RT_EOK)
    {
        if (geometry.bytes_per_sector > _MAX_SS)
        {
            rt_kprintf("The sector size of device is greater than the sector size of FAT.\n");
            return -EINVAL;
        }
    }

    fat = (FATFS *)rt_malloc(sizeof(FATFS));
    if (fat == RT_NULL)
    {
        disk[index] = RT_NULL;
        return -ENOMEM;
    }

    /* mount fatfs, always 0 logic driver */
    result = f_mount(fat, (const TCHAR*)logic_nbr, 1);
    if (result == FR_OK)
    {
        char drive[8];
        FDIR *dir;
        #ifdef RT_DFS_ELM_BLK_CACHE
        api_dfs_focus_on_cache_area(api_dfs_get_cache_handle(), fat->fatbase, fat->database);
        #endif
        rt_snprintf(drive, sizeof(drive), "%d:/", index);
        dir = (FDIR *)rt_malloc(sizeof(FDIR));
        if (dir == RT_NULL)
        {
            f_mount(RT_NULL, (const TCHAR*)logic_nbr, 1);
            disk[index] = RT_NULL;
            rt_free(fat);
            return -ENOMEM;
        }

        /* open the root directory to test whether the fatfs is valid */
        result = f_opendir(dir, drive);
        if (result != FR_OK)
            goto __err;

        /* mount succeed! */
        fs->data = fat;
        rt_free(dir);
        return 0;
    }

__err:
    f_mount(RT_NULL, (const TCHAR*)logic_nbr, 1);
    disk[index] = RT_NULL;
    rt_free(fat);
    return elm_result_to_dfs(result);
}

int dfs_elm_unmount(struct dfs_filesystem *fs)
{
    FATFS *fat;
    FRESULT result;
    int  index;
    char logic_nbr[2] = {'0',':'};

    fat = (FATFS *)fs->data;
    invaildate_finfo_cache(-1);

    RT_ASSERT(fat != RT_NULL);

    /* find the device index and then umount it */
    index = get_disk(fs->dev_id);
    if (index == -1) /* not found */
        return -ENOENT;

    logic_nbr[0] = '0' + index;
    result = f_mount(RT_NULL, logic_nbr, (BYTE)1);
    if (result != FR_OK)
        return elm_result_to_dfs(result);

    fs->data = RT_NULL;
    disk[index] = RT_NULL;
    rt_free(fat);

    return RT_EOK;
}

int dfs_elm_mkfs(rt_device_t dev_id)
{
#define FSM_STATUS_INIT            0
#define FSM_STATUS_USE_TEMP_DRIVER 1
    FATFS *fat = RT_NULL;
    BYTE *work;
    int flag;
    FRESULT result;
    int index;
    char logic_nbr[2] = {'0',':'};

    work = rt_malloc(_MAX_SS);
    if (work == RT_NULL)
        return -ENOMEM;

    if (dev_id == RT_NULL)
    {
        rt_free(work); /* release memory */
        return -EINVAL;
    }

    /* if the device is already mounted, then just do mkfs to the drv,
     * while if it is not mounted yet, then find an empty drive to do mkfs
     */

    flag = FSM_STATUS_INIT;
    index = get_disk(dev_id);
    if (index == -1)
    {
        /* not found the device id */
        index = get_disk(RT_NULL);
        if (index == -1)
        {
            /* no space to store an temp driver */
            rt_kprintf("sorry, there is no space to do mkfs! \n");
            rt_free(work); /* release memory */
            return -ENOSPC;
        }
        else
        {
            fat = rt_malloc(sizeof(FATFS));
            if (fat == RT_NULL)
            {
                rt_free(work); /* release memory */
                return -ENOMEM;
            }

            flag = FSM_STATUS_USE_TEMP_DRIVER;

            disk[index] = dev_id;
            /* try to open device */
            rt_device_open(dev_id, RT_DEVICE_OFLAG_RDWR);

            /* just fill the FatFs[vol] in ff.c, or mkfs will failded!
             * consider this condition: you just umount the elm fat,
             * then the space in FatFs[index] is released, and now do mkfs
             * on the disk, you will get a failure. so we need f_mount here,
             * just fill the FatFS[index] in elm fatfs to make mkfs work.
             */
            logic_nbr[0] = '0' + index;
            f_mount(fat, logic_nbr, (BYTE)index);
        }
    }
    else
    {
        logic_nbr[0] = '0' + index;
    }

    /* [IN] Logical drive number */
    /* [IN] Format options */
    /* [IN] Size of the allocation unit */
    /* [-]  Working buffer */
    /* [IN] Size of working buffer */
    result = f_mkfs(logic_nbr, mkfs_fs_type | FM_SFD, 0, work, _MAX_SS);
    rt_free(work); work = RT_NULL;

    /* check flag status, we need clear the temp driver stored in disk[] */
    if (flag == FSM_STATUS_USE_TEMP_DRIVER)
    {
        rt_free(fat);
        f_mount(RT_NULL, logic_nbr,(BYTE)index);
        disk[index] = RT_NULL;
        /* close device */
        rt_device_close(dev_id);
    }

    if (result != FR_OK)
    {
        rt_kprintf("format error\n");
        return elm_result_to_dfs(result);
    }

    return RT_EOK;
}

int dfs_elm_statfs(struct dfs_filesystem *fs, struct statfs *buf)
{
    FATFS *f;
    FRESULT res;
    char driver[4];
    DWORD fre_clust, fre_sect, tot_sect;

    RT_ASSERT(fs != RT_NULL);
    RT_ASSERT(buf != RT_NULL);

    f = (FATFS *)fs->data;
    rt_snprintf(driver, sizeof(driver), "%d:", f->drv);
    res = f_getfree(driver, &fre_clust, &f);
    if (res)
        return elm_result_to_dfs(res);

    /* Get total sectors and free sectors */
    tot_sect = (f->n_fatent - 2) * f->csize;
    fre_sect = fre_clust * f->csize;

    buf->f_bfree = fre_sect;
    buf->f_blocks = tot_sect;
#if _MAX_SS != 512
    buf->f_bsize = f->ssize;
#else
    buf->f_bsize = 512;
#endif

    return 0;
}

int dfs_elm_open(struct dfs_fd *file)
{
    FIL *fd;
    BYTE mode;
    FRESULT result;
    char *drivers_fn;

    invaildate_finfo_cache(-1);
#if (_VOLUMES > 1)
    int vol;
    struct dfs_filesystem *fs = (struct dfs_filesystem *)file->data;
    extern int elm_get_vol(FATFS * fat);

    if (fs == NULL)
        return -ENOENT;

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
        return -ENOENT;
    drivers_fn = rt_malloc(256);
    if (drivers_fn == RT_NULL)
        return -ENOMEM;

    rt_snprintf(drivers_fn, 256, "%d:%s", vol, file->path);
#else
    drivers_fn = file->path;
#endif

    if (file->flags & O_DIRECTORY)
    {
        FDIR *dir;

        if (file->flags & O_CREAT)
        {
            result = f_mkdir(drivers_fn);
            if (result != FR_OK)
            {
#if _VOLUMES > 1
                rt_free(drivers_fn);
#endif
                return elm_result_to_dfs(result);
            }
        }

        /* open directory */
        dir = (FDIR *)rt_malloc(sizeof(FDIR));
        if (dir == RT_NULL)
        {
#if _VOLUMES > 1
            rt_free(drivers_fn);
#endif
            return -ENOMEM;
        }

        result = f_opendir(dir, drivers_fn);
#if _VOLUMES > 1
        rt_free(drivers_fn);
#endif
        if (result != FR_OK)
        {
            rt_free(dir);
            return elm_result_to_dfs(result);
        }

        file->data = dir;
        return RT_EOK;
    }
    else
    {
        mode = FA_READ;

        if (file->flags & O_WRONLY)
            mode |= FA_WRITE;
        if ((file->flags & O_ACCMODE) & O_RDWR)
            mode |= FA_WRITE;
        /* Opens the file, if it is existing. If not, a new file is created. */
        if (file->flags & O_CREAT)
            mode |= FA_OPEN_ALWAYS;
        /* Creates a new file. If the file is existing, it is truncated and overwritten. */
        if (file->flags & O_TRUNC)
            mode |= FA_CREATE_ALWAYS;
        /* Creates a new file. The function fails if the file is already existing. */
        if (file->flags & O_EXCL)
            mode |= FA_CREATE_NEW;

        /* allocate a fd */
        fd = (FIL *)rt_malloc(sizeof(FIL));
        if (fd == RT_NULL)
        {
#if _VOLUMES > 1
            rt_free(drivers_fn);
#endif
            return -ENOMEM;
        }

        result = f_open(fd, drivers_fn, mode);
#if _VOLUMES > 1
        rt_free(drivers_fn);
#endif
        if (result == FR_OK)
        {
            file->pos  = fd->fptr;
            file->size = f_size(fd);
            file->data = fd;

            if (file->flags & O_APPEND)
            {
                /* seek to the end of file */
                f_lseek(fd, f_size(fd));
                file->pos = fd->fptr;
            }
        }
        else
        {
            /* open failed, return */
            rt_free(fd);
            return elm_result_to_dfs(result);
        }
    }

    return RT_EOK;
}

int dfs_elm_close(struct dfs_fd *file)
{
    FRESULT result;

    result = FR_OK;
    invaildate_finfo_cache(-1);
    if (file->type == FT_DIRECTORY)
    {
        FDIR *dir;

        dir = (FDIR *)(file->data);
        RT_ASSERT(dir != RT_NULL);

        /* release memory */
        rt_free(dir);
    }
    else if (file->type == FT_REGULAR)
    {
        FIL *fd;
        fd = (FIL *)(file->data);
        RT_ASSERT(fd != RT_NULL);

        result = f_close(fd);
        if (result != FR_OK)
            rt_kprintf("[WARN]:Close Error:%d\n", result);
        /* release memory */
        /* force result FR_OK to avoid fd leek */
        result = FR_OK;
        rt_free(fd);
    }

    return elm_result_to_dfs(result);
}

int dfs_elm_ioctl(struct dfs_fd *file, int cmd, void *args)
{
    return -ENOSYS;
}

int dfs_elm_read(struct dfs_fd *file, void *buf, size_t len)
{
    FIL *fd;
    FRESULT result;
    UINT byte_read;

    if (file->type == FT_DIRECTORY)
    {
        return -EISDIR;
    }

    fd = (FIL *)(file->data);
    RT_ASSERT(fd != RT_NULL);

    result = f_read(fd, buf, len, &byte_read);
    /* update position */
    file->pos  = fd->fptr;
    if (result == FR_OK)
        return byte_read;

    return elm_result_to_dfs(result);
}

int dfs_elm_write(struct dfs_fd *file, const void *buf, size_t len)
{
    FIL *fd;
    FRESULT result;
    UINT byte_write;

    if (file->type == FT_DIRECTORY)
    {
        return -EISDIR;
    }

    fd = (FIL *)(file->data);
    RT_ASSERT(fd != RT_NULL);

    result = f_write(fd, buf, len, &byte_write);
    /* update position and file size */
    file->pos  = fd->fptr;
    file->size = f_size(fd);
    if (result == FR_OK)
        return byte_write;

    return elm_result_to_dfs(result);
}

int dfs_elm_flush(struct dfs_fd *file)
{
    FIL *fd;
    FRESULT result;

    invaildate_finfo_cache(-1);
    fd = (FIL *)(file->data);
    RT_ASSERT(fd != RT_NULL);

    result = f_sync(fd);
    return elm_result_to_dfs(result);
}

int dfs_elm_ftruncate(struct dfs_fd *file, rt_off_t len)
{
    FIL *fd;
    FRESULT result = FR_OK;

    if (file->type == FT_DIRECTORY)
        return -EISDIR;

    fd = (FIL *)(file->data);
    RT_ASSERT(fd != RT_NULL);

    result = f_lseek(fd, len);
    if (result != FR_OK)
        return elm_result_to_dfs(result);

    result = f_truncate(fd);
    return elm_result_to_dfs(result);
}

int dfs_elm_fallocate(struct dfs_fd *file, int mode, rt_off64_t offset, rt_off64_t len)
{
    FIL *fd;
    FRESULT result = FR_OK;

    if (file->type == FT_DIRECTORY)
        return -EISDIR;

    fd = (FIL *)(file->data);
    RT_ASSERT(fd != RT_NULL);

    result = f_fallocate(fd, mode, offset, len);
    return elm_result_to_dfs(result);
}

int dfs_elm_fhide(struct dfs_fd *file, rt_uint32_t enable)
{
    FRESULT result = FR_OK;

    if (file->type == FT_DIRECTORY)
        return -EISDIR;

    result = f_chmod(file->path, enable ? AM_HID : 0, AM_HID);
    return elm_result_to_dfs(result);
}

int dfs_elm_lseek(struct dfs_fd *file, rt_off_t offset)
{
    FRESULT result = FR_OK;
    if (file->type == FT_REGULAR)
    {
        FIL *fd;

        /* regular file type */
        fd = (FIL *)(file->data);
        RT_ASSERT(fd != RT_NULL);

        result = f_lseek(fd, offset);
        if (result == FR_OK)
        {
            /* return current position */
            file->pos = fd->fptr;
            return fd->fptr;
        }
    }
    else if (file->type == FT_DIRECTORY)
    {
        /* which is a directory */
        FDIR *dir;

        dir = (FDIR *)(file->data);
        RT_ASSERT(dir != RT_NULL);

        result = f_seekdir(dir, offset / sizeof(struct dirent));
        if (result == FR_OK)
        {
            /* update file position */
            file->pos = offset;
            return file->pos;
        }
    }

    return elm_result_to_dfs(result);
}

int dfs_elm_getdents(struct dfs_fd *file, struct dirent *dirp, uint32_t count)
{
    FDIR *dir;
    FILINFO fno;
    FRESULT result;
    rt_uint32_t index;
    struct dirent *d;

    dir = (FDIR *)(file->data);
    RT_ASSERT(dir != RT_NULL);

    /* make integer count */
    count = (count / sizeof(struct dirent)) * sizeof(struct dirent);
    if (count == 0)
        return -EINVAL;

    index = 0;
    while (1)
    {
        char *fn;
        char namebuf[_MAX_LFN + 7] = {0};
        d = dirp + index;

        result = f_readdir(dir, &fno);
        if (result != FR_OK || fno.fname[0] == 0)
            break;

#if _USE_LFN
        fn = *fno.fname ? fno.fname : fno.altname;
#else
        fn = fno.fname;
#endif
        invaildate_finfo_cache(-1);
        createfullpath(file->data, namebuf, file->path, fn);
        add_finfo_cache(namebuf, &fno);

        d->d_type = DT_UNKNOWN;
        if (fno.fattrib & AM_DIR)
            d->d_type = DT_DIR;
        else
            d->d_type = DT_REG;

        d->d_namlen = (rt_uint8_t)rt_strlen(fn);
        d->d_reclen = (rt_uint16_t)sizeof(struct dirent);
        rt_strncpy(d->d_name, fn, rt_strlen(fn) + 1);

        index ++;
        if (index * sizeof(struct dirent) >= count)
            break;
    }

    if (index == 0)
        return elm_result_to_dfs(result);

    file->pos += index * sizeof(struct dirent);

    return index * sizeof(struct dirent);
}

int dfs_elm_unlink(struct dfs_filesystem *fs, const char *path)
{
    FRESULT result;

#if _VOLUMES > 1
    int vol;
    char *drivers_fn;
    extern int elm_get_vol(FATFS * fat);

    invaildate_finfo_cache(-1);
    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
        return -ENOENT;
    drivers_fn = rt_malloc(256);
    if (drivers_fn == RT_NULL)
        return -ENOMEM;

    rt_snprintf(drivers_fn, 256, "%d:%s", vol, path);
#else
    const char *drivers_fn;
    drivers_fn = path;
#endif

    result = f_unlink(drivers_fn);
#if _VOLUMES > 1
    rt_free(drivers_fn);
#endif
    return elm_result_to_dfs(result);
}

int dfs_elm_rename(struct dfs_filesystem *fs, const char *oldpath, const char *newpath)
{
    FRESULT result;

    invaildate_finfo_cache(-1);
#if _VOLUMES > 1
    char *drivers_oldfn;
    const char *drivers_newfn;
    int vol;
    extern int elm_get_vol(FATFS * fat);

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
        return -ENOENT;

    drivers_oldfn = rt_malloc(256);
    if (drivers_oldfn == RT_NULL)
        return -ENOMEM;
    drivers_newfn = newpath;

    rt_snprintf(drivers_oldfn, 256, "%d:%s", vol, oldpath);
#else
    const char *drivers_oldfn, *drivers_newfn;

    drivers_oldfn = oldpath;
    drivers_newfn = newpath;
#endif

    result = f_rename(drivers_oldfn, drivers_newfn);
#if _VOLUMES > 1
    rt_free(drivers_oldfn);
#endif
    return elm_result_to_dfs(result);
}

int dfs_elm_stat(struct dfs_filesystem *fs, const char *path, struct stat *st)
{
    FILINFO file_info;
    FRESULT result;

#if _VOLUMES > 1
    int vol;
    char *drivers_fn;
    extern int elm_get_vol(FATFS * fat);

    /* add path for ELM FatFS driver support */
    vol = elm_get_vol((FATFS *)fs->data);
    if (vol < 0)
        return -ENOENT;
    drivers_fn = rt_malloc(256);
    if (drivers_fn == RT_NULL)
        return -ENOMEM;

    rt_snprintf(drivers_fn, 256, "%d:%s", vol, path);
#else
    const char *drivers_fn;
    drivers_fn = path;
#endif
    char namebuff[_MAX_LFN + 7] = {0};

    if (path[0] == '/')
        createfullpath(fs, namebuff, NULL, (char *)path);
    else
        createfullpath(fs, namebuff, fs->path, (char *)path);

    if (find_finfo_cache(namebuff, &file_info) != 1)
        result = f_stat(drivers_fn, &file_info);
    else
        result = FR_OK;

#if _VOLUMES > 1
    rt_free(drivers_fn);
#endif
    if (result == FR_OK)
    {
        /* convert to dfs stat structure */
        st->st_dev = 0;

        st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH |
                      S_IWUSR | S_IWGRP | S_IWOTH;
        if (file_info.fattrib & AM_DIR)
        {
            st->st_mode &= ~S_IFREG;
            st->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;
        }
        if (file_info.fattrib & AM_RDO)
            st->st_mode &= ~(S_IWUSR | S_IWGRP | S_IWOTH);

        st->st_size  = file_info.fsize;

        /* get st_mtime. */
        {
            struct tm tm_file;
            int year, mon, day, hour, min, sec;
            WORD tmp;

            tmp = file_info.fdate;
            day = tmp & 0x1F;           /* bit[4:0] Day(1..31) */
            tmp >>= 5;
            mon = tmp & 0x0F;           /* bit[8:5] Month(1..12) */
            tmp >>= 4;
            year = (tmp & 0x7F) + 1980; /* bit[15:9] Year origin from 1980(0..127) */

            tmp = file_info.ftime;
            sec = (tmp & 0x1F) * 2;     /* bit[4:0] Second/2(0..29) */
            tmp >>= 5;
            min = tmp & 0x3F;           /* bit[10:5] Minute(0..59) */
            tmp >>= 6;
            hour = tmp & 0x1F;          /* bit[15:11] Hour(0..23) */

            memset(&tm_file, 0, sizeof(tm_file));
            tm_file.tm_year = year - 1900; /* Years since 1900 */
            tm_file.tm_mon  = mon - 1;     /* Months *since* january: 0-11 */
            tm_file.tm_mday = day;         /* Day of the month: 1-31 */
            tm_file.tm_hour = hour;        /* Hours since midnight: 0-23 */
            tm_file.tm_min  = min;         /* Minutes: 0-59 */
            tm_file.tm_sec  = sec;         /* Seconds: 0-59 */

            st->st_mtime = mktime(&tm_file);
        } /* get st_mtime. */
    }

    return elm_result_to_dfs(result);
}

static const struct dfs_file_ops dfs_elm_fops = 
{
    .open = dfs_elm_open,
    .close = dfs_elm_close,
    .ioctl = dfs_elm_ioctl,
    .read = dfs_elm_read,
    .write = dfs_elm_write,
    .flush = dfs_elm_flush,
    .lseek = dfs_elm_lseek,
    .getdents = dfs_elm_getdents,
    .ftruncate = dfs_elm_ftruncate,
    .fhide = dfs_elm_fhide,
    .fallocate = dfs_elm_fallocate,
};

static const struct dfs_filesystem_ops dfs_elm =
{
    .name = "elm",
    .flags = DFS_FS_FLAG_DEFAULT,
    .fops = &dfs_elm_fops,
    .mount = dfs_elm_mount,
    .unmount = dfs_elm_unmount,
    .mkfs = dfs_elm_mkfs,
    .statfs = dfs_elm_statfs,
    .unlink = dfs_elm_unlink,
    .stat = dfs_elm_stat,
    .rename = dfs_elm_rename,
};

int elm_init(void)
{
    /* register fatfs file system */
    dfs_register(&dfs_elm);
#ifdef RT_DFS_ELM_BLK_CACHE
    api_dfs_block_cache_init();
#endif
    return 0;
}
INIT_COMPONENT_EXPORT(elm_init);

/*
 * RT-Thread Device Interface for ELM FatFs
 */
#include "diskio.h"

/* Initialize a Drive */
DSTATUS disk_initialize(BYTE drv)
{

    return 0;
}

/* Return Disk Status */
DSTATUS disk_status(BYTE drv)
{
    return 0;
}

/* Read Sector(s) */
DRESULT disk_read (BYTE drv, BYTE* buff, DWORD sector, UINT count)
{
    rt_size_t result;
    rt_device_t device = disk[drv];

    if (device)
    {
#ifdef RT_DFS_ELM_BLK_CACHE
        int ret;
        ret = api_blk_cache_read(api_dfs_get_cache_handle(), sector, (void *)buff, count);
        if (ret)
            return RES_ERROR;
        return RES_OK;
#else
        result = rt_device_read(device, sector, buff, count);
        if (result == count)
        {
            return RES_OK;
        }
#endif
    }
    return RES_ERROR;
}

/* Write Sector(s) */
DRESULT disk_write (BYTE drv, const BYTE* buff, DWORD sector, UINT count)
{
    rt_size_t result;
    rt_device_t device = disk[drv];

    if (device)
    {
#ifdef RT_DFS_ELM_BLK_CACHE
        int ret;
        ret = api_blk_cache_write(api_dfs_get_cache_handle(), sector, (const void *)buff, count);
        if (ret)
            return RES_ERROR;
        return RES_OK;
#else
        result = rt_device_write(device, sector, buff, count);
        if (result == count)
        {
            return RES_OK;
        }
#endif
    }

    return RES_ERROR;
}

/* Miscellaneous Functions */
DRESULT disk_ioctl(BYTE drv, BYTE ctrl, void *buff)
{
    rt_device_t device = disk[drv];

    if (device == RT_NULL)
        return RES_ERROR;

    if (ctrl == GET_SECTOR_COUNT)
    {
        struct rt_device_blk_geometry geometry;

        rt_memset(&geometry, 0, sizeof(geometry));
        rt_device_control(device, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry);

        *(DWORD *)buff = geometry.sector_count;
        if (geometry.sector_count == 0)
            return RES_ERROR;
    }
    else if (ctrl == GET_SECTOR_SIZE)
    {
        struct rt_device_blk_geometry geometry;

        rt_memset(&geometry, 0, sizeof(geometry));
        rt_device_control(device, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry);

        *(WORD *)buff = (WORD)(geometry.bytes_per_sector);
    }
    else if (ctrl == GET_BLOCK_SIZE) /* Get erase block size in unit of sectors (DWORD) */
    {
        struct rt_device_blk_geometry geometry;

        rt_memset(&geometry, 0, sizeof(geometry));
        rt_device_control(device, RT_DEVICE_CTRL_BLK_GETGEOME, &geometry);

        *(DWORD *)buff = geometry.block_size / geometry.bytes_per_sector;
    }
    else if (ctrl == CTRL_SYNC)
    {

#ifdef RT_DFS_ELM_BLK_CACHE
        api_flush_all_blk_cache(api_dfs_get_cache_handle());
#endif
        rt_device_control(device, RT_DEVICE_CTRL_BLK_SYNC, RT_NULL);

    }
    else if (ctrl == CTRL_TRIM)
    {
        rt_device_control(device, RT_DEVICE_CTRL_BLK_ERASE, buff);
    }

    return RES_OK;
}

DWORD get_fattime(void)
{
    DWORD fat_time = 0;

#ifdef RT_USING_LIBC
    time_t now;
    struct tm *p_tm;
    struct tm tm_now;
    struct timeval tval;
    /* get current time */
    gettimeofday(&tval, RT_NULL);
    now = tval.tv_sec;

    /* lock scheduler. */
    rt_enter_critical();
    /* converts calendar time time into local time. */
    p_tm = localtime(&now);
    /* copy the statically located variable */
    memcpy(&tm_now, p_tm, sizeof(struct tm));
    /* unlock scheduler. */
    rt_exit_critical();

    fat_time =  (DWORD)(tm_now.tm_year - 80) << 25 |
                (DWORD)(tm_now.tm_mon + 1)   << 21 |
                (DWORD)tm_now.tm_mday        << 16 |
                (DWORD)tm_now.tm_hour        << 11 |
                (DWORD)tm_now.tm_min         <<  5 |
                (DWORD)tm_now.tm_sec / 2 ;
#endif /* RT_USING_LIBC  */

    return fat_time;
}

#if _FS_REENTRANT
int ff_cre_syncobj(BYTE drv, _SYNC_t *m)
{
    char name[8];
    rt_mutex_t mutex;

    rt_snprintf(name, sizeof(name), "fat%d", drv);
    mutex = rt_mutex_create(name, RT_IPC_FLAG_FIFO);
    if (mutex != RT_NULL)
    {
        *m = mutex;
        return RT_TRUE;
    }

    return RT_FALSE;
}

int ff_del_syncobj(_SYNC_t m)
{
    if (m != RT_NULL)
        rt_mutex_delete(m);

    return RT_TRUE;
}

int ff_req_grant(_SYNC_t m)
{
    if (rt_mutex_take(m, _FS_TIMEOUT) == RT_EOK)
        return RT_TRUE;

    return RT_FALSE;
}

void ff_rel_grant(_SYNC_t m)
{
    rt_mutex_release(m);
}

#endif

/* Memory functions */
#if _USE_LFN == 3
/* Allocate memory block */
void *ff_memalloc(UINT size)
{
    return rt_malloc(size);
}

/* Free memory block */
void ff_memfree(void *mem)
{
    rt_free(mem);
}
#endif /* _USE_LFN == 3 */



