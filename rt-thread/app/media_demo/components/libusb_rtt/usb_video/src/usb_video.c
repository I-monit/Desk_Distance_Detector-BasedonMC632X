#include "libusb_rtt/usb_video/include/usb_video.h"

#define UVC_VERSION "20200915_v2.2"

pthread_mutex_t g_mutex_sensor;
static struct uvc_dev_app *g_uvc_dev[2] = {NULL, NULL};
struct uvc_dev_app *get_uvc_dev(FH_SINT32 id)
{
    if (id != STREAM_ID1 && id != STREAM_ID2)
    {
        printf("Error get_uvc_dev failed! id not exist!\n");
        return NULL;
    }

    return g_uvc_dev[id];
}

static struct uvc_stream_ops uvc_ops = {
    .format_change = NULL,
#ifdef FH_APP_USING_HEX_PARA
    .uvc_extern_data = uvc_extern_intr_ctrl,
    .uvc_extern_intr = uvc_extern_intr_proc,
    .uvc_vision = uvc_info_option,
#ifdef FH_APP_UVC_PARAM_HISTORY
    .uvc_uspara = uvc_para_read,
#endif
#endif
    .stream_on_callback = uvc_stream_on,
    .stream_off_callback = uvc_stream_off,

    .SetAEMode = Uvc_SetAEMode,
    .SetExposure = Uvc_SetExposure,
    .SetGain = Uvc_SetGain,
    .SetBrightness = Uvc_SetBrightness,
    .SetContrast = Uvc_SetContrast,
    .SetSaturation = Uvc_SetSaturation,
    .SetSharpeness = Uvc_SetSharpeness,
    .SetAWBGain = Uvc_SetAWBGain,
    .SetAwbMode = Uvc_SetAwbMode,
};

#ifdef H264_P_ADD_S_P_PS
static FH_UINT8 *h264_sps_buf = NULL;
static FH_UINT8 *h264_pps_buf = NULL;
static FH_UINT32 h264_sps_len = 0;
static FH_UINT32 h264_pps_len = 0;
#endif

#ifndef ENABLE_PSRAM
static FH_VOID yuv_buf_init(struct uvc_dev_app *pDev)
{
    FH_UINT32 y_size = 0;
    FH_UINT32 uv_size = 0;

    if (pDev->stream_id == STREAM_ID1)
    {
        y_size = UVC1_MAX_W * UVC1_MAX_H;
        uv_size = UVC1_MAX_W * UVC1_MAX_H;
    }
    else if (pDev->stream_id == STREAM_ID2)
    {
        y_size = UVC2_MAX_W * UVC2_MAX_H;
        uv_size = UVC2_MAX_W * UVC2_MAX_H;
    }

#ifdef NV12_TO_YUY2
    pDev->trans_buffer.data = malloc(y_size + uv_size / 2);
    pDev->trans_buffer.bufsize = y_size + uv_size;
    pDev->trans_buffer.ysize = y_size;
    pDev->trans_buffer.uvsize = uv_size;
    pDev->trans_buffer.is_lock = 0;
    pDev->trans_buffer.is_full = 0;
    pDev->trans_buffer.time_stamp = 0;
#endif

    // TODO
    for (int i = 0; i < UVC_YUV_BUF_NUM; i++)
    {
        pDev->yuv_buf[i].data = malloc(y_size + uv_size + 4);
        pDev->yuv_buf[i].bufsize = y_size + uv_size;
        pDev->yuv_buf[i].ysize = y_size;
        pDev->yuv_buf[i].uvsize = uv_size;
        pDev->yuv_buf[i].is_lock = 0;
        pDev->yuv_buf[i].is_full = 0;
        pDev->yuv_buf[i].time_stamp = 0;
    }
}
#endif

struct uvc_format_info *get_uvc_format(int stream_id)
{
    if (stream_id == STREAM_ID1)
        return (struct uvc_format_info *)uvc_formats;
#ifdef UVC_DOUBLE_STREAM
    else
        return (struct uvc_format_info *)uvc_formats2;
#endif
    return NULL;
}

int get_uvc_format_num(int stream_id)
{
    if (stream_id == STREAM_ID1)
        return ARRAY_SIZE(uvc_formats);
#ifdef UVC_DOUBLE_STREAM
    else
        return ARRAY_SIZE(uvc_formats2);
#endif
    return 0;
}

FH_SINT32 init_dev_info(struct uvc_dev_app *uvc_dev, FH_SINT32 id)
{
    FH_SINT32 ret = 0;

    if (uvc_dev == NULL)
    {
        printf("Error init_dev_info failed! uvc_dev == NULL!\n");
        return -1;
    }

    uvc_dev->stream_id = id;
    // TODO
    uvc_dev->w_index = 0;
    uvc_dev->r_index = 0;
    fh_uvc_init(id, get_uvc_format(id), get_uvc_format_num(id));
    pthread_mutex_init(&uvc_dev->mutex_fmt, NULL);
    uvc_dev->isp_complete = 1;
    if (id == STREAM_ID1 || id == STREAM_ID2)
    {
        if (g_uvc_dev[id] == NULL)
        {
            g_uvc_dev[id] = uvc_dev;
            printf("UVC device[%d] addr = %p create success!\n", id, uvc_dev);
        }
        else
        {
            printf("UVC device[%d] has created!\n", id);
        }
    }
    else
    {
        ret = -1;
        printf("UVC device[%d] not exist!\n", id);
    }
#ifndef ENABLE_PSRAM
    yuv_buf_init(uvc_dev);
#endif
    // if (id == STREAM_ID1)
    // {
    //     yuv_buf_init(uvc_dev);
    // }

    return ret;
}

FH_SINT32 uvc_init(FH_VOID (*stream_probe_change)(FH_SINT32 stream_id, UVC_FORMAT fmt, FH_SINT32 width, FH_SINT32 height, FH_SINT32 fps))
{
    FH_SINT32 ret = 0;
    printf("uvc run %s\n", UVC_VERSION);
    fh_uvc_flash_init();
    uvc_ops.format_change = stream_probe_change;
    fh_uvc_ops_register(&uvc_ops);
    pthread_mutex_init(&g_mutex_sensor, NULL);

    printf("%s uvc_init finish!\n", __func__);
    return ret;
}
