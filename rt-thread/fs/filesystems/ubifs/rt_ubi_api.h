
#ifndef __RT_UBI_API_H__
#define __RT_UBI_API_H__

void rt_ubiinit(const char *mtdName[], int mtdNum);
extern int dfs_ubifs_init(void); /* dfs_ubifs_api.c */
void dfs_ubifs_register(void);   /* dfs_ubifs_api.c */
int rt_ubi_create_defaultVolume(const char *mtdName);
char *rt_ubi_get_defautlVolName(void);
#endif
