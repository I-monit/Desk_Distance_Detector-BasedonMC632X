#include "fbv_common.h"
#include "fbv_init_nna.h"

#define MAX_NN_MODEL_SIZE (0x300000)
extern int load_nbg_file(unsigned char *nn_buf, unsigned int nn_size);

static NN_HANDLE g_nn_handle;

static FH_SINT32 fbv_nna_init(FH_VOID)
{
    FH_SINT32 ret = 0;
    NN_HANDLE_t *handle_t = (NN_HANDLE_t *)rt_malloc(sizeof(NN_HANDLE_t));
    FH_NN_MODEL_INIT_PARAM_T model_param;
    FH_MEM_INFO pp_conf;
    NnPostInitParam post_param;
    FH_NN_TASK_INIT_PARAM_T task_param;
    MEM_DESC nn_buf;
    unsigned int nn_size = MAX_NN_MODEL_SIZE;

    rt_memset(&post_param, 0, sizeof(NnPostInitParam));

    ret = buffer_malloc_withname(&nn_buf, nn_size, 64, "nn_buf");
    FBV_FUNC_ERROR_PRT("nna buffer_malloc_withname", ret);

    if (load_nbg_file((unsigned char *)nn_buf.base, nn_size) < 0)
    {
        rt_kprintf("No NBG file found.\n");
        return -1;
    }

    model_param.type = FN_DET_TYPE_PERSON;
    model_param.src_w_in = NN_WIDTH;
    model_param.src_h_in = NN_HEIGHT;
    model_param.src_c_in = 4;
    model_param.nbg_data_mem.vbase = nn_buf.vbase;
    model_param.nbg_data_mem.size = nn_buf.size;
    ret = FH_NNA_Model_Init(&handle_t->modelHandle, &model_param, NULL);
    FBV_FUNC_ERROR_GOTO("FH_NNA_Model_Init", ret);
    ret = FH_NNA_GetPpconfig(handle_t->modelHandle, &pp_conf);
    FBV_FUNC_ERROR_GOTO("FH_NNA_GetPpconfig", ret);

    post_param.pp_conf_in.base = pp_conf.base;
    post_param.pp_conf_in.vbase = pp_conf.vbase;
    post_param.pp_conf_in.size = pp_conf.size;
    post_param.config_th = 0.8;
    post_param.rotate = FN_ROT_0;

    ret = FH_NNPost_PrimaryDet_Create(&(handle_t->postHandle), &post_param);
    FBV_FUNC_ERROR_GOTO("FH_NNPost_PrimaryDet_Create", ret);

    task_param.rotate = FN_ROT_0;
    ret = FH_NNA_Task_Init(&(handle_t->tskHandle), handle_t->modelHandle, &task_param, NULL);
    FBV_FUNC_ERROR_GOTO("FH_NNA_Task_Init", ret);

    g_nn_handle = handle_t;
    return ret;
Exit:
    if (handle_t->tskHandle)
    {
        ret = FH_NNA_ReleaseTask(handle_t->tskHandle);
        FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseTask", ret);
    }

    if (handle_t->postHandle)
    {
        ret = FH_NNPost_PrimaryDet_Destroy(handle_t->postHandle);
        FBV_FUNC_ERROR_PRT("FH_NNPost_PrimaryDet_Destroy", ret);
    }
    if (handle_t->modelHandle)
    {
        ret = FH_NNA_ReleaseModel(handle_t->modelHandle);
        FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseModel", ret);
    }

    rt_free(handle_t);

    return ret;
}

FH_SINT32 fbv_nna_release(FH_VOID)
{
    FH_SINT32 ret = 0;
    NN_HANDLE_t *handle_t = NULL;
    handle_t = (NN_HANDLE_t *)g_nn_handle;

    if (handle_t == RT_NULL)
    {
        rt_kprintf("func = %s, null pointer\n", __func__);
        return -1;
    }

    if (handle_t->tskHandle)
    {
        ret = FH_NNA_ReleaseTask(handle_t->tskHandle);
        FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseTask", ret);
    }

    if (handle_t->postHandle)
    {
        ret = FH_NNPost_PrimaryDet_Destroy(handle_t->postHandle);
        FBV_FUNC_ERROR_PRT("FH_NNPost_PrimaryDet_Destroy", ret);
    }
    if (handle_t->modelHandle)
    {
        ret = FH_NNA_ReleaseModel(handle_t->modelHandle);
        FBV_FUNC_ERROR_PRT("FH_NNA_ReleaseModel", ret);
    }

    rt_free(handle_t);

    return ret;
}

FH_SINT32 fbv_nna_process(FH_VIDEO_FRAME *nn_src, NN_RESULT_DET *nn_result)
{
    FH_SINT32 ret = 0;
    FH_SINT32 i = 0;
    NN_HANDLE_t *handle_t = NULL;
    handle_t = (NN_HANDLE_t *)g_nn_handle;
    FH_NN_OUT_INFO hw_out_info;
    FH_DETECTION_T det_out;
    FH_NN_POST_INPUT_INFO post_input_info;

    if (handle_t == RT_NULL)
    {
        rt_kprintf("func = %s, null pointer\n", __func__);
        return -1;
    }

    ret = FH_NNA_DetectProcess(handle_t->tskHandle, nn_src, &hw_out_info);
    FBV_FUNC_ERROR_PRT("FH_NNA_DetectProcess", ret);

    post_input_info.hw_output_mem.base = hw_out_info.hw_output_mem.base;
    post_input_info.hw_output_mem.vbase = hw_out_info.hw_output_mem.vbase;
    post_input_info.hw_output_mem.size = hw_out_info.hw_output_mem.size;
    post_input_info.current_rotate = hw_out_info.current_rotate;
    post_input_info.current_stamp = hw_out_info.current_stamp;
    post_input_info.frame_id = hw_out_info.frame_id;
    ret = FH_NNPost_PrimaryDet_Process(handle_t->postHandle, &post_input_info, &det_out);
    FBV_FUNC_ERROR_GOTO("FH_NNPost_PrimaryDet_Process", ret);

    nn_result->nn_result_num = det_out.boxNum;
    for (i = 0; i < nn_result->nn_result_num; i++)
    {
        nn_result->nn_result_info[i].class_id = det_out.detBBox[i].clsType;
        nn_result->nn_result_info[i].box.x = det_out.detBBox[i].bbox.x * NN_WIDTH;
        nn_result->nn_result_info[i].box.y = det_out.detBBox[i].bbox.y * NN_HEIGHT;
        nn_result->nn_result_info[i].box.w = det_out.detBBox[i].bbox.w * NN_WIDTH;
        nn_result->nn_result_info[i].box.h = det_out.detBBox[i].bbox.h * NN_HEIGHT;
        nn_result->nn_result_info[i].conf = det_out.detBBox[i].conf * 100;
#if 0
        rt_kprintf("[%d] class_id = %d\n", i, nn_result->nn_result_info[i].class_id);
        rt_kprintf("[%d] x = %d\n", i, nn_result->nn_result_info[i].box.x);
        rt_kprintf("[%d] y = %d\n", i, nn_result->nn_result_info[i].box.y);
        rt_kprintf("[%d] w = %d\n", i, nn_result->nn_result_info[i].box.w);
        rt_kprintf("[%d] h = %d\n", i, nn_result->nn_result_info[i].box.h);
        rt_kprintf("[%d] conf = %d\n", i, nn_result->nn_result_info[i].conf);
#endif
    }

    ret = FH_NNA_DetectRelease(handle_t->tskHandle, &hw_out_info);
    FBV_FUNC_ERROR_PRT("FH_NNA_DetectRelease", ret);
    return ret;

Exit:
    ret = FH_NNA_DetectRelease(handle_t->tskHandle, &hw_out_info);
    FBV_FUNC_ERROR_PRT("FH_NNA_DetectRelease", ret);

    return ret;
}

static void nn_dect_thrd(void *args)
{
    int ret;
    FH_VIDEO_FRAME stVpuframeinfo;
    FH_UINT32 handle_lock = 0;
    NN_RESULT_DET nn_result;
    rt_memset(&nn_result, 0, sizeof(nn_result));
    static unsigned int idx = 0;
    static unsigned int frame = 0;
    while (1)
    {
        idx++;
        ret = FH_VPSS_GetChnFrame(FH_APP_GRP_ID, 2, &stVpuframeinfo, 1000, &handle_lock, 1, 0);
        if (ret)
        {
            continue;
        }

        ret = fbv_nna_process(&stVpuframeinfo, &nn_result);
        if (ret)
        {
            rt_kprintf("FH_NNA_DET_Process failed %x !\n", ret);
        }
        else
        {
            if (idx == 1)
            {
                /* rt_kprintf("first NN frame PTS: 0x%x%08x, current time(us): %10u\n", (unsigned int)(stVpuframeinfo.time_stamp >> 32), (unsigned int)(stVpuframeinfo.time_stamp & 0xffffffff), (unsigned int)read_pts()); */
                fbv_time_stat_add("fbv_time", "First nn detetct");
            }
        }

        /* process with nn_out */
        if (ret == 0 && nn_result.nn_result_num > 0)
        {
            frame++;
            if (frame == 1)
            {
                rt_kprintf("\n$$$$$$$$$$$$$$$   first nn detected pts= %u  $$$$$$$$$$$$$$$$$$$$$\n", (unsigned int)read_pts());
            }
        }

        FH_VPSS_ReleaseChnFrame(0, 2, &stVpuframeinfo, handle_lock);
    }
}

void fbv_nna_start_do_detect(void)
{
    rt_thread_t tid;

    fbv_nna_init();

    tid = rt_thread_create("nn_detect", nn_dect_thrd, RT_NULL, 0x4000, 30, 10);
    if (tid != RT_NULL)
        rt_thread_startup(tid);
}
