
/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-02-21     zhangy       first version
 */

#ifndef __FH_FAL_SFUD_ADAPT_H__
#define __FH_FAL_SFUD_ADAPT_H__

#include <rtthread.h>
#include "fh_arch.h"
#include <fal_def.h>
#include <fal.h>
#include <sfud.h>
#include <spi_flash.h>
#include <spi_flash_sfud.h>

struct fh_fal_partition_adapt_info
{
    char                            *name;              /* identifier string */
    rt_uint32_t                     size;               /* partition size */
    rt_uint32_t                     offset;             /* offset within the master MTD space */
    int                             cache_enable;
    #define FH_FAL_PART_BLK_SIZE_4K           0x1000
    #define FH_FAL_PART_BLK_SIZE_64K          0x10000
    #define FH_FAL_PART_BLK_SIZE_128K         0x20000
    #define FH_FAL_PART_BLK_SIZE_UNSET        0xdeadbeef
    int                             erase_block_size;
};

/* maybe i should support more than one flash ,but now just one flash is ok.*/
/* make a list or array for platform data..*/
struct fh_fal_sfud_adapt_plat_info
{
    char        *flash_name;
    char        *spi_name;
    struct fh_fal_partition_adapt_info *parts;
    unsigned int    nr_parts;
};

void fh_fal_sfud_adapt_init(void);
void fh_nand_flash_init(void);

#endif

