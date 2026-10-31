#ifndef __VPU_BIND_INFO_H__
#define __VPU_BIND_INFO_H__
#include "sample_common_media.h"
#include "dsp/fh_system_mpi.h"

typedef struct vpu_bind_info
{
    FH_SINT32 toVpu;
    FH_SINT32 toEnc;
    FH_SINT32 toVou;
} VPU_BIND_INFO;

VPU_BIND_INFO *get_vpu_bind_info(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 vpu_bind_bgm(FH_SINT32 grp_id);

FH_SINT32 vpu_bind_enc(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 vpu_bind_vpu(FH_UINT32 src_grp_id, FH_UINT32 src_chn_id, FH_UINT32 dst_grp_id);

FH_SINT32 vpu_bind_jpeg(FH_UINT32 src_grp_id, FH_UINT32 src_chn_id, FH_UINT32 dst_chn_id);

FH_SINT32 vpu_bind_vise(FH_UINT32 src_grp_id, FH_UINT32 src_chn_id, FH_UINT32 dst_grp_id, FH_UINT32 dst_chn_id);

FH_SINT32 unbind_vpu_chn(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 unbind_enc_chn(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 unbind_jpeg_chn(FH_UINT32 jpeg_chn);

#endif // __VPU_BIND_INFO_H__
