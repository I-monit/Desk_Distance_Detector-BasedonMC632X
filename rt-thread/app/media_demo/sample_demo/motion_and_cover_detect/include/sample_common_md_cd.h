#ifndef __SAMPLE_COMMON_MD_CD_H__
#define __SAMPLE_COMMON_MD_CD_H__
#include "vpu_config.h"
#include "overlay/include/overlay_gbox.h"

#define MD_CD_CHN 0

#define MDEX_MBNUM_H (2)
#define MDEX_MBNUM_V (2)
#define MDEX_BOX_NUM (10)

#define MD_AREA_NUM_H (2) /*MD模块，水平方向上的检测区域数,16*16的宏块为单位*/
#define MD_AREA_NUM_V (2) /*MD模块，垂直方向上的检测区域数,16*16的宏块为单位*/

#define MD_CD_CHECK_INTERVAL (1) /*25毫秒检测一次,FHAdv_MD_CD_Check函数同时检测MD/MD_EX/CD, \
                                     如果用户对于MD/MD_EX/CD配置了不同的检测间隔,       \
                                     advapi内部会取他们的最小检测间隔*/

FH_SINT32 sample_fh_md_cd_start(FH_SINT32 grp_id);

FH_SINT32 sample_fh_md_cd_stop(FH_SINT32 grp_id);

#endif /* __SAMPLE_COMMON_MD_CD_H__ */
