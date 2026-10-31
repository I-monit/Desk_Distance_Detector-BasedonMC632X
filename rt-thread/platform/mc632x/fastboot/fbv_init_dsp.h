#ifndef __FBV_INIT_DSP_H__
#define __FBV_INIT_DSP_H__

typedef struct grp_vpu_info
{
    FH_SINT32 enable;
    FH_SINT32 grp_id;
    FH_VPU_SET_GRP_INFO grp_info;
    FH_VPU_VI_MODE mode;
    FH_UINT32 bgm_enable;
    FH_UINT32 cpy_enable;
    FH_UINT32 sad_enable;
    FH_UINT32 bgm_ds;
} VPU_INFO;

typedef struct vpu_channel_info
{
    FH_SINT32 enable;
    FH_SINT32 channel;
    FH_UINT32 width;
    FH_UINT32 height;
    FH_UINT32 yuv_type;
    FH_UINT32 bufnum;
    FH_VPU_CROP chn_out_crop;
} VPU_CHN_INFO;

typedef struct enc_channel_info
{
    FH_SINT32 enable;
    FH_SINT32 channel;
    FH_UINT32 width;
    FH_UINT32 height;
    FH_UINT8 frame_count;
    FH_UINT8 frame_time;
    FH_UINT32 bps;
    FH_UINT32 enc_type;
    FH_UINT32 rc_type;
} ENC_INFO;

FH_SINT32 fbv_dsp_init(FH_VOID);
FH_SINT32 fbv_dsp_enc_init(FH_VOID);
FH_SINT32 change_size(FH_SINT32 grp_id);
VPU_CHN_INFO *get_vpu_chn_config(FH_SINT32 grp_id, FH_SINT32 chn_id);
VPU_INFO *get_vpu_config(FH_SINT32 grp_id);

extern VPU_CHN_INFO g_vpu_chn_info[MAX_GRP_NUM][MAX_VPU_CHN_NUM];

#endif /* __FBV_INIT_DSP_H__ */