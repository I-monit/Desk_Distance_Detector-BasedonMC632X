#include "nna_app/include/sample_common_nna.h"
#include "nna_app/include/algorithm.h"
#include <rtthread.h>

static char *model_name = "face_feature_locate";

static FH_UINT32 g_nna_grpid[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_running[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_stop[MAX_GRP_NUM] = {0};

static FH_SINT32 draw_gbox(FH_SINT32 grp_id, FH_SINT32 chn_id, NN_RESULT *nn_result)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static OSD_GBOX gbox[BOX_MAX_NUM];
    static FH_SINT32 last_num = 0;
    FH_SINT32 i = 0;
    FH_UINT32 chn_w = get_vpu_chn_w(grp_id, chn_id);
    FH_UINT32 chn_h = get_vpu_chn_h(grp_id, chn_id);

    for (i = 0; i < nn_result->result_det.nn_result_num; i++)
    {
        gbox[i].gbox_id = i;
        gbox[i].enable = 1;
        gbox[i].color.fAlpha = 255;
        gbox[i].color.fRed = 0;
        gbox[i].color.fGreen = 255;
        gbox[i].color.fBlue = 0;
        gbox[i].x = nn_result->result_det.nn_result_info[i].box.x * chn_w;
        gbox[i].y = nn_result->result_det.nn_result_info[i].box.y * chn_h;
        gbox[i].w = nn_result->result_det.nn_result_info[i].box.w * chn_w;
        gbox[i].h = nn_result->result_det.nn_result_info[i].box.h * chn_h;

        ret = sample_set_gbox(grp_id, chn_id, &gbox[i]);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);
    }

    for (; i < last_num; i++)
    {
        gbox[i].enable = 0;

        ret = sample_set_gbox(grp_id, chn_id, &gbox[i]);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);
    }

    last_num = nn_result->result_det.nn_result_num;

    return ret;
}

static FH_VOID clear_gbox(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    OSD_GBOX gbox;

    memset(&gbox, 0, sizeof(gbox));

    for (FH_SINT32 i = 0; i < BOX_MAX_NUM; i++)
    {
        gbox.gbox_id = i;
        gbox.enable = 0;
        sample_set_gbox(grp_id, chn_id, &gbox);
    }
}

FH_VOID *nna_face_feature_locate(FH_VOID *args)
{
    FH_SINT32 ret;
    NN_HANDLE nn_handle = NULL;
    NN_TYPE nn_type;
    FH_CHAR model_path[256];
    VPU_INFO *vpu_info = NULL;
    VPU_CHN_INFO *vpu_chn_info = NULL;
    VPU_CHN_INFO *vpu_out_chn_info = NULL;
    NN_INPUT_DATA snap_input;
    NN_RESULT snap_result;
    FH_SINT32 grp_id = *((FH_SINT32 *)args);
    FH_UINT32 handle_lock = 0;
    FH_CHAR thread_name[20];
    FH_VIDEO_FRAME vpuStr;

    vpu_info = get_vpu_config(grp_id);

    if (vpu_info->enable)
    {
        vpu_chn_info = get_vpu_chn_config(grp_id, NN_CHN);
        if (!vpu_chn_info->enable)
        {
            SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable !\n", grp_id, NN_CHN);
            goto Exit;
        }
    }
    else
    {
        SDK_ERR_PRT(model_name, "VPU GRP[%d] not enable !\n", grp_id);
        goto Exit;
    }

    nn_type = PERSON_DET;
    vpu_out_chn_info = get_vpu_chn_config(grp_id, 0);
    if (!vpu_out_chn_info->enable)
    {
        vpu_out_chn_info = NULL;
    }
    memset(&model_path, 0, sizeof(model_path));
    strcpy(model_path, "./models/persondet.nbg");
    ret = nna_init(grp_id, &nn_handle, nn_type, model_path, vpu_chn_info->width, vpu_chn_info->height);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    memset(&snap_input, 0, sizeof(snap_input));
    snap_input.width = vpu_chn_info->width;
    snap_input.height = vpu_chn_info->height;
    snap_input.stride = vpu_chn_info->width;

    SDK_PRT(model_name, "nna_face_feature_locate[%d] Start!\n", grp_id);

    g_nna_running[grp_id] = 1;

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "nna_face_ft_%d", grp_id);
    prctl(PR_SET_NAME, thread_name);
    sleep(1);
    while (!g_nna_stop[grp_id])
    {
        ret = lock_vpu_stream(grp_id, NN_CHN, &vpuStr, 0, &handle_lock, 0);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = getVPUStreamToNN(vpuStr, &snap_input);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = nna_get_obj_det_result(nn_handle, snap_input, &snap_result);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        /* Phase4 钩子: k 算法 NN 输入. NN_RESULT_DET_INFO 归一化 box/conf 转 ALG_BBOX,
         * 时间戳用 rt_tick_get_millisecond 与 TOF 侧(alg_glue)同源配对(±200ms) */
        {
            ALG_BBOX alg_boxes[8];
            FH_SINT32 alg_n = snap_result.result_det.nn_result_num;

            if (alg_n > (FH_SINT32)(sizeof(alg_boxes) / sizeof(alg_boxes[0])))
            {
                alg_n = (FH_SINT32)(sizeof(alg_boxes) / sizeof(alg_boxes[0]));
            }

            for (FH_SINT32 ai = 0; ai < alg_n; ai++)
            {
                alg_boxes[ai].x    = snap_result.result_det.nn_result_info[ai].box.x;
                alg_boxes[ai].y    = snap_result.result_det.nn_result_info[ai].box.y;
                alg_boxes[ai].w    = snap_result.result_det.nn_result_info[ai].box.w;
                alg_boxes[ai].h    = snap_result.result_det.nn_result_info[ai].box.h;
                alg_boxes[ai].conf = snap_result.result_det.nn_result_info[ai].conf;
            }

            alg_input_nn(rt_tick_get_millisecond(), alg_boxes, alg_n,
                         vpu_chn_info->width, vpu_chn_info->height);
        }

        /* 打印节流: detect_num 变化或每50帧输出一次 (全量打印 25fps*5行 刷屏会导致串口工具丢包) */
        {
            static FH_SINT32 last_det_num = -1;
            FH_SINT32 det_num = snap_result.result_det.nn_result_num;

            if (det_num == last_det_num && (snap_input.frame_id % 50) != 0)
            {
                goto SkipPrint;
            }
            last_det_num = det_num;

            SDK_PRT(model_name, "frame[%lld] detect_num=%d\n",
                    snap_input.frame_id, det_num);

            for (FH_SINT32 i = 0; i < det_num; i++)
            {
            FH_FLOAT box_x = snap_result.result_det.nn_result_info[i].box.x;
            FH_FLOAT box_y = snap_result.result_det.nn_result_info[i].box.y;
            FH_FLOAT box_w = snap_result.result_det.nn_result_info[i].box.w;
            FH_FLOAT box_h = snap_result.result_det.nn_result_info[i].box.h;
            FH_SINT32 tl_x = (FH_SINT32)(box_x * vpu_chn_info->width + 0.5f);
            FH_SINT32 tl_y = (FH_SINT32)(box_y * vpu_chn_info->height + 0.5f);
            FH_SINT32 br_x = (FH_SINT32)((box_x + box_w) * vpu_chn_info->width + 0.5f);
            FH_SINT32 br_y = (FH_SINT32)((box_y + box_h) * vpu_chn_info->height + 0.5f);

            SDK_PRT(model_name, "  obj[%d]: class_id=%d conf=%.2f box_norm=(%.3f,%.3f,%.3f,%.3f)\n",
                    i,
                    snap_result.result_det.nn_result_info[i].class_id,
                    snap_result.result_det.nn_result_info[i].conf,
                    box_x, box_y, box_w, box_h);

            SDK_PRT(model_name, "    corners[%dx%d]: TL=(%d,%d) TR=(%d,%d) BR=(%d,%d) BL=(%d,%d)\n",
                    vpu_chn_info->width, vpu_chn_info->height,
                    tl_x, tl_y, br_x, tl_y, br_x, br_y, tl_x, br_y);

            if (vpu_out_chn_info)
            {
                FH_SINT32 o_tl_x = (FH_SINT32)(box_x * vpu_out_chn_info->width + 0.5f);
                FH_SINT32 o_tl_y = (FH_SINT32)(box_y * vpu_out_chn_info->height + 0.5f);
                FH_SINT32 o_br_x = (FH_SINT32)((box_x + box_w) * vpu_out_chn_info->width + 0.5f);
                FH_SINT32 o_br_y = (FH_SINT32)((box_y + box_h) * vpu_out_chn_info->height + 0.5f);

                SDK_PRT(model_name, "    corners_chn0[%dx%d]: TL=(%d,%d) TR=(%d,%d) BR=(%d,%d) BL=(%d,%d)\n",
                        vpu_out_chn_info->width, vpu_out_chn_info->height,
                        o_tl_x, o_tl_y, o_br_x, o_tl_y, o_br_x, o_br_y, o_tl_x, o_br_y);
            }
            }
        }

SkipPrint:
        ret = draw_gbox(grp_id, 0, &snap_result);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = unlock_vpu_stream(grp_id, NN_CHN, handle_lock, &vpuStr);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);
    }

    clear_gbox(grp_id, 0);

    ret = nna_release(grp_id, &nn_handle);
    SDK_FUNC_ERROR_PRT(model_name, ret);

    SDK_PRT(model_name, "nna_face_feature_locate[%d] End!\n", grp_id);

Exit:
    g_nna_running[grp_id] = 0;

    return NULL;
}

FH_SINT32 sample_fh_nna_face_feature_locate_start(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    pthread_t thread;
    pthread_attr_t attr;

    if (g_nna_running[grp_id])
    {
        SDK_PRT(model_name, "nna_face_feature_locate[%d] already running!\n", grp_id);
        return FH_SDK_SUCCESS;
    }

    g_nna_grpid[grp_id] = grp_id;
    g_nna_stop[grp_id] = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    pthread_attr_setstacksize(&attr, 256 * 1024);
#endif
    if (pthread_create(&thread, &attr, nna_face_feature_locate, &g_nna_grpid[grp_id]))
    {
        SDK_ERR_PRT(model_name, "pthread_create failed\n");
        goto Exit;
    }

    return ret;

Exit:
    return -1;
}

FH_SINT32 sample_fh_nna_face_feature_locate_stop(FH_SINT32 grp_id)
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
        SDK_PRT(model_name, "nna_face_feature_locate[%d] not running!\n", grp_id);
    }

    return ret;
}