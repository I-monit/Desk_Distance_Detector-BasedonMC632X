#ifndef __SAMPLE_COMMON_AF_H__
#define __SAMPLE_COMMON_AF_H__

#include "sample_common_media.h"
#include "dsp_ext/FHAdv_SmartIR_mpi.h"
#include "isp_config.h"

FH_SINT32 sample_fh_smartIR_init(FH_SINT32 grp_id);

FH_SINT32 sample_fh_smartIR_exit(FH_SINT32 grp_id);

FH_SINT32 sample_fh_smartIR_run(FH_SINT32 grp_id);

#endif /* __SAMPLE_COMMON_ISP_DEMO_H__ */
