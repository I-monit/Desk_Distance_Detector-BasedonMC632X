#ifndef __UBIFS_PORT_H__
#define __UBIFS_PORT_H__

struct block_drvr
{
    char *name;
    int (*select_hwpart)(int dev_num, int hwpart);
};

#define LOG2(x) (((x & 0xaaaaaaaa) ? 1 : 0) + ((x & 0xcccccccc) ? 2 : 0) + \
                 ((x & 0xf0f0f0f0) ? 4 : 0) + ((x & 0xff00ff00) ? 8 : 0) + \
                 ((x & 0xffff0000) ? 16 : 0))
#define LOG2_INVALID(type) ((type)((sizeof(type) << 3) - 1))

/* Part types */
#define PART_TYPE_UNKNOWN 0x00
#define PART_TYPE_MAC 0x01
#define PART_TYPE_DOS 0x02
#define PART_TYPE_ISO 0x03
#define PART_TYPE_AMIGA 0x04
#define PART_TYPE_EFI 0x05

/* maximum number of partition entries supported by search */
#define DOS_ENTRY_NUMBERS 8
#define ISO_ENTRY_NUMBERS 64
#define MAC_ENTRY_NUMBERS 64
#define AMIGA_ENTRY_NUMBERS 8
/*
 * Type string for U-Boot bootable partitions
 */
#define BOOT_PART_TYPE "U-Boot"  /* primary boot partition type    */
#define BOOT_PART_COMP "PPCBoot" /* PPCBoot compatibility type    */

/* device types */
#define DEV_TYPE_UNKNOWN 0xff  /* not connected */
#define DEV_TYPE_HARDDISK 0x00 /* harddisk */
#define DEV_TYPE_TAPE 0x01     /* Tape */
#define DEV_TYPE_CDROM 0x05    /* CD-ROM */
#define DEV_TYPE_OPDISK 0x07   /* optical disk */

#define PART_NAME_LEN 32
#define PART_TYPE_LEN 32
#define MAX_SEARCH_PARTITIONS 64

typedef struct disk_partition
{
    lbaint_t start;            /* # of first block in partition    */
    lbaint_t size;             /* number of blocks in partition    */
    ulong blksz;               /* block size in bytes            */
    uchar name[PART_NAME_LEN]; /* partition name            */
    uchar type[PART_TYPE_LEN]; /* string type description        */
    int bootable;              /* Active/Bootable flag is set        */
#if 0
#if CONFIG_IS_ENABLED(PARTITION_UUIDS)
    char    uuid[UUID_STR_LEN + 1];    /* filesystem UUID as string, if exists    */
#endif
#ifdef CONFIG_PARTITION_TYPE_GUID
    char    type_guid[UUID_STR_LEN + 1];    /* type GUID as string, if exists    */
#endif
#ifdef CONFIG_DOS_PARTITION
    uchar    sys_ind;    /* partition type             */
#endif
#endif
} disk_partition_t;

#define smp_wmb() __asm__ __volatile__("" \
                                       :  \
                                       :  \
                                       : "memory")
#define smp_mb() __asm__ __volatile__("" \
                                      :  \
                                      :  \
                                      : "memory")
#define smp_mb__before_atomic() smp_mb()
#define smp_mb__after_atomic() smp_mb()
#define smp_rmb() smp_mb()

#endif
