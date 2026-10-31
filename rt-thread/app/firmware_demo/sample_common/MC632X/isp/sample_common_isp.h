#ifndef __SAMPLE_COMMON_ISP_H__
#define __SAMPLE_COMMON_ISP_H__
#include "sample_common_media.h"
#include "sample_common_vicap.h"
#include "isp_config.h"
#include "isp_bind_info.h"

FH_SINT32 sample_common_isp_init(FH_VOID);

FH_SINT32 sample_common_isp_exit(FH_VOID);

FH_SINT32 sample_common_isp_start(FH_VOID);

FH_SINT32 sample_common_isp_bind_vpu(FH_VOID);

FH_SINT32 sample_common_isp_get_frame_blk_size(FH_SINT32 grp_id, FH_UINT32 *blk_size);

#endif // __SAMPLE_COMMON_ISP_H__
