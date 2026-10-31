#ifndef __OVERLAY_LOGO_H__
#define __OVERLAY_LOGO_H__
#include "sample_common_media.h"
#include "dsp_ext/FHAdv_OSD_mpi.h"

#define SAMPLE_LOGO_GRAPH FHT_OSD_GRAPH_CTRL_LOGO_BEFORE_VP /* 设置LOGO在VPU分通道前的GOSD层 */

FH_SINT32 sample_set_logo(FH_SINT32 grp_id);

#endif /*__OVERLAY_LOGO_H__*/
