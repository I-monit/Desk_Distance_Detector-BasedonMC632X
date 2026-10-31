#include <rtdevice.h>
#include "fh_fal_sfud_adapt.h"
#include <spi-nand.h>
#include "board_info.h"


#define __MAX_NAND_FLASH_PART_SIZE      10

static struct fal_flash_dev fh_fal_nand_flash_obj = {0};
static struct fal_partition fh_fal_nand_part_obj[__MAX_NAND_FLASH_PART_SIZE] = {0};
static struct fh_fal_sfud_adapt_plat_info g_plat_info = {0};

int fh_nand_flash_probe(void *p_plat)
{
    int i;
    rt_mtd_t * mtd_info = RT_NULL;
    struct fh_fal_sfud_adapt_plat_info *p_info = (struct fh_fal_sfud_adapt_plat_info *)p_plat;
    struct fh_fal_partition_adapt_info *p_part_info = p_info->parts;

    if(rt_strlen(p_info->spi_name) > FAL_DEV_NAME_MAX)
    {
        rt_kprintf("spi:[%s] len : %d larger than %d\n",p_info->spi_name,rt_strlen(p_info->spi_name),FAL_DEV_NAME_MAX);
        return -1;
    }

    if(rt_strlen(p_info->flash_name) > FAL_DEV_NAME_MAX)
    {
        rt_kprintf("flash:[%s] len : %d larger than %d\n",p_info->flash_name,rt_strlen(p_info->spi_name),FAL_DEV_NAME_MAX);
        return -1;
    }

    if(p_info->nr_parts > __MAX_NAND_FLASH_PART_SIZE)
    {
        rt_kprintf("part size : %d larger than %d\n",p_info->nr_parts,__MAX_NAND_FLASH_PART_SIZE);
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

    mtd_info = rt_nand_flash_probe(p_info->flash_name, p_info->spi_name);
    if (!mtd_info)
    {
        rt_kprintf("%s init error..\n", __func__);
        mtd_info = RT_NULL;
        goto error;
    }

    rt_memcpy(&g_plat_info, p_info, sizeof(struct fh_fal_sfud_adapt_plat_info));
    /*flash para cpy*/
    rt_strncpy(&fh_fal_nand_flash_obj.name[0], p_info->flash_name, rt_strlen(p_info->flash_name));
    /*soft process base add is 0..*/
    fh_fal_nand_flash_obj.addr = 0;

    /*part para cpy*/
    for (i = 0; i < p_info->nr_parts ; i++)
    {
        fh_fal_nand_part_obj[i].magic_word = FAL_PART_MAGIC_WROD;
        rt_strncpy(&fh_fal_nand_part_obj[i].name[0], p_part_info[i].name, rt_strlen(p_part_info[i].name));
        rt_strncpy(&fh_fal_nand_part_obj[i].flash_name[0], p_info->flash_name, rt_strlen(p_info->flash_name));
        fh_fal_nand_part_obj[i].offset = p_part_info[i].offset;
        fh_fal_nand_part_obj[i].len = p_part_info[i].size;
        fh_fal_nand_part_obj[i].erase_block_size = p_part_info[i].erase_block_size;
        /*do not parse cache enable..*/
    }

    fh_fal_nand_flash_obj.len = mtd_info->size;
    fh_fal_nand_flash_obj.blk_size = mtd_info->block_size;
    /*Connect FAL Flash Device & SPI Device*/
    fh_fal_nand_flash_obj.user_data = mtd_info;
    fal_init(&fh_fal_nand_flash_obj, 1, &fh_fal_nand_part_obj[0], p_info->nr_parts);
    for (i = 0; i <  p_info->nr_parts; i++)
    {
        if (!fal_mtd_nand_device_create(p_info->parts[i].name))
        {
            rt_kprintf("creat mtd not dev : %s error\n",p_info->parts[i].name);
            goto error;
        }
    }

error:
    return -1;

}

int fh_nand_flash_exit()
{
    return 0;
}

int fh_nand_flash_shutdown()
{
    return 0;
}

struct fh_board_ops fh_nand_flash_driver_ops={
    .probe = fh_nand_flash_probe,
    .exit = fh_nand_flash_exit,
    .shutdown = fh_nand_flash_shutdown,
};


void fh_nand_flash_init(void)
{
    // rt_kprintf("%s start\n", __func__);
    fh_board_driver_register("fh_nand_flash", &fh_nand_flash_driver_ops);
    // rt_kprintf("%s end\n", __func__);
}