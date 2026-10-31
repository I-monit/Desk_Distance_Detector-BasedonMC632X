/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author        Notes
   2018-09-10     heyuanjie87   first version

 */

#ifndef __MTD_H__
#define __MTD_H__

#include <rtthread.h>
#include <stddef.h>
#include <stdint.h>
#include <mtd/mtd-user.h>

#define MTD_TYPE_NOR    1
#define MTD_TYPE_NAND   2

#define MTD_FAIL_ADDR_UNKNOWN -1LL

#define MTD_ERASE_PENDING      	0x01
#define MTD_ERASING		0x02
#define MTD_ERASE_SUSPEND	0x04
#define MTD_ERASE_DONE          0x08
#define MTD_ERASE_FAILED        0x10
 /**
  * MTD operation modes
  *
  * @MTD_OPM_PLACE_OOB:	OOB data are placed at the given offset (default)
  * @MTD_OPM_AUTO_OOB:	OOB data are automatically placed at the free areas
  * @MTD_OPM_RAW:	    data are transferred as-is, with no error correction;
  */
enum mtd_opm
{
    MTD_OPM_PLACE_OOB = 0,
    MTD_OPM_AUTO_OOB = 1,
    MTD_OPM_RAW = 2,
};

#ifndef loff_t
typedef long loff_t;
#endif

struct mtd_oob_region
{
    uint8_t offset;
    uint8_t length;
};

typedef enum {
	MTD_OOB_PLACE,
	MTD_OOB_AUTO,
	MTD_OOB_RAW,
} mtd_oob_mode_t;

struct mtd_oob_ops {
	mtd_oob_mode_t	mode;
	size_t		len;
	size_t		retlen;
	size_t		ooblen;
	size_t		oobretlen;
	uint32_t	ooboffs;
	uint8_t		*datbuf;
	uint8_t		*oobbuf;
};

struct nand_ecclayout {
	uint32_t eccbytes;
	uint32_t eccpos[128];
	uint32_t oobavail;
	struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES];
};

struct erase_info {
	struct mtd_info *mtd;
	uint64_t addr;
	uint64_t len;
	uint64_t fail_addr;
	u_long time;
	u_long retries;
	unsigned dev;
	unsigned cell;
	void (*callback) (struct erase_info *self);
	u_long priv;
	u_char state;
	struct erase_info *next;
};
typedef struct mtd_info
{
    struct rt_device parent;

    const struct mtd_ops *ops;

    uint16_t oob_size;
    uint16_t sector_size;   /* Minimal writable flash unit size */
    uint32_t block_size:28; /* Erase size for the device */
    uint32_t type:4;

    size_t size;    /* Total size of the MTD */
    loff_t offset;  /* At which this MTD starts, from the beginning of the MEMORY */
    struct mtd_info *master;

    void *priv;
	uint32_t oobsize;   // Amount of OOB data per block (e.g. 16)
	uint32_t oobavail;  // Available OOB bytes per block
	unsigned int bitflip_threshold;

	/* OOB layout description */
	const struct mtd_ooblayout_ops *ooblayout;

	/* the ecc step size. */
	unsigned int ecc_step_size;

	/* max number of correctible bit errors per ecc step */
	unsigned int ecc_strength;

	/* Data for variable erase regions. If numeraseregions is zero,
	 * it means that the whole device has erasesize as given above.
	 */
	int numeraseregions;
	struct mtd_erase_region_info *eraseregions;

	/*
	 * Do not call via these pointers, use corresponding mtd_*()
	 * wrappers instead.
	 */
	int (*_erase) (struct mtd_info *mtd, struct erase_info *instr);
	int (*_read) (struct mtd_info *mtd, loff_t from, size_t len,
		      size_t *retlen, u_char *buf);
	int (*_write) (struct mtd_info *mtd, loff_t to, size_t len,
		       size_t *retlen, const u_char *buf);
	int (*_read_oob) (struct mtd_info *mtd, loff_t from,
			  struct mtd_oob_ops *ops);
	int (*_write_oob) (struct mtd_info *mtd, loff_t to,
			   struct mtd_oob_ops *ops);
	int (*_block_isreserved) (struct mtd_info *mtd, loff_t ofs);
	int (*_block_isbad) (struct mtd_info *mtd, loff_t ofs);
	int (*_block_markbad) (struct mtd_info *mtd, loff_t ofs);

	/* ECC status information */
	struct mtd_ecc_stats ecc_stats;	
}rt_mtd_t;

struct mtd_io_desc
{
    uint8_t mode;      /* operation mode(enum mtd_opm) */
    uint8_t ooblen;    /* number of oob bytes to write/read */
    uint8_t oobretlen; /* number of oob bytes written/read */
    uint8_t ooboffs;   /* offset in the oob area  */
    uint8_t *oobbuf;

    size_t  datlen;    /* number of data bytes to write/read */
    size_t  datretlen; /* number of data bytes written/read */
    uint8_t *datbuf;   /* if NULL only oob are read/written */
};

struct mtd_ops
{
    int(*erase)(rt_mtd_t *mtd, loff_t addr, size_t len);    /* return 0 if success */
    int(*read) (rt_mtd_t *mtd, loff_t from, struct mtd_io_desc *ops); /* return 0 if success */
    int(*write) (rt_mtd_t *mtd, loff_t to, struct mtd_io_desc *ops);  /* return 0 if success */
    int(*isbad) (rt_mtd_t *mtd, uint32_t block);    /* return 1 if bad, 0 not bad */
    int(*markbad) (rt_mtd_t *mtd, uint32_t block);  /* return 0 if success */
};

struct mtd_part
{
    const char *name;           /* name of the MTD partion */
    loff_t offset;              /* start addr of partion */
    size_t size;                /* size of partion */
};

int rt_mtd_erase(rt_mtd_t *mtd, loff_t addr, size_t size);
int rt_mtd_block_erase(rt_mtd_t *mtd, uint32_t block);
int rt_mtd_read_oob(rt_mtd_t *mtd, loff_t from, struct mtd_io_desc *desc);
int rt_mtd_write_oob(rt_mtd_t *mtd, loff_t to, struct mtd_io_desc *desc);
int rt_mtd_read(rt_mtd_t *mtd, loff_t from, uint8_t *buf, size_t len);
int rt_mtd_write(rt_mtd_t *mtd, loff_t to, const uint8_t *buf, size_t len);
int rt_mtd_block_markbad(rt_mtd_t *mtd, uint32_t block);
int rt_mtd_block_isbad(rt_mtd_t *mtd, uint32_t block);

rt_mtd_t* rt_mtd_get(const char *name);
int rt_mtd_register(rt_mtd_t *master, const struct mtd_part *parts, int np);

#endif
