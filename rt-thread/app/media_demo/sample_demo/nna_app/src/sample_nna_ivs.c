#include "nna_app/include/sample_common_nna.h"
#include "fh_trip_area.h"

static char *model_name = MODEL_TAG_DEMO_NNA_IVS;

#define IVS_CHN 0

static FH_UINT32 g_nna_grpid[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_running[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_stop[MAX_GRP_NUM] = {0};

static FH_TASK_RULE_t trip_task_cfg[2];
static FH_TASK_RULE_t peri_task_cfg[2];

static FH_UINT32 current_max_gbox_id[MAX_GRP_NUM] = {0};
static FH_UINT32 g_box_id_start = 4; // 0-3为规则线

static FH_SINT32 clear_screen(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_SINT32 reserve_num)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    OSD_GBOX gbox;
#if (FH_APP_TRIP_NUM > 0) || (FH_APP_PERI_NUM > 0)
    FH_UINT32 chn_w = 0;
    FH_UINT32 chn_h = 0;

    chn_w = get_vpu_chn_w(grp_id, chn_id);
    chn_h = get_vpu_chn_h(grp_id, chn_id);
#endif

    gbox.enable = 0;
    for (FH_SINT32 i = g_box_id_start; i < current_max_gbox_id[grp_id]; i++)
    {
        gbox.gbox_id = i;
        ret = sample_set_gbox(grp_id, chn_id, &gbox);
        SDK_FUNC_ERROR(model_name, ret);
    }

    gbox.color.fAlpha = 255;
    gbox.color.fRed = 255;
    gbox.color.fGreen = 0;
    gbox.color.fBlue = 0;

#if (FH_APP_TRIP_NUM > 0)
    gbox.enable = 1;
    gbox.gbox_id = 0;
    gbox.x = trip_task_cfg[0].trip_cfg.endPoint.x;
    gbox.y = trip_task_cfg[0].trip_cfg.endPoint.y;
    gbox.w = 4;
    gbox.h = chn_h;
    ret = sample_set_gbox(grp_id, chn_id, &gbox);
    SDK_FUNC_ERROR(model_name, ret);
#endif

#if (FH_APP_TRIP_NUM > 1)
    gbox.enable = 1;
    gbox.gbox_id = 1;
    gbox.x = trip_task_cfg[1].trip_cfg.endPoint.x;
    gbox.y = trip_task_cfg[1].trip_cfg.endPoint.y;
    gbox.w = chn_w;
    gbox.h = 4;
    ret = sample_set_gbox(grp_id, chn_id, &gbox);
    SDK_FUNC_ERROR(model_name, ret);
#endif

#if (FH_APP_PERI_NUM > 0)
    gbox.enable = 1;
    gbox.gbox_id = 2;
    gbox.x = peri_task_cfg[0].area_cfg.rectPoint[0].x;
    gbox.y = peri_task_cfg[0].area_cfg.rectPoint[0].y;
    gbox.w = chn_w / 6;
    gbox.h = chn_h * 4 / 6;
    ret = sample_set_gbox(grp_id, chn_id, &gbox);
    SDK_FUNC_ERROR(model_name, ret);
#endif

#if (FH_APP_PERI_NUM > 1)
    gbox.enable = 1;
    gbox.gbox_id = 3;
    gbox.x = peri_task_cfg[1].area_cfg.rectPoint[0].x;
    gbox.y = peri_task_cfg[1].area_cfg.rectPoint[0].y;
    gbox.w = chn_w / 6;
    gbox.h = chn_h * 4 / 6;
    ret = sample_set_gbox(grp_id, chn_id, &gbox);
    SDK_FUNC_ERROR(model_name, ret);
#endif

    return ret;
}

static FH_SINT32 draw_box(FH_SINT32 grp_id, FH_SINT32 chn_id, FH_TAD_GROUP_t tad_in)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    OSD_GBOX gbox;

    gbox.enable = 1;
    gbox.color.fAlpha = 255;
    gbox.color.fRed = 255;
    gbox.color.fGreen = 0;
    gbox.color.fBlue = 0;

    current_max_gbox_id[grp_id] = g_box_id_start;
    for (FH_SINT32 i = 0; i < tad_in.num; i++)
    {
        gbox.gbox_id = current_max_gbox_id[grp_id]++;
        gbox.x = tad_in.object[i].pos.x;
        gbox.y = tad_in.object[i].pos.y;
        gbox.w = tad_in.object[i].pos.w;
        gbox.h = tad_in.object[i].pos.h;
        ret = sample_set_gbox(grp_id, chn_id, &gbox);
        SDK_FUNC_ERROR(model_name, ret);
    }

    return ret;
}

static TAD_HANDLE tad_init(FH_VOID)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    TAD_HANDLE tadHandle = NULL;
    FH_TAD_HANDLE_CFG_t tad_cfg;

    tad_cfg.maxTrackNum = 20;
    tad_cfg.iouThresh = 10;
    tad_cfg.minHits = 3;
    tad_cfg.maxAge = 20;

    ret = FH_TAD_CreatHandle(&tadHandle, &tad_cfg);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    return tadHandle;

Exit:
    return NULL;
}

static FH_VOID tad_deinit(TAD_HANDLE tadHandle)
{
    if (tadHandle)
    {
        FH_TAD_HandleRelease(tadHandle);
    }
}

static FH_SINT32 trip_setting(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

#if FH_APP_TRIP_NUM
    FH_SINT32 trip_curminHits = 2;
    FH_SINT32 trip_preminHits = 2;
    OSD_GBOX gbox;
    FH_UINT32 chn_w = 0;
    FH_UINT32 chn_h = 0;

    gbox.color.fAlpha = 255;
    gbox.color.fRed = 255;
    gbox.color.fGreen = 0;
    gbox.color.fBlue = 0;

    chn_w = get_vpu_chn_w(grp_id, chn_id);
    chn_h = get_vpu_chn_h(grp_id, chn_id);

#if (FH_APP_TRIP_NUM > 0)
    trip_task_cfg[0].ruleid = 0;
    trip_task_cfg[0].enable = 1;
    trip_task_cfg[0].curminHits = trip_curminHits;
    trip_task_cfg[0].preminHits = trip_preminHits;
    trip_task_cfg[0].mode = TAD_TRIP_ALL;
    trip_task_cfg[0].trip_cfg.startPoint.x = chn_w / 2; // 左进右出
    trip_task_cfg[0].trip_cfg.startPoint.y = chn_h;
    trip_task_cfg[0].trip_cfg.endPoint.x = chn_w / 2;
    trip_task_cfg[0].trip_cfg.endPoint.y = 0;

    gbox.enable = 1;
    gbox.gbox_id = 0;
    gbox.x = trip_task_cfg[0].trip_cfg.endPoint.x;
    gbox.y = trip_task_cfg[0].trip_cfg.endPoint.y;
    gbox.w = 4;
    gbox.h = chn_h;
    ret = sample_set_gbox(grp_id, chn_id, &gbox);
    SDK_FUNC_ERROR(model_name, ret);
#endif

#if (FH_APP_TRIP_NUM > 1)
    trip_task_cfg[1].ruleid = 1;
    trip_task_cfg[1].enable = 1;
    trip_task_cfg[1].curminHits = trip_curminHits;
    trip_task_cfg[1].preminHits = trip_preminHits;
    trip_task_cfg[1].mode = TAD_TRIP_ALL;
    trip_task_cfg[1].trip_cfg.startPoint.x = chn_w; // 上出下进
    trip_task_cfg[1].trip_cfg.startPoint.y = chn_h / 2;
    trip_task_cfg[1].trip_cfg.endPoint.x = 0;
    trip_task_cfg[1].trip_cfg.endPoint.y = chn_h / 2;

    gbox.enable = 1;
    gbox.gbox_id = 1;
    gbox.x = trip_task_cfg[1].trip_cfg.endPoint.x;
    gbox.y = trip_task_cfg[1].trip_cfg.endPoint.y;
    gbox.w = chn_w;
    gbox.h = 4;
    ret = sample_set_gbox(grp_id, chn_id, &gbox);
    SDK_FUNC_ERROR(model_name, ret);
#endif
#endif

    return ret;
}

static FH_SINT32 area_setting(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

#if FH_APP_PERI_NUM
    FH_SINT32 peri_curminHits = 2;
    FH_SINT32 peri_preminHits = 2;
    OSD_GBOX gbox;
    FH_UINT32 chn_w = 0;
    FH_UINT32 chn_h = 0;

    gbox.color.fAlpha = 255;
    gbox.color.fRed = 255;
    gbox.color.fGreen = 0;
    gbox.color.fBlue = 0;

    chn_w = get_vpu_chn_w(grp_id, chn_id);
    chn_h = get_vpu_chn_h(grp_id, chn_id);

#if (FH_APP_PERI_NUM > 0)
    peri_task_cfg[0].ruleid = 2;
    peri_task_cfg[0].enable = 1;
    peri_task_cfg[0].curminHits = peri_curminHits;
    peri_task_cfg[0].preminHits = peri_preminHits;
    peri_task_cfg[0].mode = TAD_AREA_ALL;
    peri_task_cfg[0].area_cfg.iTimeMin = 10;
    peri_task_cfg[0].area_cfg.iPointNum = 4;
    peri_task_cfg[0].area_cfg.rectPoint[0].x = chn_w / 6;
    peri_task_cfg[0].area_cfg.rectPoint[0].y = chn_h / 6;
    peri_task_cfg[0].area_cfg.rectPoint[1].x = chn_w * 2 / 6;
    peri_task_cfg[0].area_cfg.rectPoint[1].y = chn_h / 6;
    peri_task_cfg[0].area_cfg.rectPoint[2].x = chn_w * 2 / 6;
    peri_task_cfg[0].area_cfg.rectPoint[2].y = chn_h * 5 / 6;
    peri_task_cfg[0].area_cfg.rectPoint[3].x = chn_w / 6;
    peri_task_cfg[0].area_cfg.rectPoint[3].y = chn_h * 5 / 6;

    gbox.enable = 1;
    gbox.gbox_id = 2;
    gbox.x = peri_task_cfg[0].area_cfg.rectPoint[0].x;
    gbox.y = peri_task_cfg[0].area_cfg.rectPoint[0].y;
    gbox.w = chn_w / 6;
    gbox.h = chn_h * 4 / 6;
    ret = sample_set_gbox(grp_id, chn_id, &gbox);
    SDK_FUNC_ERROR(model_name, ret);
#endif

#if (FH_APP_PERI_NUM > 1)
    peri_task_cfg[1].ruleid = 3;
    peri_task_cfg[1].enable = 1;
    peri_task_cfg[1].curminHits = peri_curminHits;
    peri_task_cfg[1].preminHits = peri_preminHits;
    peri_task_cfg[1].mode = TAD_AREA_ALL;
    peri_task_cfg[1].area_cfg.iTimeMin = 10;
    peri_task_cfg[1].area_cfg.iPointNum = 4;
    peri_task_cfg[1].area_cfg.rectPoint[0].x = chn_w * 4 / 6;
    peri_task_cfg[1].area_cfg.rectPoint[0].y = chn_h / 6;
    peri_task_cfg[1].area_cfg.rectPoint[1].x = chn_w * 5 / 6;
    peri_task_cfg[1].area_cfg.rectPoint[1].y = chn_h / 6;
    peri_task_cfg[1].area_cfg.rectPoint[2].x = chn_w * 5 / 6;
    peri_task_cfg[1].area_cfg.rectPoint[2].y = chn_h * 5 / 6;
    peri_task_cfg[1].area_cfg.rectPoint[3].x = chn_w * 4 / 6;
    peri_task_cfg[1].area_cfg.rectPoint[3].y = chn_h * 5 / 6;

    gbox.enable = 1;
    gbox.gbox_id = 3;
    gbox.x = peri_task_cfg[1].area_cfg.rectPoint[0].x;
    gbox.y = peri_task_cfg[1].area_cfg.rectPoint[0].y;
    gbox.w = chn_w / 6;
    gbox.h = chn_h * 4 / 6;
    ret = sample_set_gbox(grp_id, chn_id, &gbox);
    SDK_FUNC_ERROR(model_name, ret);
#endif
#endif

    return ret;
}

static FH_VOID res_deal(FH_TAD_GROUP_t *tad_data)
{
    FH_SINT32 i = 0;
    for (i = 0; i < tad_data->num; i++)
    {
        if (tad_data->object[i].objId != -1)
        {
            switch (tad_data->object[i].mode)
            {
            case TAD_NO:
                SDK_PRT(model_name, "TAD_NO\n");
                break;
            case TAD_TRIP_LTOR:
                SDK_PRT(model_name, "TAD_TRIP_LTOR\n");
                break;
            case TAD_TRIP_RTOL:
                SDK_PRT(model_name, "TAD_TRIP_RTOL\n");
                break;
            case TAD_TRIP_ALL:
                SDK_PRT(model_name, "TAD_TRIP_ALL\n");
                break;
            case TAD_AREA_ENTER:
                SDK_PRT(model_name, "TAD_AREA_ENTER\n");
                break;
            case TAD_AREA_EXIT:
                SDK_PRT(model_name, "TAD_AREA_EXIT\n");
                break;
            case TAD_AREA_INTRUSION:
                SDK_PRT(model_name, "TAD_AREA_INTRUSION\n");
                break;
            case TAD_AREA_ALL:
                SDK_PRT(model_name, "TAD_AREA_ALL\n");
                break;
            }
        }
    }
}

static FH_SINT32 nna_result_to_tad(FH_SINT32 grp_id, TAD_HANDLE tadHandle, NN_RESULT nn_result)
{
    FH_TAD_GROUP_t tad_in;
    FH_TAD_GROUP_t tad_out;
    FH_UINT32 chn_w = 0;
    FH_UINT32 chn_h = 0;
    FH_SINT32 i = 0;

    if (nn_result.result_det.nn_result_num == 0)
    {
        return FH_SDK_SUCCESS;
    }

    chn_w = get_vpu_chn_w(grp_id, IVS_CHN);
    chn_h = get_vpu_chn_h(grp_id, IVS_CHN);

    tad_in.num = nn_result.result_det.nn_result_num;

    for (i = 0; i < tad_in.num; i++)
    {
        tad_in.object[i].pos.x = nn_result.result_det.nn_result_info[i].box.x * chn_w;
        tad_in.object[i].pos.y = nn_result.result_det.nn_result_info[i].box.y * chn_h;
        tad_in.object[i].pos.w = nn_result.result_det.nn_result_info[i].box.w * chn_w;
        tad_in.object[i].pos.h = nn_result.result_det.nn_result_info[i].box.h * chn_h;
    }

    clear_screen(grp_id, IVS_CHN, FH_APP_TRIP_NUM + FH_APP_PERI_NUM);

    for (i = 0; i < FH_APP_TRIP_NUM; i++)
    {
        FH_TAD_Process(tadHandle, &trip_task_cfg[i], &tad_in, &tad_out);
        draw_box(grp_id, IVS_CHN, tad_in);
        res_deal(&tad_out);
    }

    for (i = 0; i < FH_APP_PERI_NUM; i++)
    {
        FH_TAD_Process(tadHandle, &peri_task_cfg[i], &tad_in, &tad_out);
        draw_box(grp_id, IVS_CHN, tad_in);
        res_deal(&tad_out);
    }

    return FH_SDK_SUCCESS;
}

FH_VOID *nna_ivs(FH_VOID *args)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    TAD_HANDLE tadHandle = NULL;
    NN_HANDLE nn_handle = NULL;
    NN_TYPE nn_type;
    FH_CHAR model_path[256];
    VPU_INFO *vpu_info;
    VPU_CHN_INFO *vpu_chn_info;
    NN_INPUT_DATA nn_input_data;
    NN_RESULT nn_result;
    FH_SINT32 grp_id = *((FH_SINT32 *)args);
    FH_CHAR thread_name[20];
    FH_VIDEO_FRAME pstVpuframeinfo;
    FH_UINT32 handle_lock;

    vpu_info = get_vpu_config(grp_id);

    if (vpu_info->enable)
    {
        vpu_chn_info = get_vpu_chn_config(grp_id, NN_CHN);
        if (!vpu_chn_info->enable)
        {
            SDK_ERR_PRT(model_name, "VPU CHN[%d] not enable !\n", NN_CHN);
            goto Exit;
        }
    }
    else
    {
        SDK_ERR_PRT(model_name, "VPU GRP[%d] not enable !\n", grp_id);
        goto Exit;
    }

#ifdef FH_NN_PERSON_DETECT_ENABLE
    nn_type = PERSON_DET;
    strcpy(model_path, "./models/persondet.nbg");
#elif defined(FH_NN_C2D_ENABLE)
    nn_type = PERSON_VEHICLE_DET;
    strcpy(model_path, "./models/c2det.nbg");
#elif defined(FH_NN_FACE_DET_ENABLE)
    nn_type = FACE_DET;
    strcpy(model_path, "./models/facedet.nbg");
#else
    SDK_ERR_PRT(model_name, "nn_type NULL !\n");
    goto Exit;
#endif

    tadHandle = tad_init(); // tad 初始化
    SDK_CHECK_NULL_PTR(model_name, tadHandle);

    ret = trip_setting(grp_id, IVS_CHN);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    ret = area_setting(grp_id, IVS_CHN);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    ret = nna_init(grp_id, &nn_handle, nn_type, model_path, vpu_chn_info->width, vpu_chn_info->height);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    nn_input_data.width = vpu_chn_info->width;
    nn_input_data.height = vpu_chn_info->height;
    nn_input_data.stride = vpu_chn_info->width;

    SDK_PRT(model_name, "nna_ivs[%d] Start!\n", grp_id);

    g_nna_running[grp_id] = 1;

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "nna_ivs_%d", grp_id);
    prctl(PR_SET_NAME, thread_name);
    sleep(1);
    while (!g_nna_stop[grp_id])
    {
        ret = lock_vpu_stream(grp_id, NN_CHN, &pstVpuframeinfo, 0, &handle_lock, 0);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = getVPUStreamToNN(pstVpuframeinfo, &nn_input_data);
        if (!ret)
        {
            ret = nna_get_obj_det_result(nn_handle, nn_input_data, &nn_result);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);

            ret = unlock_vpu_stream(grp_id, NN_CHN, handle_lock, &pstVpuframeinfo);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);

            ret = nna_result_to_tad(grp_id, tadHandle, nn_result);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
        }
        else
        {
            ret = unlock_vpu_stream(grp_id, NN_CHN, handle_lock, &pstVpuframeinfo);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
            usleep(10 * 1000);
        }
    }

    ret = nna_release(grp_id, &nn_handle);
    SDK_FUNC_ERROR_GOTO(model_name, ret);
    SDK_PRT(model_name, "nna_ivs[%d] End!\n", grp_id);
Exit:
    tad_deinit(tadHandle); // tad 反初始化
    g_nna_running[grp_id] = 0;

    return NULL;
}

FH_SINT32 sample_fh_nna_ivs_start(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    pthread_t thread;
    pthread_attr_t attr;

    if (g_nna_running[grp_id])
    {
        SDK_PRT(model_name, "nna_ivs[%d] already running!\n", grp_id);
        return FH_SDK_SUCCESS;
    }

    g_nna_grpid[grp_id] = grp_id;
    g_nna_stop[grp_id] = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    pthread_attr_setstacksize(&attr, 256 * 1024);
#endif
    if (pthread_create(&thread, &attr, nna_ivs, &g_nna_grpid[grp_id]))
    {
        SDK_ERR_PRT(model_name, "pthread_create failed\n");
        goto Exit;
    }

    return ret;

Exit:
    return -1;
}

FH_SINT32 sample_fh_nna_ivs_stop(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (g_nna_running[grp_id])
    {
        g_nna_stop[grp_id] = 1;

        while (g_nna_running[grp_id])
        {
            usleep(10);
        }
    }
    else
    {
        SDK_PRT(model_name, "nna_ivs[%d] not running!\n", grp_id);
    }

    return ret;
}
