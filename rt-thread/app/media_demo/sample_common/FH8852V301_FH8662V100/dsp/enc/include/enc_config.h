#ifndef __ENC_CONFIG_H__
#define __ENC_CONFIG_H__
#include "sample_common_media.h"
#include "enc_type.h"
#include "enc_rc_mode.h"

#define H264_BREATH_IP_QPDELTA 5
#define H264_BREATH_I_BITPROP 15
#define H264_BREATH_P_BITPROP 1

#define H265_BREATH_IP_QPDELTA 3
#define H265_BREATH_I_BITPROP 10
#define H265_BREATH_P_BITPROP 1

// 公共缓冲buffer设置,UVC使用独立缓冲，IPC使用公共缓冲
#ifdef UVC_ENABLE
#define H264_COMMON_BUFFER_SIZE 0
#define JPEG_COMMON_BUFFER_SIZE 0
#define MJPEG_COMMON_BUFFER_SIZE 0
#define JPEG_SINGLE_BUFFER 1 * 1024 * 1024 // 独立缓冲中JPEG缓冲设置为1MByte
#define MJPEG_SINGLE_BUFFER 4 * 1024 * 1024 // 独立缓冲中MJPEG缓冲设置为2MByte
#define H264_SINGLE_BUFFER 4 * 1024 * 1024 // 独立缓冲中H264,H265缓冲设置为2MByte
#else
#define H264_COMMON_BUFFER_SIZE 1024 * 1024 * 2 // 2MByte
#define JPEG_COMMON_BUFFER_SIZE 1024 * 1024 * 1 // 1MByte
#define MJPEG_COMMON_BUFFER_SIZE 1024 * 1024 * 2  // 2MByte
#endif

typedef struct enc_channel_info
{
    FH_SINT32 channel;
    FH_SINT32 enable;
    FH_UINT32 width;
    FH_UINT32 height;
    FH_UINT32 max_width;
    FH_UINT32 max_height;
    FH_UINT32 bps;
    FH_UINT32 enc_type;
    FH_UINT32 rc_type;
    FH_UINT16 breath_on;
    FH_UINT8 frame_count;
    FH_UINT8 frame_time;
    FH_UINT8 run_status;
} ENC_INFO;

ENC_INFO *get_enc_config(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 enc_mod_set(FH_VOID);

FH_SINT32 enc_create_chn(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 enc_set_chn_attr(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 set_enc_rotate(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 rotate);

FH_SINT32 enc_start(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 enc_stop(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 enc_destroy(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 enc_get_chn_status(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_CHN_STATUS *chnstat);

FH_SINT32 enc_get_common_stream(FH_UINT32 request_type, FH_VENC_STREAM *stream);

FH_SINT32 enc_get_chn_stream(FH_UINT32 enc_chn, FH_VENC_STREAM *stream);

FH_SINT32 enc_release_stream(FH_VENC_STREAM *stream);

FH_SINT32 getAllEncStreamToDmc(FH_VOID);

FH_SINT32 enc_chn_relase(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 changeEncAttr(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_VENC_TYPE encType, FH_VENC_RC_MODE rcType, FH_SINT32 width, FH_SINT32 height);

FH_SINT32 getENCStreamBsp(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_UINT32 *bsp);

#endif // __ENC_CONFIG_H__
