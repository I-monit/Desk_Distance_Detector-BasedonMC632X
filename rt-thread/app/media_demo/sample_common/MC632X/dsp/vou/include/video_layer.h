#ifndef __OSD_LAYER_CONFIG_H__
#define __OSD_LAYER_CONFIG_H__
#include "sample_common_media.h"
#include "dsp/fh_vou_mpi.h"
#include "dsp/fh_vou_mpipara.h"

#define MAX_VIDEO_LAYER_NUM 1

#if defined DHD0_VIDEO_LAYER0_INPUT_YUV420
#define DHD0_VIDEO_LAYER0_INPUT PIXEL_FORMAT_YUV_SEMIPLANAR_420
#endif

#ifndef DHD0_VIDEO_LAYER0_INPUT
#define DHD0_VIDEO_LAYER0_INPUT PIXEL_FORMAT_YUV_SEMIPLANAR_420
#endif

typedef struct video_layer_info
{
    FH_SINT32 enable;
    VO_DEV VoDev; // 所属device
    VO_LAYER VoLayer;
    PIXEL_FORMAT_E enPixFormat;
    FH_AREA stDispRect;
    FH_UINT32 u32BufLen;
} VIDEO_LAYER_INFO;

#endif // __OSD_LAYER_CONFIG_H__
