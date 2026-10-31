#ifndef __uvc_init_h__
#define __uvc_init_h__

/* 可配置uvc buf数量和大小,当内存紧张时可适当减少UVC_BUF_NUM和UVC_BUF_SIZE */
/* UVC_BUF_NUM >= 4， UVC_BUF_SIZE >= (ISOC_CNT_ON_MICROFRAME * UVC_PACKET_SIZE) */
#ifdef FH_RT_UVC_EP_TYPE_ISOC
#define UVC_BUF_SIZE (99 * ISOC_CNT_ON_MICROFRAME * UVC_PACKET_SIZE)
#define UVC_BUF_NUM  (10)

/* 动态带宽配置，根据幅面和格式的不同选择不同带宽传输 */
/* 注意： 所有可选配置端点大小最好全部符合UVC_BUF_SIZE % UVC_PACKET_SIZE_ALTx == 0*/
#ifdef ISOC_HIGH_BANDWIDTH_EP
#define UVC_PACKET_SIZE_ALT1 (UVC_PACKET_SIZE * ISOC_CNT_ON_MICROFRAME)
#define UVC_PACKET_SIZE_ALT2 (((UVC_PACKET_SIZE/8) * 6) * ISOC_CNT_ON_MICROFRAME)
#define UVC_PACKET_SIZE_ALT3 (((UVC_PACKET_SIZE/8) * 4) * ISOC_CNT_ON_MICROFRAME)
#else
#define UVC_PACKET_SIZE_ALT1 (UVC_PACKET_SIZE)
#define UVC_PACKET_SIZE_ALT2 (UVC_PACKET_SIZE)
#define UVC_PACKET_SIZE_ALT3 (UVC_PACKET_SIZE)
#endif
#define UVC_PACKET_SIZE_ALT4 (UVC_PACKET_SIZE)
#define UVC_PACKET_SIZE_ALT5 ((UVC_PACKET_SIZE/8) * 6)
#define UVC_PACKET_SIZE_ALT6 (UVC_PACKET_SIZE/2)
#define UVC_PACKET_SIZE_ALT7 (UVC_PACKET_SIZE/4)
#define UVC_PACKET_SIZE_ALT8 (UVC_PACKET_SIZE/8)

#else   /* FH_RT_UVC_EP_TYPE_BULK */
#define UVC_BUF_SIZE (UVC_PACKET_SIZE * UVC_BULK_PAYLOAD_PACKET_NUM)
#define UVC_BUF_NUM  (10)
#endif  /* FH_RT_UVC_EP_TYPE_ISOC */

/*  Four-character-code (FOURCC) */
#define v4l2_fourcc(a, b, c, d)\
    ((unsigned int)(a) | ((unsigned int)(b) << 8) | ((unsigned int)(c) << 16) | ((unsigned int)(d) << 24))

typedef enum
{
    V4L2_PIX_FMT_NV12   = v4l2_fourcc('N', 'V', '1', '2'),
    V4L2_PIX_FMT_YUY2   = v4l2_fourcc('Y', 'U', 'Y', '2'),
    V4L2_PIX_FMT_MJPEG  = v4l2_fourcc('M', 'J', 'P', 'G'),
    V4L2_PIX_FMT_H264   = v4l2_fourcc('H', '2', '6', '4'),
    V4L2_PIX_FMT_H265   = v4l2_fourcc('H', '2', '6', '5'),
    V4L2_PIX_FMT_IR     = v4l2_fourcc('Y', 'U', 'V', '8'),
    V4L2_PIX_FMT_reserved2   = v4l2_fourcc('0', '0', '0', '0')
} UVC_FORMAT;

/*
 *功能:uvc 自定义扩展功能请求的数据结构
 *length: 请求输入或输出的数据长度;
 *data: uvc控制请求输入或输出的数据buf
 */
struct uvc_request_data
{
    int length;
    unsigned char data[60];
};

/*
 *功能:uvc 版本信息数据结构
 *size: 有效数据长度, 有效数据不包含size和crc字段,= sizeof(struct UvcInfo) - 8;
 *crc: 有效数据crc校验值；
 *uvcinfo_change: 标志位, 1：flash有uvc版本信息，0：flash中尚未保存uvc信息；
 *other: uvc设备的产品和版本信息。
 */
struct UvcInfo
{
    unsigned int size;
    unsigned int crc;
    unsigned int uvcinfo_change;

    union _Info
    {
        struct InfoData
        {
            unsigned char data[1012];
        } infoData;

        struct InfoVal
        {
            unsigned int vid;
            unsigned int pid;
            unsigned int bcd;
            char szFacture[32];
            char szProduct[32];
            char szSerial[32];
            char szVideoIntf[32];
            char szMicIntf[32];
        } infoVal;
    } info;
} __packed;

/***** Not Modifiable *****/
#define UVC_STREAM_ON   2
#define UVC_STREAM_OFF   0

#define STREAM_ID1      0
#define STREAM_ID2      1

#define EXTERN_DATA_LEN    0x0b
/***********************/

/**
 * uvc_camera_terminal_descriptor Controls:
 * CAMERA_TERMINAL_CONTROLS
 * D0: Scanning Mode
 * D1: Auto-Exposure Mode
 * D2: Auto-Exposure Priority
 * D3: Exposure Time (Absolute)
 * D4: Exposure Time (Relative)
 * D5: Focus (Absolute)
 * D6 : Focus (Relative)
 * D7: Iris (Absolute)
 * D8 : Iris (Relative)
 * D9: Zoom (Absolute)
 * D10: Zoom (Relative)
 * D11: PanTilt (Absolute)
 * D12: PanTilt (Relative)
 * D13: Roll (Absolute)
 * D14: Roll (Relative)
 * D15: Reserved
 * D16: Reserved
 * D17: Focus, Auto
 * D18: Privacy
 * D19..(n*8-1): Reserved, set to zero
 */
#define CAMERA_TERMINAL_CONTROLS (0x00020e)
/* BIT(0) ~ BIT(3) */
#define AEMODE_DEF       2
#define AEMODE_BITMAP    0x0f
/* 0x0 ~ 0xffffffff*/
#define EXPOSURE_MAX     1047
#define EXPOSURE_MIN     3
#define EXPOSURE_DEF     250
/* 0x0 ~ 0xffff*/
#define IRIS_MAX     100
#define IRIS_MIN     0
#define IRIS_DEF     50
/* 0/1 */
#define FOCUSMODE_DEF   0
/* 0x0 ~ 0xffff */
#define FOCUS_MAX       255
#define FOCUS_MIN       0
#define FOCUS_DEF       0
/* 0x0 ~ 0xffff */
#define ZOOMABS_MAX     500
#define ZOOMABS_MIN     100
#define ZOOMABS_DEF     100
/* -128 ~ 128 */
#define PANABS_MAX     10
#define PANABS_MIN     -10
#define PANABS_DEF     0
/* -128 ~ 128 */
#define TILTABS_MAX     10
#define TILTABS_MIN     -10
#define TILTABS_DEF     0
/* -180 ~ 180 */
#define ROLLABS_MAX     180
#define ROLLABS_MIN     -180
#define ROLLABS_DEF     0

/************************************/
/**
 * uvc_processing_unit_descriptor Controls:
 * PROCESSING_UNIT_CONTROLS
 * D0: Brightness
 * D1: Contrast
 * D2: Hue
 * D3: Saturation
 * D4: Sharpness
 * D5: Gamma
 * D6: White Balance Temperature
 * D7: White Balance Component
 * D8: Backlight Compensation
 * D9: Gain
 * D10: Power Line Frequency
 * D11: Hue, Auto
 * D12: White Balance Temperature, Auto
 * D13: White Balance Component, Auto
 * D14: Digital Multiplier
 * D15: Digital Multiplier Limit
 * D16: Analog Video Standard
 * D17: Analog Video Lock Status
 * D18..(n*8-1): Reserved. Set to zero.
 */
#define PROCESSING_UNIT_CONTROLS (0x165b)
/* UVC视频效果参数配置，包括最大值、最小值和默认值 */
/* PROCESSING_UNIT_CONTROLS PARAM*/
/* 0x0 ~ 0xffff */
#define GAIN_MAX         255
#define GAIN_MIN         0
#define GAIN_DEF         0
/* 0x0 ~ 0xffff */
#define BIGHTNESS_MAX    255
#define BIGHTNESS_MIN    0
#define BIGHTNESS_DEF    128
/* 0x0 ~ 0xffff */
#define CONTRAST_MAX     255
#define CONTRAST_MIN     0
#define CONTRAST_DEF     128
/* 0x0 ~ 0xffff */
#define SATURATION_MAX   255
#define SATURATION_MIN   0
#define SATURATION_DEF   128
/* 0x0 ~ 0xffff */
#define SHARPENESS_MAX   255
#define SHARPENESS_MIN   0
#define SHARPENESS_DEF   128
/* 0/1 */
#define AWBMODE_DEF      1
/* 0x0 ~ 0xffff */
#define AWBGAIN_MAX      7500
#define AWBGAIN_MIN      2000
#define AWBGAIN_DEF      4000
/* 0x0 ~ 0xffff */
#define GAMMACFG_MAX     255
#define GAMMACFG_MIN     0
#define GAMMACFG_DEF     128
/* 0x0 ~ 0xffff */
#define BACKLIGHT_MAX    1
#define BACKLIGHT_MIN    0
#define BACKLIGHT_DEF    1
/* 0/1 */
#define HUEAUTO_DEF      1
/* 0x0 ~ 0xffff */
#define HUEGAIN_MAX      0
#define HUEGAIN_MIN      0
#define HUEGAIN_DEF      0
/************************************/

#define EXTENSION_UNIT_CONTROLS (0x3fffff)

#ifdef UVC_SUPPORT_WINDOWS_HELLO_FACE
#define MSXU_EXTENSION_UNIT_CONTROLS (0x0120)
#else
#define MSXU_EXTENSION_UNIT_CONTROLS (0x0)
#endif

/* USER PARA ID */
#define USER_PARAM_GAIN         0
#define USER_PARAM_BRIGHTNESS   1
#define USER_PARAM_CONTRAST     2
#define USER_PARAM_SATURATION   3
#define USER_PARAM_SHARPENESS   4
#define USER_PARAM_AWBGAIN      5
#define USER_PARAM_AWBMODE      6
#define USER_PARAM_BACKLIGHT    7
#define USER_PARAM_HUEAUTO      8
#define USER_PARAM_HUEGAIN      9
#define USER_PARAM_POWERLINE    10
#define USER_PARAM_AEMODE       11
#define USER_PARAM_EXPOSURE     12
#define USER_PARAM_AE_PRI       13
#define USER_PARAM_GAMMACFG     14
#define USER_PARAM_FOCUSMODE    15
#define USER_PARAM_FOCUSABS     16
#define USER_PARAM_ZOOMABS      17
#define USER_PARAM_PANABS       18
#define USER_PARAM_TILTABS      19
#define USER_PARAM_ROLLABS      20
#define USER_PARAM_IRIS         21

#define USER_PARAM_SUPPORT_CNT  40
#define USER_PARAM_NO_CHANGE_FLAG  -1001
/*
 *功能:uvc驱动操作函数结构表
 *format_change: 格式配置操作函数，用于改变视频格式和幅面参数；
 */
struct uvc_stream_ops
{
    void (*format_change)(int stream_id, UVC_FORMAT fmt, int w, int h, int fps);
    void (*stream_on_callback)(int stream_id);
    void (*stream_off_callback)(int stream_id);
    void (*uvc_extern_data)(unsigned char *data, unsigned int data_size, unsigned int cs);
    void (*uvc_extern_intr)(unsigned char req, unsigned char cs, struct uvc_request_data *resp);
    struct UvcInfo *(*uvc_vision)(void);
    int *(*uvc_uspara)(void);

    /* PROCESSING_UNIT_CONTROLS callback func */
    void (*SetGain)(unsigned int gain_level);      /* 增益 */
    void (*SetBrightness)(unsigned int value);     /* 亮度 */
    void (*SetContrast)(unsigned int value);       /* 对比度 */
    void (*SetSaturation)(int value);              /* 饱和度 */
    void (*SetSharpeness)(int value);              /* 清晰度 */
    void (*SetAWBGain)(unsigned int value);        /* 白平衡 */
    void (*SetAwbMode)(unsigned int value);        /* 白平衡0: 手动, 1: 自动 */
    void (*SetGammaCfg)(unsigned int value);       /* 伽马 */
    void (*SetBacklight)(unsigned int value);      /* 逆光对比 */
    void (*SetHueAuto)(unsigned int value);        /* 色调0: 手动, 1: 自动 */
    void (*SetHueGain)(unsigned int value);        /* 色调 */
    void (*SetPowerLine)(unsigned int value);      /* 电力线1:50hz, 2:60hz*/

    /* CAMERA_TERMINAL_CONTROLS callback func */
    void (*SetAEMode)(unsigned int mode);          /* 1:曝光手动,光圈手动; */
                                                   /* 2:曝光自动,光圈自动; */
                                                   /* 4:曝光手动,光圈自动; */
                                                   /* 8:曝光自动,光圈手动 */
    void (*SetExposure)(unsigned int time_value);  /* 曝光时间 */
    void (*SetIris)(unsigned int value);           /* 光圈 */
    void (*SetAe_Priority)(unsigned int value);    /* 低亮度补偿:enable/disable */
    void (*SetFocusMode)(unsigned int value);      /* 焦点模式 1: 自动, 0: 手动*/
    void (*SetFocusAbs)(unsigned int value);       /* 焦点 */
    void (*SetZoomAbs)(unsigned int value);        /* 缩放 */
    void (*SetPanTiltAbs)(int pan, int tilt);      /* 全景&倾斜 */
    void (*SetRollAbs)(int value);                 /* 滚动 */
    int reserved[12];
};


/*
 *struct uvc_format_info
 *width: 分辨率宽度
 *height: 分辨率高度
 *intervals: 该分辨率支持的帧率, 最多支持8种
 */
struct uvc_frame_info
{
    unsigned int width;
    unsigned int height;
    unsigned int intervals[8];
    unsigned int alt_set;
};

/*
 *struct uvc_format_info
 *fcc: uvc视频格式,enum UVC_FORMAT中的一种
 *frames: 幅面结构体struct uvc_frame_info数组的指针。
 */
struct uvc_format_info
{
    unsigned int fcc;
    const struct uvc_frame_info *frames;
    unsigned int defaultindex;
};

/*
 *功能:获取uvc stream驱动状态
 *steram_id: 双uvc码流时用于码流选择
 *          STREAM_ID1: 码流1
 *          STREAM_ID2: 码流2
 *return: 2-stream on; 0-stream off
 *注意: host端操作会改变uvc stream status
 */
int fh_uvc_status_get(int steram_id);


/*
 *功能:注册操作函数
 *ops: 操作函数结构体指针struct uvc_stream_ops；
 */
void fh_uvc_ops_register(struct uvc_stream_ops *ops);

/*
 *功能:初始化uvc驱动。
 *steram_id: 双uvc码流时用于码流选择
 *          STREAM_ID1: 码流1
 *          STREAM_ID2: 码流2
 */
void fh_uvc_init(int steram_id, struct uvc_format_info *fmt, int fmt_num);

/*
 *功能:将视频数据发送到usb端点。
 *steram_id: 双uvc码流时用于码流选择
 *          STREAM_ID1: 码流1
 *          STREAM_ID2: 码流2
 *img_data: 视频数据缓存区首地址
 *img_size: 视频数据长度
 */
void fh_uvc_stream_enqueue(int steram_id, uint8_t *img_data, uint32_t img_size);

/*
 *功能:将视频的时间戳传到驱动中。
 *steram_id: 双uvc码流时用于码流选择
 *          STREAM_ID1: 码流1
 *          STREAM_ID2: 码流2
 *pts: stream码流的时间戳
 */
void fh_uvc_stream_pts(int stream_id, uint64_t pts);

#endif

