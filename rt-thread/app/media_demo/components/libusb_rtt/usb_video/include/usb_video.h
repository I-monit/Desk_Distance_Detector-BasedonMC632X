#ifndef __USB_VIDEO_H__
#define __USB_VIDEO_H__

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <fcntl.h>
#include <sys/prctl.h>

#include "types/type_def.h"
#include "list.h"
#include "config.h"
#include "format.h"
#include "libusb_rtt/usb_update/include/usb_update.h"
#include "uvc_callback.h"
#include "uvc_extern.h"
#include "uvc_info.h"

/* #define H264_P_ADD_S_P_PS */

#define FH_STREAM_NV12 (1 << 6)
#define FH_STREAM_YUY2 (1 << 7)
#define FH_STREAM_IR (1 << 8)

struct yuv_frame_buf
{
    FH_UINT8 *ydata;
    FH_UINT8 *uvdata;
    FH_UINT32 ysize;
    FH_UINT32 uvsize;
    FH_UINT64 time_stamp;
    struct list_head list;
};

struct yuy2_frame_buf
{
    FH_UINT8 *data;
    FH_UINT32 size;
    FH_UINT64 time_stamp;
    struct list_head list;
};

struct yuv_buf
{
    FH_UINT8 *data;
    FH_UINT32 ysize;
    FH_UINT32 uvsize;
    FH_UINT32 bufsize;
    FH_UINT32 is_lock;
    FH_UINT32 is_full;
    FH_UINT64 time_stamp;
};
#ifdef ENABLE_PSRAM
#define UVC_YUV_BUF_NUM 0
#else
#define UVC_YUV_BUF_NUM 2
#endif
struct uvc_dev_app
{
    FH_UINT32 fcc;
    FH_UINT32 fps;
    FH_UINT32 width;
    FH_UINT32 height;
    FH_UINT32 iframe;
    FH_UINT32 finterval;

    FH_UINT32 first_change;
    FH_UINT32 isp_complete;
    FH_UINT32 stream_id;
    FH_UINT32 g_h264_delay;
    FH_UINT32 g_h265_delay;

    struct yuv_buf yuv_buf[UVC_YUV_BUF_NUM];
    FH_UINT32 w_index;
    FH_UINT32 r_index;
    struct yuv_buf trans_buffer; // nv12->yuy2
    pthread_mutex_t mutex_fmt;

    FH_VOID(*stream_probe_change)
    (FH_SINT32 stream_id, UVC_FORMAT fmt, FH_SINT32 width, FH_SINT32 height, FH_SINT32 fps);
};

extern pthread_mutex_t g_mutex_sensor;

struct uvc_dev_app *get_uvc_dev(FH_SINT32 id);

FH_SINT32 init_dev_info(struct uvc_dev_app *uvc_dev, FH_SINT32 id);

FH_SINT32 uvc_init(FH_VOID (*stream_probe_change)(FH_SINT32 stream_id, UVC_FORMAT fmt, FH_SINT32 width, FH_SINT32 height, FH_SINT32 fps));

#endif /* __USB_VIDEO_H__ */
