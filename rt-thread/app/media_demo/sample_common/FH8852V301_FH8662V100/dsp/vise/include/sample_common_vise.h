#ifndef __SAMPLE_COMMON_VISE_H__
#define __SAMPLE_COMMON_VISE_H__
#include "vise_config.h"
#include "vise_bind_info.h"
#include "vpu_config.h"
#include "vpu_chn_config.h"
#include "vpu_bind_info.h"
#include "mpp/fh_vb_mpi.h"

#define MAX_VISE_CANVAS_NUM 4
#define MAX_VISE_LAYER_NUM 4

FH_SINT32 checkViseAttr(FH_SINT32 chn_id, FH_SINT32 width, FH_SINT32 height, FH_SINT32 yuv_type);

FH_SINT32 changeVise_PipAttr(FH_SINT32 vise_id, FH_AREA outside, FH_AREA inside, FH_SINT32 yuv_type);

FH_SINT32 changeVise_LrsAttr(FH_SINT32 vise_id, FH_AREA left, FH_AREA right, FH_SINT32 yuv_type);

FH_SINT32 changeVise_TbsAttr(FH_SINT32 vise_id, FH_AREA top, FH_AREA bottom, FH_SINT32 yuv_type);

FH_SINT32 changeVise_MbsAttr(FH_SINT32 vise_id, FH_AREA left_top, FH_AREA left_bottom, FH_AREA right_top, FH_AREA right_bottom, FH_SINT32 yuv_type);

FH_SINT32 sample_common_vise_start(FH_SINT32 vise_id);

FH_SINT32 sample_common_vise_pip_init(FH_SINT32 vise_id);

FH_SINT32 sample_common_vise_lrs_init(FH_SINT32 vise_id);

FH_SINT32 sample_common_vise_tbs_init(FH_SINT32 vise_id);

FH_SINT32 sample_common_vise_mbs_init(FH_SINT32 vise_id);

FH_SINT32 sample_common_vise_exit(FH_SINT32 vise_id);

#endif // __SAMPLE_COMMON_VISE_H__
