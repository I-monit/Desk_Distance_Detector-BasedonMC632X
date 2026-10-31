#include <rtthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <rthw.h>
#include "../core/usbdevice.h"
#include "audio.h"
#include "uac_init.h"
#include "mmu.h"

#define UAC_MICALIGN_SIZE 256
static struct uac_audio_buf gAudioBuf;
static unsigned int g_audio_init = 0;
extern struct uac_audio *g_audio;

static rt_thread_t thread_uac_callback;

extern struct rt_messagequeue uac_event_mq;

static struct uac_stream_ops *g_uac_ops = NULL;

void fh_uac_ops_register(struct uac_stream_ops *ops)
{
    g_uac_ops = ops;
}

void uac_event_proc(void *arg)
{
    struct uac_ctrl_event event;

    while (1)
    {
        if (g_uac_ops && rt_mq_recv(&uac_event_mq, &event, sizeof(struct uac_ctrl_event), RT_WAITING_FOREVER) == RT_EOK)
        {
            switch (event.func_num)
            {
            case UAC_CALLBACK_MIC_SAMPLE:
                if (g_uac_ops->mic_sample)
                {
                    fh_uac_micsample_change(0);
                    g_uac_ops->mic_sample(event.parm1);
                }
                break;
            case UAC_CALLBACK_SPK_SAMPLE:
                if (g_uac_ops->spk_sample)
                    g_uac_ops->spk_sample(event.parm1);
                break;
            case UAC_CALLBACK_MIC_MUTE:
                if (g_uac_ops->mic_mute)
                    g_uac_ops->mic_mute(event.parm1);
                break;
            case UAC_CALLBACK_SPK_MUTE:
                if (g_uac_ops->spk_mute)
                    g_uac_ops->spk_mute(event.parm1);
                break;
            case UAC_CALLBACK_MIC_VOLUME:
                if (g_uac_ops->mic_volume)
                    g_uac_ops->mic_volume(event.parm1);
                break;
            case UAC_CALLBACK_SPK_VOLUME:
                if (g_uac_ops->spk_volume)
                    g_uac_ops->spk_volume(event.parm1);
                break;
            }
        } else
            rt_thread_mdelay(1);
    }
}

void fh_uac_micsample_change(int set)
{
    gAudioBuf.sample = set;
}

int fh_uac_micstatus_get(void)
{
    return gAudioBuf.play_status;
}

void fh_uac_micstatus_set(int set)
{
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    gAudioBuf.play_status = set;
    rt_hw_interrupt_enable(level);
}

void fh_uac_init(void)
{
    int i;
    struct uac_audio_buf *buf = &gAudioBuf;

    if (g_audio_init)
        return;

    rt_list_init(&buf->list_free);
    rt_list_init(&buf->list_used);

    for (i = 0; i < UAC_MICBUF_NUM; i++)
    {
        buf->data_buf[i].data = rt_malloc_align(RT_ALIGN(UAC_MICBUF_SIZE + UAC_MICALIGN_SIZE, CACHE_LINE_SIZE), CACHE_LINE_SIZE);
        rt_memset(buf->data_buf[i].data, 0, RT_ALIGN(UAC_MICBUF_SIZE + UAC_MICALIGN_SIZE, CACHE_LINE_SIZE));
        rt_list_insert_after(&buf->list_free, &buf->data_buf[i].list);
    }

    buf->cur_buf = RT_NULL;

#ifdef ENABLE_SPEAKER_DESC
    void fh_uac_spkbuf_init(void);
    fh_uac_spkbuf_init();
#endif

    rt_usb_device_init();

    thread_uac_callback = rt_thread_create("uac_callback_setup", uac_event_proc,
                   NULL, 4 * 1024, 8, 1);
    rt_thread_startup(thread_uac_callback);

    g_audio_init = 1;
}

void fh_uac_micbuf_reinit(void)
{
    int i;
    struct uac_audio_buf *buf = &gAudioBuf;

    if (g_audio_init)
    {
        rt_list_init(&buf->list_free);
        rt_list_init(&buf->list_used);

        for (i = 0; i < UAC_MICBUF_NUM; i++)
        {
            buf->data_buf[i].uac_frame_size = 0;
            rt_list_insert_after(&buf->list_free, &buf->data_buf[i].list);
        }
        buf->cur_buf = RT_NULL;
    }
}

void uac_audio_complete(struct ufunction *func, struct uendpoint *ep)
{
    struct uio_request *req;
    struct uac_audio_buf *buf;
    rt_base_t level;

    req = &ep->request;
    buf = &gAudioBuf;
    if (buf->cur_buf != RT_NULL)
    {
        level = rt_hw_interrupt_disable();
        rt_list_insert_before(&buf->list_free, &buf->cur_buf->list);
        buf->cur_buf = RT_NULL;

        if (rt_list_isempty(&buf->list_used))
        {
            rt_hw_interrupt_enable(level);
            return;
        }

        buf->cur_buf = rt_list_first_entry(&buf->list_used, struct frame_buf, list);
        if (buf->cur_buf != NULL)
        {
            rt_list_remove(&buf->cur_buf->list);
            rt_hw_interrupt_enable(level);
            req->buffer = buf->cur_buf->data;
            req->size = buf->cur_buf->uac_frame_size;
            req->req_type = UIO_REQUEST_WRITE;
            rt_usbd_io_request(func->device, ep, req);
        }
        else
            rt_hw_interrupt_enable(level);
    }
}

static int uac_audio_pump(struct uac_audio_buf *buf)
{
    struct uendpoint *ep = g_audio->ep;
    struct uio_request *req;
    rt_base_t level;

    if (buf->cur_buf == RT_NULL)
    {
        level = rt_hw_interrupt_disable();
        if (rt_list_len(&buf->list_used) > 2)
        {
            buf->cur_buf = rt_list_first_entry(&buf->list_used, struct frame_buf, list);
            if (buf->cur_buf != NULL)
            {
                rt_list_remove(&buf->cur_buf->list);
                rt_hw_interrupt_enable(level);
                req = &ep->request;
                req->buffer = buf->cur_buf->data;
                req->size = buf->cur_buf->uac_frame_size;
                req->req_type = UIO_REQUEST_WRITE;
                rt_usbd_io_request(g_audio->func->device, ep, req);
            }
            else
                rt_hw_interrupt_enable(level);

        }
    }
    return 0;
}

void fh_uac_micstream_enqueue(uint8_t *audio_data, uint32_t audio_size)
{
    struct frame_buf *frame_buf = RT_NULL;
    struct uac_audio_buf *buf = &gAudioBuf;
    static char align_buf[UAC_MICALIGN_SIZE];
    static int align_len = 0;
    int temp_len = 0;
    int status;
    rt_base_t level;

    if (audio_size > UAC_MICBUF_SIZE)
    {
        rt_kprintf("[ERROR]%s-%d audio_size %d > UAC_MICBUF_SIZE %d\n", __func__, __LINE__, audio_size, UAC_MICBUF_SIZE);
        return;
    }
    level = rt_hw_interrupt_disable();
    if (rt_list_len(&buf->list_free) == 0)
    {
        if (rt_list_len(&buf->list_used))
        {
            frame_buf = rt_list_first_entry(&buf->list_used, struct frame_buf, list);
            rt_list_remove(&frame_buf->list);
            rt_list_insert_before(&buf->list_free, &frame_buf->list);
        }
    }

    if (rt_list_len(&buf->list_free))
    {
        frame_buf = rt_list_first_entry(&buf->list_free, struct frame_buf, list);
        rt_list_remove(&frame_buf->list);
        rt_hw_interrupt_enable(level);
        if (align_len)
            rt_memcpy(frame_buf->data, align_buf,
                align_len > UAC_MICALIGN_SIZE ? UAC_MICALIGN_SIZE : align_len);
        temp_len = align_len;

        align_len = (audio_size + align_len) % EP_MAXPACKET(g_audio->ep);
        if (align_len > UAC_MICALIGN_SIZE)
            rt_kprintf("UAC/mic %s ep maxpacket size(%d) > %d!\n",
                                    __func__, EP_MAXPACKET(g_audio->ep), UAC_MICALIGN_SIZE);
        rt_memcpy(frame_buf->data + temp_len, audio_data, audio_size - align_len);

        if (align_len)
            rt_memcpy(align_buf, audio_data + audio_size - align_len, align_len);

        frame_buf->uac_frame_size = audio_size + temp_len - align_len;

        level = rt_hw_interrupt_disable();
        rt_list_insert_before(&buf->list_used, &frame_buf->list);
        rt_hw_interrupt_enable(level);
        status = fh_uac_micstatus_get();
        if (status == UAC_STREAM_ON)
            uac_audio_pump(buf);
        else
            fh_uac_micbuf_reinit();
    }

    level = rt_hw_interrupt_disable();
    if (buf->sample)
    {
        int change = buf->sample;

        fh_uac_micsample_change(0);
        rt_hw_interrupt_enable(level);
        if (g_uac_ops->mic_sample)
            g_uac_ops->mic_sample(change);
    } else
        rt_hw_interrupt_enable(level);

}

#ifdef ENABLE_SPEAKER_DESC
static struct uac_speaker_buf gSpeakerBuf;
unsigned char test_buf[256];

int fh_uac_spkstatus_get(void)
{
    return gSpeakerBuf.play_status;
}

void fh_uac_spkstatus_set(int set)
{
    gSpeakerBuf.play_status = set;
}

void fh_uac_spkbuf_init(void)
{
    int i;
    struct uac_speaker_buf *buf = &gSpeakerBuf;

    /* 解决先播放声音再上电导致第一次播放没有声音的问题 */
    i = buf->play_status;
    rt_memset(buf, 0, sizeof(gSpeakerBuf));
    buf->play_status = i;

    rt_list_init(&buf->list_free);
    rt_list_init(&buf->list_used);
    for (i = 0; i < UAC_SPKBUF_NUM; i++)
    {
        buf->data_buf[i].data = rt_malloc_align(RT_ALIGN(UAC_SPKBUF_SIZE, CACHE_LINE_SIZE), CACHE_LINE_SIZE);//(UAC_SPKBUF_SIZE, 1);
        rt_memset(buf->data_buf[i].data, 0, RT_ALIGN(UAC_SPKBUF_SIZE, CACHE_LINE_SIZE));
        buf->data_buf[i].uac_frame_size = 0;
        rt_list_insert_after(&buf->list_free, &buf->data_buf[i].list);
    }

    buf->cur_buf = RT_NULL;
}

void fh_uac_spkbuf_reinit(void)
{
    int i;
    struct uac_speaker_buf *buf = &gSpeakerBuf;

    rt_list_init(&buf->list_free);
    rt_list_init(&buf->list_used);
    for (i = 0; i < UAC_SPKBUF_NUM; i++)
    {
        buf->data_buf[i].uac_frame_size = 0;
        rt_list_insert_after(&buf->list_free, &buf->data_buf[i].list);
    }
    buf->cur_buf = RT_NULL;

    fh_uac_spkstatus_set(UAC_STREAM_OFF);
}

int uac_speaker_pump(udevice_t device, uep_t ep)
{
    struct uio_request *req;
    struct uac_speaker_buf *buf = &gSpeakerBuf;
    rt_base_t level;

    if (g_audio_init == 0)
    {
        req = &ep->request;
        req->buffer = test_buf;
        req->size = UAC_SPKBUF_SIZE;
        req->req_type = UIO_REQUEST_READ_BEST;
        rt_usbd_io_request(device, ep, req);
        return 0;
    }
    level = rt_hw_interrupt_disable();
    if (buf->cur_buf == RT_NULL)
    {
        if (rt_list_len(&buf->list_free))
        {
            buf->cur_buf = rt_list_first_entry(&buf->list_free, struct frame_buf, list);
            if (buf->cur_buf != NULL)
            {
                rt_list_remove(&buf->cur_buf->list);

                req = &ep->request;
                req->buffer = buf->cur_buf->data;
                req->size = UAC_SPKBUF_SIZE;
                req->req_type = UIO_REQUEST_READ_BEST;
                rt_hw_interrupt_enable(level);
                rt_usbd_io_request(device, ep, req);
            }
            else
                rt_hw_interrupt_enable(level);

        }
    } else
    {
        rt_hw_interrupt_enable(level);
        rt_kprintf("%s--%d error\n", __func__, __LINE__);
    }
    return 0;
}

void uac_speaker_complete(struct ufunction *func, struct uendpoint *ep, int size)
{
    struct uio_request *req;
    struct uac_speaker_buf *buf;
    static int last_packet_size = 0;
    int sample = 0;
    struct uac_ctrl_event event;
    int result;

    req = &ep->request;
    buf = &gSpeakerBuf;

    if (g_audio_init == 0)
    {
        req = &ep->request;
        req->buffer = test_buf;
        req->size = UAC_SPKBUF_SIZE;
        req->req_type = UIO_REQUEST_READ_BEST;
        rt_usbd_io_request(func->device, ep, req);
        return;
    }
    if (size && size != last_packet_size)
    {
        last_packet_size = size;
        /* change speaker sample rate */
        switch (size)
        {
        case 16 * AUDIO_CHN_NUM_SPK:
            sample = 8000;
            break;
        case 32 * AUDIO_CHN_NUM_SPK:
            sample = 16000;
            break;
        case 44 * AUDIO_CHN_NUM_SPK:
        case 46 * AUDIO_CHN_NUM_SPK:
            sample = 22050;
            break;
        case 64 * AUDIO_CHN_NUM_SPK:
            sample = 32000;
            break;
        case 88 * AUDIO_CHN_NUM_SPK:
        case 90 * AUDIO_CHN_NUM_SPK:
            sample = 44100;
            break;
        case 96 * AUDIO_CHN_NUM_SPK:
            sample = 48000;
            break;
        default:
            rt_kprintf("%s-%d default sample size %d\n", __func__, __LINE__, size);
            break;
        }
        if (g_audio->spk_sample != sample)
        {
            g_audio->spk_sample = sample;
            /* fh_uac_spkstatus_set(sample); */
            event.func_num = UAC_CALLBACK_SPK_SAMPLE;
            event.parm1 = sample;
            result = rt_mq_send(&uac_event_mq, (void *)&event, sizeof(event));
            if (result == -RT_EFULL)
            {
                rt_kprintf("uac callback message send failed\n");
            }
        }
    }

    if (buf->cur_buf != RT_NULL)
    {
        if (rt_list_isempty(&buf->list_free))
        {
            // rt_kprintf("%d\n", rt_list_len(&buf->list_used));
            /* rt_list_remove(&buf->cur_buf->list); */
            req->buffer = buf->cur_buf->data;
            req->size = UAC_SPKBUF_SIZE;
            req->req_type = UIO_REQUEST_READ_BEST;
            rt_usbd_io_request(func->device, ep, req);
            return;
        }

        if (size)
        {
            buf->cur_buf->uac_frame_size = size;
            rt_list_insert_before(&buf->list_used, &buf->cur_buf->list);
        } else
        {
            rt_list_insert_before(&buf->list_free, &buf->cur_buf->list);
        }
        buf->cur_buf = RT_NULL;

        buf->cur_buf = rt_list_first_entry(&buf->list_free, struct frame_buf, list);
        if (buf->cur_buf != NULL)
        {
            rt_list_remove(&buf->cur_buf->list);
            req->buffer = buf->cur_buf->data;
            req->size = UAC_SPKBUF_SIZE;
            req->req_type = UIO_REQUEST_READ_BEST;
            rt_usbd_io_request(func->device, ep, req);
        } else
            rt_kprintf("%s-%d buf->cur_buf != NULL\n", __func__, __LINE__);
    } else
    {
        uac_speaker_pump(func->device, ep);
    }
}

struct uac_frame_buf *fh_uac_spkstream_dequeue(void)
{
    struct uac_speaker_buf *buf;
    struct frame_buf *spk_buf;
    rt_base_t level;

    if (!g_audio_init)
        return NULL;

    buf = &gSpeakerBuf;
    level = rt_hw_interrupt_disable();
    if (rt_list_isempty(&buf->list_used))
    {
        rt_hw_interrupt_enable(level);
        return NULL;
    }

    spk_buf = rt_list_first_entry(&buf->list_used, struct frame_buf, list);
    if (spk_buf != NULL)
    {
        rt_list_remove(&spk_buf->list);
        rt_hw_interrupt_enable(level);
        return (struct uac_frame_buf *)spk_buf;
    }
    else
    {
        rt_hw_interrupt_enable(level);
        return NULL;
    }
}

void fh_uac_spkstream_enqueue(struct uac_frame_buf *spk_buf)
{
    struct uac_speaker_buf *buf;
    rt_base_t level;

    if (!g_audio_init)
        return;

    buf = &gSpeakerBuf;
    level = rt_hw_interrupt_disable();
    rt_list_insert_before(&buf->list_free, &((struct frame_buf *)spk_buf)->list);
    rt_hw_interrupt_enable(level);
}

#endif
