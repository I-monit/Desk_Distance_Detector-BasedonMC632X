/*
 * Date           Author       Notes
 * 2020/07/10     wf_wangfeng@qq.com           glue for ubi and rt devices.
 */
#ifndef __UBI_GLUE_H__
#define __UBI_GLUE_H__

void glue_ubi_setMtd(const char *mtdName[], int mtdNum);
int glue_ubiinit(void);
void glue_ubiexit(void);
int glue_checkUBIHeader(const char *mtdName);
int glue_ubi_getUbiDevName(const char *mtdName, const char *volName, char *ubiDevName, int devNameLen);
int glue_ubi_create_vol(const char *mtdname, char *volume, int size, int dynamic, int vol_id, int auto_resize);
int glue_ubi_attach_mtd(char *mtdname);
int glue_ubi_detach_mtd(char *mtdname);
int glue_ubi_changeUbiThreadPriority(void);

void glue_ubi_mtd_putdevice(void *mtd);
void *glue_ubi_mtd_getdevice(const char *mtdName);
int glue_ubi_mtd_read(void *mtd, unsigned int to, unsigned int len, unsigned int *retlen, unsigned char *buf);
int glue_ubi_mtd_write(void *mtd, unsigned int to, unsigned int len, unsigned int *retlen, const unsigned char *buf);
int glue_ubi_mtd_erase(char *mtdName);
#endif
