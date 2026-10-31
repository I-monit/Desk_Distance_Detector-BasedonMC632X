#ifndef __ISP_BIND_INFO_H__
#define __ISP_BIND_INFO_H__
#include "sample_common_media.h"
#include "dsp/fh_system_mpi.h"
#include "vpu_config.h"

typedef struct isp_bind_info
{
    FH_SINT32 toVpu;
    FH_SINT32 lut2d;
} ISP_BIND_INFO;

ISP_BIND_INFO *get_isp_bind_info(FH_SINT32 grp_id);

FH_SINT32 isp_bind_vpu(FH_SINT32 grp_id);

FH_SINT32 dual_isp_bind_vpu(FH_SINT32 grp_id);

#endif // __ISP_BIND_INFO_H__
