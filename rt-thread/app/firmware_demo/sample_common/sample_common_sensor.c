#include "sample_common_sensor.h"

static FH_CHAR *model_name = MODEL_TAG_SENSOR_COMMON;

static FH_VOID selectIspFormat_FPS_12P5(ISP_INFO *isp_info)
{
    switch (isp_info->isp_format)
    {
    case FORMAT_500WP25:
    case FORMAT_500WP20:
    case FORMAT_500WP15:
    case FORMAT_500W12P5:
        isp_info->isp_format = FORMAT_500W12P5;
        break;
    case FORMAT_WDR_500WP30:
    case FORMAT_WDR_500WP25:
    case FORMAT_WDR_500WP20:
    case FORMAT_WDR_500WP15:
    case FORMAT_WDR_500W12P5:
        isp_info->isp_format = FORMAT_WDR_500W12P5;
        break;
    }
}

static FH_VOID selectIspFormat_FPS_P15(ISP_INFO *isp_info)
{
    switch (isp_info->isp_format)
    {
    case FORMAT_1080P60:
    case FORMAT_1080P30:
    case FORMAT_1080P25:
    case FORMAT_1080P20:
    case FORMAT_1080P15:
        isp_info->isp_format = FORMAT_1080P15;
        break;
    case FORMAT_1536X1536P30:
    case FORMAT_1536X1536P25:
    case FORMAT_1536X1536P15:
        isp_info->isp_format = FORMAT_1536X1536P15;
        break;
    case FORMAT_1536P40:
    case FORMAT_1536P30:
    case FORMAT_1536P25:
    case FORMAT_1536P15:
        isp_info->isp_format = FORMAT_1536P15;
        break;
    case FORMAT_400WP30:
    case FORMAT_400WP25:
    case FORMAT_400WP20:
    case FORMAT_400WP15:
        isp_info->isp_format = FORMAT_400WP15;
        break;
    case FORMAT_500WP25:
    case FORMAT_500WP20:
    case FORMAT_500WP15:
    case FORMAT_500W12P5:
        isp_info->isp_format = FORMAT_500WP15;
        break;
    case FORMAT_800WP30:
    case FORMAT_800WP25:
    case FORMAT_800WP15:
        isp_info->isp_format = FORMAT_800WP15;
        break;
    case FORMAT_2304X1296P30:
    case FORMAT_2304X1296P25:
    case FORMAT_2304X1296P15:
        isp_info->isp_format = FORMAT_2304X1296P15;
        break;
    case FORMAT_WDR_1536P40:
    case FORMAT_WDR_1536P30:
    case FORMAT_WDR_1536P25:
    case FORMAT_WDR_1536P15:
        isp_info->isp_format = FORMAT_WDR_1536P15;
        break;
    case FORMAT_WDR_400WP30:
    case FORMAT_WDR_400WP25:
    case FORMAT_WDR_400WP20:
    case FORMAT_WDR_400WP15:
        isp_info->isp_format = FORMAT_WDR_400WP15;
        break;
    case FORMAT_WDR_500WP30:
    case FORMAT_WDR_500WP25:
    case FORMAT_WDR_500WP20:
    case FORMAT_WDR_500WP15:
    case FORMAT_WDR_500W12P5:
        isp_info->isp_format = FORMAT_WDR_500WP15;
        break;
    case FORMAT_WDR_800WP30:
    case FORMAT_WDR_800WP25:
    case FORMAT_WDR_800WP15:
        isp_info->isp_format = FORMAT_WDR_800WP15;
        break;
    case FORMAT_WDR_2304X1296P25:
    case FORMAT_WDR_2304X1296P15:
        isp_info->isp_format = FORMAT_WDR_2304X1296P15;
        break;
    case FORMAT_WDR_2688X1520P30:
    case FORMAT_WDR_2688X1520P25:
    case FORMAT_WDR_2688X1520P20:
    case FORMAT_WDR_2688X1520P15:
        isp_info->isp_format = FORMAT_WDR_2688X1520P15;
        break;
    }
}

static FH_VOID selectIspFormat_FPS_P20(ISP_INFO *isp_info)
{
    switch (isp_info->isp_format)
    {
    case FORMAT_1080P60:
    case FORMAT_1080P30:
    case FORMAT_1080P25:
    case FORMAT_1080P20:
    case FORMAT_1080P15:
        isp_info->isp_format = FORMAT_1080P20;
        break;
    case FORMAT_400WP30:
    case FORMAT_400WP25:
    case FORMAT_400WP20:
    case FORMAT_400WP15:
        isp_info->isp_format = FORMAT_400WP20;
        break;
    case FORMAT_500WP25:
    case FORMAT_500WP20:
    case FORMAT_500WP15:
    case FORMAT_500W12P5:
        isp_info->isp_format = FORMAT_500WP20;
        break;
    case FORMAT_2688X1520P30:
    case FORMAT_2688X1520P25:
    case FORMAT_2688X1520P20:
    case FORMAT_2688X1520P15:
        isp_info->isp_format = FORMAT_2688X1520P20;
        break;
    case FORMAT_WDR_400WP30:
    case FORMAT_WDR_400WP25:
    case FORMAT_WDR_400WP20:
    case FORMAT_WDR_400WP15:
        isp_info->isp_format = FORMAT_WDR_400WP20;
        break;
    case FORMAT_WDR_500WP30:
    case FORMAT_WDR_500WP25:
    case FORMAT_WDR_500WP20:
    case FORMAT_WDR_500WP15:
    case FORMAT_WDR_500W12P5:
        isp_info->isp_format = FORMAT_WDR_500WP20;
        break;
    case FORMAT_WDR_2688X1520P30:
    case FORMAT_WDR_2688X1520P25:
    case FORMAT_WDR_2688X1520P20:
    case FORMAT_WDR_2688X1520P15:
        isp_info->isp_format = FORMAT_WDR_2688X1520P20;
        break;
    }
}

static FH_VOID selectIspFormat_FPS_P25(ISP_INFO *isp_info)
{
    switch (isp_info->isp_format)
    {
    case FORMAT_720P30:
    case FORMAT_720P25:
        isp_info->isp_format = FORMAT_720P25;
        break;
    case FORMAT_960P30:
    case FORMAT_960P25:
        isp_info->isp_format = FORMAT_960P25;
        break;
    case FORMAT_1080P60:
    case FORMAT_1080P30:
    case FORMAT_1080P25:
    case FORMAT_1080P20:
    case FORMAT_1080P15:
        isp_info->isp_format = FORMAT_1080P25;
        break;
    case FORMAT_1536X1536P30:
    case FORMAT_1536X1536P25:
    case FORMAT_1536X1536P15:
        isp_info->isp_format = FORMAT_1536X1536P25;
        break;
    case FORMAT_1536P40:
    case FORMAT_1536P30:
    case FORMAT_1536P25:
    case FORMAT_1536P15:
        isp_info->isp_format = FORMAT_1536P25;
        break;
    case FORMAT_400WP30:
    case FORMAT_400WP25:
    case FORMAT_400WP20:
    case FORMAT_400WP15:
        isp_info->isp_format = FORMAT_400WP25;
        break;
    case FORMAT_500WP25:
    case FORMAT_500WP20:
    case FORMAT_500WP15:
    case FORMAT_500W12P5:
        isp_info->isp_format = FORMAT_500WP25;
        break;
    case FORMAT_800WP30:
    case FORMAT_800WP25:
    case FORMAT_800WP15:
        isp_info->isp_format = FORMAT_800WP25;
        break;
    case FORMAT_2304X1296P30:
    case FORMAT_2304X1296P25:
    case FORMAT_2304X1296P15:
        isp_info->isp_format = FORMAT_2304X1296P25;
        break;
    case FORMAT_2688X1520P30:
    case FORMAT_2688X1520P25:
    case FORMAT_2688X1520P20:
    case FORMAT_2688X1520P15:
        isp_info->isp_format = FORMAT_2688X1520P25;
        break;
    case FORMAT_4096X128P30:
    case FORMAT_4096X128P25:
        isp_info->isp_format = FORMAT_4096X128P25;
        break;
    case FORMAT_WDR_1080P30:
    case FORMAT_WDR_1080P25:
        isp_info->isp_format = FORMAT_WDR_1080P25;
        break;
    case FORMAT_WDR_1536P40:
    case FORMAT_WDR_1536P30:
    case FORMAT_WDR_1536P25:
    case FORMAT_WDR_1536P15:
        isp_info->isp_format = FORMAT_WDR_1536P25;
        break;
    case FORMAT_WDR_400WP30:
    case FORMAT_WDR_400WP25:
    case FORMAT_WDR_400WP20:
    case FORMAT_WDR_400WP15:
        isp_info->isp_format = FORMAT_WDR_400WP25;
        break;
    case FORMAT_WDR_500WP30:
    case FORMAT_WDR_500WP25:
    case FORMAT_WDR_500WP20:
    case FORMAT_WDR_500WP15:
    case FORMAT_WDR_500W12P5:
        isp_info->isp_format = FORMAT_WDR_500WP25;
        break;
    case FORMAT_WDR_800WP30:
    case FORMAT_WDR_800WP25:
    case FORMAT_WDR_800WP15:
        isp_info->isp_format = FORMAT_WDR_800WP25;
        break;
    case FORMAT_WDR_2304X1296P25:
    case FORMAT_WDR_2304X1296P15:
        isp_info->isp_format = FORMAT_WDR_2304X1296P25;
        break;
    case FORMAT_WDR_2688X1520P30:
    case FORMAT_WDR_2688X1520P25:
    case FORMAT_WDR_2688X1520P20:
    case FORMAT_WDR_2688X1520P15:
        isp_info->isp_format = FORMAT_WDR_2688X1520P25;
        break;
    case FORMAT_WDR_4096X128P30:
    case FORMAT_WDR_4096X128P25:
        isp_info->isp_format = FORMAT_WDR_4096X128P25;
        break;
    }
}

static FH_VOID selectIspFormat_FPS_P30(ISP_INFO *isp_info)
{
    switch (isp_info->isp_format)
    {
    case FORMAT_720P30:
    case FORMAT_720P25:
        isp_info->isp_format = FORMAT_720P30;
        break;
    case FORMAT_960P30:
    case FORMAT_960P25:
        isp_info->isp_format = FORMAT_960P30;
        break;
    case FORMAT_1080P60:
    case FORMAT_1080P30:
    case FORMAT_1080P25:
    case FORMAT_1080P20:
    case FORMAT_1080P15:
        isp_info->isp_format = FORMAT_1080P30;
        break;
    case FORMAT_1536X1536P30:
    case FORMAT_1536X1536P25:
    case FORMAT_1536X1536P15:
        isp_info->isp_format = FORMAT_1536X1536P30;
        break;
    case FORMAT_1536P40:
    case FORMAT_1536P30:
    case FORMAT_1536P25:
    case FORMAT_1536P15:
        isp_info->isp_format = FORMAT_1536P30;
        break;
    case FORMAT_400WP30:
    case FORMAT_400WP25:
    case FORMAT_400WP20:
    case FORMAT_400WP15:
        isp_info->isp_format = FORMAT_400WP30;
        break;
    case FORMAT_800WP30:
    case FORMAT_800WP25:
    case FORMAT_800WP15:
        isp_info->isp_format = FORMAT_800WP30;
        break;
    case FORMAT_2304X1296P30:
    case FORMAT_2304X1296P25:
    case FORMAT_2304X1296P15:
        isp_info->isp_format = FORMAT_2304X1296P30;
        break;
    case FORMAT_2688X1520P30:
    case FORMAT_2688X1520P25:
    case FORMAT_2688X1520P20:
    case FORMAT_2688X1520P15:
        isp_info->isp_format = FORMAT_2688X1520P30;
        break;
    case FORMAT_4096X128P30:
    case FORMAT_4096X128P25:
        isp_info->isp_format = FORMAT_4096X128P30;
        break;
    case FORMAT_WDR_1080P30:
    case FORMAT_WDR_1080P25:
        isp_info->isp_format = FORMAT_WDR_1080P30;
        break;
    case FORMAT_WDR_1536P40:
    case FORMAT_WDR_1536P30:
    case FORMAT_WDR_1536P25:
    case FORMAT_WDR_1536P15:
        isp_info->isp_format = FORMAT_WDR_1536P30;
        break;
    case FORMAT_WDR_400WP30:
    case FORMAT_WDR_400WP25:
    case FORMAT_WDR_400WP20:
    case FORMAT_WDR_400WP15:
        isp_info->isp_format = FORMAT_WDR_400WP30;
        break;
    case FORMAT_WDR_500WP30:
    case FORMAT_WDR_500WP25:
    case FORMAT_WDR_500WP20:
    case FORMAT_WDR_500WP15:
    case FORMAT_WDR_500W12P5:
        isp_info->isp_format = FORMAT_WDR_500WP30;
        break;
    case FORMAT_WDR_800WP30:
    case FORMAT_WDR_800WP25:
    case FORMAT_WDR_800WP15:
        isp_info->isp_format = FORMAT_WDR_800WP30;
        break;
    case FORMAT_WDR_2688X1520P30:
    case FORMAT_WDR_2688X1520P25:
    case FORMAT_WDR_2688X1520P20:
    case FORMAT_WDR_2688X1520P15:
        isp_info->isp_format = FORMAT_WDR_2688X1520P30;
        break;
    case FORMAT_WDR_4096X128P30:
    case FORMAT_WDR_4096X128P25:
        isp_info->isp_format = FORMAT_WDR_4096X128P30;
        break;
    }
}

static FH_VOID selectIspFormat_FPS_P40(ISP_INFO *isp_info)
{
    switch (isp_info->isp_format)
    {
    case FORMAT_1536P40:
    case FORMAT_1536P30:
    case FORMAT_1536P25:
    case FORMAT_1536P15:
        isp_info->isp_format = FORMAT_1536P40;
        break;
    case FORMAT_WDR_1536P40:
    case FORMAT_WDR_1536P30:
    case FORMAT_WDR_1536P25:
    case FORMAT_WDR_1536P15:
        isp_info->isp_format = FORMAT_WDR_1536P40;
        break;
    }
}

static FH_VOID selectIspFormat_FPS_P60(ISP_INFO *isp_info)
{
    switch (isp_info->isp_format)
    {
    case FORMAT_1080P60:
    case FORMAT_1080P30:
    case FORMAT_1080P25:
    case FORMAT_1080P20:
    case FORMAT_1080P15:
        isp_info->isp_format = FORMAT_1080P60;
        break;
    }
}

#ifdef __LINUX_OS__
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

static void get_program_path(FH_CHAR *prog_path)
{
    FH_CHAR path[256];
    FH_CHAR *dname;
    FILE *proc_file;
    pid_t pid = getpid();

    *prog_path = 0;
    sprintf(path, "/proc/%i/cmdline", pid);
    proc_file = fopen(path, "r");
    if (proc_file)
    {
        fgets(path, 256, proc_file);
        fclose(proc_file);
        dname = dirname(path);
        strcpy(prog_path, dname);
    }
}

static FH_CHAR *os_get_isp_sensor_param(FH_SINT32 grp_id, FH_CHAR *file_name, FH_SINT32 *olen)
{
    FH_CHAR prog_path[256];
    FH_SINT32 fd;
    FH_SINT32 size;
    FH_SINT32 ret = FH_SDK_SUCCESS;
    FH_CHAR *buf = NULL;
    ISP_PARAM_CONFIG isp_param_config;
    struct stat _fstat;

    get_program_path(prog_path);
    strcat(prog_path, "/");
    strcat(prog_path, file_name);

    fd = open(prog_path, O_RDONLY);
    if (fd >= 0)
    {
        if (fstat(fd, &_fstat) == 0 && _fstat.st_size > 0 && _fstat.st_size <= 0x10000) /*0x10000应该足够大了,正常就几KB而已*/
        {
            ret = API_ISP_GetBinAddr(grp_id, &isp_param_config);
            SDK_FUNC_ERROR_GOTO(model_name, ret);
            size = isp_param_config.u32BinSize;
            if (size <= 0)
            {
                size = _fstat.st_size;
                SDK_PRT(model_name, "ISP[%d]API_ISP_GetBinAddr Size == 0!\n", grp_id);
            }
            buf = malloc(size);
            if (buf)
            {
                FH_UINT32 readNum = read(fd, buf, size);
                SDK_PRT(model_name, "ISP[%d] Param Size: 0x%x, Load %s (0x%x)!\n", grp_id, size, file_name, readNum);
            }
        }
    }

Exit:
    if (fd)
    {
        close(fd);
    }

    return buf;
}
#elif defined __RTTHREAD_OS__
#include "all_sensor_array.h"

#define SNS_HEX_ENTRY_MAGIC (0x6ad7bfc5)
struct hex_file_hdr
{
    FH_UINT32 total_len; /*文件总长度*/
    FH_UINT32 magic;
    FH_UINT32 reserved[2];
};

struct hex_file_entry
{
    FH_UINT32 offset;
    FH_UINT32 len;
    FH_UINT32 magic;
    FH_CHAR file_name[36];
};

extern FH_CHAR g_sensor_hex_array[];

static FH_CHAR *os_get_isp_sensor_param(FH_SINT32 grp_id, FH_CHAR *file_name, FH_SINT32 *olen)
{
    struct hex_file_entry *entry = (struct hex_file_entry *)(g_sensor_hex_array + sizeof(struct hex_file_hdr));

    while (1)
    {
        if (entry->magic != SNS_HEX_ENTRY_MAGIC || !entry->offset)
            break;

        if (!strcmp(entry->file_name, file_name)) /*find it*/
        {
            *olen = entry->len;
            return (g_sensor_hex_array + entry->offset);
        }
        entry++;
    }

    return NULL;
}
#endif

static void get_hex_file_name(FH_CHAR *sensor_name, FH_SINT32 flags, FH_CHAR *file_name)
{
    FH_CHAR chr;
    FH_CHAR *suffix = "";

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

FH_CHAR *getSensorHex(FH_SINT32 grp_id, FH_CHAR *sensor_name, FH_SINT32 flags, FH_SINT32 *olen)
{
    FH_CHAR file_name[64];

    get_hex_file_name(sensor_name, flags, file_name);

    return os_get_isp_sensor_param(grp_id, file_name, olen);
}

FH_VOID freeSensorHex(FH_CHAR *param)
{
#ifdef __LINUX_OS__
    if (param)
    {
        free(param);
    }
#elif defined __RTTHREAD_OS__
#endif
}

FH_SINT32 get_sensorFormat_wdr_mode(FH_SINT32 format)
{
    FH_SINT32 wdr_mode;

    wdr_mode = (format & 0x10000) >> 16;

    return wdr_mode;
}

FH_SINT32 changeSensorFPS(FH_SINT32 grp_id, FH_SINT32 fps)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    ISP_INFO *isp_info;
    FH_SINT32 isp_format_bak;
    FH_UINT32 isp_fps;

    isp_info = get_isp_config(grp_id);

    if (!isp_info->enable)
    {
        SDK_ERR_PRT(model_name, "changeSensorFPS Failed, ISP[%d] not enable!\n", grp_id);
        return FH_SDK_FAILED;
    }

    ret = get_isp_fps(grp_id, &isp_fps);
    SDK_FUNC_ERROR(model_name, ret);

    if (fps == isp_fps)
    {
        return ret;
    }

    SDK_PRT(model_name, "Current FPS = %d,Target FPS = %d\n", isp_fps, fps);

    isp_format_bak = isp_info->isp_format;

    switch (fps)
    {
    case 12:
        selectIspFormat_FPS_12P5(isp_info);
        break;
    case 15:
        selectIspFormat_FPS_P15(isp_info);
        break;
    case 20:
        selectIspFormat_FPS_P20(isp_info);
        break;
    case 25:
        selectIspFormat_FPS_P25(isp_info);
        break;
    case 30:
        selectIspFormat_FPS_P30(isp_info);
        break;
    case 40:
        selectIspFormat_FPS_P40(isp_info);
        break;
    case 60:
        selectIspFormat_FPS_P60(isp_info);
        break;

    default:
        break;
    }

    ret = change_isp_format(grp_id, isp_format_bak);
    SDK_FUNC_ERROR(model_name, ret);

    return ret;
}

struct isp_sensor_if *start_sensor(FH_CHAR *sensor_name, FH_SINT32 grp_id)
{
    FH_SINT32 ret = FH_SDK_SUCCESS;
    struct isp_sensor_if *sensor = NULL;

#ifdef FH_USING_DUMMY_SENSOR
    if (!strcmp(sensor_name, "dummy_sensor"))
    {
        sensor = SENSOR_CREATE_MULTI(dummy_sensor)();
    }
#endif
#ifdef FH_USING_OVOS02K_MIPI
    if (!strcmp(sensor_name, "ovos02k_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(ovos02k_mipi)();
    }
#endif
#ifdef FH_USING_OVOS02D_MIPI
    if (!strcmp(sensor_name, "ovos02d_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(ovos02d_mipi)();
    }
#endif
#ifdef FH_USING_OVOS02H_MIPI
    if (!strcmp(sensor_name, "ovos02h_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(ovos02h_mipi)();
    }
#endif
#ifdef FH_USING_OVOS02G10_MIPI
    if (!strcmp(sensor_name, "ovos02g10_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(ovos02g10_mipi)();
    }
#endif
#ifdef FH_USING_OVOS03A_MIPI
    if (!strcmp(sensor_name, "ovos03a_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(ovos03a_mipi)();
    }
#endif
#ifdef FH_USING_OVOS04C10_MIPI
    if (!strcmp(sensor_name, "ovos04c10_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(ovos04c10_mipi)();
    }
#endif
#ifdef FH_USING_OVOS05_MIPI
    if (!strcmp(sensor_name, "ovos05_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(ovos05_mipi)();
    }
#endif
#ifdef FH_USING_OVOS08_MIPI
    if (!strcmp(sensor_name, "ovos08_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(ovos08_mipi)();
    }
#endif
#ifdef FH_USING_IMX415_MIPI
    if (!strcmp(sensor_name, "imx415_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(imx415_mipi)();
    }
#endif
#ifdef FH_USING_SC3335_MIPI
    if (!strcmp(sensor_name, "sc3335_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(sc3335_mipi)();
    }
#endif
#ifdef FH_USING_SC3336_MIPI
    if (!strcmp(sensor_name, "sc3336_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(sc3336_mipi)();
    }
#endif
#ifdef FH_USING_SC4336_MIPI
    if (!strcmp(sensor_name, "sc4336_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(sc4336_mipi)();
    }
#endif
#ifdef FH_USING_SC5336_MIPI
    if (!strcmp(sensor_name, "sc5336_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(sc5336_mipi)();
    }
#endif
#ifdef FH_USING_SC200AI_MIPI
    if (!strcmp(sensor_name, "sc200ai_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(sc200ai_mipi)();
    }
#endif
#ifdef FH_USING_SC450AI_MIPI
    if (!strcmp(sensor_name, "sc450ai_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(sc450ai_mipi)();
    }
#endif
#ifdef FH_USING_SC530AI_MIPI
    if (!strcmp(sensor_name, "sc530ai_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(sc530ai_mipi)();
    }
#endif
#ifdef FH_USING_OV2735_MIPI
    if (!strcmp(sensor_name, "ov2735_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(ov2735_mipi)();
    }
#endif
#ifdef FH_USING_GC2083_MIPI
    if (!strcmp(sensor_name, "gc2083_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(gc2083_mipi)();
    }
#endif
#ifdef FH_USING_CV2005_MIPI
    if (!strcmp(sensor_name, "cv2005_mipi"))
    {
        sensor = SENSOR_CREATE_MULTI(cv2005_mipi)();
    }
#endif

    if (sensor)
    {
        ret = FHAdv_Isp_SensorInit(grp_id, sensor);
        if (ret)
        {
            printf("Error: FHAdv_Isp_SensorInit failed, ret = %d\n", ret);
        }
    }
    else
    {
        printf("sensor %s not support yet!\n", sensor_name);
    }

    return sensor;
}

void destroy_sensor(FH_CHAR *sensor_name, struct isp_sensor_if *sensor)
{
#ifdef FH_USING_DUMMY_SENSOR
    if (!strcmp(sensor_name, "dummy_sensor"))
    {
        SENSOR_DESTROY_MULTI(dummy_sensor)
        (sensor);
    }
#endif
#ifdef FH_USING_OVOS02K_MIPI
    if (!strcmp(sensor_name, "ovos02k_mipi"))
    {
        SENSOR_DESTROY_MULTI(ovos02k_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_OVOS02D_MIPI
    if (!strcmp(sensor_name, "ovos02d_mipi"))
    {
        SENSOR_DESTROY_MULTI(ovos02d_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_OVOS02H_MIPI
    if (!strcmp(sensor_name, "ovos02h_mipi"))
    {
        SENSOR_DESTROY_MULTI(ovos02h_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_OVOS02G10_MIPI
    if (!strcmp(sensor_name, "ovos02g10_mipi"))
    {
        SENSOR_DESTROY_MULTI(ovos02g10_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_OVOS03A_MIPI
    if (!strcmp(sensor_name, "ovos03a_mipi"))
    {
        SENSOR_DESTROY_MULTI(ovos03a_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_OVOS04C10_MIPI
    if (!strcmp(sensor_name, "ovos04c10_mipi"))
    {
        SENSOR_DESTROY_MULTI(ovos04c10_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_OVOS05_MIPI
    if (!strcmp(sensor_name, "ovos05_mipi"))
    {
        SENSOR_DESTROY_MULTI(ovos05_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_OVOS08_MIPI
    if (!strcmp(sensor_name, "ovos08_mipi"))
    {
        SENSOR_DESTROY_MULTI(ovos08_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_IMX415_MIPI
    if (!strcmp(sensor_name, "imx415_mipi"))
    {
        SENSOR_DESTROY_MULTI(imx415_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_SC3335_MIPI
    if (!strcmp(sensor_name, "sc3335_mipi"))
    {
        SENSOR_DESTROY_MULTI(sc3335_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_SC3336_MIPI
    if (!strcmp(sensor_name, "sc3336_mipi"))
    {
        SENSOR_DESTROY_MULTI(sc3336_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_SC4336_MIPI
    if (!strcmp(sensor_name, "sc4336_mipi"))
    {
        SENSOR_DESTROY_MULTI(sc4336_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_SC5336_MIPI
    if (!strcmp(sensor_name, "sc5336_mipi"))
    {
        SENSOR_DESTROY_MULTI(sc5336_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_SC200AI_MIPI
    if (!strcmp(sensor_name, "sc200ai_mipi"))
    {
        SENSOR_DESTROY_MULTI(sc200ai_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_SC450AI_MIPI
    if (!strcmp(sensor_name, "sc450ai_mipi"))
    {
        SENSOR_DESTROY_MULTI(sc450ai_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_SC530AI_MIPI
    if (!strcmp(sensor_name, "sc530ai_mipi"))
    {
        SENSOR_DESTROY_MULTI(sc530ai_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_OV2735_MIPI
    if (!strcmp(sensor_name, "ov2735_mipi"))
    {
        SENSOR_DESTROY_MULTI(ov2735_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_GC2083_MIPI
    if (!strcmp(sensor_name, "gc2083_mipi"))
    {
        SENSOR_DESTROY_MULTI(gc2083_mipi)
        (sensor);
    }
#endif
#ifdef FH_USING_CV2005_MIPI
    if (!strcmp(sensor_name, "cv2005_mipi"))
    {
        SENSOR_DESTROY_MULTI(cv2005_mipi)
        (sensor);
    }
#endif

}
