#ifndef __SAMPLE_OVERLAY_H__
#define __SAMPLE_OVERLAY_H__
#include "sample_common_media.h"
#include "dsp_ext/FHAdv_OSD_mpi.h"
#include "overlay/include/overlay_gbox.h"
#ifdef FH_APP_OVERLAY_TOSD
#include "overlay/include/overlay_tosd.h"
#endif

FH_SINT32 sample_fh_overlay_start(FH_SINT32 grp_id);

FH_SINT32 sample_fh_overlay_stop(FH_SINT32 grp_id);

#endif /*__SAMPLE_OVERLAY_H__*/
