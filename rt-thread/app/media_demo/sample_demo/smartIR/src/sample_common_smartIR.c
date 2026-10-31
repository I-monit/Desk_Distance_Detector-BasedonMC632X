#include "smartIR/include/sample_common_smartIR.h"

static char *model_name = MODEL_TAG_DEMO_SMART_IR;

static FH_CHAR *sensor_param_day[MAX_GRP_NUM];
static FH_CHAR *sensor_param_night[MAX_GRP_NUM];
static FH_SINT32 g_smartir_inited[MAX_GRP_NUM] = {0};
static FHADV_SMARTIR_STATUS_e g_sirStaPrev[MAX_GRP_NUM];

FH_SINT32 sample_fh_smartIR_init(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 hex_file_len;
    ISP_INFO *isp_info;
    FHADV_SMARTIR_CFG_t smartIR_cfg;
    FH_SINT32 wdr_mode = 0;
    FH_SINT32 day_flag = 0;
    FH_SINT32 night_flag = 0;

    if (g_smartir_inited[grp_id])
    {
        SDK_PRT(model_name, "SmartIR[%d] already running!\n", grp_id);
        return FH_SDK_SUCCESS;
    }

    isp_info = get_isp_config(grp_id);
    if (!isp_info->enable)
    {
        SDK_ERR_PRT(model_name, "ISP[%d] not enable!\n", grp_id);
        return FH_SDK_FAILED;
    }

    wdr_mode = get_sensorFormat_wdr_mode(isp_info->isp_format);

    if (wdr_mode)
    {
        day_flag = SAMPLE_SENSOR_FLAG_WDR;
        night_flag = SAMPLE_SENSOR_FLAG_WDR_NIGHT;
    }
    else
    {
        day_flag = SAMPLE_SENSOR_FLAG_NORMAL;
        night_flag = SAMPLE_SENSOR_FLAG_NIGHT;
    }

    sensor_param_day[grp_id] = getSensorHex(grp_id, isp_info->sensor->name, day_flag, &hex_file_len);
    if (!sensor_param_day[grp_id])
    {
        SDK_ERR_PRT(model_name, "Cann't load sensor[%s] hex Day[%d] file!\n", isp_info->sensor->name, grp_id);
        goto Exit;
    }

    sensor_param_night[grp_id] = getSensorHex(grp_id, isp_info->sensor->name, night_flag, &hex_file_len);
    if (!sensor_param_night[grp_id])
    {
        SDK_ERR_PRT(model_name, "Cann't load sensor[%s] hex Night[%d] file!\n", isp_info->sensor->name, grp_id);
        goto Exit;
    }

    ret = FHAdv_SmartIR_Init(grp_id);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    ret = FHAdv_SmartIR_GetCfg(grp_id, &smartIR_cfg);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    smartIR_cfg.d2n_thre = 0x1000;
    smartIR_cfg.n2d_value = 0x500;
    smartIR_cfg.n2d_gain = 0x140;
    ret = FHAdv_SmartIR_SetCfg(grp_id, &smartIR_cfg);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    ret = FHAdv_SmartIR_SetDebugLevel(grp_id, SMARTIR_DEBUG_OFF);
    SDK_FUNC_ERROR_GOTO(model_name, ret);

    g_smartir_inited[grp_id] = 1;

    for (FH_SINT32 i = 0; i < MAX_GRP_NUM; i++)
    {
        g_sirStaPrev[i] = SMARTIR_STATUS_DAY; /*the initial value must be FHADV_IR_DAY*/
    }

    SDK_PRT(model_name, "SmartIR[%d] init success!\n", grp_id);
    return ret;
Exit:
    if (sensor_param_day[grp_id])
    {
        freeSensorHex(sensor_param_day[grp_id]);
        sensor_param_day[grp_id] = NULL;
    }

    if (sensor_param_night[grp_id])
    {
        freeSensorHex(sensor_param_night[grp_id]);
        sensor_param_night[grp_id] = NULL;
    }

    return FH_SDK_FAILED;
}

FH_SINT32 sample_fh_smartIR_exit(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    g_smartir_inited[grp_id] = 0;

    if (sensor_param_day[grp_id])
    {
        freeSensorHex(sensor_param_day[grp_id]);
        sensor_param_day[grp_id] = NULL;
    }

    if (sensor_param_night[grp_id])
    {
        freeSensorHex(sensor_param_night[grp_id]);
        sensor_param_night[grp_id] = NULL;
    }

    ret = FHAdv_SmartIR_UnInit(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 sample_fh_smartIR_run(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 irStaCurr;

    if (!g_smartir_inited[grp_id])
    {
        return FH_SDK_FAILED;
    }

    irStaCurr = FHAdv_SmartIR_GetDayNightStatus(grp_id, g_sirStaPrev[grp_id]);

    if (irStaCurr == g_sirStaPrev[grp_id])
    {
        return ret;
    }

    if (irStaCurr == SMARTIR_STATUS_DAY)
    {
        SDK_PRT(model_name, "SmartIR[%d] Switch to Day!\n", grp_id);
        ret = API_ISP_LoadIspParam(grp_id, sensor_param_day[grp_id]);
        SDK_FUNC_ERROR(model_name, ret);
    }
    else if (irStaCurr == SMARTIR_STATUS_NIGHT) /*night*/
    {
        SDK_PRT(model_name, "SmartIR[%d] Switch to Night!\n", grp_id);
        ret = API_ISP_LoadIspParam(grp_id, sensor_param_night[grp_id]);
        SDK_FUNC_ERROR(model_name, ret);
    }
    else
    {
        SDK_ERR_PRT(model_name, "SmartIR[%d] FHAdv_SmartIR_GetDayNightStatus failed with:%x!\n", grp_id, irStaCurr);
    }

    g_sirStaPrev[grp_id] = irStaCurr;

    return ret;
}