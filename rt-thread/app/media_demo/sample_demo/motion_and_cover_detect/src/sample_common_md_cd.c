#include "motion_and_cover_detect/include/sample_common_md_cd.h"
#include "dsp_ext/FHAdv_MD_mpi.h"

static char *model_name = MODEL_TAG_DEMO_MD_CD;

static FH_UINT32 g_md_cd_grip[MAX_GRP_NUM] = {0};
static FH_SINT32 g_md_cd_running[MAX_GRP_NUM] = {0};
static FH_SINT32 g_md_cd_stop[MAX_GRP_NUM] = {0};

#ifdef FH_APP_OPEN_RECT_MOTION_DETECT
static FH_SINT32 md_init(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 vi_w = 0;
    FH_SINT32 vi_h = 0;
    FH_SINT32 block_w = 0;
    FH_SINT32 block_h = 0;
    FHT_MDConfig_t md_config;

    ret = get_vpu_vi_info(grp_id, &vi_w, &vi_h);
    SDK_FUNC_ERROR(model_name, ret);

    block_w = vi_w / MD_AREA_NUM_H;
    block_h = vi_h / MD_AREA_NUM_V;

    ret = FHAdv_MD_Init(grp_id); /* 初始化区域运动检测功能 */
    SDK_FUNC_ERROR(model_name, ret);

    memset(&md_config, 0, sizeof(md_config));
    md_config.enable = 1;                        /* 使能控制 */
    md_config.threshold = 128;                   /* 运动检测阈值，该值越大，检测灵敏度越高 */
    md_config.framedelay = MD_CD_CHECK_INTERVAL; /* 设置检测帧间隔或者时间间隔，默认使用帧间隔，可以调用FHAdv_MD_Set_Check_Mode设置使用时间间隔 */

    /*
     * MD模块支持32个检测区域，
     * 我们的sample程序设置了(MD_AREA_NUM_H*MD_AREA_NUM_V),即4个检测区域,
     * 其他28个区域会被忽略，因为其fWidth,fHeigh为0
     */
    for (FH_SINT32 i = 0; i < MD_AREA_NUM_V; i++)
    {
        for (FH_SINT32 j = 0; j < MD_AREA_NUM_H; j++)
        {
            md_config.md_area[MD_AREA_NUM_H * i + j].fTopLeftX = j * block_w;
            md_config.md_area[MD_AREA_NUM_H * i + j].fTopLeftY = i * block_h;
            md_config.md_area[MD_AREA_NUM_H * i + j].fWidth = block_w;
            md_config.md_area[MD_AREA_NUM_H * i + j].fHeigh = block_h;
        }
    }

    /* 设置区域检测配置 */
    ret = FHAdv_MD_SetConfig(grp_id, &md_config);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

#ifdef FH_APP_OPEN_RECT_MOTION_DETECT_DUMP_RESULT
static FH_VOID handle_md_result(FH_UINT32 result)
{
    if (result == 0)
    {
        printf("\r[INFO]: MD result: NO         ");
    }
    else
    {
        printf("\r[INFO]: MD result: YES");
        for (FH_SINT32 i = 0; i < MD_AREA_NUM_H * MD_AREA_NUM_V; i++)
        {
            if (result & (1 << i))
            {
                printf(",%d", i); /*此区域有运动*/
            }
            else
            {
                printf(", "); /*此区域没有运动*/
            }
        }
    }
}
#endif

static FH_SINT32 md_get_result(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_UINT32 result;

    ret = FHAdv_MD_GetResult(grp_id, &result);
    SDK_FUNC_ERROR(model_name, ret);

#ifdef FH_APP_OPEN_RECT_MOTION_DETECT_DUMP_RESULT
    handle_md_result(result);
#endif

    return ret;
}
#endif

#ifdef FH_APP_OPEN_MOTION_DETECT
static FH_SINT32 md_ex_init(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FHT_MDConfig_Ex_t md_config;

    ret = FHAdv_MD_Ex_Init(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    md_config.threshold = 128;                   /* 运动检测阈值，该值越大，检测灵敏度越高 */
    md_config.framedelay = MD_CD_CHECK_INTERVAL; /* 设置检测帧间隔 | 时间间隔，默认使用帧间隔，可以调用FHAdv_MD_Set_Check_Mode设置使用时间间隔 */
    md_config.enable = 1;

    /* 设置区域检测配置 */
    ret = FHAdv_MD_Ex_SetConfig(grp_id, &md_config);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

#ifdef FH_APP_OPEN_MOTION_DETECT_DUMP_RESULT
static FH_SINT32 handle_md_ex_result(FH_SINT32 grp_id, FH_SINT32 chn_id, FHT_MDConfig_Ex_Result_t *result)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static FH_SINT32 last_num[MAX_GRP_NUM] = {0};
    Motion_BGM_RUNTB_RECT run;
    FH_UINT32 chn_w = 0;
    FH_UINT32 chn_h = 0;
    FH_SINT32 i = 0;
    OSD_GBOX gbox;

    chn_w = get_vpu_chn_w(grp_id, chn_id);
    chn_h = get_vpu_chn_h(grp_id, chn_id);

    gbox.color.fAlpha = 255;
    gbox.color.fRed = 255;
    gbox.color.fGreen = 0;
    gbox.color.fBlue = 0;
    gbox.enable = 1;

    run.md_ex_result_addr = result->start;
    run.horizontal_count = result->horizontal_count;
    run.vertical_count = result->vertical_count;
    run.area_th_w = 1;
    run.area_th_h = 1;
    run.rect_num = MDEX_BOX_NUM;
    run.rect = malloc(sizeof(struct _rect) * MDEX_BOX_NUM);
    SDK_CHECK_NULL_PTR(model_name, run.rect);

    getOrdFromGau(&run, 1 /* MDEx 此处仅能为1 */); /* 框融合 */

    for (i = 0; i < run.rect_num; i++)
    {
        run.rect[i].u32X = run.rect[i].u32X * chn_w / result->horizontal_count;
        run.rect[i].u32Y = run.rect[i].u32Y * chn_h / result->vertical_count;
        run.rect[i].u32Width = run.rect[i].u32Width * chn_w / result->horizontal_count;
        run.rect[i].u32Height = run.rect[i].u32Height * chn_h / result->vertical_count;

        gbox.gbox_id = i;
        gbox.x = run.rect[i].u32X;
        gbox.y = run.rect[i].u32Y;
        gbox.w = run.rect[i].u32Width;
        gbox.h = run.rect[i].u32Height;
        ret = sample_set_gbox(grp_id, chn_id, &gbox);
        SDK_FUNC_ERROR_GOTO(model_name, ret);
    }

    for (; i < last_num[grp_id]; i++)
    {
        gbox.enable = 0;
        gbox.gbox_id = i;
        ret = sample_set_gbox(grp_id, chn_id, &gbox);
        SDK_FUNC_ERROR_GOTO(model_name, ret);
    }

Exit:
    last_num[grp_id] = run.rect_num;

    if (run.rect)
    {
        free(run.rect);
    }

    return ret;
}
#endif

static FH_SINT32 md_ex_get_result(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FHT_MDConfig_Ex_Result_t result;

    ret = FHAdv_MD_Ex_GetResult(grp_id, &result); /* 获取设置全局运动检测结果 */
    SDK_FUNC_ERROR(model_name, ret);

#ifdef FH_APP_OPEN_MOTION_DETECT_DUMP_RESULT
    handle_md_ex_result(grp_id, chn_id, &result);
#endif

    return ret;
}
#endif

#ifdef FH_APP_OPEN_COVER_DETECT
static FH_SINT32 cd_init(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FHT_CDConfig_t cd_config;

    ret = FHAdv_CD_Init(grp_id); /* 初始化遮挡检测 */
    SDK_FUNC_ERROR(model_name, ret);

    /* 设置遮挡检测参数 */
    cd_config.framedelay = MD_CD_CHECK_INTERVAL; /* 设置检测帧间隔 | 时间间隔，默认使用帧间隔，可以调用FHAdv_MD_Set_Check_Mode设置使用时间间隔 */
    cd_config.cd_level = CD_LEVEL_MID;           /* 设置遮挡检测灵敏度 */
    cd_config.enable = 1;

    ret = FHAdv_CD_SetConfig(grp_id, &cd_config);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

#ifdef FH_APP_OPEN_COVER_DETECT_DUMP_RESULT
static FH_VOID handle_cd_result(FH_UINT32 result)
{
    if (result != 0)
    {
        printf("\r[INFO]:  CD result: YES");
    }
    else
    {
        printf("\r[INFO]:  CD result: NO ");
    }

    fflush(stdout);
}
#endif

static FH_SINT32 cd_get_result(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_UINT32 result;

    ret = FHAdv_CD_GetResult(grp_id, &result);
    SDK_FUNC_ERROR(model_name, ret);

#ifdef FH_APP_OPEN_COVER_DETECT_DUMP_RESULT
    handle_cd_result(result);
#endif

    return ret;
}
#endif

static FH_VOID *md_cd_task(FH_VOID *args)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 grp_id = *((FH_SINT32 *)args);
    VPU_INFO *vpu_info;
    VPU_CHN_INFO *vpu_chn_info;
    FH_CHAR thread_name[20];

    vpu_info = get_vpu_config(grp_id);

    if (vpu_info->enable)
    {
        vpu_chn_info = get_vpu_chn_config(grp_id, MD_CD_CHN); // 在CHN0中实现MD_CD
        if (!vpu_chn_info->enable)
        {
            SDK_ERR_PRT(model_name, "VPU CHN[%d] not enable !\n", MD_CD_CHN);
            goto Exit;
        }
    }
    else
    {
        SDK_ERR_PRT(model_name, "VPU GRP[%d] not enable !\n", grp_id);
        goto Exit;
    }

#ifdef FH_APP_OPEN_RECT_MOTION_DETECT
    ret = md_init(grp_id);
    SDK_FUNC_ERROR_GOTO(model_name, ret);
#endif

#ifdef FH_APP_OPEN_MOTION_DETECT
    ret = md_ex_init(grp_id);
    SDK_FUNC_ERROR_GOTO(model_name, ret);
#endif

#ifdef FH_APP_OPEN_COVER_DETECT
    ret = cd_init(grp_id);
    SDK_FUNC_ERROR_GOTO(model_name, ret);
#endif

    g_md_cd_running[grp_id] = 1;

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "demo_md_cd_%d", grp_id);
    prctl(PR_SET_NAME, thread_name);
    while (!g_md_cd_stop[grp_id])
    {
        ret = FHAdv_MD_CD_Check(grp_id); /* 做一次运动、遮挡检测 */
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

#ifdef FH_APP_OPEN_RECT_MOTION_DETECT
        ret = md_get_result(grp_id);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif

#ifdef FH_APP_OPEN_MOTION_DETECT
        ret = md_ex_get_result(grp_id, MD_CD_CHN);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif

#ifdef FH_APP_OPEN_COVER_DETECT
        ret = cd_get_result(grp_id);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);
#endif
        usleep((MD_CD_CHECK_INTERVAL + 10) * 1000); /*这里增加10ms的目的是,确保每次调用FHAdv_MD_CD_Check都能真正做MD/CD的check*/
    }

Exit:
    g_md_cd_running[grp_id] = 0;

    return NULL;
}

FH_SINT32 sample_fh_md_cd_start(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    pthread_t thread;
    pthread_attr_t attr;

    if (g_md_cd_running[grp_id])
    {
        SDK_PRT(model_name, "MD_CD[%d] already running!\n", grp_id);
        return FH_SDK_SUCCESS;
    }

    g_md_cd_grip[grp_id] = grp_id;
    g_md_cd_stop[grp_id] = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    pthread_attr_setstacksize(&attr, 8 * 1024);
#endif
    if (pthread_create(&thread, &attr, md_cd_task, &g_md_cd_grip[grp_id]))
    {
        SDK_ERR_PRT(model_name, "pthread_create failed\n");
        goto Exit;
    }

    return ret;

Exit:
    return -1;
}

FH_SINT32 sample_fh_md_cd_stop(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (g_md_cd_running[grp_id])
    {
        g_md_cd_stop[grp_id] = 1;

        while (g_md_cd_running[grp_id])
        {
            usleep(10);
        }
    }
    else
    {
        SDK_PRT(model_name, "MD_CD[%d] not running!\n", grp_id);
    }

    return ret;
}
