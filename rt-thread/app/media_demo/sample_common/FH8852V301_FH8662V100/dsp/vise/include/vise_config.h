#ifndef __VISE_CONFIG_H__
#define __VISE_CONFIG_H__

#include "sample_common_media.h"
#include "dsp/fh_vise_mpi.h"
#include "vpu_config.h"

#define VISE_MAX_W 4096
#define VISE_MAX_H 4096

#define VISE_MIN_W 64
#define VISE_MIN_H 64

//=======================PIP defined============================
#if defined FH_APP_VISE_OUTSIDE_GRP0
#define FH_APP_VISE_PIP_OUTSIDE_GRP 0
#elif defined FH_APP_VISE_OUTSIDE_GRP1
#define FH_APP_VISE_PIP_OUTSIDE_GRP 1
#elif defined FH_APP_VISE_OUTSIDE_GRP2
#define FH_APP_VISE_PIP_OUTSIDE_GRP 2
#else
#define FH_APP_VISE_PIP_OUTSIDE_GRP 0
#endif

#if defined FH_APP_VISE_OUTSIDE_GRP0_CHN0 || defined FH_APP_VISE_OUTSIDE_GRP1_CHN0 || defined FH_APP_VISE_OUTSIDE_GRP2_CHN0
#define FH_APP_VISE_PIP_OUTSIDE_CHN 0
#elif defined FH_APP_VISE_OUTSIDE_GRP0_CHN1 || defined FH_APP_VISE_OUTSIDE_GRP1_CHN1 || defined FH_APP_VISE_OUTSIDE_GRP2_CHN1
#define FH_APP_VISE_PIP_OUTSIDE_CHN 1
#elif defined FH_APP_VISE_OUTSIDE_GRP0_CHN2 || defined FH_APP_VISE_OUTSIDE_GRP1_CHN2 || defined FH_APP_VISE_OUTSIDE_GRP2_CHN2
#define FH_APP_VISE_PIP_OUTSIDE_CHN 2
#else
#define FH_APP_VISE_PIP_OUTSIDE_CHN 1
#endif

#if defined FH_APP_VISE_INSIDE_GRP0
#define FH_APP_VISE_PIP_INSIDE_GRP 0
#elif defined FH_APP_VISE_INSIDE_GRP1
#define FH_APP_VISE_PIP_INSIDE_GRP 1
#elif defined FH_APP_VISE_INSIDE_GRP2
#define FH_APP_VISE_PIP_INSIDE_GRP 2
#else
#define FH_APP_VISE_PIP_INSIDE_GRP 0
#endif

#if defined FH_APP_VISE_INSIDE_GRP0_CHN0 || defined FH_APP_VISE_INSIDE_GRP1_CHN0 || defined FH_APP_VISE_INSIDE_GRP2_CHN0
#define FH_APP_VISE_PIP_INSIDE_CHN 0
#elif defined FH_APP_VISE_INSIDE_GRP0_CHN1 || defined FH_APP_VISE_INSIDE_GRP1_CHN1 || defined FH_APP_VISE_INSIDE_GRP2_CHN1
#define FH_APP_VISE_PIP_INSIDE_CHN 1
#elif defined FH_APP_VISE_INSIDE_GRP0_CHN2 || defined FH_APP_VISE_INSIDE_GRP1_CHN2 || defined FH_APP_VISE_INSIDE_GRP2_CHN2
#define FH_APP_VISE_PIP_INSIDE_CHN 2
#else
#define FH_APP_VISE_PIP_INSIDE_CHN 1
#endif
//=======================PIP defined============================

//=======================LRS defined============================
#if defined FH_APP_VISE_LEFT_GRP0
#define FH_APP_VISE_LRS_LEFT_GRP 0
#elif defined FH_APP_VISE_LEFT_GRP1
#define FH_APP_VISE_LRS_LEFT_GRP 1
#elif defined FH_APP_VISE_LEFT_GRP2
#define FH_APP_VISE_LRS_LEFT_GRP 2
#else
#define FH_APP_VISE_LRS_LEFT_GRP 0
#endif

#if defined FH_APP_VISE_LEFT_GRP0_CHN0 || defined FH_APP_VISE_LEFT_GRP1_CHN0 || defined FH_APP_VISE_LEFT_GRP2_CHN0
#define FH_APP_VISE_LRS_LEFT_CHN 0
#elif defined FH_APP_VISE_LEFT_GRP0_CHN1 || defined FH_APP_VISE_LEFT_GRP1_CHN1 || defined FH_APP_VISE_LEFT_GRP2_CHN1
#define FH_APP_VISE_LRS_LEFT_CHN 1
#elif defined FH_APP_VISE_LEFT_GRP0_CHN2 || defined FH_APP_VISE_LEFT_GRP1_CHN2 || defined FH_APP_VISE_LEFT_GRP2_CHN2
#define FH_APP_VISE_LRS_LEFT_CHN 2
#else
#define FH_APP_VISE_LRS_LEFT_CHN 1
#endif

#if defined FH_APP_VISE_RIGHT_GRP0
#define FH_APP_VISE_LRS_RIGHT_GRP 0
#elif defined FH_APP_VISE_RIGHT_GRP1
#define FH_APP_VISE_LRS_RIGHT_GRP 1
#elif defined FH_APP_VISE_RIGHT_GRP2
#define FH_APP_VISE_LRS_RIGHT_GRP 2
#else
#define FH_APP_VISE_LRS_RIGHT_GRP 0
#endif

#if defined FH_APP_VISE_RIGHT_GRP0_CHN0 || defined FH_APP_VISE_RIGHT_GRP1_CHN0 || defined FH_APP_VISE_RIGHT_GRP2_CHN0
#define FH_APP_VISE_LRS_RIGHT_CHN 0
#elif defined FH_APP_VISE_RIGHT_GRP0_CHN1 || defined FH_APP_VISE_RIGHT_GRP1_CHN1 || defined FH_APP_VISE_RIGHT_GRP2_CHN1
#define FH_APP_VISE_LRS_RIGHT_CHN 1
#elif defined FH_APP_VISE_RIGHT_GRP0_CHN2 || defined FH_APP_VISE_RIGHT_GRP1_CHN2 || defined FH_APP_VISE_RIGHT_GRP2_CHN2
#define FH_APP_VISE_LRS_RIGHT_CHN 2
#else
#define FH_APP_VISE_LRS_RIGHT_CHN 1
#endif
//=======================LRS defined============================

//=======================TBS defined============================
#if defined FH_APP_VISE_TOP_GRP0
#define FH_APP_VISE_TBS_TOP_GRP 0
#elif defined FH_APP_VISE_TOP_GRP1
#define FH_APP_VISE_TBS_TOP_GRP 1
#elif defined FH_APP_VISE_TOP_GRP2
#define FH_APP_VISE_TBS_TOP_GRP 2
#else
#define FH_APP_VISE_TBS_TOP_GRP 0
#endif

#if defined FH_APP_VISE_TOP_GRP0_CHN0 || defined FH_APP_VISE_TOP_GRP1_CHN0 || defined FH_APP_VISE_TOP_GRP2_CHN0
#define FH_APP_VISE_TBS_TOP_CHN 0
#elif defined FH_APP_VISE_TOP_GRP0_CHN1 || defined FH_APP_VISE_TOP_GRP1_CHN1 || defined FH_APP_VISE_TOP_GRP2_CHN1
#define FH_APP_VISE_TBS_TOP_CHN 1
#elif defined FH_APP_VISE_TOP_GRP0_CHN2 || defined FH_APP_VISE_TOP_GRP1_CHN2 || defined FH_APP_VISE_TOP_GRP2_CHN2
#define FH_APP_VISE_TBS_TOP_CHN 2
#else
#define FH_APP_VISE_TBS_TOP_CHN 1
#endif

#if defined FH_APP_VISE_BOTTOM_GRP0
#define FH_APP_VISE_TBS_BOTTOM_GRP 0
#elif defined FH_APP_VISE_BOTTOM_GRP1
#define FH_APP_VISE_TBS_BOTTOM_GRP 1
#elif defined FH_APP_VISE_BOTTOM_GRP2
#define FH_APP_VISE_TBS_BOTTOM_GRP 2
#else
#define FH_APP_VISE_TBS_BOTTOM_GRP 0
#endif

#if defined FH_APP_VISE_BOTTOM_GRP0_CHN0 || defined FH_APP_VISE_BOTTOM_GRP1_CHN0 || defined FH_APP_VISE_BOTTOM_GRP2_CHN0
#define FH_APP_VISE_TBS_BOTTOM_CHN 0
#elif defined FH_APP_VISE_BOTTOM_GRP0_CHN1 || defined FH_APP_VISE_BOTTOM_GRP1_CHN1 || defined FH_APP_VISE_BOTTOM_GRP2_CHN1
#define FH_APP_VISE_TBS_BOTTOM_CHN 1
#elif defined FH_APP_VISE_BOTTOM_GRP0_CHN2 || defined FH_APP_VISE_BOTTOM_GRP1_CHN2 || defined FH_APP_VISE_BOTTOM_GRP2_CHN2
#define FH_APP_VISE_TBS_BOTTOM_CHN 2
#else
#define FH_APP_VISE_TBS_BOTTOM_CHN 1
#endif
//=======================TBS defined============================

//=======================MBS defined============================
#if defined FH_APP_VISE_LEFT_TOP_GRP0
#define FH_APP_VISE_MBS_LEFT_TOP_GRP 0
#elif defined FH_APP_VISE_LEFT_TOP_GRP1
#define FH_APP_VISE_MBS_LEFT_TOP_GRP 1
#elif defined FH_APP_VISE_LEFT_TOP_GRP2
#define FH_APP_VISE_MBS_LEFT_TOP_GRP 2
#else
#define FH_APP_VISE_MBS_LEFT_TOP_GRP 0
#endif

#if defined FH_APP_VISE_LEFT_TOP_GRP0_CHN0 || defined FH_APP_VISE_LEFT_TOP_GRP1_CHN0 || defined FH_APP_VISE_LEFT_TOP_GRP2_CHN0
#define FH_APP_VISE_MBS_LEFT_TOP_CHN 0
#elif defined FH_APP_VISE_LEFT_TOP_GRP0_CHN1 || defined FH_APP_VISE_LEFT_TOP_GRP1_CHN1 || defined FH_APP_VISE_LEFT_TOP_GRP2_CHN1
#define FH_APP_VISE_MBS_LEFT_TOP_CHN 1
#elif defined FH_APP_VISE_LEFT_TOP_GRP0_CHN2 || defined FH_APP_VISE_LEFT_TOP_GRP1_CHN2 || defined FH_APP_VISE_LEFT_TOP_GRP2_CHN2
#define FH_APP_VISE_MBS_LEFT_TOP_CHN 2
#else
#define FH_APP_VISE_MBS_LEFT_TOP_CHN 1
#endif

#if defined FH_APP_VISE_LEFT_BOTTOM_GRP0
#define FH_APP_VISE_MBS_LEFT_BOTTOM_GRP 0
#elif defined FH_APP_VISE_LEFT_BOTTOM_GRP1
#define FH_APP_VISE_MBS_LEFT_BOTTOM_GRP 1
#elif defined FH_APP_VISE_LEFT_BOTTOM_GRP2
#define FH_APP_VISE_MBS_LEFT_BOTTOM_GRP 2
#else
#define FH_APP_VISE_MBS_LEFT_BOTTOM_GRP 0
#endif

#if defined FH_APP_VISE_LEFT_BOTTOM_GRP0_CHN0 || defined FH_APP_VISE_LEFT_BOTTOM_GRP1_CHN0 || defined FH_APP_VISE_LEFT_BOTTOM_GRP2_CHN0
#define FH_APP_VISE_MBS_LEFT_BOTTOM_CHN 0
#elif defined FH_APP_VISE_LEFT_BOTTOM_GRP0_CHN1 || defined FH_APP_VISE_LEFT_BOTTOM_GRP1_CHN1 || defined FH_APP_VISE_LEFT_BOTTOM_GRP2_CHN1
#define FH_APP_VISE_MBS_LEFT_BOTTOM_CHN 1
#elif defined FH_APP_VISE_LEFT_BOTTOM_GRP0_CHN2 || defined FH_APP_VISE_LEFT_BOTTOM_GRP1_CHN2 || defined FH_APP_VISE_LEFT_BOTTOM_GRP2_CHN2
#define FH_APP_VISE_MBS_LEFT_BOTTOM_CHN 2
#else
#define FH_APP_VISE_MBS_LEFT_BOTTOM_CHN 1
#endif

#if defined FH_APP_VISE_RIGHT_TOP_GRP0
#define FH_APP_VISE_MBS_RIGHT_TOP_GRP 0
#elif defined FH_APP_VISE_RIGHT_TOP_GRP1
#define FH_APP_VISE_MBS_RIGHT_TOP_GRP 1
#elif defined FH_APP_VISE_RIGHT_TOP_GRP2
#define FH_APP_VISE_MBS_RIGHT_TOP_GRP 2
#else
#define FH_APP_VISE_MBS_RIGHT_TOP_GRP 0
#endif

#if defined FH_APP_VISE_RIGHT_TOP_GRP0_CHN0 || defined FH_APP_VISE_RIGHT_TOP_GRP1_CHN0 || defined FH_APP_VISE_RIGHT_TOP_GRP2_CHN0
#define FH_APP_VISE_MBS_RIGHT_TOP_CHN 0
#elif defined FH_APP_VISE_RIGHT_TOP_GRP0_CHN1 || defined FH_APP_VISE_RIGHT_TOP_GRP1_CHN1 || defined FH_APP_VISE_RIGHT_TOP_GRP2_CHN1
#define FH_APP_VISE_MBS_RIGHT_TOP_CHN 1
#elif defined FH_APP_VISE_RIGHT_TOP_GRP0_CHN2 || defined FH_APP_VISE_RIGHT_TOP_GRP1_CHN2 || defined FH_APP_VISE_RIGHT_TOP_GRP2_CHN2
#define FH_APP_VISE_MBS_RIGHT_TOP_CHN 2
#else
#define FH_APP_VISE_MBS_RIGHT_TOP_CHN 1
#endif

#if defined FH_APP_VISE_RIGHT_BOTTOM_GRP0
#define FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP 0
#elif defined FH_APP_VISE_RIGHT_BOTTOM_GRP1
#define FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP 1
#elif defined FH_APP_VISE_RIGHT_BOTTOM_GRP2
#define FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP 2
#else
#define FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP 0
#endif

#if defined FH_APP_VISE_RIGHT_BOTTOM_GRP0_CHN0 || defined FH_APP_VISE_RIGHT_BOTTOM_GRP1_CHN0 || defined FH_APP_VISE_RIGHT_BOTTOM_GRP2_CHN0
#define FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN 0
#elif defined FH_APP_VISE_RIGHT_BOTTOM_GRP0_CHN1 || defined FH_APP_VISE_RIGHT_BOTTOM_GRP1_CHN1 || defined FH_APP_VISE_RIGHT_BOTTOM_GRP2_CHN1
#define FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN 1
#elif defined FH_APP_VISE_RIGHT_BOTTOM_GRP0_CHN2 || defined FH_APP_VISE_RIGHT_BOTTOM_GRP1_CHN2 || defined FH_APP_VISE_RIGHT_BOTTOM_GRP2_CHN2
#define FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN 2
#else
#define FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN 1
#endif
//=======================MBS defined============================

FH_SINT32 vise_canvas_init(FH_SINT32 vise_id, FH_VISE_CANVAS_ATTR *pstcanvasattr);

FH_SINT32 vise_pip_change_area_attr(FH_SINT32 vise_id, FH_AREA outside, FH_AREA inside, FH_SINT32 yuv_type);

FH_SINT32 vise_lrs_change_area_attr(FH_SINT32 vise_id, FH_AREA left, FH_AREA right, FH_SINT32 yuv_type);

FH_SINT32 vise_tbs_change_area_attr(FH_SINT32 vise_id, FH_AREA top, FH_AREA bottom, FH_SINT32 yuv_type);

FH_SINT32 vise_mbs_change_area_attr(FH_SINT32 vise_id, FH_AREA left_top, FH_AREA left_bottom, FH_AREA right_top, FH_AREA right_bottom, FH_SINT32 yuv_type);

FH_SINT32 vise_set_buffer(FH_SINT32 vise_id, FH_UINT32 vb_pool_id);

FH_SINT32 vise_detach_buffer(FH_SINT32 vise_id);

FH_SINT32 vise_get_stream(FH_SINT32 vise_id, FH_VIDEO_FRAME *pstframeinfo);

FH_SINT32 vise_release_stream(FH_SINT32 vise_id, FH_VIDEO_FRAME *pstframeinfo);

FH_SINT32 vise_grp_start(FH_SINT32 vise_id);

FH_SINT32 vise_grp_stop(FH_SINT32 vise_id);

FH_SINT32 vise_exit(FH_SINT32 vise_id);

#endif // __VISE_CONFIG_H__
