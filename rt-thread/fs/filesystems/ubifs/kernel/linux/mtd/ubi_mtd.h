/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright © 1999-2010 David Woodhouse <dwmw2@infradead.org> et al.
 *
 */
#if 1
#ifndef __UBI_MTD_H__
#define __UBI_MTD_H__

#include <linux/compat.h>
/* #include <mtd/mtd-abi.h> */
#include <ubi_errno.h>
/* #include <linux/list.h> */
#include "port_cpu.h"

#define MAX_MTD_DEVICES 32

#define MTD_ERASE_PENDING 0x01
#define MTD_ERASING 0x02
#define MTD_ERASE_SUSPEND 0x04
#define MTD_ERASE_DONE 0x08
#define MTD_ERASE_FAILED 0x10

#define MTD_FAIL_ADDR_UNKNOWN -1LL

/* def from config */
#define CONFIG_MTD_UBI_BEB_LIMIT 20
#define CONFIG_MTD_UBI_WL_THRESHOLD 4096
#define CONFIG_MTD_UBI_FASTMAP 1

#ifndef EUCLEAN
#define EUCLEAN 117 /* Structure needs cleaning */
#endif

#define MTD_ABSENT 0
#define MTD_RAM 1
#define MTD_ROM 2
#define MTD_NORFLASH 3
#define MTD_NANDFLASH 4 /* SLC NAND */
#define MTD_DATAFLASH 6
#define MTD_UBIVOLUME 7
#define MTD_MLCNANDFLASH 8 /* MLC NAND (including TLC) */

#define MTD_WRITEABLE 0x400     /* Device is writeable */
#define MTD_BIT_WRITEABLE 0x800 /* Single bits can be flipped */
#define MTD_NO_ERASE 0x1000     /* No erase necessary */
#define MTD_POWERUP_LOCK 0x2000 /* Always locked after reset */

/*
 * If the erase fails, fail_addr might indicate exactly which block failed. If
 * fail_addr = MTD_FAIL_ADDR_UNKNOWN, the failure was not at the device level
 * or was not specific to any particular block.
 */
struct erase_info
{
    struct mtd_info *mtd;
    uint64_t addr;
    uint64_t len;
    uint64_t fail_addr;
    u_long time;
    u_long retries;
    unsigned dev;
    unsigned cell;
    void (*callback)(struct erase_info *self);
    u_long priv;
    u_char state;
    struct erase_info *next;
    int scrub;
};

struct mtd_info
{
    u_char type;
    uint32_t flags;
    uint64_t size; /* Total size of the MTD */

    /* "Major" erase size for the device. Naïve users may take this
     * to be the only erase size available, or may use the more detailed
     * information below if they desire
     */
    uint32_t erasesize;
    /* Minimal writable flash unit size. In case of NOR flash it is 1 (even
     * though individual bits can be cleared), in case of NAND flash it is
     * one NAND page (or half, or one-fourths of it), in case of ECC-ed NOR
     * it is of ECC block size, etc. It is illegal to have writesize = 0.
     * Any driver registering a struct mtd_info must ensure a writesize of
     * 1 or larger.
     */
    uint32_t writesize;

    /*
     * Size of the write buffer used by the MTD. MTD devices having a write
     * buffer can write multiple writesize chunks at a time. E.g. while
     * writing 4 * writesize bytes to a device with 2 * writesize bytes
     * buffer the MTD driver can (but doesn't have to) do 2 writesize
     * operations, but not 4. Currently, all NANDs have writebufsize
     * equivalent to writesize (NAND page size). Some NOR flashes do have
     * writebufsize greater than writesize.
     */
    uint32_t writebufsize;

    uint32_t oobsize;  /* Amount of OOB data per block (e.g. 16) */
    uint32_t oobavail; /* Available OOB bytes per block */

    /*
     * If erasesize is a power of 2 then the shift is stored in
     * erasesize_shift otherwise erasesize_shift is zero. Ditto writesize.
     */
    unsigned int erasesize_shift;
    unsigned int writesize_shift;
    /* Masks based on erasesize_shift and writesize_shift */
    unsigned int erasesize_mask;
    unsigned int writesize_mask;

    /*
     * read ops return -EUCLEAN if max number of bitflips corrected on any
     * one region comprising an ecc step equals or exceeds this value.
     * Settable by driver, else defaults to ecc_strength.  User can override
     * in sysfs.  N.B. The meaning of the -EUCLEAN return code has changed;
     * see Documentation/ABI/testing/sysfs-class-mtd for more detail.
     */
    unsigned int bitflip_threshold;

    /* Kernel-only stuff starts here. */
    char *name;
    int index;
    /* Data for variable erase regions. If numeraseregions is zero,
     * it means that the whole device has erasesize as given above.
     */
    int numeraseregions;
    /* Subpage shift (NAND) */
    int subpage_sft;
    void (*_sync)(struct mtd_info *mtd);
    int (*_block_isbad)(struct mtd_info *mtd, loff_t ofs);
    void *priv_data;
};

int mtd_erase(struct mtd_info *mtd, struct erase_info *instr);
int mtd_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen,
             u_char *buf);
int mtd_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen,
              const u_char *buf);
int mtd_block_isbad(struct mtd_info *mtd, loff_t ofs);
int mtd_block_markbad(struct mtd_info *mtd, loff_t ofs);
struct mtd_info *get_mtd_device(struct mtd_info *mtd, void *pDev);
struct mtd_info *get_mtd_device_nm(const char *name);
void put_mtd_device(struct mtd_info *mtd);
uint64_t mtd_get_device_size(const struct mtd_info *mtd);

static inline void mtd_sync(struct mtd_info *mtd)
{
    if (mtd->_sync)
        mtd->_sync(mtd);
}
static inline uint32_t mtd_div_by_eb(uint64_t sz, struct mtd_info *mtd)
{
    if (mtd->erasesize_shift)
        return sz >> mtd->erasesize_shift;
    do_div(sz, mtd->erasesize);
    return sz;
}
static inline uint32_t mtd_mod_by_eb(uint64_t sz, struct mtd_info *mtd)
{
    if (mtd->erasesize_shift)
        return sz & mtd->erasesize_mask;
    return do_div(sz, mtd->erasesize);
}
static inline int mtd_can_have_bb(const struct mtd_info *mtd)
{
    return !!mtd->_block_isbad;
}
static inline int mtd_is_bitflip(int err)
{
    return err == -EUCLEAN;
}

static inline int mtd_is_eccerr(int err)
{
    return err == -EBADMSG;
}

static inline int mtd_is_bitflip_or_eccerr(int err)
{
    return mtd_is_bitflip(err) || mtd_is_eccerr(err);
}

#endif /* __MTD_MTD_H__ */
#endif
