#ifndef __OVERLAY_MASK_H__
#define __OVERLAY_MASK_H__
#include "sample_common_media.h"
#include "dsp_ext/FHAdv_OSD_mpi.h"

#define SAMPLE_MASK_GRAPH FHT_OSD_GRAPH_CTRL_MASK_AFTER_VP  /* 设置MASK在VPU分通道后的GOSD层 */

FH_SINT32 sample_set_mask(FH_SINT32 grp_id);

#endif /*__OVERLAY_MASK_H__*/
