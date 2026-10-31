#ifndef __SAMPLE_COMMON_VENC_DEMO_H__
#define __SAMPLE_COMMON_VENC_DEMO_H__
#include "vpu_config.h"
#include "vpu_chn_config.h"
#include "enc_config.h"
#include "jpeg_config.h"
#include "bgm_config.h"
#include "vpu_bind_info.h"
#include "isp_config.h"

#define VENC_DEMO_CHN 0
#ifndef FH_APP_JPEG_FPS
#define FH_APP_JPEG_FPS 1
#endif // !FH_APP_JPEG_FPS

FH_SINT32 sample_fh_venc_start(FH_SINT32 grp_id);

FH_SINT32 sample_fh_venc_stop(FH_SINT32 grp_id);

#endif /* __SAMPLE_COMMON_VENC_DEMO_H__ */
