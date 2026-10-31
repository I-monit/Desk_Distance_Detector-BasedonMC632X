/**
 * port mtd api for ubi.
 */
#include "linux/mtd/ubi_mtd.h"
#include "ubi_rt_mtd.h"
#include "port_rt.h"
#include "ubiport_common.h"

int mtd_read(struct mtd_info *mtd, loff_t from, size_t len, size_t *retlen, u_char *buf)
{
    ubi_rt_mtd_info *pRtMtd = (ubi_rt_mtd_info *)mtd->priv_data;
    return ubirt_mtd_read(pRtMtd, from, len, retlen, buf);
}
int mtd_write(struct mtd_info *mtd, loff_t to, size_t len, size_t *retlen, const u_char *buf)
{
    ubi_rt_mtd_info *pRtMtd = (ubi_rt_mtd_info *)mtd->priv_data;
    return ubirt_mtd_write(pRtMtd, to, len, retlen, buf);
}
int mtd_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    ubi_rt_mtd_info *pRtMtd = (ubi_rt_mtd_info *)mtd->priv_data;
    return ubirt_mtd_erase(pRtMtd, instr->addr, instr->len, &instr->state);
}
int mtd_block_isbad(struct mtd_info *mtd, loff_t ofs)
{
    ubi_rt_mtd_info *pRtMtd = (ubi_rt_mtd_info *)mtd->priv_data;
    return ubirt_mtd_block_isbad(pRtMtd, ofs);
}
int mtd_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
    ubi_rt_mtd_info *pRtMtd = (ubi_rt_mtd_info *)mtd->priv_data;
    return ubirt_mtd_block_markbad(pRtMtd, ofs);
}
struct mtd_info *get_mtd_device(struct mtd_info *mtd, void *pDev)
{
    struct mtd_info *pMtd = kmalloc(sizeof(struct mtd_info), __GFP_ZERO);
    ubi_rt_mtd_info *pRtMtd = kmalloc(sizeof(ubi_rt_mtd_info), __GFP_ZERO);
    ubirt_get_mtd_device(pDev, pRtMtd);
    pMtd->priv_data = pRtMtd;
    pMtd->type = MTD_NANDFLASH;
    pMtd->flags = MTD_WRITEABLE;
    pMtd->name = strdup("noname");
    pMtd->erasesize = pRtMtd->erasesize;
    pMtd->writesize = pRtMtd->writesize;
    pMtd->writebufsize = pMtd->writesize;
    pMtd->size = pRtMtd->size;
    pMtd->oobavail = pRtMtd->oobavail;
    pMtd->oobsize = pRtMtd->oobsize;
    pMtd->_block_isbad = mtd_block_isbad;
    return pMtd;
}
struct mtd_info *get_mtd_device_nm(const char *name)
{
    struct mtd_info *pMtd = kmalloc(sizeof(struct mtd_info), __GFP_ZERO);
    ubi_rt_mtd_info *pRtMtd = kmalloc(sizeof(ubi_rt_mtd_info), __GFP_ZERO);
    ubirt_get_mtd_device_nm(name, pRtMtd);
    pMtd->priv_data = pRtMtd;
    pMtd->type = MTD_NANDFLASH;
    pMtd->flags = MTD_WRITEABLE;
    pMtd->name = strdup(name);
    pMtd->erasesize = pRtMtd->erasesize;
    pMtd->writesize = pRtMtd->writesize;
    pMtd->writebufsize = pMtd->writesize;
    pMtd->size = pRtMtd->size;
    pMtd->oobavail = pRtMtd->oobavail;
    pMtd->oobsize = pRtMtd->oobsize;
    pMtd->index = pRtMtd->index;
    pMtd->_block_isbad = mtd_block_isbad;
    return pMtd;
}
void put_mtd_device(struct mtd_info *mtd)
{ /* free mtd. */
    if (mtd)
    {
        if (mtd->priv_data)
        {
            kfree(mtd->priv_data);
        }
        if (mtd->name)
        {
            free(mtd->name); /* free from strdup */
        }
        kfree(mtd);
    }
}
uint64_t mtd_get_device_size(const struct mtd_info *mtd)
{
    ubi_rt_mtd_info *pRtMtd = (ubi_rt_mtd_info *)mtd->priv_data;
    uint64_t ret_size = ubirt_mtd_get_device_size(pRtMtd);
    return ret_size;
}
