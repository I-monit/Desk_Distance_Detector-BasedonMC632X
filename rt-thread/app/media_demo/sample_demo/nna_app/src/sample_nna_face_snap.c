#include "nna_app/include/sample_common_nna.h"

static char *model_name = MODEL_TAG_DEMO_NNA_FACE_SNAP;

#define DRAW_BOX_CHAN (0)

static pthread_mutex_t g_list_lock[MAX_GRP_NUM];
static FH_UINT32 g_nna_grpid[MAX_GRP_NUM] = {0};

static FH_SINT32 g_nna_running[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_stop[MAX_GRP_NUM] = {0};

static FH_SINT32 g_enc_running[MAX_GRP_NUM] = {0};
static FH_SINT32 g_enc_stop[MAX_GRP_NUM] = {0};

typedef struct fh_face_snap
{
    FH_UINT32 id;
    FH_UINT64 timestamp;
    NN_RESULT_FACE_SNAP best_face_info;
    FH_VOID *enc_y;
    FH_VOID *enc_uv;
    FH_UINT32 enc_y_size;
    FH_UINT32 enc_uv_size;
    struct fh_face_snap *next;
    struct fh_face_snap *pre;
} FH_FACE_SNAP;

typedef struct fh_alg_tools
{
    FH_SINT32 id;
    FHA_FACEQUALITY_PROCESSOR best_face_alg;
} FH_ALG_TOOLS;

FH_FACE_SNAP *g_face_snap_head[MAX_GRP_NUM];

static FH_FACE_SNAP *face_snap_list_init(FH_VOID)
{
    FH_FACE_SNAP *head = (FH_FACE_SNAP *)malloc(sizeof(FH_FACE_SNAP));
    SDK_CHECK_NULL_PTR(model_name, head);

    head->next = NULL;
    head->pre = head;

    return head;
Exit:
    return NULL;
}

static FH_SINT32 face_snap_list_add(FH_SINT32 grp_id, FH_FACE_SNAP **head, FH_FACE_SNAP **item)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_FACE_SNAP *tmp = NULL;

    SDK_CHECK_NULL_PTR(model_name, *head);
    SDK_CHECK_NULL_PTR(model_name, *item);

    tmp = (*head);
    while (tmp->next)
    {
        tmp = tmp->next;
    }

    pthread_mutex_lock(&g_list_lock[grp_id]);
    tmp->next = (*item);
    (*item)->pre = tmp;
    pthread_mutex_unlock(&g_list_lock[grp_id]);

    return ret;
Exit:
    return FH_SDK_FAILED;
}

static FH_FACE_SNAP *face_snap_list_get(FH_SINT32 grp_id, FH_FACE_SNAP **head)
{
    FH_FACE_SNAP *item = NULL;

    SDK_CHECK_NULL_PTR(model_name, *head);

    pthread_mutex_lock(&g_list_lock[grp_id]);
    item = (*head)->next;
    if (item)
    {
        (*head)->next = item->next;
        if (item->next)
        {
            item->next->pre = (*head);
        }
    }
    pthread_mutex_unlock(&g_list_lock[grp_id]);

    return item;
Exit:
    return NULL;
}

static FH_SINT32 face_snap_list_free(FH_FACE_SNAP **item)
{
    SDK_CHECK_NULL_PTR(model_name, *item);

    (*item)->id = 0;
    (*item)->timestamp = 0;
    memset(&(*item)->best_face_info, 0, sizeof(NN_RESULT_FACE_SNAP));
    SDK_FREE_PTR((*item)->enc_y);
    SDK_FREE_PTR((*item)->enc_uv);
    (*item)->enc_y_size = 0;
    (*item)->enc_uv_size = 0;
    (*item)->next = NULL;
    (*item)->pre = NULL;
    SDK_FREE_PTR(*item);

    return FH_SDK_SUCCESS;
Exit:
    return FH_SDK_FAILED;
}

static FH_FACE_SNAP *face_snap_list_node_create(FH_VIDEO_FRAME vpuStr, NN_RESULT_FACE_SNAP *best_face_info, void *stream_cache_luma, void *stream_cache_chroma)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_FACE_SNAP *new_node = NULL;
    static FH_UINT32 id = 0;

    new_node = (FH_FACE_SNAP *)malloc(sizeof(FH_FACE_SNAP));
    SDK_CHECK_NULL_PTR(model_name, new_node);

    new_node->id = id++;
    new_node->timestamp = vpuStr.time_stamp;
    memcpy(&new_node->best_face_info, best_face_info, sizeof(NN_RESULT_FACE_SNAP));

    new_node->enc_y_size = vpuStr.frm_scan.luma.data.size;
    new_node->enc_y = malloc(vpuStr.frm_scan.luma.data.size);
    SDK_CHECK_NULL_PTR(model_name, new_node->enc_y);
    memcpy(new_node->enc_y, stream_cache_luma, new_node->enc_y_size);

    new_node->enc_uv_size = vpuStr.frm_scan.chroma.data.size;
    new_node->enc_uv = malloc(vpuStr.frm_scan.chroma.data.size);
    SDK_CHECK_NULL_PTR(model_name, new_node->enc_uv);
    memcpy(new_node->enc_uv, stream_cache_chroma, new_node->enc_uv_size);

    ret = FH_SYS_VmmFlushCache(0, stream_cache_luma, vpuStr.frm_scan.luma.data.size);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    ret = FH_SYS_VmmFlushCache(0, stream_cache_chroma, vpuStr.frm_scan.chroma.data.size);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    new_node->next = NULL;
    new_node->pre = NULL;

    return new_node;

Exit:
    SDK_FREE_PTR(new_node->enc_y);
    SDK_FREE_PTR(new_node->enc_uv);
    SDK_FREE_PTR(new_node);

    return NULL;
}

static FH_SINT32 draw_face_box(unsigned char *luma, unsigned char *chroma, unsigned int frame_w, unsigned int frame_h, unsigned int dt_x, unsigned int dt_y, unsigned int dt_w, unsigned int dt_h)
{
    FH_SINT32 i, j;
    FH_UINT8 *luma_start;
    FH_UINT16 *chroma_start;
    FH_UINT16 stride = frame_w;
    FH_UINT8 y = 150;
    FH_UINT16 c = (43 << 8) | 21;
    FH_SINT32 line_len = 2;

    if (dt_x >= frame_w || dt_y >= frame_h || (dt_x + dt_w) < 0 || (dt_y + dt_h) < 0)
    {
        SDK_PRT(model_name, "%s %d %d %d %d %d %d\n", __func__, frame_w, frame_h, dt_x, dt_y, dt_w, dt_h);
        return 0;
    }

    if (dt_x < 0)
        dt_x = 0;

    if (dt_y < 0)
        dt_y = 0;

    if ((dt_x + dt_w) > frame_w)
        dt_w = frame_w - dt_x;

    if ((dt_y + dt_h) > frame_h)
        dt_h = frame_h - dt_y;

    // SDK_PRT(model_name, "face %d %d %d %d\n", dt_x, dt_y, dt_w, dt_h);

    luma_start = luma + dt_y * stride + dt_x;

    for (i = 0; i < line_len; i++)
    {
        memset(luma_start, y, dt_w);
        luma_start += stride;
    }

    chroma_start = (unsigned short *)(chroma + dt_y / 2 * stride + dt_x);

    for (i = 0; i < line_len / 2; i++)
    {
        for (j = 0; j < dt_w / 2; j++)
            chroma_start[j] = c;
        chroma_start += (stride / 2);
    }

    luma_start = luma + (dt_y + dt_h) * stride + dt_x;

    for (i = 0; i < line_len; i++)
    {
        memset(luma_start, y, dt_w);
        luma_start += stride;
    }

    chroma_start = (unsigned short *)(chroma + (dt_y + dt_h) / 2 * stride + dt_x);
    for (i = 0; i < line_len / 2; i++)
    {
        for (j = 0; j < dt_w / 2; j++)
            chroma_start[j] = c;
        chroma_start += (stride / 2);
    }

    luma_start = luma + dt_y * stride + dt_x;
    for (i = 0; i < dt_h; i++)
    {
        memset(luma_start, y, line_len);
        memset(luma_start + dt_w, y, line_len);
        luma_start += stride;
    }

    chroma_start = (unsigned short *)(chroma + dt_y / 2 * stride + dt_x);
    for (i = 0; i < dt_h / 2; i++)
    {
        for (j = 0; j < line_len / 2; j++)
        {
            chroma_start[j] = c;
            chroma_start[dt_w / 2 + j] = c;
        }
        chroma_start += (stride / 2);
    }

    return FH_SDK_SUCCESS;
}

static FH_SINT32 draw_face_point(FH_UINT8 *luma, FH_UINT8 *chroma, FH_UINT32 frame_w, FH_UINT32 frame_h, FH_UINT32 x, FH_UINT32 y)
{
    FH_UINT32 start_x, start_y;
    FH_UINT32 point_w, point_h;
    FH_UINT8 *luma_start;
    FH_UINT16 *chroma_start;
    FH_UINT8 y_value = 156;
    FH_UINT16 c_value = (43 << 8) | 21;
    FH_SINT32 i, j;

    if (x > frame_w || y > frame_h || x < 0 || y < 0)
    {
        return 0;
    }

    start_x = x & (~15);
    start_y = y & (~15);

    point_w = (start_x + 16) > frame_w ? (frame_w - start_x) : 16;
    point_h = (start_y + 16) > frame_h ? (frame_h - start_y) : 16;

    luma_start = luma + start_y * frame_w + start_x;
    for (i = 0; i < point_h; i++)
    {
        memset(luma_start, y_value, point_w);
        luma_start += frame_w;
    }

    chroma_start = (unsigned short *)(chroma + start_y / 2 * frame_w + start_x);
    for (i = 0; i < point_h / 2; i++)
    {
        for (j = 0; j < point_w / 2; j++)
        {
            chroma_start[j] = c_value;
        }

        chroma_start += (frame_w / 2);
    }

    return FH_SDK_SUCCESS;
}

/*
 *
 * frame_w chn_w
 * frame_h chn_h
 * dt_x,y,w,h face box
 * ext_w,h draw area
 *
 */
static FH_SINT32 draw_face_label(FH_VOID *src_y_addr, FH_VOID *src_uv_addr, FH_VOID *dst_y_addr, FH_VOID *dst_uv_addr, FH_UINT32 frame_w, FH_UINT32 frame_h, FH_UINT32 dt_x, FH_UINT32 dt_y, FH_UINT32 dt_w, FH_UINT32 dt_h, FH_UINT32 ext_w, FH_UINT32 ext_h)
{
    static FH_UINT32 start_x = 0;
    FH_SINT32 j = 0;
    FH_UINT8 *src = NULL;
    FH_UINT8 *dst = NULL;

    if (dt_x >= frame_w || dt_y >= frame_h || (dt_x + dt_w) < 0 || (dt_y + dt_h) < 0)
        return FH_SDK_FAILED;

    if (dt_x < 0)
        dt_x = 0;

    if (dt_y < 0)
        dt_y = 0;

    if ((dt_x + dt_w) > frame_w)
        dt_w = frame_w - dt_x;

    if ((dt_y + dt_h) > frame_h)
        dt_h = frame_h - dt_y;

    ext_w = ext_w & (~3);
    ext_h = ext_h & (~3);
    dt_x = (dt_x + 3) & (~3);
    dt_y = (dt_y + 3) & (~3);
    dt_w = (dt_w + 7) & (~7);
    dt_h = (dt_h + 7) & (~7);

    if (dt_w < ext_w && dt_h < ext_h)
    {
        src = src_y_addr + dt_y * frame_w + dt_x;
        dst = dst_y_addr + frame_w * frame_h + start_x;

        for (j = 0; j < dt_h; j++)
        {
            memset(dst, 0, ext_w);
            memcpy(dst, src, dt_w);
            src += frame_w;
            dst += frame_w;
        }

        for (; j < ext_h; j++)
        {
            memset(dst, 0, ext_w);
            dst += frame_w;
        }

        src = src_uv_addr + frame_w * dt_y / 2 + dt_x;
        dst = dst_uv_addr + frame_h * frame_w / 2 + start_x;

        for (j = 0; j < (dt_h + 1) / 2; j++)
        {
            memset(dst, 0, ext_w);
            memcpy(dst, src, dt_w);
            src += frame_w;
            dst += frame_w;
        }

        for (; j < (ext_h + 1) / 2; j++)
        {
            memset(dst, 0, ext_w);
            dst += frame_w;
        }

        start_x += ext_w;

        if (start_x >= (frame_w - ext_w))
            start_x = 0;
    }
    else
    {
        luma_resize(dst_y_addr + frame_w * frame_h + start_x, ext_w, ext_h, frame_w, src_y_addr, frame_w, frame_h, frame_w,
                    dt_x, dt_y, dt_w, dt_h);

        chroma_resize(dst_uv_addr + frame_w * frame_h / 2 + start_x, ext_w, ext_h, frame_w, src_uv_addr, frame_w, frame_h, frame_w,
                      dt_x, dt_y, dt_w, dt_h);

        start_x += ext_w;

        if (start_x >= (frame_w - ext_w))
            start_x = 0;
    }

    return FH_SDK_SUCCESS;
}

static FH_SINT32 get_best_face(FH_ALG_TOOLS *fh_alg_tools, NN_INPUT_DATA snap_input, NN_RESULT *snap_result)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FHIMGPROC_Run_Face_Quality_Param run_param;
    FHA_DET_BBOX_EXT_s best_face_in;
    FHA_FACEQUALITY_OUT_s best_face_out;

    run_param.width = snap_input.width;
    run_param.height = snap_input.height;
    // quality param
    run_param.quality_param_quality_th = (FH_FLOAT)(FH_FACE_CAPTURE_TH / 1000);
    run_param.quality_param_pose_alpha = 0.06;
    run_param.quality_param_roll_th = FH_FACE_ROLL_TH;
    run_param.quality_param_yaw_th = FH_FACE_YAW_TH;
    run_param.quality_param_pitch_th = FH_FACE_PITCH_TH;
    run_param.quality_param_hold_time = 2;

    FH_FLOAT pose_weight[3] = {0.5, 1, 2};
    FH_FLOAT occlude_weight[5] = {2, 2, 1, 1, 1};
    memcpy(run_param.quality_param_pose_weight, pose_weight, sizeof(run_param.quality_param_pose_weight));
    memcpy(run_param.quality_param_occlude_weight, occlude_weight, sizeof(run_param.quality_param_occlude_weight));

    // trk param
    run_param.trk_param_iouThresh = 10; // 0 - 100, match threshold
    run_param.trk_param_maxAge = 10;
    run_param.trk_param_maxTrackNum = 10;
    run_param.trk_param_minHits = 1;
    run_param.trk_param_type = FHIMGPROC_TRK_TYPE_SORT;
    run_param.rotate_angle = FN_ROT_0;

    if (fh_alg_tools->best_face_alg == NULL)
    {
        fh_alg_tools->best_face_alg = fhimgproc_face_quality_create(&run_param);
    }

    best_face_in.bbox_num = snap_result->result_face_snap.nn_result_num;

    for (FH_SINT32 i = 0; i < snap_result->result_face_snap.nn_result_num; i++)
    {
        best_face_in.bbox[i].class_id = snap_result->result_face_snap.nn_result_info[i].class_id;
        best_face_in.bbox[i].bbox.x = snap_result->result_face_snap.nn_result_info[i].box.x;
        best_face_in.bbox[i].bbox.y = snap_result->result_face_snap.nn_result_info[i].box.y;
        best_face_in.bbox[i].bbox.w = snap_result->result_face_snap.nn_result_info[i].box.w;
        best_face_in.bbox[i].bbox.h = snap_result->result_face_snap.nn_result_info[i].box.h;
        best_face_in.bbox[i].conf = snap_result->result_face_snap.nn_result_info[i].conf;
        for (FH_SINT32 j = 0; j < LANDMARK_PTS_NUM; j++)
        {
            best_face_in.bbox[i].landmark[j].x = snap_result->result_face_snap.nn_result_info[i].landmark[j].x;
            best_face_in.bbox[i].landmark[j].y = snap_result->result_face_snap.nn_result_info[i].landmark[j].y;
            best_face_in.bbox[i].occlude[j] = snap_result->result_face_snap.nn_result_info[i].occlude[j];
        }
        best_face_in.bbox[i].quality = snap_result->result_face_snap.nn_result_info[i].quality;
    }

    ret = fhimgproc_face_quality_process(fh_alg_tools->best_face_alg, &best_face_in, &best_face_out);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    snap_result->result_face_snap.nn_result_num = 0;
    memset(&snap_result->result_face_snap.nn_result_info, 0, sizeof(snap_result->result_face_snap.nn_result_info));
#if NN_DEBUG
    SDK_PRT(model_name, "*****************get_best_face[Start]%d*****************\n", best_face_out.objNum);
    if (best_face_out.objNum == 1)
    {
        char file_name[64];
        static int num = 0;
        sprintf(file_name, "best_face_%d.raw", num++);
        saveDataToFile(snap_input.data.vbase, snap_input.data.size, file_name);
    }
#endif
    for (FH_SINT32 i = 0; i < best_face_out.objNum; i++)
    {
        // best_face_out.objs[i].is_best_face = 1;
        if (best_face_out.objs[i].is_best_face)
        {
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].class_id = best_face_out.objs[i].clsType;
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.x = best_face_out.objs[i].bbox.x;
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.y = best_face_out.objs[i].bbox.y;
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.w = best_face_out.objs[i].bbox.w;
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.h = best_face_out.objs[i].bbox.h;
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].conf = best_face_out.objs[i].conf;
            for (FH_SINT32 j = 0; j < LANDMARK_PTS_NUM; j++)
            {
                snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].landmark[j].x = best_face_out.objs[i].landmark[j].x;
                snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].landmark[j].y = best_face_out.objs[i].landmark[j].y;
                snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].occlude[j] = best_face_out.objs[i].occlude[j];
            }
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].quality = best_face_out.objs[i].quality;
#if NN_DEBUG
            SDK_PRT(model_name, "[%d] class_id = %d\n", snap_result->result_face_snap.nn_result_num,
                    snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].class_id);
            SDK_PRT(model_name, "[%d] x = %f\n", snap_result->result_face_snap.nn_result_num,
                    snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.x);
            SDK_PRT(model_name, "[%d] y = %f\n", snap_result->result_face_snap.nn_result_num,
                    snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.y);
            SDK_PRT(model_name, "[%d] w = %f\n", snap_result->result_face_snap.nn_result_num,
                    snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.w);
            SDK_PRT(model_name, "[%d] h = %f\n", snap_result->result_face_snap.nn_result_num,
                    snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.h);
            SDK_PRT(model_name, "[%d] conf = %f\n", snap_result->result_face_snap.nn_result_num,
                    snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].conf);
            for (int j = 0; j < LANDMARK_PTS_NUM; j++)
            {
                SDK_PRT(model_name, "[%d] landmark[%d] = (x = %f, y= %f) occlude = %f\n", snap_result->result_face_snap.nn_result_num, j,
                        snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].landmark[j].x,
                        snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].landmark[j].y,
                        snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].occlude[j]);
            }
            SDK_PRT(model_name, "[%d] quality = %f\n\n", snap_result->result_face_snap.nn_result_num,
                    snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].quality);
#endif
            snap_result->result_face_snap.nn_result_num++;
        }
    }
#if NN_DEBUG
    SDK_PRT(model_name, "*****best face num = %d******\n", snap_result->result_face_snap.nn_result_num);
    SDK_PRT(model_name, "*****************get_best_face[End]*****************\n");
#endif

Exit:
    return ret;
}

/*get stream*/
FH_VOID *nna_face_snap(FH_VOID *args)
{
    FH_SINT32 ret;
    NN_HANDLE nn_handle_snap = NULL;
    NN_TYPE nn_type;
    FH_CHAR model_path[256];
    VPU_INFO *vpu_info = NULL;
    VPU_CHN_INFO *vpu_chn_info = NULL;
    VPU_CHN_INFO *vpu_chn0_info = NULL;
    NN_INPUT_DATA snap_input;
    NN_RESULT snap_result;
    FH_SINT32 grp_id = *((FH_SINT32 *)args);
    FH_UINT32 handle_lock = 0;
    FH_UINT32 handle_lock1 = 0;
    FH_CHAR thread_name[20];
    FH_VIDEO_FRAME vpuStr;
    FH_VIDEO_FRAME rgbStr;
    FH_ALG_TOOLS fh_alg_tools;
    FH_FACE_SNAP *item = NULL;

    FH_VOID *stream_cache_luma = NULL;
    FH_VOID *stream_cache_chroma = NULL;

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

    if (vpu_info->enable)
    {
        vpu_chn0_info = get_vpu_chn_config(grp_id, DRAW_BOX_CHAN);
        if (!vpu_chn_info->enable)
        {
            SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable !\n", grp_id, DRAW_BOX_CHAN);
            goto Exit;
        }
    }
    else
    {
        SDK_ERR_PRT(model_name, "VPU GRP[%d] not enable !\n", grp_id);
        goto Exit;
    }

    nn_type = FACE_KPT_DET;
    memset(&model_path, 0, sizeof(model_path));
    strcpy(model_path, "./models/facedet_full.nbg");
    ret = nna_init(grp_id, &nn_handle_snap, nn_type, model_path, vpu_chn_info->width, vpu_chn_info->height);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    ret = unbind_vpu_chn(grp_id, DRAW_BOX_CHAN);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    memset(&snap_input, 0, sizeof(snap_input));
    snap_input.width = vpu_chn_info->width;
    snap_input.height = vpu_chn_info->height;
    snap_input.stride = vpu_chn_info->width;

    memset(&fh_alg_tools, 0, sizeof(fh_alg_tools));
    fh_alg_tools.id = grp_id;

    g_nna_running[grp_id] = 1;
    sprintf(thread_name, "nna_snap_%d", grp_id);
    prctl(PR_SET_NAME, thread_name);
    while (!g_nna_stop[grp_id])
    {
        ret = lock_vpu_stream(grp_id, DRAW_BOX_CHAN, &vpuStr, 1000, &handle_lock, 0);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = lock_vpu_stream(grp_id, NN_CHN, &rgbStr, 0, &handle_lock1, 0);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        if (rgbStr.time_stamp != vpuStr.time_stamp)
        {
            SDK_PRT(model_name, "frame not sync! %lld ; %lld\n", rgbStr.time_stamp, vpuStr.time_stamp);

            ret = unlock_vpu_stream(grp_id, NN_CHN, handle_lock1, &rgbStr);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);

            ret = unlock_vpu_stream(grp_id, DRAW_BOX_CHAN, handle_lock, &vpuStr);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
            continue;
        }

        stream_cache_luma = FH_SYS_MmapCached(vpuStr.frm_scan.luma.data.base, vpuStr.frm_scan.luma.data.size);
        SDK_CHECK_NULL_PTR(model_name, stream_cache_luma);

        stream_cache_chroma = FH_SYS_MmapCached(vpuStr.frm_scan.chroma.data.base, vpuStr.frm_scan.chroma.data.size);
        SDK_CHECK_NULL_PTR(model_name, stream_cache_chroma);

        ret = getVPUStreamToNN(rgbStr, &snap_input);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = nna_get_face_snap_result(nn_handle_snap, snap_input, &snap_result);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = get_best_face(&fh_alg_tools, snap_input, &snap_result);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        for (FH_SINT32 i = 0; i < snap_result.result_face_snap.nn_result_num; i++)
        {
            draw_face_box(stream_cache_luma, stream_cache_chroma,
                          vpu_chn0_info->width,
                          vpu_chn0_info->height,
                          snap_result.result_face_snap.nn_result_info[i].box.x * vpu_chn0_info->width,
                          snap_result.result_face_snap.nn_result_info[i].box.y * vpu_chn0_info->height,
                          snap_result.result_face_snap.nn_result_info[i].box.w * vpu_chn0_info->width,
                          snap_result.result_face_snap.nn_result_info[i].box.h * vpu_chn0_info->height);

            for (FH_SINT32 j = 0; j < LANDMARK_PTS_NUM; j++)
            {
                draw_face_point(stream_cache_luma, stream_cache_chroma,
                                vpu_chn0_info->width,
                                vpu_chn0_info->height,
                                snap_result.result_face_snap.nn_result_info[i].landmark[j].x * vpu_chn0_info->width,
                                snap_result.result_face_snap.nn_result_info[i].landmark[j].y * vpu_chn0_info->height);
            }
        }

        item = face_snap_list_node_create(vpuStr, &snap_result.result_face_snap, stream_cache_luma, stream_cache_chroma);
        SDK_CHECK_NULL_PTR(model_name, item);

        ret = face_snap_list_add(grp_id, &g_face_snap_head[grp_id], &item);
        SDK_FUNC_ERROR_GOTO(model_name, ret);

        ret = FH_SYS_Munmap(stream_cache_luma, vpuStr.frm_scan.luma.data.size);
        SDK_FUNC_ERROR_GOTO(model_name, ret);
        stream_cache_luma = NULL;

        ret = FH_SYS_Munmap(stream_cache_chroma, vpuStr.frm_scan.chroma.data.size);
        SDK_FUNC_ERROR_GOTO(model_name, ret);
        stream_cache_chroma = NULL;

        ret = unlock_vpu_stream(grp_id, NN_CHN, handle_lock1, &rgbStr);
        SDK_FUNC_ERROR_GOTO(model_name, ret);

        ret = unlock_vpu_stream(grp_id, DRAW_BOX_CHAN, handle_lock, &vpuStr);
        SDK_FUNC_ERROR_GOTO(model_name, ret);
    }

    ret = nna_release(grp_id, &nn_handle_snap);
    SDK_FUNC_ERROR_PRT(model_name, ret);

    g_nna_running[grp_id] = 0;

    return NULL;
Exit:
    ret = unlock_vpu_stream(grp_id, NN_CHN, handle_lock1, &rgbStr);
    SDK_FUNC_ERROR_PRT(model_name, ret);

    ret = unlock_vpu_stream(grp_id, DRAW_BOX_CHAN, handle_lock, &vpuStr);
    SDK_FUNC_ERROR_PRT(model_name, ret);

    g_nna_running[grp_id] = 0;

    return NULL;
}

FH_VOID *nna_face_snap_submit(FH_VOID *args)
{
    FH_SINT32 ret;
    FH_SINT32 grp_id = *((FH_SINT32 *)args);
    ENC_INFO *enc_info = NULL;
    VPU_INFO *vpu_info = NULL;
    VPU_CHN_INFO *vpu_chn0_info = NULL;
    FH_ENC_FRAME frame;
    MEM_DESC encBuf[2]; // y[0] ,uv[1]
    FH_FACE_SNAP *item = NULL;
    FH_CHAR thread_name[20];

    vpu_info = get_vpu_config(grp_id);

    if (vpu_info->enable)
    {
        vpu_chn0_info = get_vpu_chn_config(grp_id, DRAW_BOX_CHAN);
        if (!vpu_chn0_info->enable)
        {
            SDK_ERR_PRT(model_name, "VPU[%d] CHN[%d] not enable !\n", grp_id, DRAW_BOX_CHAN);
            goto Exit;
        }
    }
    else
    {
        SDK_ERR_PRT(model_name, "VPU GRP[%d] not enable !\n", grp_id);
        goto Exit;
    }

    enc_info = get_enc_config(grp_id, DRAW_BOX_CHAN);

    if (!enc_info->enable)
    {

        SDK_ERR_PRT(model_name, "ENC [%d] not enable !\n", enc_info->channel);
        goto Exit;
    }

    ret = enc_destroy(grp_id, DRAW_BOX_CHAN);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    enc_info->max_height = vpu_chn0_info->height + vpu_chn0_info->height / 4;
    ret = changeEncAttr(grp_id, DRAW_BOX_CHAN, enc_info->enc_type, enc_info->rc_type, vpu_chn0_info->width, vpu_chn0_info->height + vpu_chn0_info->height / 4);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    buffer_malloc_withname(&encBuf[0], enc_info->width * enc_info->height, 1024, "face_y_buf");
    buffer_malloc_withname(&encBuf[1], enc_info->width * enc_info->height / 2, 1024, "face_uv_buf");

    memset(encBuf[0].vbase, 0, encBuf[0].size);
    memset(encBuf[1].vbase, 0, encBuf[1].size);

    frame.size.u32Width = enc_info->width;
    frame.size.u32Height = enc_info->height;
    frame.lumma_addr = encBuf[0].base;
    frame.chroma_addr = encBuf[1].base;
    frame.pool_id = -1;

    g_enc_running[grp_id] = 1;
    sprintf(thread_name, "nna_snap_sub_%d", grp_id);
    prctl(PR_SET_NAME, thread_name);
    while (!g_enc_stop[grp_id])
    {
        item = face_snap_list_get(grp_id, &g_face_snap_head[grp_id]);
        if (!item)
        {
            usleep(1000);
            continue;
        }

        for (FH_SINT32 i = 0; i < item->best_face_info.nn_result_num; i++)
        {
            draw_face_label(item->enc_y, item->enc_uv, encBuf[0].vbase, encBuf[1].vbase, vpu_chn0_info->width, vpu_chn0_info->height,
                            item->best_face_info.nn_result_info[i].box.x * vpu_chn0_info->width,
                            item->best_face_info.nn_result_info[i].box.y * vpu_chn0_info->height,
                            item->best_face_info.nn_result_info[i].box.w * vpu_chn0_info->width,
                            item->best_face_info.nn_result_info[i].box.h * vpu_chn0_info->height,
                            vpu_chn0_info->height / 4, vpu_chn0_info->height / 4);
        }

        memcpy(encBuf[0].vbase, item->enc_y, item->enc_y_size);
        memcpy(encBuf[1].vbase, item->enc_uv, item->enc_uv_size);

        frame.time_stamp = item->timestamp;
        ret = enc_sendframe(DRAW_BOX_CHAN, &frame, 0);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = face_snap_list_free(&item);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);
    }

Exit:
    g_enc_running[grp_id] = 0;

    return NULL;
}

FH_SINT32 sample_fh_nna_face_snap_start(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    pthread_t nn_thread;
    pthread_attr_t nn_attr;
    pthread_t submit_thread;
    pthread_attr_t submit_attr;

    if (g_nna_running[grp_id] || g_enc_running[grp_id])
    {
        SDK_PRT(model_name, "nna_face_snap[%d] already running!\n", grp_id);

        return FH_SDK_SUCCESS;
    }

    g_nna_grpid[grp_id] = grp_id;

    g_nna_stop[grp_id] = 0;
    g_enc_stop[grp_id] = 0;

    /*init list*/
    g_face_snap_head[grp_id] = face_snap_list_init();
    SDK_CHECK_NULL_PTR(model_name, g_face_snap_head[grp_id]);
    pthread_mutex_init(&g_list_lock[grp_id], NULL);

    pthread_attr_init(&nn_attr);
    pthread_attr_setdetachstate(&nn_attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    pthread_attr_setstacksize(&attr, 256 * 1024);
#endif
    if (pthread_create(&nn_thread, &nn_attr, nna_face_snap, &g_nna_grpid[grp_id]))
    {
        SDK_ERR_PRT(model_name, "nn_pthread_create failed!\n");
        goto Exit;
    }

    pthread_attr_init(&submit_attr);
    pthread_attr_setdetachstate(&submit_attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    pthread_attr_setstacksize(&attr, 256 * 1024);
#endif
    if (pthread_create(&submit_thread, &submit_attr, nna_face_snap_submit, &g_nna_grpid[grp_id]))
    {
        SDK_ERR_PRT(model_name, "submit_pthread_create failed!\n");
        goto Exit;
    }

    return ret;
Exit:
    SDK_FREE_PTR(g_face_snap_head[grp_id]);
    return FH_SDK_FAILED;
}

FH_SINT32 sample_fh_nna_face_snap_stop(FH_SINT32 grp_id)
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
        SDK_PRT(model_name, "nna_face_snap nna[%d] not running!\n", grp_id);
    }

    if (g_enc_running[grp_id])
    {
        g_enc_stop[grp_id] = 1;

        while (g_enc_running[grp_id])
        {
            usleep(10);
        }
    }
    else
    {
        SDK_PRT(model_name, "nna_face_snap enc[%d] not running!\n", grp_id);
    }

    SDK_FREE_PTR(g_face_snap_head[grp_id]);

    return ret;
}