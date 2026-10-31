#include <spi-nand.h>

#define ONE_WIRE_SUPPORT		(1<<0)
#define DUAL_WIRE_SUPPORT		(1<<1)
#define QUAD_WIRE_SUPPORT		(1<<2)
#define MULTI_WIRE_SUPPORT		(1<<8)

#ifndef RT_NAND_DEFAULT_SPI_CFG
/* read the JEDEC SFDP command must run at 50 MHz or less */
#define RT_NAND_DEFAULT_SPI_CFG                  \
{                                                \
    .mode = RT_SPI_MODE_0 | RT_SPI_MSB,          \
    .data_width = 8,                             \
    .max_hz = 50 * 1000 * 1000,                  \
	.sample_delay = 1                            \
}
#endif

#ifdef RT_SFUD_USING_QSPI
#define RT_NAND_DEFAULT_QSPI_CFG                 \
{                                                \
    RT_NAND_DEFAULT_SPI_CFG,                     \
    .ddr_mode = 0,                               \
    .qspi_dl_width = 4,                          \
}
#endif

extern nand_err nand_spi_port_init(struct spi_nand_chip *chip);

static nand_err nand_hardware_init(struct spi_nand_chip *chip);
static nand_err nand_software_init(struct spi_nand_chip *chip);

static char log_buf[RT_CONSOLEBUF_SIZE];

void nand_log_info(const char *format, ...)
{
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    rt_kprintf("[NAND] ");
    /* must use vprintf to print */
    rt_vsnprintf(log_buf, sizeof(log_buf), format, args);
    rt_kprintf("%s\n", log_buf);
    va_end(args);
}

void nand_log_debug(const char *file, const long line, const char *format, ...)
{
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    rt_kprintf("[NAND] (%s:%ld) ", file, line);
    /* must use vprintf to print */
    rt_vsnprintf(log_buf, sizeof(log_buf), format, args);
    rt_kprintf("%s\n", log_buf);
    va_end(args);
}


nand_err nand_device_init(struct spi_nand_chip *chip)
{
    nand_err result = NAND_SUCCESS;

    /* hardware initialize */
    result = nand_hardware_init(chip);
    if (result == NAND_SUCCESS)
        result = nand_software_init(chip);
    if (result == NAND_SUCCESS)
    {
        chip->init_ok = true;
        NAND_INFO("%s flash device is initialize success.", chip->name);
    }
    else
    {
        chip->init_ok = false;
        NAND_INFO("%s flash device is initialize fail.", chip->name);
    }

    return result;
}

static nand_err nand_hardware_init(struct spi_nand_chip *chip)
{
	nand_err result = NAND_SUCCESS;
#ifdef RT_SFUD_USING_QSPI
    /* set default read instruction */
    chip->read_cmd_format.instruction = SPINAND_CMD_READ_RDM;
#endif /* RT_SFUD_USING_QSPI */
	result = nand_read_id(chip);

	return result;
}

static nand_err nand_software_init(struct spi_nand_chip *chip)
{
	nand_err result = NAND_SUCCESS;

	RT_ASSERT(chip);

	return result;
}

static void spi_nand_ecc_status(struct spi_nand_chip *chip, unsigned int status,
				      unsigned int *corrected, unsigned int *ecc_error)
{
	unsigned int ecc_status = (status >> SPI_NAND_ECC_SHIFT) &
				  chip->ecc_mask;

	*ecc_error = (ecc_status == chip->ecc_uncorr);
	if (*ecc_error == 0)
		*corrected = ecc_status;
}

#ifdef RT_SFUD_USING_QSPI
static void qspi_set_read_cmd_format(struct spi_nand_chip *chip, uint8_t ins, uint8_t ins_lines, uint8_t addr_lines,
                                     uint8_t dummy_cycles, uint8_t data_lines)
{
	chip->read_cmd_format.instruction = ins;
	chip->read_cmd_format.address_size = 24;

    chip->read_cmd_format.instruction_lines = ins_lines;
    chip->read_cmd_format.address_lines = addr_lines;
    chip->read_cmd_format.alternate_bytes_lines = 0;
    chip->read_cmd_format.dummy_cycles = dummy_cycles;
    chip->read_cmd_format.data_lines = data_lines;
}

nand_err nand_qspi_fast_read_enable(struct spi_nand_chip *chip, uint8_t data_line_width)
{
    uint8_t read_mode = NORMAL_SPI_READ;
    sfud_err result = SFUD_SUCCESS;

    RT_ASSERT(chip);
    RT_ASSERT(data_line_width == 1 || data_line_width == 2 || data_line_width == 4);

	read_mode = NORMAL_SPI_READ|DUAL_OUTPUT|QUAD_OUTPUT;
    switch (data_line_width)
    {
    case 1:
        qspi_set_read_cmd_format(chip, SPINAND_CMD_READ_RDM, 1, 1, 0, 1);
        break;
    case 2:
        if (read_mode & DUAL_OUTPUT)
            qspi_set_read_cmd_format(chip, SPINAND_CMD_READ_CACHE_X2, 1, 1, 0, 2);
        else if (read_mode & DUAL_IO)
            qspi_set_read_cmd_format(chip, SPINAND_CMD_READ_CACHE_DUAL, 1, 2, 8, 2);
        else
            qspi_set_read_cmd_format(chip, SPINAND_CMD_READ_RDM, 1, 1, 0, 1);
        break;
    case 4:
        if (read_mode & QUAD_OUTPUT)
            qspi_set_read_cmd_format(chip, SPINAND_CMD_READ_CACHE_X4, 1, 1, 0, 4);
        else if (read_mode & QUAD_IO)
            qspi_set_read_cmd_format(chip, SPINAND_CMD_READ_CACHE_QUAD, 1, 4, 6, 4);
        else
            qspi_set_read_cmd_format(chip, SPINAND_CMD_READ_RDM, 1, 1, 0, 1);
        break;
    }

    return result;
}
#endif /* RT_SFUD_USING_QSPI */

rt_mtd_t * rt_nand_flash_probe(const char *spi_flash_dev_name, const char *spi_dev_name)
{
	struct spi_nand_chip *chip;
	struct rt_spi_configuration cfg = RT_NAND_DEFAULT_SPI_CFG;
    struct mtd_info *mtd = NULL;
    struct rt_spi_device * spi_device = NULL;
    int ret = 0;
#ifdef RT_SFUD_USING_QSPI
    struct rt_qspi_configuration qspi_cfg = RT_NAND_DEFAULT_QSPI_CFG;
    struct rt_qspi_device *qspi_dev = RT_NULL;
#endif

	chip = rt_malloc(sizeof(struct spi_nand_chip));
	rt_memset(chip,0,sizeof(struct spi_nand_chip));
	if (!chip) {
        ret = -ENOMEM;
		goto error;
	}

	mtd = rt_malloc(sizeof(struct mtd_info));
	rt_memset(mtd,0,sizeof(struct mtd_info));
	if (!mtd) {
		ret = -ENOMEM;
		goto error;
	}

	/* SPI configure */
	{
		/* RT-Thread SPI device initialize */
		spi_device = (struct rt_spi_device *) rt_device_find(spi_dev_name);
		if (spi_device == RT_NULL || spi_device->parent.type != RT_Device_Class_SPIDevice)
		{
			DEBUG_TRACE("ERROR: SPI device %s not found!\n", spi_dev_name);
		}
#ifdef RT_SFUD_USING_QSPI
	/* set the qspi line number and configure the QSPI bus */
	if (spi_device->bus->mode & RT_SPI_BUS_MODE_QSPI)
	{
		qspi_dev = (struct rt_qspi_device *)spi_device;
		qspi_cfg.qspi_dl_width = qspi_dev->config.qspi_dl_width;
		rt_qspi_configure(qspi_dev, &qspi_cfg);
	}
	else
#endif
		rt_spi_configure(spi_device, &cfg);
	}
	/* NAND flash device initialize */
	{
	if (rt_mutex_init(&chip->chip_lock, "nand_chip", RT_IPC_FLAG_FIFO) != RT_EOK)
        DEBUG_TRACE("init spi nand lock mutex failed\n");

    chip->user_data = spi_device;
    chip->get_ecc_status = spi_nand_ecc_status;

    ret = nand_spi_port_init(chip);
    if (nand_device_init(chip) != NAND_SUCCESS)
    {
        DEBUG_TRACE("ERROR: SPI nand flash probe failed by SPI device %s.\n", spi_dev_name);
        goto error;
    }
#ifdef RT_SFUD_USING_QSPI
		if (qspi_dev->enter_qspi_mode != RT_NULL)
			qspi_dev->enter_qspi_mode(qspi_dev);
		/* set data lines width */
		nand_qspi_fast_read_enable(chip, qspi_dev->config.qspi_dl_width);
#endif /* NAND_USING_QSPI */
	}
	NAND_INFO("Probe SPI flash %s by SPI device %s success.", spi_flash_dev_name, spi_dev_name);

	mtd->priv = chip;
	chip->mtd = mtd;
    spi_device->user_data = mtd;

	ret = spi_nand_scan_tail(mtd);
    if (ret != 0)
    {
        goto error;
    }

	return mtd;
error:
    rt_mutex_detach(&chip->chip_lock);
	if (mtd)
		rt_free(mtd);
	if (chip)
		rt_free(chip);
	return RT_NULL;
}
