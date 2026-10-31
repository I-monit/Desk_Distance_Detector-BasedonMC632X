#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/prctl.h>
#include "isp_config.h"
#include "isp/isp_sensor_if.h"
#include "dsp_ext/FHAdv_Isp_mpi_v3.h"
#include "libusb_rtt/usb_video/include/uvc_callback.h"

#define SENSOR_SLEEP_DELAY_S 20

static FH_SINT32 g_AE_mode = 0;
static FH_SINT32 g_AE_time_level = 0;
static FH_SINT32 g_AE_gain_level = 0;

static FH_SINT32 g_uspara_change = 0;
static FH_SINT32 g_uvc_uspara_save[USER_PARAM_SUPPORT_CNT];

#ifdef UVC_WINDOWS_HELLO_FACE
static FH_SINT32 ir_stream_on = 0;
#endif
FH_VOID uvc_stream_on(FH_SINT32 stream_id)
{
    struct uvc_dev_app *uvc_dev = get_uvc_dev(stream_id);

    if (!uvc_dev)
    {
        printf("%s failed,uvc dev not ready!\n", __func__);
        return;
    }

    while (!uvc_dev->isp_complete)
        usleep(10000);

    pthread_mutex_lock(&g_mutex_sensor);
    set_isp_sleep(FH_APP_GRP_ID, 0);
    pthread_mutex_unlock(&g_mutex_sensor);
    printf("%s:stream_id %d\n", __func__, stream_id);
#ifdef UVC_WINDOWS_HELLO_FACE
    if (stream_id == STREAM_ID2)
        ir_stream_on = 1;
#endif
}

FH_VOID uvc_stream_off(FH_SINT32 stream_id)
{
    g_uspara_change++;
#ifdef UVC_WINDOWS_HELLO_FACE
    if (stream_id == STREAM_ID2)
        ir_stream_on = 0;
#endif
    printf("%s:stream_id %d\n", __func__, stream_id);
}

FH_VOID Uvc_SetAEMode(FH_UINT32 mode)
{
    FH_UINT8 setflg = 0;

    if (mode == 4 || mode == 1)
    {
        if (g_AE_mode != 3)
        {
            g_AE_mode = 3;
            g_AE_time_level = 3;
            g_AE_gain_level = GAIN_DEF; /* defaut */
            setflg = 1;
        }
    }
    else
    {
        if (g_AE_mode != 0)
        {
            g_AE_mode = 0; /* auto Exposure */
            setflg = 1;
        }
    }
    if (setflg)
    {
        FHAdv_Isp_SetAEMode(FH_APP_GRP_ID, g_AE_mode, g_AE_time_level, g_AE_gain_level);
        g_uvc_uspara_save[USER_PARAM_AEMODE] = mode;
        g_uspara_change++;
    }
}

/**
 * level:
 * 0:inttMax*8
 * 1:inttMax*4
 * 2:inttMax*2
 * 3:inttMax --< begin exp=2500
 * 4:inttMax/2 exp=1250
 * 5:inttMax/4 exp=625
 * 6:inttMax/6 exp=312
 * 7:inttMax/8 exp=156
 * 8:inttMax/10 exp=78
 * 9:inttMax/20 exp=39
 * 10:inttMax/30 exp=20
 * 11:inttMax/40  exp=10
 * 12:inttMax/50 --<end exp=5
 */
static FH_SINT32 ct_ae_exp_time2level(FH_UINT32 time)
{
    FH_UINT32 i;
    FH_UINT32 res;

    for (i = 0; i < 31; i++)
    {
        if ((time >> i) == 0)
        {
            break;
        }
    }
    res = 15 - i;
    return res;
}

FH_VOID Uvc_SetExposure(FH_UINT32 time_value)
{
    static FH_UINT32 last_time_v = 0;

    if (g_AE_mode == 3)
    {
        if (last_time_v == time_value)
            return;

        last_time_v = time_value;
        g_AE_time_level = ct_ae_exp_time2level(time_value);
        FHAdv_Isp_SetAEMode(FH_APP_GRP_ID, g_AE_mode, g_AE_time_level, g_AE_gain_level);
        g_uvc_uspara_save[USER_PARAM_EXPOSURE] = time_value;
        g_uspara_change++;
    }
}

FH_VOID Uvc_SetGain(FH_UINT32 gain_level)
{
    static FH_UINT32 last_time_v = 0;

    if (g_AE_mode == 3)
    {
        if (last_time_v == gain_level)
            return;

        last_time_v = gain_level;
        g_AE_gain_level = gain_level;
        FHAdv_Isp_SetAEMode(FH_APP_GRP_ID, g_AE_mode, g_AE_time_level, g_AE_gain_level);
        g_uvc_uspara_save[USER_PARAM_GAIN] = gain_level;
        g_uspara_change++;
    }
}

FH_VOID Uvc_SetBrightness(FH_UINT32 value)
{
    static FH_SINT32 cur_Brightness = USER_PARAM_NO_CHANGE_FLAG;
    static FH_UINT32 last_time_v = 0;
    FH_SINT32 set_Brightness = 0;

    if (last_time_v == value)
        return;

    last_time_v = value;
    if (cur_Brightness < 0)
        FHAdv_Isp_GetBrightness(FH_APP_GRP_ID, (FH_UINT32 *)&cur_Brightness);

    set_Brightness = cur_Brightness + value - BIGHTNESS_DEF;
    if (set_Brightness <= BIGHTNESS_MIN)
        set_Brightness = BIGHTNESS_MIN;
    if (set_Brightness >= BIGHTNESS_MAX)
        set_Brightness = BIGHTNESS_MAX;
    FHAdv_Isp_SetBrightness(FH_APP_GRP_ID, set_Brightness);
    g_uvc_uspara_save[USER_PARAM_BRIGHTNESS] = value;
    g_uspara_change++;
}

FH_VOID Uvc_SetContrast(FH_UINT32 value)
{
    static FH_SINT32 cur_Contrast = USER_PARAM_NO_CHANGE_FLAG;
    static FH_UINT32 cur_mode = 0;
    FH_SINT32 set_Contrast = 0;
    static FH_UINT32 last_time_v = 0;

    if (last_time_v == value)
        return;

    last_time_v = value;

    if (cur_Contrast < 0)
        FHAdv_Isp_GetContrast(FH_APP_GRP_ID, &cur_mode, (FH_UINT32 *)&cur_Contrast);

    set_Contrast = cur_Contrast + value - CONTRAST_DEF;
    if (set_Contrast <= CONTRAST_MIN)
        set_Contrast = CONTRAST_MIN;
    if (set_Contrast >= CONTRAST_MAX)
        set_Contrast = CONTRAST_MAX;
    FHAdv_Isp_SetContrast(FH_APP_GRP_ID, cur_mode, set_Contrast);
    g_uvc_uspara_save[USER_PARAM_CONTRAST] = value;
    g_uspara_change++;
}

FH_VOID Uvc_SetSaturation(FH_SINT32 value)
{
    static FH_SINT32 cur_Saturation = USER_PARAM_NO_CHANGE_FLAG;
    static FH_UINT32 cur_mode = 0;
    FH_SINT32 set_Saturation = 0;
    static FH_SINT32 last_time_v = 0;

    if (last_time_v == value)
        return;

    last_time_v = value;
    if (cur_Saturation < 0)
        FHAdv_Isp_GetSaturation(FH_APP_GRP_ID, &cur_mode, (FH_UINT32 *)&cur_Saturation);

    set_Saturation = cur_Saturation + value - SATURATION_DEF;
    if (set_Saturation <= SATURATION_MIN)
        set_Saturation = SATURATION_MIN;
    if (set_Saturation >= SATURATION_MAX)
        set_Saturation = SATURATION_MAX;
    FHAdv_Isp_SetSaturation(FH_APP_GRP_ID, cur_mode, set_Saturation);
    g_uvc_uspara_save[USER_PARAM_SATURATION] = value;
    g_uspara_change++;
}

FH_VOID Uvc_SetSharpeness(FH_SINT32 value)
{
    static FH_SINT32 cur_Sharpeness = USER_PARAM_NO_CHANGE_FLAG;
    static FH_UINT32 cur_mode = 0;
    FH_SINT32 set_Sharpeness = 0;
    static FH_SINT32 last_time_v = 0;

    if (last_time_v == value)
        return;

    last_time_v = value;
    if (cur_Sharpeness < (-SHARPENESS_MAX))
        FHAdv_Isp_GetSharpeness(FH_APP_GRP_ID, &cur_mode, (FH_UINT32 *)&cur_Sharpeness);

    set_Sharpeness = cur_Sharpeness + 2 * (value - SHARPENESS_DEF);
    if (set_Sharpeness <= (-SHARPENESS_MAX))
        set_Sharpeness = (-SHARPENESS_MAX);
    if (set_Sharpeness >= SHARPENESS_MAX)
        set_Sharpeness = SHARPENESS_MAX;
    FHAdv_Isp_SetSharpeness(FH_APP_GRP_ID, cur_mode, set_Sharpeness);
    g_uvc_uspara_save[USER_PARAM_SHARPENESS] = value;
    g_uspara_change++;
}

FH_VOID Uvc_SetAwbMode(FH_UINT32 value)
{
    static FH_UINT32 last_time_v = 0;

    if (last_time_v == value)
        return;

    last_time_v = value;
    /* TODO */
    g_uvc_uspara_save[USER_PARAM_AWBMODE] = value;
    g_uspara_change++;
}

FH_VOID Uvc_SetAWBGain(FH_UINT32 value)
{
    static FH_SINT32 cur_awbgain = USER_PARAM_NO_CHANGE_FLAG;
    FH_SINT32 set_awbgain;
    static FH_UINT32 cur_mode = 0;
    static FH_UINT32 last_time_v = 0;

    if (last_time_v == value)
        return;

    last_time_v = value;
    /* TODO : current just as example*/
    if (cur_awbgain < 0)
        FHAdv_Isp_GetContrast(FH_APP_GRP_ID, &cur_mode, (FH_UINT32 *)&cur_awbgain);

    set_awbgain = cur_awbgain + value - AWBGAIN_DEF;
    if (set_awbgain <= AWBGAIN_MIN)
        set_awbgain = AWBGAIN_MIN;
    if (set_awbgain >= AWBGAIN_MAX)
        set_awbgain = AWBGAIN_MAX;
    /* FHAdv_Isp_SetContrast(FH_APP_GRP_ID, cur_mode, set_awbgain); */
    g_uvc_uspara_save[USER_PARAM_AWBGAIN] = value;
    g_uspara_change++;
}

FH_VOID Uvc_SetGammaCfg(FH_UINT32 value)
{
    /* TODO : current just as example*/
    static FH_SINT32 cur_gammacfg = USER_PARAM_NO_CHANGE_FLAG;
    static FH_UINT32 cur_mode = 0;
    FH_SINT32 set_gammacfg = 0;
    static FH_UINT32 last_time_v = 0;

    if (last_time_v == value)
        return;

    last_time_v = value;
    if (cur_gammacfg < (-GAMMACFG_MAX))
        FHAdv_Isp_GetSharpeness(FH_APP_GRP_ID, &cur_mode, (FH_UINT32 *)&cur_gammacfg); /* example */

    set_gammacfg = cur_gammacfg + 2 * (value - GAMMACFG_DEF);
    if (set_gammacfg <= (-GAMMACFG_MAX))
        set_gammacfg = (-GAMMACFG_MAX);
    if (set_gammacfg >= GAMMACFG_MAX)
        set_gammacfg = GAMMACFG_MAX;
    /* FHAdv_Isp_SetSharpeness(FH_APP_GRP_ID, cur_mode, set_gammacfg); */ /* example */
    g_uvc_uspara_save[USER_PARAM_GAMMACFG] = value;
    g_uspara_change++;
}

#ifdef FH_APP_UVC_PARAM_HISTORY
static FH_VOID sensor_sleep_delay(FH_VOID)
{
    FH_SINT32 i;

#ifdef UVC_DOUBLE_STREAM
    if (!fh_uvc_status_get(STREAM_ID1) && !fh_uvc_status_get(STREAM_ID2))
    {
        for (i = 0; i < (SENSOR_SLEEP_DELAY_S * 10); i++)
        {
            usleep(100000);
            if (fh_uvc_status_get(STREAM_ID1) || fh_uvc_status_get(STREAM_ID2))
                break;
        }
        pthread_mutex_lock(&g_mutex_sensor);
        if (i == (SENSOR_SLEEP_DELAY_S * 10) &&
            !fh_uvc_status_get(STREAM_ID1) &&
            !fh_uvc_status_get(STREAM_ID2))
        {
            set_isp_sleep(FH_APP_GRP_ID, 1);
        }
        pthread_mutex_unlock(&g_mutex_sensor);
    }
#else
    if (!fh_uvc_status_get(STREAM_ID1))
    {
        for (i = 0; i < (SENSOR_SLEEP_DELAY_S * 10); i++)
        {
            usleep(100000);
            if (fh_uvc_status_get(STREAM_ID1))
                break;
        }

        pthread_mutex_lock(&g_mutex_sensor);
        if (i == (SENSOR_SLEEP_DELAY_S * 10) && !fh_uvc_status_get(STREAM_ID1))
        {
            set_isp_sleep(FH_APP_GRP_ID, 1);
        }
        pthread_mutex_unlock(&g_mutex_sensor);
    }
#endif
}

FH_SINT32 *uvc_para_read(FH_VOID)
{
    FH_UINT32 ret, i;
    FH_SINT32 para_size = sizeof(g_uvc_uspara_save);

    for (i = 0; i < USER_PARAM_SUPPORT_CNT; i++)
        g_uvc_uspara_save[i] = USER_PARAM_NO_CHANGE_FLAG;

    ret = fh_uvc_ispara_load(ISP_USER_PARAM_ADDR, (FH_CHAR *)g_uvc_uspara_save, &para_size);
    if (ret == 0)
        printf("uvc_para_read read 0x%x ok!\n", ISP_USER_PARAM_ADDR);

    return g_uvc_uspara_save;
}

FH_SINT32 uvc_para_write(FH_SINT32 *para)
{
    FH_UINT32 crc = 0;
    FH_UINT32 para_size = sizeof(g_uvc_uspara_save);
    FH_UINT8 user_param_buff[para_size + 16];
    FH_UINT32 ret;

    memcpy(user_param_buff + 16, para, para_size);
    crc = calcCRC((FH_VOID *)user_param_buff + 16, para_size);
    int2Byte(user_param_buff, crc);
    int2Byte(user_param_buff + 4, 0xffffffff - crc);
    int2Byte(user_param_buff + 8, para_size);
    int2Byte(user_param_buff + 12, 0xffffffff - para_size);

    ret = Flash_Base_Write(ISP_USER_PARAM_ADDR, (FH_VOID *)user_param_buff, para_size + 16);
    if (ret == 0)
    {
        printf("Error: uvc_para_write fail!!!\n");
        return -1;
    }
    return (ret == para_size + 16);
}

FH_VOID *uvc_uspara_proc(FH_VOID *arg)
{
    prctl(PR_SET_NAME, "uvc_uspara_proc");
    while (1)
    {
        if (g_uspara_change)
        {
            g_uspara_change = 0;
#ifdef FH_APP_USING_HEX_PARA
            uvc_para_write(g_uvc_uspara_save); // 存用户参数
#endif
            sensor_sleep_delay(); // 图关了，sensor进入低功耗，20s（可定义）为了防止切图的时候来回进入低功耗
            usleep(1000 * 100);
        }
        else
            usleep(1000 * 1000);
    }
    return NULL;
}

#endif
