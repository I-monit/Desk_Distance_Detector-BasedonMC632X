#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <pthread.h>
#include "config.h"
#include "libdmc/include/libdmc.h"
#include "libdmc/include/libdmc_record_raw.h"

/********************************************
 ************cbuf impelement begin...********
 ********************************************/
#define __USING_WRITE_BUFFER
#ifndef FH_APP_REC_BUF_SIZE_MB
#define REC_BUFFER_SIZE 8 * 1024 * 1024
#else
#define REC_BUFFER_SIZE FH_APP_REC_BUF_SIZE_MB * 1024 * 1024
#endif // !REC_BUFFER_SIZE

#define STREAM_MAGIC (0x13769849)
struct stream_head
{
    int magic;
    int type;
    int channel;
    int data_length; /*not include head itself*/
    int buffer_length; /*not include head itself*/
};

struct cbuf
{
    int  rd;
    int  wr;
    int  size;

    char buf[0];
};

static struct cbuf *g_mcbuf;
static int g_mfp[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static int g_mfpsz[MAX_GRP_NUM * MAX_VPU_CHN_NUM];
static int g_record_printed[MAX_GRP_NUM * MAX_VPU_CHN_NUM];

struct cbuf *cbuf_init(int size)
{
    if (size <= 0 || size > 20*1024*1024)
        return (void *)0;

    size = (size + 31) & (~31);

    struct cbuf *cbuf = (struct cbuf *)malloc(size + sizeof(struct cbuf));

    if (cbuf)
    {
        memset(cbuf, 0, sizeof(struct cbuf));
        cbuf->size = size;
    }

    return cbuf;
}

int cbuf_write(struct cbuf *cbuf, int type, int channel, void *data, int length)
{
    int rd;
    int wr;
    int size;
    int room;
    int segsz;
    int aligned_length = (length + 3) & (~3);
    struct stream_head *hdr;

    if (!cbuf || !data)
        return -1;

    rd   = cbuf->rd;
    wr   = cbuf->wr;
    size = cbuf->size;

    if (length <=0 || length >= size - 1024)
    {
        printf("%s: ignore big frame(%d)!\n", __func__, length);
        return -1;
    }

    if (rd > wr)
        room = rd - wr;
    else
        room = size - wr + rd;

    if (sizeof(struct stream_head) * 2 + aligned_length + 1 > room)
    {
        printf("record: drop frame(%d)!\n", length);
        return -1;
    }

    if (wr + sizeof(struct stream_head) > size)
    {
        printf("%s: internal error, line %d!\n", __func__, __LINE__);
        return -1;
    }

    hdr = (struct stream_head *)(cbuf->buf + wr);
    hdr->magic = STREAM_MAGIC;
    hdr->type = type;
    hdr->channel = channel;
    hdr->data_length = length;
    hdr->buffer_length = aligned_length;

    wr += sizeof(struct stream_head);
    if (wr >= size)
        wr = 0;

    segsz = size - wr;
    if (length > segsz)
    {
        memcpy(cbuf->buf + wr, data, segsz);
        memcpy(cbuf->buf, data + segsz, length - segsz);
    }
    else
    {
        memcpy(cbuf->buf + wr, data, length);
    }

    wr = (wr + aligned_length) % size;

    if (size - wr < sizeof(struct stream_head))
    {
        hdr->buffer_length += (size - wr);
        wr = 0;
    }

    cbuf->wr = wr;

    return 0;
}

int cbuf_prefetch(struct cbuf *cbuf,
        int *type,
        int *channel,
        void **seg1,
        void **seg2,
        int  *seg1sz,
        int  *seg2sz,
        int  *advance_length)

{
    int rd;
    int wr;
    int size;
    int clen;
    struct stream_head *hdr;

    if (!cbuf)
        return -1;

    rd   = cbuf->rd;
    wr   = cbuf->wr;
    size = cbuf->size;
    if (rd <= wr)
    {
        clen = wr - rd;
    }
    else
    {
        clen = size - rd + wr;
    }

    if (rd == wr)
    {
        return -1;
    }

    hdr = (struct stream_head *)(cbuf->buf + rd);
    if (rd + sizeof(struct stream_head) > size ||
            hdr->magic != STREAM_MAGIC ||
            hdr->data_length <= 0 || hdr->data_length >= size ||
            hdr->data_length > hdr->buffer_length ||
            sizeof(struct stream_head) + hdr->buffer_length > clen)
    {
        printf("%s: internal error, line %d!\n", __func__, __LINE__);
        return -1;
    }

    *type = hdr->type;
    *channel = hdr->channel;

    rd += sizeof(struct stream_head);
    if (rd >= size)
        rd = 0;

    *seg1 = cbuf->buf + rd;
    if (size - rd >= hdr->data_length)
    {
        *seg1sz = hdr->data_length;
        *seg2 = (void *)0;
        *seg2sz = 0;
    }
    else
    {
        *seg1sz = size - rd;
        *seg2 = cbuf->buf;
        *seg2sz = hdr->data_length - (size - rd);
    }

    *advance_length = sizeof(struct stream_head) + hdr->buffer_length;

    hdr->magic = 0; /*clear magic*/

    return 0;
}

int cbuf_clen(struct cbuf *cbuf)
{
    int rd;
    int wr;
    int size;
    int clen;

    rd   = cbuf->rd;
    wr   = cbuf->wr;
    size = cbuf->size;
    if (rd <= wr)
    {
        clen = wr - rd;
    }
    else
    {
        clen = size - rd + wr;
    }

    return clen;
}

int cbuf_advance(struct cbuf *cbuf, int advance_length)
{
    int rd;
    int size;

    if (!cbuf || advance_length <= 0)
        return -1;

    rd   = cbuf->rd;
    size = cbuf->size;

    rd = (rd + advance_length) % size;
    cbuf->rd = rd;

    return 0;
}

void mystatstic(unsigned int len)
{
    static unsigned int length;
    static struct timeval tv1;

    int t;
    struct timeval tv2;

    length += len;

    gettimeofday(&tv2, NULL);

    if (tv2.tv_sec - tv1.tv_sec >= 1)
    {
         t = (tv2.tv_sec - tv1.tv_sec) * 1000000 + tv2.tv_usec - tv1.tv_usec;
         t = 1000*length/t;
         printf("Speed:%dKB/S, %dMB/S, %dKB\n", t, t/1000, cbuf_clen(g_mcbuf)/1024);
         length = 0;
         tv1 = tv2;
    }
}

void *cbuf_write_thread_proc(void *arg)
{
    int k;
    int ret;
    int media_type;
    int media_subtype;
    int media_chn;
    void *seg1;
    void *seg2;
    int  seg1sz;
    int  seg2sz;
    int  advance_length;
    int  grpid;
    int  channel;
    char path[256];
    char online[16];

    prctl(PR_SET_NAME, "demo_record");

    while (*((volatile int *)arg) == 0)
    {
        ret = cbuf_prefetch(g_mcbuf,
                &media_type,
                &media_chn,
                &seg1,
                &seg2,
                &seg1sz,
                &seg2sz,
                &advance_length);
        if (ret != 0)
        {
            usleep(10*1000);
            continue;
        }

        media_subtype = media_type & 0xffff;
        media_type >>= 16;

        if ((media_type == DMC_MEDIA_TYPE_H264 || media_type == DMC_MEDIA_TYPE_H265) && g_mfpsz[media_chn] == 0)
        {
            if (media_subtype != DMC_MEDIA_SUBTYPE_IFRAME)
            {
                cbuf_advance(g_mcbuf, advance_length);
                continue;
            }
        }

        if (g_mfp[media_chn] < 0)
        {
            grpid = media_chn / MAX_VPU_CHN_NUM;
            channel = media_chn % MAX_VPU_CHN_NUM;
#ifdef CONFIG_VICAP_OFFLINE_MODE
            sprintf(online, "offline");
#else
            sprintf(online, "online");
#endif

            if (media_type == DMC_MEDIA_TYPE_H264)
            {
                snprintf(path, sizeof(path), "grp_%d_chan_%d_%s.h264", grpid, channel, online);
            }
            else if (media_type == DMC_MEDIA_TYPE_H265)
            {
                snprintf(path, sizeof(path), "grp_%d_chan_%d_%s.h265", grpid, channel, online);
            }
            else if (media_type == DMC_MEDIA_TYPE_AUDIO)
            {
                snprintf(path, sizeof(path), "grp_%d_chan_%d.data", grpid, channel);
            }
            else if (media_type == DMC_MEDIA_TYPE_MJPEG)
            {
                snprintf(path, sizeof(path), "grp_%d_chan_%d.mjpeg", grpid, channel);
            }

            g_mfp[media_chn] = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
            if (g_mfp[media_chn] < 0)
            {
                printf("Error: open file %s failed\n", path);
                cbuf_advance(g_mcbuf, advance_length);
                usleep(1000*1000);
                continue;
            }

            if(!g_record_printed[media_chn])
            {
                printf("Save stream in %s\n", path);
                g_record_printed[media_chn] = 1;
            }
            g_mfpsz[media_chn] = 0;
        }

        if (seg1)
        {
            ret = write(g_mfp[media_chn], seg1, seg1sz);
            if (ret != seg1sz)
            {
                printf("Error: write file fail, <%d> <%d> <%d>\n", ret, media_chn, seg1sz);
                cbuf_advance(g_mcbuf, advance_length);
                continue;
            }
            g_mfpsz[media_chn] += seg1sz;
        }

        if (seg2)
        {
            ret = write(g_mfp[media_chn], seg2, seg2sz);
            if (ret != seg2sz)
            {
                printf("Error: write file fail, <%d> <%d> <%d>\n", ret, media_chn, seg2sz);
                cbuf_advance(g_mcbuf, advance_length);
                continue;
            }
            g_mfpsz[media_chn] += seg2sz;
        }

        /*mystatstic(seg1sz + seg2sz);*/

        cbuf_advance(g_mcbuf, advance_length);

        /*fdatasync(g_mfp[media_chn]);*/

        if (g_mfpsz[media_chn] > 0x70000000)
        {
            ret = lseek(g_mfp[media_chn], 0, SEEK_SET);
            if (ret < 0)
            {
                printf("%s: lseek failed!!!\n", __func__);
            }

            printf("%s: lseek, ch=%d!!!\n", __func__, media_chn);
            g_mfpsz[media_chn] = 0;
        }
    }

    for (k=0; k < sizeof(g_mfp)/sizeof(int); k++)
    {
        if (g_mfp[k] >= 0)
        {
            close(g_mfp[k]);
            g_mfp[k] = -1;
            g_record_printed[k] = 0;
        }
    }

    *((volatile int *)arg) = 0;

    return (void *)0;
}

volatile int g_record_stream_stop = 0;

int start_cbuf_write_thread(void)
{
    pthread_attr_t attr;
    pthread_t thread_stream;

    g_record_stream_stop = 0;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
#ifdef __RTTHREAD_OS__
    pthread_attr_setstacksize(&attr, 8 * 1024);
#endif
    pthread_create(&thread_stream, &attr, cbuf_write_thread_proc, (void *)&g_record_stream_stop);
    pthread_attr_destroy(&attr);

    return 0;
}

int stop_cbuf_write_thread(void)
{
    g_record_stream_stop = 1;

    while (g_record_stream_stop)
    {
        usleep(10*1000);
    }

    if(g_mcbuf)
        free(g_mcbuf);

    return 0;
}


/********************************************
 ************Cbuf impelement end***********
 ********************************************/

static int _record_input_fn(int media_chn,
        int media_type,
        int media_subtype,
        unsigned long long frame_pts,
        unsigned char *frame_data,
        int frame_len,
        int frame_end_flag)
{
#ifdef __USING_WRITE_BUFFER
    cbuf_write(g_mcbuf, (media_type << 16) | media_subtype, media_chn, frame_data, frame_len);
    return 0;
#endif

    return 0;
}

int dmc_record_subscribe(int rawgrp)
{
#ifdef __USING_WRITE_BUFFER
    int k;
    for (k=0; k < sizeof(g_mfp)/sizeof(int); k++)
    {
        g_mfp[k] = -1;
        g_mfpsz[k] = 0;
        g_record_printed[k] = 0;
    }

    g_mcbuf = cbuf_init(REC_BUFFER_SIZE);
    start_cbuf_write_thread();
#endif

    dmc_subscribe("RECORD", rawgrp, DMC_MEDIA_TYPE_H264 | DMC_MEDIA_TYPE_H265 | DMC_MEDIA_TYPE_AUDIO | DMC_MEDIA_TYPE_MJPEG, _record_input_fn);

    return 0;
}

int dmc_record_unsubscribe(void)
{
    dmc_unsubscribe("RECORD",  DMC_MEDIA_TYPE_H264 | DMC_MEDIA_TYPE_H265 | DMC_MEDIA_TYPE_AUDIO | DMC_MEDIA_TYPE_MJPEG);

#ifdef __USING_WRITE_BUFFER
    stop_cbuf_write_thread();
#endif

    return 0;
}
