#ifndef __OVERLAY_TOSD_H__
#define __OVERLAY_TOSD_H__
#include "sample_common_media.h"
#include "dsp_ext/FHAdv_OSD_mpi.h"
#include "vpu_chn_config.h"
#include "enc_config.h"

#define OSD_FONT_DISP_SIZE (32)
#define SAMPLE_TOSD_GRAPH FHT_OSD_GRAPH_CTRL_TOSD_BEFORE_VP  /* 设置TOSD在VPU分通道后的GOSD层 */

FH_SINT32 sample_set_tosd(FH_SINT32 grp_id);

FH_SINT32 sample_unload_tosd(FH_SINT32 grp_id);

#endif /*__OVERLAY_TOSD_H__*/
