#include <fal.h>

static uint8_t init_ok = 0;

/**
 * FAL (Flash Abstraction Layer) initialization.
 * It will initialize all flash device and all flash partition.
 *
 * @return >= 0: partitions total number
 */
int fal_init(struct fal_flash_dev *p_flash_dev, int flash_dev_size, struct fal_partition *p_part_dev, int part_dev_size)
{
    extern int fal_flash_init(struct fal_flash_dev *p_flash_dev, int flash_dev_size);
    extern int fal_partition_init(struct fal_partition *p_part_dev, int part_dev_size);

    int result;

    /* initialize all flash device on FAL flash table */
    result = fal_flash_init(p_flash_dev, flash_dev_size);

    if (result < 0)
    {
        goto __exit;
    }

    /* initialize all flash partition on FAL partition table */
    result = fal_partition_init(p_part_dev, part_dev_size);

__exit:

    if ((result > 0) && (!init_ok))
    {
        init_ok = 1;
#ifndef FH_FAST_BOOT
        log_i("RT-Thread Flash Abstraction Layer(V%s) initialize success.", FAL_SW_VERSION);
#endif
    }
    else if (result <= 0)
    {
        init_ok = 0;
        log_e("RT-Thread Flash Abstraction Layer(V%s) initialize failed.", FAL_SW_VERSION);
    }

    return result;
}

/**
 * Check if the FAL is initialized successfully
 *
 * @return 0: not init or init failed; 1: init success
 */
int fal_init_check(void)
{
    return init_ok;
}
