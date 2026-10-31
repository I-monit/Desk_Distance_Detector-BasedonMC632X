#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "libuvc.h"
#include "uvc_init.h"


extern struct uvc_format_info *uvc_formats[2];
extern int uvc_formats_num[2];


#define TMP_DBG(fmt, x...)

void uvc_events_process_vc_data(struct  uvc_stream_info *dev, struct uvc_request_data *data);
void uvc_events_process_vc_setup(struct uvc_stream_info *dev, uint8_t req, uint8_t cs,
        uint8_t id, struct uvc_request_data *resp);


static const unsigned int  uvc_vs_fmt[] = {

    V4L2_PIX_FMT_NV12,
    V4L2_PIX_FMT_YUY2,
    V4L2_PIX_FMT_MJPEG,
    V4L2_PIX_FMT_H264,
    V4L2_PIX_FMT_H265,
    V4L2_PIX_FMT_IR,
};


static unsigned int v4l2_fmt_to_idx(unsigned int fmt)
{
    unsigned int idx = 0, i = 0;

    for (i = 0; i < sizeof(uvc_vs_fmt) / sizeof(uvc_vs_fmt[0]); i++)
    {
        if (fmt == uvc_vs_fmt[i])
        {
            idx = i;
            break;
        }
    }
    return idx;
}

void uvc_fill_streaming_control(struct uvc_stream_info *dev,
               struct uvc_streaming_control *ctrl,
               int iframe, int iformat)
{
    const struct uvc_format_info *format;
    const struct uvc_frame_info *frame;
    unsigned int nframes;

    if (iformat < 0)
        iformat = uvc_formats_num[dev->stream_id] + iformat;
    if (iformat < 0 || iformat >= (int)uvc_formats_num[dev->stream_id])
        return;
    format = &uvc_formats[dev->stream_id][iformat];
    nframes = 0;
    while (format->frames[nframes].width != 0)
        ++nframes;
    if (iframe < 0)
        iframe = nframes + iframe;
    if (iframe < 0 || iframe >= (int)nframes)
        return;
    frame = &format->frames[iframe];
    memset(ctrl, 0, sizeof(*ctrl));

    ctrl->bmHint = 1;
    ctrl->bFormatIndex = iformat + 1;
    ctrl->bFrameIndex = iframe + 1;
    ctrl->dwFrameInterval = dev->probe.dwFrameInterval;
    switch (format->fcc)
    {
    case V4L2_PIX_FMT_YUY2:
        ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    case V4L2_PIX_FMT_NV12:
        ctrl->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    case V4L2_PIX_FMT_MJPEG:
        ctrl->dwMaxVideoFrameSize = 0;
        break;
    case V4L2_PIX_FMT_H264:
        ctrl->dwMaxVideoFrameSize = 0;
        break;
    case V4L2_PIX_FMT_H265:
        ctrl->dwMaxVideoFrameSize = 0;
        break;
    case V4L2_PIX_FMT_IR:
        ctrl->dwMaxVideoFrameSize = frame->width * frame->height;
        break;
    default:
        break;
    }
    /* for mac os compatible,also need to modify usb_endpoint_descriptor wMaxPacketSize */
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    if (dev->stream_id == STREAM_ID1)
        ctrl->dwMaxPayloadTransferSize = UVC_PACKET_SIZE * ISOC_CNT_ON_MICROFRAME;
    else
        ctrl->dwMaxPayloadTransferSize = UVC_PACKET_SIZE;
#else
    ctrl->dwMaxPayloadTransferSize = UVC_BUF_SIZE*2;
#endif
    ctrl->bmFramingInfo = 3;
    ctrl->bPreferedVersion = 1;
    ctrl->bMaxVersion = 1;
}


static void
uvc_events_process_vs_setup(struct uvc_stream_info *dev, uint8_t req, uint8_t cs,
                 struct uvc_request_data *resp)
{
    struct uvc_streaming_control *ctrl;
    struct uvc_vs_user_data *pUser;
    struct stream_info *info;

    UVC_DBG("#%s streaming request(req %02x cs %02x)\n", __func__, req, cs);

    if (cs != UVC_VS_PROBE_CONTROL && cs != UVC_VS_COMMIT_CONTROL
        && cs != UVC_VS_USER_DEF)
        return;

    ctrl = (struct uvc_streaming_control *)&resp->data;
    resp->length = sizeof(*ctrl);


    switch (req)
    {
    case UVC_SET_CUR:
        UVC_DBG("# %s  UVC_SET_CUR\n", __func__);
        dev->context.cs = cs;
        dev->context.id = UVC_CTRL_INTERFACE_ID;
        dev->context.intf = UVC_INTF_STREAMING;
        resp->length = 34;
        break;

    case UVC_GET_CUR:
        UVC_DBG("# %s UVC_GET_CUR\n", __func__);
        if (cs == UVC_VS_USER_DEF)
        {
            pUser = (struct uvc_vs_user_data *)&resp->data;
            info = dev->stream;
            resp->length = 0x0c;
            pUser->cmd = 0x02;
            pUser->fmt = v4l2_fmt_to_idx(info->gFmt);/* TODO */
            pUser->width = info->gPicW;
            pUser->height = info->gPicH;
            pUser->fps = info->gFps;
            pUser->gop = info->gop;
            pUser->bitRate = info->bitRate;
        }
        else
        {
            if (cs == UVC_VS_PROBE_CONTROL)
            {
                rt_memcpy(ctrl, &dev->probe, sizeof(*ctrl));
            }
            else
            {
                rt_memcpy(ctrl, &dev->commit, sizeof(*ctrl));
            }
        }
        break;
    case UVC_GET_MIN:
    case UVC_GET_MAX:
    case UVC_GET_DEF:
        UVC_DBG("# %s UVC_GET_MIN | UVC_GET_MAX |"
            "UVC_GET_DEF\n", __func__);
        rt_memcpy(ctrl, &dev->probe, sizeof(*ctrl));
        // uvc_fill_streaming_control(dev, ctrl,
        //             req == UVC_GET_MAX ? -1 : 0, /* iframe */
        //             req == UVC_GET_MAX ? -1 : 0); /* iformat */
        break;

    case UVC_GET_RES:
        UVC_DBG("# %s UVC_GET_RES\n", __func__);
        memset(ctrl, 0, sizeof(*ctrl));
        break;

    case UVC_GET_LEN:
        UVC_DBG("# %s  UVC_GET_LEN\n", __func__);
        resp->data[0] = 0x00;
        resp->data[1] = 0x22;
        resp->length = 2;
        break;

    case UVC_GET_INFO:
        UVC_DBG("# %s UVC_GET_INFO\n", __func__);
        resp->data[0] = 0x03;
        resp->length = 1;
        break;
    }
}

static void
uvc_events_process_class(struct uvc_stream_info *dev, struct usb_ctrlrequest *ctrl,
             struct uvc_request_data *resp)
{
    UVC_DBG("# %s\n", __func__);
    if ((ctrl->bRequestType & USB_RECIP_MASK) != USB_RECIP_INTERFACE)
        return;

    dev->context.size = ctrl->wLength;
    switch (ctrl->wIndex & 0xff)
    {
    case UVC_INTF_CONTROL:
        uvc_events_process_vc_setup(dev, ctrl->bRequest, ctrl->wValue >> 8,
                ctrl->wIndex >> 8, resp);
        break;

    case UVC_INTF_STREAMING:
    case UVC_INTF_STREAMING2:
        uvc_events_process_vs_setup(dev, ctrl->bRequest, ctrl->wValue >> 8, resp);
        break;

    default:
        rt_kprintf("# %s intf %d is no valid\n", __func__, ctrl->wIndex & 0xff);
        break;
    }
}

/* setup transaction*/
static void
uvc_events_process_setup(struct uvc_stream_info *dev, struct usb_ctrlrequest *ctrl,
             struct uvc_request_data *resp)
{
    UVC_DBG("# bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
        "wLength %04x\n", ctrl->bRequestType, ctrl->bRequest,
        ctrl->wValue, ctrl->wIndex, ctrl->wLength);

    switch (ctrl->bRequestType & USB_TYPE_MASK)
    {
    case USB_TYPE_CLASS:
        uvc_events_process_class(dev, ctrl, resp);
        break;
    default:
        rt_kprintf("# bRequestType %02x bRequest %02x wValue %04x wIndex %04x "
        "wLength %04x\n", ctrl->bRequestType, ctrl->bRequest,
        ctrl->wValue, ctrl->wIndex, ctrl->wLength);
        break;
    }
}

#ifdef FH_RT_UVC_EP_TYPE_ISOC
static int uvc_setalt_select(unsigned int alt_set)
{
    switch (alt_set)
    {
    case 1:
        return UVC_PACKET_SIZE_ALT1;
    case 2:
        return UVC_PACKET_SIZE_ALT2;
    case 3:
        return UVC_PACKET_SIZE_ALT3;
    case 4:
        return UVC_PACKET_SIZE_ALT4;
    case 5:
        return UVC_PACKET_SIZE_ALT5;
    case 6:
        return UVC_PACKET_SIZE_ALT6;
    case 7:
        return UVC_PACKET_SIZE_ALT7;
    case 8:
        return UVC_PACKET_SIZE_ALT8;
    default:
        rt_kprintf("%s-%d:[warning] Alt %d is not implemented!", __func__, __LINE__, alt_set);
        return UVC_PACKET_SIZE_ALT1;
    }
    return UVC_PACKET_SIZE_ALT1;
}
#endif
/*data transaction*/
void uvc_events_process_vs_data(struct uvc_stream_info *dev,
    struct uvc_request_data *data)
{
    UVC_DBG("#%s\n",__func__);
    struct uvc_streaming_control *target;
    struct uvc_streaming_control *ctrl;
    const struct uvc_format_info *format;
    const struct uvc_frame_info *frame;
    const unsigned int *interval;
    unsigned int iformat, iframe;
    unsigned int nframes;
    int i;
    struct stream_info *info;

    struct uvc_vs_user_data *pUser = (struct uvc_vs_user_data *)data->data;


    switch (dev->context.cs)
    {
    case UVC_VS_PROBE_CONTROL:
        UVC_DBG("# setting probe control, length = %d\n", data->length);
        target = &dev->probe;
        break;

    case UVC_VS_COMMIT_CONTROL:
        UVC_DBG("# setting commit control, length = %d,\n", data->length);
        target = &dev->commit;
        break;

    case UVC_VS_USER_DEF:
        UVC_DBG("user cmd:\r\n");
        for (i = 0; i < data->length; i++)
        {
            rt_kprintf("[%d] = 0x%02x\r\n", i, data->data[i]);
        }

        rt_kprintf("\r\n");
        info = dev->stream;
        if (pUser->cmd == 1)
        {
            if (V4L2_PIX_FMT_H264 == info->gFmt || V4L2_PIX_FMT_H265 == info->gFmt)
            {
                rt_kprintf("TODO: require i frame\r\n");
            }
        }
        else if (pUser->cmd == 2)
        {
            rt_kprintf("fmt = %d\r\n",      pUser->fmt);
            rt_kprintf("width = %d\r\n",    pUser->width);
            rt_kprintf("height = %d\r\n",   pUser->height);
            rt_kprintf("fps = %d\r\n",      pUser->fps);
            rt_kprintf("gop = %d\r\n",      pUser->gop);
            rt_kprintf("bitRate = %d\r\n",  pUser->bitRate);
            rt_sem_release(dev->uvc_format_sem);
        }
        return;

    default:
        UVC_DBG("# setting unknown control, cs = %d, length = %d\n", dev->context.cs, data->length);
         return;
    }

    ctrl = (struct uvc_streaming_control *)&data->data;


    iformat = clamp((unsigned int)ctrl->bFormatIndex, 1U,
        (unsigned int)uvc_formats_num[dev->stream_id]);
    format = &uvc_formats[dev->stream_id][iformat-1];

    nframes = 0;
    while (format->frames[nframes].width != 0)
        ++nframes;

    iframe = clamp((unsigned int)ctrl->bFrameIndex, 1U, nframes);
    frame = &format->frames[iframe-1];
    interval = frame->intervals;

    while (interval[0] < ctrl->dwFrameInterval && interval[1])
        ++interval;
    UVC_DBG("#%s dwFrameInterval=%d\n",__func__,
            ctrl->dwFrameInterval);

    target->bFormatIndex = iformat;
    target->bFrameIndex = iframe;
    switch (format->fcc)
    {
    case V4L2_PIX_FMT_YUY2:
        target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    case V4L2_PIX_FMT_NV12:
        target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    case V4L2_PIX_FMT_MJPEG:
        /* TODO diff resolution diff size */
        target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    case V4L2_PIX_FMT_H264:
    case V4L2_PIX_FMT_H265:
        target->dwMaxVideoFrameSize = frame->width * frame->height * 2;
        break;
    case V4L2_PIX_FMT_IR:
        target->dwMaxVideoFrameSize = frame->width * frame->height;
        break;
    default:
        break;
    }

#ifdef FH_RT_UVC_EP_TYPE_ISOC
    if (dev->stream_id == STREAM_ID1)
        target->dwMaxPayloadTransferSize = uvc_setalt_select(frame->alt_set);
    else
        target->dwMaxPayloadTransferSize = UVC_PACKET_SIZE;
#else
    target->dwMaxPayloadTransferSize = UVC_BUF_SIZE*2;
#endif
    target->dwFrameInterval = *interval;
    dev->commit.dwFrameInterval = *interval;
    dev->probe.dwFrameInterval = *interval;
    if (dev->context.cs == UVC_VS_COMMIT_CONTROL)
    {
        dev->fcc = format->fcc;
        dev->width = frame->width;
        dev->height = frame->height;
        dev->iframe = iframe-1; /* save current resolution */
        dev->finterval = *interval;
        // rt_sem_release(dev->uvc_format_sem);
    }
}

static void
uvc_events_process_data(struct uvc_stream_info *dev, struct uvc_request_data *data)
{

    switch (dev->context.intf)
    {
    case UVC_INTF_CONTROL:
        uvc_events_process_vc_data(dev, data);
        break;
    case UVC_INTF_STREAMING:
    case UVC_INTF_STREAMING2:
        uvc_events_process_vs_data(dev, data);
        break;
    default:
        UVC_DBG("#%s interface %d is not exist\n",
            __func__, dev->context.intf);
        break;
    }
}

extern struct uvc_stream_info *stream_info[2];

struct uvc_request_data *
uvc_events_process(int stream_id, void *event, unsigned int type)
{
    UVC_DBG("# %s\n", __func__);
    static struct uvc_request_data resp;
    struct uvc_stream_info *pDev = stream_info[stream_id];
    struct uvc_event *uvc_event = (struct uvc_event *)event;

    memset(&resp, 0, sizeof(resp));
    resp.length = 0;

    switch (type)
    {
    case UVC_EVENT_SETUP:
        UVC_DBG("# %s => setup phase\n", __func__);
        uvc_events_process_setup(pDev, &uvc_event->req, &resp);
        break;

    case UVC_EVENT_DATA:
        UVC_DBG("# %s => data phase\n", __func__);
        uvc_events_process_data(pDev, &uvc_event->data);
        return NULL;

    default:
        rt_kprintf("# unkown cmd: %x\n", type);
        break;
    }

    return &resp;
}


