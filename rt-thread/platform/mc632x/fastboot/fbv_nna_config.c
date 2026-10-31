#include "fbv_common.h"
#include "fbv_init_nna.h"
#if defined ENABLE_Face_Recognition

FH_SINT32 fbvgetVPUStreamToNN(FH_VIDEO_FRAME chnStr, NN_INPUT_DATA *nn_input_data)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    FBV_SDK_CHECK_NULL_PTR("fbvgetVPUStreamToNN", nn_input_data);

    nn_input_data->frame_id = chnStr.frame_id;
    nn_input_data->pool_id = chnStr.pool_id;
    nn_input_data->timestamp = chnStr.time_stamp;

    switch (chnStr.data_format)
    {
    case VPU_VOMODE_ARGB888:
        nn_input_data->data.base = chnStr.frm_argb8888.data.base;
        nn_input_data->data.vbase = chnStr.frm_argb8888.data.vbase;
        nn_input_data->data.size = chnStr.frm_argb8888.data.size;
        nn_input_data->imageType = IMAGE_TYPE_RGB888;
        break;
    case VPU_VOMODE_RRGGBB:
        nn_input_data->data.base = chnStr.frm_rrggbb.r_data.data.base;
        nn_input_data->data.vbase = chnStr.frm_rrggbb.r_data.data.vbase;
        nn_input_data->data.size = chnStr.frm_rrggbb.r_data.data.size * 3;
        nn_input_data->imageType = IMAGE_TYPE_RRGGBB;
        break;
    default:
        FBV_FUNC_ERROR_PRT("vpu out mode[%d] not found!\n", chnStr.data_format);
        break;
    }
    return ret;

Exit:
    return FH_SDK_FAILED;
}

static FH_SINT32 chooseFormat(NN_IMAGE_TYPE imageType, FH_UINT32 *data_format)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    *data_format = 0;

    switch (imageType)
    {
    case IMAGE_TYPE_RGB888:
        *data_format = VPU_VOMODE_ARGB888;
        break;
    default:
        FBV_FUNC_ERROR_PRT("NNA data_format[%d] not found!\n", *data_format);
        break;
    }

    if (*data_format == 0)
    {
        ret = FH_SDK_FAILED;
    }

    return ret;
}

FH_SINT32 fbv_get_face_snap_result(NN_HANDLE nn_handle, NN_INPUT_DATA nn_input_data, NN_RESULT *nn_result)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    NN_HANDLE_t *handle_t = NULL;
    FH_VIDEO_FRAME nn_src;
    FH_NN_OUT_INFO hw_out_info;
    FH_NN_POST_INPUT_INFO post_input_info;
    FH_EXT_DETECTION_FACEDET_FULL_T post_out;
    FH_UINT32 data_format;

    FBV_SDK_CHECK_NULL_PTR("nna_get_face_snap_result", nn_handle);

    handle_t = (NN_HANDLE_t *)nn_handle;
    FBV_SDK_CHECK_NULL_PTR("nna_get_face_snap_result", handle_t);
    FBV_SDK_CHECK_NULL_PTR("nna_get_face_snap_result", nn_result);

    nn_src.size.u32Width = nn_input_data.width;
    nn_src.size.u32Height = nn_input_data.height;

    ret = chooseFormat(nn_input_data.imageType, &data_format);
    FBV_FUNC_ERROR_PRT("chooseFormat", ret);

    nn_src.data_format = data_format;
    nn_src.frm_argb8888.data.base = nn_input_data.data.base;
    nn_src.frm_argb8888.data.vbase = nn_input_data.data.vbase;
    nn_src.frm_argb8888.data.size = nn_input_data.data.size;
    nn_src.time_stamp = nn_input_data.timestamp;
    nn_src.frame_id = nn_input_data.frame_id;
    nn_src.pool_id = nn_input_data.pool_id;

    ret = FH_NNA_DetectProcess(handle_t->tskHandle, &nn_src, &hw_out_info);
    FBV_FUNC_ERROR_PRT("FH_NNA_DetectProcess", ret);

    rt_memset(&post_input_info, 0, sizeof(post_input_info));
    post_input_info.hw_output_mem.base = hw_out_info.hw_output_mem.base;
    post_input_info.hw_output_mem.vbase = hw_out_info.hw_output_mem.vbase;
    post_input_info.hw_output_mem.size = hw_out_info.hw_output_mem.size;
    post_input_info.current_rotate = hw_out_info.current_rotate;
    post_input_info.current_stamp = hw_out_info.current_stamp;
    post_input_info.frame_id = hw_out_info.frame_id;
    ret = FH_NNPost_FACE_DET_FULL_Process(handle_t->postHandle, &post_input_info, &post_out);
    FBV_FUNC_ERROR_GOTO("FH_NNPost_FACE_DET_FULL_Process", ret);

    nn_result->result_face_snap.nn_result_num = post_out.boxNum;

#if NN_DEBUG
    FBV_SDK_PRT("*****************nna_get_face_snap_result[Start]*****************\n");
    FBV_SDK_PRT("*****face_snap num = %d******\n", nn_result->result_face_snap.nn_result_num);
#endif
    for (int i = 0; i < post_out.boxNum; i++)
    {
        nn_result->result_face_snap.nn_result_info[i].class_id = post_out.detBBox[i].clsType;
        nn_result->result_face_snap.nn_result_info[i].box.x = post_out.detBBox[i].bbox.x;
        nn_result->result_face_snap.nn_result_info[i].box.y = post_out.detBBox[i].bbox.y;
        nn_result->result_face_snap.nn_result_info[i].box.w = post_out.detBBox[i].bbox.w;
        nn_result->result_face_snap.nn_result_info[i].box.h = post_out.detBBox[i].bbox.h;
        nn_result->result_face_snap.nn_result_info[i].conf = post_out.detBBox[i].conf;
        for (int j = 0; j < LANDMARK_PTS_NUM; j++)
        {
            nn_result->result_face_snap.nn_result_info[i].landmark[j].x = post_out.detBBox[i].landmark[j].x;
            nn_result->result_face_snap.nn_result_info[i].landmark[j].y = post_out.detBBox[i].landmark[j].y;
            nn_result->result_face_snap.nn_result_info[i].occlude[j] = post_out.detBBox[i].occlude[j];
        }
        nn_result->result_face_snap.nn_result_info[i].quality = post_out.detBBox[i].quality;
#if NN_DEBUG
        FBV_SDK_PRT("[%d] class_id = %d\n", i, nn_result->result_face_snap.nn_result_info[i].class_id);
        FBV_SDK_PRT("[%d] x = %lld\n", i, (FH_UINT32)(nn_result->result_face_snap.nn_result_info[i].box.x * 100));
        FBV_SDK_PRT("[%d] y = %lld\n", i, (FH_UINT32)(nn_result->result_face_snap.nn_result_info[i].box.y * 100));
        FBV_SDK_PRT("[%d] w = %lld\n", i, (FH_UINT32)(nn_result->result_face_snap.nn_result_info[i].box.w * 100));
        FBV_SDK_PRT("[%d] h = %lld\n", i, (FH_UINT32)(nn_result->result_face_snap.nn_result_info[i].box.h * 100));
        FBV_SDK_PRT("[%d] conf = %lld\n", i, (FH_UINT32)(nn_result->result_face_snap.nn_result_info[i].conf * 100));
        for (int j = 0; j < LANDMARK_PTS_NUM; j++)
        {
            FBV_SDK_PRT("[%d] landmark[%d] = (x = %ld, y= %ld) occlude = %ld\n", i, j,
                        (FH_UINT32)(nn_result->result_face_snap.nn_result_info[i].landmark[j].x * 100),
                        (FH_UINT32)(nn_result->result_face_snap.nn_result_info[i].landmark[j].y * 100),
                        (FH_UINT32)(nn_result->result_face_snap.nn_result_info[i].occlude[j] * 100));
        }
        FBV_SDK_PRT("[%d] quality = %d\n\n", i, (FH_UINT32)(nn_result->result_face_snap.nn_result_info[i].quality * 100));
#endif
    }
#if NN_DEBUG
    FBV_SDK_PRT("*****************nna_get_face_snap_result[End]*****************\n");
#endif

Exit:
    ret = FH_NNA_DetectRelease(handle_t->tskHandle, &hw_out_info);
    FBV_FUNC_ERROR_PRT("FH_NNA_DetectRelease", ret);

    return ret;
}

FH_SINT32 fbv_get_face_rec_result(NN_HANDLE nn_handle, NN_INPUT_DATA nn_input_data, NN_RESULT *nn_result)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    NN_HANDLE_t *handle_t = NULL;
    FH_VIDEO_FRAME nn_src;
    FH_NN_OUT_INFO hw_out_info;
    FH_NN_POST_INPUT_INFO post_input_info;
    FH_EXT_FACEREC_EMBEDDING_T post_out;
    FH_UINT32 data_format;

    FBV_SDK_CHECK_NULL_PTR("nna_get_face_rec_result", nn_handle);

    handle_t = (NN_HANDLE_t *)nn_handle;
    FBV_SDK_CHECK_NULL_PTR("nna_get_face_rec_result", handle_t);
    FBV_SDK_CHECK_NULL_PTR("nna_get_face_rec_result", nn_result);

    nn_src.size.u32Width = nn_input_data.width;
    nn_src.size.u32Height = nn_input_data.height;

    ret = chooseFormat(nn_input_data.imageType, &data_format);
    FBV_FUNC_ERROR_PRT("chooseFormat", ret);

    nn_src.data_format = data_format;
    nn_src.frm_argb8888.data.base = nn_input_data.data.base;
    nn_src.frm_argb8888.data.vbase = nn_input_data.data.vbase;
    nn_src.frm_argb8888.data.size = nn_input_data.data.size;
    nn_src.time_stamp = nn_input_data.timestamp;
    nn_src.frame_id = nn_input_data.frame_id;
    nn_src.pool_id = nn_input_data.pool_id;

    ret = FH_NNA_DetectProcess(handle_t->tskHandle, &nn_src, &hw_out_info);
    FBV_FUNC_ERROR_PRT("FH_NNA_DetectProcess", ret);

    rt_memset(&post_input_info, 0, sizeof(post_input_info));
    post_input_info.hw_output_mem.base = hw_out_info.hw_output_mem.base;
    post_input_info.hw_output_mem.vbase = hw_out_info.hw_output_mem.vbase;
    post_input_info.hw_output_mem.size = hw_out_info.hw_output_mem.size;
    post_input_info.current_rotate = hw_out_info.current_rotate;
    post_input_info.current_stamp = hw_out_info.current_stamp;
    post_input_info.frame_id = hw_out_info.frame_id;
    ret = FH_NNPost_FACEREC_Process(handle_t->postHandle, &post_input_info, &post_out);
    FBV_FUNC_ERROR_GOTO("FH_NNPost_FACEREC_Process", ret);

#if NN_DEBUG
    FBV_SDK_PRT("*****************nna_get_face_rec_result[Start]*****************\n");
#endif
    nn_result->result_face_rec.frame_id = post_out.frame_id;
    nn_result->result_face_rec.time_stamp = post_out.time_stamp;
    nn_result->result_face_rec.scale = post_out.scale;
    nn_result->result_face_rec.zero_point = post_out.zero_point;
    rt_memcpy(nn_result->result_face_rec.val, post_out.val, post_out.length);
    nn_result->result_face_rec.length = post_out.length;
#if NN_DEBUG
    FBV_SDK_PRT("frame_id = %lld\n", nn_result->result_face_rec.frame_id);
    FBV_SDK_PRT("time_stamp = %lld\n", nn_result->result_face_rec.time_stamp);
    FBV_SDK_PRT("scale = %lld\n", nn_result->result_face_rec.scale * 10);
    FBV_SDK_PRT("zero_point = %d\n", nn_result->result_face_rec.zero_point);
    FBV_SDK_PRT("length = %d\n", nn_result->result_face_rec.length);
    FBV_SDK_PRT("*****************nna_get_face_rec_result[End]*****************\n");
#endif

Exit:
    ret = FH_NNA_DetectRelease(handle_t->tskHandle, &hw_out_info);
    FBV_FUNC_ERROR_PRT("FH_NNA_DetectRelease", ret);

    return ret;
}
#endif