#include "nna_app/include/sample_common_nna.h"

#define CLEAR_FRAME_NUM 40

static char *model_name = MODEL_TAG_DEMO_NNA_FACE_REC;

static FH_UINT32 g_nna_grpid[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_running[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_stop[MAX_GRP_NUM] = {0};

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
    OSD_GBOX gbox;
} FACE_REC_BOX;

FACE_REC_BOX face_box[MAX_GRP_NUM][BOX_MAX_NUM];

static FH_SINT32 clean_box(FH_SINT32 grp_id, FH_SINT32 chn_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    for (int i = 0; i < BOX_MAX_NUM; i++)
    {
        if (strcmp(face_box[grp_id][i].label, ""))
        {
            if (face_box[grp_id][i].last_frame % CLEAR_FRAME_NUM == 0)
            {
                face_box[grp_id][i].gbox.enable = 0;

                ret = sample_set_gbox(grp_id, chn_id, &face_box[grp_id][i].gbox);
                SDK_FUNC_ERROR(model_name, ret);

                memset(&face_box[grp_id][i], 0, sizeof(face_box[grp_id][i]));
            }

            face_box[grp_id][i].last_frame++;
        }
    }

    return ret;
}

static FH_SINT32 draw_box(FH_SINT32 grp_id, FH_SINT32 chn_id, NN_RESULT_FACE_SNAP_INFO best_face, FH_CHAR *label)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_UINT32 chn_w = 0;
    FH_UINT32 chn_h = 0;

    chn_w = get_vpu_chn_w(grp_id, chn_id);
    chn_h = get_vpu_chn_h(grp_id, chn_id);

    for (int i = 0; i < BOX_MAX_NUM; i++)
    {
        if (!strcmp(face_box[grp_id][i].label, label))
        {
            face_box[grp_id][i].last_frame = 1;

            face_box[grp_id][i].gbox.x = best_face.box.x * chn_w;
            face_box[grp_id][i].gbox.y = best_face.box.y * chn_h;
            face_box[grp_id][i].gbox.w = best_face.box.w * chn_w;
            face_box[grp_id][i].gbox.h = best_face.box.h * chn_h;

            ret = sample_set_gbox(grp_id, chn_id, &face_box[grp_id][i].gbox);
            SDK_FUNC_ERROR(model_name, ret);

            return ret;
        }
    }

    for (int i = 0; i < BOX_MAX_NUM; i++)
    {
        if (!strcmp(face_box[grp_id][i].label, ""))
        {
            face_box[grp_id][i].box_id = i;
            memcpy(face_box[grp_id][i].label, label, sizeof(face_box[grp_id][i].label));
            face_box[grp_id][i].last_frame = 1;
            face_box[grp_id][i].gbox.gbox_id = i;
            face_box[grp_id][i].gbox.color.fAlpha = 255;
            face_box[grp_id][i].gbox.color.fRed = 255;
            face_box[grp_id][i].gbox.color.fGreen = 0;
            face_box[grp_id][i].gbox.color.fBlue = 0;
            face_box[grp_id][i].gbox.x = best_face.box.x * chn_w;
            face_box[grp_id][i].gbox.y = best_face.box.y * chn_h;
            face_box[grp_id][i].gbox.w = best_face.box.w * chn_w;
            face_box[grp_id][i].gbox.h = best_face.box.h * chn_h;
            face_box[grp_id][i].gbox.enable = 1;

            ret = sample_set_gbox(grp_id, chn_id, &face_box[grp_id][i].gbox);
            SDK_FUNC_ERROR(model_name, ret);

            return ret;
        }
    }

    SDK_PRT(model_name, "Warning Best Face Num > %d\n", BOX_MAX_NUM);

    return ret;
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
    run_param.quality_param_pose_alpha = 0.06;
    run_param.quality_param_quality_th = (FH_FLOAT)(FH_FACE_CAPTURE_TH / 1000.0);
    run_param.quality_param_roll_th = FH_FACE_ROLL_TH;
    run_param.quality_param_yaw_th = FH_FACE_YAW_TH;
    run_param.quality_param_pitch_th = FH_FACE_PITCH_TH;
    run_param.quality_param_hold_time = -1;

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
    run_param.rotate_angle = FN_ROT_0; //set rotate 270

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
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    snap_result->result_face_snap.nn_result_num = 0;
    memset(&snap_result->result_face_snap.nn_result_info, 0, sizeof(snap_result->result_face_snap.nn_result_info));
#if NN_DEBUG
    SDK_PRT(model_name, "*****************get_best_face[Start]*****************\n");
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
        SDK_ERR_PRT(model_name, "NNA imageType[%d] not found!\n", *imageType);
        break;
    }

    if (*imageType == 0)
    {
        ret = FH_SDK_FAILED;
    }

    return ret;
}

static FH_SINT32 get_face_rec_input(FH_ALG_TOOLS *fh_alg_tools, NN_RESULT_FACE_SNAP_INFO best_face, NN_INPUT_DATA img_src, NN_INPUT_DATA *rec_input, FH_SINT32 index)
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
    SDK_FUNC_ERROR(model_name, ret);

    src.stride = src.width * chn;
    src.data = (FH_UINT8 *)img_src.data.vbase;

    dst.width = 112;
    dst.height = 112;
    dst.imageType = src.imageType;
    dst.stride = dst.width * chn;
    if (rec_input->data.vbase == NULL)
    {
        FH_CHAR buffer_name[20];
        sprintf(buffer_name, "face_rec_%d", fh_alg_tools->id);
        ret = buffer_malloc_withname(&rec_input->data, dst.width * dst.width * chn, 1024, buffer_name);
        SDK_FUNC_ERROR(model_name, ret);
    }
    dst.data = (FH_UINT8 *)rec_input->data.vbase;

    for (int i = 0; i < LANDMARK_PTS_NUM; i++)
    {
        face_landmark.landmark[i].x = best_face.landmark[i].x;
        face_landmark.landmark[i].y = best_face.landmark[i].y;
    }

    ret = fhalg_face_align_process(fh_alg_tools->face_align_alg, &src, &face_landmark, &dst);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    rec_input->frame_id = img_src.frame_id;
    rec_input->width = dst.width;
    rec_input->height = dst.height;
    rec_input->stride = dst.stride;
    rec_input->imageType = img_src.imageType;
    rec_input->pool_id = -1;
    rec_input->timestamp = img_src.timestamp;

#if NN_DEBUG
    char file_name[32];
    SDK_PRT(model_name, "frameid:%lld\n", img_src.frame_id);
    for (int i = 0; i < LANDMARK_PTS_NUM; i++)
    {
        SDK_PRT(model_name, "[%d] x = %f, y= %f\n", i, face_landmark.landmark[i].x, face_landmark.landmark[i].y);
    }
    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "face_rec_512x288_%lld.raw", img_src.frame_id);
    saveDataToFile(src.data, src.width * src.height * chn, file_name);

    memset(file_name, 0, sizeof(file_name));
    sprintf(file_name, "face_rec_112x112[%d]_%lld.raw", index, img_src.frame_id);
    saveDataToFile(dst.data, dst.width * dst.height * chn, file_name);
#endif // NN_DEBUG

Exit:
    return ret;
}

static FH_SINT32 face_save_or_rec(FH_ALG_TOOLS *fh_alg_tools, NN_RESULT_FACE_SNAP_INFO best_face, NN_INPUT_DATA rec_input, NN_RESULT rec_result)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    static FH_SINT32 rec_mode = FH_FACE_REC_MODE;
    FHA_EMBEDDING_s face_data_base_in;
    FHA_REC_FACEINFO_s_MPI out;
    FH_CHAR label[64];

    face_data_base_in.scale = rec_result.result_face_rec.scale;
    face_data_base_in.zero_point = rec_result.result_face_rec.zero_point;
    memcpy(face_data_base_in.val, rec_result.result_face_rec.val, rec_result.result_face_rec.length);
    face_data_base_in.length = rec_result.result_face_rec.length;

    if (rec_mode) // rec
    {
        if (fh_alg_tools->face_rec_data_base == NULL)
        {
            fh_alg_tools->face_rec_data_base = fhalg_face_rec_createfacebase("face_recg.bin");
            if (fh_alg_tools->face_rec_data_base == NULL)
            {
                SDK_PRT(model_name, "face_recg.bin not found,switch to save mode!\n");
                rec_mode = 0;
                return ret;
            }
        }

        ret = fhalg_face_rec_probe(fh_alg_tools->face_rec_data_base, &face_data_base_in, &out);
        SDK_FUNC_ERROR_GOTO(model_name, ret);

        if (out.similarity > 0.5)
        {
            SDK_PRT(model_name, "Find Label[%s] Similarity = %d%%!\n", out.label, (FH_SINT32)(out.similarity * 100));

            ret = draw_box(fh_alg_tools->id, 0, best_face, out.label);
            SDK_FUNC_ERROR_GOTO(model_name, ret);
        }
    }
    else // save
    {
        if (fh_alg_tools->face_rec_data_base == NULL)
        {
            fh_alg_tools->face_rec_data_base = fhalg_face_rec_createfacebase(NULL);
        }

        memset(label, 0, sizeof(label));
        sprintf(label, "pic[%lld].raw", rec_input.frame_id);
        saveDataToFile(rec_input.data.vbase, rec_input.data.size, label);
        memset(label, 0, sizeof(label));
        SDK_PRT(model_name, "Please check pic[%lld] and input Label!\n", rec_input.frame_id);
        scanf("%s", label);

        if (!strcmp(label, "skip"))
        {
            printf("label %s\n", label);
            return ret;
        }
        else
        {
            ret = fhalg_face_rec_register(fh_alg_tools->face_rec_data_base, &face_data_base_in, label);
            SDK_FUNC_ERROR_GOTO(model_name, ret);
            SDK_PRT(model_name, "Register Label %s success!\n", label);

            ret = fhalg_face_rec_save(fh_alg_tools->face_rec_data_base, "face_recg.bin");
            SDK_FUNC_ERROR_GOTO(model_name, ret);
            SDK_PRT(model_name, "Save Face Rec Database success!\n");
        }
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
        SDK_FUNC_ERROR_PRT(model_name, ret);

        fh_alg_tools->best_face_alg = NULL;
    }

    if (fh_alg_tools->face_align_alg)
    {
        ret = fhalg_face_align_destory(fh_alg_tools->face_align_alg);
        SDK_FUNC_ERROR_PRT(model_name, ret);

        fh_alg_tools->face_align_alg = NULL;
    }

    if (fh_alg_tools->face_rec_data_base)
    {
        ret = fhalg_face_rec_destroyfacebase(fh_alg_tools->face_rec_data_base);
        SDK_FUNC_ERROR_PRT(model_name, ret);

        fh_alg_tools->face_rec_data_base = NULL;
    }

    return ret;
}

FH_VOID *nna_face_rec(FH_VOID *args)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    NN_HANDLE nn_handle_snap = NULL;
    NN_HANDLE nn_handle_rec = NULL;
    NN_TYPE nn_type;
    FH_CHAR model_path[256];
    VPU_INFO *vpu_info;
    VPU_CHN_INFO *vpu_chn_info;
    NN_INPUT_DATA snap_input;
    NN_RESULT snap_result;
    NN_INPUT_DATA rec_input;
    NN_RESULT rec_result;
    FH_SINT32 grp_id = *((FH_SINT32 *)args);
    FH_CHAR thread_name[20];
    FH_VIDEO_FRAME pstVpuframeinfo;
    FH_UINT32 handle_lock;
    FH_ALG_TOOLS fh_alg_tools;

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

    nn_type = FACE_KPT_DET;
    memset(&model_path, 0, sizeof(model_path));
    strcpy(model_path, "./models/facedet_full.nbg");

    ret = nna_init(grp_id, &nn_handle_snap, nn_type, model_path, vpu_chn_info->width, vpu_chn_info->height);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    nn_type = FACE_REC;
    memset(&model_path, 0, sizeof(model_path));
    strcpy(model_path, "./models/facerec.nbg");

    ret = nna_init(grp_id, &nn_handle_rec, nn_type, model_path, 112, 112);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    memset(&rec_input, 0, sizeof(rec_input));
    memset(&snap_input, 0, sizeof(snap_input));
    snap_input.width = vpu_chn_info->width;
    snap_input.height = vpu_chn_info->height;
    snap_input.stride = vpu_chn_info->width;

    memset(&fh_alg_tools, 0, sizeof(fh_alg_tools));
    fh_alg_tools.id = grp_id;

    memset(&face_box[grp_id], 0, sizeof(face_box[grp_id]));

    SDK_PRT(model_name, "nna_face_rec[%d] Start!\n", grp_id);

    g_nna_running[grp_id] = 1;

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "nna_face_rec_%d", grp_id);
    prctl(PR_SET_NAME, thread_name);
    sleep(1);
    while (!g_nna_stop[grp_id])
    {
        ret = lock_vpu_stream(grp_id, NN_CHN, &pstVpuframeinfo, 0, &handle_lock, 0);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = getVPUStreamToNN(pstVpuframeinfo, &snap_input);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = nna_get_face_snap_result(nn_handle_snap, snap_input, &snap_result);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = get_best_face(&fh_alg_tools, snap_input, &snap_result); // snap_result -> best_face , NN_INPUT_DATA &rec_input
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        for (int i = 0; i < snap_result.result_face_snap.nn_result_num; i++) // best face num
        {
            ret = get_face_rec_input(&fh_alg_tools, snap_result.result_face_snap.nn_result_info[i], snap_input, &rec_input, i);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);

            ret = nna_get_face_rec_result(nn_handle_rec, rec_input, &rec_result);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);

            ret = face_save_or_rec(&fh_alg_tools, snap_result.result_face_snap.nn_result_info[i], rec_input, rec_result);
            SDK_FUNC_ERROR_CONTINUE(model_name, ret);
        }

        ret = clean_box(grp_id, 0);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);

        ret = unlock_vpu_stream(grp_id, NN_CHN, handle_lock, &pstVpuframeinfo);
        SDK_FUNC_ERROR_CONTINUE(model_name, ret);
    }

    ret = nna_release(grp_id, &nn_handle_snap);
    SDK_FUNC_ERROR_PRT(model_name, ret);

    ret = nna_release(grp_id, &nn_handle_rec);
    SDK_FUNC_ERROR_PRT(model_name, ret);

    ret = fh_alg_exit(&fh_alg_tools);
    SDK_FUNC_ERROR_PRT(model_name, ret);
    SDK_PRT(model_name, "nna_face_rec[%d] End!\n", grp_id);
Exit:
    g_nna_running[grp_id] = 0;

    return NULL;
}

FH_SINT32 sample_fh_nna_face_rec_start(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    pthread_t thread;
    pthread_attr_t attr;

    if (g_nna_running[grp_id])
    {
        SDK_PRT(model_name, "nna_face_rec[%d] already running!\n", grp_id);
        return FH_SDK_SUCCESS;
    }

    g_nna_grpid[grp_id] = grp_id;
    g_nna_stop[grp_id] = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    pthread_attr_setstacksize(&attr, 256 * 1024);
#endif
    if (pthread_create(&thread, &attr, nna_face_rec, &g_nna_grpid[grp_id]))
    {
        SDK_ERR_PRT(model_name, "pthread_create failed\n");
        goto Exit;
    }

    return ret;

Exit:
    return -1;
}

FH_SINT32 sample_fh_nna_face_rec_stop(FH_SINT32 grp_id)
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
        SDK_PRT(model_name, "nna_face_rec[%d] not running!\n", grp_id);
    }

    return ret;
}
