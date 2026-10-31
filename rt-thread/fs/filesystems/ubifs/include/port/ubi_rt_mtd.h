#ifndef _UBI_RT_MTD_H_
#define _UBI_RT_MTD_H_

typedef struct ubi_rt_mtd_info_t
{
    unsigned char type;
    unsigned int flags;
    unsigned int size; /* Total size of the MTD */
    unsigned int erasesize;
    unsigned int writesize;
    unsigned int oobsize;  /* Amount of OOB data per block (e.g. 16) */
    unsigned int oobavail; /* Available OOB bytes per block */
    char name[32];
    void *pDevHandle;
    int index;
} ubi_rt_mtd_info;

int ubirt_mtd_read(ubi_rt_mtd_info *mtd, unsigned int from, unsigned int len, unsigned int *retlen, unsigned char *buf);
int ubirt_mtd_write(ubi_rt_mtd_info *mtd, unsigned int to, unsigned int len, unsigned int *retlen, const unsigned char *buf);
int ubirt_mtd_erase(ubi_rt_mtd_info *mtd, unsigned int from, unsigned int length, unsigned char *state);
int ubirt_mtd_block_isbad(ubi_rt_mtd_info *mtd, unsigned int ofs);
int ubirt_mtd_block_markbad(ubi_rt_mtd_info *mtd, unsigned int ofs);
int ubirt_get_mtd_device(void *pNandDev, ubi_rt_mtd_info *mtd);
int ubirt_get_mtd_device_nm(const char *name, ubi_rt_mtd_info *rt_mtdinfo);
unsigned int ubirt_mtd_get_device_size(const ubi_rt_mtd_info *mtd);

#endif
