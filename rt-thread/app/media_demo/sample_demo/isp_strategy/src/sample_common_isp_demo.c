#include "isp_strategy/include/sample_common_isp_demo.h"

static char *model_name = MODEL_TAG_DEMO_ISP;

static FH_UINT32 g_isp_demo_grip[MAX_GRP_NUM] = {0};
static FH_SINT32 g_isp_demo_running[MAX_GRP_NUM] = {0};
static FH_SINT32 g_isp_demo_stop[MAX_GRP_NUM] = {0};

#ifdef FH_APP_ISP_MIRROR_FLIP
static FH_SINT32 setMirrorAndFlip(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static FH_SINT64 changeCount = 0;
    FH_UINT32 mirror = 0;
    FH_UINT32 flip = 0;

    switch (changeCount % 4)
    {
    case 0:
        mirror = 1;
        SDK_PRT(model_name, "set Mirror!\n");
        break;
    case 1:
        mirror = 1;
        flip = 1;
        SDK_PRT(model_name, "set Mirror and Flip!\n");
        break;
    case 2:
        flip = 1;
        SDK_PRT(model_name, "set Flip!\n");
        break;
    case 3:
        SDK_PRT(model_name, "reset Mirror and Flip!\n");
        break;
    }

    ret = set_isp_mirror_flip(grp_id, mirror, flip);
    SDK_FUNC_ERROR(model_name, ret);

    changeCount++;

    return ret;
}
#endif

#ifdef FH_APP_CHANGE_AE_MODE
static FH_SINT32 setAEMode(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static FH_SINT64 changeCount = 0;
    FH_UINT32 mode = 0;
    FH_UINT32 intt_us = 0;
    FH_UINT32 gain_level = 0;

    switch (changeCount % 4)
    {
    case 0:
        mode = 0;
        intt_us = FH_SINT32t_1_3;
        gain_level = 0;
        SDK_PRT(model_name, "set AEMode[AUTO] time = %d us,gain_level = %d!\n", intt_us, gain_level);
        break;
    case 1:
        mode = 1;
        intt_us = FH_SINT32t_1_25;
        gain_level = 0;
        SDK_PRT(model_name, "set AEMode[TIME_MANUAL] time = %d us,gain_level = %d!\n", intt_us, gain_level);
        break;
    case 2:
        mode = 2;
        intt_us = FH_SINT32t_1_3;
        gain_level = 50;
        SDK_PRT(model_name, "set AEMode[GAIN_MANUAL] time = %d us,gain_level = %d!\n", intt_us, gain_level);
        break;
    case 3:
        mode = 3;
        intt_us = FH_SINT32t_1_25;
        gain_level = 100;
        SDK_PRT(model_name, "set AEMode[MANUAL] time = %d us,gain_level = %d!\n", intt_us, gain_level);
        break;
    }

    ret = set_isp_ae_mode(grp_id, mode, intt_us, gain_level);
    SDK_FUNC_ERROR(model_name, ret);

    changeCount++;

    return ret;
}
#endif

#ifdef FH_APP_CHANGE_NPMODE
static FH_SINT32 setSharpness(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static FH_SINT64 changeCount = 0;
    FH_UINT32 mode = 0;
    FH_UINT32 value = 0;

    switch (changeCount % 4)
    {
    case 0:
        mode = 0;
        value = 255;
        SDK_PRT(model_name, "set SharpnessMode[MANUAL] value = %d!\n", value);
        break;
    case 1:
        mode = 0;
        value = 16;
        SDK_PRT(model_name, "set SharpnessMode[MANUAL] value = %d!\n", value);
        break;
    case 2:
        mode = 0;
        value = 200;
        SDK_PRT(model_name, "set SharpnessMode[MANUAL] value = %d!\n", value);
        break;
    case 3:
        mode = 1;
        value = 128;
        SDK_PRT(model_name, "set SharpnessMode[MAPPING] value = %d!\n", value);
        break;
    }

    ret = set_isp_sharpeness(grp_id, mode, value);
    SDK_FUNC_ERROR(model_name, ret);

    changeCount++;

    return ret;
}
#endif

#ifdef FH_APP_CHANGE_SATURATION
static FH_SINT32 setSaturation(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static FH_SINT64 changeCount = 0;
    FH_UINT32 mode = 0;
    FH_UINT32 value = 0;

    switch (changeCount % 4)
    {
    case 0:
        mode = 0;
        value = 255;
        SDK_PRT(model_name, "set SaturationMode[MANUAL] value = %d!\n", value);
        break;
    case 1:
        mode = 0;
        value = 16;
        SDK_PRT(model_name, "set SaturationMode[MANUAL] value = %d!\n", value);
        break;
    case 2:
        mode = 0;
        value = 200;
        SDK_PRT(model_name, "set SaturationMode[MANUAL] value = %d!\n", value);
        break;
    case 3:
        mode = 1;
        value = 128;
        SDK_PRT(model_name, "set SaturationMode[MAPPING] value = %d!\n", value);
        break;
    }

    ret = set_isp_saturation(grp_id, mode, value);
    SDK_FUNC_ERROR(model_name, ret);

    changeCount++;

    return ret;
}
#endif

#ifdef FH_APP_CHANGE_CHROMA
static FH_SINT32 setChroma(FH_SINT32 grp_id)
{
    // TODO 暂未实现
    FH_SINT32 ret = FH_SDK_SUCCESS;
    SDK_PRT(model_name, "NOT SUPPORT!\n");
    // static FH_SINT64 changeCount = 0;
    // FH_UINT32 mode = 0;
    // FH_UINT32 value = 0;

    // switch (changeCount % 4)
    // {
    // case 0:
    //     break;
    // case 1:
    //     break;
    // case 2:
    //     break;
    // case 3:
    //     break;
    // }

    // ret = set_isp_sharpeness(grp_id, mode, value);
    // SDK_FUNC_ERROR(model_name, ret);

    // changeCount++;

    return ret;
}
#endif

static FH_VOID *isp_demo_task(FH_VOID *args)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 grp_id = *((FH_SINT32 *)args);
    ISP_INFO *isp_info;
    FH_SINT32 cmd = -1;
    FH_CHAR thread_name[20];

    isp_info = get_isp_config(grp_id);

    if (!isp_info->enable)
    {
        ret = FH_SDK_FAILED;
        SDK_ERR_PRT(model_name, "ISP[%d]not enable 0x[%x]!\n", grp_id, ret);
        goto Exit;
    }

    g_isp_demo_running[grp_id] = 1;

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "demo_isp_demo_%d", grp_id);
    prctl(PR_SET_NAME, thread_name);
    while (!g_isp_demo_stop[grp_id])
    {
        cmd++;
        usleep(1 * 1000 * 1000); // 1s

        switch (cmd)
        {
        case 0:
#ifdef FH_APP_ISP_MIRROR_FLIP
            ret = setMirrorAndFlip(grp_id);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
            break;
        case 1:
#ifdef FH_APP_CHANGE_AE_MODE
            ret = setAEMode(grp_id);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
            break;
        case 2:
#ifdef FH_APP_CHANGE_NPMODE
            ret = setSharpness(grp_id);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
            break;
        case 3:
#ifdef FH_APP_CHANGE_SATURATION
            ret = setSaturation(grp_id);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
            break;
        case 4:
#ifdef FH_APP_CHANGE_CHROMA
            ret = setChroma(grp_id);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
            break;
        default:
            cmd = -1;
            break;
        }
    }

Exit:
    g_isp_demo_running[grp_id] = 0;

    return NULL;
}

FH_SINT32 sample_fh_isp_demo_start(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    pthread_t thread;
    pthread_attr_t attr;

    if (g_isp_demo_running[grp_id])
    {
        SDK_PRT(model_name, "Isp demo[%d] already running!\n", grp_id);
        return FH_SDK_SUCCESS;
    }

    g_isp_demo_grip[grp_id] = grp_id;
    g_isp_demo_stop[grp_id] = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    pthread_attr_setstacksize(&attr, 8 * 1024);
#endif
    if (pthread_create(&thread, &attr, isp_demo_task, &g_isp_demo_grip[grp_id]))
    {
        SDK_ERR_PRT(model_name, "pthread_create failed\n");
        goto Exit;
    }

    return ret;

Exit:
    return -1;
}

FH_SINT32 sample_fh_isp_demo_stop(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (g_isp_demo_running[grp_id])
    {
        g_isp_demo_stop[grp_id] = 1;

        while (g_isp_demo_running[grp_id])
        {
            usleep(10);
        }
    }
    else
    {
        SDK_PRT(model_name, "Isp demo[%d] not running!\n", grp_id);
    }

    return ret;
}
