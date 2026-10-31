#include "fbv_common.h"
static VPU_INFO g_vpu_grp_infos[MAX_GRP_NUM] = {
    {
        .enable = 0,
#ifdef VIDEO_GRP0
        .enable = 1,
        .grp_id = 0,
        .mode = VPU_MODE_ISP,
#endif
    },
    {
        .enable = 0,
#ifdef VIDEO_GRP1
        .enable = 1,
        .grp_id = 0,
        .mode = VPU_MODE_ISP,
#endif
    },
};

VPU_CHN_INFO g_vpu_chn_info[MAX_GRP_NUM][MAX_VPU_CHN_NUM] = {
    {
        {
            .enable = 0,
#ifdef CH0_ENABLE_G0
            .enable = 1,
            .channel = 0,
            .width = VPU_WIDTH_CH0_G0,
            .height = VPU_HEIGHT_CH0_G0,
            .yuv_type = VPU_VOMODE_SCAN,
            .bufnum = VPU_BUF_NUM_CH0_G0,
#endif
        },
        {
            .enable = 0,
#ifdef CH1_ENABLE_G0
            .enable = 1,
            .channel = 1,
            .width = VPU_WIDTH_CH1_G0,
            .height = VPU_HEIGHT_CH1_G0,
            .yuv_type = VPU_VOMODE_SCAN,
            .bufnum = VPU_BUF_NUM_CH1_G0,
#endif
        },
        {
            .enable = 0,
#ifdef CH2_ENABLE_G0
            .enable = 1,
            .channel = 2,
            .width = VPU_WIDTH_CH2_G0,
            .height = VPU_HEIGHT_CH2_G0,
            .yuv_type = VPU_VOMODE_RGB888,
            .bufnum = VPU_BUF_NUM_CH2_G0,
#endif
        },
    },
    {
        {
            .enable = 0,
#ifdef CH0_ENABLE_G1
            .enable = 1,
            .channel = 0,
            .width = VPU_WIDTH_CH0_G1,
            .height = VPU_HEIGHT_CH0_G1,
            .yuv_type = VPU_VOMODE_SCAN,
            .bufnum = VPU_BUF_NUM_CH0_G1,
#endif
        },
        {
            .enable = 0,
#ifdef CH1_ENABLE_G1
            .enable = 1,
            .channel = 1,
            .width = VPU_WIDTH_CH1_G1,
            .height = VPU_HEIGHT_CH1_G1,
            .yuv_type = VPU_VOMODE_SCAN,
            .bufnum = VPU_BUF_NUM_CH1_G1,
#endif
        },
        {
            .enable = 0,
#ifdef CH2_ENABLE_G1
            .enable = 1,
            .channel = 2,
            .width = VPU_WIDTH_CH2_G1,
            .height = VPU_HEIGHT_CH2_G1,
            .yuv_type = VPU_VOMODE_RGB888,
            .bufnum = VPU_BUF_NUM_CH2_G1,
#endif
        },
    },
};

static ENC_INFO g_enc_chn_infos[MAX_GRP_NUM][MAX_VPU_CHN_NUM] = {
    {

        {
            .enable = 0,
#ifdef VENC_ENABLE_G0_CH0
            .enable = 1,
            .channel = 0,
            .frame_count = CH0_FRAME_COUNT_G0,
            .frame_time = CH0_FRAME_TIME_G0,
            .bps = CH0_BIT_RATE_G0 * 1024,
            .enc_type = CH0_ENC_TYPE_G0,
            .rc_type = CH0_RC_TYPE_G0,
#endif
        },
        {
            .enable = 0,
#ifdef VENC_ENABLE_G0_CH1
            .enable = 1,
            .channel = 1,
            .frame_count = CH1_FRAME_COUNT_G0,
            .frame_time = CH1_FRAME_TIME_G0,
            .bps = CH1_BIT_RATE_G0 * 1024,
            .enc_type = CH1_ENC_TYPE_G0,
            .rc_type = CH1_RC_TYPE_G0,
#endif
        },

        {
            .enable = 0,
#ifdef VENC_ENABLE_G0_CH2
            .enable = 1,
            .channel = 2,
            .frame_count = CH2_FRAME_COUNT_G0,
            .frame_time = CH2_FRAME_TIME_G0,
            .bps = CH2_BIT_RATE_G0 * 1024,
            .enc_type = CH2_ENC_TYPE_G0,
            .rc_type = CH2_RC_TYPE_G0,
#endif
        },
    },
    {
        {
            .enable = 0,
#ifdef VENC_ENABLE_G1_CH0
            .enable = 1,
            .channel = 0,
            .frame_count = CH0_FRAME_COUNT_G1,
            .frame_time = CH0_FRAME_TIME_G1,
            .bps = CH0_BIT_RATE_G1 * 1024,
            .enc_type = CH0_ENC_TYPE_G1,
            .rc_type = CH0_RC_TYPE_G1,
#endif
        },
        {
            .enable = 0,
#ifdef VENC_ENABLE_G1_CH1
            .enable = 1,
            .channel = 1,
            .frame_count = CH1_FRAME_COUNT_G1,
            .frame_time = CH1_FRAME_TIME_G1,
            .bps = CH1_BIT_RATE_G1 * 1024,
            .enc_type = CH1_ENC_TYPE_G1,
            .rc_type = CH1_RC_TYPE_G1,
#endif
        },

        {
            .enable = 0,
#ifdef VENC_ENABLE_G1_CH2
            .enable = 1,
            .channel = 2,
            .frame_count = CH2_FRAME_COUNT_G1,
            .frame_time = CH2_FRAME_TIME_G1,
            .bps = CH2_BIT_RATE_G1 * 1024,
            .enc_type = CH2_ENC_TYPE_G1,
            .rc_type = CH2_RC_TYPE_G1,
#endif
        },
    },
};

extern FH_SINT32 vpu_write_proc(FH_CHAR *s);
extern FH_SINT32 isp_write_proc(FH_CHAR *s);
extern FH_SINT32 enc_write_proc(FH_CHAR *s);
static FH_SINT32 fbv_dsp_system_init(FH_VOID)
{
    FH_SINT32 ret = 0;
    FH_UINT32 blk_size = 0;
    FH_SINT32 grp_id = 0;
    FH_SINT32 chn_id = 0;
    FH_SINT32 poolid = 0;
    VB_CONF_S stVbConf;

    fh_media_process_module_init();
    fh_vpu_module_init();
    fh_isp_module_init();
    fh_enc_module_init();
    fh_nna_module_init();

    FH_VB_Exit();

    rt_memset(&stVbConf, 0, sizeof(VB_CONF_S));

    vpu_write_proc("swtoms_100");
    isp_write_proc("binddelay_0");
    enc_write_proc("maxencnum_32");
    enc_write_proc("procmode_1");

    ret = FH_SYS_Init();
    FBV_FUNC_ERROR_PRT("FH_SYS_Init", ret);

    ret = API_ISP_Open();
    FBV_FUNC_ERROR_PRT("API_ISP_Open", ret);

    stVbConf.u32MaxPoolCnt = MAX_GRP_NUM * MAX_VPU_CHN_NUM + MAX_GRP_NUM * 2;
    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        for (chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++)
        {
            if (g_vpu_chn_info[grp_id][chn_id].enable && !g_vpu_chn_info[grp_id][chn_id].bufnum)
            {
                ret = FH_VPSS_GetFrameBlkSize(g_vpu_chn_info[grp_id][chn_id].width, g_vpu_chn_info[grp_id][chn_id].height, 1 << g_vpu_chn_info[grp_id][chn_id].yuv_type, &blk_size);
                FBV_FUNC_ERROR_PRT("FH_VPSS_GetFrameBlkSize", ret);
                if (blk_size)
                {
                    stVbConf.astCommPool[poolid].u32BlkSize = blk_size;
                    stVbConf.astCommPool[poolid].u32BlkCnt = 3;
                    poolid++;
                }
            }
        }
    }

    ret = FH_VB_SetConf(&stVbConf);
    FBV_FUNC_ERROR_PRT("FH_VB_SetConf", ret);

    ret = FH_VB_Init();
    FBV_FUNC_ERROR_PRT("FH_VB_Init", ret);

    return ret;
}

static FH_SINT32 vpu_init(FH_SINT32 grp_id)
{
    FH_SINT32 ret = 0;
    FH_VPU_SIZE vi_pic;
    rt_memset(&vi_pic, 0, sizeof(FH_VPU_SIZE));

    g_vpu_grp_infos[grp_id].grp_info.vi_max_size.u32Width = fbv_isp_w(grp_id);
    g_vpu_grp_infos[grp_id].grp_info.vi_max_size.u32Height = fbv_isp_h(grp_id);
    g_vpu_grp_infos[grp_id].grp_info.ycmean_en = 1;
    g_vpu_grp_infos[grp_id].grp_info.ycmean_ds = 16;

    ret = FH_VPSS_CreateGrp(grp_id, &g_vpu_grp_infos[grp_id].grp_info);
    FBV_FUNC_ERROR_PRT("FH_VPSS_GetFrameBlkSize", ret);

#ifdef ENBALE_SMALL_TO_BIG
    vi_pic.vi_size.u32Width = fbv_isp_w_pre(grp_id);
    vi_pic.vi_size.u32Height = fbv_isp_h_pre(grp_id);
    vi_pic.crop_area.crop_en = 0;
    vi_pic.crop_area.vpu_crop_area.u32X = 0;
    vi_pic.crop_area.vpu_crop_area.u32Y = 0;
    vi_pic.crop_area.vpu_crop_area.u32Width = 0;
    vi_pic.crop_area.vpu_crop_area.u32Height = 0;
#else
    vi_pic.vi_size.u32Width = fbv_isp_w(grp_id);
    vi_pic.vi_size.u32Height = fbv_isp_h(grp_id);
    vi_pic.crop_area.crop_en = 0;
    vi_pic.crop_area.vpu_crop_area.u32X = 0;
    vi_pic.crop_area.vpu_crop_area.u32Y = 0;
    vi_pic.crop_area.vpu_crop_area.u32Width = 0;
    vi_pic.crop_area.vpu_crop_area.u32Height = 0;
#endif

    ret = FH_VPSS_SetViAttr(grp_id, &vi_pic);
    FBV_FUNC_ERROR_PRT("FH_VPSS_SetViAttr", ret);

    ret = FH_VPSS_Enable(grp_id, g_vpu_grp_infos[grp_id].mode);
    FBV_FUNC_ERROR_PRT("FH_VPSS_Enable", ret);
    return 0;
}

static FH_SINT32 fbv_dsp_vpu_init(FH_VOID)
{
    FH_SINT32 ret = 0;
    FH_SINT32 grp_id = 0;
    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        if (g_vpu_grp_infos[grp_id].enable)
        {
            ret = vpu_init(grp_id);
            FBV_FUNC_ERROR_PRT("vpu_init", ret);
        }
    }
    return 0;
}

FH_SINT32 change_size(FH_SINT32 grp_id)
{
    FH_SINT32 ret = 0;
    FH_SINT32 chn_id = 0;
    FH_VPU_CHN_CONFIG chn_attr;
    FH_VPU_SIZE vi_size;

    fbv_isp_pause(grp_id);

    if (isp_change_resolution(grp_id))
    {
        return -1;
    }

    for (chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++)
    {
        if (g_vpu_chn_info[grp_id][chn_id].enable)
        {
            vi_size.vi_size.u32Width = fbv_isp_w(grp_id);
            vi_size.vi_size.u32Height = fbv_isp_h(grp_id);
            vi_size.crop_area.crop_en = 0;
            vi_size.crop_area.vpu_crop_area.u32X = 0;
            vi_size.crop_area.vpu_crop_area.u32Y = 0;
            vi_size.crop_area.vpu_crop_area.u32Width = 0;
            vi_size.crop_area.vpu_crop_area.u32Height = 0;

            ret = FH_VPSS_SetViAttr(grp_id, &vi_size);
            FBV_FUNC_ERROR_PRT("FH_VPSS_SetViAttr", ret);

            chn_attr.vpu_chn_size.u32Width = g_vpu_chn_info[grp_id][chn_id].width;
            chn_attr.vpu_chn_size.u32Height = g_vpu_chn_info[grp_id][chn_id].height;
            chn_attr.crop_area.crop_en = 0;
            chn_attr.crop_area.vpu_crop_area.u32X = 0;
            chn_attr.crop_area.vpu_crop_area.u32Y = 0;
            chn_attr.crop_area.vpu_crop_area.u32Width = 0;
            chn_attr.crop_area.vpu_crop_area.u32Height = 0;
            chn_attr.stride = 0;
            chn_attr.offset = 0;
            chn_attr.depth = 1;
            ret = FH_VPSS_SetChnAttr(grp_id, chn_id, &chn_attr);
            FBV_FUNC_ERROR_PRT("FH_VPSS_SetChnAttr", ret);

            ret = FH_VPSS_SetVOMode(grp_id, chn_id, g_vpu_chn_info[grp_id][chn_id].yuv_type);
            FBV_FUNC_ERROR_PRT("FH_VPSS_SetVOMode", ret);

            ret = FH_VPSS_OpenChn(grp_id, chn_id);
            FBV_FUNC_ERROR_PRT("FH_VPSS_OpenChn", ret);

            ret = FH_VPSS_Enable(grp_id, g_vpu_grp_infos[grp_id].mode);
            FBV_FUNC_ERROR_PRT("FH_VPSS_Enable", ret);
        }
    }

    fbv_isp_resume(grp_id);

    return 0;
}

static FH_SINT32 vpu_chn_init(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = 0;
    FH_VPU_CHN_INFO chn_info;
    FH_VPU_CHN_CONFIG chn_attr;

    if (chn_id == 0)
    {
        chn_info.bgm_enable = 1;
        chn_info.cpy_enable = 1;
        chn_info.sad_enable = 1;
        chn_info.bgm_ds = 8;
    }
    else
    {
        chn_info.bgm_enable = 0;
        chn_info.cpy_enable = 0;
        chn_info.sad_enable = 0;
        chn_info.bgm_ds = 0;
    }

    chn_info.chn_max_size.u32Width = g_vpu_chn_info[grp_id][chn_id].width;
    chn_info.chn_max_size.u32Height = g_vpu_chn_info[grp_id][chn_id].height;
    chn_info.out_mode = g_vpu_chn_info[grp_id][chn_id].yuv_type;
    chn_info.support_mode = 1 << chn_info.out_mode;
    chn_info.bufnum = g_vpu_chn_info[grp_id][chn_id].bufnum;
    chn_info.max_stride = 0;

    ret = FH_VPSS_CreateChn(grp_id, chn_id, &chn_info);
    FBV_FUNC_ERROR_PRT("FH_VPSS_CreateChn", ret);
    if (chn_id == 2)
    {
        chn_attr.vpu_chn_size.u32Width = g_vpu_chn_info[grp_id][chn_id].width;
        chn_attr.vpu_chn_size.u32Height = g_vpu_chn_info[grp_id][chn_id].height;
        chn_attr.crop_area.crop_en = 0;
        chn_attr.crop_area.vpu_crop_area.u32X = 0;
        chn_attr.crop_area.vpu_crop_area.u32Y = 0;
        chn_attr.crop_area.vpu_crop_area.u32Width = 0;
        chn_attr.crop_area.vpu_crop_area.u32Height = 0;
        chn_attr.stride = 0;
        chn_attr.offset = 0;
        chn_attr.depth = 1;
    }
    else
    {
#ifdef ENBALE_SMALL_TO_BIG
        chn_attr.vpu_chn_size.u32Width = fbv_isp_w_pre(grp_id);
        chn_attr.vpu_chn_size.u32Height = fbv_isp_h_pre(grp_id);
        chn_attr.crop_area.crop_en = 0;
        chn_attr.crop_area.vpu_crop_area.u32X = 0;
        chn_attr.crop_area.vpu_crop_area.u32Y = 0;
        chn_attr.crop_area.vpu_crop_area.u32Width = 0;
        chn_attr.crop_area.vpu_crop_area.u32Height = 0;
        chn_attr.stride = 0;
        chn_attr.offset = 0;
        chn_attr.depth = 0;
#else
        chn_attr.vpu_chn_size.u32Width = g_vpu_chn_info[grp_id][chn_id].width;
        chn_attr.vpu_chn_size.u32Height = g_vpu_chn_info[grp_id][chn_id].height;
        chn_attr.crop_area.crop_en = 0;
        chn_attr.crop_area.vpu_crop_area.u32X = 0;
        chn_attr.crop_area.vpu_crop_area.u32Y = 0;
        chn_attr.crop_area.vpu_crop_area.u32Width = 0;
        chn_attr.crop_area.vpu_crop_area.u32Height = 0;
        chn_attr.stride = 0;
        chn_attr.offset = 0;
        chn_attr.depth = 1;
#endif
    }
    ret = FH_VPSS_SetChnAttr(grp_id, chn_id, &chn_attr);
    FBV_FUNC_ERROR_PRT("FH_VPSS_SetChnAttr", ret);

    ret = FH_VPSS_SetVOMode(grp_id, chn_id, g_vpu_chn_info[grp_id][chn_id].yuv_type);
    FBV_FUNC_ERROR_PRT("FH_VPSS_SetVOMode", ret);

    ret = FH_VPSS_OpenChn(grp_id, chn_id);
    FBV_FUNC_ERROR_PRT("FH_VPSS_OpenChn", ret);

    return ret;
}

static FH_SINT32 fbv_dsp_vpu_chn_init(FH_VOID)
{
    FH_SINT32 ret = 0;
    FH_SINT32 grp_id = 0;
    FH_SINT32 chn_id = 0;

    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        if (g_vpu_grp_infos[grp_id].enable)
        {
            for (chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++)
            {
                if (g_vpu_chn_info[grp_id][chn_id].enable)
                {
                    ret = vpu_chn_init(grp_id, chn_id);
                    FBV_FUNC_ERROR_PRT("vpu_chn_init", ret);
                }
            }
        }
    }

    return ret;
}

FH_SINT32 fbv_enc_mod_set(FH_VOID)
{
    FH_SINT32 ret = 0;
    FH_VENC_MOD_PARAM h264_param;
    FH_VENC_MOD_PARAM jpeg_param;
    FH_VENC_MOD_PARAM mjpeg_param;

    h264_param.type = FH_STREAM_H264;
    ret = FH_VENC_GetModParam(&h264_param);
    FBV_FUNC_ERROR_PRT("FH_VENC_GetModParam", ret);

    h264_param.stm_size = H264_COMMON_BUFFER_SIZE;

    h264_param.max_stm_num = 128;
    h264_param.stm_cache = 0;

    ret = FH_VENC_SetModParam(&h264_param);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetModParam", ret);

    jpeg_param.max_stm_num = 128;
    jpeg_param.stm_cache = 0;

    return ret;
}

static FH_SINT32 dsp_enc_init(FH_SINT32 grpid, FH_SINT32 chn_id)
{
    FH_SINT32 ret;
    FH_VENC_CHN_PARAM pstchnparam;
    ret = FH_VENC_GetChnParam(grpid * MAX_VPU_CHN_NUM + chn_id, &pstchnparam);
    FBV_FUNC_ERROR_PRT("FH_VENC_GetChnParam", ret);
    pstchnparam.stm_mode = FH_STM_PUB_BUF;
    ret = FH_VENC_SetChnParam(grpid * MAX_VPU_CHN_NUM + chn_id, &pstchnparam);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetChnParam", ret);

    FH_VENC_CHN_CAP cfg_vencmem;
    FH_VENC_CHN_CONFIG cfg_param;

    rt_memset(&cfg_vencmem, 0, sizeof(FH_VENC_CHN_CAP));
    rt_memset(&cfg_param, 0, sizeof(FH_VENC_CHN_CONFIG));

    cfg_vencmem.support_type = g_enc_chn_infos[grpid][chn_id].enc_type;
    cfg_vencmem.max_size.u32Width = g_vpu_chn_info[grpid][chn_id].width;
    cfg_vencmem.max_size.u32Height = g_vpu_chn_info[grpid][chn_id].height;
    ret = FH_VENC_CreateChn(grpid * MAX_VPU_CHN_NUM + chn_id, &cfg_vencmem);
    FBV_FUNC_ERROR_PRT("FH_VENC_CreateChn", ret);

    cfg_param.chn_attr.enc_type = g_enc_chn_infos[grpid][chn_id].enc_type;

    if (g_enc_chn_infos[grpid][chn_id].enc_type == FH_NORMAL_H265)
    {
        cfg_param.chn_attr.h265_attr.profile = H265_PROFILE_MAIN;
        cfg_param.chn_attr.h265_attr.i_frame_intterval = 50;
        cfg_param.chn_attr.h265_attr.size.u32Width = g_vpu_chn_info[grpid][chn_id].width;
        cfg_param.chn_attr.h265_attr.size.u32Height = g_vpu_chn_info[grpid][chn_id].height;

        cfg_param.rc_attr.rc_type = FH_RC_H265_VBR;
        cfg_param.rc_attr.h265_vbr.init_qp = 35;
        cfg_param.rc_attr.h265_vbr.bitrate = g_enc_chn_infos[grpid][chn_id].bps;
        cfg_param.rc_attr.h265_vbr.ImaxQP = 51;
        cfg_param.rc_attr.h265_vbr.IminQP = 28;
        cfg_param.rc_attr.h265_vbr.PmaxQP = 51;
        cfg_param.rc_attr.h265_vbr.PminQP = 28;
        cfg_param.rc_attr.h265_vbr.maxrate_percent = 200;
        cfg_param.rc_attr.h265_vbr.IFrmMaxBits = 0;
        cfg_param.rc_attr.h265_vbr.IP_QPDelta = 3;
        cfg_param.rc_attr.h265_vbr.I_BitProp = 5;
        cfg_param.rc_attr.h265_vbr.P_BitProp = 1;
        cfg_param.rc_attr.h265_vbr.fluctuate_level = 0;
        cfg_param.rc_attr.h265_vbr.FrameRate.frame_count = g_enc_chn_infos[grpid][chn_id].frame_count;
        cfg_param.rc_attr.h265_vbr.FrameRate.frame_time = g_enc_chn_infos[grpid][chn_id].frame_time;
    }
    if (g_enc_chn_infos[grpid][chn_id].enc_type == FH_NORMAL_H264)
    {
        cfg_param.chn_attr.h264_attr.profile = H264_PROFILE_MAIN;
        cfg_param.chn_attr.h264_attr.i_frame_intterval = 50;
        cfg_param.chn_attr.h264_attr.size.u32Width = g_vpu_chn_info[grpid][chn_id].width;
        cfg_param.chn_attr.h264_attr.size.u32Height = g_vpu_chn_info[grpid][chn_id].height;

        cfg_param.rc_attr.rc_type = FH_RC_H264_VBR;
        cfg_param.rc_attr.h264_vbr.init_qp = 35;
        cfg_param.rc_attr.h264_vbr.bitrate = g_enc_chn_infos[grpid][chn_id].bps;
        cfg_param.rc_attr.h264_vbr.ImaxQP = 50;
        cfg_param.rc_attr.h264_vbr.IminQP = 28;
        cfg_param.rc_attr.h264_vbr.PmaxQP = 50;
        cfg_param.rc_attr.h264_vbr.PminQP = 28;
        cfg_param.rc_attr.h264_vbr.maxrate_percent = 200;
        cfg_param.rc_attr.h264_vbr.IFrmMaxBits = 0;
        cfg_param.rc_attr.h264_vbr.IP_QPDelta = 3;
        cfg_param.rc_attr.h264_vbr.I_BitProp = 5;
        cfg_param.rc_attr.h264_vbr.P_BitProp = 1;
        cfg_param.rc_attr.h264_vbr.fluctuate_level = 0;
        cfg_param.rc_attr.h264_vbr.FrameRate.frame_count = g_enc_chn_infos[grpid][chn_id].frame_count;
        cfg_param.rc_attr.h264_vbr.FrameRate.frame_time = g_enc_chn_infos[grpid][chn_id].frame_time;
    }
    ret = FH_VENC_SetChnAttr(grpid * MAX_VPU_CHN_NUM + chn_id, &cfg_param);
    FBV_FUNC_ERROR_PRT("FH_VENC_SetChnAttr", ret);

    ret = FH_VENC_StartRecvPic(grpid * MAX_VPU_CHN_NUM + chn_id);
    FBV_FUNC_ERROR_PRT("FH_VENC_StartRecvPic", ret);

    FH_BIND_INFO src, dst;
    src.obj_id = FH_OBJ_VPU_VO;
    src.dev_id = grpid;
    src.chn_id = chn_id;

    dst.obj_id = FH_OBJ_ENC;
    dst.dev_id = 0;
    dst.chn_id = grpid * MAX_VPU_CHN_NUM + chn_id;

    ret = FH_SYS_UnBindbyDst(dst);
    FBV_FUNC_ERROR_PRT("FH_SYS_UnBindbyDst", ret);

    ret = FH_SYS_Bind(src, dst);
    FBV_FUNC_ERROR_PRT("FH_SYS_Bind", ret);

    return ret;
}

FH_SINT32 fbv_dsp_enc_init(FH_VOID)
{
    FH_SINT32 ret = 0;
    FH_SINT32 grp_id = 0;
    FH_SINT32 chn_id = 0;
    fbv_enc_mod_set();
    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        if (g_vpu_grp_infos[grp_id].enable)
        {
            for (chn_id = 0; chn_id < MAX_VPU_CHN_NUM; chn_id++)
            {
                if (g_enc_chn_infos[grp_id][chn_id].enable)
                {
                    ret = dsp_enc_init(grp_id, chn_id);
                    FBV_FUNC_ERROR_PRT("dsp_enc_init", ret);
                }
            }
        }
    }
    return ret;
}

FH_SINT32 fbv_dsp_init(FH_VOID)
{
    FH_SINT32 ret = 0;

    ret = fbv_dsp_system_init();
    FBV_FUNC_ERROR_PRT("fbv_dsp_system_init", ret);

    ret = fbv_dsp_vpu_init();
    FBV_FUNC_ERROR_PRT("fbv_dsp_vpu_init", ret);

    ret = fbv_dsp_vpu_chn_init();
    FBV_FUNC_ERROR_PRT("fbv_dsp_vpu_chn_init", ret);

    return ret;
}
