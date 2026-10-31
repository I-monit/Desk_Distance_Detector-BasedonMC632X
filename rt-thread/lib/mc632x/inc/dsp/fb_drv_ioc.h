
#ifndef __FB_DRV_IOC_H__
#define __FB_DRV_IOC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

#ifdef __KERNEL__
//#include <linux/fb.h>
#endif
#include "fh_type.h"
#include "fh_common.h"
#include "fh_vou_mpipara.h"
/*************************** Structure Definition ****************************/

#define IOC_TYPE_FYFB       'F'
/** To obtain the colorkey of an overlay layer */
#define FBIOGET_COLORKEY_FYFB       _IOR(IOC_TYPE_FYFB, 90, FYFB_COLORKEY_S)
/** To set the colorkey of an overlay layer */
#define FBIOPUT_COLORKEY_FYFB       _IOW(IOC_TYPE_FYFB, 91, FYFB_COLORKEY_S)
/** To get the alpha of an overlay layer */
#define FBIOGET_ALPHA_FYFB          _IOR(IOC_TYPE_FYFB, 92, FYFB_ALPHA_S)
/** To set the alpha of an overlay layer */
#define FBIOPUT_ALPHA_FYFB          _IOW(IOC_TYPE_FYFB, 93, FYFB_ALPHA_S)
/** To get the origin of an overlay layer on the screen */
#define FBIOGET_SCREEN_ORIGIN_FYFB  _IOR(IOC_TYPE_FYFB, 94, FYFB_POINT_S)
/** To set the origin of an overlay layer on the screen */
#define FBIOPUT_SCREEN_ORIGIN_FYFB  _IOW(IOC_TYPE_FYFB, 95, FYFB_POINT_S)
/** To obtain the anti-flicker setting of an overlay layer */
#define FBIOGET_DEFLICKER_FYFB       _IOR(IOC_TYPE_FYFB, 98, FYFB_DEFLICKER_S)
/** To set the anti-flicker setting of an overlay layer */
#define FBIOPUT_DEFLICKER_FYFB       _IOW(IOC_TYPE_FYFB, 99, FYFB_DEFLICKER_S)
/** To wait for the vertical blanking region of an overlay layer */
#define FBIOGET_VBLANK_FYFB         _IO(IOC_TYPE_FYFB, 100)
/** To set the display state of an overlay layer */
#define FBIOPUT_SHOW_FYFB           _IOW(IOC_TYPE_FYFB, 101, FY_BOOL)
/** To obtain the display state of an overlay layer */
#define FBIOGET_SHOW_FYFB           _IOR(IOC_TYPE_FYFB, 102, FY_BOOL)
/** to obtain the capability of an overlay layer */
#define FBIOGET_CAPABILITY_FYFB    _IOR(IOC_TYPE_FYFB, 103, FYFB_CAPABILITY_S)
/** set the screen output size */
#define FBIOPUT_SCREENSIZE          _IOW(IOC_TYPE_FYFB, 130, FYFB_SIZE_S*)
/** get the screen output size */
#define FBIOGET_SCREENSIZE          _IOR(IOC_TYPE_FYFB, 131, FYFB_SIZE_S*)
/** To display multiple surfaces in turn and set the alpha and colorkey attributes */
#define FBIOFLIP_SURFACE            _IOW(IOC_TYPE_FYFB, 132, FYFB_SURFACEEX_S)
/**To set the compression function status of an overlay layer*/
#define FBIOPUT_COMPRESSION_FYFB	_IOW(IOC_TYPE_FYFB, 133, FY_BOOL)
/**To obtain the compression function status of an overlay layer*/
#define FBIOGET_COMPRESSION_FYFB	_IOR(IOC_TYPE_FYFB, 134, FY_BOOL)
/**To release the layer*/
#define FBIO_RELEASE_FYFB         _IO(IOC_TYPE_FYFB, 140)


typedef struct fyFB_EXPORT_FUNC_S
{
    FY_S32 (*pfnFrameBufferNotify)(VO_DEV voDev, GRAPHIC_LAYER voLayer, const FY_U32 u32AddrPhy);
} FB_EXPORT_FUNC_S;

typedef struct
{
    FY_U32  u32Width;
    FY_U32  u32Height;
}FYFB_SIZE_S;

#if 0
static inline FY_U8  fyfb_rgb(const struct fb_bitfield* pBit, FY_S32 color)
{
    return ((FY_U8)((((FY_U32)color)>>pBit->offset) << (8-pBit->length)) +
             ((FY_U8)(((FY_U32)(color)>>pBit->offset) << (8-pBit->length)) >> pBit->length));
}

static inline FY_S32 fyfb_color2key(const struct fb_var_screeninfo* pVar, FY_S32 color)
{
   if (pVar->bits_per_pixel <= 8)
   {
       return color;
   }
   else
   {
      FY_U8 r, g, b;
      r = fyfb_rgb(&pVar->red, color);
      g = fyfb_rgb(&pVar->green, color);
      b = fyfb_rgb(&pVar->blue, color);
      return (r<<16) + (g<<8) + b;
   }
}
#endif

typedef struct
{
    FY_BOOL bKeyEnable;         /* colorkey enable flag */
    FY_U32 u32Key;              /* colorkey value, maybe contains alpha */
}FYFB_COLORKEY_S;

typedef struct
{
    FY_S32 x;
    FY_S32 y;
    FY_S32 w;
    FY_S32 h;
} FYFB_RECT;

typedef struct
{
    FY_S32 s32XPos;         /**<  horizontal position */
    FY_S32 s32YPos;         /**<  vertical position */
}FYFB_POINT_S;

typedef struct fyFB_DEFLICKER_S
{
    FY_U32  u32HDfLevel;    /**<  horizontal deflicker level */
    FY_U32  u32VDfLevel;    /**<  vertical deflicker level */
    FY_U8   *pu8HDfCoef;    /**<  horizontal deflicker coefficient */
    FY_U8   *pu8VDfCoef;    /**<  vertical deflicker coefficient */
}FYFB_DEFLICKER_S;

/** Alpha info */
typedef struct
{
    FY_BOOL bAlphaEnable;   /**<  alpha enable flag */
    FY_BOOL bAlphaChannel;  /**<  alpha channel enable flag */
    FY_U8 u8Alpha0;         /**<  alpha0 value, used in ARGB1555 */
    FY_U8 u8Alpha1;         /**<  alpha1 value, used in ARGB1555 */
    FY_U8 u8GlobalAlpha;    /**<  global alpha value */
    FY_U8 u8Reserved;
}FYFB_ALPHA_S;

typedef enum
{
    FYFB_FMT_RGB565 = 0,
    FYFB_FMT_RGB888,		      /**<  RGB888 24bpp */

    FYFB_FMT_KRGB444,       /**<  RGB444 16bpp */
    FYFB_FMT_KRGB555,       /**<  RGB555 16bpp */
    FYFB_FMT_KRGB888,       /**<  RGB888 32bpp */

    FYFB_FMT_ARGB4444,      /**< ARGB4444 */
    FYFB_FMT_ARGB1555,      /**< ARGB1555 */
    FYFB_FMT_ARGB8888,      /**< ARGB8888 */
    FYFB_FMT_ARGB8565,      /**< ARGB8565 */

    FYFB_FMT_RGBA4444,      /**< ARGB4444 */
    FYFB_FMT_RGBA5551,      /**< RGBA5551 */
    FYFB_FMT_RGBA5658,      /**< RGBA5658 */
    FYFB_FMT_RGBA8888,      /**< RGBA8888 */

    FYFB_FMT_BGR565,        /**< BGR565 */
    FYFB_FMT_BGR888,        /**< BGR888 */
    FYFB_FMT_ABGR4444,      /**< ABGR4444 */
    FYFB_FMT_ABGR1555,      /**< ABGR1555 */
    FYFB_FMT_ABGR8888,      /**< ABGR8888 */
    FYFB_FMT_ABGR8565,      /**< ABGR8565 */
    FYFB_FMT_KBGR444,       /**< BGR444 16bpp */
    FYFB_FMT_KBGR555,       /**< BGR555 16bpp */
    FYFB_FMT_KBGR888,       /**< BGR888 32bpp */

    FYFB_FMT_1BPP,          /**<  clut1 */
    FYFB_FMT_2BPP,          /**<  clut2 */
    FYFB_FMT_4BPP,          /**<  clut4 */
    FYFB_FMT_8BPP,          /**< clut8 */
    FYFB_FMT_ACLUT44,       /**< AClUT44*/
    FYFB_FMT_ACLUT88,         /**< ACLUT88 */
    FYFB_FMT_PUYVY,         /**< UYVY */
    FYFB_FMT_PYUYV,         /**< YUYV */
    FYFB_FMT_PYVYU,         /**< YVYU */
    FYFB_FMT_YUV888,        /**< YUV888 */
    FYFB_FMT_AYUV8888,      /**< AYUV8888 */
    FYFB_FMT_YUVA8888,      /**< YUVA8888 */
    FYFB_FMT_BUTT
}FYFB_COLOR_FMT_E;

typedef struct
{
    FY_BOOL bKeyRgb;
    FY_BOOL bKeyAlpha;      /**<  whether support colorkey alpha */
    FY_BOOL bGlobalAlpha;   /**<  whether support global alpha */
    FY_BOOL bCmap;          /**<  whether support color map */
    FY_BOOL bHasCmapReg;    /**<  whether has color map register*/
    FY_BOOL bColFmt[FYFB_FMT_BUTT]; /**<  support which color format */
    FY_BOOL bVoScale;       /**< support vo scale*/
    FY_BOOL bLayerSupported;    /**< whether support a certain layer, for example:x5 HD support FYFB_SD_0 not support FYFB_SD_1*/
    FY_U32  u32MaxWidth;    /**<  the max pixels per line */
    FY_U32  u32MaxHeight;   /**<  the max lines */
    FY_U32  u32MinWidth;    /**<  the min pixels per line */
    FY_U32  u32MinHeight;   /**<  the min lines */
    FY_U32  u32VDefLevel;   /**<  vertical deflicker level, 0 means vertical deflicker is unsupported */
    FY_U32  u32HDefLevel;   /**<  horizontal deflicker level, 0 means horizontal deflicker is unsupported  */
    FY_BOOL  bDcmp;
    FY_BOOL  bPreMul;
}FYFB_CAPABILITY_S;

/*refresh mode*/
typedef enum
{
    FYFB_LAYER_BUF_DOUBLE = 0x0,       /**<  2 display buf in fb */
    FYFB_LAYER_BUF_ONE    = 0x1,       /**<  1 display buf in fb */
    FYFB_LAYER_BUF_NONE   = 0x2,       /**<  no display buf in fb,the buf user refreshed will be directly set to VO*/
    FYFB_LAYER_BUF_DOUBLE_IMMEDIATE=0x3, /**< 2 display buf in fb, each refresh will be displayed*/
    FYFB_LAYER_BUF_BUTT
} FYFB_LAYER_BUF_E;

/* surface info */
typedef struct
{
    FY_U32  u32PhyAddr;     /**<  start physical address */
    FY_U32  u32Width;       /**<  width pixels */
    FY_U32  u32Height;      /**<  height pixels */
    FY_U32  u32Pitch;       /**<  line pixels */
    FYFB_COLOR_FMT_E enFmt; /**<  color format */
}FYFB_SURFACE_S;

typedef struct
{
    FY_U32 u32PhyAddr;
    FYFB_ALPHA_S stAlpha;
    FYFB_COLORKEY_S stColorkey;
}FYFB_SURFACEEX_S;

/* refresh surface info */
typedef struct
{
    FYFB_SURFACE_S stCanvas;
    FYFB_RECT UpdateRect;       /* refresh region*/
}FYFB_BUFFER_S;

/* cursor info */
typedef struct
{
    FYFB_SURFACE_S stCursor;
    FYFB_POINT_S stHotPos;
} FYFB_CURSOR_S;

/* DDR detect zone info */
typedef struct
{
    FY_U32 u32StartSection;
    FY_U32 u32ZoneNums;
} FYFB_DDRZONE_S;

/*fb var info*/
typedef struct
{
	int bits_per_pixel;
	int xres_virtual;
	int yres_virtual;
	int xres;
	int yres;
	int xoffset;
	int yoffset;
} FYFB_VAR_SCREENINFO;
/* crusor handle */
/* Attention:surface in cursor will be released by user*/
#define FBIOPUT_CURSOR_INFO		      _IOW(IOC_TYPE_FYFB, 104, FYFB_CURSOR_S *)
#define FBIOGET_CURSOR_INFO		      _IOW(IOC_TYPE_FYFB, 105, FYFB_CURSOR_S *)
#define FBIOPUT_CURSOR_STATE		  _IOW(IOC_TYPE_FYFB, 106, FY_BOOL *)
#define FBIOGET_CURSOR_STATE		  _IOW(IOC_TYPE_FYFB, 107, FY_BOOL *)
#define FBIOPUT_CURSOR_POS		      _IOW(IOC_TYPE_FYFB, 108, FYFB_POINT_S *)
#define FBIOGET_CURSOR_POS		      _IOR(IOC_TYPE_FYFB, 109, FYFB_POINT_S *)
#define FBIOPUT_CURSOR_COLORKEY       _IOR(IOC_TYPE_FYFB, 110, FYFB_COLORKEY_S *)
#define FBIOGET_CURSOR_COLORKEY       _IOW(IOC_TYPE_FYFB, 111, FYFB_COLORKEY_S *)
#define FBIOPUT_CURSOR_ALPHA          _IOR(IOC_TYPE_FYFB, 112, FYFB_ALPHA_S *)
#define FBIOGET_CURSOR_ALPHA          _IOW(IOC_TYPE_FYFB, 113, FYFB_ALPHA_S *)
/** cursor will be separated from attached layer automatically if you attach cursor to another layer,that means
cursor can be attached to only one layer at any time*/
#define FBIOPUT_CURSOR_ATTCHCURSOR    _IOW(IOC_TYPE_FYFB, 114, FY_U32 *)
#define FBIOPUT_CURSOR_DETACHCURSOR   _IOW(IOC_TYPE_FYFB, 115, FY_U32 *)

/**antiflicker level*/
/**Auto means fb will choose a appropriate antiflicker level automatically according to the color info of map*/
typedef enum
{
    FYFB_LAYER_ANTIFLICKER_NONE = 0x0,	/**< no antiflicker*/
    FYFB_LAYER_ANTIFLICKER_LOW = 0x1,	/**< low level*/
    FYFB_LAYER_ANTIFLICKER_MIDDLE = 0x2,/**< middle level*/
    FYFB_LAYER_ANTIFLICKER_HIGH = 0x3, /**< fygh level*/
    FYFB_LAYER_ANTIFLICKER_AUTO = 0x4, /**< auto*/
    FYFB_LAYER_ANTIFLICKER_BUTT
}FYFB_LAYER_ANTIFLICKER_LEVEL_E;

/*layer info maskbit*/
typedef enum
{
    FYFB_LAYERMASK_BUFMODE = 0x1,           /**< BUFMODE bitmask */
    FYFB_LAYERMASK_ANTIFLICKER_MODE = 0x2,  /**< ANTIFLICKER_MODE bitmask */
    FYFB_LAYERMASK_POS = 0x4,               /**< the position bitmask */
    FYFB_LAYERMASK_CANVASSIZE = 0x8,      /**< canvassize bitmask */
    FYFB_LAYERMASK_DISPSIZE = 0x10,       /**< displaysize bitmask */
    FYFB_LAYERMASK_SCREENSIZE = 0x20,     /**< screensize bitmask */
    FYFB_LAYERMASK_BMUL = 0x40,           /**< pre-mult bitmask */
    FYFB_LAYERMASK_BUTT
}FYFB_LAYER_INFO_MASKBIT;

/**layer info*/
typedef struct
{
    FYFB_LAYER_BUF_E BufMode;
    FYFB_LAYER_ANTIFLICKER_LEVEL_E eAntiflickerLevel;
    FY_S32 s32XPos;           /**<  the x pos of origion point in screen */
    FY_S32 s32YPos;	          /**<  the y pos of origion point in screen */
    FY_S32 u32CanvasWidth;    /**<  the width of canvas buffer */
    FY_S32 u32CanvasHeight;   /**<  the height of canvas buffer */
    FY_U32 u32DisplayWidth;	  /**<  the width of display buf in fb.for 0 buf ,there is no display buf in fb, so it's effectless*/
    FY_U32 u32DisplayHeight;  /**<  the height of display buf in fb. */
    FY_U32 u32ScreenWidth;    /**<  the width of screen */
    FY_U32 u32ScreenHeight;   /**<  the height of screen */
    FY_BOOL bPreMul;          /**<  The data drawed in buf is premul data or not*/
    FY_U32 u32Mask;			  /**<  param modify mask bit*/
    FY_U32 fb_len;            /**<  the size of display buffer */
    FY_U32 fb_phys;           /**<  the addr of display buffer */
}FYFB_LAYER_INFO_S;

/** To set the layer information */
#define FBIOPUT_LAYER_INFO                _IOW(IOC_TYPE_FYFB, 120, FYFB_LAYER_INFO_S*)
/** To get the layer information */
#define FBIOGET_LAYER_INFO                _IOR(IOC_TYPE_FYFB, 121, FYFB_LAYER_INFO_S*)
/** To get canvas buf */
#define FBIOGET_CANVAS_BUFFER             _IOR(IOC_TYPE_FYFB, 123, FYFB_BUFFER_S*)
/** To refresh the displayed contents in extended mode */
#define FBIO_REFRESH                      _IOW(IOC_TYPE_FYFB, 124, FYFB_BUFFER_S*)
/**sync refresh*/
#define FBIO_WAITFOR_FREFRESH_DONE        _IO(IOC_TYPE_FYFB, 125)
/**To set the DDR detect zone of an overlay layer*/
#define FBIOPUT_MDDRDETECT_FYFB    _IOW(IOC_TYPE_FYFB, 135, FYFB_DDRZONE_S*)
/**To get the DDR detect zone of an overlay layer*/
#define FBIOGET_MDDRDETECT_FYFB    _IOW(IOC_TYPE_FYFB, 136, FYFB_DDRZONE_S*)

/**To enable the layer*/
#define FBIOBIND_GFX_LAYER        	  _IOW(IOC_TYPE_FYFB, 141, FY_S32)
/**To cfg var info*/
#define FBIOPUT_VSCREENINFO        	  _IOW(IOC_TYPE_FYFB, 142, FYFB_VAR_SCREENINFO)

#define FBIOPAN_DISPLAY        		  _IOW(IOC_TYPE_FYFB, 143, FYFB_VAR_SCREENINFO)
/**To disable the layer*/
#define FBIOUNBIND_GFX_LAYER          _IOW(IOC_TYPE_FYFB, 144, FY_S32)


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __FB_DRV_IOC_H__ */

