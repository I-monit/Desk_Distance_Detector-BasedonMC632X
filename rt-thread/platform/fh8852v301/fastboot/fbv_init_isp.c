#include "fbv_common.h"
#include "all_sensor_array.h"

#ifndef FRAMES_TO_CHANGE_SIZE
#define FRAMES_TO_CHANGE_SIZE (0)
#endif

#ifdef ENBALE_SMALL_TO_BIG
static unsigned short g_awb_speed_save;
static unsigned short g_ae_speed_save;
#endif
static ISP_INFO g_isp_info[MAX_GRP_NUM] = {

    {
        .enable = 0,
#ifdef VIDEO_GRP0
        .enable = 1,
        .grpid = 0,
#ifdef ENBALE_SMALL_TO_BIG
        .isp_init_width_pre = SMALL_ISP_WIGHT,
        .isp_init_height_pre = SMALL_ISP_HEIGHT,
#endif
        .isp_init_width = ISP_WIDTH_G0,
        .isp_init_height = ISP_HEIGHT_G0,
#ifdef ENBALE_SMALL_TO_BIG
        .isp_format_pre = ISP_FORMAT_G9,
#endif
        .isp_format = ISP_FORMAT_G0,
        .sensor_name = ISP_SENSOR_NAME_G0,
        .lut2dWorkMode = ISP_LUT2D_BYPASS,
#endif
    },
    {
        .enable = 0,
#ifdef VIDEO_GRP1
        .enable = 1,
        .grpid = 1,
#ifdef ENBALE_SMALL_TO_BIG
        .isp_init_width_pre = SMALL_ISP_WIGHT,
        .isp_init_height_pre = SMALL_ISP_HEIGHT,
#endif
        .isp_init_width = ISP_WIDTH_G1,
        .isp_init_height = ISP_HEIGHT_G1,
#ifdef ENBALE_SMALL_TO_BIG
        .isp_format_pre = ISP_FORMAT_G9,
#endif
        .isp_format = ISP_FORMAT_G1,
        .sensor_name = ISP_SENSOR_NAME_G1,
        .lut2dWorkMode = ISP_LUT2D_BYPASS,
#endif
    },
};
/* load hex */
#define SNS_HEX_ENTRY_MAGIC (0x6ad7bfc5)
struct hex_file_hdr
{
    unsigned int total_len; /*文件总长度*/
    unsigned int magic;
    unsigned int reserved[2];
};

struct hex_file_entry
{
    unsigned int offset;
    unsigned int len;
    unsigned int magic;
    char file_name[36];
};

#define SAMPLE_SENSOR_FLAG_NORMAL (0x00)
#define SAMPLE_SENSOR_FLAG_NIGHT (0x02)
#define SAMPLE_SENSOR_FLAG_WDR (0x01)
#define SAMPLE_SENSOR_FLAG_WDR_NIGHT (SAMPLE_SENSOR_FLAG_NIGHT | SAMPLE_SENSOR_FLAG_WDR)

static unsigned int readl(unsigned int addr)
{
    volatile unsigned int *paddr = (volatile unsigned int *)addr;

    return paddr[0];
}

int fbv_get_chipinfo(void)
{
    int chip_id = 0;
    chip_id = readl(0xf0000004);
    if (chip_id & 0x1)
    {
        /* rt_kprintf("CHIP_ID = FH8852V301\n"); */
        return 1;
    }
    else
    {
        /* rt_kprintf("CHIP_ID = FH8662V100\n"); */
        return 0;
    }
}

static void _get_hex_file_name(char *sensor_name, int flags, char *file_name)
{
    char chr;
    char *suffix = "";

    if (flags == SAMPLE_SENSOR_FLAG_NORMAL)
        suffix = "_attr.hex";
    else if (flags == SAMPLE_SENSOR_FLAG_WDR)
        suffix = "_wdr_attr.hex";
    else if (flags == SAMPLE_SENSOR_FLAG_WDR_NIGHT)
        suffix = "_wdr_night_attr.hex";
    else if (flags == SAMPLE_SENSOR_FLAG_NIGHT)
        suffix = "_night_attr.hex";

    while (1)
    {
        chr = *(sensor_name++);
        if (!chr)
            break;
        if (chr >= 'A' && chr <= 'Z')
            chr += 32; /*转为小写字母*/
        *(file_name++) = chr;
    }

    do
    {
        chr = *(suffix++);
        *(file_name++) = chr;
    } while (chr);
}

static char *_os_get_isp_sensor_param(char *file_name, int *olen)
{
    struct hex_file_entry *entry = (struct hex_file_entry *)(g_sensor_hex_array_fast + sizeof(struct hex_file_hdr));

    while (1)
    {
        if (entry->magic != SNS_HEX_ENTRY_MAGIC || !entry->offset)
            break;

        if (!rt_strcmp(entry->file_name, file_name)) /*find it*/
        {
            *olen = entry->len;
            return (g_sensor_hex_array_fast + entry->offset);
        }
        entry++;
    }

    return NULL;
}

static char *get_sensor_param(char *sensor_name, int flags, int *olen)
{
    char file_name[64];

    _get_hex_file_name(sensor_name, flags, file_name);

    return _os_get_isp_sensor_param(file_name, olen);
}
/* load hex */

struct isp_sensor_if *Sensor_Create(char *name)
{
#ifdef FH_USING_PS5270_MIPI_G0
    if (rt_strcmp(name, "ps5270_mipi") == 0)
    {
        return Sensor_Create_ps5270_mipi();
    }
#endif
#ifdef FH_USING_OVOS02K_MIPI_G0
    if (rt_strcmp(name, "ovos02k_mipi") == 0)
    {
        return Sensor_Create_ovos02k_mipi();
    }
#endif
#ifdef FH_USING_SC3336_MIPI_G0
    if (rt_strcmp(name, "sc3336_mipi") == 0)
    {
        return Sensor_Create_sc3336_mipi();
    }
#endif
#ifdef FH_USING_OVOS04C10_MIPI_G0
    if (rt_strcmp(name, "ovos04c10_mipi") == 0)
    {
        return Sensor_Create_ovos04c10_mipi();
    }
#endif
    return RT_NULL;
}

static FH_SINT32 choose_i2c(FH_SINT32 grp_id)
{
    if (fbv_get_chipinfo() == 1)
    {
        switch (grp_id)
        {
        case 0:
            return 0;
        case 1:
            return 0;
        default:
            break;
        }
    }
    else
    {
        switch (grp_id)
        {
        case 0:
            return 2;
        case 1:
            return 0;
        default:
            break;
        }
    }
    return 0;
}

static FH_SINT32 get_sensorFormat_wdr_mode(FH_SINT32 format)
{
    FH_SINT32 wdr_mode;

    wdr_mode = (format & 0x10000) >> 16;

    return wdr_mode;
}

static void udelay(int us)
{
    unsigned int t;
    int diff;

    t = read_pts();
    do
    {
        diff = read_pts() - t;
    } while (diff <= us);
}

static int fbv_gpio_set_value(unsigned int gpio, int value)
{
    FH_SINT32 ret = 0;

    ret = gpio_request(gpio);
    FBV_FUNC_ERROR_PRT("gpio_request", ret);

    ret = gpio_direction_output(gpio, value);
    FBV_FUNC_ERROR_PRT("gpio_direction_output", ret);

    ret = gpio_release(gpio);
    FBV_FUNC_ERROR_PRT("gpio_release", ret);

    return ret;
}

void sensor_reset(unsigned long rst_delay, unsigned int grp_id)
{
    int gpio = 0;
    if (fbv_get_chipinfo() == 1)
    {
        if (grp_id == 0)
        {
            gpio = 48;
        }
        else
        {
            gpio = 49;
        }
    }
    else
    {
        if (grp_id == 0)
        {
            gpio = 49;
        }
        else
        {
            gpio = 48;
        }
    }

    if ((rst_delay & 0xffff) > 0)
    {
        fbv_gpio_set_value(gpio, 0);
        udelay(rst_delay & 0xffff);
        fbv_gpio_set_value(gpio, 1);
    }

    if ((rst_delay >> 16) > 0)
    {
        udelay(rst_delay >> 16);
    }
}

#ifdef ENBALE_SMALL_TO_BIG
void isp_strategy(FH_SINT32 grp_id)
{
    FH_SINT32 ret;
    FH_UINT32 frameCnt = 0;
    while (frameCnt < FRAMES_TO_CHANGE_SIZE)
    {
        ret = API_ISP_Run(grp_id);
        if (ret == 0)
        {
            frameCnt++;
        }
        else
        {
            rt_kprintf("[ISP STRATEGY]API_ISP_Run faild %x\n", ret);
        }
        /* save_chan_frame();   */
    }
}
#endif

FH_SINT32 isp_change_resolution(FH_SINT32 grp_id)
{
    unsigned int rst_delay;
    ISP_SENSOR_COMMON_CMD_DATA0 fsParam;

#if 0
    if (g_sensor != RT_NULL)
    {
        rt_kprintf("common_if = %x\n", g_sensor->common_if);
    }
#endif
    g_isp_info[grp_id].sensor->common_if((void *)g_isp_info[grp_id].sensor, CMD_GET_GPIO_PARAM, &fsParam, RT_NULL);
    if (fsParam.gpio_param_cfg.gpio_enable)
    {
        rst_delay = fsParam.gpio_param_cfg.gpio_time;
        sensor_reset(rst_delay, grp_id);
    }

    /*AWB_DEFAULT_CFG awbCfg;*/
    AE_DEFAULT_CFG aeCfg;
    ISP_AE_INFO aeInfo;

    ISP_SENSOR_COMMON_CMD_DATA0 mapCfg;

    API_ISP_GetAeInfo(grp_id, &aeInfo);
    int ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
    if (ret)
    {
        rt_kprintf("API_ISP_SetSensorFmt faild ret%x\n", ret);
    }

    {
        ISP_VI_ATTR_S gst_sensor_vi_attr;
        FH_VICAP_VI_ATTR_S stViAttr;
        API_ISP_GetViAttr(grp_id, &gst_sensor_vi_attr);
        FH_VICAP_GetViAttr(grp_id, &stViAttr);
        stViAttr.stInSize.u16Width = gst_sensor_vi_attr.u16InputWidth;
        stViAttr.stInSize.u16Height = gst_sensor_vi_attr.u16InputHeight;
        stViAttr.enBayerType = gst_sensor_vi_attr.enBayerType;
        FH_VICAP_SetViAttr(grp_id, &stViAttr);
    }

    mapCfg.mapInttGainCfg.currFormat = g_isp_info[grp_id].isp_format_pre;
    mapCfg.mapInttGainCfg.desFormat = g_isp_info[grp_id].isp_format;
    mapCfg.mapInttGainCfg.currIntt = aeInfo.u32Intt;
    mapCfg.mapInttGainCfg.currTotalGain = aeInfo.u32TotalGain;
    g_isp_info[grp_id].sensor->common_if((void *)g_isp_info[grp_id].sensor, CMD_MAP_INTT_GAIN, &mapCfg, RT_NULL);

    API_ISP_SetSensorIntt(grp_id, mapCfg.mapInttGainCfg.desIntt);
    API_ISP_SetSensorGain(grp_id, mapCfg.mapInttGainCfg.desTotalGain);

    API_ISP_SensorKick(grp_id);
#if 1
    API_ISP_GetAeDefaultCfg(grp_id, &aeCfg);
    API_ISP_SetAeDefaultCfg(grp_id, &aeCfg);
#else
    /* API_ISP_GetAwbDefaultCfg(grp_id, &awbCfg);
    API_ISP_GetAeDefaultCfg(grp_id, &aeCfg);
    awbCfg.u16Speed = g_awb_speed_save;
    aeCfg.stAeSpeedCfg.u08Speed = g_ae_speed_save;
    API_ISP_SetAwbDefaultCfg(grp_id, &awbCfg);
    API_ISP_SetAeDefaultCfg(grp_id, &aeCfg);*/
#endif
    return 0;
}

static int startof_pict(void)
{
    static int idx_small = 0;
    static int idx_big = 0;
    static int count = 0;

    if ((count < FRAMES_TO_CHANGE_SIZE) && (idx_small == 0))
    {
        fbv_time_stat_add("fbv_time", "First small pic start");
        idx_small++;
    }

    if ((count >= FRAMES_TO_CHANGE_SIZE) && (idx_big == 0))
    {
        fbv_time_stat_add("fbv_time", "First big pic start");
        idx_big++;
    }

    count++;
    return 0;
}

static int endof_pict(void)
{
    static int idx_small = 0;
    static int idx_big = 0;
    static int count = 0;

    if ((count < FRAMES_TO_CHANGE_SIZE) && (idx_small == 0))
    {
        fbv_time_stat_add("fbv_time", "First small pic end");
        idx_small++;
    }

    if ((count >= FRAMES_TO_CHANGE_SIZE) && (idx_big == 0))
    {
        fbv_time_stat_add("fbv_time", "First big pic end");
        idx_big++;
    }

    count++;
    return 0;
}

static FH_SINT32 isp_init(FH_SINT32 grp_id)
{
    FH_SINT32 ret = 0;
    ISP_MEM_INIT stMemInit = {0};

    fbv_get_chipinfo();
    stMemInit.enOfflineWorkMode = ISP_OFFLINE_MODE_DISABLE;
    stMemInit.stPicConf.u32Width = g_isp_info[grp_id].isp_init_width;
    stMemInit.stPicConf.u32Height = g_isp_info[grp_id].isp_init_height;
    stMemInit.enIspOutMode = ISP_OUT_TO_VPU;
    ret = API_ISP_MemInit(grp_id, &stMemInit);
    FBV_FUNC_ERROR_PRT("API_ISP_MemInit", ret);

    g_isp_info[grp_id].sensor = Sensor_Create(g_isp_info[grp_id].sensor_name);
    ret = API_ISP_SensorRegCb(grp_id, 0, g_isp_info[grp_id].sensor);
    FBV_FUNC_ERROR_PRT("API_ISP_SensorRegCb", ret);

    Sensor_Init_t initConf = {0};
    initConf.u8CsiDeviceId = grp_id;
    initConf.u8CciDeviceId = choose_i2c(grp_id);
    initConf.bGrpSync = FH_FALSE;
    ret = API_ISP_SensorInit(grp_id, &initConf);
    FBV_FUNC_ERROR_PRT("API_ISP_SensorInit", ret);

#ifdef ENBALE_SMALL_TO_BIG
    ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format_pre);
    FBV_FUNC_ERROR_PRT("API_ISP_SetSensorFmt", ret);
#else
    ret = API_ISP_SetSensorFmt(grp_id, g_isp_info[grp_id].isp_format);
    FBV_FUNC_ERROR_PRT("API_ISP_SetSensorFmt", ret);
#endif

    API_ISP_RegisterPicStartCallback(grp_id, startof_pict);
    API_ISP_RegisterPicEndCallback(grp_id, endof_pict);

    fbv_time_stat_add("fbv_time", "Sensor Kick");
    ret = API_ISP_SensorKick(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_SensorKick", ret);

    ret = API_ISP_Init(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_Init", ret);

    ISP_VI_ATTR_S sensor_vi_attr = {0};
    ret = API_ISP_GetViAttr(grp_id, &sensor_vi_attr);
    FBV_FUNC_ERROR_PRT("API_ISP_GetViAttr", ret);

    FH_VICAP_DEV_ATTR_S stViDev = {0};
    FH_VICAP_VI_ATTR_S stViAttr = {0};

    stViAttr.enWorkMode = VICAP_WORK_MODE_ONLINE;
    stViDev.enWorkMode = VICAP_WORK_MODE_ONLINE;
    stViDev.stSize.u16Width = g_isp_info[grp_id].isp_init_width;
    stViDev.stSize.u16Height = g_isp_info[grp_id].isp_init_height;
    stViDev.bUsingVb = 0;
    ret = FH_VICAP_InitViDev(grp_id, &stViDev);
    FBV_FUNC_ERROR_PRT("FH_VICAP_InitViDev", ret);

    stViAttr.stInSize.u16Width = sensor_vi_attr.u16InputWidth;
    stViAttr.stInSize.u16Height = sensor_vi_attr.u16InputHeight;
    stViAttr.stCropSize.bCutEnable = 0;
    stViAttr.stCropSize.stRect.u16Width = sensor_vi_attr.u16PicWidth;
    stViAttr.stCropSize.stRect.u16Height = sensor_vi_attr.u16PicHeight;
    stViAttr.stCropSize.stRect.u16X = sensor_vi_attr.u16OffsetX;
    stViAttr.stCropSize.stRect.u16Y = sensor_vi_attr.u16OffsetY;
    stViAttr.stOfflineCfg.u8Priority = 0;
    stViAttr.u8WdrMode = get_sensorFormat_wdr_mode(g_isp_info[grp_id].isp_format);
    stViAttr.enBayerType = sensor_vi_attr.enBayerType;
    ret = FH_VICAP_SetViAttr(grp_id, &stViAttr);
    FBV_FUNC_ERROR_PRT("FH_VICAP_SetViAttr", ret);

    char *isp_param_buff;
    int olen;

    isp_param_buff = get_sensor_param(g_isp_info[grp_id].sensor_name, 0, &olen);
    API_ISP_LoadIspParam(grp_id, isp_param_buff);
    FBV_FUNC_ERROR_PRT("API_ISP_LoadIspParam", ret);

    FH_BIND_INFO src, dst;
    src.obj_id = FH_OBJ_ISP;
    src.dev_id = grp_id;
    src.chn_id = 0;

    dst.obj_id = FH_OBJ_VPU_VI;
    dst.dev_id = grp_id;
    dst.chn_id = 0;

    ret = FH_SYS_Bind(src, dst);
    if (ret != 0)
    {
        rt_kprintf("Error(%d - %x): FH_SYS_Bind ISP[%d] to VPU[%d]\n", ret, ret, grp_id, grp_id);
        return -1;
    }

#ifdef ENBALE_SMALL_TO_BIG
    AWB_DEFAULT_CFG awbCfg;
    AE_DEFAULT_CFG aeCfg;
    API_ISP_GetAwbDefaultCfg(grp_id, &awbCfg);
    API_ISP_GetAeDefaultCfg(grp_id, &aeCfg);
    g_awb_speed_save = awbCfg.u16Speed;
    g_ae_speed_save = aeCfg.stAeSpeedCfg.u08Speed;
    awbCfg.u16Speed = 0xfff;         /* awb调整速率调到最大 */
    aeCfg.stAeSpeedCfg.u08Speed = 0; /* ae调整速率调到最大 */
    aeCfg.stAeInitCfg.u08InitSpeed = 192;
    API_ISP_SetAwbDefaultCfg(grp_id, &awbCfg);
    API_ISP_SetAeDefaultCfg(grp_id, &aeCfg);
#endif

    return ret;
}

FH_SINT32 fbv_isp_init(FH_VOID)
{
    FH_SINT32 ret = 0;
    FH_SINT32 grp_id = 0;
    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        if (g_isp_info[grp_id].enable)
        {
            ret = isp_init(grp_id);
            FBV_FUNC_ERROR_PRT("isp_init", ret);

#ifdef ENBALE_SMALL_TO_BIG
            isp_strategy(grp_id);
            if (change_size(grp_id))
            {
                rt_kprintf("\nFAST_VIDEO_RESIZE faild!!!!\n");
                return -1;
            }
#endif
        }
    }
    return ret;
}

static FH_VOID fbv_isp_proc(FH_VOID *args)
{
    ISP_INFO *isp_info = (ISP_INFO *)args;
    while (1)
    {
        API_ISP_Run(isp_info->grpid);
    }
}

FH_VOID fbv_isp_start_proc(FH_VOID)
{
    rt_thread_t thread_isp[MAX_GRP_NUM];
    FH_SINT32 grp_id = 0;
    /*
     * ISP策略线程
     */
    for (grp_id = 0; grp_id < MAX_GRP_NUM; grp_id++)
    {
        if (g_isp_info[grp_id].enable)
        {
            thread_isp[grp_id] = rt_thread_create("fast_isp_thrd", fbv_isp_proc, &g_isp_info[grp_id],
                                                  4 * 1024 - 64, 25, 10);
            rt_thread_startup(thread_isp[grp_id]);
        }
    }
}

FH_SINT32 fbv_isp_pause(FH_SINT32 grp_id)
{
    FH_SINT32 ret = 0;
    ret = API_ISP_Pause(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_Pause", ret);
    return ret;
}

FH_SINT32 fbv_isp_resume(FH_SINT32 grp_id)
{
    FH_SINT32 ret = 0;
    ret = API_ISP_Resume(grp_id);
    FBV_FUNC_ERROR_PRT("API_ISP_Resume", ret);
    return ret;
}

FH_SINT32 fbv_isp_w(FH_SINT32 grp_id)
{
    return g_isp_info[grp_id].isp_init_width;
}

FH_SINT32 fbv_isp_w_pre(FH_SINT32 grp_id)
{
    return g_isp_info[grp_id].isp_init_width_pre;
}

FH_SINT32 fbv_isp_h(FH_SINT32 grp_id)
{
    return g_isp_info[grp_id].isp_init_height;
}

FH_SINT32 fbv_isp_h_pre(FH_SINT32 grp_id)
{
    return g_isp_info[grp_id].isp_init_height_pre;
}