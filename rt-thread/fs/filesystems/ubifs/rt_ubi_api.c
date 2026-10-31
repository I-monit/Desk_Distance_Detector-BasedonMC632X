#include "ubi_glue.h"
#include "rtthread.h"
#include "rt_ubi_api.h"

#ifndef UBIFS_VOLUME_NAME
#define UBIFS_VOLUME_NAME "fhfs"
#endif
#define UBI_VER "1.05"
/*
* long nand_readtest(void)
* {
*    struct rt_mtd_nand_device *pNand;
*    pNand = RT_MTD_NAND_DEVICE(rt_device_find("inand0"));
*    if (pNand == RT_NULL || pNand->ops == RT_NULL)
*    {
*        rt_kprintf("error not find device or opt\n");
*        return -1;
*    }
*    rt_uint8_t buf[128] = { 0 };
*    pNand->ops->read_page(pNand, 0, buf, 4, NULL, 0);
*    rt_kprintf("%s\n", buf);
*    memset(buf,0,sizeof(buf));
*    pNand->ops->read_page(pNand, 1, buf, 4, NULL, 0);
*    rt_kprintf("%s\n", buf);
*    memset(buf, 0, sizeof(buf));
*    pNand->ops->read_page(pNand, 2, buf, 20, NULL, 0);
*    rt_kprintf("%s\n", buf+16);
*    return 0;
* }
*FINSH_FUNCTION_EXPORT(nand_readtest, read nand data);
*MSH_CMD_EXPORT(nand_readtest, read nand data);
*/
char *rt_ubi_get_defautlVolName(void)
{
    return UBIFS_VOLUME_NAME;
}
int rt_ubi_create_defaultVolume(const char *mtdName)
{
    return glue_ubi_create_vol(mtdName, rt_ubi_get_defautlVolName(), 1, 1, -1, 1);
}
void rt_ubiinit(const char *mtdName[], int mtdNum)
{
    rt_kprintf("version:%s\n", UBI_VER);
    dfs_ubifs_register();
    glue_ubi_setMtd(mtdName, mtdNum);
    if (glue_ubiinit() != 0)
    {
        rt_kprintf("ubi init error\n");
        return;
    }
    for (int i = 0; i < mtdNum; i++)
    {
        if (glue_checkUBIHeader(mtdName[i]) == 2) /* create volume */
        {
            rt_kprintf("to create volume,%s\n", mtdName[i]);
            rt_ubi_create_defaultVolume(mtdName[i]);
        }
    }

    dfs_ubifs_init();
}

void rt_ubiexit(void)
{
    glue_ubiexit();
}
