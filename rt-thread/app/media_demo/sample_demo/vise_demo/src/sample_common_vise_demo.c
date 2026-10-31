#include "vise_demo/include/sample_common_vise_demo.h"

static char *model_name = MODEL_TAG_DEMO_VISE;

FH_SINT32 sample_fh_vise_pip(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    VPU_CHN_INFO *outside_chn;
    VPU_CHN_INFO *inside_chn;
    ENC_INFO *outside_mjpeg_info;

    SDK_CHECK_MAX_VALID(model_name, vise_id, MAX_VISE_CANVAS_NUM, "vise_id[%d] > MAX_VISE_CANVAS_NUM[%d]\n", vise_id, MAX_VISE_CANVAS_NUM);

    outside_chn = get_vpu_chn_config(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
    inside_chn = get_vpu_chn_config(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);
    outside_mjpeg_info = get_mjpeg_config(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);

    if (!outside_chn->enable)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable!Please enable the chn!", FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
        return FH_SDK_FAILED;
    }

    if (!inside_chn->enable)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable!Please enable the chn!\n", FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);
        return FH_SDK_FAILED;
    }

    if (outside_chn->yuv_type != inside_chn->yuv_type)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output yuvtype not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN, FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);
        return FH_SDK_FAILED;
    }

    ret = checkViseAttr(FH_APP_VISE_PIP_OUTSIDE_CHN, outside_chn->width, outside_chn->height, outside_chn->yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_PIP_INSIDE_CHN, inside_chn->width, inside_chn->height, inside_chn->yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_stop(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_destroy(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_vise_pip_init(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN, vise_id, 0);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN, vise_id, 1);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_bind_enc(vise_id, FH_APP_VISE_PIP_OUTSIDE_GRP * MAX_VPU_CHN_NUM + FH_APP_VISE_PIP_OUTSIDE_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    if (outside_mjpeg_info->enable)
    {
        ret = vise_bind_jpeg(vise_id, outside_mjpeg_info->channel);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = sample_common_vise_start(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    SDK_PRT(model_name, "Vise Pip Start!\n");
    SDK_PRT(model_name, "Outside Group[%d] CHN[%d]!\n", FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
    SDK_PRT(model_name, "Inside Group[%d] CHN[%d]!\n", FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);

    return ret;
}

FH_SINT32 sample_fh_vise_lrs(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    VPU_CHN_INFO *left_chn;
    VPU_CHN_INFO *right_chn;
    ENC_INFO *left_mjpeg_info;

    SDK_CHECK_MAX_VALID(model_name, vise_id, MAX_VISE_CANVAS_NUM, "vise_id[%d] > MAX_VISE_CANVAS_NUM[%d]\n", vise_id, MAX_VISE_CANVAS_NUM);

    left_chn = get_vpu_chn_config(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
    left_mjpeg_info = get_mjpeg_config(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
    right_chn = get_vpu_chn_config(FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);

    if (!left_chn->enable)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable!Please enable the chn!", FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
        return FH_SDK_FAILED;
    }

    if (!right_chn->enable)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable!Please enable the chn!\n", FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);
        return FH_SDK_FAILED;
    }

    if (left_chn->yuv_type != right_chn->yuv_type)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output yuvtype not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);
        return FH_SDK_FAILED;
    }

    if (left_chn->height != right_chn->height)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output height not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);
        return FH_SDK_FAILED;
    }

    ret = checkViseAttr(FH_APP_VISE_LRS_LEFT_CHN, left_chn->width, left_chn->height, left_chn->yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_LRS_RIGHT_CHN, right_chn->width, right_chn->height, right_chn->yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_chn_relase(FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_chn_relase(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    if (left_mjpeg_info->enable)
    {
        ret = mjpeg_chn_relase(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = changeEncAttr(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, 0, 0, left_chn->width + right_chn->width, left_chn->height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_vise_lrs_init(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, vise_id, 0);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN, vise_id, 1);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_bind_enc(vise_id, FH_APP_VISE_LRS_LEFT_GRP * MAX_VPU_CHN_NUM + FH_APP_VISE_LRS_LEFT_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    if (left_mjpeg_info->enable)
    {
        ret = changeMjpegAttr(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, left_chn->width + right_chn->width, left_chn->height);
        SDK_FUNC_ERROR(model_name, ret);

        ret = vise_bind_jpeg(vise_id, left_mjpeg_info->channel);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = sample_common_vise_start(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    SDK_PRT(model_name, "Vise Lrs Start!\n");
    SDK_PRT(model_name, "Left Group[%d] CHN[%d]!\n", FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
    SDK_PRT(model_name, "Right Group[%d] CHN[%d]!\n", FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);

    return ret;
}

FH_SINT32 sample_fh_vise_tbs(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    VPU_CHN_INFO *top_chn;
    VPU_CHN_INFO *bottom_chn;
    ENC_INFO *top_mjpeg_info;

    SDK_CHECK_MAX_VALID(model_name, vise_id, MAX_VISE_CANVAS_NUM, "vise_id[%d] > MAX_VISE_CANVAS_NUM[%d]\n", vise_id, MAX_VISE_CANVAS_NUM);

    top_chn = get_vpu_chn_config(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
    top_mjpeg_info = get_mjpeg_config(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
    bottom_chn = get_vpu_chn_config(FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);

    if (!top_chn->enable)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable!Please enable the chn!", FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
        return FH_SDK_FAILED;
    }

    if (!bottom_chn->enable)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable!Please enable the chn!\n", FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);
        return FH_SDK_FAILED;
    }

    if (top_chn->yuv_type != bottom_chn->yuv_type)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output yuvtype not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);
        return FH_SDK_FAILED;
    }

    if (top_chn->height != bottom_chn->height)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output height not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);
        return FH_SDK_FAILED;
    }

    ret = checkViseAttr(FH_APP_VISE_TBS_TOP_CHN, top_chn->width, top_chn->height, top_chn->yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_TBS_BOTTOM_CHN, bottom_chn->width, bottom_chn->height, bottom_chn->yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_chn_relase(FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_chn_relase(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    if (top_mjpeg_info->enable)
    {
        ret = mjpeg_chn_relase(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = changeEncAttr(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, 0, 0, top_chn->width, top_chn->height + bottom_chn->height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_vise_tbs_init(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, vise_id, 0);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN, vise_id, 1);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_bind_enc(vise_id, FH_APP_VISE_TBS_TOP_GRP * MAX_VPU_CHN_NUM + FH_APP_VISE_TBS_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    if (top_mjpeg_info->enable)
    {
        ret = changeMjpegAttr(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, top_chn->width, top_chn->height + bottom_chn->height);
        SDK_FUNC_ERROR(model_name, ret);

        ret = vise_bind_jpeg(vise_id, top_mjpeg_info->channel);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = sample_common_vise_start(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    SDK_PRT(model_name, "Vise Tbs Start!\n");
    SDK_PRT(model_name, "Top Group[%d] CHN[%d]!\n", FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
    SDK_PRT(model_name, "Bottom Group[%d] CHN[%d]!\n", FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);

    return ret;
}

FH_SINT32 sample_fh_vise_mbs(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    VPU_CHN_INFO *left_top_chn;
    VPU_CHN_INFO *left_bottom_chn;
    VPU_CHN_INFO *right_top_chn;
    VPU_CHN_INFO *right_bottom_chn;
    ENC_INFO *left_top_mjpeg_info;

    SDK_CHECK_MAX_VALID(model_name, vise_id, MAX_VISE_CANVAS_NUM, "vise_id[%d] > MAX_VISE_CANVAS_NUM[%d]\n", vise_id, MAX_VISE_CANVAS_NUM);

    left_top_chn = get_vpu_chn_config(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
    left_top_mjpeg_info = get_mjpeg_config(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);

    left_bottom_chn = get_vpu_chn_config(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);

    right_top_chn = get_vpu_chn_config(FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);

    right_bottom_chn = get_vpu_chn_config(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);

    if (!left_top_chn->enable)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable!Please enable the chn!", FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
        return FH_SDK_FAILED;
    }

    if (!left_bottom_chn->enable)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable!Please enable the chn!\n", FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);
        return FH_SDK_FAILED;
    }

    if (!right_top_chn->enable)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable!Please enable the chn!", FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);
        return FH_SDK_FAILED;
    }

    if (!right_bottom_chn->enable)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable!Please enable the chn!\n", FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);
        return FH_SDK_FAILED;
    }

    if (left_top_chn->yuv_type != left_bottom_chn->yuv_type)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output yuvtype not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);
        return FH_SDK_FAILED;
    }

    if (left_top_chn->yuv_type != right_top_chn->yuv_type)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output yuvtype not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);
        return FH_SDK_FAILED;
    }

    if (left_top_chn->yuv_type != right_bottom_chn->yuv_type)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output yuvtype not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);
        return FH_SDK_FAILED;
    }

    if (left_top_chn->height != right_top_chn->height)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output height not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);
        return FH_SDK_FAILED;
    }

    if (left_top_chn->width != left_bottom_chn->width)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output width not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);
        return FH_SDK_FAILED;
    }

    if (left_bottom_chn->height != right_bottom_chn->height)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output height not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN, FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);
        return FH_SDK_FAILED;
    }

    if (right_top_chn->width != right_bottom_chn->width)
    {
        SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] output width not same with VPU[%d] CHN[%d]!\n",
                    FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN, FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);
        return FH_SDK_FAILED;
    }

    ret = checkViseAttr(FH_APP_VISE_MBS_LEFT_TOP_CHN, left_top_chn->width, left_top_chn->height, left_top_chn->yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_MBS_LEFT_BOTTOM_CHN, left_bottom_chn->width, left_bottom_chn->height, left_bottom_chn->yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_MBS_RIGHT_TOP_CHN, right_top_chn->width, right_top_chn->height, right_top_chn->yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN, right_bottom_chn->width, right_bottom_chn->height, right_bottom_chn->yuv_type);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_chn_relase(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_chn_relase(FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_chn_relase(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_chn_relase(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    if (left_top_mjpeg_info->enable)
    {
        ret = mjpeg_chn_relase(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = changeEncAttr(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, 0, 0, left_top_chn->width + right_top_chn->width, left_top_chn->height + left_bottom_chn->height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = sample_common_vise_mbs_init(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, vise_id, 0);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN, vise_id, 1);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN, vise_id, 2);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN, vise_id, 3);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vise_bind_enc(vise_id, FH_APP_VISE_MBS_LEFT_TOP_GRP * MAX_VPU_CHN_NUM + FH_APP_VISE_MBS_LEFT_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    if (left_top_mjpeg_info->enable)
    {
        ret = changeMjpegAttr(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, left_top_chn->width + right_top_chn->width, left_top_chn->height + left_bottom_chn->height);
        SDK_FUNC_ERROR(model_name, ret);

        ret = vise_bind_jpeg(vise_id, left_top_mjpeg_info->channel);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = sample_common_vise_start(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    SDK_PRT(model_name, "Vise Mbs Start!\n");
    SDK_PRT(model_name, "Left Top Group[%d] CHN[%d]!\n", FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
    SDK_PRT(model_name, "Left Bottom Group[%d] CHN[%d]!\n", FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);
    SDK_PRT(model_name, "Right Top Group[%d] CHN[%d]!\n", FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);
    SDK_PRT(model_name, "Right Bottom Group[%d] CHN[%d]!\n", FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);

    return ret;
}

FH_SINT32 sample_fh_vise_demo_start(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

#ifdef FH_APP_VISE_PIP
    ret = sample_fh_vise_pip(vise_id);
    SDK_FUNC_ERROR(model_name, ret);
#endif

#ifdef FH_APP_VISE_LRS
    ret = sample_fh_vise_lrs(vise_id);
    SDK_FUNC_ERROR(model_name, ret);
#endif

#ifdef FH_APP_VISE_TBS
    ret = sample_fh_vise_tbs(vise_id);
    SDK_FUNC_ERROR(model_name, ret);
#endif

#ifdef FH_APP_VISE_MBS
    ret = sample_fh_vise_mbs(vise_id);
    SDK_FUNC_ERROR(model_name, ret);
#endif

    return ret;
}

FH_SINT32 sample_fh_vise_demo_stop(FH_SINT32 vise_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = sample_common_vise_exit(vise_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
