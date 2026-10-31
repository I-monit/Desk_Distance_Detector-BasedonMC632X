#ifndef __SAMPLE_COMMON_VOU_H__
#define __SAMPLE_COMMON_VOU_H__
#include "vou_config.h"

FH_SINT32 sample_common_vou_device_init(FH_VOID);

FH_SINT32 sample_common_video_layer_init(FH_VOID);

FH_SINT32 sample_common_video_layer_chn_init(FH_VOID);

FH_SINT32 sample_common_osd_layer_init(FH_VOID);

FH_SINT32 sample_common_vou_init(FH_VOID);

FH_SINT32 sample_common_video_layer_chn_exit(FH_VOID);

FH_SINT32 sample_common_osd_layer_exit(FH_VOID);

FH_SINT32 sample_common_video_layer_exit(FH_VOID);

FH_SINT32 sample_common_vou_device_exit(FH_VOID);

FH_SINT32 sample_common_vou_exit(FH_VOID);

#endif // __SAMPLE_COMMON_VOU_H__
