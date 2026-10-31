#ifndef __SAMPLE_COMMON_NNA_H__
#define __SAMPLE_COMMON_NNA_H__
#include "vpu_config.h"
#include "vpu_chn_config.h"
#include "enc_config.h"
#include "overlay/include/overlay_gbox.h"

#if defined FH_NN_ENABLE || defined FH_NN_AINR_ENABLE
#include "nna_config.h"

#ifndef FH_NN_DET_FPS
#define FH_NN_DET_FPS 10
#endif

#define BOX_MAX_NUM 50

FH_SINT32 sample_fh_nna_obj_detect_start(FH_SINT32 grp_id);
FH_SINT32 sample_fh_nna_obj_detect_stop(FH_SINT32 grp_id);

FH_SINT32 sample_fh_nna_ivs_start(FH_SINT32 grp_id);
FH_SINT32 sample_fh_nna_ivs_stop(FH_SINT32 grp_id);

FH_SINT32 sample_fh_nna_gesture_rec_start(FH_SINT32 grp_id);
FH_SINT32 sample_fh_nna_gesture_rec_stop(FH_SINT32 grp_id);

FH_SINT32 sample_fh_nna_face_snap_start(FH_SINT32 grp_id);
FH_SINT32 sample_fh_nna_face_snap_stop(FH_SINT32 grp_id);

FH_SINT32 sample_fh_nna_face_feature_locate_start(FH_SINT32 grp_id);
FH_SINT32 sample_fh_nna_face_feature_locate_stop(FH_SINT32 grp_id);

FH_SINT32 sample_fh_nna_face_rec_start(FH_SINT32 grp_id);
FH_SINT32 sample_fh_nna_face_rec_stop(FH_SINT32 grp_id);

FH_SINT32 sample_fh_nna_lprec_start(FH_SINT32 grp_id);
FH_SINT32 sample_fh_nna_lprec_stop(FH_SINT32 grp_id);

FH_SINT32 sample_fh_ainr_start(FH_SINT32 grp_id);
FH_SINT32 sample_fh_ainr_stop(FH_SINT32 grp_id);
#endif

#endif // __SAMPLE_COMMON_NNA_H__