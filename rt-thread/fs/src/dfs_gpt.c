/*
 * Copyright (C) 2010 Kevin Cernekee <cernekee@gmail.com>
 *
 * Licensed under GPLv2, see file LICENSE in this source tree.
 */
#include <dfs.h>
#include <dfs_fs.h>
#ifdef FH_USING_FHCRC
#include "fh_crc32.h"
#endif

#define GPT_MAGIC 0x5452415020494645ULL

enum {
    LEGACY_GPT_TYPE        = 0xee,
    GPT_MAX_PARTS          = 256,
    GPT_MAX_PART_ENTRY_LEN = 4096,
    GUID_LEN               = 16,
};

typedef struct {
    uint64_t magic;
    uint32_t revision;
    uint32_t hdr_size;
    uint32_t hdr_crc32;
    uint32_t reserved;
    uint64_t current_lba;
    uint64_t backup_lba;
    uint64_t first_usable_lba;
    uint64_t last_usable_lba;
    uint8_t  disk_guid[GUID_LEN];
    uint64_t first_part_lba;
    uint32_t n_parts;
    uint32_t part_entry_len;
    uint32_t part_array_crc32;
} gpt_header;

typedef struct {
    uint8_t  type_guid[GUID_LEN];
    uint8_t  part_guid[GUID_LEN];
    uint64_t lba_start;
    uint64_t lba_end;
    uint64_t flags;
    uint16_t name36[36];
} gpt_partition;

struct partition
{
    unsigned char boot_ind;         /* 0x80 - active */
    unsigned char head;             /* starting head */
    unsigned char sector;           /* starting sector */
    unsigned char cyl;              /* starting cylinder */
    unsigned char sys_ind;          /* what partition type */
    unsigned char end_head;         /* end head */
    unsigned char end_sector;       /* end sector */
    unsigned char end_cyl;          /* end cylinder */
    unsigned char start4[4];        /* starting sector counting from 0 */
    unsigned char size4[4];         /* nr of sectors in partition */
};

#define pt_offset(b, n) \
    ((struct partition *)((b) + 0x1be + (n) * sizeof(struct partition)))

static gpt_header *gpt_hdr;
static char *part_array;
static unsigned int n_parts;
static unsigned int part_entry_len;

static int valid_part_table_flag(const char *mbuffer)
{
    return (mbuffer[510] == 0x55 && (uint8_t)mbuffer[511] == 0xaa);
}

static inline gpt_partition *gpt_part(int i)
{
    if (i >= n_parts)
        return NULL;

    return (gpt_partition *)&part_array[i * part_entry_len];
}

static uint32_t
gpt_crc32(void *buf, int len)
{
    return fh_ether_crc32(0, buf, len);
}

#if 0
static void gpt_print_guid(uint8_t *buf)
{
    printf(
        "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        buf[3], buf[2], buf[1], buf[0],
        buf[5], buf[4],
        buf[7], buf[6],
        buf[8], buf[9],
        buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
}
#endif

void get_gpt_partition_info(uint32_t *first_part_lba, uint32_t *entry_len, uint32_t *parts)
{
    *entry_len      = part_entry_len;
    *parts          = n_parts;
    *first_part_lba = gpt_hdr->first_part_lba;
}

int dfs_filesystem_get_gpt_partition(struct dfs_partition *part,
                                    uint8_t         *buf,
                                    uint32_t        pindex)
{
    part_array = (char *)buf;
    gpt_partition *p = gpt_part(pindex);

    if (p->lba_start)
    {
        part->type   = LEGACY_GPT_TYPE;
        part->size   = p->lba_end - p->lba_start;
        part->offset = p->lba_start;
        rt_kprintf("found part[%d], begin: %lu, size: ",
           pindex, (unsigned long long)(part->offset * 512));
        if ((part->size >> 11) == 0)
            rt_kprintf("%d%s", part->size >> 1, "KB\n");     /* KB */
        else
        {
            unsigned int part_size;

            part_size = (part->size >> 11);                /* MB */
            if ((part_size >> 10) == 0)
                rt_kprintf("%d.%d%s", part_size, (part->size >> 1) & 0x3FF, "MB\n");
            else
                rt_kprintf("%d.%d%s",part_size >> 10,part_size & 0x3FF, "GB\n");
        }

        return RT_EOK;
    }

    return -EIO;
}

int check_gpt_label(void *mbuffer, void *gpt)
{
    uint32_t crc;
    struct partition *first = pt_offset(mbuffer, 0);

    if (!valid_part_table_flag(mbuffer) || first->sys_ind != LEGACY_GPT_TYPE)
        return 0;

    gpt_hdr = (void *)gpt;
    if (gpt_hdr->magic != GPT_MAGIC)
        return 0;

    crc = gpt_hdr->hdr_crc32;
    gpt_hdr->hdr_crc32 = 0;
    if (gpt_crc32(gpt_hdr, gpt_hdr->hdr_size) != crc)
    {
        rt_kprintf("\nwarning: GPT header CRC is invalid\n");
        return 0;
    }

    n_parts        = gpt_hdr->n_parts;
    part_entry_len = gpt_hdr->part_entry_len;
    if (n_parts > GPT_MAX_PARTS
    || part_entry_len > GPT_MAX_PART_ENTRY_LEN
    || (gpt_hdr->hdr_size) > 512
    ) {
        rt_kprintf("\nwarning: unable to parse GPT disklabel\n");
        return 0;
    }

    rt_kprintf("Found valid GPT with protective MBR; using GPT\n");
    return 1;
}
