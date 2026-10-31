
#include <linux/err.h>
#include <ubi_uboot.h>
#include "ubi.h"
#include "ubi_glue.h"
#include "linux/mtd/ubi_mtd.h"

static int gUbiInited = 0;
int glue_ubiinit(void)
{
    int ret = 0;
    if (gUbiInited == 0)
    {
        ret = ubi_init();
        ubi_InitSchedule();
    }
    gUbiInited = 1;
    return ret;
}
void glue_ubiexit(void)
{
    if (gUbiInited == 1)
    {
        ubi_exit();
    }
    gUbiInited = 0;
}

static struct ubi_device *glue_ubi_getMtdUbiDevice(const char *mtdname)
{
    struct ubi_device *ubi = ubi_get_deviceByName(mtdname); /* ubi_get_device(0); */
    if (ubi == NULL)
    {
        rt_kprintf("no ubi device \n");
        glue_ubiinit();
    }
    if (ubi && ubi->mtd && ubi->mtd->name && strcmp(ubi->mtd->name, mtdname) != 0)
    {
        glue_ubiexit(); /* detach last mtd, */
        ubi = NULL;
        glue_ubiinit();
    }
    if (ubi == NULL)
    {
        ubi = ubi_get_deviceByName(mtdname); /* ubi_get_device(0); */
    }
    return ubi;
}

int glue_checkUBIHeader(const char *mtdName)
{
    int ret = -1;
    struct ubi_device *ubi = glue_ubi_getMtdUbiDevice(mtdName);
    if (ubi == NULL)
    {
        rt_kprintf("get ubi error.%s\n", mtdName);
        return -1;
    }
    if (ubi && ubi->vol_count == UBI_INT_VOL_COUNT)
    {
        ret = 2;
    }
    ubi_put_device(ubi);
    return ret;
}
int glue_ubi_getUbiDevName(const char *mtdName, const char *volName, char *ubiDevName, int devNameLen)
{
    struct ubi_device *ubi = glue_ubi_getMtdUbiDevice(mtdName);
    if (ubi == NULL)
    {
        rt_kprintf("get ubi error.%s\n", mtdName);
        return -1;
    }
    if (ubi && ubi->vol_count == UBI_INT_VOL_COUNT)
    {
        ubi_put_device(ubi);
        return -1;
    }
    int volNum = 0;
    if (volName != NULL)
    {
        for (int i = 0; i < ubi->vol_count; i++)
        {
            if (ubi->volumes[i] != NULL)
            {
                /*                printf("volname:%s,%s,%d\n", ubi->volumes[i]->name, volName, ubi->vol_count);*/
                if (strcmp(ubi->volumes[i]->name, volName) == 0)
                {
                    volNum = i;
                    break;
                }
            }
        }
    }
    snprintf(ubiDevName, devNameLen, "%s_%d", ubi->ubi_name, volNum);
    ubi_put_device(ubi);
    return 0;
}
void glue_ubi_setMtd(const char *mtdName[], int mtdNum)
{
    int i = 0;
    for (i = 0; i < mtdNum; i++)
    {
        ubi_mtd_param_parse(mtdName[i], 0);
    }
}
/* need to reinit ubi to refresh size. */
int glue_ubi_set_vol_autosize(const char *mtdname, char *volName)
{
    struct ubi_device *ubi = ubi_get_deviceByName(mtdname); /* ubi_get_device(0); */
    int i = 0, vol_id = -1;
    for (i = 0; i < ubi->vol_count; i++)
    {
        if (strcmp(ubi->volumes[i]->name, volName) == 0)
        {
            vol_id = i;
            break;
        }
    }
    if (vol_id == -1)
    {
        rt_kprintf("not find vol.%s\n", volName);
    }
    /* Change volume table record */
    struct ubi_vtbl_record *vtbl_rec = &ubi->vtbl[vol_id];
    vtbl_rec->flags |= UBI_VTBL_AUTORESIZE_FLG;
    int err = ubi_change_vtbl_record(ubi, vol_id, vtbl_rec);
    if (err)
    {
        rt_kprintf("change autosize error.%d\n", err);
    }
    ubi_put_device(ubi);
    return 0;
}

/**
 * create ubi volume.Note: must erase the mtd before creating
 * volume.
 *
 * @param mtdname : the mtd name.e.g. mtd8
 * @param volume : the volume name.the name was used when mount
 * @param size : the volume size,MiB.
 * @param dynamic:0: readonly;1:read/write.
 * @param vol_id: volume ID. -1:auto; other: use the set id.
 * @param auto_resize: 1: auto resize when create volume;0:use
 *                   the set size.
 *
 * @return int : 0:success;other: fail.
 */
int glue_ubi_create_vol(const char *mtdname, char *volume, int size, int dynamic, int vol_id, int auto_resize)
{
    struct ubi_mkvol_req req;
    memset(&req, 0, sizeof(req));
    bool skipcheck = false;

    if (dynamic)
        req.vol_type = UBI_DYNAMIC_VOLUME;
    else
        req.vol_type = UBI_STATIC_VOLUME;

    req.vol_id = vol_id; /* UBI_VOL_NUM_AUTO -1 */
    req.alignment = 1;
    req.bytes = size;

    strcpy(req.name, volume);
    req.name_len = strlen(volume);
    req.name[req.name_len] = '\0';
    req.flags = 0;
    if (skipcheck)
        req.flags |= UBI_VOL_SKIP_CRC_CHECK_FLG;
    /* UBI_VTBL_AUTORESIZE_FLG */
    /* It's duplicated at drivers/mtd/ubi/cdev.c */
    /* err = verify_mkvol_req(ubi, &req);
    if (err)
    {
        printf("verify_mkvol_req failed %d\n", err);
        return err;
    }*/
    struct ubi_device *ubi = glue_ubi_getMtdUbiDevice(mtdname);
    if (ubi == NULL)
    {
        glue_ubi_attach_mtd((char *)mtdname);
        ubi = glue_ubi_getMtdUbiDevice(mtdname);
        if (ubi == NULL)
        {
            rt_kprintf("get ubi device error\n");
            return -1;
        }
    }
    if (auto_resize)
    {
        req.bytes = ubi->avail_pebs * ubi->leb_size;
    }
    printf("Creating %s volume %s of size %ld\n",
           dynamic ? "dynamic" : "static", volume, (long)req.bytes);
    /* Call real ubi create volume */
    int ret = ubi_create_volume(ubi, &req);
    /*   if (auto_resize && ret == 0)
       {
           glue_ubi_set_vol_autosize(mtdname,volume);
            glue_ubiexit(); //must reinit ubi if set autosize. if not and mount fs,will mount fail when reboot.
           glue_ubiinit();
       }*/
    ubi_put_device(ubi);
    return ret;
}

void glue_ubi_mtd_putdevice(void *mtd)
{
    put_mtd_device((struct mtd_info *)mtd);
}
void *glue_ubi_mtd_getdevice(const char *mtdName)
{
    return (void *)get_mtd_device_nm(mtdName);
}
int glue_ubi_mtd_read(void *mtd, unsigned int to, unsigned int len, unsigned int *retlen, unsigned char *buf)
{
    return mtd_read((struct mtd_info *)mtd, to, len, retlen, buf);
}
int glue_ubi_mtd_write(void *mtd, unsigned int to, unsigned int len, unsigned int *retlen, const unsigned char *buf)
{
    return mtd_write((struct mtd_info *)mtd, to, len, retlen, buf);
}
int glue_ubi_mtd_erase(char *mtdName)
{
    struct mtd_info *mtd = get_mtd_device_nm(mtdName);
    if (mtd == NULL)
    {
        rt_kprintf("get mtd error\n");
        return -1;
    }
    struct erase_info eInfo;
    memset(&eInfo, 0, sizeof(eInfo));
    eInfo.addr = 0;
    eInfo.len = mtd->size;
    int ret = mtd_erase(mtd, &eInfo);
    put_mtd_device(mtd);
    rt_kprintf("erase %s,size:0x%x,ret:%d\n", mtdName, eInfo.len, ret);
    return ret;
}
int glue_ubi_attach_mtd(char *mtdname)
{
    struct ubi_device *ubi = ubi_get_deviceByName(mtdname); /* ubi_get_device(0); */
    if (ubi == NULL)
    {
        rt_kprintf("no ubi device \n");
        ubi_attach_mtdByName(mtdname);
    }
    if (ubi == NULL)
    {
        ubi = ubi_get_deviceByName(mtdname); /* ubi_get_device(0); */
    }
    if (ubi == NULL)
    {
        rt_kprintf("attach mtd error \n");
        return -1;
    }
    return 0;
}
int glue_ubi_detach_mtd(char *mtdname)
{
    struct ubi_device *ubi = ubi_get_deviceByName(mtdname); /* ubi_get_device(0); */
    if (ubi && ubi->mtd)
    {
        mutex_lock(&ubi_devices_mutex);
        ubi_detach_mtd_dev(ubi->ubi_num, 1);
        mutex_unlock(&ubi_devices_mutex);
        /* glue_ubiexit(); //detach last mtd, */
        ubi = NULL;
    }
    return 0;
}
int glue_ubi_changeUbiThreadPriority(void)
{
    struct ubi_device *ubi = ubi_get_device(0);
    if (ubi == NULL)
    {
        rt_kprintf("Priority:no ubi device \n");
        return -1;
    }
    return setThreadPriority(ubi->bgt_thread, 10);
}
