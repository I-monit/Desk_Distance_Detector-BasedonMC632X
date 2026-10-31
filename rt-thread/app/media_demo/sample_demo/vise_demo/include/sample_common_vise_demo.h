#ifndef __SAMPLE_COMMON_VISE_DEMO_H__
#define __SAMPLE_COMMON_VISE_DEMO_H__
#include "sample_common_vise.h"
#include "vpu_chn_config.h"
#include "enc_config.h"
#include "jpeg_config.h"

#define MAX_VISE_GRP 4

FH_SINT32 sample_fh_vise_demo_start(FH_SINT32 vise_id);

FH_SINT32 sample_fh_vise_demo_stop(FH_SINT32 vise_id);

#endif /* __SAMPLE_COMMON_VISE_DEMO_H__ */
