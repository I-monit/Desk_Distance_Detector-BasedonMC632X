#ifndef __FAL_DEF_H__
#define __FAL_DEF_H__

#include <stdint.h>
#include <stdio.h>
#include <rtthread.h>
#include "sfud_def.h"

#define FAL_SW_VERSION                 "0.4.0"

#ifndef FAL_MALLOC
#define FAL_MALLOC                     malloc
#endif

#ifndef FAL_CALLOC
#define FAL_CALLOC                     calloc
#endif

#ifndef FAL_REALLOC
#define FAL_REALLOC                    realloc
#endif

#ifndef FAL_FREE
#define FAL_FREE                       free
#endif

#ifndef FAL_DEBUG
#define FAL_DEBUG                      0
#endif

#if FAL_DEBUG
#ifdef assert
#undef assert
#endif
#define assert(EXPR) do { \
    if (!(_expr)) \
    { rt_kprintf("%s:%d:\n",  \
                      __FILE__, __LINE__);\
     while (1);\
    } \
} while (0)


/* debug level log */
#ifdef log_d
#undef log_d
#endif
#define log_d(...) do { \
    rt_kprintf("[D/FAL] (%s:%d) ", __func__, __LINE__);\
    rt_kprintf(__VA_ARGS__);\
    rt_kprintf("\n");\
} while (0)

#else

#ifdef assert
#undef assert
#endif
#define assert(EXPR)                   ((void)0)

/* debug level log */
#ifdef log_d
#undef log_d
#endif
#define log_d(...)
#endif /* FAL_DEBUG */

/* error level log */
#ifdef log_e
#undef log_e
#endif
#define log_e(...) do { \
    rt_kprintf("\033[31;22m[E/FAL] (%s:%d) ", __func__, __LINE__);\
    rt_kprintf(__VA_ARGS__);\
    rt_kprintf("\033[0m\n");\
} while (0)

/* info level log */
#ifdef log_i
#undef log_i
#endif
#define log_i(...) do { \
    rt_kprintf("\033[32;22m[I/FAL] ");\
    rt_kprintf(__VA_ARGS__);\
    rt_kprintf("\033[0m\n");\
} while (0)


/* FAL flash and partition device name max length */
#ifndef FAL_DEV_NAME_MAX
#define FAL_DEV_NAME_MAX    24
#endif

#ifndef FAL_PARTITION_TABLE_SIZE
#define FAL_PARTITION_TABLE_SIZE    RT_MAX_FAL_PART_NUM
#endif

#ifndef FAL_FLASH_DEV_TABLE_SIZE
#define FAL_FLASH_DEV_TABLE_SIZE    1
#endif

/* partition magic word */
#define FAL_PART_MAGIC_WROD         0x45503130
#define FAL_PART_MAGIC_WROD_H       0x4550L
#define FAL_PART_MAGIC_WROD_L       0x3130L

struct fal_flash_dev_ops
{
    int (*init)(void);
    int (*read)(sfud_flash *p_sfud_flash, long offset, uint8_t *buf, size_t size);
    int (*write)(sfud_flash *p_sfud_flash, long offset, const uint8_t *buf, size_t size);
    int (*erase)(sfud_flash *p_sfud_flash, long offset, size_t size);
};

struct fal_flash_dev
{
    char name[FAL_DEV_NAME_MAX];

    /* flash device start address and len  */
    uint32_t addr;
    size_t len;
    /* the block size in the flash for erase minimum granularity */
    size_t blk_size;

    struct fal_flash_dev_ops ops;
    /* Contain rt_spi_flash_device_t */
    void *user_data;
};
typedef struct fal_flash_dev *fal_flash_dev_t;

/**
 * FAL partition
 */
struct fal_partition
{
    uint32_t magic_word;

    /* partition name */
    char name[FAL_DEV_NAME_MAX];
    /* flash device name for partition */
    char flash_name[FAL_DEV_NAME_MAX];

    /* partition offset address on flash device */
    long offset;
    size_t len;

    uint32_t erase_block_size;
    /* Contain fal flash device */
    void *user_data;
    uint32_t reserved;
};
typedef struct fal_partition *fal_partition_t;

#endif /* _FAL_DEF_H_ */
