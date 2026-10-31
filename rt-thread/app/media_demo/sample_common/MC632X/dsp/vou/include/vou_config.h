#ifndef __VOU_CONFIG_H__
#define __VOU_CONFIG_H__
#include "sample_common_media.h"
#include "dsp/fh_vou_mpi.h"
#include "dsp/fh_vou_mpipara.h"
#include "video_layer_chn.h"
#include "video_layer.h"
#include "dhd_config.h"
#include "osd_layer.h"

DHD_INFO *get_dhd_config(FH_SINT32 dhd_id);

VIDEO_LAYER_INFO *get_video_layer_config(FH_SINT32 layer_id);

VIDEO_LAYER_CHN_INFO *get_video_layer_chn_config(FH_SINT32 chn_id);

OSD_LAYER_INFO *get_osd_layer_config(FH_SINT32 layer_id);

FH_SINT32 dhd_device_init(FH_SINT32 dhd_id);

FH_SINT32 vou_video_layer_init(FH_SINT32 layer_id);

FH_SINT32 vou_video_layer_chn_init(FH_SINT32 chn_id);

FH_SINT32 vou_osd_layer_init(FH_SINT32 layer_id);

FH_SINT32 vou_close_fd(FH_VOID);

FH_SINT32 dhd_device_exit(FH_SINT32 dhd_id);

FH_SINT32 vou_video_layer_exit(FH_SINT32 layer_id);

FH_SINT32 vou_video_layer_chn_exit(FH_SINT32 chn_id);

FH_SINT32 vou_osd_layer_exit(FH_SINT32 layer_id);

FH_SINT32 vou_send_stream(FH_VOID); // TEST CODE

FH_SINT32 osd_send_stream(FH_SINT32 layer_id);

#endif // __VOU_CONFIG_H__
