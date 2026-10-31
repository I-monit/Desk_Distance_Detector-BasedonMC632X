#ifndef __OVERLAY_GBOX_H__
#define __OVERLAY_GBOX_H__
#include "sample_common_media.h"
#include "dsp_ext/FHAdv_OSD_mpi.h"
#include "vpu_chn_config.h"

#define MAX_GBOX_LAYER_W (CHN0_MAX_WIDTH)
#define MAX_GBOX_LAYER_H (CHN0_MAX_HEIGHT)

#define SAMPLE_GBOX_GRAPH FHT_OSD_GRAPH_CTRL_GBOX_AFTER_VP  /* 设置GBOX在VPU分通道后的GOSD层 */

typedef struct
{
    FH_UINT32 enable;     /* gbox使能 0: 关闭  1: 开启 */
    FH_UINT32 gbox_id;    /* gbox id */
    FH_SINT32 x;          /*矩形框x坐标  */
    FH_SINT32 y;          /*矩形框y坐标  */
    FH_SINT32 w;          /* 矩形框宽度　 */
    FH_SINT32 h;          /* 矩形框高度　 */
    FHT_RgbColor_t color; /* 矩形框颜色　 */
} OSD_GBOX;

FH_SINT32 sample_set_gbox(FH_SINT32 grp_id, FH_SINT32 chn_id, OSD_GBOX *gbox);

#endif /*__OVERLAY_GBOX_H__*/
