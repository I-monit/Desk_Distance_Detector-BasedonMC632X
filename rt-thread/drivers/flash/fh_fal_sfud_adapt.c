/**
 * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 *
 * Change Logs:
 * Date           Author       Notes
 * 2019-02-21    zhangy       first version
 *
 */

#include <rtdevice.h>
#include "fh_fal_sfud_adapt.h"
#include "fh_arch.h"
#include "board_info.h"
#include "fal.h"

#define __MAX_FAL_SFUD_ADAPT_PART_SIZE      FAL_PARTITION_TABLE_SIZE

static struct fal_flash_dev fh_fal_sfud_flash_obj = {0};
static struct fal_partition fh_fal_sfud_part_obj[__MAX_FAL_SFUD_ADAPT_PART_SIZE] = {0};
#if 0
static struct fh_fal_sfud_adapt_plat_info g_plat_info = {0};
#endif
static int fh_fal_sfud_adapt_ops_read(sfud_flash *p_sfud_flash, long offset, uint8_t *buf, size_t size)
{
    if (sfud_read(p_sfud_flash, offset, size, buf) != SFUD_SUCCESS)
    {
        return -1;
    }
    return size;
}

static int fh_fal_sfud_adapt_ops_write(sfud_flash *p_sfud_flash, long offset, const uint8_t *buf, size_t size)
{
    if (sfud_write(p_sfud_flash, offset, size, buf) != SFUD_SUCCESS)
    {
        return -1;
    }
    return size;

}

static int fh_fal_sfud_adapt_ops_erase(sfud_flash *p_sfud_flash, long offset, size_t size)
{
    if (sfud_erase(p_sfud_flash, offset, size) != SFUD_SUCCESS)
    {
        return -1;
    }
    return size;

}

static int fh_fal_sfud_adapt_ops_init(void)
{
    return 0;
}


void fh_fal_parse_part_info_para(struct fh_fal_partition_adapt_info *p_part_info)
{
    if ((p_part_info->erase_block_size != FH_FAL_PART_BLK_SIZE_4K) &&
    (p_part_info->erase_block_size != FH_FAL_PART_BLK_SIZE_64K)){
        p_part_info->erase_block_size = FH_FAL_PART_BLK_SIZE_UNSET;
    }
}
int fh_fal_sfud_adapt_probe(void *p_plat)
{
    int i;
    rt_spi_flash_device_t rt_p_flash_dev = RT_NULL;
    struct fh_fal_sfud_adapt_plat_info *p_info = (struct fh_fal_sfud_adapt_plat_info *)p_plat;
    struct fh_fal_partition_adapt_info *p_part_info = p_info->parts;

    if (rt_strlen(p_info->spi_name) > FAL_DEV_NAME_MAX ||
        rt_strlen(p_info->flash_name) > FAL_DEV_NAME_MAX)
    {
        rt_kprintf("spi:[%s] len : %d or flash:[%s] len : %d larger than %d\n",
        p_info->spi_name, rt_strlen(p_info->spi_name), p_info->flash_name,
        rt_strlen(p_info->flash_name), FAL_DEV_NAME_MAX);
        return -1;
    }

    if (p_info->nr_parts > __MAX_FAL_SFUD_ADAPT_PART_SIZE)
    {
        rt_kprintf("part size: %d larger than %d\n", p_info->nr_parts, __MAX_FAL_SFUD_ADAPT_PART_SIZE);
        return -1;
    }
    for (i = 0; i < p_info->nr_parts ; i++)
    {
        if (rt_strlen(p_part_info[i].name) > FAL_DEV_NAME_MAX)
        {
            rt_kprintf("part:[%s] : %d larger than %d\n",
                       p_part_info[i].name, rt_strlen(p_part_info[i].name), FAL_DEV_NAME_MAX);
            return -1;
        }
    }

#if 0
    rt_memcpy(&g_plat_info, p_info, sizeof(struct fh_fal_sfud_adapt_plat_info));
#endif
    /*flash para cpy*/
    rt_strncpy(&fh_fal_sfud_flash_obj.name[0], p_info->flash_name, rt_strlen(p_info->flash_name));
    /*soft process base add is 0..*/
    fh_fal_sfud_flash_obj.addr = 0;

    fh_fal_sfud_flash_obj.ops.init = fh_fal_sfud_adapt_ops_init;
    fh_fal_sfud_flash_obj.ops.read = fh_fal_sfud_adapt_ops_read;
    fh_fal_sfud_flash_obj.ops.write = fh_fal_sfud_adapt_ops_write;
    fh_fal_sfud_flash_obj.ops.erase = fh_fal_sfud_adapt_ops_erase;

    /*part para cpy*/
    for (i = 0; i < p_info->nr_parts ; i++)
    {
        fh_fal_sfud_part_obj[i].magic_word = FAL_PART_MAGIC_WROD;
        rt_strncpy(&fh_fal_sfud_part_obj[i].name[0], p_part_info[i].name, rt_strlen(p_part_info[i].name));
        rt_strncpy(&fh_fal_sfud_part_obj[i].flash_name[0], p_info->flash_name, rt_strlen(p_info->flash_name));
        fh_fal_sfud_part_obj[i].offset = p_part_info[i].offset;
        fh_fal_sfud_part_obj[i].len = p_part_info[i].size;
        fh_fal_parse_part_info_para(&p_part_info[i]);
        fh_fal_sfud_part_obj[i].erase_block_size = p_part_info[i].erase_block_size;
        /*do not parse cache enable..*/
    }
    rt_p_flash_dev = rt_sfud_flash_probe(p_info->flash_name, p_info->spi_name);
    if (!rt_p_flash_dev)
    {
        rt_kprintf("%s init error..\n", __func__);
        rt_p_flash_dev = RT_NULL;
        goto error;
    }

    fh_fal_sfud_flash_obj.len = rt_p_flash_dev->geometry.sector_count * rt_p_flash_dev->geometry.bytes_per_sector;
    fh_fal_sfud_flash_obj.blk_size = rt_p_flash_dev->geometry.block_size;
    /*Connect FAL Flash Device & SPI Device*/
    fh_fal_sfud_flash_obj.user_data = rt_p_flash_dev;
    fal_init(&fh_fal_sfud_flash_obj, 1, &fh_fal_sfud_part_obj[0], p_info->nr_parts);
    for (i = 0; i <  p_info->nr_parts; i++)
    {
        if (!fal_mtd_nor_device_create(p_info->parts[i].name))
        {
            rt_kprintf("creat mtd not dev : %s error\n",p_info->parts[i].name);
            goto error;
        }
    }
    return 0;
error:
    return -1;
}

int fh_fal_sfud_exit(void *p_plat)
{
    return 0;
}

int fh_fal_sfud_shutdown(void *p_plat)
{
    rt_device_t flash_dev;
    struct fh_fal_sfud_adapt_plat_info *p_info = (struct fh_fal_sfud_adapt_plat_info *)p_plat;

    flash_dev = rt_device_find(p_info->flash_name);
    if (!flash_dev)
    {
        rt_kprintf("cann't find the flash dev %s\n",p_info->flash_name);
        return -1;
    }
#ifdef RT_USING_DEVICE_OPS
    flash_dev->ops->control(flash_dev, 0xff, 0);
#else
    flash_dev->control(flash_dev, 0xff, 0);
#endif
    return 0;
}

struct fh_board_ops fh_fal_sfud_driver_ops = {

    .probe = fh_fal_sfud_adapt_probe,
    .exit = fh_fal_sfud_exit,
    .shutdown = fh_fal_sfud_shutdown,

};


void fh_fal_sfud_adapt_init(void)
{

    /*rt_kprintf("%s start\n", __func__);*/
    fh_board_driver_register("fh_fal_sfud_adapt", &fh_fal_sfud_driver_ops);
    /*rt_kprintf("%s end\n", __func__);*/
    /* fixme: never release? */
}
