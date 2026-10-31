#ifndef __SAMPLE_COMMON_DSP_H__
#define __SAMPLE_COMMON_DSP_H__
#include "sample_common_media.h"
#include "sample_common_vpu.h"
#include "sample_common_isp.h"
#include "sample_common_vicap.h"
#include "sample_common_enc.h"
#include "dsp/fh_system_mpi.h"
#include "mpp/fh_vb_mpi.h"
#include "mpp/fh_vb_mpipara.h"

FH_SINT32 sample_common_dsp_system_init(FH_VOID);

FH_SINT32 sample_common_dsp_system_exit(FH_VOID);

FH_SINT32 sample_common_dsp_init(FH_VOID);

FH_SINT32 sample_common_dsp_exit(FH_VOID);

#endif // __SAMPLE_COMMON_DSP_H__
