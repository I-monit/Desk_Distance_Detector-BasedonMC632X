#ifndef __JPEG_CONFIG_H__
#define __JPEG_CONFIG_H__

#include "sample_common_media.h"
#include "enc_config.h"

#define JPEG_CHN 31
#define MAX_MJPEG_CHN 30 // MJPEG从30递减，31位JPEG通道

#define MJPEG_GRP_CODE (1 << (MAX_MJPEG_CHN / MAX_VPU_CHN_NUM))

FH_SINT32 get_jpeg_chn(FH_VOID);

ENC_INFO *get_mjpeg_config(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 jpeg_create_chn(FH_UINT32 chn_w, FH_UINT32 chn_h);

FH_SINT32 jpeg_destroy(FH_VOID);

FH_SINT32 mjpeg_create_chn(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 mjpeg_set_chn_attr(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 mjpeg_start(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 mjpeg_stop(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 mjpeg_destroy(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 mjpeg_jpeg_exit(FH_VOID);

FH_SINT32 getJpegStream(FH_VOID);

FH_SINT32 mjpeg_chn_relase(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 changeMjpegAttr(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 width, FH_SINT32 height);

#endif // __JPEG_CONFIG_H__
