#ifndef __FH_VOU_MPIPARA_H__
#define __FH_VOU_MPIPARA_H__

#include "fh_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/**********************************************************************
	PART I		- Macro
**********************************************************************/

#define VO_DEF_CHN_BUF_LEN		8
#define VO_DEF_DISP_BUF_LEN		5
#define VO_DEF_VIRT_BUF_LEN		3

/* vo inteface type */
#define VO_INTF_CVBS     (0x01L<<0)
#define VO_INTF_YPBPR    (0x01L<<1)
#define VO_INTF_VGA      (0x01L<<2)
#define VO_INTF_BT656    (0x01L<<3)
#define VO_INTF_BT1120   (0x01L<<4)
#define VO_INTF_HDMI     (0x01L<<5)
#define VO_INTF_LCD      (0x01L<<6)
#define VO_INTF_BT656_H  (0x01L<<7)
#define VO_INTF_BT656_L  (0x01L<<8)


#define VO_DEFAULT_CHN      -1          /* use vo buffer as pip buffer */

/**********************************************************************
	PART II		- Structure and Enumeration
**********************************************************************/
typedef FH_SINT32 VO_INTF_TYPE_E;
typedef FH_SINT32 JPEGE_CHN;
typedef FH_SINT32 VDEC_CHN;
typedef FH_SINT32 VO_DEV;
typedef FH_SINT32 VO_LAYER;
typedef FH_SINT32 VO_CHN;
typedef FH_SINT32 VO_WBC;
typedef FH_SINT32 GRAPHIC_LAYER;
typedef FH_SINT32 VO_GRP;
typedef FH_SINT32 VGS_CHN;
typedef FH_SINT32 VGS_PTH;

typedef enum
{
    PIC_QCIF = 0,
    PIC_CIF,
    PIC_2CIF,
    PIC_HD1,
    PIC_D1,
    PIC_960H,
    PIC_1280H,
    PIC_1440H,

    PIC_QVGA,    /* 320 * 240 */
    PIC_VGA,     /* 640 * 480 */
    PIC_XGA,     /* 1024 * 768 */
    PIC_SXGA,    /* 1400 * 1050 */
    PIC_UXGA,    /* 1600 * 1200 */
    PIC_QXGA,    /* 2048 * 1536 */

    PIC_WVGA,    /* 854 * 480 */
    PIC_WSXGA,   /* 1680 * 1050 */
    PIC_WUXGA,   /* 1920 * 1200 */
    PIC_WQXGA,   /* 2560 * 1600 */

    PIC_HD720,   /* 1280 * 720 */
    PIC_HD1080,  /* 1920 * 1080 */
    PIC_qHD,     /*960 * 540*/

    PIC_UHD2K,   /* 2560 * 1440 */
    PIC_UHD4K,   /* 3840*2160 */

    PIC_1080LITE,/* 960*1080 */
    PIC_BUTT
}PIC_SIZE_E;

typedef enum
{
    VIDEO_ENCODING_MODE_PAL=0,
    VIDEO_ENCODING_MODE_NTSC,
    VIDEO_ENCODING_MODE_AUTO,
    VIDEO_ENCODING_MODE_BUTT
} VIDEO_NORM_E;

typedef enum
{
    PIXEL_FORMAT_RGB_1BPP = 0,
    PIXEL_FORMAT_RGB_2BPP,
    PIXEL_FORMAT_RGB_4BPP,
    PIXEL_FORMAT_RGB_8BPP,
    PIXEL_FORMAT_RGB_444,

    PIXEL_FORMAT_RGB_4444,
    PIXEL_FORMAT_RGB_555,
    PIXEL_FORMAT_RGB_565,
    PIXEL_FORMAT_RGB_1555,

    /*  9 reserved */
    PIXEL_FORMAT_RGB_888,
    PIXEL_FORMAT_RGB_8888,

    PIXEL_FORMAT_RGB_PLANAR_888,
    PIXEL_FORMAT_RGB_BAYER_8BPP,
    PIXEL_FORMAT_RGB_BAYER_10BPP,
    PIXEL_FORMAT_RGB_BAYER_12BPP,
    PIXEL_FORMAT_RGB_BAYER_14BPP,

    PIXEL_FORMAT_RGB_BAYER,         /* 16 bpp */

    PIXEL_FORMAT_YUV_A422,
    PIXEL_FORMAT_YUV_A444,

    PIXEL_FORMAT_YUV_PLANAR_422,
    PIXEL_FORMAT_YUV_PLANAR_420,

    PIXEL_FORMAT_YUV_PLANAR_444,

    PIXEL_FORMAT_YUV_SEMIPLANAR_422,    // 22
    PIXEL_FORMAT_YUV_SEMIPLANAR_420,    // 23
    PIXEL_FORMAT_YUV_SEMIPLANAR_444,

    PIXEL_FORMAT_UYVY_PACKAGE_422,
    PIXEL_FORMAT_YUYV_PACKAGE_422,
    PIXEL_FORMAT_VYUY_PACKAGE_422,
    PIXEL_FORMAT_YCbCr_PLANAR,

    PIXEL_FORMAT_YUV_400,

    /******FY add begin********/
    PIXEL_FORMAT_YUV_422_10BPP=43,
    PIXEL_FORMAT_YUV_420_BLOCK,     //<yuv420 block for venc

    PIXEL_FORMAT_YUV_422_MIRROR,    //only support 1D compress mode.

    PIXEL_FORMAT_YUV_FIELD_422,

    PIXEL_FORMAT_YVU_SEMIPLANAR_420,	// NV21

    /******FY add end*********/

    PIXEL_FORMAT_BUTT
} PIXEL_FORMAT_E;


typedef enum
{
    VIDEO_FIELD_TOP         = 0x1,    /* even field */
    VIDEO_FIELD_BOTTOM      = 0x2,    /* odd field */
    VIDEO_FIELD_INTERLACED  = 0x3,    /* two interlaced fields */
    VIDEO_FIELD_FRAME       = 0x4,    /* frame */

    VIDEO_FIELD_BUTT
} VIDEO_FIELD_E;

typedef enum
{
    VIDEO_FORMAT_LINEAR     = 0x0,      /* nature video line */
    VIDEO_FORMAT_TILE       = 0x1,      /* tile cell: 256pixel x 16line, default tile mode */
    VIDEO_FORMAT_TILE64     = 0x2,      /* tile cell: 64pixel x 16line */
    VIDEO_FORMAT_BLOCK      = 0x3,      /* macro block */
    VIDEO_FORMAT_BUTT
} VIDEO_FORMAT_E;

typedef enum
{
    COMPRESS_MODE_NONE      = 0x0,      /* no compress */
    COMPRESS_MODE_SEG       = 0x1,      /* compress unit is 256 bytes as a segment, default seg mode */
    COMPRESS_MODE_SEG128    = 0x2,      /* compress unit is 128 bytes as a segment */
    COMPRESS_MODE_LINE      = 0x3,      /* compress unit is the whole line */
    COMPRESS_MODE_FRAME     = 0x4,      /* compress unit is the whole frame */

    /*FY add, compress type 1D,match VI:VI_EC_LEVEL_E*/
    COMPRESS_MODE_128byte   = 0x5,  /*compress rate 50%*/
    COMPRESS_MODE_160byte   = 0x6,  /*compress rate 63%*/
    COMPRESS_MODE_192byte   = 0x7,  /*compress rate 75%*/

    /*FY add, compress type TILE,*/
    COMPRESS_MODE_TILE_192  = 0x8,

    /*FY add, compress type SLICE,*/
    COMPRESS_MODE_SLICE     = 0x9,

    COMPRESS_MODE_CF10      = 0xa,
    COMPRESS_MODE_FBC       = 0xb,

    /*FY add, compress type TILE,*/
    COMPRESS_MODE_TILE_224  = 0xc,
    COMPRESS_MODE_TILE_256  = 0xd,

    COMPRESS_MODE_MFBC      = 0xe,

    COMPRESS_MODE_BUTT
} COMPRESS_MODE_E;

typedef enum
{
    FRAME_FLASH_OFF  = 0,
    FRAME_FLASH_ON   = 1,
    FRAME_FLASH_BUTT,
}FRAME_FLASH_TYPE_E;

typedef struct
{
    FH_UINT32   u32JpegDcfPhyAddr;
    FH_VOID *pJpegDcfVirAddr;

    FRAME_FLASH_TYPE_E enFlashType;
}VIDEO_SUPPLEMENT_S;

typedef struct
{
    FH_UINT32          u32Width;
    FH_UINT32          u32Height;
    VIDEO_FIELD_E   u32Field;
    PIXEL_FORMAT_E  enPixelFormat;

    VIDEO_FORMAT_E  enVideoFormat;
    COMPRESS_MODE_E enCompressMode;

    FH_UINT32          u32PhyAddr[3];
    FH_VOID         *pVirAddr[3];
    FH_UINT32          u32Stride[3];

    FH_UINT32          u32HeaderPhyAddr[3];
    FH_VOID         *pHeaderVirAddr[3];
    FH_UINT32          u32HeaderStride[3];

    FH_SINT16          s16OffsetTop;       /* top offset of show area */
    FH_SINT16          s16OffsetBottom;    /* bottom offset of show area */
    FH_SINT16          s16OffsetLeft;      /* left offset of show area */
    FH_SINT16          s16OffsetRight;     /* right offset of show area */

    FH_UINT64          u64pts;
    FH_UINT32          u32TimeRef;

    FH_UINT32          u32PrivateData;
    VIDEO_SUPPLEMENT_S stSupplement;
}VIDEO_FRAME_S;

typedef struct
{
    VIDEO_FRAME_S stVFrame;
    FH_UINT32 u32PoolId;
} VIDEO_FRAME_INFO_S;

typedef enum
{
    VO_OUTPUT_PAL = 0,
    VO_OUTPUT_NTSC,
    VO_OUTPUT_960H_PAL,              /* ITU-R BT.1302 960 x 576 at 50 Hz (interlaced)*/
    VO_OUTPUT_960H_NTSC,             /* ITU-R BT.1302 960 x 480 at 60 Hz (interlaced)*/

    VO_OUTPUT_1080P24,
    VO_OUTPUT_1080P25,
    VO_OUTPUT_1080P30,

    VO_OUTPUT_720P25,
    VO_OUTPUT_720P30,
    VO_OUTPUT_720P50,
    VO_OUTPUT_720P60,
    VO_OUTPUT_1080I50,
    VO_OUTPUT_1080I60,
    VO_OUTPUT_1080P50,
    VO_OUTPUT_1080P60,

    VO_OUTPUT_576P50,
    VO_OUTPUT_480P60,

	VO_OUTPUT_480x800_60,
    VO_OUTPUT_640x480_60,            /* VESA 640 x 480 at 60 Hz (non-interlaced) CVT */
    VO_OUTPUT_800x480_60,
    VO_OUTPUT_800x600_60,            /* VESA 800 x 600 at 60 Hz (non-interlaced) */
    VO_OUTPUT_800x1280_60,
    VO_OUTPUT_1024x768_60,           /* VESA 1024 x 768 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1024x600_60,
    VO_OUTPUT_1280x1024_60,          /* VESA 1280 x 1024 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1366x768_60,           /* VESA 1366 x 768 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1440x900_60,           /* VESA 1440 x 900 at 60 Hz (non-interlaced) CVT Compliant */
    VO_OUTPUT_1280x800_60,           /* 1280*800@60Hz VGA@60Hz*/
    VO_OUTPUT_1680x1050_60,          /* VESA 1680 x 1050 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1920x2160_30,          /* 1920x2160_30 */
    VO_OUTPUT_1600x1200_60,          /* VESA 1600 x 1200 at 60 Hz (non-interlaced) */
    VO_OUTPUT_1920x1200_60,          /* VESA 1920 x 1600 at 60 Hz (non-interlaced) CVT (Reduced Blanking)*/
    VO_OUTPUT_2560x1440_30,          /* 2560x1440_30 */
    VO_OUTPUT_2560x1440_60,          /* 2560x1440_60 */
    VO_OUTPUT_2560x1600_60,          /* 2560x1600_60 */
    VO_OUTPUT_3840x2160_25,          /* 3840x2160_25 */
    VO_OUTPUT_3840x2160_30,          /* 3840x2160_30 */
    VO_OUTPUT_3840x2160_50,          /* 3840x2160_50 */
    VO_OUTPUT_3840x2160_60,          /* 3840x2160_60 */
    VO_OUTPUT_4096x2160_30,          /* 4096x2160_30 */
    VO_OUTPUT_USER,
    VO_OUTPUT_BUTT

} VO_INTF_SYNC_E;

typedef enum
{
    VO_SAMPLE_RATE_UNKNOWN	= 0,	/* unknown sample rate	*/
    VO_SAMPLE_RATE_8K	= 8000,		/* 8K sample rate	*/
    VO_SAMPLE_RATE_11K	= 11025,	/* 11.025K sample rate	*/
    VO_SAMPLE_RATE_12K	= 12000,	/* 12K sample rate	*/
    VO_SAMPLE_RATE_16K	= 16000,	/* 16K sample rate	*/
    VO_SAMPLE_RATE_22K	= 22050,	/* 22.050K sample rate	*/
    VO_SAMPLE_RATE_24K	= 24000,	/* 24K sample rate	*/
    VO_SAMPLE_RATE_32K	= 32000,	/* 32K sample rate	*/
    VO_SAMPLE_RATE_44K	= 44100,	/* 44.1K sample rate	*/
    VO_SAMPLE_RATE_48K	= 48000,	/* 48K sample rate	*/
    VO_SAMPLE_RATE_88K	= 88200,	/* 88.2K sample rate	*/
    VO_SAMPLE_RATE_96K	= 96000,	/* 96K sample rate	*/
    VO_SAMPLE_RATE_176K	= 176400,	/* 176K sample rate	*/
    VO_SAMPLE_RATE_192K	= 192000,	/* 192K sample rate	*/
    VO_SAMPLE_RATE_BUTT
} VO_SAMPLE_RATE_E;

typedef enum
{
    VO_FIELD_TOP,                 /* top field*/
    VO_FIELD_BOTTOM,              /* bottom field*/
    VO_FIELD_BOTH,                /* top and bottom field*/
    VO_FIELD_BUTT
} VO_DISPLAY_FIELD_E;

typedef enum
{
    VOU_ZOOM_IN_RECT = 0,       /* zoom in by rect */
    VOU_ZOOM_IN_RATIO,          /* zoom in by ratio */
    VOU_ZOOM_IN_BUTT
} VOU_ZOOM_IN_E;

typedef enum
{
    ASPECT_RATIO_NONE   = 0,        /* full screen */
    ASPECT_RATIO_AUTO   = 1,        /* ratio no change, 1:1*/
    ASPECT_RATIO_MANUAL = 2,        /* ratio manual set */
    ASPECT_RATIO_BUTT
}ASPECT_RATIO_MODE_E;

typedef struct
{
    FH_UINT32  u32Priority;                /* video out overlay pri sd */
    FH_AREA  stRect;                     /* rect of video out chn */
    FH_BOOL bDeflicker;                 /* deflicker or not sd */
}VO_CHN_ATTR_S;

typedef struct
{
    ASPECT_RATIO_MODE_E enMode;          /* aspect ratio mode: none/auto/manual */
    FH_UINT32         u32BgColor;      /* background color, RGB 888 */
    FH_AREA         stVideoRect;     /* valid in ASPECT_RATIO_MANUAL mode */
}FH_ASPECT_RATIO;

typedef struct
{
    FH_ASPECT_RATIO stAspectRatio;       /* aspect ratio */
}VO_CHN_PARAM_S;

typedef struct
{
    FH_UINT32 u32TopWidth;            /* top border weight, in pixel*/
    FH_UINT32 u32BottomWidth;         /* bottom border weight, in pixel*/
    FH_UINT32 u32LeftWidth;           /* left border weight, in pixel*/
    FH_UINT32 u32RightWidth;          /* right border weight, in pixel*/
    FH_UINT32 u32Color;               /* border color, RGB888*/
}FH_BORDER;

typedef struct
{
    FH_BOOL bBorderEn;                   /*do Frame or not*/
    FH_BORDER stBorder;
}VO_BORDER_S;

typedef struct
{
    FH_BOOL  bSynm;     /* sync mode(0:timing,as BT.656; 1:signal,as LCD) */
    FH_BOOL  bIop;      /* interlaced or progressive display(0:i; 1:p) */
    FH_UINT8    u8Intfb;   /* interlace bit width while output */

    FH_UINT16   u16Vact ;  /* vertical active area */
    FH_UINT16   u16Vbb;    /* vertical back blank porch */
    FH_UINT16   u16Vfb;    /* vertical front blank porch */

    FH_UINT16   u16Hact;   /* herizontal active area */
    FH_UINT16   u16Hbb;    /* herizontal back blank porch */
    FH_UINT16   u16Hfb;    /* herizontal front blank porch */
    FH_UINT16   u16Hmid;   /* bottom herizontal active area */

    FH_UINT16   u16Bvact;  /* bottom vertical active area */
    FH_UINT16   u16Bvbb;   /* bottom vertical back blank porch */
    FH_UINT16   u16Bvfb;   /* bottom vertical front blank porch */

    FH_UINT16   u16Hpw;    /* horizontal pulse width */
    FH_UINT16   u16Vpw;    /* vertical pulse width */

    FH_BOOL  bIdv;      /* inverse data valid of output */
    FH_BOOL  bIhs;      /* inverse horizontal synch signal */
    FH_BOOL  bIvs;      /* inverse vertical synch signal */
} VO_SYNC_INFO_S;

typedef struct
{
    FH_UINT32                   u32BgColor;          /* Background color of a device, in RGB format. */
    VO_INTF_TYPE_E           enIntfType;          /* Type of a VO interface */
    VO_INTF_SYNC_E           enIntfSync;          /* Type of a VO interface timing */
    VO_SYNC_INFO_S           stSyncInfo;          /* Information about VO interface timings */
}VO_PUB_ATTR_S;

typedef enum
{
    VO_PART_MODE_SINGLE	= 0,		/* single partition, which use software to make multi-picture in one hardware cell */
    VO_PART_MODE_MULTI	= 1,		/* muliti partition, each partition is a hardware cell */
    VO_PART_MODE_BUTT

}VO_PART_MODE_E;

typedef struct
{
    FH_AREA stDispRect;                  /* Display resolution */
    FH_SIZE stImageSize;                 /* Canvas size of the video layer */
    FH_UINT32 u32DispFrmRt;                /* Display frame rate */
    PIXEL_FORMAT_E enPixFormat;         /* Pixel format of the video layer */
    FH_BOOL bDoubleFrame;               /* Whether to double frames */
    FH_BOOL bClusterMode;               /* Whether to take Cluster way to use memory*/
} VO_VIDEO_LAYER_ATTR_S;

typedef enum
{
    VOU_LAYER_DDR0 = 0,
    VOU_LAYER_DDR1 = 1,
    VOU_LAYER_DDR_BUTT
}VOU_LAYER_DDR_E;

typedef enum
{
    VO_CSC_MATRIX_IDENTITY = 0,         /* do not change color space */

    VO_CSC_MATRIX_BT601_TO_BT709,       /* change color space from BT.601 to BT.709 */
    VO_CSC_MATRIX_BT709_TO_BT601,       /* change color space from BT.709 to BT.601 */

    VO_CSC_MATRIX_BT601_TO_RGB_PC,      /* change color space from BT.601 to RGB */
    VO_CSC_MATRIX_BT709_TO_RGB_PC,      /* change color space from BT.709 to RGB */

    VO_CSC_MATRIX_RGB_TO_BT601_PC,      /* change color space from RGB to BT.601 */
    VO_CSC_MATRIX_RGB_TO_BT709_PC,      /* change color space from RGB to BT.709 */

    VO_CSC_MATRIX_BUTT
} VO_CSC_MATRIX_E;

typedef struct
{
    FH_UINT32 u32XRatio;
    FH_UINT32 u32YRatio;
    FH_UINT32 u32WRatio;
    FH_UINT32 u32HRatio;
} VO_ZOOM_RATIO_S;

typedef struct
{
    VOU_ZOOM_IN_E   enZoomType;         /* choose the type of zoom in */
	union {
        FH_AREA          stZoomRect;     /* zoom in by rect */
        VO_ZOOM_RATIO_S stZoomRatio;    /* zoom in by ratio */
    };
} VO_ZOOM_ATTR_S;

typedef struct
{
    FH_BOOL bAlphaEnable;   /**<  alpha enable flag */
    FH_BOOL bAlphaChannel;  /**<  alpha channel enable flag */
    FH_UINT8 u8Alpha0;         /**<  alpha0 value, used in ARGB1555 */
    FH_UINT8 u8Alpha1;         /**<  alpha1 value, used in ARGB1555 */
    FH_UINT8 u8GlobalAlpha;    /**<  global alpha value */
    FH_UINT8 u8Reserved;
} ALPHA_S;

typedef struct
{
    VO_CSC_MATRIX_E enCscMatrix;
    FH_UINT32 u32Luma;                     /* luminance:   0 ~ 100 default: 50 */
    FH_UINT32 u32Contrast;                 /* contrast :   0 ~ 100 default: 50 */
    FH_UINT32 u32Hue;                      /* hue      :   0 ~ 100 default: 50 */
    FH_UINT32 u32Saturation;               /* saturation:  0 ~ 100 default: 50 */
} VO_CSC_S;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif /* End of #ifndef __FH_VOU_MPIPARA_H__ */

