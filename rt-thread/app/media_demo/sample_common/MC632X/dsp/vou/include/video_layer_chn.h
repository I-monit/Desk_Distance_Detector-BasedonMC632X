#ifndef __VIDEO_LAYER_CHN_CONFIG_H__
#define __VIDEO_LAYER_CHN_CONFIG_H__
#include "sample_common_media.h"
#include "dsp/fh_vou_mpi.h"
#include "dsp/fh_vou_mpipara.h"

#define MAX_VIDEO_LAYER_CHN_NUM 1

typedef struct video_layer_chn_info
{
    FH_SINT32 enable;
    VO_LAYER VoLayer; // 所属layer
    VO_CHN VoChn;
    FH_AREA stDispRect;
    // vou_bind_info; 见vpu/src/vpu_bind_info.c
} VIDEO_LAYER_CHN_INFO;

#endif // __VIDEO_LAYER_CHN_CONFIG_H__
