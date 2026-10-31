#ifndef __LINUX_MTD_SPI_NAND_H
#define __LINUX_MTD_SPI_NAND_H

#include <rtthread.h>
#include <rtdevice.h>
#include <rtdef.h>
#include <rtlibc.h>
#include <sfud_def.h>
#include <flashchip.h>
#include "drivers/mtd.h"
#include <mtd/mtd-user.h>

/*
 * Standard SPI-NAND flash commands
 */
#define SPINAND_CMD_READ			0x13
#define SPINAND_CMD_READ_RDM			0x03
#define SPINAND_CMD_PROG_LOAD			0x02
#define SPINAND_CMD_PROG_RDM			0x84
#define SPINAND_CMD_PROG			0x10
#define SPINAND_CMD_ERASE_BLK			0xd8
#define SPINAND_CMD_WR_ENABLE			0x06
#define SPINAND_CMD_WR_DISABLE			0x04
#define SPINAND_CMD_READ_ID			0x9f
#define SPINAND_CMD_RESET			0xff
#define SPINAND_CMD_READ_REG			0x0f
#define SPINAND_CMD_WRITE_REG			0x1f

#define SPINAND_CMD_READ_CACHE_X2		0x3b
#define SPINAND_CMD_READ_CACHE_X4		0x6b
#define SPINAND_CMD_READ_CACHE_DUAL		0xbb
#define SPINAND_CMD_READ_CACHE_QUAD		0xeb

#define SPINAND_CMD_PROG_LOAD_X4		0x32
#define SPINAND_CMD_PROG_RDM_X4			0xC4 /*or 34*/

/* feature registers */
#define REG_BLOCK_LOCK			0xa0
#define REG_OTP				0xb0
#define REG_STATUS			0xc0/* timing */

/* status */
#define STATUS_OIP_MASK			0x01
#define STATUS_READY			(0 << 0)
#define STATUS_BUSY			(1 << 0)

#define STATUS_E_FAIL_MASK		0x04
#define STATUS_E_FAIL			(1 << 2)

#define STATUS_WEL_MASK			0x02
#define STATUS_W_PASS			(1 << 1)

#define STATUS_P_FAIL_MASK		0x08
#define STATUS_P_FAIL			(1 << 3)

/*OTP register defines*/
#define OTP_ECC_MASK			0X10
#define OTP_ECC_ENABLE			(1 << 4)
#define OTP_ENABLE			(1 << 6)
#define OTP_LOCK			(1 << 7)
#define QE_ENABLE			(1 << 0)


/* block lock */
#define BL_ALL_LOCKED      0x38
#define BL_1_2_LOCKED      0x30
#define BL_1_4_LOCKED      0x28
#define BL_1_8_LOCKED      0x20
#define BL_1_16_LOCKED     0x18
#define BL_1_32_LOCKED     0x10
#define BL_1_64_LOCKED     0x08
#define BL_ALL_UNLOCKED    0

#define SPI_NAND_ECC_SHIFT		4

#define SPI_NAND_MT29F_ECC_MASK		3
#define SPI_NAND_MT29F_ECC_CORRECTED	1
#define SPI_NAND_MT29F_ECC_UNCORR	2
#define SPI_NAND_MT29F_ECC_RESERVED	3
#define SPI_NAND_MT29F_ECC_SHIFT	4

#define SPI_NAND_GD5F_ECC_MASK		7
#define SPI_NAND_GD5F_ECC_UNCORR	7
#define SPI_NAND_GD5F_ECC_SHIFT		4

#define	SPI_TX_DUAL	0x100			/* transmit with 2 wires */
#define	SPI_TX_QUAD	0x200			/* transmit with 4 wires */
#define	SPI_RX_DUAL	0x400			/* receive with 2 wires */
#define	SPI_RX_QUAD	0x800			/* receive with 4 wires */

#ifndef NAND_INFO
#define NAND_INFO(...)  nand_log_info(__VA_ARGS__)
#endif

// #define RT_NAND_ECC_DEBUG
// #define RT_NAND_DEBUG

/* assert for developer. */
#ifdef RT_NAND_DEBUG
#define DEBUG_TRACE         rt_kprintf("[NAND] "); rt_kprintf
#define NAND_DEBUG(...) nand_log_debug(__FILE__, __LINE__, __VA_ARGS__)
#define fh_debug_dump(buf,len)  do { \
		unsigned int i; \
		printk("\t %s:L%d", __func__,__LINE__); \
		for (i=0;i<len/4;i++) { \
			if (0 == i % 4 ) \
				printk("\n\t\t 0x%08x:\t",(unsigned int) buf+i*4 ); \
			printk("%08x ", *(unsigned int*) (buf + i*4));\
		} \
	} while(0)
#else
#define NAND_DEBUG(...)
#define DEBUG_TRACE(...)
#define fh_debug_dump(buf,len)
#endif /* NAND_DEBUG_MODE */

#ifdef RT_NAND_ECC_DEBUG
#define ECC_DEBUG_TRACE         rt_kprintf("[NAND] "); rt_kprintf
#else
#define ECC_DEBUG_TRACE(...)
#endif

#define D_ALIGN(x, a)	(((x) + (a) - 1) & ~((a) - 1))

#define SPINAND_MAX_ID_LEN		4

#define NAND_RETRY_PROCESS(delay, retry, result)                               \
    void (*__delay_temp)(void) = (void (*)(void))delay;                        \
    if (retry == 0)                                                            \
    {                                                                          \
        result = NAND_ERR_TIMEOUT;                                             \
        break;                                                                 \
    }                                                                          \
    else if (__delay_temp)                                                     \
        __delay_temp();                                                        \
    retry--;

#define SPINAND_NEED_PLANE_SELECT	(1 << 0)
#define FIRST_DUMMY_BYTE            (1 << 1)

#define SPINAND_MFR_MICRON		0x2C
#define SPINAND_MFR_GIGADEVICE	0xC8

enum {
    NAND_STATUS_READ = (1 << 0),
    NAND_STATUS_WRITE = (1 << 1),
    NAND_STATUS_ERASE = (1 << 2),
    NAND_STATUS_RESET = (1 << 3),
};

/**
 * status register bits
 */
enum {
    NAND_STATUS_REGISTER_BUSY = (1 << 0),                  /**< busing */
    NAND_STATUS_REGISTER_WEL = (1 << 1),                   /**< write enable latch */
    NAND_STATUS_REGISTER_SRP = (1 << 7),                   /**< status register protect */
};

enum nand_qspi_read_mode
{
    NORMAL_SPI_READ = 1 << 0,               /**< mormal spi read mode */
    DUAL_OUTPUT = 1 << 1,                   /**< qspi fast read dual output */
    DUAL_IO = 1 << 2,                       /**< qspi fast read dual input/output */
    QUAD_OUTPUT = 1 << 3,                   /**< qspi fast read quad output */
    QUAD_IO = 1 << 4,                       /**< qspi fast read quad input/output */
};

/**
 * struct spi_nand_chip - SPI-NAND Private Flash Chip Data
 * @chip_lock:		[INTERN] protection lock
 * @name:		name of the chip
 * @wq:			[INTERN] wait queue to sleep on if a SPI-NAND operation
 *			is in progress used instead of the per chip wait queue
 *			when a hw controller is available.
 * @mfr_id:		[BOARDSPECIFIC] manufacture id
 * @dev_id:		[BOARDSPECIFIC] device id
 * @state:		[INTERN] the current state of the SPI-NAND device
 * @spi:		[INTERN] point to spi device structure
 * @mtd:		[INTERN] point to MTD device structure
 * @reset:		[REPLACEABLE] function to reset the device
 * @read_id:		[REPLACEABLE] read manufacture id and device id
 * @load_page:		[REPLACEABLE] load page from NAND to cache
 * @read_cache:		[REPLACEABLE] read data from cache
 * @store_cache:	[REPLACEABLE] write data to cache
 * @write_page:		[REPLACEABLE] program NAND with cache data
 * @erase_block:	[REPLACEABLE] erase a given block
 * @waitfunc:		[REPLACEABLE] wait for ready.
 * @write_enable:	[REPLACEABLE] set write enable latch
 * @get_ecc_status:	[REPLACEABLE] get ecc and bitflip status
 * @enable_ecc:		[REPLACEABLE] enable on-die ecc
 * @disable_ecc:	[REPLACEABLE] disable on-die ecc
 * @buf:		[INTERN] buffer for read/write
 * @oobbuf:		[INTERN] buffer for read/write oob
 * @pagebuf:		[INTERN] holds the pagenumber which is currently in
 *			data_buf.
 * @pagebuf_bitflips:	[INTERN] holds the bitflip count for the page which is
 *			currently in data_buf.
 * @size:		[INTERN] the size of chip
 * @block_size:		[INTERN] the size of eraseblock
 * @page_size:		[INTERN] the size of page
 * @page_spare_size:	[INTERN] the size of page oob size
 * @block_shift:	[INTERN] number of address bits in a eraseblock
 * @page_shift:		[INTERN] number of address bits in a page (column
 *			address bits).
 * @pagemask:		[INTERN] page number mask = number of (pages / chip) - 1
 * @options:		[BOARDSPECIFIC] various chip options. They can partly
 *			be set to inform nand_scan about special functionality.
 * @ecc_strength_ds:	[INTERN] ECC correctability from the datasheet.
 *			Minimum amount of bit errors per @ecc_step_ds guaranteed
 *			to be correctable. If unknown, set to zero.
 * @ecc_step_ds:	[INTERN] ECC step required by the @ecc_strength_ds,
 *                      also from the datasheet. It is the recommended ECC step
 *			size, if known; if unknown, set to zero.
 * @ecc_mask:
 * @ecc_uncorr:
 * @bits_per_cell:	[INTERN] number of bits per cell. i.e., 1 means SLC.
 * @ecclayout:		[BOARDSPECIFIC] ECC layout control structure
 *			See the defines for further explanation.
 * @bbt_options:	[INTERN] bad block specific options. All options used
 *			here must come from bbm.h. By default, these options
 *			will be copied to the appropriate nand_bbt_descr's.
 * @bbt:		[INTERN] bad block table pointer
 * @badblockpos:	[INTERN] position of the bad block marker in the oob
 *			area.
 * @bbt_td:		[REPLACEABLE] bad block table descriptor for flash
 *			lookup.
 * @bbt_md:		[REPLACEABLE] bad block table mirror descriptor
 * @badblock_pattern:	[REPLACEABLE] bad block scan pattern used for initial
 *			bad block scan.
 * @onfi_params:	[INTERN] holds the ONFI page parameter when ONFI is
 *			supported, 0 otherwise.
 */

struct spi_nand_id_info{
#define SPI_NAND_ID_NO_DUMMY  (0xff)
	uint8_t id_addr;
	uint8_t id_len;
};

struct spi_nand_flash {
	char		*name;
	struct spi_nand_id_info id_info;
	uint8_t		dev_id[SPINAND_MAX_ID_LEN];
	uint32_t		page_size;
	uint32_t		page_spare_size;
	uint32_t		pages_per_blk;
	uint32_t		blks_per_chip;
	uint32_t		options;
	uint8_t		ecc_mask;
	uint8_t		ecc_uncorr;
	struct nand_ecclayout *ecc_layout;
	uint32_t		qe_addr;
	uint32_t		qe_flag;
	uint32_t		qe_mask;
    uint32_t        multi_wire_command_length;
    uint32_t        bbt_options;
    uint32_t        bitflip_threshold;
};

typedef struct {
    uint8_t instruction;
    uint8_t instruction_lines;
    uint8_t address_size;
    uint8_t address_lines;
    uint8_t alternate_bytes_lines;
    uint8_t dummy_cycles;
    uint8_t data_lines;
} nand_qspi_read_cmd_format;

typedef enum {
    NAND_SUCCESS = 0,                                      /**< success */
    NAND_ERR_NOT_FOUND = -1,                                /**< not found or not supported */
    NAND_ERR_WRITE = -2,                                    /**< write error */
    NAND_ERR_READ = -3,                                     /**< read error */
    NAND_ERR_TIMEOUT = -4,                                  /**< timeout error */
    NAND_ERR_ADDR_OUT_OF_BOUND = -5,                        /**< address is out of flash bound */
} nand_err;

typedef struct __nand_spi {
    /* SPI device name */
    char *name;
    /* SPI bus write read data function */
    nand_err (*wr)(const struct __nand_spi *spi, const uint8_t *write_buf, size_t write_size, uint8_t *read_buf,
                   size_t read_size);
#ifdef RT_SFUD_USING_QSPI
    /* QSPI fast read function */
    nand_err (*qspi_read)(const struct __nand_spi *spi, uint32_t addr, nand_qspi_read_cmd_format *qspi_read_cmd_format,
                          uint8_t *read_buf, size_t read_size);
#endif
    /* lock SPI bus */
    void (*lock)(const struct __nand_spi *spi);
    /* unlock SPI bus */
    void (*unlock)(const struct __nand_spi *spi);
    /* some user data */
    void *user_data;
} nand_spi, *nand_spi_t;

struct nand_flash;

struct spi_nand_chip {
	struct rt_mutex	chip_lock;
	char		*name;
	uint8_t		dev_id_len;
	uint8_t		dev_id[SPINAND_MAX_ID_LEN];
	flstate_t	state;
    nand_spi 	spi;
	struct mtd_info	*mtd;

	int (*reset)(struct spi_nand_chip *chip);
	int (*read_id)(struct spi_nand_chip *chip, uint8_t *id);
	int (*load_page)(struct spi_nand_chip *chip, unsigned int page_addr);
	int (*read_cache)(struct spi_nand_chip *chip, unsigned int page_addr,
		unsigned int page_offset,	size_t length, uint8_t *read_buf);
	int (*store_cache)(struct spi_nand_chip *chip, unsigned int page_addr,
		unsigned int page_offset,	size_t length, uint8_t *write_buf);
	int (*write_page)(struct spi_nand_chip *chip, unsigned int page_addr);
	int (*erase_block)(struct spi_nand_chip *chip, uint32_t page_addr);
	int (*waitfunc)(struct spi_nand_chip *chip, uint8_t *status);
	int (*write_enable)(struct spi_nand_chip *chip);
	void (*get_ecc_status)(struct spi_nand_chip *chip, unsigned int status,
						unsigned int *corrected,
						unsigned int *ecc_errors);
	int (*enable_ecc)(struct spi_nand_chip *chip);
	int (*disable_ecc)(struct spi_nand_chip *chip);
	int (*block_bad)(struct mtd_info *mtd, loff_t ofs, int getchip);
	int (*set_qe)(struct spi_nand_chip *chip);

	uint8_t		*buf;
	uint8_t		*oobbuf;
	int		pagebuf;
	uint32_t		pagebuf_bitflips;
	uint64_t		size;
	uint32_t		block_size;
	uint16_t		page_size;
	uint16_t		page_spare_size;
	uint8_t		block_shift;
	uint8_t		page_shift;
	uint16_t		page_mask;
	uint32_t		options;
	uint16_t		ecc_strength_ds;
	uint16_t		ecc_step_ds;
	uint8_t		ecc_mask;
	uint8_t		ecc_uncorr;
	uint8_t		bits_per_cell;
	struct nand_ecclayout *ecclayout;
	uint32_t		bbt_options;
	uint8_t		*bbt;
	int		badblockpos;
	struct nand_bbt_descr *bbt_td;
	struct nand_bbt_descr *bbt_md;
	struct nand_bbt_descr *badblock_pattern;
	uint32_t    qe_addr;
	uint32_t    qe_flag;
	uint32_t    qe_mask;
	uint32_t    multi_wire_command_length;
    uint32_t    bitflip_threshold;

	uint8_t mf_id;                               /**< manufacturer ID */
    uint8_t type_id;                             /**< memory type ID */
    uint8_t capacity_id;                         /**< capacity ID */
    uint32_t capacity;                           /**< flash capacity (bytes) */
    uint16_t write_mode;                         /**< write mode @see nand_write_mode */
    uint32_t erase_gran;                         /**< erase granularity (bytes) */
    uint8_t erase_gran_cmd;                      /**< erase granularity size block command */

	void *user_data;
	int flag;
	struct {
		void (*delay)(void);                     /**< every retry's delay */
		size_t times;                            /**< default times for error retry */
	} retry;

	bool init_ok;
    unsigned int erase_block_size;

#ifdef RT_SFUD_USING_QSPI
    nand_qspi_read_cmd_format read_cmd_format;   /**< fast read cmd format */
#endif
};

int spi_nand_read_from_cache_snor_protocol(struct spi_nand_chip *chip,
		uint32_t page_addr, uint32_t column, size_t len, uint8_t *rbuf);
int spi_nand_scan_ident(struct mtd_info *mtd);
int spi_nand_scan_tail(struct mtd_info *mtd);
int spi_nand_scan_ident_release(struct mtd_info *mtd);
int spi_nand_scan_tail_release(struct mtd_info *mtd);
int spi_nand_release(struct mtd_info *mtd);
int __spi_nand_erase(struct mtd_info *mtd, struct erase_info *einfo,
		int allowbbt);
int spi_nand_isbad_bbt(struct mtd_info *mtd, loff_t offs, int allowbbt);
int spi_nand_default_bbt(struct mtd_info *mtd);
int spi_nand_markbad_bbt(struct mtd_info *mtd, loff_t offs);
void spi_nand_set_defaults(struct spi_nand_chip *chip);
int spi_nand_check(struct spi_nand_chip *chip);
rt_bool_t spi_nand_scan_id_table(struct spi_nand_chip *chip, uint8_t *id);
rt_mtd_t *rt_nand_flash_probe(const char *spi_flash_dev_name, const char *spi_dev_name);
void nand_log_info(const char *format, ...);
nand_err nand_spi_port_init(struct spi_nand_chip *chip);
int nand_read_id(struct spi_nand_chip *chip);

#endif /* __LINUX_MTD_SPI_NAND_H */

