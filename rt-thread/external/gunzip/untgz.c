#include <rtdef.h>
#include <rtthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>
#include "dfs.h"

extern int gunzip(unsigned char *buf, int len,
               int (*fill)(void*, unsigned int),
               int (*flush)(void*, unsigned int),
               unsigned char *out_buf,
               int *pos,
               unsigned int *olen,
               void (*error)(char *x));
void untgz_report(char *szbuf)
{
    rt_kprintf("\n\n");
    rt_kprintf(szbuf);
    rt_kprintf("\n\n");
}

#define TGZ_DEBUG
#ifdef TGZ_DEBUG
#define tgz_info(format, arg...)   rt_kprintf(format, ##arg)
#else
#define tgz_info(format, arg...)
#endif

static char *usr_path = NULL;

struct file_header_t
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char linkflag;
    char linkname[100];
    char magic[8];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
};

union record
{
    char charptr[512];
    struct file_header_t header;
};

static int rt_pow8(int y)
{
    int sum = 1;

    sum = sum << (y * 3);
    return sum;
}

static int Get_tar_headerSize(char *size)
{
    int D_size = 0;
    int i;

    for (i = 0; i < 11; i++)
    {
        D_size = D_size + (size[i] - 0x30) * rt_pow8(10 - i);
    }
    return D_size;
}

static int Get_tar_Checksum(char *checksum)
{
    int D_size = 0;
    int i;

    for (i = 1; i < 6; i++)
    {
        D_size = D_size + (checksum[i] - 0x30) * rt_pow8(5 - i);
    }
    return D_size;
}

static int Check_tar_header(struct file_header_t *tar_header)
{
    int i;
    unsigned int crc_sum = 0;
    char *p_buf = (char *)tar_header;

    if (rt_strncmp(tar_header->magic, "ustar", 5))
    {
        rt_kprintf("\n%s: tar magic[%s] verification failed, untar exit!\n\n", __func__, tar_header->magic);
        return -ENOENT;
    }
    for (i = 0; i < sizeof(*tar_header); i++)
    {
        if (i >= 148 && i < 156)
            crc_sum = crc_sum + ' ';
        else
            crc_sum = crc_sum + p_buf[i];
    }
    if (crc_sum != Get_tar_Checksum(tar_header->chksum))
    {
        rt_kprintf("%s: tar checksum verification failed, crc_sum %d, checksum %d\n",
                        __func__, crc_sum, Get_tar_Checksum(tar_header->chksum));
        return -ENOENT;
    }

    return 0;
}

static int Get_decompress_path(char *path, char *usr_path, char *file_name, char *mode)
{
    char path_end;
    int file_mode = atoi(mode);

    if (rt_strlen(usr_path) + rt_strlen(file_name) >= DFS_PATH_MAX)
    {
        printf("The name(%d) or path(%d) of the extracted file is too long!\n",
            (int)rt_strlen(file_name), (int)rt_strlen(usr_path));
        return -EINVAL;
    }

    rt_memset(path, 0, DFS_PATH_MAX);
    if (rt_strlen(usr_path) > 1)
    {
        rt_strncpy(path, usr_path, rt_strlen(usr_path));

        path_end = *(path + rt_strlen(path) - 1);
        if (path_end != '/')
            strcat(path, "/");

        if (*(file_name + rt_strlen(file_name) - 1) == '/')
        {
            *(file_name + rt_strlen(file_name) - 1) = '\0';
            strcat(path, file_name);
            mkdir(path, file_mode);
            return file_mode;
        }
        strcat(path, file_name);
    } else
    {
        if (*(file_name + rt_strlen(file_name) - 1) == '/')
        {
            *(file_name + rt_strlen(file_name) - 1) = '\0';
            mkdir(file_name, file_mode);
            return file_mode;
        }
        rt_strncpy(path, file_name, rt_strlen(file_name));
    }
    return 0;
}

static int tar_data_analysis(char *buf, int len, char *usr_path)
{
    FILE *fp;
    char *p_buf;
    char *data_buf;
    char cur_path = '\0';
    char *path;
    unsigned int offset = 0;
    unsigned int align_data;
    unsigned int data_size = 0;
    struct file_header_t *tar_header;
    int ret = 0;

    p_buf = buf;
    if (!usr_path)
        usr_path = &cur_path;

    path = rt_malloc(DFS_PATH_MAX);
    if (path == NULL)
    {
        printf("%s:malloc(path) buf failed!\n", __func__);
        ret = -ENOMEM;
        goto out;
    }
    while (*p_buf != 0 && offset < len - 1)
    {
        tar_header = (struct file_header_t *)p_buf;
        data_buf = p_buf + 512;
        data_size = Get_tar_headerSize(tar_header->size);

        ret = Check_tar_header(tar_header);
        if (ret)
            goto out;

        ret = Get_decompress_path(path, usr_path, tar_header->name, tar_header->mode);
        if (ret < 0)
            goto out1;
        else if (ret == atoi(tar_header->mode))
        {
            p_buf = p_buf + 512;
            continue;
        }

        fp = fopen(path, "w+");
        if (!fp)
        {
            printf("create file %s failed!\n", path);
            ret =  -EIO;
            goto out1;
        }
        fwrite(data_buf, data_size, 1, fp);
        fclose(fp);
        fp = NULL;

        tgz_info("decompress file::name:%s", path);
        tgz_info("    size:%d\n", data_size);

        align_data = (data_size + 511) & 0xfffffe00;
        p_buf = p_buf + 512 + align_data;
        offset = p_buf - buf;
    }

out1:
    rt_free(path);
out:
    return ret;
}

int untar(char *name, char *path)
{
    FILE *fp;
    char *buf;
    unsigned int len;
    int ret = -1;

    if (!name)
    {
        ret = -EINVAL;
        goto out;
    }

    fp = fopen(name, "r");
    if (!fp)
    {
        printf("%s:open %s failed!\n", __func__, name);
        ret = -ENOENT;
        goto out;
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    tgz_info("%s:get %s length: %d\n", __func__, name, len);

    rewind(fp);
    buf = rt_malloc(len);
    if (buf == NULL)
    {
        printf("%s:malloc(tar) buf failed!\n", __func__);
        ret = -ENOMEM;
        goto out1;
    }
    ret = fread(buf, len, 1, fp);
    if (ret != 1)
    {
        printf("%s:fread error!\n", __func__);
        goto out2;
    }
    fclose(fp);
    fp = NULL;
    ret = tar_data_analysis(buf, len, path);
    rt_free(buf);

    return ret;
out2:
    rt_free(buf);
out1:
    fclose(fp);
    fp = NULL;
out:
    return ret;
}

static int tgz_file_flush(void *buf, unsigned int len)
{
    static FILE *fp = NULL;
    char *p_buf;
    char *data_buf;
    char cur_path = '\0';
    unsigned int total_len = len;
    unsigned int header_len = len;
    static char path[DFS_PATH_MAX];
    static char last_header[512];
    static int last_header_size = 0;
    static unsigned int data_size = 0;
    static unsigned int align_data;
    struct file_header_t *tar_header;
    int ret = 0;

    if (!usr_path)
    {
        tgz_info("untgz extract the file to the current directory!\n", __func__);
        usr_path = &cur_path;
    }
    p_buf = buf;
    do
    {
        if (!fp)
        {
            header_len = 512 - last_header_size;
            if (last_header_size)
            {
                rt_memcpy(last_header + last_header_size, p_buf, header_len);
                tar_header = (struct file_header_t *)last_header;
            } else
            {
                tar_header = (struct file_header_t *)p_buf;
            }

            ret = Check_tar_header(tar_header);
            if (ret)
                goto out;

            ret = Get_decompress_path(path, usr_path, tar_header->name, tar_header->mode);
            if (ret < 0)
                goto out1;
            else if (ret == atoi(tar_header->mode))
            {
                p_buf = p_buf + header_len;
                len = len -header_len;
                continue;
            }
            data_buf = p_buf + header_len;
            data_size = Get_tar_headerSize(tar_header->size);
            align_data = (data_size + 511) & 0xfffffe00;

            fp = fopen(path, "w+");
            if (!fp)
            {
                printf("%s-%d:open %s failed!\n", __func__, __LINE__, path);
                ret = -ENOENT;
                goto out;
            }
            tgz_info("decompress file::name:%s", path);
            tgz_info("    size:%d\n", data_size);
            len = len - header_len;
            if (data_size > len)
            {
                fwrite(data_buf, len, 1, fp);
                p_buf = p_buf + header_len + len;
                data_size = data_size - len;
                align_data = align_data -len;
                break;
            } else
            {
                fwrite(data_buf, data_size, 1, fp);
                p_buf = p_buf + header_len + align_data;
                len = len - align_data;
                align_data = 0;
                data_size = 0;
            }
            fclose(fp);
            fp = NULL;
            last_header_size = 0;
            if (len < 512 && len)
            {
                rt_memcpy(last_header, p_buf, len);
                last_header_size = len;
                break;
            }
        }

        if (fp)
        {
            if (data_size > len)
            {
                fwrite(buf, len, 1, fp);
                data_size = data_size - len;
                align_data = align_data -len;
                break;
            } else
            {
                fwrite(buf, data_size, 1, fp);
                p_buf = buf + align_data;
                len = len - align_data;
                data_size = 0;
                align_data = 0;
            }
            fclose(fp);
            fp = NULL;
            last_header_size = 0;
            if (len < 512 && len)
            {
                rt_memcpy(last_header, p_buf, len);
                last_header_size = len;
                break;
            }
        }
    } while (*p_buf != 0 && len);

    return total_len;
out1:
    fclose(fp);
    fp = NULL;
out:
    return ret;
}

int untgz(char *name, char *path)
{
    unsigned int olen;
    unsigned int tgz_len = 0;
    unsigned char *buf;
    struct timeval tv1, tv2;
    unsigned long int untgz_time;
    FILE *tgz_fp = NULL;
    int ret = -1;

    if (!name)
    {
        ret = -EINVAL;
        goto out;
    }
    usr_path = path;

    tgz_fp = fopen(name, "r");
    if (!tgz_fp)
    {
        printf("%s:open %s failed!\n", __func__, name);
        ret = -ENOENT;
        goto out;
    }
    fseek(tgz_fp, 0, SEEK_END);
    tgz_len = ftell(tgz_fp);
    tgz_info("%s:get %s length: %d\n", __func__, name, tgz_len);
    rewind(tgz_fp);

    buf = rt_malloc(tgz_len);
    if (buf == NULL)
    {
        printf("%s:malloc(gunzip) buf(size:%d) failed!\n", __func__, tgz_len);
        ret = -ENOMEM;
        goto out1;
    }
    ret = fread(buf, tgz_len, 1, tgz_fp);
    if (ret != 1)
    {
        printf("%s:fread error!\n", __func__);
        goto out2;
    }
    gettimeofday(&tv1, NULL);
    ret = gunzip(buf, tgz_len, NULL, tgz_file_flush, NULL, NULL, &olen, untgz_report);
    if (ret)
    {
        printf("%s:gunzip decompress faile ret = %d!", __func__,ret);
        goto out2;
    }
    gettimeofday(&tv2, NULL);
    untgz_time = (tv2.tv_sec-tv1.tv_sec)*1000000+(tv2.tv_usec - tv1.tv_usec);
    printf("%s: untgz file %s complete, process times %ld ms\n", __func__, name, untgz_time/1000);
    fclose(tgz_fp);
    tgz_fp = NULL;
    rt_free(buf);

    return ret;
out2:
    rt_free(buf);
out1:
    fclose(tgz_fp);
    tgz_fp = NULL;
out:
    return ret;
}

/**
 * #ifdef RT_USING_FINSH
 * #include <finsh.h>
 * FINSH_FUNCTION_EXPORT(untar, untar("path.tar", "mnt/tar/"));
 * FINSH_FUNCTION_EXPORT(untgz, untgz("path.tgz", "mnt/tgz/"));
 * #endif
 */
