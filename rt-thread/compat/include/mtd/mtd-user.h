/*
 *  * Copyright (c) 2015-2019 Shanghai Fullhan Microelectronics Co., Ltd.
 *  *
 *  * SPDX-License-Identifier: Apache-2.0
 *  *
 *  * Change Logs:
 *  * Date           Author       Notes
 *  * 2019-01-18      {fullhan}   the first version
 *  *
 */

#ifndef __MTD_ABI_H__
#define __MTD_ABI_H__

#ifdef __cplusplus
extern "C" {
#endif

struct erase_info_user
{
    unsigned int start;
    unsigned int length;
};
/*
 *struct erase_info_user64 {
 *    __u64 start;
 *    __u64 length;
 *};
 *
 *struct mtd_oob_buf {
 *    unsigned int start;
 *    unsigned int length;
 *    unsigned char __user *ptr;
 *};
 *
 *struct mtd_oob_buf64 {
 *    __u64 start;
 *    unsigned int pad;
 *    unsigned int length;
 *    __u64 usr_ptr;
 *};
 */

typedef struct mtd_info_user mtd_info_t;
typedef struct erase_info_user erase_info_t;
typedef struct region_info_user region_info_t;
typedef struct nand_oobinfo nand_oobinfo_t;
typedef struct nand_ecclayout_user nand_ecclayout_t;

#define MTD_ABSENT      0
#define MTD_RAM         1
#define MTD_ROM         2
#define MTD_NORFLASH        3
#define MTD_NANDFLASH       4
#define MTD_DATAFLASH       6
#define MTD_UBIVOLUME       7
#define MTD_MLCNANDFLASH    8

#define MTD_WRITEABLE       0x400    /* Device is writeable */
#define MTD_BIT_WRITEABLE   0x800    /* Single bits can be flipped */
#define MTD_NO_ERASE        0x1000   /* No erase necessary */
#define MTD_POWERUP_LOCK    0x2000   /* Always locked after reset */

/* Some common devices / combinations of capabilities */
#define MTD_CAP_ROM     0
#define MTD_CAP_RAM     (MTD_WRITEABLE | MTD_BIT_WRITEABLE | MTD_NO_ERASE)
#define MTD_CAP_NORFLASH    (MTD_WRITEABLE | MTD_BIT_WRITEABLE)
#define MTD_CAP_NANDFLASH   (MTD_WRITEABLE)

/* ECC byte placement */
#define MTD_NANDECC_OFF     0   /* Switch off ECC (Not recommended) */
#define MTD_NANDECC_PLACE   1   /* Use the given placement in the structure (YAFFS1 legacy mode) */
#define MTD_NANDECC_AUTOPLACE   2   /* Use the default placement scheme */
#define MTD_NANDECC_PLACEONLY   3   /* Use the given placement in the structure (Do not store ecc result on read) */
#define MTD_NANDECC_AUTOPL_USR  4   /* Use the given autoplacement scheme rather than using the default */

/* OTP mode selection */
#define MTD_OTP_OFF     0
#define MTD_OTP_FACTORY     1
#define MTD_OTP_USER        2

struct mtd_info_user
{
    unsigned char type;
    unsigned int flags;
    unsigned int size;     /* Total size of the MTD */
    unsigned int erasesize;
    unsigned int writesize;
    unsigned int oobsize;   /* Amount of OOB data per block (e.g. 16)*/
    /* The below two fields are obsolete and broken, do not use them
     * (TODO: remove at some point)
     */
    unsigned int ecctype;
    unsigned int eccsize;
};

struct region_info_user
{
    unsigned int offset;        /* At which this region starts,
                                 * from the beginning of the MTD
                                 */
    unsigned int erasesize;     /* For this region */
    unsigned int numblocks;     /* Number of blocks in this region */
    unsigned int regionindex;
};

struct otp_info
{
    unsigned int start;
    unsigned int length;
    unsigned int locked;
};

#define MEMGETINFO      0x01
#define MEMERASE        0x02
/*
 *#define MEMWRITEOOB        _IOWR('M', 3, struct mtd_oob_buf)
 *#define MEMREADOOB        _IOWR('M', 4, struct mtd_oob_buf)
 *#define MEMLOCK            _IOW('M', 5, struct erase_info_user)
 *#define MEMUNLOCK        _IOW('M', 6, struct erase_info_user)
 *#define MEMGETREGIONCOUNT    _IOR('M', 7, int)
 *#define MEMGETREGIONINFO    _IOWR('M', 8, struct region_info_user)
 *#define MEMSETOOBSEL        _IOW('M', 9, struct nand_oobinfo)
 *#define MEMGETOOBSEL        _IOR('M', 10, struct nand_oobinfo)
 *#define MEMGETBADBLOCK        _IOW('M', 11, __kernel_loff_t)
 *#define MEMSETBADBLOCK        _IOW('M', 12, __kernel_loff_t)
 *#define OTPSELECT        _IOR('M', 13, int)
 *#define OTPGETREGIONCOUNT    _IOW('M', 14, int)
 *#define OTPGETREGIONINFO    _IOW('M', 15, struct otp_info)
 *#define OTPLOCK            _IOR('M', 16, struct otp_info)
 *#define ECCGETLAYOUT        _IOR('M', 17, struct nand_ecclayout_user)
 *#define ECCGETSTATS        _IOR('M', 18, struct mtd_ecc_stats)
 *#define MTDFILEMODE        _IO('M', 19)
 *#define MEMERASE64        _IOW('M', 20, struct erase_info_user64)
 *#define MEMWRITEOOB64        _IOWR('M', 21, struct mtd_oob_buf64)
 *#define MEMREADOOB64        _IOWR('M', 22, struct mtd_oob_buf64)
 *#define MEMISLOCKED        _IOR('M', 23, struct erase_info_user)
 */
/*
 * Obsolete legacy interface. Keep it in order not to break userspace
 * interfaces
 */
struct nand_oobinfo
{
    unsigned int useecc;
    unsigned int eccbytes;
    unsigned int oobfree[8][2];
    unsigned int eccpos[32];
};

struct nand_oobfree
{
    unsigned int offset;
    unsigned int length;
};

#define MTD_MAX_OOBFREE_ENTRIES    8
#define MTD_MAX_ECCPOS_ENTRIES    64
/*
 * OBSOLETE: ECC layout control structure. Exported to user-space via ioctl
 * ECCGETLAYOUT for backwards compatbility and should not be mistaken as a
 * complete set of ECC information. The ioctl truncates the larger internal
 * structure to retain binary compatibility with the static declaration of the
 * ioctl. Note that the "MTD_MAX_..._ENTRIES" macros represent the max size of
 * the user struct, not the MAX size of the internal struct nand_ecclayout.
 */
struct nand_ecclayout_user
{
    unsigned int eccbytes;
    unsigned int eccpos[MTD_MAX_ECCPOS_ENTRIES];
    unsigned int oobavail;
    struct nand_oobfree oobfree[MTD_MAX_OOBFREE_ENTRIES];
};

/**
 * struct mtd_ecc_stats - error correction stats
 *
 * @corrected:    number of corrected bits
 * @failed:    number of uncorrectable errors
 * @badblocks:    number of bad blocks in this partition
 * @bbtblocks:    number of blocks reserved for bad block tables
 */
struct mtd_ecc_stats
{
    unsigned int corrected;
    unsigned int failed;
    unsigned int badblocks;
    unsigned int bbtblocks;
};

/*
 * Read/write file modes for access to MTD
 */
enum mtd_file_modes
{
    MTD_MODE_NORMAL = MTD_OTP_OFF,
    MTD_MODE_OTP_FACTORY = MTD_OTP_FACTORY,
    MTD_MODE_OTP_USER = MTD_OTP_USER,
    MTD_MODE_RAW,
};

#ifdef __cplusplus
}
#endif

#endif /* __MTD_ABI_H__ */
