#include <fal.h>
#include <string.h>

#if (0)
/* driver must depend on usr????*/
/* flash device table, must defined by user */
#if !defined(FAL_FLASH_DEV_TABLE)
#error "You must defined flash device table (FAL_FLASH_DEV_TABLE) on 'fal_cfg.h'"
#endif
#endif


/*static const struct fal_flash_dev * const device_table[] = FAL_FLASH_DEV_TABLE;*/
static  struct fal_flash_dev *device_table[FAL_FLASH_DEV_TABLE_SIZE] = {0};
static  size_t device_table_len;
/*static  size_t device_table_len = sizeof(device_table) / sizeof(device_table[0]);*/
static uint8_t init_ok = 0;

/**
 * Initialize all flash device on FAL flash table
 *
 * @return result
 */
int fal_flash_init(struct fal_flash_dev *p_flash_dev, int flash_dev_size)
{
    size_t i;

    if (flash_dev_size > FAL_FLASH_DEV_TABLE_SIZE)
    {
        log_d("flash dev get in 0x%x larger than max size 0x%x\n", flash_dev_size, FAL_FLASH_DEV_TABLE_SIZE);
        return -1;
    }

    if (init_ok)
    {
        return 0;
    }

    for (i = 0; i < flash_dev_size; i++)
    {
        assert(p_flash_dev->ops.read);
        assert(p_flash_dev->ops.write);
        assert(p_flash_dev->ops.erase);
        /* init flash device on flash table */
        if (p_flash_dev->ops.init)
        {
            p_flash_dev->ops.init();
        }
        /*cpy to self table*/
        device_table[i] = p_flash_dev;
    }

    device_table_len = flash_dev_size;

    for (i = 0; i < device_table_len; i++)
    {
        log_d("Flash device | %*.*s | addr: 0x%08lx | len: 0x%08x | blk_size: 0x%08x |initialized finish.",
                FAL_DEV_NAME_MAX, FAL_DEV_NAME_MAX, device_table[i]->name, device_table[i]->addr, device_table[i]->len,
                device_table[i]->blk_size);
    }

    init_ok = 1;
    return 0;
}

/**
 * find flash device by name
 *
 * @param name flash device name
 *
 * @return != NULL: flash device
 *            NULL: not found
 */
struct fal_flash_dev *fal_flash_device_find(const char *name)
{
    assert(init_ok);
    assert(name);

    size_t i;

    for (i = 0; i < device_table_len; i++)
    {
        if (!rt_strncmp(name, device_table[i]->name, FAL_DEV_NAME_MAX))
        {
            return device_table[i];
        }
    }

    return NULL;
}
