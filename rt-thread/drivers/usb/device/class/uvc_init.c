#include <rtthread.h>
#include <stdio.h>
#include <pthread.h>
#include "libuvc.h"
#include "video.h"
#include "uvc_init.h"
#include "mmu.h"

extern long set_memory(unsigned int addr, unsigned int val);

static rt_thread_t g_uvc_driver_thread[2];
static rt_thread_t g_thread_uvc_event;

extern void *fh_dma_memcpy(void *dst, const void *src, int len);
extern struct udevice *g_uvcdevice;

struct rt_messagequeue g_uvc_event_mq;
static rt_uint8_t uvc_mq_pool[(sizeof(struct uvc_ctrl_event) + sizeof(void *)) * 8];

#ifdef UVC_DOUBLE_STREAM
static struct uvc_video_buf gVideoBuf[2];
#else
static struct uvc_video_buf gVideoBuf[1];
#endif

struct uvc_stream_info *stream_info[2];
struct uvc_stream_ops *g_uvc_ops = NULL;

#define UVC_STREAM_EOF                    (1 << 1)
#define UVC_STREAM_FID                    (1 << 0)

/* #define UVCQ_DWC_DEBUG */
#ifdef UVCQ_DWC_DEBUG
#define UVCQ_DWC_DBG(fmt, args...)  rt_kprintf(fmt, ##args)
#else
#define UVCQ_DWC_DBG(fmt, args...)
#endif

extern struct uvc_control_ops uvc_control_ct[];
extern struct uvc_control_ops uvc_control_pu[];
void send_uvc_status_packet(uint8_t intf, uint8_t id, uint8_t bEvent, uint8_t cs, uint8_t attr, uint32_t value)
{
    struct UVC_STATUS_PACKET_FORMAT_VC uvc_status_packet_vc;
    struct UVC_STATUS_PACKET_FORMAT_VS uvc_status_packet_vs;
    struct uio_request *req;
    struct uendpoint *ep = video_control_ep;
    unsigned char *status_buf = NULL;
    unsigned char status_len = 0;

    if (intf == 1)
    {
        uvc_status_packet_vc.bStatusType = intf;
        uvc_status_packet_vc.bOriginator = id;
        uvc_status_packet_vc.bEvent = bEvent;
        uvc_status_packet_vc.bSelector = cs;
        uvc_status_packet_vc.bAttribute = attr;
        rt_memset(uvc_status_packet_vc.bValue, 0, 8);
        uvc_status_packet_vc.bValue[0] = value & 0xff;
        uvc_status_packet_vc.bValue[1] = (value >> 8) & 0xff;
        uvc_status_packet_vc.bValue[2] = (value >> 16) & 0xff;
        uvc_status_packet_vc.bValue[3] = (value >> 24) & 0xff;
        status_buf = (unsigned char *)&uvc_status_packet_vc;
        status_len = 5 + uvc_control_pu[cs].len;
    }
    else if (intf == 2)
    {
        uvc_status_packet_vs.bStatusType = intf;
        uvc_status_packet_vs.bOriginator = id;
        uvc_status_packet_vs.bEvent = bEvent;
        uvc_status_packet_vs.bValue = value & 0xff;
        status_buf = (unsigned char *)&uvc_status_packet_vs;
        status_len = 4;
    }
    req = &ep->request;
    req->buffer = status_buf;
    req->size = status_len;
    req->req_type   = UIO_REQUEST_WRITE;
    rt_usbd_io_request(g_uvcdevice, ep, req);
}


void uvc_event_proc(void *arg)
{
    struct uvc_ctrl_event uvc_event;

    while (1)
    {
        if (g_uvc_ops && rt_mq_recv(&g_uvc_event_mq, &uvc_event, sizeof(uvc_event), RT_WAITING_FOREVER) == RT_EOK)
        {
            switch (uvc_event.func_num)
            {
            case UVC_PARAM_FUNC_SETAEMODE:
                if (g_uvc_ops->SetAEMode)
                {
                    g_uvc_ops->SetAEMode(uvc_event.parm1);
                    send_uvc_status_packet(UVC_STATUS_TYPE_VC,
                        UVC_STATUS_VC_TERMINAL_ID,
                        UVC_STATUS_EVENT_CTRL_CHANGE,
                        UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL,
                        UVC_STATUS_ATTRI_CONTROL_INFO_CHANGE,
                        (uvc_event.parm1 == 4 || uvc_event.parm1 == 1) ? 0x0b : 0x0f);
                }
                break;
            case UVC_PARAM_FUNC_SETEXPOSURE:
                if (g_uvc_ops->SetExposure)
                    g_uvc_ops->SetExposure(uvc_control_ct[UVC_CT_EXPOSURE_TIME_ABSOLUTE_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_SETIRIS:
                if (g_uvc_ops->SetIris)
                    g_uvc_ops->SetIris(uvc_control_ct[UVC_CT_IRIS_ABSOLUTE_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_SETGAIN:
                if (g_uvc_ops->SetGain)
                    g_uvc_ops->SetGain(uvc_control_pu[UVC_PU_GAIN_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_SETBRIGHTNESS:
                if (g_uvc_ops->SetBrightness)
                    g_uvc_ops->SetBrightness(uvc_control_pu[UVC_PU_BRIGHTNESS_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_SETCONTRAST:
                if (g_uvc_ops->SetContrast)
                    g_uvc_ops->SetContrast(uvc_control_pu[UVC_PU_CONTRAST_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_SETSATURATION:
                if (g_uvc_ops->SetSaturation)
                    g_uvc_ops->SetSaturation(uvc_control_pu[UVC_PU_SATURATION_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_SETSHARPENESS:
                if (g_uvc_ops->SetSharpeness)
                    g_uvc_ops->SetSharpeness(uvc_control_pu[UVC_PU_SHARPNESS_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_SETGAMMACFG:
                if (g_uvc_ops->SetGammaCfg)
                    g_uvc_ops->SetGammaCfg(uvc_control_pu[UVC_PU_GAMMA_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_SETAWBGAIN:
                if (g_uvc_ops->SetAWBGain)
                    g_uvc_ops->SetAWBGain(uvc_control_pu[UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_SETAWBMODE:
                if (g_uvc_ops->SetAwbMode)
                {
                    g_uvc_ops->SetAwbMode(uvc_event.parm1);
                    send_uvc_status_packet(UVC_STATUS_TYPE_VC,
                        UVC_STATUS_VC_PU_ID,
                        UVC_STATUS_EVENT_CTRL_CHANGE,
                        UVC_PU_WHITE_BALANCE_TEMPERATURE_CONTROL,
                        UVC_STATUS_ATTRI_CONTROL_INFO_CHANGE,
                        uvc_event.parm1 == 0 ? 0x0B : 0x0F);
                }
                break;
            case UVC_PARAM_FUNC_SETBACKLIGHT:
                if (g_uvc_ops->SetBacklight)
                    g_uvc_ops->SetBacklight(uvc_control_pu[UVC_PU_BACKLIGHT_COMPENSATION_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_HUEAUTO:
                if (g_uvc_ops->SetHueAuto)
                    g_uvc_ops->SetHueAuto(uvc_event.parm1);
                break;
            case UVC_PARAM_FUNC_HUEGAIN:
                if (g_uvc_ops->SetHueGain)
                    g_uvc_ops->SetHueGain(uvc_control_pu[UVC_PU_HUE_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_POWERLINE:
                if (g_uvc_ops->SetPowerLine)
                    g_uvc_ops->SetPowerLine(uvc_event.parm1);
                break;
            case UVC_PARAM_FUNC_AE_PRIORITY:
                if (g_uvc_ops->SetAe_Priority)
                    g_uvc_ops->SetAe_Priority(uvc_event.parm1);
                break;
            case UVC_PARAM_FUNC_FOCUSMODE:
                if (g_uvc_ops->SetFocusMode)
                    g_uvc_ops->SetFocusMode(uvc_event.parm1);
                break;
            case UVC_PARAM_FUNC_FOCUSABS:
                if (g_uvc_ops->SetFocusAbs)
                    g_uvc_ops->SetFocusAbs(uvc_control_ct[UVC_CT_FOCUS_ABSOLUTE_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_ZOOMABS:
                if (g_uvc_ops->SetZoomAbs)
                    g_uvc_ops->SetZoomAbs(uvc_control_ct[UVC_CT_ZOOM_ABSOLUTE_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_PANTILTABS:
                if (g_uvc_ops->SetPanTiltAbs)
                    g_uvc_ops->SetPanTiltAbs(uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.cur && 0xff,
                                            (uvc_control_ct[UVC_CT_PANTILT_ABSOLUTE_CONTROL].val.cur >> 8) && 0xff);
                break;
            case UVC_PARAM_FUNC_ROLLABS:
                if (g_uvc_ops->SetRollAbs)
                    g_uvc_ops->SetRollAbs(uvc_control_ct[UVC_CT_ROLL_ABSOLUTE_CONTROL].val.cur);
                break;
            case UVC_PARAM_FUNC_EXTERN_DATA:
                if (g_uvc_ops->uvc_extern_data)
                    g_uvc_ops->uvc_extern_data((rt_uint8_t *)uvc_event.parm1,
                                                uvc_event.parm2,
                                                uvc_event.parm3);
                break;
            case UVC_PARAM_FUNC_STREAMOFF:
                if (g_uvc_ops->stream_off_callback)
                    g_uvc_ops->stream_off_callback(uvc_event.parm1);
                break;
            case UVC_PARAM_FUNC_STREAMON:
                if (g_uvc_ops->stream_on_callback)
                    g_uvc_ops->stream_on_callback(uvc_event.parm1);
                break;
            }
        } else
            rt_thread_mdelay(1);
    }
}

int fh_uvc_status_get(int stream_id)
{
    if (stream_id == STREAM_ID1)
        return gVideoBuf[stream_id].play_status;
#ifdef UVC_DOUBLE_STREAM
    else if (stream_id == STREAM_ID2)
        return gVideoBuf[stream_id].play_status;
#endif
    else
        rt_kprintf("%s stream_id error: %d\n", __func__, stream_id);
    return -1;
}

void fh_uvc_status_set(int stream_id, int set)
{
    struct uvc_ctrl_event uvc_event;
    int result;

    gVideoBuf[stream_id].play_status = set;
    if (g_uvc_ops && g_uvc_ops->stream_off_callback)
    {
        if (set == UVC_STREAM_OFF)
        {
            uvc_event.func_num = UVC_PARAM_FUNC_STREAMOFF;
            uvc_event.parm1 = stream_id;
            result = rt_mq_send(&g_uvc_event_mq, (void *)&uvc_event, sizeof(uvc_event));
            if (result == -RT_EFULL)
            {
                rt_kprintf("uvc message send failed\n");
            }
        }
    }
}

void fh_uvc_ops_register(struct uvc_stream_ops *ops)
{
    g_uvc_ops = ops;
}

void uvc_driver_buf_init(int stream_id)
{
    int i;
    struct uvc_video_buf *buf;
    rt_base_t level;

    if (stream_id == STREAM_ID1)
        buf = &gVideoBuf[stream_id];
#ifdef UVC_DOUBLE_STREAM
    else if (stream_id == STREAM_ID2)
        buf = &gVideoBuf[stream_id];
#endif
    else
        rt_kprintf("%s stream_id error: %d\n", __func__, stream_id);

    rt_memset(buf, 0, sizeof(struct uvc_video_buf));
    if (stream_id == STREAM_ID1)
        buf->uvc_next_sem = rt_sem_create("uvc_next_sem", UVC_BUF_NUM, RT_IPC_FLAG_FIFO);
#ifdef UVC_DOUBLE_STREAM
    else
        buf->uvc_next_sem = rt_sem_create("uvc_next_sem1", UVC_BUF_NUM, RT_IPC_FLAG_FIFO);
#endif
    level = rt_hw_interrupt_disable();
    rt_list_init(&buf->list_free);
    rt_list_init(&buf->list_used);

    for (i = 0; i < UVC_BUF_NUM; i++)
    {
        buf->data_buf[i].id = i;
        buf->data_buf[i].data = rt_malloc_align(RT_ALIGN(UVC_BUF_SIZE, CACHE_LINE_SIZE), CACHE_LINE_SIZE);
        if (buf->data_buf[i].data == NULL)
        {
            rt_kprintf("uvc buf out of memory!\n");
            return;
        }
        /* rt_memset(buf->data_buf[i].data, 0, UVC_BUF_SIZE); */
        rt_list_insert_after(&buf->list_free, &buf->data_buf[i].list);
    }
    buf->cur_buf = RT_NULL;
    rt_hw_interrupt_enable(level);
    buf->fid = 0;
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    if (stream_id == STREAM_ID1)
        buf->packet_size = UVC_PACKET_SIZE * ISOC_CNT_ON_MICROFRAME;
    else
        buf->packet_size = UVC_PACKET_SIZE;
#else
    buf->packet_size = UVC_BUF_SIZE;
#endif

}

void uvc_driver_buf_reinit(int stream_id, int alt_size)
{
    struct uvc_video_buf *buf = NULL;
    int i;
    rt_base_t level;

    if (stream_id == STREAM_ID1)
        buf = &gVideoBuf[stream_id];
#ifdef UVC_DOUBLE_STREAM
    else if (stream_id == STREAM_ID2)
        buf = &gVideoBuf[stream_id];
#endif
    else
        rt_kprintf("%s stream_id error: %d\n", __func__, stream_id);

    level = rt_hw_interrupt_disable();
    rt_list_init(&buf->list_free);
    rt_list_init(&buf->list_used);

    for (i = 0; i < UVC_BUF_NUM; i++)
    {
        buf->data_buf[i].id = i;
        rt_list_insert_after(&buf->list_free, &buf->data_buf[i].list);
    }
    buf->uvc_next_sem->value = UVC_BUF_NUM;
    buf->cur_buf = RT_NULL;
    rt_hw_interrupt_enable(level);

    buf->fid = 0;
#ifdef FH_RT_UVC_EP_TYPE_ISOC
    if (stream_id == STREAM_ID1)
        buf->packet_size = alt_size;
    else
        buf->packet_size = UVC_PACKET_SIZE;
#else
    buf->packet_size = UVC_BUF_SIZE;
#endif
}

uint32_t uvc_frame_encode(int stream_id, struct uvc_frame_buf *frame_buf,
    uint32_t frame_packet_size,
    uint32_t fid,
    unsigned char *img_data,
    uint32_t img_size)
{
    struct uvc_video_buf *buf = &gVideoBuf[stream_id];
    uint32_t j, copy_count, packets_in_frame, last_packet_size = 0, picture_pos;
    uint32_t data_packet_size;
    uint32_t frame_size = 0;
    uint32_t payload_data_size = 0;
    uint32_t buf_size = 0;
    uint32_t last_buf = 0;
    uint32_t remainder = 0;
    uint32_t uvc_header_size = 0;
    uint8_t *pBuf = frame_buf->data;
    unsigned char *img_poi = img_data;
    static uint8_t remainder_warning = 0;
    img_poi = img_data + buf->current_size;

    buf_size = img_size - buf->current_size;

#ifdef UVC_SUPPORT_WINDOWS_HELLO_FACE
    if (stream_id == STREAM_ID2)
        uvc_header_size = UVC_HEADER_LEN + UVC_HELLO_FACE_HEADER_LEN;
    else
        uvc_header_size = UVC_HEADER_LEN;
#else
    uvc_header_size = UVC_HEADER_LEN;
#endif
    data_packet_size = frame_packet_size - uvc_header_size;

    if (UVC_BUF_SIZE % frame_packet_size != 0 && !remainder_warning)
    {
        remainder_warning = 1;
        rt_kprintf("WARNING[UVC]:%s-%d UVC_BUF_SIZE(%d) % frame_packet_size(%d) != 0\n",
            __func__, __LINE__, UVC_BUF_SIZE, frame_packet_size);
    }
    remainder = UVC_BUF_SIZE % frame_packet_size;
    payload_data_size = (UVC_BUF_SIZE - UVC_BUF_SIZE/frame_packet_size * uvc_header_size) - remainder;

    if (buf_size > payload_data_size)
    {
        packets_in_frame = payload_data_size / data_packet_size;
        frame_size = UVC_BUF_SIZE - remainder;
        buf->current_size += payload_data_size;
        last_buf = 0;
    } else
    {
        last_buf = 1;
        packets_in_frame = (buf_size + data_packet_size - 1) / data_packet_size;
        last_packet_size = (buf_size - ((packets_in_frame - 1) * data_packet_size) + uvc_header_size);
        frame_size = (packets_in_frame - 1)*frame_packet_size + last_packet_size;
        buf->current_size += buf_size;
    }

    picture_pos = 0;

    for (j = 0; j < packets_in_frame; j++)
    {
        pBuf[0] = uvc_header_size;
        pBuf[1] = /* UVC_STREAM_EOH |  */fid;

        if (buf->stream_pts && uvc_header_size >= 12)
        {
            pBuf[1] |= UVC_STREAM_PTS;
            pBuf[1] |= UVC_STREAM_SCR;
            pBuf[2] = (uint8_t)(buf->stream_pts & 0xff);
            pBuf[3] = (uint8_t)(buf->stream_pts >> 8 & 0xff);
            pBuf[4] = (uint8_t)(buf->stream_pts >> 16 & 0xff);
            pBuf[5] = (uint8_t)(buf->stream_pts >> 24 & 0xff);
        }
#ifdef UVC_SUPPORT_WINDOWS_HELLO_FACE
        if (uvc_header_size == (UVC_HEADER_LEN + UVC_HELLO_FACE_HEADER_LEN))
        {
            rt_memset(&pBuf[UVC_HEADER_LEN], 0, UVC_HELLO_FACE_HEADER_LEN);
            pBuf[UVC_HEADER_LEN] = 0x06;
            pBuf[UVC_HEADER_LEN + 4] = 0x10;
            if (fid)
                pBuf[UVC_HEADER_LEN + 8] = 0x01;
        }
#endif

        if (j == packets_in_frame - 1 && last_buf)
        {
            copy_count = last_packet_size;
            pBuf[1] |= UVC_STREAM_EOF | UVC_STREAM_RES;
            buf->stream_pts = 0;
        }
        else
        {
            copy_count = frame_packet_size;
        }
        if (stream_info[stream_id]->fcc == V4L2_PIX_FMT_NV12 ||
                    stream_info[stream_id]->fcc == V4L2_PIX_FMT_YUY2)
            rt_memcpy(pBuf + uvc_header_size, img_poi + picture_pos, copy_count - uvc_header_size);
        else
            fh_dma_memcpy(pBuf + uvc_header_size, img_poi + picture_pos, copy_count - uvc_header_size);

        pBuf += copy_count;
        picture_pos += (copy_count - uvc_header_size);

    }

    return frame_size;
}

static void uvc_header_send(struct udevice *device, struct uendpoint *ep)
{
    struct uvc_video_buf *buf = NULL;
    struct uio_request *req;

    buf = (struct uvc_video_buf *)ep->driver_data;
    req = &ep->request;

    buf->header_buf[0] = UVC_HEADER_LEN;
    buf->header_buf[1] |= /* UVC_STREAM_EOH |  */buf->uvc_fid_last;

    req->buffer = buf->header_buf;
    req->size = UVC_HEADER_LEN;
    req->req_type   = UIO_REQUEST_WRITE;
#ifdef UVC_SUPPORT_WINDOWS_HELLO_FACE
    if (buf == &gVideoBuf[STREAM_ID2])
    {
        rt_memset(&buf->header_buf[UVC_HEADER_LEN], 0, UVC_HELLO_FACE_HEADER_LEN);
        buf->header_buf[0] = UVC_HEADER_LEN + UVC_HELLO_FACE_HEADER_LEN;
        buf->header_buf[UVC_HEADER_LEN] = 0x06;
        buf->header_buf[UVC_HEADER_LEN + 4] = 0x10;
        req->size = UVC_HEADER_LEN + UVC_HELLO_FACE_HEADER_LEN;
        if (buf->uvc_fid_last)
                buf->header_buf[UVC_HEADER_LEN + 8] = 0x01;
    }
#endif
    rt_usbd_io_request(device, ep, req);
}

static void uvc_last_header(struct uvc_video_buf *buf, unsigned char *last_buf)
{
    buf->uvc_fid_last = last_buf[1] & UVC_STREAM_FID;
    // buf->uvc_fid_last ^= UVC_STREAM_FID;
    buf->header_buf[1] = 0;
    if (last_buf[1] & UVC_STREAM_SCR)
    {
        buf->header_buf[1] |= UVC_STREAM_PTS;
        buf->header_buf[1] |= UVC_STREAM_SCR;
        buf->header_buf[2] = last_buf[2];
        buf->header_buf[3] = last_buf[3];
        buf->header_buf[4] = last_buf[4];
        buf->header_buf[5] = last_buf[5];

    } else
    {
        buf->header_buf[1] &= ~UVC_STREAM_PTS;
        buf->header_buf[1] &= ~UVC_STREAM_SCR;
    }
}

void uvc_video_complete(struct ufunction *func, struct uendpoint *ep, rt_size_t size)
{
    struct uio_request *req;
    struct uvc_video_buf *buf;

    req = &ep->request;
    buf = (struct uvc_video_buf *)ep->driver_data;

    if (buf->cur_buf != RT_NULL)
    {
#ifdef FH_RT_UVC_EP_TYPE_BULK
        if (size%UVC_PACKET_SIZE == 0 && size)
        {
            req->buffer = buf->cur_buf->data;
            req->size = 0;
            req->req_type   = UIO_REQUEST_WRITE;
            rt_usbd_io_request(func->device, ep, req);
            return;
        }
#endif
        uvc_last_header(buf, buf->cur_buf->data);
        rt_list_insert_before(&buf->list_free, &buf->cur_buf->list);

        rt_sem_release(buf->uvc_next_sem);
        buf->cur_buf = RT_NULL;
        if (rt_list_isempty(&buf->list_used))
        {
            uvc_header_send(func->device, ep);
            return;
        }
        buf->cur_buf = rt_list_first_entry(&buf->list_used, struct uvc_frame_buf, list);
        if (buf->cur_buf != NULL)
        {
            rt_list_remove(&buf->cur_buf->list);
            req->buffer = buf->cur_buf->data;
            req->size = buf->cur_buf->uvc_frame_size;
            req->req_type   = UIO_REQUEST_WRITE;
            rt_usbd_io_request(func->device, ep, req);
        }
    } else
    {
        if (rt_list_isempty(&buf->list_used))
        {
            uvc_header_send(func->device, ep);
        }
        else
        {
            buf->cur_buf = rt_list_first_entry(&buf->list_used, struct uvc_frame_buf, list);
            if (buf->cur_buf != NULL)
            {
                rt_list_remove(&buf->cur_buf->list);
                req->buffer = buf->cur_buf->data;
                req->size = buf->cur_buf->uvc_frame_size;
                req->req_type   = UIO_REQUEST_WRITE;
                rt_usbd_io_request(func->device, ep, req);
            }
        }
    }

}

int uvc_video_pump(int stream_id)
{
    struct uendpoint *ep = NULL;
    struct uio_request *req;
    struct uvc_video_buf *buf;
    rt_base_t level;

    buf = &gVideoBuf[stream_id];
    if (stream_id == STREAM_ID1)
        ep = video_stream1_ep1;
#ifdef UVC_DOUBLE_STREAM
    else
        ep = video_stream2_ep1;
#endif
    ep->driver_data = (void *)buf;

    level = rt_hw_interrupt_disable();
    if (buf->cur_buf == RT_NULL)
    {
        if (rt_list_len(&buf->list_used))
        {
            buf->cur_buf = rt_list_first_entry(&buf->list_used, struct uvc_frame_buf, list);
            if (buf->cur_buf != NULL)
            {
                rt_list_remove(&buf->cur_buf->list);
                req = &ep->request;
                req->buffer = buf->cur_buf->data;
                req->size = buf->cur_buf->uvc_frame_size;
                req->req_type   = UIO_REQUEST_WRITE;
				rt_hw_interrupt_enable(level);
                rt_usbd_io_request(g_uvcdevice, ep, req);
            } else
                rt_hw_interrupt_enable(level);
        } else
            rt_hw_interrupt_enable(level);
    } else
        rt_hw_interrupt_enable(level);

    return 0;
}
void fh_uvc_stream_pts(int stream_id, uint64_t pts)
{
    struct uvc_video_buf *buf = NULL;

    if (stream_id == STREAM_ID1)
        buf = &gVideoBuf[stream_id];
#ifdef UVC_DOUBLE_STREAM
    else if (stream_id == STREAM_ID2)
        buf = &gVideoBuf[stream_id];
#endif
    else
        rt_kprintf("%s stream_id error: %d\n", __func__, stream_id);

    buf->stream_pts = (uint32_t)(pts & 0xffffffff);
}

void fh_uvc_stream_enqueue(int stream_id, uint8_t *img_data, uint32_t img_size)
{
    struct uvc_frame_buf *frame_buf = RT_NULL;
    struct uvc_video_buf *buf = NULL;
    rt_base_t level;
    int ret;

    if (img_size == 0)
        return;

    if (stream_id == STREAM_ID1)
        buf = &gVideoBuf[stream_id];
#ifdef UVC_DOUBLE_STREAM
    else if (stream_id == STREAM_ID2)
        buf = &gVideoBuf[stream_id];
#endif
    else
        rt_kprintf("%s stream_id error: %d\n", __func__, stream_id);

    buf->current_size = 0;

    while (buf->current_size < img_size && fh_uvc_status_get(stream_id))
    {
        ret = rt_sem_take(buf->uvc_next_sem, RT_TICK_PER_SECOND);
        if (ret != RT_EOK)
        {
            break;
        }
        if (rt_list_len(&buf->list_free) && fh_uvc_status_get(stream_id))
        {
            level = rt_hw_interrupt_disable();
            frame_buf = NULL;
            frame_buf = rt_list_first_entry(&buf->list_free, struct uvc_frame_buf, list);
            if (frame_buf != NULL)
            {
                rt_list_remove(&frame_buf->list);
                rt_hw_interrupt_enable(level);
                frame_buf->uvc_frame_size = uvc_frame_encode(stream_id,
                                            frame_buf,
                                            buf->packet_size,
                                            buf->fid,
                                            img_data,
                                            img_size);
                level = rt_hw_interrupt_disable();
                rt_list_insert_before(&buf->list_used, &frame_buf->list);
                rt_hw_interrupt_enable(level);
            } else
                rt_hw_interrupt_enable(level);
        }
    }
    buf->fid ^= UVC_STREAM_FID;

}

void video_driver_proc(void *arg)
{
    int stream_id = (int)arg;
    struct uendpoint *ep = NULL;
    struct uvc_video_buf *buf;

    buf = &gVideoBuf[stream_id];
    buf->header_buf = rt_malloc_align(RT_ALIGN(UVC_HEADER_LEN + UVC_HELLO_FACE_HEADER_LEN, CACHE_LINE_SIZE), CACHE_LINE_SIZE);
    rt_memset(buf->header_buf, 0, RT_ALIGN(UVC_HEADER_LEN + UVC_HELLO_FACE_HEADER_LEN, CACHE_LINE_SIZE));
    while (1)
    {
        if (rt_sem_take(stream_info[stream_id]->uvc_format_sem, RT_WAITING_FOREVER) == RT_EOK)
        {
            if (g_uvc_ops && g_uvc_ops->format_change)
            {
                if (stream_id == STREAM_ID1)
                        ep = video_stream1_ep1;
                #ifdef UVC_DOUBLE_STREAM
                    else
                        ep = video_stream2_ep1;
                #endif
                    ep->driver_data = (void *)buf;
                uvc_header_send(g_uvcdevice, ep);

                if (g_uvc_ops->stream_on_callback)
                    g_uvc_ops->stream_on_callback(stream_id);
                g_uvc_ops->format_change(stream_id,
                                        stream_info[stream_id]->fcc,
                                        stream_info[stream_id]->width,
                                        stream_info[stream_id]->height,
                                        10000000/stream_info[stream_id]->finterval);

                uvc_para_init();

                fh_uvc_status_set(stream_id, UVC_STREAM_ON);
            }
        }
    }
}

void uvc_sem_release(int stream_id)
{
    rt_sem_release(stream_info[stream_id]->uvc_format_sem);
}

void fh_uvc_init(int stream_id, struct uvc_format_info *fmt, int fmt_num)
{
    uvc_driver_buf_init(stream_id);
    fh_uvc_fmt_init(stream_id, fmt, fmt_num);

    if (!stream_info[stream_id])
    {
        stream_info[stream_id] = rt_calloc(sizeof(struct uvc_stream_info), 1);
        if (!stream_info[stream_id])
            rt_kprintf("%s rt_calloc error!\n", __func__);
        stream_info[stream_id]->stream_id = stream_id;
    }
    uvc_fill_streaming_control(stream_info[stream_id], &stream_info[stream_id]->probe,  0, 0);
    uvc_fill_streaming_control(stream_info[stream_id], &stream_info[stream_id]->commit, 0, 0);

    if (stream_id == STREAM_ID1)
    {

        stream_info[stream_id]->uvc_format_sem = rt_sem_create("uvc_format_sem", 0, RT_IPC_FLAG_FIFO);

        rt_mq_init(&g_uvc_event_mq, "uvc_ctrl_mq", uvc_mq_pool, sizeof(struct uvc_ctrl_event),
                        sizeof(uvc_mq_pool), RT_IPC_FLAG_FIFO);

        g_uvc_driver_thread[stream_id] = rt_thread_create("uvc_driver_proc", video_driver_proc,
                    (void *)stream_id, 8 * 1024, 29, 1);
        rt_thread_startup(g_uvc_driver_thread[stream_id]);

        g_thread_uvc_event = rt_thread_create("uvc_control_event", uvc_event_proc,
                   NULL, 4 * 1024, 8, 1);
        rt_thread_startup(g_thread_uvc_event);
        uvc_uspara_load();
    }
#ifdef UVC_DOUBLE_STREAM
    if (stream_id == STREAM_ID2)
    {
        stream_info[stream_id]->uvc_format_sem = rt_sem_create("uvc_format_sem1", 0, RT_IPC_FLAG_FIFO);
        g_uvc_driver_thread[stream_id] = rt_thread_create("uvc_driver_proc1", video_driver_proc,
                (void *)stream_id, 8 * 1024, 29, 1);
        rt_thread_startup(g_uvc_driver_thread[stream_id]);
    }
#endif

#ifndef UVC_DOUBLE_STREAM
    rt_usb_device_init();
#else
    if (stream_id == STREAM_ID2)
        rt_usb_device_init();
#endif
}
