#include <fal.h>

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#ifdef RT_USING_POSIX
#include <drivers/block.h>
#endif

#include "spi_flash.h"
#include "sfud_def.h"

#ifdef FH_USING_NAND_FLASH
#include <drivers/mtd_nand.h>
#include <f_errno.h>
#endif
/* ========================== block device ======================== */
struct fal_blk_device
{
    struct rt_device                parent;
    struct rt_device_blk_geometry   geometry;
    const struct fal_partition     *fal_part;
};

/* RT-Thread device interface */
#if RTTHREAD_VERSION >= 30000
static rt_err_t blk_dev_control(rt_device_t dev, int cmd, void *args)
#else
static rt_err_t blk_dev_control(rt_device_t dev, rt_uint8_t cmd, void *args)
#endif
{
    struct fal_blk_device *part = (struct fal_blk_device *)dev;

    assert(part != RT_NULL);

    if (cmd == RT_DEVICE_CTRL_BLK_GETGEOME)
    {
        struct rt_device_blk_geometry *geometry;

        geometry = (struct rt_device_blk_geometry *) args;
        if (geometry == RT_NULL)
        {
            return -RT_ERROR;
        }

        memcpy(geometry, &part->geometry, sizeof(struct rt_device_blk_geometry));
    }
    else if (cmd == RT_DEVICE_CTRL_BLK_ERASE)
    {
        rt_uint32_t *addrs = (rt_uint32_t *) args, start_addr = addrs[0], end_addr = addrs[1], phy_start_addr;
        rt_size_t phy_size;

        /* BUGS, if addrs is RT_NULL, system will crash, addrs[0] can't be accessed! */
        if (addrs == RT_NULL || start_addr > end_addr)
        {
            return -RT_ERROR;
        }

        if (end_addr == start_addr)
        {
            end_addr++;
        }

        phy_start_addr = start_addr * part->geometry.bytes_per_sector;
        phy_size = (end_addr - start_addr) * part->geometry.bytes_per_sector;

        if (fal_partition_erase(part->fal_part, phy_start_addr, phy_size) < 0)
        {
            return -RT_ERROR;
        }
    }

    return RT_EOK;
}

static rt_size_t blk_dev_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    int ret = 0;
    struct fal_blk_device *part = (struct fal_blk_device *)dev;

    assert(part != RT_NULL);

    ret = fal_partition_read(part->fal_part, pos * part->geometry.block_size, buffer, size * part->geometry.block_size);

    if (ret != (int)(size * part->geometry.block_size))
    {
        ret = 0;
    }
    else
    {
        ret = size;
    }

    return ret;
}

static rt_size_t blk_dev_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    int ret = 0;
    struct fal_blk_device *part;
    rt_off_t phy_pos;
    rt_size_t phy_size;

    part = (struct fal_blk_device *) dev;
    assert(part != RT_NULL);

    /* change the block device's logic address to physical address */
    phy_pos = pos * part->geometry.bytes_per_sector;
    phy_size = size * part->geometry.bytes_per_sector;

    ret = fal_partition_erase(part->fal_part, phy_pos, phy_size);

    if (ret == (int) phy_size)
    {
        ret = fal_partition_write(part->fal_part, phy_pos, buffer, phy_size);
    }

    if (ret != (int) phy_size)
    {
        ret = 0;
    }
    else
    {
        ret = size;
    }

    return ret;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops blk_dev_ops = {
    .read = blk_dev_read,
    .write = blk_dev_write,
    .control = blk_dev_control
};
#endif

/**
 * create RT-Thread block device by specified partition
 *
 * @param parition_name partition name
 *
 * @return != NULL: created block device
 *            NULL: created failed
 */
struct rt_device *fal_blk_device_create(const char *parition_name)
{
    struct fal_blk_device *blk_dev;
    const struct fal_partition *fal_part = fal_partition_find(parition_name);
    const struct fal_flash_dev *fal_flash = NULL;

    if (!fal_part)
    {
        log_e("Error: the partition name(%s) is not found.", parition_name);
        return NULL;
    }
    fal_flash = fal_flash_device_find(fal_part->flash_name);
    if (fal_flash == NULL)
    {
        log_e("Error: the flash device name(%s) is not found.", fal_part->flash_name);
        return NULL;
    }

    blk_dev = (struct fal_blk_device *) rt_malloc(sizeof(struct fal_blk_device));
    if (blk_dev)
    {
        blk_dev->fal_part = fal_part;
        blk_dev->geometry.bytes_per_sector = fal_flash->blk_size;
        blk_dev->geometry.block_size = fal_flash->blk_size;
        blk_dev->geometry.sector_count = fal_part->len / fal_flash->blk_size;

        /* register device */
        blk_dev->parent.type = RT_Device_Class_Block;

#ifdef RT_USING_DEVICE_OPS
        blk_dev->parent.ops  = &blk_dev_ops;
#else
        blk_dev->parent.init = NULL;
        blk_dev->parent.open = NULL;
        blk_dev->parent.close = NULL;
        blk_dev->parent.read = blk_dev_read;
        blk_dev->parent.write = blk_dev_write;
        blk_dev->parent.control = blk_dev_control;
#endif

        /* no private */
        blk_dev->parent.user_data = RT_NULL;

        log_i("The FAL block device(%s) created successfully", fal_part->name);
        rt_device_register(RT_DEVICE(blk_dev), fal_part->name, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_STANDALONE);
#ifdef RT_USING_POSIX
        blk_dev->parent.fops = &_blk_fops;
#endif

    }
    else
    {
        log_e("Error: no memory for create FAL block device");
    }

    return RT_DEVICE(blk_dev);
}

/* ========================== MTD nor device ======================== */
#if defined(RT_USING_MTD_NOR)

static rt_size_t mtd_nor_dev_read(struct rt_mtd_nor_device *device,
                                  rt_off_t offset, rt_uint8_t *data, rt_uint32_t length)
{
    int ret = 0;
    struct fal_partition *fal_part = (struct fal_partition *)device->user_data;

    assert(fal_part != RT_NULL);
    ret = fal_partition_read(fal_part, offset, data, length);

    if (ret != (int)length)
    {
        ret = 0;
    }
    else
    {
        ret = length;
    }

    return ret;
}

static rt_size_t mtd_nor_dev_write(struct rt_mtd_nor_device *device,
                                   rt_off_t offset, const rt_uint8_t *data, rt_uint32_t length)
{
    int ret = 0;
    struct fal_partition *fal_part = (struct fal_partition *)device->user_data;

    assert(fal_part != RT_NULL);

    ret = fal_partition_write(fal_part, offset, data, length);

    if (ret != (int) length)
    {
        ret = 0;
    }
    else
    {
        ret = length;
    }

    return ret;
}

static rt_err_t mtd_nor_dev_erase(struct rt_mtd_nor_device *device, rt_off_t offset, rt_uint32_t length)
{
    int ret = 0;
    struct fal_partition *fal_part = (struct fal_partition *)device->user_data;

    assert(fal_part != RT_NULL);

    ret = fal_partition_erase(fal_part, offset, length);

    if (ret != length)
    {
        return -RT_ERROR;
    }
    else
    {
        return RT_EOK;
    }
}

static const struct rt_mtd_nor_driver_ops _ops = {
    RT_NULL,
    mtd_nor_dev_read,
    mtd_nor_dev_write,
    mtd_nor_dev_erase,
};

/**
 * create RT-Thread MTD NOR device by specified partition
 *
 * @param parition_name partition name
 *
 * @return != NULL: created MTD NOR device
 *            NULL: created failed
 */
struct rt_device *fal_mtd_nor_device_create(const char *parition_name)
{
    struct rt_mtd_nor_device *mtd_nor_dev;
    struct fal_partition *fal_part = fal_partition_find(parition_name);
    int partition_id = fal_partition_no_find(parition_name);
    struct fal_flash_dev *fal_flash = NULL;
    char mtd_nor_dev_name[RT_NAME_MAX] = {0};

    if (!fal_part)
    {
        log_e("Error: the partition name(%s) is not found.", parition_name);
        return NULL;
    }
    fal_flash = (struct fal_flash_dev *)fal_part->user_data;
    if (fal_flash == NULL)
    {
        log_e("Error: the flash device name(%s) is not found.", fal_part->flash_name);
        return NULL;
    }

    mtd_nor_dev = (struct rt_mtd_nor_device *) rt_malloc(sizeof(struct rt_mtd_nor_device));
    if (mtd_nor_dev)
    {
        rt_memset(mtd_nor_dev, 0, sizeof(struct rt_mtd_nor_device));
        mtd_nor_dev->block_start = 0;
        mtd_nor_dev->block_end = fal_part->len / fal_flash->blk_size;
        mtd_nor_dev->block_size = fal_flash->blk_size;

        /* set ops */
        mtd_nor_dev->ops = &_ops;

        /* set user data */
        mtd_nor_dev->user_data = fal_part;

#if FAL_DEBUG
        log_i("The FAL MTD NOR device(%s) created successfully", fal_part->name);
#endif
        rt_snprintf(mtd_nor_dev_name, RT_NAME_MAX, "%s%d", "mtd", partition_id);
        rt_mtd_nor_register_device(mtd_nor_dev_name, mtd_nor_dev);
    }
    else
    {
        log_e("Error: no memory for create FAL MTD NOR device");
        return RT_NULL;
    }

    return RT_DEVICE(&mtd_nor_dev->parent);
}

#endif /* defined(RT_USING_MTD_NOR) */
#if defined(RT_USING_MTD_NAND) && defined(FH_USING_NAND_FLASH)
#include <spi-nand.h>
static rt_err_t mtd_nand_dev_erase(struct rt_mtd_nand_device* device, struct erase_info* einfo)
{
    int ret = 0;
    struct fal_partition *fal_part = (struct fal_partition *)device->user_data;
	const struct fal_flash_dev *flash_dev = (struct fal_flash_dev *)fal_part->user_data;
	struct mtd_info *mtd = flash_dev->user_data;

	einfo->addr = einfo->addr + fal_part->offset;
	ret = mtd->_erase(mtd, einfo);
	return ret;
}

static rt_err_t mtd_nand_dev_read(struct rt_mtd_nand_device* device, loff_t from,
			     struct mtd_oob_ops *ops)
{
    struct fal_partition *fal_part = (struct fal_partition *)device->user_data;
	const struct fal_flash_dev *flash_dev = (struct fal_flash_dev *)fal_part->user_data;
	struct mtd_info *mtd = flash_dev->user_data;
	int ret_code;
	loff_t addr;

	ops->retlen = ops->oobretlen = 0;
	if (!mtd->_read_oob)
		return -EOPNOTSUPP;
    addr = from + fal_part->offset;
    /*rt_kprintf("mtd_nand_dev_read:%x,%x,%x\n",addr, ops->datbuf, ops->oobbuf);*/
    ret_code = mtd->_read_oob(mtd, addr, ops);
	if (ret_code < 0)
		return ret_code;
	return ret_code >= mtd->bitflip_threshold ? -EUCLEAN : 0;
}

static rt_err_t mtd_nand_dev_write(struct rt_mtd_nand_device* device, loff_t to,
			      struct mtd_oob_ops *ops)
{
    struct fal_partition *fal_part = (struct fal_partition *)device->user_data;
	const struct fal_flash_dev *flash_dev = (struct fal_flash_dev *)fal_part->user_data;
	struct mtd_info *mtd = flash_dev->user_data;

	loff_t addr;

	ops->retlen = ops->oobretlen = 0;
	addr = to + fal_part->offset;
	if (!mtd->_write_oob)
		return -EOPNOTSUPP;
	/*rt_kprintf("mtd_nand_dev_write:%x,%x,%x\n",addr, ops->datbuf, ops->oobbuf);*/
	return mtd->_write_oob(mtd, addr, ops);
}

static rt_err_t mtd_nand_dev_checkblock(struct rt_mtd_nand_device* device,loff_t ofs)
{
    struct fal_partition *fal_part = (struct fal_partition *)device->user_data;
	const struct fal_flash_dev *flash_dev = (struct fal_flash_dev *)fal_part->user_data;
	struct mtd_info *mtd = flash_dev->user_data;
	loff_t addr;

    addr = ofs + fal_part->offset;
    if (ofs < 0 || ofs >= mtd->size)
		return -EINVAL;
	if (!mtd->_block_isbad)
		return 0;
	return mtd->_block_isbad(mtd, addr);
}

static rt_err_t mtd_nand_dev_markbadblock(struct rt_mtd_nand_device* device, loff_t ofs)
{
    struct fal_partition *fal_part = (struct fal_partition *)device->user_data;
	const struct fal_flash_dev *flash_dev = (struct fal_flash_dev *)fal_part->user_data;
	struct mtd_info *mtd = flash_dev->user_data;
	loff_t addr;

    addr = ofs + fal_part->offset;
	if (!mtd->_block_markbad)
		return -EOPNOTSUPP;
	if (ofs < 0 || ofs >= mtd->size)
		return -EINVAL;
	return mtd->_block_markbad(mtd, addr);
}


static const struct rt_mtd_nand_driver_ops nand_ops = {
    RT_NULL,
    mtd_nand_dev_read,
    mtd_nand_dev_write,
    RT_NULL,
    mtd_nand_dev_erase,
    mtd_nand_dev_checkblock,
    mtd_nand_dev_markbadblock,
};

struct rt_device *fal_mtd_nand_device_create(const char *parition_name)
{
    struct rt_mtd_nand_device *mtd_nand_dev;
    struct fal_partition *fal_part = fal_partition_find(parition_name);
    int partition_id = fal_partition_no_find(parition_name);
    struct fal_flash_dev *fal_flash = NULL;
	rt_mtd_t * mtd_info = RT_NULL;
    char mtd_nand_dev_name[RT_NAME_MAX] = {0};

    if (!fal_part)
    {
        log_e("Error: the partition name(%s) is not found.\n", parition_name);
        return NULL;
    }
    fal_flash = (struct fal_flash_dev *)fal_part->user_data;
    if (fal_flash == NULL)
    {
        log_e("Error: the flash device name(%s) is not found.\n", fal_part->flash_name);
        return NULL;
    }
    mtd_info = fal_flash->user_data;
	if (mtd_info == NULL)
    {
        log_e("Error: the nand flash device name is not found.");
        return NULL;
    }
    mtd_nand_dev = (struct rt_mtd_nand_device *) rt_malloc(sizeof(struct rt_mtd_nand_device));
    if (mtd_nand_dev)
    {
        rt_memset(mtd_nand_dev, 0, sizeof(struct rt_mtd_nand_device));
        mtd_nand_dev->block_start = 0;
        mtd_nand_dev->block_end = fal_part->len / fal_flash->blk_size;
        mtd_nand_dev->block_size = fal_flash->blk_size;
		mtd_nand_dev->page_size = mtd_info->sector_size;
		mtd_nand_dev->block_total = mtd_nand_dev->block_end;
		mtd_nand_dev->pages_per_block = mtd_nand_dev->block_size/mtd_nand_dev->page_size;

        /* set ops */
         /* set ops */
        mtd_nand_dev->ops = &nand_ops;
        /* set user data */
        mtd_nand_dev->user_data = fal_part;

#if FAL_DEBUG
        log_i("The FAL MTD nand device(%s) created successfully", fal_part->name);
#endif
        rt_snprintf(mtd_nand_dev_name, RT_NAME_MAX, "%s%d", "mtd", partition_id);
        rt_mtd_nand_register_device(mtd_nand_dev_name, mtd_nand_dev);
    }
    else
    {
        log_e("Error: no memory for create FAL MTD nand device");
        return RT_NULL;
    }

    return RT_DEVICE(&mtd_nand_dev->parent);
}
#endif
/* ========================== char device ======================== */
struct fal_char_device
{
    struct rt_device                parent;
    const struct fal_partition     *fal_part;
};

/* RT-Thread device interface */
static rt_size_t char_dev_read(rt_device_t dev, rt_off_t pos, void *buffer, rt_size_t size)
{
    int ret = 0;
    struct fal_char_device *part = (struct fal_char_device *) dev;

    assert(part != RT_NULL);

    if (pos + size > part->fal_part->len)
        size = part->fal_part->len - pos;

    ret = fal_partition_read(part->fal_part, pos, buffer, size);

    if (ret != (int)(size))
        ret = 0;

    return ret;
}

static rt_size_t char_dev_write(rt_device_t dev, rt_off_t pos, const void *buffer, rt_size_t size)
{
    int ret = 0;
    struct fal_char_device *part;

    part = (struct fal_char_device *) dev;
    assert(part != RT_NULL);

    if (pos == 0)
    {
        fal_partition_erase_all(part->fal_part);
    }
    else if (pos + size > part->fal_part->len)
    {
        size = part->fal_part->len - pos;
    }

    ret = fal_partition_write(part->fal_part, pos, buffer, size);

    if (ret != (int) size)
        ret = 0;

    return ret;
}

#ifdef RT_USING_DEVICE_OPS
const static struct rt_device_ops char_dev_ops = {
    .read = char_dev_read,
    .write = char_dev_write,
};
#endif

#ifdef RT_USING_POSIX
#include <dfs_posix.h>

/* RT-Thread device filesystem interface */
static int char_dev_fopen(struct dfs_fd *fd)
{
    struct fal_char_device *part = (struct fal_char_device *) fd->data;

    assert(part != RT_NULL);

    switch (fd->flags & O_ACCMODE)
    {
    case O_RDONLY:
        break;
    case O_WRONLY:
    case O_RDWR:
        /* erase partition when device file open */
        fal_partition_erase_all(part->fal_part);
        break;
    default:
        break;
    }
    fd->pos = 0;

    return RT_EOK;
}

static int char_dev_fread(struct dfs_fd *fd, void *buf, size_t count)
{
    int ret = 0;
    struct fal_char_device *part = (struct fal_char_device *) fd->data;

    assert(part != RT_NULL);

    if (fd->pos + count > part->fal_part->len)
        count = part->fal_part->len - fd->pos;

    ret = fal_partition_read(part->fal_part, fd->pos, buf, count);

    if (ret != (int)(count))
        return 0;

    fd->pos += ret;

    return ret;
}

static int char_dev_fwrite(struct dfs_fd *fd, const void *buf, size_t count)
{
    int ret = 0;
    struct fal_char_device *part = (struct fal_char_device *) fd->data;

    assert(part != RT_NULL);

    if (fd->pos + count > part->fal_part->len)
        count = part->fal_part->len - fd->pos;

    ret = fal_partition_write(part->fal_part, fd->pos, buf, count);

    if (ret != (int) count)
        return 0;

    fd->pos += ret;

    return ret;
}

const static struct dfs_file_ops char_dev_fops = {
    .open = char_dev_fopen,
    .read = char_dev_fread,
    .write = char_dev_fwrite,
};
#endif /* defined(RT_USING_POSIX) */

/**
 * create RT-Thread char device by specified partition
 *
 * @param parition_name partition name
 *
 * @return != NULL: created char device
 *            NULL: created failed
 */
struct rt_device *fal_char_device_create(const char *parition_name)
{
    struct fal_char_device *char_dev;
    const struct fal_partition *fal_part = fal_partition_find(parition_name);

    if (!fal_part)
    {
        log_e("Error: the partition name(%s) is not found.", parition_name);
        return NULL;
    }

    if ((fal_flash_device_find(fal_part->flash_name)) == NULL)
    {
        log_e("Error: the flash device name(%s) is not found.", fal_part->flash_name);
        return NULL;
    }

    char_dev = (struct fal_char_device *) rt_malloc(sizeof(struct fal_char_device));
    if (char_dev)
    {
        char_dev->fal_part = fal_part;

        /* register device */
        char_dev->parent.type = RT_Device_Class_Char;

#ifdef RT_USING_DEVICE_OPS
        char_dev->parent.ops  = &char_dev_ops;
#else
        char_dev->parent.init = NULL;
        char_dev->parent.open = NULL;
        char_dev->parent.close = NULL;
        char_dev->parent.read = char_dev_read;
        char_dev->parent.write = char_dev_write;
        char_dev->parent.control = NULL;
        /* no private */
        char_dev->parent.user_data = NULL;
#endif

        rt_device_register(RT_DEVICE(char_dev), fal_part->name, RT_DEVICE_FLAG_RDWR);
        log_i("The FAL char device(%s) created successfully", fal_part->name);

#ifdef RT_USING_POSIX
        /* set fops */
        char_dev->parent.fops = &char_dev_fops;
#endif

    }
    else
    {
        log_e("Error: no memory for create FAL char device");
    }

    return RT_DEVICE(char_dev);
}

#if defined(RT_USING_FINSH) && defined(FINSH_USING_MSH)

#include <finsh.h>
extern int fal_init_check(void);

static void fal(uint8_t argc, char **argv)
{

#define __is_print(ch)                ((unsigned int)((ch) - ' ') < 127u - ' ')
#define HEXDUMP_WIDTH                 16
#define CMD_PROBE_INDEX               0
#define CMD_READ_INDEX                1
#define CMD_WRITE_INDEX               2
#define CMD_ERASE_INDEX               3
#define CMD_BENCH_INDEX               4

    int result = -1;
    static const struct fal_flash_dev *flash_dev = NULL;
    static const struct fal_partition *part_dev = NULL;
    rt_spi_flash_device_t rt_p_flash_dev = NULL;
    sfud_flash *p_sfud_flash = NULL;
    size_t i = 0, j = 0;

    static char *help_info[] = {
            [CMD_PROBE_INDEX]     = "fal probe [dev_name|part_name]   - probe flash device or partition by given name",
            [CMD_READ_INDEX]      = "fal read addr size               - read 'size' bytes starting at 'addr'",
            [CMD_WRITE_INDEX]     = "fal write addr data1 ... dataN   - write some bytes 'data' starting at 'addr'",
            [CMD_ERASE_INDEX]     = "fal erase addr size              - erase 'size' bytes starting at 'addr'",
            [CMD_BENCH_INDEX]     = "fal bench <blk_size>             - benchmark test with per block size",
    };

    if (fal_init_check() != 1)
    {
        rt_kprintf("\n[Warning] FAL is not initialized or failed to initialize!\n\n");
        return;
    }

    if (argc < 2)
    {
        rt_kprintf("Usage:\n");
        for (i = 0; i < sizeof(help_info) / sizeof(char *); i++)
        {
            rt_kprintf("%s\n", help_info[i]);
        }
        rt_kprintf("\n");
    }
    else
    {
        const char *operator = argv[1];
        uint32_t addr, size;

        if (!strcmp(operator, "probe"))
        {
            if (argc >= 3)
            {
                char *dev_name = argv[2];

                flash_dev = fal_flash_device_find(dev_name);
                part_dev = fal_partition_find(dev_name);
                if (flash_dev != NULL)
                {
                    rt_p_flash_dev = (rt_spi_flash_device_t)flash_dev->user_data;
                    p_sfud_flash = (sfud_flash *)rt_p_flash_dev->user_data;
                    part_dev = NULL;
                }
                else if (part_dev != NULL)
                {
                    flash_dev = NULL;
                }
                else
                {
                    rt_kprintf("Device %s NOT found. Probe failed.\n", dev_name);
                    flash_dev = NULL;
                    part_dev = NULL;
                }
            }

            if (flash_dev)
            {
                rt_kprintf("Probed a flash device | %s | addr: %ld | len: %d |.\n", flash_dev->name,
                        flash_dev->addr, flash_dev->len);
            }
            else if (part_dev)
            {
                rt_kprintf("Probed a flash partition | %s | flash_dev: %s | offset: %ld | len: %d |.\n",
                        part_dev->name, part_dev->flash_name, part_dev->offset, part_dev->len);
            }
            else
            {
                rt_kprintf("No flash device or partition was probed.\n");
                rt_kprintf("Usage: %s.\n", help_info[CMD_PROBE_INDEX]);
                fal_show_part_table();
            }
        }
        else
        {
            if (!flash_dev && !part_dev)
            {
                rt_kprintf("No flash device or partition was probed. Please run 'fal probe'.\n");
                return;
            }
            if (!rt_strcmp(operator, "read"))
            {
                if (argc < 4)
                {
                    rt_kprintf("Usage: %s.\n", help_info[CMD_READ_INDEX]);
                    return;
                }
                else
                {
                    addr = strtol(argv[2], NULL, 0);
                    size = strtol(argv[3], NULL, 0);
                    uint8_t *data = rt_malloc(size);

                    if (data)
                    {
                        if (flash_dev)
                        {
                            result = flash_dev->ops.read(p_sfud_flash, addr, data, size);
                        }
                        else if (part_dev)
                        {
                            result = fal_partition_read(part_dev, addr, data, size);
                        }
                        if (result >= 0)
                        {
                            rt_kprintf("Read data success. Start from 0x%08X, size is %ld. The data is:\n", addr,
                                    size);
                            rt_kprintf("Offset h 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n");
                            for (i = 0; i < size; i += HEXDUMP_WIDTH)
                            {
                                rt_kprintf("[%08X] ", addr + i);
                                /* dump hex */
                                for (j = 0; j < HEXDUMP_WIDTH; j++)
                                {
                                    if (i + j < size)
                                    {
                                        rt_kprintf("%02X ", data[i + j]);
                                    }
                                    else
                                    {
                                        rt_kprintf("   ");
                                    }
                                }
                                /* dump char for hex */
                                for (j = 0; j < HEXDUMP_WIDTH; j++)
                                {
                                    if (i + j < size)
                                    {
                                        rt_kprintf("%c", __is_print(data[i + j]) ? data[i + j] : '.');
                                    }
                                }
                                rt_kprintf("\n");
                            }
                            rt_kprintf("\n");
                        }
                        rt_free(data);
                    }
                    else
                    {
                        rt_kprintf("Low memory!\n");
                    }
                }
            }
            else if (!strcmp(operator, "write"))
            {
                if (argc < 4)
                {
                    rt_kprintf("Usage: %s.\n", help_info[CMD_WRITE_INDEX]);
                    return;
                }
                else
                {
                    addr = strtol(argv[2], NULL, 0);
                    size = argc - 3;
                    uint8_t *data = rt_malloc(size);

                    if (data)
                    {
                        for (i = 0; i < size; i++)
                        {
                            data[i] = strtol(argv[3 + i], NULL, 0);
                        }
                        if (flash_dev)
                        {
                            result = flash_dev->ops.write(p_sfud_flash, addr, data, size);
                        }
                        else if (part_dev)
                        {
                            result = fal_partition_write(part_dev, addr, data, size);
                        }
                        if (result >= 0)
                        {
                            rt_kprintf("Write data success. Start from 0x%08X, size is %ld.\n", addr, size);
                            rt_kprintf("Write data: ");
                            for (i = 0; i < size; i++)
                            {
                                rt_kprintf("%d ", data[i]);
                            }
                            rt_kprintf(".\n");
                        }
                        rt_free(data);
                    }
                    else
                    {
                        rt_kprintf("Low memory!\n");
                    }
                }
            }
            else if (!rt_strcmp(operator, "erase"))
            {
                if (argc < 4)
                {
                    rt_kprintf("Usage: %s.\n", help_info[CMD_ERASE_INDEX]);
                    return;
                }
                else
                {
                    addr = strtol(argv[2], NULL, 0);
                    size = strtol(argv[3], NULL, 0);
                    if (flash_dev)
                    {
                        result = flash_dev->ops.erase(p_sfud_flash, addr, size);
                    }
                    else if (part_dev)
                    {
                        result = fal_partition_erase(part_dev, addr, size);
                    }
                    if (result >= 0)
                    {
                        rt_kprintf("Erase data success. Start from 0x%08X, size is %ld.\n", addr, size);
                    }
                }
            }
            else if (!strcmp(operator, "bench"))
            {
                if (argc < 3)
                {
                    rt_kprintf("Usage: %s.\n", help_info[CMD_BENCH_INDEX]);
                    return;
                }
                else if ((argc > 3 && strcmp(argv[3], "yes")) || argc < 4)
                {
                    rt_kprintf("DANGER: It will erase full chip or partition! Please run 'fal bench %d yes'.\n",
                               strtol(argv[2], NULL, 0));
                    return;
                }
                /* full chip benchmark test */
                uint32_t start_time, time_cast;
                size_t write_size = strtol(argv[2], NULL, 0), read_size = strtol(argv[2], NULL, 0), cur_read_size;
                uint8_t *write_data = (uint8_t *)rt_malloc(write_size), *read_data = (uint8_t *)rt_malloc(read_size);

                if (write_data && read_data)
                {
                    memset(write_data, 0x55, write_size);
                    if (flash_dev)
                    {
                        size = flash_dev->len;
                    }
                    else if (part_dev)
                    {
                        size = part_dev->len;
                    }
                    /* benchmark testing */
                    rt_kprintf("Erasing %ld bytes data, waiting...\n", size);
                    start_time = rt_tick_get();
                    if (flash_dev)
                    {
                        result = flash_dev->ops.erase(p_sfud_flash, 0, size);
                    }
                    else if (part_dev)
                    {
                        result = fal_partition_erase(part_dev, 0, size);
                    }
                    if (result >= 0)
                    {
                        time_cast = rt_tick_get() - start_time;
                        rt_kprintf("Erase benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                                time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
                    }
                    else
                    {
                        rt_kprintf("Erase benchmark has an error. Error code: %d.\n", result);
                    }
                    /* write test */
                    rt_kprintf("Writing %ld bytes data, waiting...\n", size);
                    start_time = rt_tick_get();
                    for (i = 0; i < size; i += write_size)
                    {
                        if (flash_dev)
                        {
                            result = flash_dev->ops.write(p_sfud_flash, i, write_data, write_size);
                        }
                        else if (part_dev)
                        {
                            result = fal_partition_write(part_dev, i, write_data, write_size);
                        }
                        if (result < 0)
                        {
                            break;
                        }
                    }
                    if (result >= 0)
                    {
                        time_cast = rt_tick_get() - start_time;
                        rt_kprintf("Write benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                                time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
                    }
                    else
                    {
                        rt_kprintf("Write benchmark has an error. Error code: %d.\n", result);
                    }
                    /* read test */
                    rt_kprintf("Reading %ld bytes data, waiting...\n", size);
                    start_time = rt_tick_get();
                    for (i = 0; i < size; i += read_size)
                    {
                        if (i + read_size <= size)
                        {
                            cur_read_size = read_size;
                        }
                        else
                        {
                            cur_read_size = size - i;
                        }
                        if (flash_dev)
                        {
                            result = flash_dev->ops.read(p_sfud_flash, i, read_data, cur_read_size);
                        }
                        else if (part_dev)
                        {
                            result = fal_partition_read(part_dev, i, read_data, cur_read_size);
                        }
                        /* data check */
                        if (memcmp(write_data, read_data, cur_read_size))
                        {
                            result = -RT_ERROR;
                            rt_kprintf("Data check ERROR! Please check you flash by other command.\n");
                        }
                        /* has an error */
                        if (result < 0)
                        {
                            break;
                        }
                    }
                    if (result >= 0)
                    {
                        time_cast = rt_tick_get() - start_time;
                        rt_kprintf("Read benchmark success, total time: %d.%03dS.\n", time_cast / RT_TICK_PER_SECOND,
                                time_cast % RT_TICK_PER_SECOND / ((RT_TICK_PER_SECOND * 1 + 999) / 1000));
                    }
                    else
                    {
                        rt_kprintf("Read benchmark has an error. Error code: %d.\n", result);
                    }
                }
                else
                {
                    rt_kprintf("Low memory!\n");
                }
                rt_free(write_data);
                rt_free(read_data);
            }
            else
            {
                rt_kprintf("Usage:\n");
                for (i = 0; i < sizeof(help_info) / sizeof(char *); i++)
                {
                    rt_kprintf("%s\n", help_info[i]);
                }
                rt_kprintf("\n");
                return;
            }
            if (result < 0)
            {
                rt_kprintf("This operate has an error. Error code: %d.\n", result);
            }
        }
    }
}
MSH_CMD_EXPORT(fal, FAL(Flash Abstraction Layer) operate.);

#endif /* defined(RT_USING_FINSH) && defined(FINSH_USING_MSH) */
