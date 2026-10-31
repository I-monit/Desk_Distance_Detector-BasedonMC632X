#include "nna_app/include/sample_common_nna.h"

static pthread_mutex_t g_switch_lock[MAX_GRP_NUM];
// #define AINR_ON_FORMAT FORMAT_600W12P5
// #define AINR_OFF_FORMAT FORMAT_600WP20

#define AINR_ON_FORMAT FORMAT_400WP15      // low fps
#define AINR_OFF_FORMAT FORMAT_WDR_400WP30 // high fps

static char *model_name = MODEL_TAG_DEMO_NNA_AINR;

static FH_UINT32 g_nna_grpid[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_running[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_stop[MAX_GRP_NUM] = {0};
static FH_SINT32 g_nna_pause[MAX_GRP_NUM] = {0};

static FH_SINT32 g_switch_ainr_running = 0;
static FH_SINT32 g_switch_ainr_stop = 0;
static FH_SINT32 g_ainr_on_format[MAX_GRP_NUM] = {0};
static FH_SINT32 g_ainr_off_format[MAX_GRP_NUM] = {0};

static FH_SINT32 switch_ainr_run(FH_SINT32 grp_id, FH_SINT32 *ainr_off)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ISP_INFO *isp_info;
    FH_CHAR *param;
    FH_SINT32 day_flag = 0;
    FH_SINT32 night_flag = 0;
    FH_SINT32 flag = 0;
    FH_SINT32 wdr_mode = 0;
    FH_SINT32 hex_file_len = 0;

    if (grp_id < 0)
    {
        return FH_SDK_SUCCESS;
    }

    isp_info = get_isp_config(grp_id);
    if (!isp_info->enable)
    {
        SDK_ERR_PRT(model_name, "switch_ainr_run Failed! ISP[%d] not enable!\n", grp_id);
        return FH_SDK_FAILED;
    }

    if (*ainr_off) // night[on]
    {
        if (isp_info->isp_format != g_ainr_off_format[grp_id])
        {
            SDK_ERR_PRT(model_name, "Current ISP[%d] Format[0x%x] must be same as g_ainr_off_format[%d] = 0x%x\n", grp_id, isp_info->isp_format, grp_id, g_ainr_off_format[grp_id]);
            return FH_SDK_FAILED;
        }

        pthread_mutex_lock(&g_switch_lock[grp_id]);
        isp_info->isp_format = g_ainr_on_format[grp_id];
        wdr_mode = get_sensorFormat_wdr_mode(isp_info->isp_format);
        if (wdr_mode)
        {
            night_flag = SAMPLE_SENSOR_FLAG_WDR_NIGHT;
        }
        else
        {
            night_flag = SAMPLE_SENSOR_FLAG_NIGHT;
        }
        flag = night_flag;

        ret = change_ai_isp_format(grp_id, *ainr_off);
        SDK_FUNC_ERROR(model_name, ret);

        param = getSensorHex(grp_id, isp_info->sensor->name, flag, &hex_file_len);
        if (param)
        {
            ret = API_ISP_LoadIspParam(grp_id, param);
            SDK_FUNC_ERROR(model_name, ret);
            freeSensorHex(param);
        }
        else
        {
            SDK_PRT(model_name, "Error: Cann't load sensor hex file!\n");
        }

        g_nna_pause[grp_id] = 0;

        *ainr_off = 0;
        SDK_PRT(model_name, "AINR ON!\n");
    }
    else // day[off]
    {
        if (isp_info->isp_format != g_ainr_on_format[grp_id])
        {
            SDK_ERR_PRT(model_name, "Current ISP[%d] Format[0x%x] must be same as g_ainr_on_format[%d] = 0x%x\n", grp_id, isp_info->isp_format, grp_id, g_ainr_on_format[grp_id]);
            return FH_SDK_FAILED;
        }

        isp_info->isp_format = g_ainr_off_format[grp_id];
        wdr_mode = get_sensorFormat_wdr_mode(isp_info->isp_format);
        if (wdr_mode)
        {
            day_flag = SAMPLE_SENSOR_FLAG_WDR;
        }
        else
        {
            day_flag = SAMPLE_SENSOR_FLAG_NORMAL;
        }
        flag = day_flag;

        g_nna_pause[grp_id] = 1;
        pthread_mutex_lock(&g_switch_lock[grp_id]);

        ret = change_ai_isp_format(grp_id, *ainr_off);
        SDK_FUNC_ERROR(model_name, ret);

        param = getSensorHex(grp_id, isp_info->sensor->name, flag, &hex_file_len);
        if (param)
        {
            ret = API_ISP_LoadIspParam(grp_id, param);
            SDK_FUNC_ERROR(model_name, ret);
            freeSensorHex(param);
        }
        else
        {
            SDK_PRT(model_name, "Error: Cann't load sensor hex file!\n");
        }

        *ainr_off = 1;
        SDK_PRT(model_name, "AINR OFF!\n");
    }

    return ret;
}

FH_VOID *switch_ainr(FH_VOID *args)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_CHAR cmd[20];
    FH_SINT32 ainr_off[MAX_GRP_NUM] = {0}; // off[1] on[0]
    FH_SINT32 grp_id = -1;

    if (g_switch_ainr_running)
    {
        return NULL;
    }
    g_switch_ainr_running = 1;

    for (int i = 0; i < MAX_GRP_NUM; i++)
    {
        g_ainr_on_format[i] = AINR_ON_FORMAT;
        g_ainr_off_format[i] = AINR_OFF_FORMAT;
    }

    prctl(PR_SET_NAME, "switch_ainr");
    sleep(1);
    while (!g_switch_ainr_stop)
    {
        printf("Enter '0','1','exit' to switch GRP0 or GRP1 ainr!!!\n");
        scanf("%s", cmd);
        if (strcmp(cmd, "0") == 0)
        {
            grp_id = 0;
        }
        else if (strcmp(cmd, "1") == 0)
        {
            grp_id = 1;
        }
        else if (strcmp(cmd, "exit") == 0)
        {
            grp_id = -1;
            break;
        }
        else
        {
            grp_id = -1;
            SDK_PRT(model_name, "Print Cmd Error!\n");
        }

        ret = switch_ainr_run(grp_id, &ainr_off[grp_id]);
        SDK_FUNC_ERROR_PRT(model_name, ret);
        pthread_mutex_unlock(&g_switch_lock[grp_id]);
    }

    g_switch_ainr_running = 0;

    return NULL;
}

FH_VOID *ainr(FH_VOID *args)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    NN_HANDLE nn_handle = NULL;
    NN_TYPE nn_type;
    FH_CHAR model_path[256];
    ISP_INFO *isp_info;
    NN_INPUT_DATA nn_input_data[2];
    FH_SINT32 grp_id = *((FH_SINT32 *)args);
    FH_CHAR thread_name[20];
    ISP_POST_FRAME_INFO_S stFramePost;

    isp_info = get_isp_config(grp_id);

    if (!isp_info->enable)
    {
        SDK_ERR_PRT(model_name, "ISP GRP[%d] not enable !\n", grp_id);
        goto Exit;
    }

    nn_type = AINR;
    strcpy(model_path, "./models/ainr.nbg");

    ret = nna_init(grp_id, &nn_handle, nn_type, model_path, 0, 0);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    memset(&nn_input_data, 0, sizeof(nn_input_data));
    SDK_PRT(model_name, "ainr[%d] Start!\n", grp_id);

    g_nna_running[grp_id] = 1;
    g_nna_pause[grp_id] = 0;

    memset(thread_name, 0, sizeof(thread_name));
    sprintf(thread_name, "nna_ainr_%d", grp_id);
    prctl(PR_SET_NAME, thread_name);
    while (!g_nna_stop[grp_id])
    {
        pthread_mutex_lock(&g_switch_lock[grp_id]);
        if (!g_nna_pause[grp_id])
        {
            ret = get_isp_data(grp_id, 1 * 1000, &stFramePost);
            if (!ret)
            {
                nn_input_data[0].data.base = stFramePost.stNNPreRaw.u32Base;
                nn_input_data[0].data.vbase = (void *)stFramePost.stNNPreRaw.u32Vbase;
                nn_input_data[0].data.size = stFramePost.stNNPreRaw.u32Size;

                nn_input_data[1].data.base = stFramePost.stNNPreStd.u32Base + stFramePost.stNNPreStd.u32Size;
                nn_input_data[1].data.vbase = (void *)stFramePost.stNNPreStd.u32Vbase + stFramePost.stNNPreStd.u32Size;
                nn_input_data[1].data.size = stFramePost.stNNPreStd.u32Size;

                ret = nna_get_ainr_result(nn_handle, nn_input_data);
                SDK_FUNC_ERROR_PRT(model_name, ret);

                ret = post_data_to_isp(grp_id, &stFramePost);
                SDK_FUNC_ERROR_PRT(model_name, ret);

                ret = release_isp_data(grp_id, &stFramePost);
                SDK_FUNC_ERROR_PRT(model_name, ret);
                pthread_mutex_unlock(&g_switch_lock[grp_id]);
            }
            else
            {
                pthread_mutex_unlock(&g_switch_lock[grp_id]);
                usleep(10 * 1000);
            }
        }
        else
        {
            pthread_mutex_unlock(&g_switch_lock[grp_id]);
            usleep(10 * 1000);
        }
    }

    ret = nna_release(grp_id, &nn_handle);
    SDK_FUNC_ERROR_GOTO(model_name, ret);
    SDK_PRT(model_name, "ainr[%d] End!\n", grp_id);
Exit:
    g_nna_running[grp_id] = 0;

    return NULL;
}

FH_SINT32 sample_fh_ainr_start(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    pthread_t thread;
    pthread_attr_t attr;

    if (g_nna_running[grp_id])
    {
        SDK_PRT(model_name, "ainr[%d] already running!\n", grp_id);
        return FH_SDK_SUCCESS;
    }

    g_nna_grpid[grp_id] = grp_id;
    g_nna_stop[grp_id] = 0;

    pthread_mutex_init(&g_switch_lock[grp_id], NULL);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    pthread_attr_setstacksize(&attr, 10 * 1024);
#endif
    if (pthread_create(&thread, &attr, ainr, &g_nna_grpid[grp_id]))
    {
        SDK_ERR_PRT(model_name, "pthread_create failed\n");
        goto Exit;
    }

    g_switch_ainr_stop = 0;
    if (pthread_create(&thread, &attr, switch_ainr, NULL))
    {
        SDK_ERR_PRT(model_name, "pthread_create failed\n");
        goto Exit;
    }

    return ret;

Exit:
    return -1;
}

FH_SINT32 sample_fh_ainr_stop(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (g_nna_running[grp_id])
    {
        g_nna_stop[grp_id] = 1;

        while (g_nna_running[grp_id])
        {
            usleep(10 * 1000);
        }
    }
    else
    {
        SDK_PRT(model_name, "ainr[%d] not running!\n", grp_id);
    }

    if (g_switch_ainr_running)
    {
        g_switch_ainr_stop = 1;

        while (g_switch_ainr_running)
        {
            printf("Watting Enter 'exit'\n");
            sleep(1);
        }
    }

    return ret;
}
