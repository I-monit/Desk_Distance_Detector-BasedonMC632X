#include "libusb_rtt/usb_audio/include/usb_audio.h"

static pthread_t g_thread_audio_stream;
static int g_uac_th_status = 0;

extern void fh_uac_init(void);
extern int fh_uac_micstatus_get(void);
extern void fh_uac_micstatus_set(int set);
extern void fh_uac_micstream_enqueue(uint8_t *audio_data, uint32_t audio_size);

static unsigned int mic_valume_old;
#ifdef USB_HID_ENABLE
unsigned int hid_mic_mute_status = 0;
extern unsigned char telephony_status;
#endif
void ERR_RETURN(char *name, int ret_val)
{
    if (ret_val != 0)
        printf("%s failed(%d)\n", name, ret_val);
}

/*
 * vol: [0-100]
 */
static int DSP_Audio_Codec_SetMicGain(unsigned int vol)
{
    int ret;
    int genVol = 0;
    int micVol = 0;

    if (vol < 0)
    {
        vol = 0;
    }

    if (vol > 100)
    {
        vol = 100;
    }
    if (vol != 0)
        mic_valume_old = vol;
    vol = vol * 52 / 100; /* map to FH's audio code for micin */

    if (vol == 0) /*mute*/
    {
    }
    else if (vol <= 31) /*[1-31]*/
    {
        micVol = 0;
        genVol = vol;
    }
    else if (vol <= 39) /*[32-39]*/
    {
        micVol = 1;
        genVol = vol - 8;
    }
    else if (vol <= 47) /*[40-47]*/
    {
        micVol = 2;
        genVol = vol - 16;
    }
    else /*[48-52]*/
    {
        micVol = 3;
        genVol = vol - 21;
    }

    ret = FH_AC_AI_CH_SetAnologVol(7, micVol, genVol);

    return ret;
}

extern void hid_send_cmd(char val);

int _audio_set_mute(unsigned int flag, unsigned int hid_flag)
{
    int ret;

    if (flag == 1)
    {
        ret = DSP_Audio_Codec_SetMicGain(0);
#ifdef USB_HID_ENABLE
        if (hid_flag)
            hid_mic_mute_status = 1;
        if (hid_flag && !(telephony_status & HID_TELEPHONY_OUT_MUTE))
        {
            hid_send_cmd(HID_TELEPHONY_IN_PHONE_MUTE);
            usleep(20 * 1000);
            hid_send_cmd(0);
        }
#endif
    }
    else
    {
        ret = DSP_Audio_Codec_SetMicGain(mic_valume_old);
#ifdef USB_HID_ENABLE
        if (hid_flag)
            hid_mic_mute_status = 0;
        if (hid_flag && telephony_status & HID_TELEPHONY_OUT_MUTE)
        {
            hid_send_cmd(HID_TELEPHONY_IN_PHONE_MUTE);
            usleep(20 * 1000);
            hid_send_cmd(0);
        }
#endif
    }
    return ret;
}

static int audio_set_mute(unsigned int flag)
{
    return _audio_set_mute(flag, 1);
}

static int audio_sample_change(unsigned int sample_rate)
{
    FH_AC_CONFIG ac_config;

    ERR_RETURN("FH_AC_AI_Disable", FH_AC_AI_Disable());

#ifndef CONFIG_ARCH_MC632X
    ac_config.codec_sel = FH_AC_CODEC_INTERNAL;
#endif
    ac_config.io_type = FH_AC_MIC_IN;
    ac_config.sample_rate = sample_rate;
    ac_config.bit_width = AC_BW_16;
    ac_config.enc_type = FH_PT_LPCM;
    ac_config.period_size = 512;
    ac_config.channel_mask = 1;
    ac_config.frame_num = 0;
    ac_config.reserved = 0;

    ERR_RETURN("FH_AC_Set_Config", FH_AC_Set_Config(&ac_config));

    ERR_RETURN("FH_AC_AI_MICIN_SetVol", DSP_Audio_Codec_SetMicGain(mic_valume_old));
    ERR_RETURN("FH_AC_AI_Enable", FH_AC_AI_Enable());
    printf("uac mic sample rate change %d\n", sample_rate);
    return 0;
}
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

static int uac_audio_init(void)
{
    FH_AC_CONFIG ac_config;
    FH_AC_MIX_CONFIG mix_config;
#ifndef CONFIG_ARCH_MC632X
    ac_config.codec_sel = FH_AC_CODEC_INTERNAL;
#endif
    ac_config.io_type = FH_AC_MIC_IN;
    ac_config.sample_rate = 8000;
    ac_config.bit_width = AC_BW_16;
    ac_config.enc_type = FH_PT_LPCM;
    ac_config.period_size = 512;
    ac_config.channel_mask = 1;
    ac_config.frame_num = 0;
    ac_config.reserved = 0;

    ERR_RETURN("FH_AC_Init()", FH_AC_Init());

    ERR_RETURN("FH_AC_Set_Config", FH_AC_Set_Config(&ac_config));
    printf("[INFO]: FH_AC_Set_Config ok\n");

    mix_config.mix_enable = 1;
    mix_config.mix_channel_mask = 1;
    mix_config.reserved = 0;
    ERR_RETURN("FH_AC_AI_MIX_Set_Config", FH_AC_AI_MIX_Set_Config(&mix_config));
    printf("[INFO]: FH_AC_AI_MIX_Set_Config ok\n");

    config_AI_alg_parameter(1, 1, 1, 0, 0);

    ERR_RETURN("FH_AC_AI_MICIN_SetVol()", DSP_Audio_Codec_SetMicGain(75));
    ERR_RETURN("FH_AC_AI_Enable", FH_AC_AI_Enable());
    printf("[INFO]: FH_AC_AI_Enable ok\n");
    mic_valume_old = UAC_MIC_DEFAULT_VOLUME;

    printf("[INFO]: start capturing audio data ...\n");
    return 0;
}

static unsigned char mic_buf[4096];
void *audio_stream_proc(void *arg)
{
    FH_SINT32 ret = 0;
    int status;
    FH_AC_AI_Frame_S audio_frame;

    prctl(PR_SET_NAME, "audio_stream_proc");
    while (g_uac_th_status)
    {
        status = fh_uac_micstatus_get();
        if (status == UAC_STREAM_ON)
        {
            ret = FH_AC_AI_GetFrame(&audio_frame);
            if (ret == RETURN_OK && audio_frame.ch_data_len > 0 && audio_frame.mix && AUDIO_CHN_NUM_MIC == 2)
            {
                FH_UINT32 *pData = (FH_UINT32 *)mic_buf;
                int i = 0;

                memset(mic_buf, 0, 4096);
                for (i = 0; i < audio_frame.ch_data_len; i = i + 2)
                {
                    *pData = audio_frame.mix[i] | audio_frame.mix[i + 1] << 8 |
                             audio_frame.mix[i] << 16 | audio_frame.mix[i + 1] << 24;
                    pData++;
                }
                fh_uac_micstream_enqueue(mic_buf, audio_frame.ch_data_len * AUDIO_CHN_NUM_MIC);
            }
            else if (ret == RETURN_OK && audio_frame.ch_data_len > 0 && audio_frame.mix && AUDIO_CHN_NUM_MIC == 1)
            {
                fh_uac_micstream_enqueue((FH_UINT8 *)audio_frame.mix, audio_frame.ch_data_len);
            }
            else
            {
                usleep(1000);
            }
        }
        else
        {
            FH_AC_AI_GetFrame(&audio_frame);
            usleep(1000);
        }
    }
    return NULL;
}

#ifdef ENABLE_SPEAKER_DESC
static pthread_t g_thread_speaker_stream;
static unsigned int spk_valume_old;

/*
 * vol: [0-100]
 */
static int DSP_Audio_Codec_SetSpeakerGain(unsigned int vol)
{
    int ret;
    int index;
    int map[] = {
        0, /*mute*/
        1, /*0db*/
        2, /*2db*/
        3, /*4db*/
        4, /*6db*/
    };

    if (vol != 0)
        spk_valume_old = vol;

    if (vol < 0)
    {
        vol = 0;
    }

    if (vol > 100)
    {
        vol = 100;
    }

    index = (vol + 24) * 4 / 100; /*map to FH's audio codec for speaker*/
    ret = FH_AC_AO_SetAnologVol(map[index]);

    return ret;
}

static int speaker_mute_set(unsigned int flag)
{
    int ret;

    if (flag == 1)
    {
        ret = DSP_Audio_Codec_SetSpeakerGain(0);
    }
    else
    {
        ret = DSP_Audio_Codec_SetSpeakerGain(spk_valume_old);
    }
    return ret;
}

static int uac_speaker_init(void)
{
    FH_AC_CONFIG ac_config;

#ifndef CONFIG_ARCH_MC632X
    ac_config.codec_sel = FH_AC_CODEC_INTERNAL;
#endif
    ac_config.io_type = FH_AC_LINE_OUT;
    ac_config.sample_rate = AC_SR_16K;
    ac_config.bit_width = 16;
    ac_config.enc_type = FH_PT_LPCM;
    ac_config.period_size = 512;
    ac_config.channel_mask = 0x01;
    ac_config.frame_num = 0;
    ac_config.reserved = 0;

    ERR_RETURN("FH_AC_Set_Config", FH_AC_Set_Config(&ac_config));
    printf("[INFO]: FH_AC_Set_Config ok\n");

    ERR_RETURN("FH_AC_AO_SetVol", DSP_Audio_Codec_SetSpeakerGain(75));
    ERR_RETURN("FH_AC_AO_Enable", FH_AC_AO_Enable());
    printf("[INFO]: FH_AC_AO_Enable ok\n");
    spk_valume_old = UAC_SPK_DEFAULT_VOLUME;

    return 0;
}

static int speaker_sample_change(unsigned int sample_rate)
{
    FH_AC_CONFIG ac_config;

    ERR_RETURN("FH_AC_AO_Disable", FH_AC_AO_Disable());

#ifndef CONFIG_ARCH_MC632X
    ac_config.codec_sel = FH_AC_CODEC_INTERNAL;
#endif
    ac_config.io_type = FH_AC_LINE_OUT;
    ac_config.sample_rate = sample_rate;
    ac_config.bit_width = 16;
    ac_config.enc_type = FH_PT_LPCM;
    ac_config.period_size = 512;
    ac_config.channel_mask = 0x01;
    ac_config.frame_num = 0;
    ac_config.reserved = 0;

    ERR_RETURN("FH_AC_Set_Config", FH_AC_Set_Config(&ac_config));

    ERR_RETURN("FH_AC_AO_Enable", FH_AC_AO_Enable());

    ERR_RETURN("FH_AC_AO_SetVol", DSP_Audio_Codec_SetSpeakerGain(spk_valume_old));

    printf("uac speaker sample rate change %d\n", sample_rate);
    return 0;
}

static unsigned char spk_buf[4096];
void *speaker_stream_proc(void *arg)
{
    FH_AC_AO_FRAME_S audio_frame_spk;
    struct uac_frame_buf *speaker_data = NULL;
    int status;

    prctl(PR_SET_NAME, "speaker_stream_proc");
    while (g_uac_th_status)
    {
        status = fh_uac_spkstatus_get();
        if (status == UAC_STREAM_ON)
        {
            speaker_data = fh_uac_spkstream_dequeue();
            if (speaker_data && AUDIO_CHN_NUM_SPK == 2)
            {
                FH_UINT16 *pData = (FH_UINT16 *)spk_buf;
                int i = 0;

                memset(spk_buf, 0, 4096);
                for (i = 0; i < speaker_data->uac_frame_size; i = i + 4)
                {
                    *pData = speaker_data->data[i] | speaker_data->data[i + 1] << 8;
                    pData++;
                }
                audio_frame_spk.data = (FH_UINT8 *)spk_buf;
                audio_frame_spk.len = speaker_data->uac_frame_size / 2;
                FH_AC_AO_SendFrame(&audio_frame_spk);
                fh_uac_spkstream_enqueue(speaker_data);
            }
            else if (speaker_data && AUDIO_CHN_NUM_SPK == 1)
            {
                audio_frame_spk.data = (FH_UINT8 *)speaker_data->data;
                audio_frame_spk.len = speaker_data->uac_frame_size;
                FH_AC_AO_SendFrame(&audio_frame_spk);
                fh_uac_spkstream_enqueue(speaker_data);
            }
            else
            {
                usleep(1000);
            }
        }
        else
        {
            usleep(1000);
        }
    }

    return NULL;
}

#endif

static struct uac_stream_ops uac_ops = {
    .mic_sample = audio_sample_change,
    .mic_mute = audio_set_mute,
    .mic_volume = DSP_Audio_Codec_SetMicGain,
#ifdef ENABLE_SPEAKER_DESC
    .spk_sample = speaker_sample_change,
    .spk_mute = speaker_mute_set,
    .spk_volume = DSP_Audio_Codec_SetSpeakerGain,
#endif

};

int uac_init(void)
{
    pthread_attr_t attr;
    struct sched_param param;
    int ret;

    if (!g_uac_th_status)
        g_uac_th_status = 1;
    else
    {
        g_uac_th_status = 0;
        return 0;
    }

    ret = uac_audio_init();
    if (ret)
    {
        printf("uac_audio_init fail\n");
        return -1;
    }

    fh_uac_ops_register(&uac_ops);
    fh_uac_init();

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setstacksize(&attr, 4 * 1024);
    param.sched_priority = 20;
    pthread_attr_setschedparam(&attr, &param);

    if (pthread_create(&g_thread_audio_stream, &attr, audio_stream_proc, NULL) != 0)
    {
        printf("Error: Create tcp_dbi_thread thread failed!\n");
    }

#ifdef ENABLE_SPEAKER_DESC
    uac_speaker_init();

    if (pthread_create(&g_thread_speaker_stream, &attr, speaker_stream_proc, NULL) != 0)
    {
        printf("Error: Create tcp_dbi_thread thread failed!\n");
    }
#endif
    return 0;
}
