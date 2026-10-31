#include "uvc_rtt/include/sample_common_uvc.h"
#include "overlay/include/sample_common_overlay.h"

static char *model_name = MODEL_TAG_DEMO_UVC;

static struct uvc_dev_app g_uvc_dev[2];
static pthread_t g_thread_video_stream[2];
#ifndef ENABLE_PSRAM
static pthread_t g_thread_yuv[2];
#endif
static FH_SINT32 g_uvc_th_status = 0;

#ifdef FH_APP_OPEN_VISE
#ifdef FH_APP_VISE_PIP
static FH_SINT32 changePipeLineWithVisePip(FH_SINT32 grp_id, FH_SINT32 chn_id, UVC_FORMAT format, FH_SINT32 width, FH_SINT32 height)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ENC_INFO *mjpeg_info;
    FH_AREA outside;
    FH_AREA inside;

    mjpeg_info = get_mjpeg_config(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
    outside.u32Width = ALIGNTO(width, 16);
    outside.u32Height = ALIGNTO(height, 16);
    outside.u32X = 0;
    outside.u32Y = 0;

    inside.u32Width = ALIGNTO(width / 4, 16);
    inside.u32Height = ALIGNTO(height / 4, 16);
    inside.u32X = outside.u32Width - inside.u32Width;
    inside.u32Y = 0;

    ret = checkViseAttr(FH_APP_VISE_PIP_OUTSIDE_CHN, outside.u32Width, outside.u32Height, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_PIP_INSIDE_CHN, inside.u32Width, inside.u32Height, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkVpuChnWH(FH_APP_VISE_PIP_OUTSIDE_CHN, outside.u32Width, outside.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkVpuChnWH(FH_APP_VISE_PIP_INSIDE_CHN, inside.u32Width, inside.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_exit(FH_APP_VISE_PIP_OUTSIDE_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_exit(FH_APP_VISE_PIP_INSIDE_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = changeVpuChnAttr(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN, VPU_VOMODE_SCAN, outside.u32Width, outside.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = changeVpuChnAttr(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN, VPU_VOMODE_SCAN, inside.u32Width, inside.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_init(FH_APP_VISE_PIP_OUTSIDE_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    switch (format)
    {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_IR:
        SDK_PRT(model_name, "changeVpuChnAttr V4L2_PIX_FMT_NV12/V4L2_PIX_FMT_IR\n");
        break;
    case V4L2_PIX_FMT_H264:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_H264\n");

        ret = enc_chn_relase(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeEncAttr(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN, FH_NORMAL_H264, FH_RC_H264_FIXQP, width, height);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_H265:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_H265\n");

        ret = enc_chn_relase(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeEncAttr(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN, FH_NORMAL_H265, FH_RC_H265_FIXQP, width, height);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_MJPEG\n");

        ret = mjpeg_chn_relase(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeMjpegAttr(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN, width, height);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    default:
        SDK_ERR_PRT(model_name, "format not support! \n");
        break;
    }

    ret = changeVise_PipAttr(FH_APP_VISE_PIP_OUTSIDE_GRP, outside, inside, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN, FH_APP_VISE_PIP_OUTSIDE_GRP, 0);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN, FH_APP_VISE_PIP_OUTSIDE_GRP, 1);
    SDK_FUNC_ERROR(model_name, ret);

    switch (format)
    {
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H265:
        ret = vise_bind_enc(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_GRP * MAX_VPU_CHN_NUM + FH_APP_VISE_PIP_OUTSIDE_CHN);
        SDK_FUNC_ERROR(model_name, ret);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
        ret = vise_bind_jpeg(FH_APP_VISE_PIP_OUTSIDE_GRP, mjpeg_info->channel);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    default:
        break;
    }

    ret = vpu_bind_bgm(FH_APP_VISE_PIP_OUTSIDE_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
#elif defined FH_APP_VISE_LRS
static FH_SINT32 changePipeLineWithViseLrs(FH_SINT32 grp_id, FH_SINT32 chn_id, UVC_FORMAT format, FH_SINT32 width, FH_SINT32 height)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ENC_INFO *mjpeg_info;
    FH_AREA left;
    FH_AREA right;

    mjpeg_info = get_mjpeg_config(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
    left.u32Width = ALIGNTO(width / 2, 16);
    left.u32Height = ALIGNTO(height, 16);
    left.u32X = 0;
    left.u32Y = 0;

    right.u32Width = ALIGNTO(width / 2, 16);
    right.u32Height = ALIGNTO(height, 16);
    right.u32X = left.u32Width;
    right.u32Y = 0;

    ret = checkViseAttr(FH_APP_VISE_LRS_LEFT_CHN, left.u32Width, left.u32Height, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_LRS_RIGHT_CHN, right.u32Width, right.u32Height, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkVpuChnWH(FH_APP_VISE_LRS_LEFT_CHN, left.u32Width, left.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkVpuChnWH(FH_APP_VISE_LRS_RIGHT_CHN, right.u32Width, right.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_exit(FH_APP_VISE_LRS_LEFT_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_exit(FH_APP_VISE_LRS_RIGHT_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = changeVpuChnAttr(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, VPU_VOMODE_SCAN, left.u32Width, left.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = changeVpuChnAttr(FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN, VPU_VOMODE_SCAN, right.u32Width, right.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_init(FH_APP_VISE_LRS_LEFT_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    switch (format)
    {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_IR:
        SDK_PRT(model_name, "changeVpuChnAttr V4L2_PIX_FMT_NV12/V4L2_PIX_FMT_IR\n");
        break;
    case V4L2_PIX_FMT_H264:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_H264\n");

        ret = enc_chn_relase(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeEncAttr(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, FH_NORMAL_H264, FH_RC_H264_FIXQP, width, height);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_H265:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_H265\n");

        ret = enc_chn_relase(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeEncAttr(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, FH_NORMAL_H265, FH_RC_H265_FIXQP, width, height);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_MJPEG\n");

        ret = mjpeg_chn_relase(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeMjpegAttr(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, width, height);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    default:
        SDK_ERR_PRT(model_name, "format not support! \n");
        break;
    }

    ret = changeVise_LrsAttr(FH_APP_VISE_LRS_LEFT_GRP, left, right, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN, FH_APP_VISE_LRS_LEFT_GRP, 0);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN, FH_APP_VISE_LRS_LEFT_GRP, 1);
    SDK_FUNC_ERROR(model_name, ret);

    switch (format)
    {
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H265:
        ret = vise_bind_enc(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_GRP * MAX_VPU_CHN_NUM + FH_APP_VISE_LRS_LEFT_CHN);
        SDK_FUNC_ERROR(model_name, ret);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
        ret = vise_bind_jpeg(FH_APP_VISE_LRS_LEFT_GRP, mjpeg_info->channel);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    default:
        break;
    }

    ret = vpu_bind_bgm(FH_APP_VISE_LRS_LEFT_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
#elif defined FH_APP_VISE_TBS
static FH_SINT32 changePipeLineWithViseTbs(FH_SINT32 grp_id, FH_SINT32 chn_id, UVC_FORMAT format, FH_SINT32 width, FH_SINT32 height)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ENC_INFO *mjpeg_info;
    FH_AREA top;
    FH_AREA bottom;

    mjpeg_info = get_mjpeg_config(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
    top.u32Width = ALIGNTO(width, 16);
    top.u32Height = ALIGNTO(height / 2, 16);
    top.u32X = 0;
    top.u32Y = 0;

    bottom.u32Width = ALIGNTO(width, 16);
    bottom.u32Height = ALIGNTO(height / 2, 16);
    bottom.u32X = 0;
    bottom.u32Y = top.u32Height;

    ret = checkViseAttr(FH_APP_VISE_TBS_TOP_CHN, top.u32Width, top.u32Height, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_TBS_BOTTOM_CHN, bottom.u32Width, bottom.u32Height, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkVpuChnWH(FH_APP_VISE_TBS_TOP_CHN, top.u32Width, top.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkVpuChnWH(FH_APP_VISE_TBS_BOTTOM_CHN, bottom.u32Width, bottom.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_exit(FH_APP_VISE_TBS_TOP_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_exit(FH_APP_VISE_TBS_BOTTOM_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = changeVpuChnAttr(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, VPU_VOMODE_SCAN, top.u32Width, top.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = changeVpuChnAttr(FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN, VPU_VOMODE_SCAN, bottom.u32Width, bottom.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_init(FH_APP_VISE_TBS_TOP_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    switch (format)
    {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_IR:
        SDK_PRT(model_name, "changeVpuChnAttr V4L2_PIX_FMT_NV12/V4L2_PIX_FMT_IR\n");
        break;
    case V4L2_PIX_FMT_H264:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_H264\n");

        ret = enc_chn_relase(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeEncAttr(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, FH_NORMAL_H264, FH_RC_H264_FIXQP, top.u32Width, top.u32Height * 2);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_H265:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_H265\n");

        ret = enc_chn_relase(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeEncAttr(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, FH_NORMAL_H265, FH_RC_H265_FIXQP, top.u32Width, top.u32Height * 2);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_MJPEG\n");

        ret = mjpeg_chn_relase(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeMjpegAttr(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, top.u32Width, top.u32Height * 2);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    default:
        SDK_ERR_PRT(model_name, "format not support! \n");
        break;
    }

    ret = changeVise_TbsAttr(FH_APP_VISE_TBS_TOP_GRP, top, bottom, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN, FH_APP_VISE_TBS_TOP_GRP, 0);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN, FH_APP_VISE_TBS_TOP_GRP, 1);
    SDK_FUNC_ERROR(model_name, ret);

    switch (format)
    {
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H265:
        ret = vise_bind_enc(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_GRP * MAX_VPU_CHN_NUM + FH_APP_VISE_TBS_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
        ret = vise_bind_jpeg(FH_APP_VISE_TBS_TOP_GRP, mjpeg_info->channel);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    default:
        break;
    }

    ret = vpu_bind_bgm(FH_APP_VISE_TBS_TOP_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
#elif defined FH_APP_VISE_MBS
static FH_SINT32 changePipeLineWithViseMbs(FH_SINT32 grp_id, FH_SINT32 chn_id, UVC_FORMAT format, FH_SINT32 width, FH_SINT32 height)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ENC_INFO *mjpeg_info;
    FH_AREA left_top;
    FH_AREA left_bottom;
    FH_AREA right_top;
    FH_AREA right_bottom;

    mjpeg_info = get_mjpeg_config(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
    left_top.u32Width = ALIGNTO(width / 2, 16);
    left_top.u32Height = ALIGNTO(height / 2, 16);
    left_top.u32X = 0;
    left_top.u32Y = 0;

    left_bottom.u32Width = ALIGNTO(width / 2, 16);
    left_bottom.u32Height = ALIGNTO(height / 2, 16);
    left_bottom.u32X = 0;
    left_bottom.u32Y = left_top.u32Height;

    right_top.u32Width = ALIGNTO(width / 2, 16);
    right_top.u32Height = ALIGNTO(height / 2, 16);
    right_top.u32X = left_top.u32Width;
    right_top.u32Y = 0;

    right_bottom.u32Width = ALIGNTO(width / 2, 16);
    right_bottom.u32Height = ALIGNTO(height / 2, 16);
    right_bottom.u32X = left_top.u32Width;
    right_bottom.u32Y = left_top.u32Height;

    ret = checkViseAttr(FH_APP_VISE_MBS_LEFT_TOP_CHN, left_top.u32Width, left_top.u32Height, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_MBS_LEFT_BOTTOM_CHN, left_bottom.u32Width, left_bottom.u32Height, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_MBS_RIGHT_TOP_CHN, right_top.u32Width, right_top.u32Height, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkViseAttr(FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN, right_bottom.u32Width, right_bottom.u32Height, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkVpuChnWH(FH_APP_VISE_MBS_LEFT_TOP_CHN, left_top.u32Width, left_top.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkVpuChnWH(FH_APP_VISE_MBS_LEFT_BOTTOM_CHN, left_bottom.u32Width, left_bottom.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkVpuChnWH(FH_APP_VISE_MBS_RIGHT_TOP_CHN, right_top.u32Width, right_top.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = checkVpuChnWH(FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN, right_bottom.u32Width, right_bottom.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_exit(FH_APP_VISE_MBS_LEFT_TOP_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_exit(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_exit(FH_APP_VISE_MBS_RIGHT_TOP_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_exit(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = changeVpuChnAttr(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, VPU_VOMODE_SCAN, left_top.u32Width, left_top.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = changeVpuChnAttr(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN, VPU_VOMODE_SCAN, left_bottom.u32Width, left_bottom.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = changeVpuChnAttr(FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN, VPU_VOMODE_SCAN, right_top.u32Width, right_top.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = changeVpuChnAttr(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN, VPU_VOMODE_SCAN, right_bottom.u32Width, right_bottom.u32Height);
    SDK_FUNC_ERROR(model_name, ret);

    ret = bgm_init(FH_APP_VISE_MBS_LEFT_TOP_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    switch (format)
    {
    case V4L2_PIX_FMT_NV12:
    case V4L2_PIX_FMT_IR:
        SDK_PRT(model_name, "changeVpuChnAttr V4L2_PIX_FMT_NV12/V4L2_PIX_FMT_IR\n");
        break;
    case V4L2_PIX_FMT_H264:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_H264\n");

        ret = enc_chn_relase(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeEncAttr(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, FH_NORMAL_H264, FH_RC_H264_FIXQP, left_top.u32Width * 2, left_top.u32Height * 2);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_H265:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_H265\n");

        ret = enc_chn_relase(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeEncAttr(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, FH_NORMAL_H265, FH_RC_H265_FIXQP, left_top.u32Width * 2, left_top.u32Height * 2);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_MJPEG\n");

        ret = mjpeg_chn_relase(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeMjpegAttr(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, left_top.u32Width * 2, left_top.u32Height * 2);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    default:
        break;
    }

    ret = changeVise_MbsAttr(FH_APP_VISE_TBS_TOP_GRP, left_top, left_bottom, right_top, right_bottom, VPU_VOMODE_SCAN);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN, grp_id, 0);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN, grp_id, 1);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN, grp_id, 2);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_bind_vise(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN, grp_id, 3);
    SDK_FUNC_ERROR(model_name, ret);

    switch (format)
    {
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H265:
        ret = vise_bind_enc(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_GRP * MAX_VPU_CHN_NUM + FH_APP_VISE_MBS_LEFT_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
        ret = vise_bind_jpeg(FH_APP_VISE_MBS_LEFT_TOP_GRP, mjpeg_info->channel);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    default:
        SDK_ERR_PRT(model_name, "format not support! \n");
        break;
    }

    ret = vpu_bind_bgm(FH_APP_VISE_MBS_LEFT_TOP_GRP);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}
#endif
#endif

static FH_SINT32 changePipeLine(FH_SINT32 grp_id, FH_SINT32 chn_id, UVC_FORMAT format, FH_SINT32 width, FH_SINT32 height)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ENC_INFO *mjpeg_info;
#ifdef ENABLE_PSRAM
    VPU_CHN_INFO *vpu_chn_info = NULL;
#endif
    mjpeg_info = get_mjpeg_config(grp_id, chn_id);

    ret = bgm_exit(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    ret = unbind_vpu_chn(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    // relase
    ret = vpu_chn_relase(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    switch (format)
    {
    case V4L2_PIX_FMT_YUY2:
        SDK_PRT(model_name, "changeVpuChnAttr V4L2_PIX_FMT_YUY2\n");
#ifdef ENABLE_PSRAM
        vpu_mod_change(grp_id, 0, 0, 100);
#endif
#ifdef NV12_TO_YUY2
        ret = changeVpuChnAttr(grp_id, chn_id, VPU_VOMODE_SCAN, width, height);
        SDK_FUNC_ERROR(model_name, ret);
#else
        ret = changeVpuChnAttr(grp_id, chn_id, VPU_VOMODE_YUYV, width, height);
        SDK_FUNC_ERROR(model_name, ret);
#endif
        break;
    case V4L2_PIX_FMT_IR:
    case V4L2_PIX_FMT_NV12:
        ret = changeVpuChnAttr(grp_id, chn_id, VPU_VOMODE_SCAN, width, height);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H265:
        ret = enc_chn_relase(grp_id, chn_id);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeVpuChnAttr(grp_id, chn_id, VPU_VOMODE_SCAN, width, height);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
#ifdef ENABLE_PSRAM
        vpu_chn_info = get_vpu_chn_config(grp_id, 0);
        vpu_mod_change(grp_id, vpu_chn_info->lpbuf_en, vpu_chn_info->is_sysmem, 100);
#endif
#ifndef ENABLE_PSRAM
        ret = enc_chn_relase(grp_id, chn_id);
        SDK_FUNC_ERROR(model_name, ret);
#endif
        ret = mjpeg_chn_relase(grp_id, chn_id);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeVpuChnAttr(grp_id, chn_id, VPU_VOMODE_SCAN, width, height);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    default:
        break;
    }

    ret = bgm_init(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    switch (format)
    {
    case V4L2_PIX_FMT_H264:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_H264\n");
        ret = changeEncAttr(grp_id, chn_id, FH_NORMAL_H264, FH_RC_H264_FIXQP, width, height);
        SDK_FUNC_ERROR(model_name, ret);

        ret = vpu_bind_enc(grp_id, chn_id);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_H265:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_H265\n");
        ret = changeEncAttr(grp_id, chn_id, FH_NORMAL_H265, FH_RC_H265_FIXQP, width, height);
        SDK_FUNC_ERROR(model_name, ret);

        ret = vpu_bind_enc(grp_id, chn_id);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
        SDK_PRT(model_name, "changeEncFormat V4L2_PIX_FMT_MJPEG\n");
#ifndef ENABLE_PSRAM
        ret = changeEncAttr(grp_id, chn_id, FH_NORMAL_H264, FH_RC_H264_FIXQP, width, height);
        SDK_FUNC_ERROR(model_name, ret);

        ret = vpu_bind_enc(grp_id, chn_id);
        SDK_FUNC_ERROR(model_name, ret);
#endif
        ret = changeMjpegAttr(grp_id, chn_id, width, height);
        SDK_FUNC_ERROR(model_name, ret);

        ret = vpu_bind_jpeg(grp_id, chn_id, mjpeg_info->channel);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    default:
        break;
    }

    ret = vpu_bind_bgm(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

static FH_SINT32 changeVoFormat(FH_SINT32 grp_id, FH_SINT32 chn_id, UVC_FORMAT format, FH_SINT32 width, FH_SINT32 height)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

#ifdef FH_APP_OPEN_VISE // 双码流，需要处理
    if (V4L2_PIX_FMT_YUY2 != format && grp_id != STREAM_ID2)
    {
#ifdef FH_APP_VISE_PIP
        ret = changePipeLineWithVisePip(grp_id, chn_id, format, width, height);
#elif defined FH_APP_VISE_LRS
        ret = changePipeLineWithViseLrs(grp_id, chn_id, format, width, height);
#elif defined FH_APP_VISE_TBS
        ret = changePipeLineWithViseTbs(grp_id, chn_id, format, width, height);
#elif defined FH_APP_VISE_MBS
        ret = changePipeLineWithViseMbs(grp_id, chn_id, format, width, height);
#endif
        SDK_FUNC_ERROR(model_name, ret);
    }
    else
    {
        if (grp_id == STREAM_ID2)
        {
            SDK_PRT(model_name, "UVC Stream[2] not support VISE!\n");
        }
        else
        {
            SDK_PRT(model_name, "VISE not support YUY2!\n");
        }
#ifdef FH_APP_VISE_PIP
        ret = unbind_vpu_chn(FH_APP_VISE_PIP_OUTSIDE_GRP, FH_APP_VISE_PIP_OUTSIDE_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = unbind_vpu_chn(FH_APP_VISE_PIP_INSIDE_GRP, FH_APP_VISE_PIP_INSIDE_CHN);
        SDK_FUNC_ERROR(model_name, ret);
#elif defined FH_APP_VISE_LRS
        ret = unbind_vpu_chn(FH_APP_VISE_LRS_LEFT_GRP, FH_APP_VISE_LRS_LEFT_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = unbind_vpu_chn(FH_APP_VISE_LRS_RIGHT_GRP, FH_APP_VISE_LRS_RIGHT_CHN);
        SDK_FUNC_ERROR(model_name, ret);
#elif defined FH_APP_VISE_TBS
        ret = unbind_vpu_chn(FH_APP_VISE_TBS_TOP_GRP, FH_APP_VISE_TBS_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = unbind_vpu_chn(FH_APP_VISE_TBS_BOTTOM_GRP, FH_APP_VISE_TBS_BOTTOM_CHN);
        SDK_FUNC_ERROR(model_name, ret);
#elif defined FH_APP_VISE_MBS
        ret = unbind_vpu_chn(FH_APP_VISE_MBS_LEFT_TOP_GRP, FH_APP_VISE_MBS_LEFT_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = unbind_vpu_chn(FH_APP_VISE_MBS_LEFT_BOTTOM_GRP, FH_APP_VISE_MBS_LEFT_BOTTOM_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = unbind_vpu_chn(FH_APP_VISE_MBS_RIGHT_TOP_GRP, FH_APP_VISE_MBS_RIGHT_TOP_CHN);
        SDK_FUNC_ERROR(model_name, ret);

        ret = unbind_vpu_chn(FH_APP_VISE_MBS_RIGHT_BOTTOM_GRP, FH_APP_VISE_MBS_RIGHT_BOTTOM_CHN);
        SDK_FUNC_ERROR(model_name, ret);
#endif
        ret = changePipeLine(grp_id, chn_id, format, width, height);
        SDK_FUNC_ERROR(model_name, ret);
    }
#else
    ret = changePipeLine(grp_id, chn_id, format, width, height);
    SDK_FUNC_ERROR(model_name, ret);
#endif

    return ret;
}

static char szFmt[8];
static char *f2s(int fmt)
{
    memset(szFmt, 0, sizeof(szFmt));
    memcpy(szFmt, &fmt, sizeof(fmt));
    szFmt[sizeof(fmt)] = 0;
    return szFmt;
}

FH_VOID stream_probe_change(FH_SINT32 stream_id, UVC_FORMAT fmt, FH_SINT32 width, FH_SINT32 height, FH_SINT32 fps)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 grp_id = 0;
    FH_SINT32 chn_id = 0;

    if (stream_id == STREAM_ID1)
    {
        grp_id = 0;
        chn_id = 0;
    }
    else if (stream_id == STREAM_ID2)
    {
        grp_id = UVC_DUAL_STREAM_GRP;
        chn_id = UVC_DUAL_STREAM_CHN;
    }

    g_uvc_dev[stream_id].fcc = fmt;
    g_uvc_dev[stream_id].fps = fps;
    g_uvc_dev[stream_id].width = width;
    g_uvc_dev[stream_id].height = height;

    for (int i = 0; i < UVC_YUV_BUF_NUM; i++)
    {
        g_uvc_dev[stream_id].yuv_buf[i].is_full = 0;
        g_uvc_dev[stream_id].yuv_buf[i].is_lock = 0;
    }

    if (fmt == V4L2_PIX_FMT_H264)
    {
        g_uvc_dev[stream_id].g_h264_delay = 5;
    }
    else if (fmt == V4L2_PIX_FMT_H265)
    {
        g_uvc_dev[stream_id].g_h265_delay = 5;
    }

    SDK_PRT(model_name, "Change Video:%d,fmt = %s,width = %d,height = %d,fps = %d\n", stream_id, f2s(fmt), width, height, fps);

    if (fps > 12)
    {
        ret = changeSensorFPS(grp_id, fps);
        SDK_FUNC_ERROR_GOTO(model_name, ret);
    }

    ret = changeVoFormat(grp_id, chn_id, fmt, width, height);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    if (fps < 12)
    {
        ret = vpuFPSControl(grp_id, chn_id, fps);
        SDK_FUNC_ERROR_GOTO(model_name, ret);
    }
#if defined(FH_OVERLAY_ENABLE)
    ret = sample_fh_overlay_start(grp_id);
    SDK_FUNC_ERROR_GOTO(model_name, ret);
#endif
Exit:
    return;
}

FH_SINT32 uvcGetStreamToKernel(struct uvc_dev_app *pDev)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 grp_id = 0, chn_id = 0;

    SDK_CHECK_NULL_PTR(model_name, pDev);
    pthread_mutex_lock(&pDev->mutex_fmt);
    if (pDev->stream_id == STREAM_ID1)
    {
        grp_id = 0;
        chn_id = 0;
    }
    else if (pDev->stream_id == STREAM_ID2)
    {
        grp_id = UVC_DUAL_STREAM_GRP; // TODO 需要解耦，
        chn_id = UVC_DUAL_STREAM_CHN;
    }
#ifdef UVC_STILL_IMAGE_CAPTURE_METHOD2
    if (pDev->still_image)
    {
        pthread_mutex_unlock(&pDev->mutex_fmt);
        ret = changeVoFormat(grp_id, chn_id, V4L2_PIX_FMT_MJPEG, pDev->still_image_w, pDev->still_image_h);
        SDK_FUNC_ERROR(model_name, ret);
#if defined(FH_OVERLAY_ENABLE)
        ret = sample_fh_overlay_start(grp_id);
        SDK_FUNC_ERROR(model_name, ret);
#endif
        ret = getMJPEGStreamToUVC(grp_id, chn_id, pDev, FH_STREAM_MJPEG, pDev->still_image);
        SDK_FUNC_ERROR(model_name, ret);

        ret = changeVoFormat(grp_id, chn_id, pDev->fcc, pDev->width, pDev->height);
        SDK_FUNC_ERROR(model_name, ret);
#if defined(FH_OVERLAY_ENABLE)
        ret = sample_fh_overlay_start(grp_id);
        SDK_FUNC_ERROR(model_name, ret);
#endif
        pDev->still_image = 0;

        return FH_SDK_SUCCESS;
    }
#endif
    switch (pDev->fcc)
    {
    case V4L2_PIX_FMT_IR:
    case V4L2_PIX_FMT_NV12:
#ifdef FH_APP_OPEN_VISE
        if (grp_id == STREAM_ID1)
        {
            ret = getViseStreamToUVC(grp_id, chn_id, pDev);
            SDK_FUNC_ERROR_PRT(model_name, ret);
        }
        else
        {
            ret = getVPUStreamToUVC(grp_id, chn_id, pDev);
            SDK_FUNC_ERROR_PRT(model_name, ret);
        }
#else
        ret = getVPUStreamToUVC(grp_id, chn_id, pDev);
        SDK_FUNC_ERROR_PRT(model_name, ret);
#endif
        break;
    case V4L2_PIX_FMT_YUY2:
        ret = getVPUStreamToUVC(grp_id, chn_id, pDev);
        SDK_FUNC_ERROR_PRT(model_name, ret);
        break;
    case V4L2_PIX_FMT_MJPEG:
        ret = getMJPEGStreamToUVC(grp_id, chn_id, pDev, FH_STREAM_MJPEG, 0);
        SDK_FUNC_ERROR_PRT(model_name, ret);
        break;
    case V4L2_PIX_FMT_H264:
        ret = getENCStreamToUVC(grp_id, chn_id, pDev, FH_STREAM_H264);
        SDK_FUNC_ERROR_PRT(model_name, ret);
        break;
    case V4L2_PIX_FMT_H265:
        ret = getENCStreamToUVC(grp_id, chn_id, pDev, FH_STREAM_H265);
        SDK_FUNC_ERROR_PRT(model_name, ret);
        break;
    default:
        ret = FH_SDK_FAILED;
        break;
    }
    pthread_mutex_unlock(&pDev->mutex_fmt);
    return ret;
Exit:
    return FH_SDK_FAILED;
}

void *uvcProcess_stream(void *p)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    struct uvc_dev_app *pDev;
    FH_CHAR thread_name[20];

    pDev = (struct uvc_dev_app *)p;

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "uvcProcess_stream_%d", pDev->stream_id);
    prctl(PR_SET_NAME, thread_name);
    while (g_uvc_th_status)
    {

        if (fh_uvc_status_get(pDev->stream_id) == UVC_STREAM_ON)
        {
            ret = uvcGetStreamToKernel(pDev);
            if (ret)
            {
                usleep(10 * 1000);
            }
        }
        else
        {
            usleep(10 * 1000);
        }
    }

    return NULL;
}

FH_SINT32 uvcGetStreamToList(struct uvc_dev_app *pDev)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 grp_id = 0, chn_id = 0;

    SDK_CHECK_NULL_PTR(model_name, pDev);
    pthread_mutex_lock(&pDev->mutex_fmt);
    if (pDev->stream_id == STREAM_ID1)
    {
        grp_id = 0;
        chn_id = 0;
    }
    else if (pDev->stream_id == STREAM_ID2)
    {
        grp_id = UVC_DUAL_STREAM_GRP; // TODO 需要解耦，
        chn_id = UVC_DUAL_STREAM_CHN;
    }

    switch (pDev->fcc)
    {
    case V4L2_PIX_FMT_IR:
    case V4L2_PIX_FMT_NV12:
#ifdef FH_APP_OPEN_VISE
        if (grp_id == STREAM_ID1)
        {
            ret = getViseStreamToUVCList(grp_id, chn_id, pDev);
            SDK_FUNC_ERROR_PRT(model_name, ret);
        }
        else
        {
            ret = getVPUStreamToUVCList(grp_id, chn_id, pDev);
            SDK_FUNC_ERROR_PRT(model_name, ret);
        }
#else
        ret = getVPUStreamToUVCList(grp_id, chn_id, pDev);
        SDK_FUNC_ERROR_PRT(model_name, ret);
#endif
        break;
    case V4L2_PIX_FMT_YUY2:
        ret = getVPUStreamToUVCList(grp_id, chn_id, pDev);
        SDK_FUNC_ERROR_PRT(model_name, ret);
        break;
    default:
        break;
    }
    pthread_mutex_unlock(&pDev->mutex_fmt);
    return ret;
Exit:
    return FH_SDK_FAILED;
}

#ifndef ENABLE_PSRAM
static void *yuv_get_proc(void *arg)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    struct uvc_dev_app *pDev;
    FH_CHAR thread_name[20];

    pDev = (struct uvc_dev_app *)arg;

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "yuv_get_proc_%d", pDev->stream_id);
    prctl(PR_SET_NAME, thread_name);
    while (g_uvc_th_status)
    {
        if (fh_uvc_status_get(pDev->stream_id) == UVC_STREAM_ON)
        {
            ret = uvcGetStreamToList(pDev);
            if (ret)
            {
                usleep(10 * 1000);
            }
        }
        else
        {
            usleep(10 * 1000);
        }
    }

    return NULL;
}
#endif
FH_SINT32 sample_common_uvc_start(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    struct sched_param param;
    pthread_attr_t attr;

    if (g_uvc_th_status)
    {
        SDK_PRT(model_name, "UVC already running!\n");
        return ret;
    }

#ifdef FH_APP_USING_HEX_PARA
    fh_uvc_update_init();
#endif

    ret = uvc_init(stream_probe_change);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    ret = init_dev_info(&g_uvc_dev[STREAM_ID1], STREAM_ID1);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    g_uvc_th_status = 1;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 8 * 1024);

    param.sched_priority = 30;
    pthread_attr_setschedparam(&attr, &param);
    if (pthread_create(&g_thread_video_stream[STREAM_ID1], &attr, uvcProcess_stream, (FH_VOID *)&g_uvc_dev[STREAM_ID1]) != 0)
    {
        printf("Error: Create video_stream_proc thread failed!\n");
    }
#ifndef ENABLE_PSRAM
    param.sched_priority = 31;
    pthread_attr_setschedparam(&attr, &param);
    if (pthread_create(&g_thread_yuv[STREAM_ID1], &attr, yuv_get_proc, (FH_VOID *)&g_uvc_dev[STREAM_ID1]) != 0)
    {
        printf("Error: Create yuv_get_proc thread failed!\n");
    }
#endif
#ifdef UVC_DOUBLE_STREAM
    ret = init_dev_info(&g_uvc_dev[STREAM_ID2], STREAM_ID2);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    param.sched_priority = 30;
    if (pthread_create(&g_thread_video_stream[STREAM_ID2], &attr, uvcProcess_stream, (FH_VOID *)&g_uvc_dev[STREAM_ID2]) != 0)
    {
        printf("Error: Create video_stream_proc2 thread failed!\n");
    }

    param.sched_priority = 31;
    pthread_attr_setschedparam(&attr, &param);
    if (pthread_create(&g_thread_yuv[STREAM_ID2], &attr, yuv_get_proc, (FH_VOID *)&g_uvc_dev[STREAM_ID2]) != 0)
    {
        printf("Error: Create yuv_get_proc thread failed!\n");
    }

#endif

#ifdef USB_CDC_ENABLE
    ret = cdc_init();
#endif

#ifdef USB_AUDIO_ENABLE
    ret = uac_init();
#endif

#ifdef USB_HID_ENABLE
    ret = hid_demo();
#endif

    return ret;

Exit:
    g_uvc_th_status = 0;

    return ret;
}

FH_SINT32 sample_common_uvc_stop(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
#if defined(FH_OVERLAY_ENABLE)
    ret = sample_fh_overlay_stop(0);
    SDK_FUNC_ERROR(model_name, ret);

#ifdef UVC_DOUBLE_STREAM
    ret = sample_fh_overlay_stop(UVC_DUAL_STREAM_GRP);
    SDK_FUNC_ERROR(model_name, ret);
#endif
#endif
    return ret;
}
