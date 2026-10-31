#include "venc/include/sample_common_venc.h"

static char *model_name = MODEL_TAG_DEMO_VENC;

static FH_UINT32 g_venc_grip[MAX_GRP_NUM] = {0};
static FH_SINT32 g_venc_running[MAX_GRP_NUM] = {0};
static FH_SINT32 g_venc_stop[MAX_GRP_NUM] = {0};

#ifdef FH_APP_CHANGE_DSP_RESOLUTION
static FH_SINT32 changeDSPResolution(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static FH_SINT64 changeCount = 0;
    FH_VPU_VO_MODE voMode = 0;
    FH_VENC_TYPE encType = 0;
    FH_VENC_RC_MODE rcType = 0;
    FH_SINT32 width = 0;
    FH_SINT32 height = 0;
    ENC_INFO *mjpeg_info;
    ENC_INFO *enc_info;
    VPU_INFO *vpu_info;
    VPU_CHN_INFO *vpu_chn_info;

    if (changeCount % 2 == 0)
    {
        width = 1920;
        height = 1088;
    }
    else
    {
        width = 1280;
        height = 720;
    }

    vpu_info = get_vpu_config(grp_id);
    vpu_chn_info = get_vpu_chn_config(grp_id, chn_id);
    enc_info = get_enc_config(grp_id, chn_id);
    mjpeg_info = get_mjpeg_config(grp_id, chn_id);

    voMode = vpu_chn_info->yuv_type;
    encType = enc_info->enc_type;
    rcType = enc_info->rc_type;

    if (vpu_info->bgm_enable)
    {
        ret = bgm_exit(grp_id);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = unbind_vpu_chn(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_chn_relase(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    ret = enc_chn_relase(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    if (mjpeg_info->enable)
    {
        ret = mjpeg_chn_relase(grp_id, chn_id);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = changeVpuChnAttr(grp_id, chn_id, voMode, width, height);
    SDK_FUNC_ERROR(model_name, ret);

    if (vpu_info->bgm_enable)
    {
        ret = bgm_init(grp_id);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = changeEncAttr(grp_id, chn_id, encType, rcType, width, height);
    SDK_FUNC_ERROR(model_name, ret);

    if (mjpeg_info->enable)
    {
        ret = changeMjpegAttr(grp_id, chn_id, width, height);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = vpu_bind_enc(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    if (mjpeg_info->enable)
    {
        ret = vpu_bind_jpeg(grp_id, chn_id, mjpeg_info->channel);
        SDK_FUNC_ERROR(model_name, ret);
    }

    if (vpu_info->bgm_enable)
    {
        ret = vpu_bind_bgm(grp_id);
        SDK_FUNC_ERROR(model_name, ret);
    }

    changeCount++;

    return ret;
}
#endif

#ifdef FH_APP_CHANGE_ISP_FPS
static FH_SINT32 changeIspFps(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static FH_SINT64 changeCount = 0;
    FH_SINT32 fps = 0;
    ISP_INFO *isp_info;

    isp_info = get_isp_config(grp_id);
    if (!isp_info->enable)
    {
        SDK_ERR_PRT(model_name, "changeIspFps Failed! ISP[%d] not enable!\n", grp_id);
        return FH_SDK_FAILED;
    }

    if (changeCount % 2 == 0)
    {
        fps = 25;
    }
    else
    {
        fps = 30;
    }

    ret = changeSensorFPS(grp_id, fps);
    SDK_FUNC_ERROR(model_name, ret);

    changeCount++;

    return ret;
}
#endif

#ifdef FH_APP_CHANGE_ISP_RESOLUTION
static FH_SINT32 changeIspResolution(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static FH_SINT64 changeCount = 0;
    ISP_INFO *isp_info;
    static FH_SINT32 isp_format_bak;

    isp_info = get_isp_config(grp_id);
    if (!isp_info->enable)
    {
        SDK_ERR_PRT(model_name, "changeIspResolution Failed! ISP[%d] not enable!\n", grp_id);
        return FH_SDK_FAILED;
    }

    if (!changeCount)
    {
        isp_format_bak = isp_info->isp_format;
    }

    if (changeCount % 2 == 0)
    {
        isp_info->isp_format = FORMAT_1080P25;
    }
    else
    {
        isp_info->isp_format = isp_format_bak;
    }

    ret = change_isp_format(grp_id, isp_format_bak);
    SDK_FUNC_ERROR(model_name, ret);

    changeCount++;

    return ret;
}
#endif

#ifdef FH_APP_ENC_ROTATE
static FH_SINT32 encRotate(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ENC_INFO *enc_info;
    static FH_SINT64 changeCount = 0;

    enc_info = get_enc_config(grp_id, chn_id);
    if (!enc_info->enable)
    {
        SDK_ERR_PRT(model_name, "encRotate Failed! VPU[%d] CHN[%d] enc not enable!\n", grp_id, chn_id);
        return FH_SDK_FAILED;
    }

    ret = enc_stop(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    switch (changeCount % 4)
    {
    case 0:
        SDK_PRT(model_name, "encRotate 0\n");
        ret = set_vpu_chn_rotate(grp_id, chn_id, 0);
        SDK_FUNC_ERROR(model_name, ret);

        ret = set_enc_rotate(grp_id, chn_id, 0);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case 1:
        SDK_PRT(model_name, "encRotate 90\n");
        ret = set_vpu_chn_rotate(grp_id, chn_id, 90);
        SDK_FUNC_ERROR(model_name, ret);

        ret = set_enc_rotate(grp_id, chn_id, 90);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case 2:
        SDK_PRT(model_name, "encRotate 180\n");
        ret = set_vpu_chn_rotate(grp_id, chn_id, 180);
        SDK_FUNC_ERROR(model_name, ret);

        ret = set_enc_rotate(grp_id, chn_id, 180);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    case 3:
        SDK_PRT(model_name, "encRotate 270\n");
        ret = set_vpu_chn_rotate(grp_id, chn_id, 270);
        SDK_FUNC_ERROR(model_name, ret);

        ret = set_enc_rotate(grp_id, chn_id, 270);
        SDK_FUNC_ERROR(model_name, ret);
        break;
    }

    ret = enc_start(grp_id, chn_id);
    SDK_FUNC_ERROR(model_name, ret);

    changeCount++;

    return ret;
}
#endif

#ifdef FH_APP_CAPTURE_JPEG
/* jpeg抓图 */
static FH_SINT32 capture_jpeg(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    VPU_CHN_INFO *vpu_chn_info = NULL;
    FH_UINT32 chn_w = 0;
    FH_UINT32 chn_h = 0;
    FH_UINT32 jpeg_chn;

    vpu_chn_info = get_vpu_chn_config(grp_id, chn_id);
    if (!vpu_chn_info->enable)
    {
        SDK_ERR_PRT(model_name, "capture_jpeg Failed! VPU[%d] CHN[%d] enc not enable!\n", grp_id, chn_id);
        return FH_SDK_FAILED;
    }

    chn_w = get_vpu_chn_w(grp_id, chn_id);
    chn_h = get_vpu_chn_h(grp_id, chn_id);

    ret = jpeg_create_chn(chn_w, chn_h);
    SDK_FUNC_ERROR(model_name, ret);

    jpeg_chn = get_jpeg_chn();

    ret = vpu_bind_jpeg(grp_id, chn_id, jpeg_chn);
    SDK_FUNC_ERROR(model_name, ret);

    ret = getJpegStream();
    SDK_FUNC_ERROR_PRT(model_name, ret);

    ret = unbind_jpeg_chn(jpeg_chn);
    SDK_FUNC_ERROR(model_name, ret);

#ifdef UVC_ENABLE
    ret = jpeg_destroy();
    SDK_FUNC_ERROR(model_name, ret);
#endif

    return ret;
}
#endif

#ifdef FH_APP_TOOGLE_FREEZE
static FH_SINT32 toggle_freeze(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static FH_SINT64 changeCount = 0;

    if (changeCount % 2)
    {
        SDK_PRT(model_name, "VPU[%d] unfreeze!\n", grp_id);
        ret = vpu_grp_freeze(grp_id, 0);
        SDK_FUNC_ERROR(model_name, ret);
    }
    else
    {
        SDK_PRT(model_name, "VPU[%d] freeze!\n", grp_id);
        ret = vpu_grp_freeze(grp_id, 1);
        SDK_FUNC_ERROR(model_name, ret);
    }

    changeCount++;

    return ret;
}
#endif

static FH_VOID *venc_task(FH_VOID *args)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 grp_id = *((FH_SINT32 *)args);
    FH_SINT32 chn_id = VENC_DEMO_CHN;
    VPU_INFO *vpu_info;
    VPU_CHN_INFO *vpu_chn_info;
    FH_SINT32 cmd = -1;
    FH_CHAR thread_name[20];

    vpu_info = get_vpu_config(grp_id);
    vpu_chn_info = get_vpu_chn_config(grp_id, chn_id);

    if (!vpu_info->enable || !vpu_chn_info->enable)
    {
        ret = FH_SDK_FAILED;
        SDK_ERR_PRT(model_name, "VPU[%d] or VPU_CHN[%d] not enable 0x[%x]!\n", grp_id, chn_id, ret);
        goto Exit;
    }

    g_venc_running[grp_id] = 1;

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "demo_venc_%d", grp_id);
    prctl(PR_SET_NAME, thread_name);
    while (!g_venc_stop[grp_id])
    {
        cmd++;
        usleep(1 * 1000 * 1000); // 1s

        switch (cmd)
        {
        case 0:
#ifdef FH_APP_CHANGE_DSP_RESOLUTION
            ret = changeDSPResolution(grp_id, chn_id);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
            break;
        case 1:
#ifdef FH_APP_CHANGE_ISP_FPS
            ret = changeIspFps(grp_id);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
            break;
        case 2:
#ifdef FH_APP_CHANGE_ISP_RESOLUTION
            ret = changeIspResolution(grp_id);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
            break;
        case 3:
#ifdef FH_APP_ENC_ROTATE
            ret = encRotate(grp_id, chn_id);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
            break;
        case 4:
#ifdef FH_APP_TOOGLE_FREEZE
            ret = toggle_freeze(grp_id);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
            break;
        default:
            cmd = -1;
            break;
        }
    }

Exit:
    g_venc_running[grp_id] = 0;

    return NULL;
}

#ifdef FH_APP_CAPTURE_JPEG
static FH_VOID *jpeg_task(FH_VOID *args)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 grp_id = *((FH_SINT32 *)args);
    FH_SINT32 chn_id = VENC_DEMO_CHN;
    VPU_INFO *vpu_info;
    VPU_CHN_INFO *vpu_chn_info;
    FH_CHAR thread_name[20];
    FH_UINT64 time_now = 0;
    FH_UINT64 time_bak = 0;
    FH_UINT64 time_lapse = 0;

    vpu_info = get_vpu_config(grp_id);
    vpu_chn_info = get_vpu_chn_config(grp_id, chn_id);

    if (!vpu_info->enable || !vpu_chn_info->enable)
    {
        ret = FH_SDK_FAILED;
        SDK_ERR_PRT(model_name, "VPU[%d] or VPU_CHN[%d] not enable 0x[%x]!\n", grp_id, chn_id, ret);
        goto Exit;
    }

    g_venc_running[grp_id] = 1;

    time_bak = time_now = get_ms();
    time_lapse = 1000 / FH_APP_JPEG_FPS;

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "demo_venc_%d", grp_id);
    prctl(PR_SET_NAME, thread_name);
    while (!g_venc_stop[grp_id])
    {
        time_now = get_ms();
        if (time_now - time_bak > time_lapse)
        {
            time_bak += time_lapse;
            ret = capture_jpeg(grp_id, chn_id);
            SDK_FUNC_ERROR_PRT(model_name, ret);
        }
        else
        {
            usleep(10 * 1000);
        }
    }

Exit:
    g_venc_running[grp_id] = 0;

    return NULL;
}
#endif

FH_SINT32 sample_fh_venc_start(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    pthread_t thread;
    pthread_attr_t attr;

    if (g_venc_running[grp_id])
    {
        SDK_PRT(model_name, "Venc Demo[%d] already running!\n", grp_id);
        return FH_SDK_SUCCESS;
    }

    g_venc_grip[grp_id] = grp_id;
    g_venc_stop[grp_id] = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    pthread_attr_setstacksize(&attr, 8 * 1024);
#endif
    if (pthread_create(&thread, &attr, venc_task, &g_venc_grip[grp_id]))
    {
        SDK_ERR_PRT(model_name, "pthread_create failed\n");
        goto Exit;
    }
#ifdef FH_APP_CAPTURE_JPEG
    if (pthread_create(&thread, &attr, jpeg_task, &g_venc_grip[grp_id]))
    {
        SDK_ERR_PRT(model_name, "pthread_create failed\n");
        goto Exit;
    }
#endif
    return ret;

Exit:
    return -1;
}

FH_SINT32 sample_fh_venc_stop(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (g_venc_running[grp_id])
    {
        g_venc_stop[grp_id] = 1;

        while (g_venc_running[grp_id])
        {
            usleep(10);
        }
    }
    else
    {
        SDK_PRT(model_name, "Venc Demo[%d] not running!\n", grp_id);
    }

    return ret;
}
