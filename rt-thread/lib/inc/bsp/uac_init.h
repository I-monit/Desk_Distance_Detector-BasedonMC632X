#ifndef __UAC_INIT_H__
#define __UAC_INIT_H__


#define UAC_MICBUF_SIZE 2048
#define UAC_MICBUF_NUM  (64)

#define UAC_SPKBUF_SIZE 256
#define UAC_SPKBUF_NUM  (200)

#define UAC_STREAM_ON   1
#define UAC_STREAM_OFF   0

/* value: 0 ~ 100 */
#define UAC_MIC_DEFAULT_VOLUME   75
#define UAC_SPK_DEFAULT_VOLUME   75

#define AUDIO_CHN_NUM_SPK     2 /* 1 or 2 */
#define AUDIO_CHN_NUM_MIC     2 /* 1 or 2 */

/*
 *功能:uac驱动操作函数结构表
 */
struct uac_stream_ops
{
    int (*mic_sample)(unsigned int sample);
    int (*spk_sample)(unsigned int sample);
    int (*mic_mute)(unsigned int flag);     /* flag[IN]: 1: mute; 0: no mute*/
    int (*spk_mute)(unsigned int flag);     /* flag[IN]: 1: mute; 0: no mute*/
    int (*mic_volume)(unsigned int value);  /* sample[IN]: 0 ~ 100 */
    int (*spk_volume)(unsigned int value);  /* sample[IN]: 0 ~ 100 */
    int reserved[10];
};

/*
 *功能:uac buf结构
 */
struct uac_frame_buf
{
    unsigned int uac_frame_size;
    unsigned char *data;
};

/*
 *功能:注册操作函数
 *ops: 操作函数结构体指针struct uac_stream_ops；
 */
void fh_uac_ops_register(struct uac_stream_ops *ops);

/*
 *功能:初始化uac驱动
 */
void fh_uac_init(void);

/*
 *功能:设置uac stream驱动状态
 *set: audio status状态。
 *  #define UAC_STREAM_ON   1
 *  #define UAC_STREAM_OFF   0
 *
 */
void fh_uac_micstatus_set(int set);
void fh_uac_spkstatus_set(int set);

/*
 *功能:获取uac stream驱动状态
 *return: audio status状态UAC_STREAM_ON/UAC_STREAM_OFF。
 */
int fh_uac_micstatus_get(void);
int fh_uac_spkstatus_get(void);


/*
 *功能:将音频数据发送到usb端点。
 *audio_data：音频数据缓存区首地址
 *audio_size：音频数据长度
 */
void fh_uac_micstream_enqueue(uint8_t *audio_data, uint32_t audio_size);

/*
 *功能:从usb端点获取音频数据结构及buf。
 *return: 音频数据结构体
 */
struct uac_frame_buf *fh_uac_spkstream_dequeue(void);

/*
 *功能:将音频buf放回音频管理链表。
 *spk_buf: 音频数据结构地址
 */
void fh_uac_spkstream_enqueue(struct uac_frame_buf *spk_buf);

#endif
