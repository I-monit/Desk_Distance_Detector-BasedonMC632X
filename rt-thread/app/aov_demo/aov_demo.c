#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "types/type_def.h"
#include "dsp/fh_system_mpi.h"
#include "dsp/fh_vpu_mpi.h"
#include "dsp/fh_venc_mpi.h"
#include "isp/isp_enum.h"
#include <fh_rpc_customer_mpi.h>
#include "aov_xbus.h"
#include "libpes/include/libpes.h"
#include "sample_opts.h"
#include "librecord/include/librecord.h"

#define GRP_ENABLED 2
#define MAX_PES_CHANNEL_COUNT 36

static FH_BOOL g_stop_running = FH_FALSE;
static pthread_t g_thread_stream = 0;
static pthread_t g_aov_thread = 0;
static pthread_t g_thread_status = 0;
static int g_aov_resume_reason = 0;
pthread_mutex_t g_aov_mutex = PTHREAD_MUTEX_INITIALIZER;

void sample_vlcview_exit(void)
{
    if (g_thread_stream != 0)
        pthread_join(g_thread_stream, NULL);
}

void sample_vlcview_handle_sig(FH_SINT32 signo)
{
    printf("Caught %d, program exit abnormally\n", signo);
    g_stop_running = FH_TRUE;
    sample_vlcview_exit();
    libpes_uninit();
#ifdef OPEN_RECORD
    librecord_uninit();
#endif
    exit(EXIT_FAILURE);
}


void usage(char *program_name)
{
    fprintf(stderr, "\nUsage:  %s  <VLC IP address>  [port number (optional, default 1234)]\n\n", program_name);
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "    %s 192.168.1.3\n", program_name);
    fprintf(stderr, "    %s 192.168.1.3 2345\n", program_name);
}

int linux_aov_config(int grpid, int enable, int interval_ms, int fream_num)
{
    AOV_XBUS_CMD_S aov_cmd;

    memset(&aov_cmd, 0, sizeof(aov_cmd));
    aov_cmd.cmd = AOV_CMD_START_AOV;
    aov_cmd.grpid = grpid;
    aov_cmd.data.aov.aov_enable = enable;
    /* aov_cmd.data.video.AovChnNum = 1; */
    aov_cmd.data.aov.aov_interval = interval_ms;
    aov_cmd.data.aov.fream_num = fream_num; /* 帧号好计数一些，时间不好控制 */
    return FH_RPC_Customer_Command(RPC_AOV_CMD_3, &aov_cmd, sizeof(aov_cmd));
}

int linux_aov_exit(void)
{
    AOV_XBUS_CMD_S aov_cmd;

    memset(&aov_cmd, 0, sizeof(aov_cmd));
    aov_cmd.cmd = AOV_CMD_EXIT_AOV;
    aov_cmd.data.aov.aov_enable = 0;
    printf("%s: AOV_CMD_EXIT_AOV!\n", __func__);
    return FH_RPC_Customer_Command(RPC_AOV_CMD_3, &aov_cmd, sizeof(aov_cmd));
}

void *sample_vlcview_get_stream_proc(void *arg)
{
    FH_SINT32 ret = 0, i = 0;
    FH_VENC_STREAM stream;
    unsigned int chan = 0;
#ifdef OPEN_RECORD
    FH_SINT32 subtype = 0;
#endif

    struct vlcview_enc_stream_element stream_element;
    while (!g_stop_running)
    {
        memset(&stream_element, 0, sizeof(stream_element));
        memset(&stream, 0, sizeof(stream));
        pthread_mutex_lock(&g_aov_mutex);
        ret = FH_VENC_GetStream_Timeout(FH_STREAM_H265 | FH_STREAM_H264, &stream, 10 * 1000);
        if (ret == RETURN_OK)
        {
            if (stream.stmtype == FH_STREAM_H265)
            {
                stream_element.enc_type = VLCVIEW_ENC_H265;
                stream_element.frame_type = stream.h265_stream.frame_type == FH_FRAME_I ? VLCVIEW_ENC_I_FRAME : VLCVIEW_ENC_P_FRAME;
                stream_element.frame_len = stream.h265_stream.length;
                stream_element.time_stamp = stream.h265_stream.time_stamp;
                stream_element.nalu_count = stream.h265_stream.nalu_cnt;
                chan = stream.chan;
                for (i = 0; i < stream_element.nalu_count; i++)
                {
                    stream_element.nalu[i].start = stream.h265_stream.nalu[i].start;
                    stream_element.nalu[i].len = stream.h265_stream.nalu[i].length;
                    printf("[chan %d]FH_VENC_GetChnStream_Timeout start %p length %d\n", stream.chan, stream.h264_stream.nalu[i].start, stream.h264_stream.nalu[i].length);
#ifdef OPEN_RECORD
                    subtype = stream.h265_stream.frame_type == FH_FRAME_I ? DMC_MEDIA_SUBTYPE_IFRAME : DMC_MEDIA_SUBTYPE_PFRAME;
                    librecord_save(stream.chan, DMC_MEDIA_TYPE_H265, subtype, stream.h265_stream.nalu[i].start, stream.h265_stream.nalu[i].length);
#endif
                }
            }
            else
            {
                stream_element.enc_type = VLCVIEW_ENC_H264;
                stream_element.frame_type = stream.h264_stream.frame_type == FH_FRAME_I ? VLCVIEW_ENC_I_FRAME : VLCVIEW_ENC_P_FRAME;
                stream_element.frame_len = stream.h264_stream.length;
                stream_element.time_stamp = stream.h264_stream.time_stamp;
                stream_element.nalu_count = stream.h264_stream.nalu_cnt;
                chan = stream.chan;
                for (i = 0; i < stream_element.nalu_count; i++)
                {
                    stream_element.nalu[i].start = stream.h264_stream.nalu[i].start;
                    stream_element.nalu[i].len = stream.h264_stream.nalu[i].length;
                    printf("[chan %d]FH_VENC_GetChnStream_Timeout start %p length %d\n", stream.chan, stream.h264_stream.nalu[i].start, stream.h264_stream.nalu[i].length);

#ifdef OPEN_RECORD
                    subtype = stream.h264_stream.frame_type == FH_FRAME_I ? DMC_MEDIA_SUBTYPE_IFRAME : DMC_MEDIA_SUBTYPE_PFRAME;
                    librecord_save(stream.chan, DMC_MEDIA_TYPE_H264, subtype, stream.h264_stream.nalu[i].start, stream.h264_stream.nalu[i].length);
#endif
                }
            }
#ifdef FH_USING_STREAM_UDP
            libpes_stream_pack(chan, stream_element);
#endif
            FH_VENC_ReleaseStream(&stream);
        }
        pthread_mutex_unlock(&g_aov_mutex);
        usleep(1);
    }

    return NULL;
}

void *check_event_status_proc(void *arg)
{
    int aov_event_status = 0;

    while (!g_stop_running)
    {
        pthread_mutex_lock(&g_aov_mutex);
        aov_event_status = check_event_status();
        if ((aov_event_status & AOV_WAKEUP_USER))
        {
            linux_aov_exit();
        }
        pthread_mutex_unlock(&g_aov_mutex);
        sleep(1);
    }
}

int check_event_status(void)
{
    AOV_XBUS_CMD_S aov_cmd;
    int aov_event_status = 0;
    int ret = 0;

    memset(&aov_cmd, 0, sizeof(aov_cmd));
    aov_cmd.cmd = AOV_CMD_QUERY_STATE;

    ret = FH_RPC_Customer_Command(RPC_AOV_CMD_3 | RPC_CUSTOMER_FLAG_NEED_CopyBack, &aov_cmd, sizeof(aov_cmd));
    if (0 == ret)
    {
        printf("GRPID %d AOV_CMD_QUERY_STATE = 0x%x\n", aov_cmd.data.wake_source.grpid, aov_cmd.data.wake_source.event_status);
    }
    aov_event_status = aov_cmd.data.wake_source.event_status;

    return aov_event_status;
}

void *aov_main(void *arg)
{
    FH_SINT32 ret = 0;
    int aov_event_status = 0;
    static AOV_XBUS_CMD_S aov_cmd;

    ret = FH_RPC_Customer_Init();
    if (ret != 0)
    {
        printf("FH_RPC_Customer_Init: failed, error=%08x\n", ret);
        return NULL;
    }

    pthread_create(&g_thread_status, NULL, check_event_status_proc, NULL);

    while (1)
    {
        aov_event_status = check_event_status();
        if ((aov_event_status & AOV_WAKEUP_USER))
        {
            linux_aov_exit();
            sleep(30);
        }
        pthread_mutex_lock(&g_aov_mutex);
        ret = linux_aov_config(0, 1, 1000, 300);
        if (ret)
            printf("linux linux_aov_config failed ret %x ....\n", ret);

        memset(&aov_cmd, 0, sizeof(aov_cmd));
        aov_cmd.cmd = AOV_CMD_SLEEP;
        ret = FH_RPC_Customer_Command(RPC_AOV_CMD_3, &aov_cmd, sizeof(aov_cmd));
        if (ret < 0)
        {
            printf("FH_RPC_Customer_Command: failed, error=%08x\n", ret);
        }

    extern void fh_aov_arm_sleep(void);
        fh_aov_arm_sleep();

        aov_cmd.cmd = AOV_CMD_WAKE_REASON;
        ret = FH_RPC_Customer_Command(RPC_AOV_CMD_3 | RPC_CUSTOMER_FLAG_NEED_CopyBack, &aov_cmd, sizeof(aov_cmd));
        if (ret == 0)
        {
            printf("GRPID %d AOV_CMD_WAKE_REASON = 0x%x\n", aov_cmd.data.wake_source.grpid, aov_cmd.data.wake_source.wakeup_reason);
        }
        g_aov_resume_reason = aov_cmd.data.wake_source.wakeup_reason;
        pthread_mutex_unlock(&g_aov_mutex);
        if ((g_aov_resume_reason & AOV_WAKEUP_USER))
        {
            printf("[%s-%d]: wake up by user!!!!\n", __func__, __LINE__);
            linux_aov_exit();
            sleep(30);
        }
        else
        {
            sleep(10);
        }
    }
    FH_RPC_Customer_DeInit();

    while (1)
    {
        usleep(20000);
    }
}

int aov_demo(int argc, char const *argv[])
{
    FH_SINT32 ret = 0;
    static int port = -1;
    static char ip[16];
#ifdef FH_USING_STREAM_UDP
    int i = 0;
    int index = 0;
    int code = 0;
    int curgrp = 0;
    int pesgrp = 0;
#endif

    if (argc < 2)
    {
        usage((char *)argv[0]);
        return -1;
    }
    if (argc > 2)
    {
        memcpy(ip, argv[1], strlen(argv[1]));
        port = strtol(argv[2], NULL, 0);
        if (port == 0)
        {
            printf("Error: invalid port number\n");
            return -2;
        }
    }

    if (port == -1)
        port = 1234;

#ifdef FH_USING_STREAM_UDP
    ret = libpes_init();
    if (ret != 0)
    {
        printf("Error: libpes_init failed with %d\n", ret);
    }
#endif

#ifdef OPEN_RECORD
    ret = librecord_init();
    if (ret != 0)
    {
        printf("Error: librecord_init failed with %d\n", ret);
    }
#endif

    /******************************************
     *step  12: get stream, pack as PES stream and then send to vlc
    ******************************************/
    pthread_create(&g_thread_stream, NULL, sample_vlcview_get_stream_proc, NULL);

    printf("ip: %s, %s\n", ip, (char *)argv[1]);
#ifdef FH_USING_STREAM_UDP
    while (index < MAX_GRP_NUM)
    {
        pesgrp |= 1 << index;
        index++;
    }

    code = 0x1;

    if (pesgrp > MAX_PES_CHANNEL_COUNT)
    {
        printf("Error: channel num is larger than %d\n", MAX_PES_CHANNEL_COUNT);
        return -1;
    }

    while (curgrp <= MAX_GRP_NUM)
    {
        if (pesgrp & code)
        {
            for (i = curgrp * MAX_VPU_CHN_NUM; i < (curgrp + 1) * MAX_VPU_CHN_NUM; i++)
            {
                libpes_send_to_vlc(i, (char *)argv[1], port + i);
            }
        }
        code <<= 1;
        curgrp++;
    }
#endif

    pthread_create(&g_aov_thread, NULL, aov_main, NULL);

    return ret;
}

void user_main(void)
{
}

#include <rttshell.h>
SHELL_CMD_EXPORT(aov_demo, aov_demo());
