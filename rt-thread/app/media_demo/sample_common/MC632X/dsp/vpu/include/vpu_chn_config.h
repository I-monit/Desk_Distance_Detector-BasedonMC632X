#ifndef __VPU_CHN_CONFIG_H__
#define __VPU_CHN_CONFIG_H__
#include "sample_common_vpu.h"
#include "vpu_chn_yuv_type.h"
#include "nna_config.h"
#include "dsp/fh_media.h"

#define NN_CHN FHVPU_AI_CHN_ID

#define CHN0_MAX_WIDTH 2560
#define CHN0_MAX_HEIGHT 1440

#define CHN1_MAX_WIDTH 864
#define CHN1_MAX_HEIGHT 480

#define CHN2_MAX_WIDTH 512
#define CHN2_MAX_HEIGHT 288

#define CHN_MIN_WIDTH 64
#define CHN_MIN_HEIGHT 64

#ifdef UVC_ENABLE
#if CHN0_MAX_WIDTH >= 3840 && CHN0_MAX_HEIGHT >= 2160
#define _800W_SUPPORT
#elif CHN0_MAX_WIDTH >= 2560 && CHN0_MAX_HEIGHT >= 1944
#define _500W_SUPPORT
#endif

#if !defined CH0_LPBUF_ENABLE_G0 && !defined CH0_LPBUF_ENABLE_G1 && !defined CH0_LPBUF_ENABLE_G2
#define NV12_SUPPORT
#if defined FH_CH0_USING_YUYV_G0 || defined FH_CH0_USING_YUYV_G1 || defined FH_CH0_USING_YUYV_G2 || \
    defined FH_CH1_USING_YUYV_G0 || defined FH_CH1_USING_YUYV_G1 || defined FH_CH1_USING_YUYV_G2 || \
    defined FH_CH2_USING_YUYV_G0 || defined FH_CH2_USING_YUYV_G1 || defined FH_CH2_USING_YUYV_G2 || \
    defined FH_CH3_USING_YUYV_G0 || defined FH_CH3_USING_YUYV_G1 || defined FH_CH3_USING_YUYV_G2
#define YUY2_SUPPORT
#else
#define NV12_TO_YUY2
#define YUY2_SUPPORT
#endif

#endif

#define UVC1_MAX_W 1920
#define UVC1_MAX_H 1080

#define UVC2_MAX_W 1280
#define UVC2_MAX_H 720
#endif // UVC_ENABLE

typedef struct vpu_channel_info
{
    FH_SINT32 channel;
    FH_SINT32 enable;
    FH_UINT32 width;
    FH_UINT32 height;
    FH_UINT32 max_width;
    FH_UINT32 max_height;
    FH_UINT32 yuv_type;
    FH_UINT32 bufnum;
    FH_VPU_CROP chn_out_crop;
} VPU_CHN_INFO;

/// VPU 输出模式
typedef enum
{
    NV12 = 1, // NV12 | [ ]
    RGB888,   // RGB888 | [ ]
    RRGGBB,   // RRGGBB | [ ]
} VPU_CHN_OUT_TYPE;

VPU_CHN_INFO *get_vpu_chn_config(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_UINT32 get_vpu_chn_w(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_UINT32 get_vpu_chn_h(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 vpu_chn_init(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 get_vpu_chn_blk_size(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_UINT32 *blk_size);

FH_SINT32 get_vpu_chn_blk_size_with_wh(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 width, FH_SINT32 height, FH_UINT32 *blk_size);

FH_SINT32 lock_vpu_stream(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_VIDEO_FRAME *pstVpuframeinfo, FH_UINT32 timeout_ms, FH_UINT32 *handle_lock, FH_UINT32 is_norpt);

FH_SINT32 unlock_vpu_stream(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_UINT32 handle_lock, FH_VIDEO_FRAME *pstVpuframeinfo);

FH_SINT32 set_vpu_chn_rotate(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 rotate);

FH_SINT32 checkVpuChnWH(FH_SINT32 chn_id, FH_SINT32 width, FH_SINT32 height);

FH_SINT32 vpu_chn_close(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 vpu_chn_destroy(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 vpu_chn_relase(FH_SINT32 grp_id, FH_SINT32 chn_id);

FH_SINT32 changeVpuChnAttr(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_VPU_VO_MODE mode, FH_SINT32 width, FH_SINT32 height);

FH_SINT32 getVPUStreamToNN(FH_VIDEO_FRAME chnStr, NN_INPUT_DATA *nn_input_data);

FH_SINT32 vpuFPSControl(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 fps);

#endif // __VPU_CHN_CONFIG_H__
