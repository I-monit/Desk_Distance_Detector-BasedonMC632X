#include "fbv_common.h"
#include "fbv_init_nna.h"
#if defined ENABLE_Face_Recognition

#define MAX_NN_MODEL_SIZE (0x81fd86)
extern int load_nbg_file(unsigned char *nn_buf, FH_UINT32 nn_size);

static NN_HANDLE g_nn_handle_det;
static NN_HANDLE g_nn_handle_rec;
static FH_UINT32 *g_face_rec_database = NULL;

static FH_UINT32 g_nna_grpid[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_running[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_stop[MAX_GRP_NUM] = {0};
static FH_UINT32 face_base_size[MAX_GRP_NUM] = {0};

typedef struct fh_alg_tools
{
    FH_SINT32 id;
    FHA_FACEQUALITY_PROCESSOR best_face_alg;
    FHA_FACEALIGN_PROCESSOR face_align_alg;
    FHA_REC_FACEBASE_MPI face_rec_data_base;
} FH_ALG_TOOLS;

typedef struct face_rec_box
{
    FH_SINT32 box_id;
    FH_CHAR label[20];
    FH_SINT32 last_frame;
} FACE_REC_BOX;

FACE_REC_BOX face_box[MAX_GRP_NUM][BOX_MAX_NUM];

static FH_SINT32 chooseFormat(NN_IMAGE_TYPE nn_image_type, FHA_IMAGE_TYPE_e *imageType, FH_SINT32 *chn)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    *imageType = 0;
    *chn = 0;

    switch (nn_image_type)
    {
    case IMAGE_TYPE_RGB888:
        *imageType = FH_IMAGE_FORMAT_RGBA;
        *chn = 4;
        break;
    case IMAGE_TYPE_RRGGBB:
        *imageType = FH_IMAGE_FORMAT_RGB_PLANE;
        *chn = 3;
        break;
    default:
        FBV_FUNC_ERROR_PRT("NNA imageType[%d] not found!\n", *imageType);
        break;
    }

    if (*imageType == 0)
    {
        ret = FH_SDK_FAILED;
    }

    return ret;
}

int fbv_nbg_num_get(FH_UINT32 *p)
{
    if (p == NULL)
    {
        rt_kprintf("invalid pointer!\n");
        return -1;
    }

    return *p;
}

/* idx = 1 indicates obtaining the size of the first model, and so on */
FH_UINT32 fbv_nbg_size_get(FH_UINT32 *p, FH_UINT32 idx)
{
    if (p == NULL)
    {
        rt_kprintf("invalid pointer!\n");
        return -1;
    }

    FH_UINT32 *temp = p;

    temp = temp + idx;

    return *temp;
}

static FH_SINT32 get_best_face(FH_ALG_TOOLS *fh_alg_tools, NN_INPUT_DATA snap_input, NN_RESULT *snap_result)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FHIMGPROC_Run_Face_Quality_Param run_param;
    FHA_DET_BBOX_EXT_s best_face_in;
    FHA_FACEQUALITY_OUT_s best_face_out;

    run_param.width = snap_input.width;
    run_param.height = snap_input.height;
    run_param.quality_param_pose_alpha = 0.06;
    run_param.quality_param_quality_th = (FH_FLOAT)(FH_FACE_CAPTURE_TH / 1000.0);
    run_param.quality_param_roll_th = FH_FACE_ROLL_TH;
    run_param.quality_param_yaw_th = FH_FACE_YAW_TH;
    run_param.quality_param_pitch_th = FH_FACE_PITCH_TH;
    run_param.quality_param_hold_time = -1;

    FH_FLOAT pose_weight[3] = {0.5, 1, 2};
    FH_FLOAT occlude_weight[5] = {2, 2, 1, 1, 1};
    rt_memcpy(run_param.quality_param_pose_weight, pose_weight, sizeof(run_param.quality_param_pose_weight));
    rt_memcpy(run_param.quality_param_occlude_weight, occlude_weight, sizeof(run_param.quality_param_occlude_weight));

    run_param.trk_param_iouThresh = 10;
    run_param.trk_param_maxAge = 10;
    run_param.trk_param_maxTrackNum = 10;
    run_param.trk_param_minHits = 1;
    run_param.trk_param_type = FHIMGPROC_TRK_TYPE_SORT;
    run_param.rotate_angle = 0;

    if (fh_alg_tools->best_face_alg == NULL)
    {
        fh_alg_tools->best_face_alg = fhimgproc_face_quality_create(&run_param);
    }

    best_face_in.bbox_num = snap_result->result_face_snap.nn_result_num;

    for (int i = 0; i < snap_result->result_face_snap.nn_result_num; i++)
    {
        best_face_in.bbox[i].class_id = snap_result->result_face_snap.nn_result_info[i].class_id;
        best_face_in.bbox[i].bbox.x = snap_result->result_face_snap.nn_result_info[i].box.x;
        best_face_in.bbox[i].bbox.y = snap_result->result_face_snap.nn_result_info[i].box.y;
        best_face_in.bbox[i].bbox.w = snap_result->result_face_snap.nn_result_info[i].box.w;
        best_face_in.bbox[i].bbox.h = snap_result->result_face_snap.nn_result_info[i].box.h;
        best_face_in.bbox[i].conf = snap_result->result_face_snap.nn_result_info[i].conf;
        for (int j = 0; j < LANDMARK_PTS_NUM; j++)
        {
            best_face_in.bbox[i].landmark[j].x = snap_result->result_face_snap.nn_result_info[i].landmark[j].x;
            best_face_in.bbox[i].landmark[j].y = snap_result->result_face_snap.nn_result_info[i].landmark[j].y;
            best_face_in.bbox[i].occlude[j] = snap_result->result_face_snap.nn_result_info[i].occlude[j];
        }
        best_face_in.bbox[i].quality = snap_result->result_face_snap.nn_result_info[i].quality;
    }

    ret = fhimgproc_face_quality_process(fh_alg_tools->best_face_alg, &best_face_in, &best_face_out);
    FBV_FUNC_ERROR_GOTO("fhimgproc_face_quality_process", ret);

    snap_result->result_face_snap.nn_result_num = 0;
    rt_memset(&snap_result->result_face_snap.nn_result_info, 0, sizeof(snap_result->result_face_snap.nn_result_info));
#if NN_DEBUG
    FBV_SDK_PRT("*****************get_best_face[Start]*****************\n");
#endif
    for (int i = 0; i < best_face_out.objNum; i++)
    {
        if (best_face_out.objs[i].is_best_face)
        {
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].class_id = best_face_out.objs[i].clsType;
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.x = best_face_out.objs[i].bbox.x;
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.y = best_face_out.objs[i].bbox.y;
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.w = best_face_out.objs[i].bbox.w;
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.h = best_face_out.objs[i].bbox.h;
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].conf = best_face_out.objs[i].conf;
            for (int j = 0; j < LANDMARK_PTS_NUM; j++)
            {
                snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].landmark[j].x = best_face_out.objs[i].landmark[j].x;
                snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].landmark[j].y = best_face_out.objs[i].landmark[j].y;
                snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].occlude[j] = best_face_out.objs[i].occlude[j];
            }
            snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].quality = best_face_out.objs[i].quality;
#if NN_DEBUG
            FBV_SDK_PRT("[%d] class_id = %d\n", snap_result->result_face_snap.nn_result_num,
                        snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].class_id);
            FBV_SDK_PRT("[%d] x = %f\n", snap_result->result_face_snap.nn_result_num,
                        (FH_UINT32)snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.x * 100);
            FBV_SDK_PRT("[%d] y = %f\n", snap_result->result_face_snap.nn_result_num,
                        (FH_UINT32)snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.y * 100);
            FBV_SDK_PRT("[%d] w = %f\n", snap_result->result_face_snap.nn_result_num,
                        (FH_UINT32)snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.w * 100);
            FBV_SDK_PRT("[%d] h = %f\n", snap_result->result_face_snap.nn_result_num,
                        (FH_UINT32)snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].box.h * 100);
            FBV_SDK_PRT("[%d] conf = %f\n", snap_result->result_face_snap.nn_result_num,
                        (FH_UINT32)snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].conf * 100);
            for (int j = 0; j < LANDMARK_PTS_NUM; j++)
            {
                FBV_SDK_PRT("[%d] landmark[%d] = (x = %f, y= %f) occlude = %f\n", snap_result->result_face_snap.nn_result_num, j,
                            (FH_UINT32)(snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].landmark[j].x * 100),
                            (FH_UINT32)(snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].landmark[j].y * 100),
                            (FH_UINT32)(snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].occlude[j] * 100));
            }
            FBV_SDK_PRT("[%d] quality = %f\n\n", snap_result->result_face_snap.nn_result_num,
                        snap_result->result_face_snap.nn_result_info[snap_result->result_face_snap.nn_result_num].quality);
#endif
            snap_result->result_face_snap.nn_result_num++;
        }
    }
#if NN_DEBUG
    FBV_SDK_PRT("*****best face num = %d******\n", snap_result->result_face_snap.nn_result_num);
    FBV_SDK_PRT("*****************get_best_face[End]*****************\n");
#endif

Exit:
    return ret;
}

static FH_SINT32 fbv_get_face_rec_input(FH_ALG_TOOLS *fh_alg_tools, NN_RESULT_FACE_SNAP_INFO best_face, NN_INPUT_DATA img_src, NN_INPUT_DATA *rec_input, FH_SINT32 index)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FHA_FACEALIGN_PARAM_s param;
    FHA_FACE_LANDMARK_s face_landmark;
    FHA_IMAGE_s src;
    FHA_IMAGE_s dst;
    FH_SINT32 chn = 0;

    if (fh_alg_tools->face_align_alg == NULL)
    {
        param.width = 112;
        param.height = 112;
        fh_alg_tools->face_align_alg = fhalg_face_align_create(&param);
    }

    src.width = img_src.width;
    src.height = img_src.height;

    ret = chooseFormat(img_src.imageType, &src.imageType, &chn);
    FBV_FUNC_ERROR_PRT("chooseFormat", ret);

    src.stride = src.width * chn;
    src.data = (FH_UINT8 *)img_src.data.vbase;

    dst.width = 112;
    dst.height = 112;
    dst.imageType = src.imageType;
    dst.stride = dst.width * chn;
    if (rec_input->data.vbase == NULL)
    {
        FH_CHAR buffer_name[20];
        rt_kprintf(buffer_name, "face_rec_%d\n", fh_alg_tools->id);
        ret = buffer_malloc_withname(&rec_input->data, dst.width * dst.width * chn, 1024, buffer_name);
        FBV_FUNC_ERROR_PRT("face_rec buffer_malloc_withname", ret);
    }
    dst.data = (FH_UINT8 *)rec_input->data.vbase;

    for (int i = 0; i < LANDMARK_PTS_NUM; i++)
    {
        face_landmark.landmark[i].x = best_face.landmark[i].x;
        face_landmark.landmark[i].y = best_face.landmark[i].y;
    }

    ret = fhalg_face_align_process(fh_alg_tools->face_align_alg, &src, &face_landmark, &dst);
    FBV_FUNC_ERROR_GOTO("fhalg_face_align_process", ret);

    rec_input->frame_id = img_src.frame_id;
    rec_input->width = dst.width;
    rec_input->height = dst.height;
    rec_input->stride = dst.stride;
    rec_input->imageType = img_src.imageType;
    rec_input->pool_id = -1;
    rec_input->timestamp = img_src.timestamp;

Exit:
    return ret;
}

static FH_SINT32 face_save_or_rec(FH_ALG_TOOLS *fh_alg_tools, NN_RESULT_FACE_SNAP_INFO best_face, NN_INPUT_DATA rec_input, NN_RESULT rec_result, FH_UINT32 face_base_size)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FHA_EMBEDDING_s face_data_base_in;
    FHA_REC_FACEINFO_s_MPI out;

    face_data_base_in.scale = rec_result.result_face_rec.scale;
    face_data_base_in.zero_point = rec_result.result_face_rec.zero_point;
    rt_memcpy(face_data_base_in.val, rec_result.result_face_rec.val, rec_result.result_face_rec.length);
    face_data_base_in.length = rec_result.result_face_rec.length;

    if (fh_alg_tools->face_rec_data_base == NULL)
    {
        if (face_base_size <= 0)
        {
            FBV_SDK_PRT("face_base_sise <= 0\n");
            return FH_SDK_FAILED;
        }
        else
        {
            fh_alg_tools->face_rec_data_base = fhalg_face_rec_createfacebasefrombuffer((const FH_UINT8 *)g_face_rec_database, face_base_size);
            if (fh_alg_tools->face_rec_data_base == NULL)
            {
                FBV_SDK_PRT("face_recg.bin not found!\n");
                return FH_SDK_FAILED;
            }
        }
    }

    ret = fhalg_face_rec_probe(fh_alg_tools->face_rec_data_base, &face_data_base_in, &out);
    FBV_FUNC_ERROR_GOTO("fhalg_face_rec_probe", ret);

    if (out.similarity > 0.5)
    {
        FBV_SDK_PRT("Find Label[%s] Similarity = %d%%!\n", out.label, (FH_SINT32)(out.similarity * 100));
    }

Exit:
    return ret;
}

static FH_SINT32 fh_alg_exit(FH_ALG_TOOLS *fh_alg_tools)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    fh_alg_tools->id = 0;
    if (fh_alg_tools->best_face_alg)
    {
        ret = fhimgproc_face_quality_destory(fh_alg_tools->best_face_alg);
        FBV_FUNC_ERROR_PRT("fh_alg_exit", ret);

        fh_alg_tools->best_face_alg = NULL;
    }

    if (fh_alg_tools->face_align_alg)
    {
        ret = fhalg_face_align_destory(fh_alg_tools->face_align_alg);
        FBV_FUNC_ERROR_PRT("fh_alg_exit", ret);

        fh_alg_tools->face_align_alg = NULL;
    }

    if (fh_alg_tools->face_rec_data_base)
    {
        ret = fhalg_face_rec_destroyfacebase(fh_alg_tools->face_rec_data_base);
        FBV_FUNC_ERROR_PRT("fh_alg_exit", ret);

        fh_alg_tools->face_rec_data_base = NULL;
    }

    return ret;
}

static FH_SINT32 fbv_nna_facerec_init(FH_UINT32 *face_base_size)
{
    FH_SINT32 ret = 0;
    NN_HANDLE_t *det_handle_t = (NN_HANDLE_t *)rt_malloc(sizeof(NN_HANDLE_t));
    NN_HANDLE_t *rec_handle_t = (NN_HANDLE_t *)rt_malloc(sizeof(NN_HANDLE_t));
    FH_NN_MODEL_INIT_PARAM_T model_param;
    FH_MEM_INFO pp_conf;
    NnPostInitParam post_param;
    FH_NN_TASK_INIT_PARAM_T task_param;
    MEM_DESC nn_buf;

    FH_UINT32 nn_size = MAX_NN_MODEL_SIZE;

    rt_memset(det_handle_t, 0, sizeof(NN_HANDLE_t));
    rt_memset(rec_handle_t, 0, sizeof(NN_HANDLE_t));

    rt_memset(&post_param, 0, sizeof(post_param));
    rt_memset(&model_param, 0, sizeof(model_param));

    ret = buffer_malloc_withname(&nn_buf, nn_size, 64, "nn_buf");
    FBV_FUNC_ERROR_PRT("nna buffer_malloc_withname", ret);

    if (load_nbg_file((unsigned char *)nn_buf.base, nn_size) < 0)
    {
        rt_kprintf("No NBG file found.\n");
        ret = FH_SDK_FAILED;
        goto Exit;
    }

    FH_UINT32 file_num = fbv_nbg_num_get((FH_UINT32 *)nn_buf.base);
    FH_UINT32 nbg_facedet_full_size = fbv_nbg_size_get((FH_UINT32 *)nn_buf.base, 1);
    FH_UINT32 nbg_face_rec_size = fbv_nbg_size_get((FH_UINT32 *)nn_buf.base, 2);
    *face_base_size = fbv_nbg_size_get((FH_UINT32 *)nn_buf.base, 3);

    if (file_num <= 0 || nbg_facedet_full_size <= 0 || nbg_face_rec_size <= 0 || *face_base_size <= 0)
    {
        rt_kprintf("facerec nbg wrong.\n");
        ret = FH_SDK_FAILED;
        goto Exit;
    }

    g_face_rec_database = (FH_UINT32 *)(nn_buf.vbase + nbg_facedet_full_size + nbg_face_rec_size + sizeof(FH_UINT32) * 4);

    model_param.type = FN_DET_TYPE_FACE_FULL;
    model_param.src_w_in = NN_WIDTH;
    model_param.src_h_in = NN_HEIGHT;
    model_param.src_c_in = 4;
    model_param.nbg_data_mem.vbase = nn_buf.vbase + sizeof(FH_UINT32) * 4;
    model_param.nbg_data_mem.size = nbg_facedet_full_size;

    ret = FH_NNA_Model_Init(&det_handle_t->modelHandle, &model_param, NULL);
    FBV_FUNC_ERROR_GOTO("FH_NNA_Model_Init", ret);

    ret = FH_NNA_GetPpconfig(det_handle_t->modelHandle, &pp_conf);
    FBV_FUNC_ERROR_GOTO("FH_NNA_GetPpconfig", ret);

    post_param.pp_conf_in.base = pp_conf.base;
    post_param.pp_conf_in.vbase = pp_conf.vbase;
    post_param.pp_conf_in.size = pp_conf.size;
    post_param.config_th = 0.8;
    post_param.rotate = FN_ROT_0;

    ret = FH_NNPost_FACE_DET_FULL_Create(&(det_handle_t->postHandle), &post_param);
    FBV_FUNC_ERROR_GOTO("FH_NNPost_FACE_DET_FULL_Create", ret);

    task_param.rotate = FN_ROT_0;
    ret = FH_NNA_Task_Init(&(det_handle_t->tskHandle), det_handle_t->modelHandle, &task_param, NULL);
    FBV_FUNC_ERROR_GOTO("FH_NNA_Task_Init", ret);

    g_nn_handle_det = det_handle_t;

    rt_memset(&post_param, 0, sizeof(post_param));
    rt_memset(&model_param, 0, sizeof(model_param));

    model_param.type = FN_DET_TYPE_FACEREC;
    model_param.src_w_in = 112;
    model_param.src_h_in = 112;
    model_param.src_c_in = 4;
    model_param.nbg_data_mem.vbase = nn_buf.vbase + nbg_facedet_full_size + sizeof(FH_UINT32) * 4;
    model_param.nbg_data_mem.size = nbg_face_rec_size;
    ret = FH_NNA_Model_Init(&rec_handle_t->modelHandle, &model_param, NULL);
    FBV_FUNC_ERROR_GOTO("FH_NNA_Model_Init", ret);

    ret = FH_NNA_GetPpconfig(rec_handle_t->modelHandle, &pp_conf);
    FBV_FUNC_ERROR_GOTO("FH_NNA_GetPpconfig", ret);

    post_param.pp_conf_in.base = pp_conf.base;
    post_param.pp_conf_in.vbase = pp_conf.vbase;
    post_param.pp_conf_in.size = pp_conf.size;
    post_param.config_th = 0.8;
    post_param.rotate = FN_ROT_0;

    ret = FH_NNPost_FACEREC_Create(&(rec_handle_t->postHandle), &post_param);
    FBV_FUNC_ERROR_GOTO("FH_NNPost_FACEREC_Create", ret);

    ret = FH_NNA_Task_Init(&(rec_handle_t->tskHandle), rec_handle_t->modelHandle, &task_param, NULL);
    FBV_FUNC_ERROR_GOTO("FH_NNA_Task_Init", ret);

    g_nn_handle_rec = rec_handle_t;

    return ret;
Exit:
    if (det_handle_t)
    {
        if (det_handle_t->tskHandle)
        {
            ret = FH_NNA_ReleaseTask(det_handle_t->tskHandle);
            FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseTask_det", ret);
        }
        if (det_handle_t->postHandle)
        {
            ret = FH_NNPost_FACE_DET_FULL_Destroy(det_handle_t->postHandle);
            FBV_FUNC_ERROR_PRT("FH_NNPost_PrimaryDet_Destroy_det", ret);
        }
        if (det_handle_t->modelHandle)
        {
            ret = FH_NNA_ReleaseModel(det_handle_t->modelHandle);
            FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseModel_det", ret);
        }
        rt_free(det_handle_t);
    }
    if (rec_handle_t)
    {
        if (rec_handle_t->tskHandle)
        {
            ret = FH_NNA_ReleaseTask(rec_handle_t->tskHandle);
            FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseTask_det", ret);
        }
        if (rec_handle_t->postHandle)
        {
            ret = FH_NNPost_FACEREC_Destroy(rec_handle_t->postHandle);
            FBV_FUNC_ERROR_PRT("FH_NNPost_PrimaryDet_Destroy_det", ret);
        }
        if (rec_handle_t->modelHandle)
        {
            ret = FH_NNA_ReleaseModel(rec_handle_t->modelHandle);
            FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseModel_det", ret);
        }
        rt_free(rec_handle_t);
    }
    return ret;
}

FH_SINT32 fbv_nna_release(FH_VOID)
{
    FH_SINT32 ret = 0;
    NN_HANDLE_t *det_handle_t = NULL;
    NN_HANDLE_t *rec_handle_t = NULL;
    det_handle_t = (NN_HANDLE_t *)g_nn_handle_det;
    rec_handle_t = (NN_HANDLE_t *)g_nn_handle_rec;

    if (det_handle_t == RT_NULL)
    {
        rt_kprintf("func = %s, null pointer\n", __func__);
        return -1;
    }

    if (rec_handle_t == RT_NULL)
    {
        rt_kprintf("func = %s, null pointer\n", __func__);
        return -1;
    }

    if (det_handle_t)
    {
        if (det_handle_t->tskHandle)
        {
            ret = FH_NNA_ReleaseTask(det_handle_t->tskHandle);
            FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseTask_det", ret);
        }
        if (det_handle_t->postHandle)
        {
            ret = FH_NNPost_FACE_DET_FULL_Destroy(det_handle_t->postHandle);
            FBV_FUNC_ERROR_PRT("FH_NNPost_PrimaryDet_Destroy_det", ret);
        }
        if (det_handle_t->modelHandle)
        {
            ret = FH_NNA_ReleaseModel(det_handle_t->modelHandle);
            FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseModel_det", ret);
        }
        rt_free(det_handle_t);
    }
    if (rec_handle_t)
    {
        if (rec_handle_t->tskHandle)
        {
            ret = FH_NNA_ReleaseTask(rec_handle_t->tskHandle);
            FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseTask_det", ret);
        }
        if (rec_handle_t->postHandle)
        {
            ret = FH_NNPost_FACEREC_Destroy(rec_handle_t->postHandle);
            FBV_FUNC_ERROR_PRT("FH_NNPost_PrimaryDet_Destroy_det", ret);
        }
        if (rec_handle_t->modelHandle)
        {
            ret = FH_NNA_ReleaseModel(rec_handle_t->modelHandle);
            FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseModel_det", ret);
        }
        rt_free(rec_handle_t);
    }

    return ret;
}

static void nn_facerec_thrd(void *args)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    NN_HANDLE nn_handle_snap = g_nn_handle_det;
    NN_HANDLE nn_handle_rec = g_nn_handle_rec;
    FH_UINT32 *face_base_size = (FH_UINT32 *)args;
    VPU_INFO *vpu_info;
    VPU_CHN_INFO *vpu_chn_info;
    NN_INPUT_DATA snap_input;
    NN_RESULT snap_result;
    NN_INPUT_DATA rec_input;
    NN_RESULT rec_result;
    FH_VIDEO_FRAME pstVpuframeinfo;
    FH_UINT32 handle_lock;
    FH_ALG_TOOLS fh_alg_tools;

    vpu_info = fbv_get_vpu_config(FH_APP_GRP_ID);

    if (vpu_info->enable)
    {
        vpu_chn_info = fbv_get_vpu_chn_config(FH_APP_GRP_ID, 2);
        if (!vpu_chn_info->enable)
        {
            FBV_SDK_PRT("VPU[%d] CHN[%d] not enable !\n", FH_APP_GRP_ID, 2);
            goto Exit;
        }
    }
    else
    {
        FBV_SDK_PRT("VPU GRP[%d] not enable !\n", FH_APP_GRP_ID);
        goto Exit;
    }

    rt_memset(&rec_input, 0, sizeof(rec_input));
    rt_memset(&snap_input, 0, sizeof(snap_input));
    snap_input.width = vpu_chn_info->width;
    snap_input.height = vpu_chn_info->height;
    snap_input.stride = vpu_chn_info->width;

    rt_memset(&fh_alg_tools, 0, sizeof(fh_alg_tools));
    fh_alg_tools.id = FH_APP_GRP_ID;

    rt_memset(&face_box[FH_APP_GRP_ID], 0, sizeof(face_box[FH_APP_GRP_ID]));

    g_nna_running[FH_APP_GRP_ID] = 1;

    sleep(1);

    while (!g_nna_stop[FH_APP_GRP_ID])
    {
        ret = FH_VPSS_GetChnFrame(FH_APP_GRP_ID, 2, &pstVpuframeinfo, 1000, &handle_lock, 1, 0);
        if (ret)
        {
            rt_kprintf("%s ret = %d\n", __func__, ret);
            continue;
        }

        ret = fbvgetVPUStreamToNN(pstVpuframeinfo, &snap_input);
        if (ret)
        {
            rt_kprintf("%s ret = %d\n", __func__, ret);
            continue;
        }
        ret = fbv_get_face_snap_result(nn_handle_snap, snap_input, &snap_result);
        if (ret)
        {
            rt_kprintf("%s ret = %d\n", __func__, ret);
            continue;
        }
        ret = get_best_face(&fh_alg_tools, snap_input, &snap_result);
        if (ret)
        {
            rt_kprintf("%s ret = %d\n", __func__, ret);
            continue;
        }
        for (int i = 0; i < snap_result.result_face_snap.nn_result_num; i++)
        {
            ret = fbv_get_face_rec_input(&fh_alg_tools, snap_result.result_face_snap.nn_result_info[i], snap_input, &rec_input, i);
            if (ret)
            {
                rt_kprintf("%s ret = %d\n", __func__, ret);
                continue;
            }
            ret = fbv_get_face_rec_result(nn_handle_rec, rec_input, &rec_result);
            if (ret)
            {
                rt_kprintf("%s ret = %d\n", __func__, ret);
                continue;
            }
            ret = face_save_or_rec(&fh_alg_tools, snap_result.result_face_snap.nn_result_info[i], rec_input, rec_result, *face_base_size);
            if (ret)
            {
                rt_kprintf("%s ret = %d\n", __func__, ret);
                continue;
            }
        }

        ret = FH_VPSS_ReleaseChnFrame(FH_APP_GRP_ID, 2, &pstVpuframeinfo, handle_lock);
        if (ret)
        {
            rt_kprintf("%s ret = %d\n", __func__, ret);
            continue;
        }
    }

    ret = fbv_nna_release();
    FBV_FUNC_ERROR_PRT("fbv_nna_release", ret);

    ret = fh_alg_exit(&fh_alg_tools);
    FBV_FUNC_ERROR_PRT("fh_alg_exit", ret);

Exit:
    g_nna_running[FH_APP_GRP_ID] = 0;

    return;
}

void fbv_nna_start_facerec(void)
{
    rt_thread_t tid;
    FH_UINT32 fb_size = 0;
    int ret = 0;

    ret = fbv_nna_facerec_init(&fb_size);
    if (ret < 0)
    {
        return ;
    }

    face_base_size[FH_APP_GRP_ID] = fb_size;

    g_nna_grpid[FH_APP_GRP_ID] = FH_APP_GRP_ID;
    g_nna_stop[FH_APP_GRP_ID] = 0;

    tid = rt_thread_create("fbv_nna_start_facerec", nn_facerec_thrd, &face_base_size[FH_APP_GRP_ID], 1024 * 512, 30, 10);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
}

void fbv_nna_stop_facerec(void)
{
    if (g_nna_running[FH_APP_GRP_ID])
    {
        g_nna_stop[FH_APP_GRP_ID] = 1;

        while (g_nna_running[FH_APP_GRP_ID])
        {
            usleep(10);
        }
    }
    else
    {
        FBV_SDK_PRT("nna_face_rec[%d] not running!\n", FH_APP_GRP_ID);
    }

    return;
}
#endif