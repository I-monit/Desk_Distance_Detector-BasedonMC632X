#ifndef __SAMPLE_COMMON_VPU_H__
#define __SAMPLE_COMMON_VPU_H__
#include "dsp/fh_vpu_mpi.h"
#include "vpu_chn_config.h"
#include "vpu_config.h"
#include "vpu_bind_info.h"

FH_SINT32 sample_common_vpu_get_frame_blk_size(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_UINT32 *blk_size);

FH_SINT32 sample_common_vpu_init(FH_VOID);

FH_SINT32 sample_common_vpu_chn_init(FH_VOID);

FH_SINT32 sample_common_vpu_bind(FH_VOID);

#endif // __SAMPLE_COMMON_VPU_H__
