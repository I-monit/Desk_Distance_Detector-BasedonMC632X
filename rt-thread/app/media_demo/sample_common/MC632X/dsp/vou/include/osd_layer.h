#ifndef __VIDEO_LAYER_CONFIG_H__
#define __VIDEO_LAYER_CONFIG_H__
#include "sample_common_media.h"
#include "dsp/fh_vou_mpi.h"
#include "dsp/fh_vou_mpipara.h"

#define MAX_OSD_LAYER_NUM 1

#ifdef DEVICE_DHD0_OSD0_FILE_TYPE_ARGB8888
#define OSD_FILE_FORMAT PIXEL_FORMAT_RGB_8888
#elif defined DEVICE_DHD0_OSD0_FILE_TYPE_ARGB4444
#define OSD_FILE_FORMAT PIXEL_FORMAT_RGB_4444
#elif defined DEVICE_DHD0_OSD0_FILE_TYPE_ARGB1555
#define OSD_FILE_FORMAT PIXEL_FORMAT_RGB_1555
#endif

#ifndef OSD_FILE_FORMAT
#define OSD_FILE_FORMAT PIXEL_FORMAT_RGB_8888
#endif

typedef struct osd_layer_info
{
    FH_SINT32 enable;
    VO_DEV VoDev; // 所属device
    GRAPHIC_LAYER GraphicLayer;
    FH_POINT stPosOsd;
    FH_SIZE stSizeOsd;
    ALPHA_S stAlphaOsd;
    FH_CHAR osd_file[50];
    PIXEL_FORMAT_E enPixelFormat;
} OSD_LAYER_INFO;

#endif // __VIDEO_LAYER_CONFIG_H__
