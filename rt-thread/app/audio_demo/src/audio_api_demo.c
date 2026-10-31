#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <pthread.h>
#include "dsp/fh_audio_mpi.h"

#define CHECK_RET(expr) \
    do \
    { \
    ret = expr; \
    if (ret) \
    { \
        printf("%s:%d failed %08x: "#expr"\n", __func__, __LINE__, ret); \
        goto Exit; \
    } \
    else \
    { \
        printf("done: "#expr"\n"); \
    } \
    }while (0)

static int g_running = 0;
static int g_stat_tim;
static int g_stat_samples;
static int g_exit_flag = 0;

static int config_AI_alg_parameter(int enable_hpf, int enable_nr, int enable_agc, int enable_doa, int enable_aec)
{
    int ret;
    FH_AC_SesParam alg;

    ret = FH_AC_AI_MIX_Get_Algo_Param(&alg, sizeof(alg));
    if (ret)
    {
        printf("FH_AC_AI_MIX_Get_Algo_Param: error, ret=%d!\n", ret);
        return -1;
    }

    alg.hpf_flag = enable_hpf;
    alg.anc_flag = enable_nr;
    alg.agc_flag = enable_agc;
    alg.doa_flag = enable_doa;
    alg.aec_flag = enable_aec;

    ret = FH_AC_AI_MIX_Set_Algo_Param(&alg, sizeof(alg));
    if (ret)
    {
        printf("FH_AC_AI_MIX_Set_Algo_Param: error, ret=%d!\n", ret);
        return -1;
    }

    return 0;
}

static int config_AO_alg_parameter(int enable_hpf, int enable_nr, int enable_agc)
{
    int ret;
    FH_AC_SesParam alg;

    ret = FH_AC_AO_Get_Algo_Param(&alg, sizeof(alg));
    if (ret)
    {
        printf("FH_AC_AO_Get_Algo_Param: error, ret=%d!\n", ret);
        return -1;
    }

    alg.hpf_flag = enable_hpf;
    alg.anc_flag = enable_nr;
    alg.agc_flag = enable_agc;

    ret = FH_AC_AO_Set_Algo_Param(&alg, sizeof(alg));
    if (ret)
    {
        printf("FH_AC_AO_Set_Algo_Param: error, ret=%d!\n", ret);
        return -1;
    }

    return 0;
}

static void statistic_start(void)
{
    g_stat_samples = 0;
    g_stat_tim = time(0);
}

static void statistic(int samples)
{
    int now;
    int diff;

    g_stat_samples += samples;
    now = time(0);
    diff = now - g_stat_tim;
    if (diff >= 5)
    {
        printf("Sample rate:%d\n", g_stat_samples / diff);
        g_stat_samples = 0;
        g_stat_tim = now;
    }
}

static int channel_num(int mask)
{
    int i;
    int num = 0;

    for (i=0; i<32; i++)
    {
        if (mask & (1 << i))
        {
            num++;
        }
    }

    return num;
}

static void save_ai_data(char *filename, FH_AC_AI_Frame_S *frame)
{
    int i;
    char path[256];
    char *data;
    static int total;
    static int fd_initialized = 0;
    static int fd[FH_AI_CHANNEL_NUM+2];

    if (!filename || total >= 12*1024*1024) /*don't save too large file.*/
        return;

    if (!fd_initialized)
    {
        for (i=0; i<FH_AI_CHANNEL_NUM+2; i++)
            fd[i] = -1;
        fd_initialized = 1;
    }

    for (i=0; i<FH_AI_CHANNEL_NUM+2; i++)
    {
        if (i == FH_AI_CHANNEL_NUM)
        {
            sprintf(path, "%s_far.pcm", filename);
            data = frame->far;
        }
        else if (i == FH_AI_CHANNEL_NUM + 1)
        {
            sprintf(path, "%s_mix.pcm", filename);
            data = frame->mix;
        }
        else
        {
            sprintf(path, "%s_ch%d.pcm", filename, i);
            data = frame->ch_data[i];
        }

        if (!data)
            continue;

        if (fd[i] < 0)
        {
            fd[i] = open(path, O_RDWR | O_TRUNC | O_CREAT);
            if (fd[i] < 0)
            {
                printf("Create file %s failed!\n", path);
                return;
            }
            fchmod(fd[i], 0644);
        }

        if (frame->ch_data_len != write(fd[i], data, frame->ch_data_len))
        {
            printf("write file %s failed!\n", path);
        }
    }

    total += frame->ch_data_len;
}

static int read_from_file(char *file_name, char *data, int len)
{
    int ret;
    static int fd = -1;

    if (fd < 0)
    {
        fd = open(file_name, O_RDONLY);
        if (fd < 0)
        {
            printf("Error: cann't open file %s!\n", file_name);
            return -1;
        }
    }

    while (len > 0)
    {
        ret = read(fd, data, len);
        if (ret <= 0 || ret > len || (ret & 1) != 0)
        {
            lseek(fd, 0, SEEK_SET);
            printf("Seek playback file...\n");
            continue;
        }

        data += ret;
        len  -= ret;
    }

    return 0;
}

static void *audio_run(void *arg)
{
    int test_ai = 1; /*开启AI测试*/
    int test_ao = 1; /*开启AO测试*/
    int enable_hpf = 1;
    int enable_nr = 1;
    int enable_agc = 1;
    int enable_aec = 1;
    int enable_doa = 0;

    int sample_rate = 16000;
    int period_size = 1024;
    int channel_mask = 1; /*default enable mic0*/
    int mix_mask = 0;
    int ain_select = FH_AC_MIC_IN;
    int micin_vol = 2;
    int ain_vol  = 28;
    int aout_vol = 3;
    char *record_file = NULL;
    char *playback_file = NULL;

    FH_UINT32    version1;
    FH_UINT32    version2;
    FH_UINT32    support_chmask;
    FH_AC_CONFIG ac_config;
    FH_AC_MIX_CONFIG mix_config;
    FH_AC_AI_Frame_S ai_frame;
    FH_AC_AO_FRAME_S ao_frame;
    char  playback_buffer[period_size*2];
    int ai_enabled = 0;
    int ao_enabled = 0;
    int ret;

    if (!test_ai && !test_ao)
    {
        printf("Error: please specify test AI and or AO!\n");
        goto Exit;
    }

    if (test_ao && !(playback_file || test_ai))
    {
        printf("Error: please specify playback file!\n");
        goto Exit;
    }

    printf("[INFO]: sample_rate=%d\n", sample_rate);

    CHECK_RET(FH_AC_Init());

    CHECK_RET(FH_AC_Get_Algo_Version(&version1, &version2));
    printf("[INFO]: Alg version(%d.%d.%d),%08x-%d\n", version2>>24,(version2>>16)&0xff,(version2>>8)&0xff, version2, version1);

    CHECK_RET(FH_AC_AI_Get_Feature_ChannelMask(&support_chmask));
    printf("[INFO]: supported AI channel_mask=%08x.\n", support_chmask);

    statistic_start();

    /***********************Config AI***********************************/
    if (test_ai)
    {
        if ((channel_mask & support_chmask) != channel_mask || !channel_mask)
        {
            channel_mask = support_chmask;
            printf("[INFO]: Auto change channel_mask to %08x!\n", channel_mask);
        }

#ifndef CONFIG_ARCH_MC632X
        ac_config.codec_sel = FH_AC_CODEC_INTERNAL;
#endif
        ac_config.io_type = ain_select;
        ac_config.sample_rate = sample_rate;
        ac_config.bit_width = 16;
        ac_config.enc_type  = FH_PT_LPCM;
        ac_config.period_size = period_size;
        ac_config.channel_mask = channel_mask;
        ac_config.frame_num = 0; /*default 0,mean use max buffer.*/
        ac_config.reserved = 0;

        CHECK_RET(FH_AC_Set_Config(&ac_config));

        if (!mix_mask)
            mix_mask = channel_mask;

        if (channel_num(mix_mask) > 1)
            enable_doa = 1;

        mix_config.mix_enable = 1;
        mix_config.mix_channel_mask = mix_mask;
        mix_config.reserved = 0;
        CHECK_RET(FH_AC_AI_MIX_Set_Config(&mix_config));
        CHECK_RET(FH_AC_AI_CH_SetAnologVol(0x0f, micin_vol, ain_vol));
        CHECK_RET(config_AI_alg_parameter(enable_hpf, enable_nr, enable_agc, enable_doa, enable_aec));
    }

    /***********************Config AO***********************************/
    if (test_ao)
    {
#ifndef CONFIG_ARCH_MC632X
        ac_config.codec_sel = FH_AC_CODEC_INTERNAL;
#endif
        ac_config.io_type = FH_AC_LINE_OUT;
        ac_config.sample_rate = sample_rate;
        ac_config.bit_width = 16;
        ac_config.enc_type  = FH_PT_LPCM;
        ac_config.period_size = period_size;
        ac_config.channel_mask = 0x01; /*for AO, only one channel is supported*/
        ac_config.frame_num = 0; /*default 0,mean use max buffer*/
        ac_config.reserved = 0;

        CHECK_RET(FH_AC_Set_Config(&ac_config));
        CHECK_RET(FH_AC_AO_SetAnologVol(aout_vol));
        CHECK_RET(config_AO_alg_parameter(0, 0, 0));
    }

    /***********************Enable AI/AO*********************************/
    if (test_ai && test_ao)
    {
        CHECK_RET(FH_AC_AI_AO_SYNC_Enable());
        ai_enabled = 1;
        ao_enabled = 1;
        printf("[INFO]: start capturing and playing audio data...\n");
    }
    else
    {
        if (test_ai)
        {
            CHECK_RET(FH_AC_AI_Enable());
            ai_enabled = 1;
            printf("[INFO]: start capturing audio data...\n");
        }
        if (test_ao)
        {
            CHECK_RET(FH_AC_AO_Enable());
            ao_enabled = 1;
            printf("[INFO]: start playing audio data...\n");
        }
    }

    while (!g_exit_flag)
    {
        if (test_ai)
        {
            ret = FH_AC_AI_GetFrame(&ai_frame);
            if (ret == 0)
            {
                if (ai_frame.ch_data_len != period_size * 2 || !ai_frame.mix)
                {
                    printf("Error: FH_AC_AI_GetFrame,len=%d,mix=%p,it should not happen!!!\n", ai_frame.ch_data_len, ai_frame.mix);
                    continue;
                }

                statistic(ai_frame.ch_data_len/2);
                memcpy(playback_buffer, ai_frame.mix, ai_frame.ch_data_len);
                save_ai_data(record_file, &ai_frame);
            }
            else
            {
                printf("Error: FH_AC_AI_GetFrame,ret=%d!\n", ret);
                continue;
            }
        }

        if (test_ao)
        {
            if (playback_file)
            {
                ret = read_from_file(playback_file, playback_buffer, period_size * 2);
                if (ret < 0)
                {
                    printf("Error: read playback file failed!\n");
                    break;
                }
            }

            ao_frame.len = period_size * 2;
            ao_frame.data = (FH_UINT8 *)playback_buffer;
            ret = FH_AC_AO_SendFrame(&ao_frame);
            if (ret == 0)
            {
                if (!test_ai)
                    statistic(ao_frame.len/2);
            }
            else
            {
                printf("Error: FH_AC_AO_SendFrame failed,ret=%d!\n", ret);
            }
        }
    }
    printf("[INFO]: stop audio test!\n");

Exit:
    if (ai_enabled)
        FH_AC_AI_Disable();

    if (ao_enabled)
        FH_AC_AO_Disable();

    FH_AC_DeInit();

    printf("audio_demo: finished!\n");
    g_running = 0;

    return 0;
}

void audio_demo(int argc, char *argv[])
{
    pthread_attr_t attr;
    pthread_t      thd;

    if (g_running)
    {
        g_exit_flag = 1;
        printf("audio_demo: send exit signal...\n");
        return;
    }

    g_running = 1;
    g_exit_flag = 0;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 4 * 1024);
    pthread_create(&thd, &attr, audio_run, NULL);
}

void user_main(void)
{
}

#include <rttshell.h>
SHELL_CMD_EXPORT(audio_demo, audio demo/with -h to show help);
