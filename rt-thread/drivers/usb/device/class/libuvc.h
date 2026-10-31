#ifndef __LIBUVC_H__
#define __LIBUVC_H__

#include <rthw.h>
#include <rtthread.h>
#include <rtservice.h>
#include <rtdevice.h>
#include "../core/usbdevice.h"
#include "ch9.h"
#include "video.h"
#include "uvc_init.h"

#define UVC_HEADER_LEN  (12)
#define UVC_HELLO_FACE_HEADER_LEN  (16)

#ifdef FH_RT_UVC_EP_TYPE_ISOC
#ifdef ISOC_HIGH_BANDWIDTH_EP
/* 1 ~ 3 packet cnt of microframe*/
#define ISOC_CNT_ON_MICROFRAME    3
#else
#define ISOC_CNT_ON_MICROFRAME    1
#endif
#endif

#define UVC_PACKET_SIZE_ALT(n) UVC_PACKET_SIZE_ALT##n

#define USB_TYPE_MASK            (0x03 << 5)
#define USB_TYPE_STANDARD        (0x00 << 5)
#define USB_TYPE_CLASS            (0x01 << 5)
#define USB_TYPE_VENDOR            (0x02 << 5)
#define USB_TYPE_RESERVED        (0x03 << 5)

#define V4L2_EVENT_PRIVATE_START        0x08000000
#define UVC_EVENT_FIRST            (V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_CONNECT        (V4L2_EVENT_PRIVATE_START + 0)
#define UVC_EVENT_DISCONNECT    (V4L2_EVENT_PRIVATE_START + 1)
#define UVC_EVENT_STREAMON        (V4L2_EVENT_PRIVATE_START + 2)
#define UVC_EVENT_STREAMOFF        (V4L2_EVENT_PRIVATE_START + 3)
#define UVC_EVENT_SETUP            (V4L2_EVENT_PRIVATE_START + 4)
#define UVC_EVENT_DATA            (V4L2_EVENT_PRIVATE_START + 5)
#define UVC_EVENT_LAST            (V4L2_EVENT_PRIVATE_START + 5)

/* identification number of Unit or Terminal */
#define UVC_INTERFACE_ID            0
#define UVC_CAMERAL_TERMINAL_ID        1
#define UVC_PROCESSING_UNIT_ID        2
#define UVC_H264_EXTENSION_UNIT_ID    3
#define UVC_OUTPUT_TERMINAL_ID        4
#define UVC_OUTPUT_TERMINAL2_ID        5
#define UVC_MSXU_FACE_EXTENSION_UNIT_ID  6

/* These defines matche with uvc driver bUnitID or bTerminalID field */
#define UVC_CTRL_INTERFACE_ID 0
#define UVC_CTRL_CAMERAL_TERMINAL_ID 1
#define UVC_CTRL_PROCESSING_UNIT_ID 2
#define UVC_CTRL_H264_EXTENSION_UNIT_ID 3
#define UVC_CTRL_OUTPUT_TERMINAL_ID 4
#define UVC_CTRL_OUTPUT_TERMINAL2_ID 5
#define UVC_CTRL_MSXU_FACE_EXTENSION_UNIT_ID  6

#define UVC_INTF_CONTROL        0
#define UVC_INTF_STREAMING        1
#define UVC_INTF_STREAMING2        2

#define YUV_NV12    0
#define YUV_YUY2    1

#define UVC_VS_USER_DEF 0xcc

#define BIT(n)          (0x01u << (n))

#define clamp(val, min, max) ({                 \
        typeof(val) __val = (val);              \
        typeof(min) __min = (min);              \
        typeof(max) __max = (max);              \
        (void) (&__val == &__min);              \
        (void) (&__val == &__max);              \
        __val = __val < __min ? __min : __val;   \
        __val > __max ? __max : __val; })


/* #define UVC_DEBUG */
#ifdef UVC_DEBUG
#define UVC_DBG(fmt, x...) printf("[libuvc]:"fmt, ##x)
#else
#define UVC_DBG(fmt, x...)
#endif

#define UVC_PARAM_FUNC_SETAEMODE        1000
#define UVC_PARAM_FUNC_SETEXPOSURE      1001
#define UVC_PARAM_FUNC_SETGAIN          1002
#define UVC_PARAM_FUNC_GETBRIGHTNESS    1003
#define UVC_PARAM_FUNC_SETBRIGHTNESS    1004
#define UVC_PARAM_FUNC_GETCONTRAST      1005
#define UVC_PARAM_FUNC_SETCONTRAST      1006
#define UVC_PARAM_FUNC_GETSATURATION    1007
#define UVC_PARAM_FUNC_SETSATURATION    1008
#define UVC_PARAM_FUNC_GETSHARPENESS    1009
#define UVC_PARAM_FUNC_SETSHARPENESS    1010
#define UVC_PARAM_FUNC_SETAWBGAIN       1011
#define UVC_PARAM_FUNC_SETAWBMODE       1012
#define UVC_PARAM_FUNC_GETGAMMACFG      1013
#define UVC_PARAM_FUNC_SETGAMMACFG      1014
#define UVC_PARAM_FUNC_CDC_MODE         1015
#define UVC_PARAM_FUNC_EXTERN_DATA      1016
#define UVC_PARAM_FUNC_SETBACKLIGHT     1017
#define UVC_PARAM_FUNC_HUEAUTO          1018
#define UVC_PARAM_FUNC_HUEGAIN          1019
#define UVC_PARAM_FUNC_POWERLINE        1020
#define UVC_PARAM_FUNC_STREAMON         1021
#define UVC_PARAM_FUNC_STREAMOFF        1022
#define UVC_PARAM_FUNC_AE_PRIORITY      1023
#define UVC_PARAM_FUNC_FOCUSMODE        1024
#define UVC_PARAM_FUNC_FOCUSABS         1025
#define UVC_PARAM_FUNC_ZOOMABS          1026
#define UVC_PARAM_FUNC_PANTILTABS       1027
#define UVC_PARAM_FUNC_ROLLABS          1028
#define UVC_PARAM_FUNC_SETIRIS          1029

#define UVC_STATUS_VC_TERMINAL_ID 1
#define UVC_STATUS_VC_PU_ID 2

#define UVC_STATUS_TYPE_RESERVED 0x00
#define UVC_STATUS_TYPE_VC 0x01
#define UVC_STATUS_TYPE_VS 0x02

#define UVC_STATUS_EVENT_CTRL_CHANGE 0x00

#define UVC_STATUS_ATTRI_CONTROL_VAL_CHANGE 0x00
#define UVC_STATUS_ATTRI_CONTROL_INFO_CHANGE 0x01
#define UVC_STATUS_ATTRI_CONTROL_FAILURE_CHANGE 0x02
#define UVC_STATUS_ATTRI_RESERVED 0x03

struct UVC_STATUS_PACKET_FORMAT_VC
{
    uint8_t bStatusType;
    uint8_t bOriginator;
    uint8_t bEvent;
    uint8_t bSelector;
    uint8_t bAttribute;
    uint8_t bValue[8];
};

struct UVC_STATUS_PACKET_FORMAT_VS
{
    uint8_t bStatusType;
    uint8_t bOriginator;
    uint8_t bEvent;
    uint8_t bValue;
};

struct uvc_ctrl_event
{
    int32_t parm1;
    int32_t parm2;
    int32_t parm3;
    int32_t func_num;
};

struct uvc_event
{
    union {
        struct usb_ctrlrequest req;
        struct uvc_request_data data;
    };
};

struct v4l2_event
{
    unsigned int stream_id;
    unsigned int                type;
    union {
        unsigned char            data[64];
    } u;
};

struct uvc_frame_buf
{
    unsigned int id;
    unsigned int uvc_frame_size;
    unsigned char *data;
    rt_list_t list;
};

struct uvc_video_buf
{
    unsigned int packet_size;
    unsigned int current_size;
    unsigned int play_status;
    struct uvc_frame_buf data_buf[UVC_BUF_NUM];
    struct uvc_frame_buf *cur_buf;
    rt_list_t list_free;
    rt_list_t list_used;
    rt_sem_t uvc_next_sem;
    uint32_t fid;
    uint32_t stream_pts;
    uint32_t uvc_fid_last;
    uint8_t *header_buf;
    /* struct uvc_device *uvc; */
};


struct uvc_user_dev_ctrl
{
    unsigned char cmd;
    unsigned char reboot;
    unsigned char coolview;
    unsigned char cdcCmdLine;
};

struct uvc_vs_user_data
{
    __u8  cmd;
    __u8  fmt;
    __u16 width;
    __u16 height;
    __u8  fps;
    __u8  gop;
    __u32 bitRate;
} __attribute__((__packed__));

struct stream_info
{
    int g_s32Format;
    int gFmt;
    int gPicW;
    int gPicH;
    int gFps;
    int gop;
    int bitRate;
    int id;
};

/* guvc object */
struct uvc_control_context
{
    uint8_t cs;
    uint8_t id;
    uint8_t intf;
    uint8_t size;
};

struct uvc_stream_info
{
    struct uvc_streaming_control probe;
    struct uvc_streaming_control commit;
    struct uvc_control_context context;

    unsigned int error_code;

    unsigned int fcc;
    unsigned int width;
    unsigned int height;
    unsigned int iframe;
    unsigned int finterval;
    rt_sem_t uvc_format_sem;
    unsigned int maxsize;
    unsigned char stream_id;
    struct stream_info *stream;
};

struct uvc_fmt_array_info
{
    unsigned int yuv_type;

    unsigned int fmt_num;
    struct uvc_format_info *pFmts;

    unsigned int fmt2_num;
    struct uvc_format_info *pFmts2;

};

struct uvc_control_value
{
    int max;
    int min;
    int cur;
    int def;
    int res; /* resolution -> step */
};

typedef void (*uvc_control_setup_call)(struct uvc_request_data *r); /* device -> host */
typedef void (*uvc_control_data_call)(struct uvc_request_data *r); /* host -> device */

struct uvc_control_ops
{
    uint8_t len;
    struct uvc_control_value val;
    uvc_control_setup_call fsetup;
    uvc_control_data_call fdata;
};

extern void uvc_driver_buf_reinit(int stream_id, int alt_size);
extern void fh_uvc_status_set(int stream_id, int set);
extern void uvc_sem_release(int stream_id);

extern struct uendpoint *video_stream1_ep1;
extern struct uendpoint *video_stream2_ep1;
extern struct uendpoint *video_control_ep;
extern void uvc_copy_descriptors(struct uvc_device *uvc, int other_speed);
uint32_t uvc_camera_terminal_supported(uint8_t cs);

extern void change_usb_support_fmt(struct uvc_device *uvc);
void uvc_video_complete(struct ufunction *func, struct uendpoint *ep, rt_size_t size);
void fh_uvc_fmt_init(int steram_id, struct uvc_format_info *fmt, int fmt_num);

void uvc_fill_streaming_control(struct uvc_stream_info *dev,
                                struct uvc_streaming_control *ctrl,
                                int iframe, int iformat);
struct uvc_request_data *uvc_events_process(int stream_id, void *event, unsigned int type);
void uvc_uspara_load(void);
void uvc_para_init(void);

#endif /* __LIBUVC_H__ */
