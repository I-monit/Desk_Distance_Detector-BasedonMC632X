#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "libdmc/include/libdmc.h"
#include "libdmc/include/libdmc_rtsp.h"
#include "librtsp/include/librtsp.h"

#define MAX_RTSP_CHANNEL (32) /*it should be enough*/
static RTSP_HANDLE g_rtsp_server[MAX_RTSP_CHANNEL];

static unsigned int g_rtsp_port[MAX_RTSP_CHANNEL];
static unsigned int g_printed[MAX_RTSP_CHANNEL];

static int _rtsp_input_fn(int media_chn,
        int media_type,
        int media_subtype,
        unsigned long long frame_pts,
        unsigned char *frame_data,
        int frame_len,
        int frame_end_flag)
{
    if ((unsigned int)media_chn >= MAX_RTSP_CHANNEL)
        return 0;

    if (media_type == DMC_MEDIA_TYPE_H264)
    {
        librtsp_push_data(g_rtsp_server[media_chn], frame_data,frame_len, frame_pts, 0/*FIXME, not used yet*/, FH_VENC_H264);
    }
    else if (media_type == DMC_MEDIA_TYPE_H265)
    {
        librtsp_push_data(g_rtsp_server[media_chn], frame_data,frame_len, frame_pts, 0/*FIXME, not used yet*/, FH_VENC_H265);
    }
    else if (media_type == DMC_MEDIA_TYPE_AUDIO)
    {
        librtsp_push_data(g_rtsp_server[media_chn], frame_data,frame_len, frame_pts, 0/*FIXME, not used yet*/, FH_SDP_LPCM_L16_R16/*FIXME*/);
    }

    if (!g_printed[media_chn])
    {
        printf("RTSP: listen on port %d ... \n", g_rtsp_port[media_chn]);
        g_printed[media_chn] = 1;
    }

    return 0;
}

int dmc_rtsp_subscribe(int rtspgrp, int port)
{
    int i;
    int curgrp;
    int code = 0x1;

    curgrp = 0;
    while(curgrp <= MAX_GRP_NUM) {
        if(rtspgrp & code) {
            for(i = curgrp * MAX_VPU_CHN_NUM; i < (curgrp + 1) * MAX_VPU_CHN_NUM; i++) {
                g_rtsp_server[i] = librtsp_start_server(RTP_TRANSPORT_TCP, port + i * 2 + MAX_VPU_CHN_NUM * MAX_GRP_NUM);
                g_printed[i] = 0;
                g_rtsp_port[i] = port + i * 2 + MAX_VPU_CHN_NUM * MAX_GRP_NUM;
            }
        }
        code <<= 1;
        curgrp++;
    }

    dmc_subscribe("RTSP", rtspgrp, DMC_MEDIA_TYPE_H264 | DMC_MEDIA_TYPE_H265 | DMC_MEDIA_TYPE_AUDIO, _rtsp_input_fn);

    return 0;
}

int dmc_rtsp_unsubscribe(void)
{
    int i;

    dmc_unsubscribe("RTSP", DMC_MEDIA_TYPE_H264 | DMC_MEDIA_TYPE_H265 | DMC_MEDIA_TYPE_AUDIO);

    for (i=0; i<MAX_RTSP_CHANNEL; i++)
    {
        if (g_rtsp_server[i])
        {
            librtsp_stop_server(g_rtsp_server[i]);
            g_rtsp_server[i] = NULL;
        }
    }

    return 0;
}
