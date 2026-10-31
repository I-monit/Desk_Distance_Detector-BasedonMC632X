#include "isp_config.h"
#if defined(FH_APP_USING_HEX_PARA) && defined(UVC_ENABLE)
#include "libusb_rtt/usb_video/include/uvc_extern.h"
#endif

static char *model_name = MODEL_TAG_ISP_CONFIG;

static ISP_INFO g_isp_info[MAX_GRP_NUM] = {

    {
        .enable = 0,
#if defined(VPU_MODE_MEM_G0) || defined(VPU_MODE_OFFLINE_2DLUT_DDR_G0)
        .isp_init_width = MEM_IN_WIDTH_G0,
        .isp_init_height = MEM_IN_HEIGHT_G0,
#endif
#if defined(VPU_MODE_ISP_G0) || defined(VPU_MODE_ONLINE_2DLUT_G0) || defined(VPU_MODE_OFFLINE_2DLUT_ISP_G0)
        .enable = 1,
        .channel = 0,
        .isp_format = ISP_FORMAT_G0,
        .isp_init_width = VI_INPUT_WIDTH_G0,
        .isp_init_height = VI_INPUT_HEIGHT_G0,
#ifdef MAX_ISP_WIDTH_G0
        .isp_max_width = MAX_ISP_WIDTH_G0 > VI_INPUT_WIDTH_G0 ? MAX_ISP_WIDTH_G0 : VI_INPUT_WIDTH_G0,
        .isp_max_height = MAX_ISP_HEIGHT_G0 > VI_INPUT_HEIGHT_G0 ? MAX_ISP_HEIGHT_G0 : VI_INPUT_HEIGHT_G0,
#else
        .isp_max_width = VI_INPUT_WIDTH_G0,
        .isp_max_height = VI_INPUT_HEIGHT_G0,
#endif
        .sensor_name = SENSOR_NAME_G0,
#if defined VPU_MODE_ONLINE_2DLUT_G0
        .lut2dWorkMode = ISP_LUT2D_ONLINE,
#elif defined(VPU_MODE_OFFLINE_2DLUT_ISP_G0) || defined(VPU_MODE_OFFLINE_2DLUT_DDR_G0)
        .lut2dWorkMode = ISP_LUT2D_OFFLINE,
#else
        .lut2dWorkMode = ISP_LUT2D_BYPASS,
#endif
        .running = 0,
#endif /* #if defined VPU_MODE_ISP_G0 || defined VPU_MODE_ONLINE_2DLUT_G0 */
    },
    {
        .enable = 0,
#if defined(VPU_MODE_MEM_G1) || defined(VPU_MODE_OFFLINE_2DLUT_DDR_G1)
        .isp_init_width = MEM_IN_WIDTH_G1,
        .isp_init_height = MEM_IN_HEIGHT_G1,
#endif
#if defined(VPU_MODE_ISP_G1) || defined(VPU_MODE_ONLINE_2DLUT_G1) || defined(VPU_MODE_OFFLINE_2DLUT_ISP_G1)
        .enable = 1,
        .channel = 1,
        .isp_format = ISP_FORMAT_G1,
        .isp_init_width = VI_INPUT_WIDTH_G1,
        .isp_init_height = VI_INPUT_HEIGHT_G1,
#ifdef MAX_ISP_WIDTH_G1
        .isp_max_width = MAX_ISP_WIDTH_G1 > VI_INPUT_WIDTH_G1 ? MAX_ISP_WIDTH_G1 : VI_INPUT_WIDTH_G1,
        .isp_max_height = MAX_ISP_HEIGHT_G1 > VI_INPUT_HEIGHT_G1 ? MAX_ISP_HEIGHT_G1 : VI_INPUT_HEIGHT_G1,
#else
        .isp_max_width = VI_INPUT_WIDTH_G1,
        .isp_max_height = VI_INPUT_HEIGHT_G1,
#endif
        .sensor_name = SENSOR_NAME_G1,
#if defined VPU_MODE_ONLINE_2DLUT_G1
        .lut2dWorkMode = ISP_LUT2D_ONLINE,
#elif defined(VPU_MODE_OFFLINE_2DLUT_ISP_G1) || defined(VPU_MODE_OFFLINE_2DLUT_DDR_G1)
        .lut2dWorkMode = ISP_LUT2D_OFFLINE,
#else
        .lut2dWorkMode = ISP_LUT2D_BYPASS,
#endif
        .running = 0,
#endif /* VPU_MODE_ISP_G1 */
    },
};

ISP_INFO *get_isp_config(FH_SINT32 grp_id)
{
    return &g_isp_info[grp_id];
}

FH_UINT32 get_isp_max_w(FH_SINT32 grp_id)
{
    return g_isp_info[grp_id].isp_max_width;
}

FH_UINT32 get_isp_max_h(FH_SINT32 grp_id)
{
    return g_isp_info[grp_id].isp_max_height;
}

FH_UINT32 get_isp_w(FH_SINT32 grp_id)
{
    return g_isp_info[grp_id].isp_init_width;
}

FH_UINT32 get_isp_h(FH_SINT32 grp_id)
{
    return g_isp_info[grp_id].isp_init_height;
}

FH_SINT32 isp_init(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 flag = SAMPLE_SENSOR_FLAG_NORMAL;
    FH_SINT32 hex_file_len;
    FH_CHAR *param;
    ISP_MEM_INIT stMemInit = {0};
    FH_UINT16 sensorFps = 0;

#ifdef CONFIG_VICAP_OFFLINE_MODE
    if (get_sensorFormat_wdr_mode(g_isp_info[grp_id].isp_format))
        stMemInit.enOfflineWorkMode = ISP_OFFLINE_MODE_WDR;
    else
        stMemInit.enOfflineWorkMode = ISP_OFFLINE_MODE_LINEAR;
#else
    stMemInit.enOfflineWorkMode = ISP_OFFLINE_MODE_DISABLE;
#endif
    stMemInit.stPicConf.u32Width = g_isp_info[grp_id].isp_max_width;
    stMemInit.stPicConf.u32Height = g_isp_info[grp_id].isp_max_height;
    stMemInit.enIspOutMode = ISP_OUT_TO_VPU;

    ret = API_ISP_MemInit(grp_id, &stMemInit);
    SDK_FUNC_ERROR(model_name, ret);

    g_isp_info[grp_id].sensor = start_sensor(g_isp_info[grp_id].sensor_name, grp_id);
    if (!g_isp_info[grp_id].sensor)
    {
        SDK_PRT(model_name, "Error(%d - %x): start_sensor (grp_id):(%d)!\n", ret, ret, grp_id);
        return -1;
    }
    ret = API_ISP_SensorRegCb(grp_id, 0, g_isp_info[grp_id].sensor);
    SDK_FUNC_ERROR(model_name, ret);

    Sensor_Init_t initConf = {0};
    initConf.u8CsiDeviceId = grp_id;
    initConf.u8CciDeviceId = choose_i2c(grp_id);
    initConf.bGrpSync = FH_FALSE;
    ret = API_ISP_SensorInit(grp_id, &initConf);
    SDK_FUNC_ERROR(model_name, ret);

    // 下载配置到sensor
    ISP_VI_ATTR_S sensor_vi_attr = {0};
#ifdef EMU
    ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
    SDK_FUNC_ERROR(model_name, ret);
    sensor_vi_attr.u16InputHeight = g_isp_info[grp_id].isp_max_height;
    sensor_vi_attr.u16InputWidth = g_isp_info[grp_id].isp_max_width;
    sensor_vi_attr.u16PicHeight = g_isp_info[grp_id].isp_init_height;
    sensor_vi_attr.u16PicWidth = g_isp_info[grp_id].isp_init_width;
    sensor_vi_attr.u16OffsetX = 0;
    sensor_vi_attr.u16OffsetY = 0;
    sensor_vi_attr.enBayerType = BAYER_GRBG;
    sensor_vi_attr.u16FrameRate = 30;
    ret = API_ISP_SetViAttr(grp_id, &sensor_vi_attr);
    SDK_FUNC_ERROR(model_name, ret);
#else
    ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
    SDK_FUNC_ERROR(model_name, ret);
    SDK_PRT(model_name, "ISP[%d] Format:0x%x!\n", grp_id, g_isp_info[grp_id].isp_format);
#endif

    // 启动sensor输出
    ret = API_ISP_SensorKick(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    // 初始化ISP硬件寄存器
    ret = API_ISP_Init(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    // 3 根据实际使用，配置vicap是离线模式还是在线模式
    ret = API_ISP_GetViAttr(grp_id, &sensor_vi_attr);
    SDK_FUNC_ERROR(model_name, ret);

    sensorFps = sensor_vi_attr.u16FrameRate;
    SDK_PRT(model_name, "VICAP's FPS = %d\n", sensorFps);

    FH_VICAP_DEV_ATTR_S stViDev;
    FH_VICAP_VI_ATTR_S stViAttr;
    // 初始化vi设备
#ifdef CONFIG_VICAP_OFFLINE_MODE
    stViAttr.enWorkMode = VICAP_WORK_MODE_OFFLINE;
    stViDev.enWorkMode = VICAP_WORK_MODE_OFFLINE;
#else
    stViAttr.enWorkMode = VICAP_WORK_MODE_ONLINE;
    stViDev.enWorkMode = VICAP_WORK_MODE_ONLINE;
#endif
    stViDev.stSize.u16Width = g_isp_info[grp_id].isp_init_width;
    stViDev.stSize.u16Height = g_isp_info[grp_id].isp_init_height;
    stViDev.bUsingVb = 0;
    ret = FH_VICAP_InitViDev(grp_id, &stViDev);
    SDK_FUNC_ERROR(model_name, ret);

    ret = set_sensor_sync(grp_id, sensorFps);
    SDK_FUNC_ERROR(model_name, ret);

    // 配置vi attr
    stViAttr.stInSize.u16Width = sensor_vi_attr.u16InputWidth;
    stViAttr.stInSize.u16Height = sensor_vi_attr.u16InputHeight;
    stViAttr.stCropSize.bCutEnable = 1;
    stViAttr.stCropSize.stRect.u16Width = sensor_vi_attr.u16PicWidth;
    stViAttr.stCropSize.stRect.u16Height = sensor_vi_attr.u16PicHeight;
    stViAttr.stCropSize.stRect.u16X = sensor_vi_attr.u16OffsetX;
    stViAttr.stCropSize.stRect.u16Y = sensor_vi_attr.u16OffsetY;
    stViAttr.stOfflineCfg.u8Priority = 0;
    stViAttr.u8WdrMode = get_sensorFormat_wdr_mode(g_isp_info[grp_id].isp_format);
    stViAttr.enBayerType = sensor_vi_attr.enBayerType;
    ret = FH_VICAP_SetViAttr(grp_id, &stViAttr);
    SDK_FUNC_ERROR(model_name, ret);

    ret = start_sensor_sync(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    if ((g_isp_info[grp_id].isp_format >> 16) & 1)
    {
        flag |= SAMPLE_SENSOR_FLAG_WDR;
    }

#if defined(FH_APP_USING_HEX_PARA) && defined(UVC_ENABLE)
    if (load_flash_isppara(grp_id, API_ISP_LoadIspParam))
#endif
    {
    param = getSensorHex(grp_id, (FH_CHAR *)g_isp_info[grp_id].sensor->name, flag, &hex_file_len);
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
    }

    ret = FHAdv_Isp_Init(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    g_isp_info[grp_id].init_status = 1;

    return ret;
}

FH_SINT32 isp_exit(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (g_isp_info[grp_id].init_status)
    {
        ret = FH_VICAP_Exit(grp_id);
        SDK_FUNC_ERROR(model_name, ret);

        ret = API_ISP_Exit(grp_id);
        SDK_FUNC_ERROR(model_name, ret);

        destroy_sensor(g_isp_info[grp_id].sensor_name, g_isp_info[grp_id].sensor);

        g_isp_info[grp_id].init_status = 0;
    }

    return ret;
}

FH_SINT32 isp_strategy_run(FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    if (g_isp_info[grp_id].pause && g_isp_info[grp_id].resume)
    {
        usleep(100 * 1000);
        ret = API_ISP_Run(g_isp_info[grp_id].channel);
        g_isp_info[grp_id].pause = 0;
        g_isp_info[grp_id].resume = 0;
    }
    else if (!g_isp_info[grp_id].pause)
    {
        ret = API_ISP_Run(g_isp_info[grp_id].channel);
    }

    return ret;
}

FH_VOID isp_strategy_pause(FH_SINT32 grp_id)
{
    g_isp_info[grp_id].pause = 1;
}

FH_VOID isp_strategy_resume(FH_SINT32 grp_id)
{
    g_isp_info[grp_id].resume = 1;
}

FH_SINT32 get_isp_fps(FH_SINT32 grp_id, FH_UINT32 *isp_fps)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ISP_VI_STAT_S pstStat;

    ret = API_ISP_GetVIState(grp_id, &pstStat);
    SDK_FUNC_ERROR(model_name, ret);

    *isp_fps = pstStat.u32FrmRate;
    return ret;
}

FH_SINT32 change_isp_format(FH_SINT32 grp_id, FH_SINT32 isp_format_bak)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_SINT32 flag = SAMPLE_SENSOR_FLAG_NORMAL;
    FH_SINT32 hex_file_len;
    FH_CHAR *param;
    FH_BIND_INFO dst;
    FH_VICAP_VI_ATTR_S stViAttr;
    ISP_VI_ATTR_S ispViAttr;

    if (!g_isp_info[grp_id].enable)
    {
        SDK_ERR_PRT(model_name, "change_isp_format Failed, ISP[%d] not enable!\n", grp_id);
        return FH_SDK_FAILED;
    }

    isp_strategy_pause(grp_id);

    /* 停止isp */
    ret = API_ISP_Pause(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    reset_sensor(grp_id);

    dst.obj_id = FH_OBJ_ISP;
    dst.dev_id = grp_id;
    dst.chn_id = 0;
    SDK_FUNC_ERROR(model_name, ret);
    ret = FH_SYS_UnBindbyDst(dst);

    ret = vpu_grp_disable(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
    if (ret)
    {
        SDK_PRT(model_name, "Setting ISP[%d] back to Format:0x%x! ret = %x\n", grp_id, isp_format_bak, ret);
        g_isp_info[grp_id].isp_format = isp_format_bak;
        ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
        SDK_FUNC_ERROR(model_name, ret);
    }

    ret = API_ISP_SensorKick(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    ret = API_ISP_GetViAttr(grp_id, &ispViAttr);
    SDK_FUNC_ERROR(model_name, ret);

    g_isp_info[grp_id].isp_init_width = ispViAttr.u16PicWidth;
    g_isp_info[grp_id].isp_init_height = ispViAttr.u16PicHeight;

    ret = FH_VICAP_GetViAttr(grp_id, &stViAttr);
    SDK_FUNC_ERROR(model_name, ret);

    stViAttr.stInSize.u16Width = ispViAttr.u16InputWidth;
    stViAttr.stInSize.u16Height = ispViAttr.u16InputHeight;
    stViAttr.stCropSize.stRect.u16Width = ispViAttr.u16PicWidth;
    stViAttr.stCropSize.stRect.u16Height = ispViAttr.u16PicHeight;
    stViAttr.stCropSize.stRect.u16X = ispViAttr.u16OffsetX;
    stViAttr.stCropSize.stRect.u16Y = ispViAttr.u16OffsetY;
    stViAttr.u8WdrMode = get_sensorFormat_wdr_mode(g_isp_info[grp_id].isp_format);
    ret = FH_VICAP_SetViAttr(grp_id, &stViAttr);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_grp_set_vi_attr(grp_id, ispViAttr.u16PicWidth, ispViAttr.u16PicHeight);
    SDK_FUNC_ERROR(model_name, ret);

    ret = vpu_grp_enable(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    if ((g_isp_info[grp_id].isp_format >> 16) & 1)
    {
        flag |= SAMPLE_SENSOR_FLAG_WDR;
    }

    param = getSensorHex(grp_id, (FH_CHAR *)g_isp_info[grp_id].sensor->name, flag, &hex_file_len);
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

    /* 恢复isp */
    ret = API_ISP_Resume(grp_id);
    SDK_FUNC_ERROR(model_name, ret);

    isp_strategy_resume(grp_id);

#ifdef CONFIG_VICAP_OFFLINE_MODE
    FH_BIND_INFO src;

    src.obj_id = FH_OBJ_VICAP;
    src.dev_id = 0;
    src.chn_id = grp_id;
    dst.obj_id = FH_OBJ_ISP;
    dst.dev_id = grp_id;
    dst.chn_id = 0;
    ret = FH_SYS_Bind(src, dst);
    SDK_FUNC_ERROR(model_name, ret);
#endif

    SDK_PRT(model_name, "ISP[%d] Change Format:0x%x!\n", grp_id, g_isp_info[grp_id].isp_format);

    return ret;
}

FH_SINT32 set_isp_mirror_flip(FH_SINT32 grp_id, FH_UINT32 mirror, FH_UINT32 flip)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FHAdv_Isp_SetMirrorAndflip(grp_id, grp_id, mirror, flip);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 set_isp_ae_mode(FH_SINT32 grp_id, FH_UINT32 mode, FH_UINT32 intt_us, FH_UINT32 gain_level)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FHAdv_Isp_SetAEMode(grp_id, mode, intt_us, gain_level);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 set_isp_sharpeness(FH_SINT32 grp_id, FH_UINT32 mode, FH_UINT32 value)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FHAdv_Isp_SetSharpeness(grp_id, mode, value);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 set_isp_saturation(FH_SINT32 grp_id, FH_UINT32 mode, FH_UINT32 value)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;

    ret = FHAdv_Isp_SetSaturation(grp_id, mode, value);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

FH_SINT32 set_isp_sleep(FH_SINT32 grp_id, FH_SINT32 sleep)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ISP_INFO *isp_info = NULL;

    isp_info = get_isp_config(grp_id);
    if (!isp_info->enable)
    {
        SDK_ERR_PRT(model_name, "ISP[%d] not enable!\n", grp_id);
        return FH_SDK_FAILED;
    }

    if (!isp_info->pause && sleep)
    {
        isp_strategy_pause(grp_id);

        ret = API_ISP_Pause(grp_id);
        SDK_FUNC_ERROR(model_name, ret);

        ret = sensor_sleep(grp_id);
        SDK_FUNC_ERROR(model_name, ret);

        ret = vpu_grp_disable(grp_id);
        SDK_FUNC_ERROR(model_name, ret);

        SDK_PRT(model_name, "ISP[%d] sleep!\n", grp_id);
    }
    else if (isp_info->pause && !sleep)
    {
        ret = vpu_grp_enable(grp_id);
        SDK_FUNC_ERROR(model_name, ret);

        ret = sensor_wakeup(grp_id);
        SDK_FUNC_ERROR(model_name, ret);

        ret = API_ISP_Resume(grp_id);
        SDK_FUNC_ERROR(model_name, ret);

        isp_strategy_resume(grp_id);
        SDK_PRT(model_name, "ISP[%d] wakeup!\n", grp_id);
    }

    return ret;
}
