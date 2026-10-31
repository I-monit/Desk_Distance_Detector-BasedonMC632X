#include "vpu_chn_config.h"

static char *model_name = MODEL_TAG_VPU_CHN;

static VPU_CHN_INFO g_vpu_chn_infos[MAX_GRP_NUM][MAX_VPU_CHN_NUM] = {
    {
        {
            .enable = 0,
#ifdef CH0_ENABLE_G0
            .enable = 1,
            .channel = 0,
            .width = CH0_WIDTH_G0,
            .height = CH0_HEIGHT_G0,
#if (CH0_MAX_WIDTH_G0 > CH0_WIDTH_G0)
            .max_width = CH0_MAX_WIDTH_G0,
#else
            .max_width = CH0_WIDTH_G0,
#endif
#if (CH0_MAX_HEIGHT_G0 > CH0_HEIGHT_G0)
            .max_height = CH0_MAX_HEIGHT_G0,
#else
            .max_height = CH0_HEIGHT_G0,
#endif
            .yuv_type = CH0_YUV_TYPE_G0,
            .bufnum = CH0_BUFNUM_G0,
#endif /*CH0_WIDTH_G0*/
        },

        {
            .enable = 0,
#ifdef CH1_ENABLE_G0
            .enable = 1,
            .channel = 1,
            .width = CH1_WIDTH_G0,
            .height = CH1_HEIGHT_G0,
#if (CH1_MAX_WIDTH_G0 > CH1_WIDTH_G0)
            .max_width = CH1_MAX_WIDTH_G0,
#else
            .max_width = CH1_WIDTH_G0,
#endif
#if (CH1_MAX_HEIGHT_G0 > CH1_HEIGHT_G0)
            .max_height = CH1_MAX_HEIGHT_G0,
#else
            .max_height = CH1_HEIGHT_G0,
#endif
            .yuv_type = CH1_YUV_TYPE_G0,
            .bufnum = CH1_BUFNUM_G0,
#endif /*CH1_WIDTH_G0*/
        },

        {
            .enable = 0,
#ifdef CH2_ENABLE_G0
            .enable = 1,
            .channel = 2,
            .width = CH2_WIDTH_G0,
            .height = CH2_HEIGHT_G0,
#if (CH2_MAX_WIDTH_G0 > CH2_WIDTH_G0)
            .max_width = CH2_MAX_WIDTH_G0,
#else
            .max_width = CH2_WIDTH_G0,
#endif
#if (CH2_MAX_HEIGHT_G0 > CH2_HEIGHT_G0)
            .max_height = CH2_MAX_HEIGHT_G0,
#else
            .max_height = CH2_HEIGHT_G0,
#endif
            .yuv_type = CH2_YUV_TYPE_G0,
            .bufnum = CH2_BUFNUM_G0,
#endif /*CH2_WIDTH_G0*/
        },
    }, /* group 0 */

    {
        {
            .enable = 0,
#ifdef CH0_ENABLE_G1
            .enable = 1,
            .channel = 0,
            .width = CH0_WIDTH_G1,
            .height = CH0_HEIGHT_G1,
#if (CH0_MAX_WIDTH_G1 > CH0_WIDTH_G1)
            .max_width = CH0_MAX_WIDTH_G1,
#else
            .max_width = CH0_WIDTH_G1,
#endif
#if (CH0_MAX_HEIGHT_G1 > CH0_HEIGHT_G1)
            .max_height = CH0_MAX_HEIGHT_G1,
#else
            .max_height = CH0_HEIGHT_G1,
#endif
            .yuv_type = CH0_YUV_TYPE_G1,
            .bufnum = CH0_BUFNUM_G1,
#endif /*CH0_WIDTH_G1*/
        },

        {
            .enable = 0,
#ifdef CH1_ENABLE_G1
            .enable = 1,
            .channel = 1,
            .width = CH1_WIDTH_G1,
            .height = CH1_HEIGHT_G1,
#if (CH1_MAX_WIDTH_G1 > CH1_WIDTH_G1)
            .max_width = CH1_MAX_WIDTH_G1,
#else
            .max_width = CH1_WIDTH_G1,
#endif
#if (CH1_MAX_HEIGHT_G1 > CH1_HEIGHT_G1)
            .max_height = CH1_MAX_HEIGHT_G1,
#else
            .max_height = CH1_HEIGHT_G1,
#endif
            .yuv_type = CH1_YUV_TYPE_G1,
            .bufnum = CH1_BUFNUM_G1,
#endif /*CH1_WIDTH_G1*/
        },

        {
            .enable = 0,
#ifdef CH2_ENABLE_G1
            .enable = 1,
            .channel = 2,
            .width = CH2_WIDTH_G1,
            .height = CH2_HEIGHT_G1,
#if (CH2_MAX_WIDTH_G1 > CH2_WIDTH_G1)
            .max_width = CH2_MAX_WIDTH_G1,
#else
            .max_width = CH2_WIDTH_G1,
#endif
#if (CH2_MAX_HEIGHT_G1 > CH2_HEIGHT_G1)
            .max_height = CH2_MAX_HEIGHT_G1,
#else
            .max_height = CH2_HEIGHT_G1,
#endif
            .yuv_type = CH2_YUV_TYPE_G1,
            .bufnum = CH2_BUFNUM_G1,
#endif /*CH2_WIDTH_G1*/
        },
    }, /* group 1 */
};

VPU_CHN_INFO *get_vpu_chn_config(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    return &g_vpu_chn_infos[grp_id][chn_id];
}

FH_UINT32 get_vpu_chn_w(FH_SINT32 grp_id, FH_SINT32 chn_id) // TODO no chn crop,chn out need align 16
{
    return g_vpu_chn_infos[grp_id][chn_id].width;
}

FH_UINT32 get_vpu_chn_h(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    return g_vpu_chn_infos[grp_id][chn_id].height;
}

FH_SINT32 vpu_chn_init(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_VPU_CHN_INFO chn_info;
    FH_VPU_CHN_CONFIG chn_attr;

    chn_info.chn_max_size.u32Width = g_vpu_chn_infos[grp_id][chn_id].max_width;
    chn_info.chn_max_size.u32Height = g_vpu_chn_infos[grp_id][chn_id].max_height;
    chn_info.out_mode = g_vpu_chn_infos[grp_id][chn_id].yuv_type;
    chn_info.support_mode = 1 << chn_info.out_mode;
    chn_info.bufnum = g_vpu_chn_infos[grp_id][chn_id].bufnum;
    chn_info.max_stride = 0;

    ret = FH_VPSS_CreateChn(grp_id, chn_id, &chn_info);
    SDK_FUNC_ERROR(model_name, ret);

    chn_attr.vpu_chn_size.u32Width = g_vpu_chn_infos[grp_id][chn_id].width;
    chn_attr.vpu_chn_size.u32Height = g_vpu_chn_infos[grp_id][chn_id].height;
    chn_attr.crop_area.crop_en = 0; // TODO no chn crop
    chn_attr.crop_area.vpu_crop_area.u32X = 0;
    chn_attr.crop_area.vpu_crop_area.u32Y = 0;
    chn_attr.crop_area.vpu_crop_area.u32Width = 0;
    chn_attr.crop_area.vpu_crop_area.u32Height = 0;
    chn_attr.stride = 0;
    chn_attr.offset = 0;
    if (chn_id == NN_CHN)
    {
        chn_attr.depth = 1;
    }
    else
    {
        chn_attr.depth = 0;
    }
    ret = FH_VPSS_SetChnAttr(grp_id, chn_id, &chn_attr);
    SDK_FUNC_ERROR(model_name, ret);

    ret = FH_VPSS_SetVOMode(grp_id, chn_id, g_vpu_chn_infos[grp_id][chn_id].yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = FH_VPSS_OpenChn(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 get_vpu_chn_blk_size(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_UINT32 *blk_size)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FH_VPSS_GetFrameBlkSize(g_vpu_chn_infos[grp_id][chn_id].max_width, g_vpu_chn_infos[grp_id][chn_id].max_height, 1 << g_vpu_chn_infos[grp_id][chn_id].yuv_type, blk_size);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 get_vpu_chn_blk_size_with_wh(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 width, FH_SINT32 height, FH_UINT32 *blk_size)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FH_VPSS_GetFrameBlkSize(width, height, 1 << g_vpu_chn_infos[grp_id][chn_id].yuv_type, blk_size);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 lock_vpu_stream(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_VIDEO_FRAME *pstVpuframeinfo, FH_UINT32 timeout_ms, FH_UINT32 *handle_lock, FH_UINT32 is_norpt)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FH_VPSS_GetChnFrame(grp_id, chn_id, pstVpuframeinfo, timeout_ms, handle_lock, 1, is_norpt);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 unlock_vpu_stream(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_UINT32 handle_lock, FH_VIDEO_FRAME *pstVpuframeinfo)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FH_VPSS_ReleaseChnFrame(grp_id, chn_id, pstVpuframeinfo, handle_lock);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 set_vpu_chn_rotate(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 rotate)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_ROTATE_OPS chn_rotate = FH_RO_OPS_0;

    if (rotate != 270 && rotate != 180 && rotate != 90 && rotate != 0)
    {
        SDK_ERR_PRT(model_name, "parama rotate need [0,90,180,270]\n");
        return FH_SDK_FAILED;
    }

    switch (rotate)
    {
    case 0:
        chn_rotate = FH_RO_OPS_0;
        break;
    case 90:
        chn_rotate = FH_RO_OPS_90;
        break;
    case 180:
        chn_rotate = FH_RO_OPS_180;
        break;
    case 270:
        chn_rotate = FH_RO_OPS_270;
        break;
    default:
        break;
    }

    SDK_PRT(model_name, "grp_id = %d,chn_id = %d,chn_rotate=%d\n", grp_id, chn_id, chn_rotate);
    ret = FH_VPSS_SetVORotate(grp_id, chn_id, chn_rotate);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

static FH_UINT32 change_ratio_by_res(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_UINT32 width, FH_UINT32 height)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    // not support
    return ret;

    FH_VPU_CROP stVpucropinfo;
    FH_SINT32 VI_ratio;
    FH_SINT32 cur_ratio;

    FH_SINT32 vpu_vi_w;
    FH_SINT32 vpu_vi_h;

    vpu_vi_w = get_vpu_vi_w(grp_id);
    vpu_vi_h = get_vpu_vi_h(grp_id);

    VI_ratio = vpu_vi_w * 10 / vpu_vi_h;
    cur_ratio = width * 10 / height;

    if (VI_ratio >= 17 && cur_ratio < VI_ratio) /* vi 16:9 & cur 4:3 */
    {
        stVpucropinfo.vpu_crop_area.u32Width = (vpu_vi_w * height / width) & 0xfffffffc;
        stVpucropinfo.vpu_crop_area.u32Height = vpu_vi_h;
        stVpucropinfo.vpu_crop_area.u32X = (vpu_vi_w - stVpucropinfo.vpu_crop_area.u32Width) / 2;
        stVpucropinfo.vpu_crop_area.u32Y = 0;
    }
    else if (VI_ratio < 14 && cur_ratio > VI_ratio)
    {
        stVpucropinfo.vpu_crop_area.u32Width = vpu_vi_w;
        stVpucropinfo.vpu_crop_area.u32Height = (vpu_vi_w * height / width) & 0xfffffffc;
        stVpucropinfo.vpu_crop_area.u32X = 0;
        stVpucropinfo.vpu_crop_area.u32Y = (vpu_vi_w - stVpucropinfo.vpu_crop_area.u32Height) / 2;
    }
    else
    {
        stVpucropinfo.vpu_crop_area.u32Width = vpu_vi_w;
        stVpucropinfo.vpu_crop_area.u32Height = vpu_vi_h;
        stVpucropinfo.vpu_crop_area.u32X = 0;
        stVpucropinfo.vpu_crop_area.u32Y = 0;
    }

    if (stVpucropinfo.vpu_crop_area.u32X || stVpucropinfo.vpu_crop_area.u32Y)
    {
        stVpucropinfo.crop_en = 1;
    }

    ret = FH_VPSS_SetChnCrop(grp_id, chn_id, VPU_CROP_SCALER, &stVpucropinfo);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

static FH_SINT32 change_scaler_by_res(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 width, FH_SINT32 height)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_UINT8 coeff_val;

    if (width > 1920)
    {
        coeff_val = 14; /* 0 */
    }
    else if (width > 1024)
    {
        coeff_val = 14; /* 0 */
    }
    else if (width > 800)
    {
        coeff_val = 6; /* 3 */
    }
    else if (width > 640)
    {
        coeff_val = 6; /* 4 */
    }
    else if (width > 320)
    {
        coeff_val = 6; /* 6 */
    }
    else
    {
        coeff_val = 6; /* 6 */
    }

    ret = FH_VPSS_SetScalerCoeff(grp_id, chn_id, coeff_val);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 checkVpuChnWH(FH_SINT32 chn_id, FH_SINT32 width, FH_SINT32 height)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    SDK_CHECK_MIN_VALID(model_name, width, CHN_MIN_WIDTH, "CHN[%d] width[%d] < CHN_MIN_WIDTH[%d]\n", chn_id, width, CHN_MIN_WIDTH);
    SDK_CHECK_MIN_VALID(model_name, height, CHN_MIN_HEIGHT, "CHN[%d] height[%d] < CHN_MIN_HEIGHT[%d]\n", chn_id, height, CHN_MIN_HEIGHT);

    switch (chn_id)
    {
    case 0:
        SDK_CHECK_MAX_VALID(model_name, width, CHN0_MAX_WIDTH, "CHN[%d] width[%d] > CHN0_MAX_WIDTH[%d]\n", chn_id, width, CHN0_MAX_WIDTH);
        SDK_CHECK_MAX_VALID(model_name, height, CHN0_MAX_HEIGHT, "CHN[%d] height[%d] > CHN0_MAX_HEIGHT[%d]\n", chn_id, height, CHN0_MAX_HEIGHT);
        break;
    case 1:
        SDK_CHECK_MAX_VALID(model_name, width, CHN1_MAX_WIDTH, "CHN[%d] width[%d] > CHN1_MAX_WIDTH[%d]\n", chn_id, width, CHN1_MAX_WIDTH);
        SDK_CHECK_MAX_VALID(model_name, height, CHN1_MAX_HEIGHT, "CHN[%d] height[%d] > CHN1_MAX_HEIGHT[%d]\n", chn_id, height, CHN1_MAX_HEIGHT);
        break;
    case 2:
        SDK_CHECK_MAX_VALID(model_name, width, CHN2_MAX_WIDTH, "CHN[%d] width[%d] > CHN2_MAX_WIDTH[%d]\n", chn_id, width, CHN2_MAX_WIDTH);
        SDK_CHECK_MAX_VALID(model_name, height, CHN2_MAX_HEIGHT, "CHN[%d] height[%d] > CHN2_MAX_HEIGHT[%d]\n", chn_id, height, CHN2_MAX_HEIGHT);
        break;
    }

    return ret;
}

FH_SINT32 vpu_chn_close(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FH_VPSS_CloseChn(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 vpu_chn_destroy(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FH_VPSS_DestroyChn(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 vpu_chn_relase(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (!g_vpu_chn_infos[grp_id][chn_id].enable)
    {
        SDK_ERR_PRT(model_name, "vpu_chn_relase Failed, VPU[%d] CHN[%d] not enable!\n", grp_id, chn_id);
        return FH_SDK_FAILED;
    }

    ret = vpu_chn_close(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_destroy(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 changeVpuChnAttr(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_VPU_VO_MODE mode, FH_SINT32 width, FH_SINT32 height)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (!g_vpu_chn_infos[grp_id][chn_id].enable)
    {
        SDK_ERR_PRT(model_name, "changeVpuChnAttr Failed, VPU[%d] CHN[%d] not enable!\n", grp_id, chn_id);
        return FH_SDK_FAILED;
    }

    if (mode != 0)
    {
        g_vpu_chn_infos[grp_id][chn_id].yuv_type = mode;
    }

    g_vpu_chn_infos[grp_id][chn_id].width = width;
    g_vpu_chn_infos[grp_id][chn_id].height = height;

    ret = vpu_chn_init(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    change_ratio_by_res(grp_id, chn_id, width, height);
    change_scaler_by_res(grp_id, chn_id, width, height);

    return ret;
}

FH_SINT32 getVPUStreamToNN(FH_VIDEO_FRAME chnStr, NN_INPUT_DATA *nn_input_data)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    SDK_CHECK_NULL_PTR(model_name, nn_input_data);

    nn_input_data->frame_id = chnStr.frame_id;
    nn_input_data->pool_id = chnStr.pool_id;
    nn_input_data->timestamp = chnStr.time_stamp;

    switch (chnStr.data_format)
    {
    case VPU_VOMODE_ARGB888:
        nn_input_data->data.base = chnStr.frm_argb8888.data.base;
        nn_input_data->data.vbase = chnStr.frm_argb8888.data.vbase;
        nn_input_data->data.size = chnStr.frm_argb8888.data.size;
        nn_input_data->imageType = IMAGE_TYPE_RGB888;
        break;
    case VPU_VOMODE_RRGGBB:
        nn_input_data->data.base = chnStr.frm_rrggbb.r_data.data.base;
        nn_input_data->data.vbase = chnStr.frm_rrggbb.r_data.data.vbase;
        nn_input_data->data.size = chnStr.frm_rrggbb.r_data.data.size * 3;
        nn_input_data->imageType = IMAGE_TYPE_RRGGBB;
        break;
    default:
        SDK_ERR_PRT(model_name, "vpu out mode[%d] not found!\n", chnStr.data_format);
        break;
    }

    return ret;

Exit:
    return FH_SDK_FAILED;
}

FH_SINT32 vpuFPSControl(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 fps)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_FRAMERATE pstVpuframectrl;

    pstVpuframectrl.frame_count = fps;
    pstVpuframectrl.frame_time = 1;

    ret = FH_VPSS_SetFramectrl(grp_id, chn_id, &pstVpuframectrl);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
