#ifndef _ISP_COMMON_H_
#define _ISP_COMMON_H_

#include "types/type_def.h"
#include "types/bufCtrl.h"
#include "dsp/fh_common.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
#pragma pack(4)

#define MALLOCED_MEM_BASE_ISP (0)

#define MALLOCED_MEM_ISPFPKG    (1)
#define MALLOCED_MEM_ISPFPKG_L  (2)
#define MALLOCED_MEM_ISPBPKG    (4)
#define MALLOCED_MEM_ISPBPKG_L  (5)
#define MALLOCED_MEM_ISPPPKG    (6)
#define MALLOCED_MEM_ISPPPKG_L  (7)

#define MALLOCED_MEM_DRV (3)

#define MALLOCED_ISP_OUT_LUMA   (0x10)

#define GAIN_NODES 12

typedef int (*ispInitCfgCallback)(void);
typedef int (*ispIntCallback)(void);
int isp_core_trace_interface(FH_UINT32 u32Id, const char* func);

/**|SYSTEM CONTROL|**/
typedef enum _ISP_BAYER_TYPE_S_ {
    BAYER_RGGB = 0x0, // 色彩模式RGGB
    BAYER_GRBG = 0x1, // 色彩模式GRBG
    BAYER_BGGR = 0x2, // 色彩模式BGGR
    BAYER_GBRG = 0x3, // 色彩模式GBRG
    ISP_BAYER_TYPE_DUMMY=0xffffffff,
} ISP_BAYER_TYPE;

typedef enum _TAB_COLOR_CODE {
    CC_R  = 0,  // 色彩模式R通道
    CC_GR = 1,  // 色彩模式Gr通道
    CC_B  = 2,  // 色彩模式B通道
    CC_GB = 3,  // 色彩模式Gb通道
    TAB_COLOR_CODE_DUMMY =0xffffffff,
} TAB_COLOR_CODE;

typedef enum _CURVE_TYPE_ {
    CURVE_BUILTIN     = 0x0, // 选用内定曲线
    CURVE_USER_DEFINE = 0x1, // 选用用户自定义曲线
    CURVE_TYPE_DUMMY  =0xffffffff,
} CURVE_TYPE;

typedef enum _FRAME_TYPE_ {
    FRAME_LINEAR = 0x0,      //frame类型为线性
    FRAME_WDR_SHORT = 0x1,   //frame类型为WDR的短帧
    FRAME_WDR_LONG  = 0x2,   //frame类型为WDR的长帧
    FRAME_WDR_MERGED = 0x3,  //frame类型为WDR merge
} FRAME_TYPE;

typedef enum _DUMP_TYPE_ {
    DUMP_LONG = 0x0,
    DUMP_SHORT = 0x1,
    DUMP_WDR_MERGE  = 0x2,
    DUMP_NR3D = 0x3,
    DUMP_DRC = 0x4,
    DUMP_CFA = 0x5,
    DUMP_YUV444 = 0x6,
    DUMP_YUV422 = 0x7,
} DUMP_TYPE;

typedef enum _DUMP_RAW_FORMAT_ {
    RAW_FORMAT = 0x0,
    RGB_FORMAT = 0x1,
    YUV422_FORMAT  = 0x2,
    YUV444_FORMAT = 0x3,
} DUMP_RAW_FORMAT;

typedef enum _OFFLINE_MODE_
{
    ISP_OFFLINE_MODE_DISABLE  = 0,  // ISP工作模式为在线模式
    ISP_OFFLINE_MODE_LINEAR   = 1,  // ISP工作为离线线性模式
    ISP_OFFLINE_MODE_WDR      = 2,  // ISP工作为离线款动态模式
} ISP_OFFLINE_MODE;

typedef enum _ISP_OUT_MODE_
{
    ISP_OUT_TO_VPU  = (1 << 0),// ISP输出到VPU
    ISP_OUT_TO_DDR  = (1 << 1),// ISP输出到DDR，不支持
    ISP_OUT_TO_ENG  = (1 << 2),// ISP输出到ENG，不支持
}ISP_OUT_MODE_E;

typedef enum _ISP_OUT_FORMAT_
{
    ISP_OUT_TO_DDR_YUV420_8BIT  = 0,// ISP输出格式为YUV420_8BIT
    ISP_OUT_TO_DDR_YUV422_8BIT  = 1,// ISP输出格式为YUV422_8BIT
    ISP_OUT_TO_DDR_YUV420_10BIT = 2,// ISP输出格式为YUV420_10BIT
    ISP_OUT_TO_DDR_YUV422_10BIT = 3,// ISP输出格式为YUV422_10BIT
}ISP_OUT_FORMAT_E;

typedef enum _LUT2D_WORK_MODE_E_
{
    ISP_LUT2D_BYPASS  = 0,// 2dlut 未启用
    ISP_LUT2D_ONLINE  = 1,// 2dlut 在线模式
    ISP_LUT2D_OFFLINE = 2,// 2dlut 离线模式
}LUT2D_WORK_MODE_E;

typedef enum _COEF_TYPE_ {
    NR3D_COEF = 0x0,// coef类型为nr3d
    NR2D_COEF = 0x1,// coef类型为nr2d
} COEF_TYPE;

typedef enum _VI2Y_BUFFER_MODE_
{
    VI2Y_BUFFER_OFF = 0,     // 未启用vi2y的buffer
    VI2Y_BUFFER_ON  = 1,     // 启用vi2y的buffer
} VI2Y_BUFFER_MODE;

typedef enum _VI2Y_DWOUT_SIZE_
{
    DWOUT_8BIT    = 0x8,     // vi2y输出位宽8bit
    DWOUT_10BIT   = 0xa,     // vi2y输出位宽10bit
} VI2Y_DWOUT_SIZE;

typedef struct _ISP_PIC_SIZE_S_
{
    FH_UINT32 u32Width;     // 输入幅面宽度 | [0~0xffff]
    FH_UINT32 u32Height;    // 输入幅面高度 | [0~0xffff]
} ISP_PIC_SIZE;

typedef struct _ISP_MEM_INIT_
{
    ISP_OFFLINE_MODE  enOfflineWorkMode; // 当前ISP工作模式，不支持 | [OFFLINE_MODE]
    ISP_OUT_MODE_E    enIspOutMode;      // ISP输出的方式 | [ISP_OUT_MODE_E]
    ISP_OUT_FORMAT_E  enIspOutFmt;       // ISP离线输出的格式，不支持 | [ISP_OUT_FORMAT]
    LUT2D_WORK_MODE_E enLut2dWorkMode;   // Lut缓存分配控制，仅影响内存分配，不支持 | [0~1]
    ISP_PIC_SIZE      stPicConf;         // 幅面配置 | [ISP_PIC_SIZE]
    VI2Y_BUFFER_MODE  enVi2yBufferMode;  // VI2Y的BUFFER开关 | [VI2Y_BUFFER_MODE]
    VI2Y_DWOUT_SIZE   enVi2yDWoutConf;   // VI2Y输出位宽配置 | [VI2Y_DWOUT_SIZE]
} ISP_MEM_INIT;

typedef struct _ISP_RAW_BUF_S
{
    struct mem_desc raw_mem;
    FH_UINT32 frameCnt;
    FRAME_TYPE raw_type;

    //cfg
    FH_UINT32   u32BlkMod;     // 是否分块
    FH_UINT32   u32RawSizeSf;  // Sf单帧raw的大小
    FH_UINT32   u32RawSizeLf;  // Lf单帧raw的大小(分块时为单块大小)
} ISP_RAW_BUF;

typedef struct _ISP_PARAM_CONFIG_S_
{
    FH_UINT32 u32BinAddr;// ISP参数存放的当前地址 | [0~0xffffffff]
    FH_UINT32 u32BinSize;// ISP参数的大小 | [0~0xffffffff]
} ISP_PARAM_CONFIG;

typedef struct _HW_MODULE_CFG_S{
    FH_UINT32 moduleCfg;  // 需要配置的模块 | [ISP_HW_MODULE_LIST]
    FH_BOOL enable;  // 使能开关,0表示关闭,1表示打开 | [0~1]
} HW_MODULE_CFG;

typedef struct _ISP_VERSION_S_
{
    FH_UINT32 u32SdkVer;        // sdk版本号 | [0~0xffffffff]
    FH_UINT32 FH_UINT32ChipVer; // 芯片版本号 | [0~0xffffffff]
    FH_UINT8  u08SdkSubVer;     // sdk sub版本号 | [0~0xff]
    FH_UINT8  u08BuildTime[21]; // sdk构建时间 | [0~0xff]
} ISP_VERSION;

typedef struct _ISP_VI_ATTR_
{
    FH_UINT16      u16WndHeight;   ///<sensor幅面高度 | [0~0xffff]
    FH_UINT16      u16WndWidth;    /// <sensor幅面宽度 | [0~0xffff]
    FH_UINT16      u16InputHeight; ///<sensor输入图像高度 | [0~0xffff]
    FH_UINT16      u16InputWidth;  ///<sensor输入图像宽度 | [0~0xffff]
    FH_UINT16      u16OffsetX;     ///<裁剪水平偏移 | [0~0xffff]
    FH_UINT16      u16OffsetY;     ///<裁剪垂直偏移 | [0~0xffff]
    FH_UINT16      u16PicHeight;   ///<处理的图像高度 | [0~0xffff]
    FH_UINT16      u16PicWidth;    ///<处理的图像宽度 | [0~0xffff]
    FH_UINT16      u16FrameRate;   ///<帧率 | [0~0xffff]
    ISP_BAYER_TYPE enBayerType;    ///bayer数据格式 | [0~0x3]
} ISP_VI_ATTR_S;

typedef struct _ISP_VI_STAT_S
{
    FH_UINT32 u32IPBIntCnt;   // IPB中断计数 | [0~0xffffffff]
    FH_UINT32 u32IPFIntCnt;   // IPF中断计数 | [0~0xffffffff]
    FH_UINT32 u32FrmRate;     // 当前帧率 | [0~0xffffffff]
    FH_UINT32 u32PicWidth;    // 当前处理图像宽度 | [0~0xffffffff]
    FH_UINT32 u32PicHeight;   // 当前处理图像高度 | [0~0xffffffff]
    FH_UINT32 u32IpfOverFlow; // ISP Vi溢出中断计数 | [0~0xffffffff]
    FH_UINT32 u32IspErrorSt;  // 出错标志，Vi溢出终端计数大于阈值会被值1 | [0~1]
} ISP_VI_STAT_S;

typedef struct _ISP_VI2Y_CFG_S
{
    FH_BOOL   bVi2yEn;       // VI2Y使能 0：关闭 1：打开。| [0~1]
    FH_UINT8  u08VoType;     //处理类型 0:输入输出相同 1:raw2y输出 | [0~1]
    FH_UINT8  u08FilterType; // 0:2x2滤波 1:3x3滤波 | [0~1]
    FH_UINT8  u08BitShift;   // 输出右移数 | [0~0x7]
    FH_UINT16 u16BlcChannel[4]; // 通道0 blc值 | [0x0~0xfff]
} ISP_VI2Y_CFG;

typedef struct _ISP_VI2Y_CUT_SIZE_S
{
    FH_UINT16 u32CutWidth;      // 裁剪图像宽度 | [0~0xfff]
    FH_UINT16 u32CutHeight;     // 裁剪图像高度 | [0~0xfff]
    FH_UINT16 u32CutOffectX;    // 图像水平偏移 | [0~0xfff]
    FH_UINT16 u32CutOffectY;    // 图像垂直偏移 | [0~0xfff]
} ISP_VI2Y_CUT_SIZE;

typedef struct _ISP_VIGEN_CFG_S
{
    FH_BOOL   bViGenEn;     // VIGEN使能 0：关闭 1：打开。| [0~1]
    FH_UINT32 u32ViAddr;    //线性图像（或者wdr合成后的图像）导入到ddr空间的起始地址,大小计算方式：1.非合成的短（长）帧为长×宽×1.5;2合成后的为长x宽×2| [0~0xffffffff]
    FH_UINT32 u32LfViAddr;  //长帧图像导入到ddr空间的起始地址，大小计算方式：长帧为长×宽×1.5| [0~0xffffffff]
    FH_UINT32 u32PicWidth;  // 图像宽度 | [0~0xffffffff]
    FH_UINT32 u32PicHeight; // 图像高度 | [0~0xffffffff]
    FH_UINT32 u32FrameWidth;// 框架宽度 | [0~0xffffffff]
    FH_UINT32 u32FrameHeight;// 框架高度 | [0~0xffffffff]
    ISP_BAYER_TYPE  enBayerType;// bayer数据格式 | [0~0x3]
    FH_UINT8  u08InputMode; // 模式选择 0.线性（短帧）模式 1.长短两帧帧输入模式 2.wdr合成后数据输入模式 | [0~2]
    FH_UINT8  u08FrameCnt;  // 多张图模式的帧数（需要输入大于1,否则为单图）| [0~0xff]
} ISP_VIGEN_CFG;

typedef struct _ISP_DUMP_RAW_BUF_S
{
    struct mem_desc raw_mem;
    DUMP_TYPE raw_type;
    DUMP_RAW_FORMAT raw_format;

    //cfg
    FH_UINT32   u32BlkMod;     // 是否分块
    FH_UINT16   u16DumpStride; // dump stride: Width * 2
    FH_UINT32   u32SleepTime;  // 一帧时间
    FH_UINT32   frameCnt;
    FH_UINT32   u32RawSize;    // Dump raw单帧大小(分块时为单块大小)
} ISP_DUMP_RAW_BUF;

typedef struct _ISP_COEF_RAW_CFG_S
{
    COEF_TYPE coef_type;

    //cfg
    FH_UINT32   u32BlkMod;     // 是否分块
    FH_UINT32   u32SleepTime;  // 一帧时间
    FH_UINT32   u32RawSize;    // Coef raw单帧大小(分块时为单块大小)
} ISP_COEF_RAW_CFG;

typedef struct _ISP_CTRL_CFG_S
{
    FH_UINT32  u32Id;                 // Isp通道号 | [0~1]
    FH_BOOL    bNr3dEn;               // Nr3d模块使能，默认开启，影响缓存分配 | [0~1]
    FH_BOOL    bNr3dStraightEn;       // Nr3d模块直通使能 | [0~1]
    FH_BOOL    bNr3dEcEn;             // Nr3d压缩模块是否开启,默认开启 | [0~1]
    FH_BOOL    bNr3dCirEn;            // Nr3d循环缓冲使能，默认是开启 | [0~1]
    FH_UINT32  u32Nr3dDepthAppend;    // Nr3d循环缓冲下的缓冲深度append | [0~0xffff]
    FH_UINT32  u32Nr3dEcBitMod;       // Nr3d压缩bit模式,0:12bit,1:16bit | [0~1]
    FH_BOOL    bLceWinSpaceBlkMod;    // Lce统计滤波窗口长宽最大分窗模式，0:8个窗口,1:16个窗口 | [0~1]
    FH_BOOL    bLceWinBinBlkMod;      // Lce统计滤波窗口值域最大bin数模式,0:32bin,1:64bin | [0~1]
    FH_BOOL    bWdrEn;                // Wdr模块使能，默认开启，影响缓存分配 | [0~1]
    FH_BOOL    bWdrCirEn;             // Wdr循环缓冲使能，默认是开启 | [0~1]
    FH_UINT32  u32WdrCirBufNum;       // Wdr循环缓冲buf个数 | [1~2]
    FH_BOOL    bWdrCirBufDepth;       // Wdr循环缓冲buf深度 | [0~0xffff]
    FH_BOOL    bIsppCirEn;            // Ispp循环缓冲使能 | [0~1]
    FH_BOOL    bIsppEcEn;             // Ispp压缩模块使能 | [0~1]
    FH_UINT32  u32IsppBufNum;         // Ispp缓冲buf个数 | [2~3]
    FH_BOOL    bForceKick;            // ISP是否支持VPU阻塞 | [0~1]
    FH_BOOL    bFreeRun;              // ISP是否自踢 | [0~1]
} ISP_CTRL_CFG;

/**|MIRROR|**/
typedef struct mirror_cfg_s
{
    FH_BOOL             bEN;  // 镜像使能开关 | [0~1]
    ISP_BAYER_TYPE      normal_bayer;  // 无效成员变量 | [0~3]
    ISP_BAYER_TYPE      mirror_bayer;  //　无效成员变量 | [0~3]
}MIRROR_CFG_S;

/**|STAT POS|**/
typedef enum _STAT_SEL_MODULE
{
    STAT_SEL_AWB_ADV = 0,
    STAT_SEL_GL   = 1,
    STAT_SEL_HIST = 2,
    STAT_SEL_MD   = 3,
    STAT_SEL_DRC  = 4,
    STAT_SEL_AF   = 5,   // FH885XV300不支持
} STAT_SEL_MODULE;

/**|Smart AE|**/
typedef struct _SMART_AE_ROI_S_
{
    FH_UINT16    u16RectX;        //ROI检测框左上角横坐标,以像素为单位 | [0x0-0xffff]
    FH_UINT16    u16RectY;        //ROI检测框左上角纵坐标,以像素为单位 | [0x0-0xffff]
    FH_UINT16    u16RectW;        //ROI检测框横向宽度,以像素为单位 | [0x0-0xffff]
    FH_UINT16    u16RectH;        //ROI检测框纵向高度,以像素为单位 | [0x0-0xffff]
    FH_UINT8     u08RectWeight;   //ROI检测框权重，取值范围0-15 | [0x0-0xf]
}SMART_AE_ROI;

typedef struct _SMART_AE_CFG_S_
{
    FH_BOOL      bSmartAeEn;      //智能曝光策略总开关 0：关闭智能曝光策略 1：打开智能曝光策略 | [0x0-0x1]
    FH_UINT16    u16RatioMin;     //智能曝光系数最大值 0x400表示1倍 | [0x0-0xffff]
    FH_UINT16    u16RatioMax;     //智能曝光系数最小值 0x400表示1倍 | [0x0-0xffff]
    FH_UINT16    u16DelayNum;     //智能曝光延迟恢复帧数，值越小，被检测物体离开场景后，恢复到正常曝光的时间越短,最大支持0xfff帧  | [0x0-0xfff]
    FH_UINT8     u08TargetLuma;   //智能曝光目标亮度 | [0x0-0xff]
    FH_UINT8     u08Speed;        //智能曝光速度，值越小速率越快，0是最快速率 | [0x0-0xff]
    FH_UINT8     u08Interval;     //智能曝光策略运行间隔，值为0时表示每帧都运行，值为1时表示隔1帧运行一次曝光调整策略,最大支持15帧间隔 | [0x0-0xf]
    FH_UINT8     u08RoiNum;       //ROI检测框有效数量，最大支持4个 | [0x0-0x4]
    SMART_AE_ROI stDetectRoi[4];  //智能曝光检测框坐标配置 | [SMART_AE_ROI]
}SMART_AE_CFG;

typedef struct _SMART_AE_STATUS_S_
{
    FH_UINT8 u08CurrLuma;         // 当前ROI检测框测光亮度 | [0x0-0xff]
    FH_UINT8 u08OpStatus;         // 当前智能曝光状态 | [0x0-0x2]
} SMART_AE_STATUS;

/**|AE|**/
typedef struct _AE_ENABLE_CFG_S_
{
    FH_BOOL bAecEn;          // 自动曝光策略总开关     0：关闭AE策略   1：打开AE策略 | [0x0-0x1]
    FH_UINT8 u08AeMode;      // 自动曝光模式选择： （更新时需置位refresh位）       0：自动 1：半自动(允许固定曝光时间或增益中的一个，另一个走自动模式)     2：手动 | [0x0-0x2]
    FH_BOOL bAscEn;          // 自动快门使能   (aeMode为0时生效，更新时需置位refresh位)         0：关闭自动曝光时间，曝光时间维持上一帧的数值  1：使能自动曝光时间 | [0x0-0x1]
    FH_BOOL bMscEn;          // 手动快门使能   (aeMode为1时生效，更新时需置位refresh位)        0：关闭手动曝光时间，则曝光时间自动走   1：使能手动曝光时间，则曝光时间由mIntt决定 | [0x0-0x1]
    FH_BOOL bAgcEn;          // 自动增益使能   (aeMode为0时生效，更新时需置位refresh位)        0：关闭自动增益，增益维持上一帧的数值   1：使能自动增益 | [0x0-0x1]
    FH_BOOL bMgcEn;          // 手动增益使能(aeMode为1时生效，更新时需置位refresh位)   0：关闭手动增益，则增益自动走   1：使能手动增益，则增益由mAgain,mDgain决定 | [0x0-0x1]
    FH_BOOL bSensUpEn;       // 慢快门使能(更新时需置位refresh位)   0：禁止降低帧率 1：允许降低帧率，增加快门时间 | [0x0-0x1]
    FH_BOOL bIrisEn;         // 自动光圈使能(暂不支持)        0：关闭自动光圈 1：打开自动光圈 | [0x0-0x1]
    FH_BOOL bInttFineEn;     // 精细快门使能      0：只能调整整数行曝光时间       1：允许调整小数行曝光时间(采用数字增益内插) | [0x0-0x1]
    FH_BOOL bGainFineEn;     // 精细增益使能      0：增益精度跟随sensor   1：允许1/256增益精度(采用数字增益内插) | [0x0-0x1]
    FH_BOOL bAspeedEn;       // 自动速度控制使能    0：关闭自动速度，速度由initSpeed，speed及UExpSpeedBias决定      1：打开自动速度 | [0x0-0x1]
    FH_BOOL bAdaptiveEn;     // 自适应参考亮度    0：关闭自适应参考亮度，亮度只由lumaRef决定      1：打开自适应参考亮度，亮度跟随环境光线变化 | [0x0-0x1]
    FH_BOOL bAntiFlickerEn;  // 自动抗闪使能 0：关闭抗闪模式 1：打开抗闪模式 | [0x0~0x1]
    FH_BOOL bARouteEn;       // 自动曝光路线使能（更新时需置位refresh位）   0：使用默认曝光路线     1：使用手动曝光路线 | [0x0-0x1]
    FH_BOOL bInttUnitSel;    // 曝光时间单位选择：       0: 曝光时间单位使用us   1: 曝光时间单位使用行 | [0x0-0x1]
    FH_BOOL bInttFineMode;   // Intt内插模式选择 0：使用dGain内插 1：使用aGain内插 | [0x0~0x1]
    FH_BOOL bSfInttFineEn;   // 短帧精细快门使能(仅限wdr模式)：    0：只能调整整数行曝光时间       1：允许调整小数行曝光时间(采用数字增益内插) | [0x0-0x1]
} AE_ENABLE_CFG;

typedef struct _AE_INIT_CFG_S_
{
    FH_UINT8 u08InitSpeed;     // 初始化曝光速度 | [0x0-0xff]
    FH_UINT32 u32InitExpTime;  // 初始化曝光时间，单位由inttUnitSel决定，暂不支持 | [0x0-0xfffff]
    FH_UINT16 u16InitAgain;    // 初始化sensor增益，U.6，暂不支持 | [0x40-0xffff]
    FH_UINT16 u16InitDgain;    // 初始化isp增益，U.6，暂不支持 | [0x40-0x3ff]
} AE_INIT_CFG;

typedef struct _AE_SPEED_CFG_S_
{
    FH_UINT8 u08Tolerance;        // 一般不超过lumaRef的1%,U.4精度 | [0x0-0xff]
    FH_UINT8 u08LightChangeZone;  // 较小亮度变化区间 | [0x0-0xff]
    FH_UINT8 u08GreatChangeZone;// 较大亮度变化区间 | [0x0~0xff]
    FH_UINT8 u08UnderExpLDlyFrm;// 欠曝较小调整时延时调整的帧数 | [0x0~0xff]
    FH_UINT8 u08UnderExpGDlyFrm;  // 欠曝较大调整时延时调整的帧数 | [0x0-0xff]
    FH_UINT8 u08OverExpLDlyFrm;// 过曝较小调整时延时调整的帧数 | [0x0~0xff]
    FH_UINT8 u08OverExpGDlyFrm;   // 过曝较大调整时延时调整的帧数 | [0x0-0xff]
    FH_UINT8 u08RunInterval;      // AE策略运行间隔，值为0时表示每帧运行，值为1时表示隔1帧运行一次，依次类推，一般配成0 | [0x0-0xf]
    FH_UINT8 u08Speed;            // 曝光速度，值越小速率越快，0是最快速率 | [0x0-0xff]
    FH_UINT8 u08UExpSpeedBias;// 欠曝调整时速度偏移量 | [0x0~0xff]
} AE_SPEED_CFG;

typedef struct _AE_METERING_CFG_S_
{
    FH_UINT8 u08StatSel;         // 统计模式： 0：3*3分块统计(不支持) 1：16*16分块统计 2：hist统计 | [0x0~0x2]
    FH_UINT8 u08LightMode;       // 亮度测光模式(statSel=1或2时生效)： 0：正常模式 1：高光优先 2：低光优先 | [0x0~0x2]
    FH_UINT8 u08BlockMode;       // 分块测光模式(statSel=0或1时生效)： 0：全局测光 1：前光补偿(仅3*3统计生效) 2：背光补偿(仅3*3统计生效) 3：用户自定义 | [0x0~0x3]
    FH_UINT8 u08MeteringBlock;   // 当blockMode = 1/2时生效，表示 0：最强，8最弱 | [0x0~0x8]
    FH_UINT8 u08LumaBoundary;    // 高光或低光亮度截断值(仅在lightMode下有效) | [0x0~0xff]
    FH_UINT8 u08RoiWeight;       // 高光或低光区域对平均亮度影响程度，值越大，对平均亮度影响越大，值为0时不影响(仅在lightMode下有效) | [0x0~0xff]
    FH_UINT16 u16Sensitivity;    // 亮度敏感度，值越大，对高光或低光区域越敏感, U.7(仅在lightMode下有效) | [0x0~0xffff]
    FH_UINT8 u08WeightBlk[256];  // 第0块权值（仅meteringMode=5或7，更新时需置位refresh位） | [0x0~0xf]
} AE_METERING_CFG;

typedef struct _AE_TIMING_CFG_S_
{
    FH_UINT8 u08Intt0Delay;   // 配置T1 intt时，延迟的帧数，更新时需置位refresh位 | [0x0-0x3]
    FH_UINT8 u08Intt1Delay;   // 当至少两帧合成时配置T2 intt时，延迟的帧数，更新时需置位refresh位 | [0x0-0x3]
    FH_UINT8 u08Intt2Delay;   // 当至少三帧合成时配置T3 intt时，延迟的帧数，更新时需置位refresh位 | [0x0-0x3]
    FH_UINT8 u08Intt3Delay;   // 当至少四帧合成时配置T4 intt时，延迟的帧数，更新时需置位refresh位 | [0x0-0x3]
    FH_UINT8 u08Again0Delay;  // 配置T1 sensor gain时，延迟的帧数，更新时需置位refresh位 | [0x0-0x3]
    FH_UINT8 u08Again1Delay;  // 当至少两帧合成时配置T2 again时，延迟的帧数，更新时需置位refresh位 | [0x0-0x3]
    FH_UINT8 u08Again2Delay;  // 当至少三帧合成时配置T3 again时，延迟的帧数，更新时需置位refresh位 | [0x0-0x3]
    FH_UINT8 u08Again3Delay;  // 当至少四帧合成时配置T4 again时，延迟的帧数，更新时需置位refresh位 | [0x0-0x3]
    FH_UINT8 u08DgainDelay;   // 配置isp gain时，延迟的帧数，更新时需置位refresh位 | [0x0-0xf]
    FH_UINT8 u08SensUpDelay;  // 配置帧结构时，延迟的帧数，更新时需置位refresh位 | [0x0-0xf]
    FH_UINT8 u08SfDgainDelay;  // 配置短帧精细曝光isp gain补偿时(仅限bSfInttFineEn=1)，延迟的帧数，更新时需置位refresh位 | [0x0-0xf]
} AE_TIMING_CFG;

typedef struct _AE_MANUAL_CFG_S_
{
    FH_UINT32 u32MIntt;   // 手动模式下的intt值，单位由inttUnitSel决定 | [0x0-0x3ffff]
    FH_UINT16 u16MAgain;  // 手动模式下的again值，U.6 | [0x40-0xffff]
    FH_UINT16 u16MDgain;  // 手动模式下的Dgain值，U.6 | [0x40-0x3ff]
} AE_MANUAL_CFG;

typedef struct _AE_ANTIFLICKER_CFG_S_
{
    FH_BOOL bAntiFlickerMode;  // 抗闪模式选择：    1:强抗闪模式，完全根据频闪周期配置曝光时间  0:弱抗闪模式，当环境光较亮时通过限制亮度并取消抗闪来避免过曝严重 | [0x0-0x1]
    FH_UINT16 u16Frequency;    // 抗闪频率值 | [0x0-0xfff]
    FH_UINT8 u08StabZone;      // 抗闪最大亮度差异设置，当stabZone+targetLuma<CurrLuma时取消抗闪设置 | [0x0-0xff]
} AE_ANTIFLICKER_CFG;

typedef struct _AE_LUMA_CFG_S_
{
    FH_UINT8 u08LumaRef;// 参考亮度，归一化至0~255 | [0x0~0xff]
    FH_UINT8 u08LumaRefLow;   // 低光状态下的参考亮度，仅当adaptiveEn为1且aRouteEn为0时生效，归一化至0~255 | [0x0-0xff]
    FH_UINT8 u08LumaRefHigh;// 高光状态下的参考亮度，仅当adaptiveEn为1且aRouteEn为0时生效，归一化至0~255 | [0x0~0xff]
    FH_UINT16 u16EvLow;       // 低光状态环境光强，用于aRouteEn为0时的参考亮度内插 | [0x0-0xffff]
    FH_UINT16 u16EvNormL;     // 正常光线状态环境光强下阈值，用于aRouteEn为0时的参考亮度内插 | [0x0-0xffff]
    FH_UINT16 u16EvNormH;     // 正常光线状态环境光强上阈值，用于aRouteEn为0时的参考亮度内插 | [0x0-0xffff]
    FH_UINT16 u16EvHigh;      // 高光状态环境光强，用于aRouteEn为0时的参考亮度内插 | [0x0-0xffff]
} AE_LUMA_CFG;

typedef struct _AE_RANGE_CFG_S_
{
    FH_UINT16 u16InttMin;   // 最小曝光时间，单位由inttUnitSel决定，更新时需置位refresh位 | [0x0-0xffff]
    FH_UINT32 u32InttMax;   // 最大曝光时间，单位由inttUnitSel决定，更新时需置位refresh位 | [0x0-0xfffff]
    FH_UINT16 u16AgainMin;  // sensor增益最小值，U.6，更新时需置位refresh位 | [0x40-0xffff]
    FH_UINT16 u16AgainMax;  // sensor增益最大值，U.6，更新时需置位refresh位 | [0x40-0xffff]
    FH_UINT16 u16DgainMin;  // isp增益最小值，U.6，更新时需置位refresh位 | [0x40-0x3ff]
    FH_UINT16 u16DgainMax;  // isp增益最大值，U.6，更新时需置位refresh位 | [0x40-0x3ff]
} AE_RANGE_CFG;

typedef struct _AE_DEFAULT_CFG_S_ {
    AE_ENABLE_CFG stAeEnableCfg;            // ae使能配置 | [AE_ENABLE_CFG]
    AE_INIT_CFG stAeInitCfg;                // ae初始化配置 | [AE_INIT_CFG]
    AE_SPEED_CFG stAeSpeedCfg;              // ae速率配置 | [AE_SPEED_CFG]
    AE_METERING_CFG stAeMeteringCfg;        // ae测光配置 | [AE_METERING_CFG]
    AE_TIMING_CFG stAeTimingCfg;            // ae时序配置 | [AE_TIMING_CFG]
    AE_MANUAL_CFG stAeManualCfg;            // ae手动值配置 | [AE_MANUAL_CFG]
    AE_ANTIFLICKER_CFG stAeAntiflickerCfg;  // ae抗闪配置 | [AE_ANTIFLICKER_CFG]
    AE_LUMA_CFG stAeLumaCfg;                // ae亮度配置 | [AE_LUMA_CFG]
    AE_RANGE_CFG stAeRangeCfg;              // 最小最大范围配置,更新时需置位refresh位 | [AE_RANGE_CFG]
    FH_BOOL bNodeChMode;                    // ae节点变化模式选择：     0:跨节点变化模式; 1:节点连续变化模式 | [0x0-0x1]
    FH_BOOL bSensUpMode;                    // 慢快门模式选择：     0:无极连续降帧; 1:非连续直接降帧 | [0x0-0x1]
    FH_UINT8 u08SensUpPrec;                 // 慢快门非连续降帧精度，值越大降帧分段越多越精确 | [0x0-0xf]
    FH_UINT16 u16SensUpEnableGain;         // 降帧启动增益，U.6 (aRouteEn为0时生效，更新时需置位refresh位) | [0x40-0xffff]
    FH_UINT16 u16SensUpDisableGain;        // 降帧关闭增益，U.6 (aRouteEn为0时生效，更新时需置位refresh位) | [0x40-0xffff]
    FH_UINT8 u08InttPrec;                   // 曝光精度，固定以行为单位 | [0x0-0xff]
    FH_UINT8 u08InttOffset;                 // 当sensor intt与again存在偏移关系时需要配置曝光时间的偏移，固定以μs为单位 | [0x0-0xff]
} AE_DEFAULT_CFG;

typedef struct _AE_ROUTE_CFG_S_ {
    FH_UINT16 u16Piris[16];   // 节点对应光圈参数，更新时需置位refresh位 | [0x0-0xfff]
    FH_UINT32 u32Intt[16];    // 节点对应曝光时间参数，单位由inttUnitSel决定，更新时需置位refresh位 | [0x0-0xfffff]
    FH_UINT16 u16AGain[16];   // 节点对应模拟增益，U.6，更新时需置位refresh位 | [0x40-0xffff]
    FH_UINT16 u16DGain[16];   // 节点对应数字增益，U.6，更新时需置位refresh位 | [0x40-0x3ff]
    FH_UINT8 u08LumaRef[16];  // 节点对应的参考亮度，更新时需置位refresh位 | [0x0-0xff]
} AE_ROUTE_CFG;

typedef struct _ISP_AE_INFO_S_
{
    FH_UINT32   u32Intt;          // sensor曝光行数 | [0~0xffffffff]
    FH_UINT32   u32IspGain;       // isp增益值U4.8, 0x100表示一倍 | [0~0xfff]
    FH_UINT32   u32SensorGain;    // sensor增益值U.6, 0x40表示一倍 |[0~0xffffffff]
    FH_UINT32   u32TotalGain;     // 总增益U.6 , 0x40表示一倍 | [0~0xffffffff]
} ISP_AE_INFO;

typedef struct _ISP_AE_INFO_2_S_
{
    FH_UINT32   u32Intt;          // sensor曝光行数 | [0~0xffffffff]
    FH_UINT32   u32PreGain;       // 前置isp增益值U4.8, 0x100表示一倍 | [0~0xfff]
    FH_UINT32   u32PostGain;      // 后置isp增益值U4.8, 0x100表示一倍 | [0~0xfff]
    FH_UINT32   u32SensorGain;    // sensor增益值U.6, 0x40表示一倍 |[0~0xffffffff]
    FH_UINT32   u32TotalGain;     // 总增益U.6 , 0x40表示一倍 | [0~0xffffffff]
} ISP_AE_INFO_2;

typedef struct _ISP_AE_STATUS_S_
{
    FH_UINT8 u08TarLuma;           // 目标亮度 | [0x0-0xff]
    FH_UINT8 u08CurrLuma;          // 当前亮度 | [0x0-0xff]
    FH_SINT16 s16ErrorLuma;// 当前帧亮度与目标值差异，S.4，tar-curr | [-4096~4095]
    FH_UINT16 u16CurrPiris;// 当前光圈值 | [0x0~0xfff]
    FH_UINT32 u32CurrIntt;         // 当前曝光时间，单位固定是μs | [0x0-0xfffff]
    FH_UINT16 u16CurrAgain;// 当前sensor增益，U.6 | [0x0~0xffff]
    FH_UINT16 u16CurrDgain;        // 当前isp增益，U.6 | [0x0-0xffff]
    FH_UINT32 u32CurrTotalGain;    // 当前总增益，U.6 | [0x0-0xffffffff]
    FH_UINT16 u16CurrInttLine;     // 当前曝光时间，单位固定为行 | [0x0-0xffff]
    FH_UINT16 u16CurrFrameHeight;  // 当前帧高度 | [0x0-0xffff]
    FH_UINT16 u16Ev;               // 当前环境光强 | [0x0-0xffff]
    FH_UINT8 u08CurrNodeId;        // 当前节点ID(仅aRouteEn为1时有效) | [0x0-0xf]
    FH_BOOL bOpStatus;             // Ae当前运行状态    0：停止 1：运行 | [0x0-0x1]
    FH_UINT32 u32ExpParam;         // 当前总曝光参数，曝光时间以行数计算，增益均以.6精度 | [0x0-0xffffffff]
    FH_UINT16 u16Piris[16];        // 该节点对应光圈参数 | [0x0-0xfff]
    FH_UINT32 u32Intt[16];         // 该节点对应曝光时间参数，单位由inttUnitSel决定 | [0x0-0xfffff]
    FH_UINT16 u16AGain[16];        // 该节点对应模拟增益，U.6 | [0x0-0xffff]
    FH_UINT16 u16DGain[16];// 该节点对应数字增益，U.6 | [0x0~0xffff]
    FH_UINT16 u16ActInttMax;       // 策略运行生效的inttmax，以行为单位 | [0x0-0xffff]
    FH_UINT16 u16ActAgainMax;      // 策略运行生效的againmax，U.6 | [0x0-0xffff]
    FH_UINT16 u16ActDgainMax;      // 策略运行生效的dgainmax，U.6 | [0x0-0xffff]
    FH_UINT16 u16SensorInttMax;    // Sensor支持的inttmax，以行为单位，从sensor库获取 | [0x0-0xffff]
    FH_UINT16 u16SensorInttMin;    // Sensor支持的inttMin，以行为单位，从sensor库获取 | [0x0-0xffff]
    FH_UINT16 u16SensorAgainMax;   // Sensor支持的againMax，U.6，从sensor库获取 | [0x0-0xffff]
    FH_UINT16 u16SensorAgainMin;   // Sensor支持的againMin，U.6，从sensor库获取 | [0x0-0xffff]
    FH_UINT16 u16DgainMax;         // ISP支持的DgainMax，U.6，由硬件位宽决定 | [0x0-0xffff]
    FH_UINT16 u16DgainMin;         // ISP支持的DgainMin，U.6，由硬件位宽决定 | [0x0-0xffff]
} ISP_AE_STATUS;

typedef struct _AE_USER_STATUS_S_
{
    FH_UINT8 u08TarLuma;         // 当前目标亮度 | [0x0-0xff]
    FH_UINT32 u32CurrTotalGain;  // 当前总增益，U.6 | [0x0-0xffffffff]
    FH_BOOL bOpStatus;           // Ae当前运行状态  0：停止 1：运行 | [0x0-0x1]
    FH_UINT32 u32CurrIntt;       // 当前曝光时间，单位固定是μs | [0x0-0xfffff]
    FH_UINT16 u16CurrInttLine;   // 当前曝光时间，单位固定为行 | [0x0-0xffff]
    FH_UINT16 u16ActInttMax;     // 策略运行生效的inttmax，以行为单位,非慢快门下不大于u16SensorInttMax | [0x0-0xffff]
    FH_UINT16 u16SensorInttMax;  // Sensor支持的inttmax，以行为单位，不考虑慢快门情况,表示的是一帧之内的最大曝光值 | [0x0-0xffff]
} AE_USER_STATUS;

/**|AWB|**/
typedef struct _ISP_AWB_GAIN_S_ {
    FH_UINT16 u16Rgain;  // 手动R通道增益，U.9 | [0x0-0x1fff]
    FH_UINT16 u16Ggain;  // 手动G通道增益，U.9 | [0x0-0x1fff]
    FH_UINT16 u16Bgain;  // 手动B通道增益，U.9 | [0x0-0x1fff]
} ISP_AWB_GAIN;
typedef struct _AWB_ENABLE_CFG_S_
{
    FH_BOOL bAwbEn;// Awb总使能 | [0x0~0x1]
    FH_BOOL bMwbEn;// 手动wb使能，当为1时配置手动R,G,B增益值，当为0时根据统计实时调整 | [0x0~0x1]
    FH_BOOL bStatSel;// 统计模式选择:0:gray world 1:硬件筛选统计 | [0x0~0x1]
    FH_BOOL bStatMappingEn;// 统计自适应选择:0:统计阈值固定 1:统计阈值随亮度场景变化 | [0x0~0x1]
    FH_BOOL bZoneSel;// 软件筛选白点框模式选择:0:软件筛选白点统计区域自动生成 1:软件筛选白点统计区域用户配置 | [0x0~0x1]
    FH_BOOL bZoneWeightMappingEn;// 软件筛选白点框权重模式选择:0:权重固定,由IndoorDZWeight决定 1:权重根据亮度场景联动 | [0x0~0x1]
    FH_BOOL bWinWeightEn;// 统计分块权重使能 | [0x0~0x1]
    FH_BOOL bLightWeightEn;// 亮度是否影响权重使能 | [0x0~0x1]
    FH_BOOL bInOutEn;// 室内外场景判断使能 | [0x0~0x1]
    FH_BOOL bPureColorEn;// 单色物体判断使能 | [0x0~0x1]
    FH_BOOL bMultiCTEn;// 混合色温使能 | [0x0~0x1]
    FH_BOOL bSpecWPEn;// 特殊色温点使能 | [0x0~0x1]
    FH_BOOL bFineTuneEn;// 精细调整使能，目前主要用于室内肤色保护 | [0x0~0x1]
    FH_BOOL bOutdoorEnhanceEn;// 室外蓝天绿树场景判断使能 | [0x0~0x1]
    FH_BOOL bManualCTEn;// 手动CT使能，当为1时根据配置的白点位置计算白平衡增益；当为0时根据实际统计实时调整 | [0x0~0x1]
    FH_BOOL bCompEn;// 增益补偿使能 | [0x0~0x1]
    FH_BOOL bMinMaxSel;// 0：增益计算采用最小值 1：增益计算采用最大值 | [0x0~0x1]
    FH_BOOL bMinNormEn;// 增益计算采用最小值时，是否做亮度归一化的使能 | [0x0~0x1]
    FH_BOOL bRangeWpEn;// 色温clip功能 0:超色温上下限场景不做clip处理, 1:对超色温上下限场景增益进行限制| [0x0~0x1]
    FH_BOOL bWdrSpecCtrlEn;// 特殊wdr sensor处理 0:长短帧标定白点位置相同 1:长短帧标定白点位置不同,需额外配置短帧白点| [0x0~0x1]
    FH_UINT8 u08AwbAlgType;// 0：Global Alg 1：Advanced Alg 2: low cost Alg| [0x0~0x2]
} AWB_ENABLE_CFG;

typedef struct _AWB_WHITE_POINT_S_
{
    FH_UINT16 u16BOverG;  // 标定白点的B/G分量 | [0~0xffff]
    FH_UINT16 u16ROverG;  // 标定白点的R/G分量 | [0~0xffff]
} AWB_WHITE_POINT;

typedef struct _AWB_WHITE_POINT_UV_S_
{
    FH_UINT16 u16U;  // 标定白点的u分量 | [0~0xfff]
    FH_UINT16 u16V;  // 标定白点的v分量 | [0~0xfff]
} AWB_WHITE_POINT_UV;

typedef struct _STAT_WHITE_POINT_S_
{
    FH_UINT16 u16Coordinate_w;  // awblib白框横坐标U4.8 | [0~0xfff]
    FH_UINT16 u16Coordinate_h;  // awblib白框纵坐标U4.8 | [0~0xfff]
} STAT_WHITE_POINT;

typedef struct _AWB_CALIB_CFG_S_
{
    FH_SINT16 s16Matrix[9];           // 标定用3*3转换矩阵参数 | [0x0~0xffff]
    FH_SINT16 s16StandardWPM[6];      // 6个标记白点的参数M | [0x0~0xffff]
    FH_UINT16 u16StandardWPK[6];      // 6个标记白点的参数K | [0x0~0xffff]
    FH_UINT16 u16StandardWPCCT[6];      // 6个标记白点的计算色温 | [0x0~0xffff]
    AWB_WHITE_POINT_UV stWhitePointUV[6];  // 标定白点的UV结构体 | [AWB_WHITE_POINT_UV]
    AWB_WHITE_POINT stWhitePointLF[6];  // 长帧标定白点的结构体,默认都配置长帧白点 | [AWB_WHITE_POINT]
    AWB_WHITE_POINT stWhitePointSF[6];  // 短帧标定白点的结构体,仅wdrSpecCtrlEn开启后生效 | [AWB_WHITE_POINT]
} AWB_CALIB_CFG;

typedef struct _AWB_LIGHT_CFG_S_
{
    FH_BOOL bLightWeightThMode;    // 0：内置门限值表   1：用户自定义门限值 | [0x0~0x1]
    FH_UINT8 u08LightWeightTh[6];  // 亮度阈值, 单调递增 | [0x0~0xff]
    FH_UINT8 u08LightWeight[6];    // 对应亮度阈值的亮度权重 | [0x0~0xff]
} AWB_LIGHT_CFG;

typedef struct _AWB_SPEC_CT_CFG_S_
{
    FH_BOOL bSpecWpEn;// 特殊白点使能 | [0x0~0x1]
    FH_BOOL bSpecWpMode;// 0:新增白点 1:删除白点 | [0x0~0x1]
    FH_BOOL bSpecWpType;// 0:三角形 1:矩形 | [0x0~0x1]
    AWB_WHITE_POINT stPoint[3];  // 3个顶点的横纵坐标X,Y U4.12 | [AWB_WHITE_POINT]
} AWB_SPEC_CT_CFG;

typedef struct _AWB_RANGE_WP_CFG_S_
{
    FH_BOOL bRangeWpType; // 超色温限制模式:0,自动计算时直接映射到极限白点处 1,手动配置增益阈值 | [0x0~0x1]
    FH_UINT16 u16LowCCT;  // 低色温限制阈值,仅在自动模式下生效,建议0~3000K | [0x0~0xfff]
    FH_UINT16 u16HighCCT;  // 高色温限制阈值,仅在自动模式下生效,建议7000~20000K | [0x0~0x3fff]
    FH_UINT16 u16BgLimitLow;  // 低色温下bg分量下限 | [0x0~0xffff]
    FH_UINT16 u16RgLimitHigh;  // 低色温下rg分量上限 | [0x0~0xffff]
    FH_UINT16 u16BgLimitHigh;  // 高色温下bg分量上限 | [0x0~0xffff]
    FH_UINT16 u16RgLimitLow;  // 高色温下rg分量下限 | [0x0~0xffff]
} AWB_RANGE_WP_CFG;
typedef struct _AWB_MANUAL_CFG_S_
{
    ISP_AWB_GAIN stAwbGain;  // wb手动增益值 | [ISP_AWB_GAIN]
} AWB_MANUAL_CFG;

typedef struct _AWB_COMP_CFG_S_
{
    FH_UINT8 u08REnhance;  // R通道增益补偿，U.6 | [0x0-0xff]
    FH_UINT8 u08BEnhance;  // B通道增益补偿，U.6 | [0x0-0xff]
    FH_UINT8 u08GEnhance;  // G通道增益补偿，U.6 | [0x0-0xff]
} AWB_COMP_CFG;

typedef struct _AWB_STAT_TH_CFG_S_
{
    FH_UINT8 u08StatThh;            // 统计像素亮度阈值上限，亮度按255归一化 | [0x0-0xff]
    FH_UINT8 u08StatThl;            // 统计像素亮度阈值下限，亮度按255归一化 | [0x0-0xff]
    FH_UINT8 u08StatThlOutdoor;     // 室外的统计像素亮度阈值下限，亮度按255归一化 | [0x0-0xff]
    FH_UINT8 u08StatThhOutdoor;     // 室外的统计像素亮度阈值上限，亮度按255归一化 | [0x0-0xff]
    FH_UINT8 u08StatThlLowlight;    // 低亮时统计阈值下限，按255归一化 | [0x0~0xff]
    FH_UINT8 u08StatThhLowlight;    // 低亮时统计阈值上限，按255归一化 | [0x0~0xff]
} AWB_STAT_TH_CFG;
typedef struct _AWB_LUX_SCENE_CFG_S_
{
    FH_BOOL  bInOutType;            // 场景判断模式,0为自动判断 1为手动配置亮度场景 | [0x0~0x1]
    FH_UINT8 u08ManualType;            // 手动配置场景类型: 0,室内 1,室外 2,低照度 | [0x0~0x2]
    FH_UINT16 u16InDoorThresh;     // 判断正常照度和室外的曝光时间阈值,大于该阈值室内概率100%,单位为us | [0x0~0xffff]
    FH_UINT16 u16OutDoorThresh;     // 判断正常照度和室外的曝光时间阈值,小于该阈值室外概率100%,单位为us | [0x0~0xffff]
    FH_UINT16 u16NormalLightThresh;    // 判断正常照度和低照度的曝光增益阈值,小于该阈值室内概率100%,U.4 | [0x0~0xffff]
    FH_UINT16 u16LowLightThresh;    // 判断正常照度和低照度的曝光增益阈值,大于该阈值低照度概率100%,U.4 | [0x0~0xffff]
} AWB_LUX_SCENE_CFG;

typedef struct _AWB_AUTO_ZONE_CFG_S_
{
    FH_BOOL bAutoZoneWidthType;         // 软件自动生成色温框的方式,0:左右边界宽度相同且不同白点宽度相同 1:宽度由白点位置计算得出 | [0x0~0x1]
    FH_UINT8 u08ColorTempNum;           // 软件自动生成色温框的个数,支持4~16个 | [0x4~0x10]
    FH_UINT8 u08AutoZoneWidthThl;     // 软件自动生成色温框的宽度下阈值,仅在autoZoneWidthType为0时生效,U.6 | [0x0~0xff]
    FH_UINT8 u08AutoZoneWidthThh;     // 软件自动生成色温框的宽度上阈值,仅在autoZoneWidthType为0时生效,U.6 | [0x0~0xff]
    FH_UINT8 u08ZoneLWidthThl[6];     // 软件自动生成色温框的左边界宽度下阈值,仅在autoZoneWidthType为1时生效,U.6 | [0x0~0xff]
    FH_UINT8 u08ZoneLWidthThh[6];     // 软件自动生成色温框的左边界宽度上阈值,仅在autoZoneWidthType为1时生效,U.6 | [0x0~0xff]
    FH_UINT8 u08ZoneRWidthThl[6];     // 软件自动生成色温框的右边界宽度下阈值,仅在autoZoneWidthType为1时生效,U.6 | [0x0~0xff]
    FH_UINT8 u08ZoneRWidthThh[6];     // 软件自动生成色温框的右边界宽度上阈值,仅在autoZoneWidthType为1时生效,U.6 | [0x0~0xff]
    FH_UINT16 u16ZoneGainThl;         // 判断自动软件框使用宽度l的增益下阈值,小于该阈值软件框宽度由对应的thl决定,U12.4 | [0x0~0xffff]
    FH_UINT16 u16ZoneGainThh;         // 判断自动软件框使用宽度h的增益上阈值,大于该阈值软件框宽度由对应的thh决定,U12.4 | [0x0~0xffff]
}AWB_AUTO_ZONE_CFG;

typedef struct _AWB_DZONE_S_
{
    FH_UINT8 u08DZtype;              // 每个检测框的类型 0:三角形 1:矩形 2:任意四边形 | [0x0~0x2]
    AWB_WHITE_POINT stPoint[4];   // 每个检测框最多4个顶点,x,y U4.12 | [AWB_WHITE_POINT]
} AWB_DZONE;

typedef struct _AWB_MZONE_CFG_S_
{
    FH_BOOL bZoneType;                // 手动配置软件筛选框的模式 0:库里内置 1:外部传入手动配置 | [0x0~0x1]
    FH_UINT8 u08ZoneNum;              // 手动配置软件筛选框的个数,最大16个,zonetype为0时,这个参数只读 | [0x1~0x10]
    AWB_DZONE stDZone[16];          // 手动配置的软件筛选框 | [AWB_DZONE]
} AWB_MZONE_CFG;

typedef struct _AWB_ZONE_WEIGHT_CFG_S_
{
    FH_UINT8 u08IndoorDZWeight[16];   // 软件筛选框在indoor模式下的权重,最多16个 | [0x0~0xff]
    FH_UINT8 u08OutdoorDZWeight[16];   // 软件筛选框在Outdoor模式下的权重,最多16个 | [0x0~0xff]
    FH_UINT8 u08LowNightDZWeight[16];   // 软件筛选框在LowNight模式下的权重,最多16个 | [0x0~0xff]
} AWB_ZONE_WEIGHT_CFG;

typedef struct _AWB_MULTI_CT_CFG_S_
{
    FH_BOOL  bCTWeightType;           // 混光权重选择 0:从indoor获取 1:根据亮度场景计算 | [0x0~0x1]
    FH_UINT8 u08MultiSatScaler;       // 饱和度调整比例,值越小饱和度越低,255表示完全不降饱和度 | [0x0~0xff]
    FH_UINT8 u08IndoorCTWeight[10];   // 不同色温在indoor模式下的权重 | [0x0~0xff]
    FH_UINT8 u08OutdoorCTWeight[10];   // 不同色温在Outdoor模式下的权重 | [0x0~0xff]
    FH_UINT8 u08LowNightCTWeight[10];   // 不同色温在LowNight模式下的权重 | [0x0~0xff]
} AWB_MULTI_CT_CFG;

typedef struct _AWB_PURE_CFG_S_
{
    FH_UINT8 u08LumaDiffRatioTh;    // 单色亮度差异比例门限值，差异值与平均值的比例小于此值则亮度符合纯色判断，反之则不符合 | [0x0~0x64]
    FH_UINT8 u08ChromaDiffRatioTh;  // 单色色度差异比例门限值，差异值与平均值的比例小于此值则色度符合纯色判断，反之则不符合 | [0x0~0x64]
} AWB_PURE_CFG;

typedef struct _AWB_DEFAULT_CFG_S_
{
    AWB_ENABLE_CFG stAwbEnableCfg;  // awb使能配置 | [AWB_ENABLE_CFG]
    FH_UINT16 u16Speed;             // 值越大，调整速度越快 | [0x0~0xfff]
    FH_UINT8 u08fineTuneStr;        // 精细调整强度,值越大保护越强,过大导致不稳定,建议暂时配置为0 | [0x0~0xff]
    STAT_WHITE_POINT stPoint[6];      //  awb白框点,A，B，C，D，E，F六个点，Ax=Bx By=Cy Dy=Ey Ex=Fx | [STAT_WHITE_POINT]
    AWB_STAT_TH_CFG stAwbStatThCfg; // awb统计硬件阈值设置 | [AWB_STAT_TH_CFG]
    FH_UINT8 u08WeightBlk[1024];     // 统计分块权重设置（仅winWeightEn =1生效） | [0x0~0xff]
    AWB_LIGHT_CFG stAwbLightCfg;    // awb亮度阈值及权重配置 | [AWB_LIGHT_CFG]
    AWB_LUX_SCENE_CFG stAwbLuxSceneCfg;    // awb亮度场景判断配置 | [AWB_LUX_SCENE_CFG]
    AWB_AUTO_ZONE_CFG stAwbAutoZoneCfg;     // awb自动生成软件筛选框配置 | [AWB_AUTO_ZONE_CFG]
    AWB_MZONE_CFG stAwbMZoneCfg;     // awb手动配置软件筛选框配置 | [AWB_MZONE_CFG]
    AWB_ZONE_WEIGHT_CFG stAwbZoneWeightCfg;// awb软件筛选框权重配置 | [AWB_ZONE_WEIGHT_CFG]
    AWB_MULTI_CT_CFG stAwbMultiCTCfg;// awb混色温配置 | [AWB_MULTI_CT_CFG]
    AWB_PURE_CFG stAwbPureCfg;       // awb纯色配置 | [AWB_PURE_CFG]
    AWB_SPEC_CT_CFG stAwbSpecCtCfg[4];   // 最多4组awb特殊色温参数配置 | [AWB_SPEC_CT_CFG]
    AWB_RANGE_WP_CFG stAwbRangeWPCfg;   // 超色温限制配置 | [AWB_RANGE_WP_CFG]
    AWB_MANUAL_CFG stAwbManualCfg;  // awb手动配置 | [AWB_MANUAL_CFG]
    AWB_WHITE_POINT stManualWhitePoint; // 手动CT白点 | [AWB_WHITE_POINT]
    AWB_COMP_CFG stAwbCompCfg;      // awb补偿增益配置 | [AWB_COMP_CFG]
    AWB_CALIB_CFG stAwbCalibCfg;    // awb标定配置 | [AWB_CALIB_CFG]
} AWB_DEFAULT_CFG;

typedef struct _ISP_AWB_STATUS_S_
{
    FH_UINT16 u16RGain;// 红色增益（U4.6） | [0x0~0x3ff]
    FH_UINT16 u16GGain;// 绿色增益（U4.6） | [0x0~0x3ff]
    FH_UINT16 u16BGain;// 蓝色增益（U4.6） | [0x0~0x3ff]
    FH_UINT16 u16CurrBOverG;// 当前B/G(水平坐标，U4.12) | [0x0~0xffff]
    FH_UINT16 u16CurrROverG;// 当前R/G(垂直坐标，U4.12) | [0x0~0xffff]
    FH_UINT16 u16RAvg;// 当前统计红色平均值（U10） | [0x0~0x3ff]
    FH_UINT16 u16GAvg;// 当前统计绿色平均值（U10） | [0x0~0x3ff]
    FH_UINT16 u16BAvg;// 当前统计蓝色平均值（U10） | [0x0~0x3ff]
    FH_UINT8 u08Pos;// 当前色温位置 0：最靠近左侧统计点 255：最靠近右侧统计点 | [0x0~0xff]
    FH_UINT8 u08CurrLeft;// 当前色温左侧统计点index | [0x0~0xf]
    FH_UINT8 u08CurrRight;// 当前色温右侧统计点index | [0x0~0xf]
    FH_UINT16 u16CurrCCT;// 当前帧计算CCT | [0x0~0xffff]
    FH_UINT8 u08UnknownProb;// 当前为不确定场景的概率，单位为百分比 | [0x0~0x64]
    FH_UINT8 u08PureColorProb;// 当前为纯色场景的概率，单位为百分比 | [0x0~0x64]
    FH_UINT8 u08OutdoorProb;// 当前为室外场景的概率，单位为百分比 | [0x0~0x64]
    FH_UINT8 u08LowLightProb;// 当前为低照度场景的概率，单位为百分比 | [0x0~0x64]
    FH_UINT8 u08IllumiantSingleProb;// 当前为单色温场景的概率，单位为百分比 | [0x0~0x64]
    FH_UINT8 u08IllumiantMultiProb;// 当前为混合色温场景的概率，单位为百分比 | [0x0~0x64]
    FH_UINT8 u08GreenTreeProb;// 当前为室外绿地的概率，单位为百分比 | [0x0~0x64]
    FH_UINT8 u08BlueSkyProb;// 当前为室外蓝天的概率，单位为百分比 | [0x0~0x64]
    FH_UINT8 u08Ls0Area;// 当前主色温的占比，单位为百分比 | [0x0~0x64]
    FH_UINT8 u08Ls1Area;// 当前次色温的占比，单位为百分比 | [0x0~0x64]
    FH_UINT16 u16Ls0CCT;// 当前为主色温的计算色温值 | [0x0~0xffff]
    FH_UINT16 u16Ls1CCT;// 当前为次色温的计算色温值 | [0x0~0xffff]
    FH_UINT16 u16CtNum[16];// 色温0的blk数量 | [0x0~0x400]
    FH_UINT16 u16CtSpecNum[4];// 特殊白点0的blk数量 | [0x0~0x400]
    FH_UINT16 u16X[64];// 色温框坐标x0,U4.12(工具显示色温框使用) | [0x0~0xffff]
    FH_UINT16 u16Y[64];// 色温框坐标y0 U4.12 (工具显示色温框使用) | [0x0~0xffff]
    FH_UINT16 u16WpX[1024];// 白块0坐标x0 | [0x0~0xffff]
    FH_UINT16 u16WpY[1024];// 白块0坐标y0 | [0x0~0xffff]
    FH_UINT16 u16Cnt[1024];// 有效白块0数量 | [0x0~0xffff]
} ISP_AWB_STATUS;

/**|CCM|**/
typedef struct _CCM_TABLE_S_
{
    FH_SINT16 s16CcmTable[4][12];  // ccm矩阵表 | [-4096~0xfff]
} CCM_TABLE;

typedef struct _ISP_CCM_CFG_S_ {
    FH_SINT16 s16CcmTable[12];  // ccm矩阵表 | [-4096~0xfff]
} ISP_CCM_CFG;

typedef struct _ISP_ACR_CFG_S_
{
    FH_BOOL bAcrEn;// Acr控制使能 | [0x0~0x1]
    FH_UINT16 u16AcrGainL;// 色彩矫正(ccm)弱化控制增益下限 | [0x0~0xffff]
    FH_UINT16 u16AcrGainH;// 色彩矫正(ccm)弱化控制增益上限 | [0x0~0xffff]
} ISP_ACR_CFG;

/**|GAMMA|**/
typedef enum IPB_GAMMA_MODE_S_ {
    GAMMA_OFF       = 0, // 关闭GAMMA
    GAMMA_MODE_YC   = 1, // GAMMA采用YC模式
    GAMMA_MODE_RGB  = 2, // GAMMA采用RGB模式
    GAMMA_MODE_RGBY = 3, // GAMMA采用RGBY模式
    IPB_GAMMA_MODE_DUMMY =0xffffffff,
} IPB_GAMMA_MODE;

typedef enum _GAMMA_BUILTIN_IDX_ {
    GAMMA_CURVE_10 = 0, // GAMMA曲线1.0
    GAMMA_CURVE_12 = 1, // GAMMA曲线1.2
    GAMMA_CURVE_14 = 2, // GAMMA曲线1.4
    GAMMA_CURVE_16 = 3, // GAMMA曲线1.6
    GAMMA_CURVE_18 = 4, // GAMMA曲线1.8
    GAMMA_CURVE_20 = 5, // GAMMA曲线2.0
    GAMMA_CURVE_22 = 6, // GAMMA曲线2.2
    GAMMA_CURVE_24 = 7, // GAMMA曲线2.4
    GAMMA_CURVE_26 = 8, // GAMMA曲线2.6
    GAMMA_CURVE_28 = 9, // GAMMA曲线2.8
    GAMMA_BUILTIN_IDX_DUMMY =0xffffffff,
} GAMMA_BUILTIN_IDX;

typedef struct _ISP_GAMMA_CFG_S_
{
    FH_BOOL        bGammaEn;    // gamma控制使能 | [0~1]
    IPB_GAMMA_MODE u8GammaMode; // GAMMA模块输入选择	0: gamma off	1: YC mode	2: RGB mode	3: RGBY mode |[0-0x3]
    CURVE_TYPE        eCCurveType;       // c gamma控制模式:0 内置gamma表;1 用户自定义gamma表 | [0~1]
    GAMMA_BUILTIN_IDX eCGammaBuiltInIdx; // c gamma控制模式等于0时有效，内置cgamma表索引 | [0~0xf]
    FH_UINT16         u16CGamma[1024];    // 用户自定义cgamma表 | [0~0x3ff]
    CURVE_TYPE        eYCurveType;       // y gamma控制模式:0 内置gamma表;1 用户自定义gamma表 | [0~1]
    GAMMA_BUILTIN_IDX eYGammaBuiltInIdx; // y gamma控制模式等于0时有效，内置ygamma表索引 | [0~0xf]
    FH_UINT16         u16YGamma[1024];    // 用户自定义ygamma表 | [0~0x3ff]
} ISP_GAMMA_CFG;
/**|BLC|**/
typedef struct _ISP_BLC_CFG_S_
{
    FH_BOOL bBlcEn;// Blc控制使能 | [0x0~0x1]
    FH_BOOL bMode;// 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_BOOL bChannelMode;// 0:所有通道黑电平值 1: 分通道黑电平值 | [0x0~0x1]
    FH_BOOL bBlcDeltaModeEn;// Blc delta mode使能： 0: 关闭 1：开启 | [0x0~0x1]
    FH_BOOL bBlcSfEn;// Blc 长短帧是否分开减blc 0:长短帧减相同的BLC 1:长短帧分开减BLC| [0x0~0x1]
    FH_UINT16 u16BlcLevel;// 手动黑电平值 | [0x0~0xffff]
    FH_UINT16 u16BlcSfLevel;// 短帧手动黑点平值 | [0x0~0xffff]
    FH_UINT16 u16BlcChannel[4];// 通道0 blc值 | [0x0~0xffff]
    FH_UINT16 u16BlcMapMap[12];// 增益为0dB的BLC值 | [0x0~0xffff]
    FH_UINT16 u16BlcSfMapMap[12];// 增益为0dB的短帧BLC值 | [0x0~0xffff]
    FH_SINT8 s08BlcDeltaR;// 手动模式 R通道delta值 | [-128~127]
    FH_SINT8 s08BlcDeltaG;// 手动模式 G通道delta值 | [-128~127]
    FH_SINT8 s08BlcDeltaB;// 手动模式 B通道delta值 | [-128~127]
    FH_UINT8 u08BlcDeltaShift;// Blc delta位移，控制blc delta数值位宽，Blc delta mode开启有效 | [0x0~0x7]
    FH_SINT8 s08BlcDeltaRMap[12];// 增益为0db R通道delta值 | [-128~127]
    FH_SINT8 s08BlcDeltaGMap[12];// 增益为0db G通道delta值 | [-128~127]
    FH_SINT8 s08BlcDeltaBMap[12];// 增益为0db B通道delta值 | [-128~127]
} ISP_BLC_CFG;
/**|GB|**/
typedef struct _ISP_GB_CFG_S_
{
    FH_BOOL bGbEn;// Gb控制使能 | [0x0~0x1]
    FH_BOOL bMode;// GB控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_UINT8 u08SensiThl;// GB敏感度下阈值，值越大表示边界上的校正强度也越大U8，sensiThl<sensiThh | [0x0~0xff]
    FH_UINT8 u08SensiThh;// GB敏感度上阈值，值越大表示边界上的校正强度也越大U8 | [0x0~0xff]
    FH_UINT8 u08GbTh;// G通道差异阈值，值越小整体校正强度越大U8 | [0x0~0xff]
    FH_UINT8 u08GbDiffTh0;// GB阈值0 用于坏点和GB同时存在，坏点周围点G校正门限，越小校正力度越大U8 | [0x0~0xff]
    FH_UINT8 u08GbDiffTh1;// GB阈值1 用于坏点和GB同时存在，坏点周围点G校正门限，越小校正力度越大U8，一般gbDiffTh1>gbDiffTh0 | [0x0~0xff]
    FH_UINT8 u08SensiThhMap[12];// 0db阈值上限U8 | [0x0~0xff]
    FH_UINT8 u08SensiThlMap[12];// 0db阈值下限U8 | [0x0~0xff]
    FH_UINT8 u08GbThMap[12];// 0db G通道差异阈值U8 | [0x0~0xff]
} ISP_GB_CFG;
/**|DPC|**/
typedef struct _ISP_DPC_CFG_S_
{
    FH_BOOL bDpcEn;// Dpc控制使能 | [0x0~0x1]
    FH_BOOL bMode;// 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_BOOL bCtrlMode;// 去坏点控制模式： 0：单个坏点校正 1：多个坏点校正 | [0x0~0x1]
    FH_UINT8 u08Enable;// 0：关闭去坏点 1：打开去白点功能 2：打开去黑点功能 3：同时打开去白点黑点功能 | [0x0~0x3]
    FH_BOOL bMedBlendEn;// 平滑使能 | [0x0~0x1]
    FH_BOOL bSupTwinkleEn;// 闪烁抑制使能 | [0x0~0x1]
    FH_UINT8 u08DpcWTh0;// 白点阈值0，用于单个及多个坏点模式，值越小，去坏点强度越大 | [0x0~0xff]
    FH_UINT8 u08DpcWTh1;// 白点阈值1，仅在多个坏点下生效，值越小，去坏点强度越大 | [0x0~0xff]
    FH_UINT8 u08DpcBTh0;// 黑点阈值0，用于单个及多个坏点模式，值越小，去坏点强度越大 | [0x0~0xff]
    FH_UINT8 u08DpcBTh1;// 黑点阈值1，用于单个及多个坏点模式，值越小，去坏点强度越大 | [0x0~0xff]
    FH_UINT8 u08SupTwinkleThl;// 闪烁抑制阈值下限，值越大，抑制强度越大，去坏点也越弱，supTwinkleThl<supTwinkleThh | [0x0~0xff]
    FH_UINT8 u08SupTwinkleThh;// 闪烁抑制阈值上限值越大，抑制强度越大，去坏点也越弱 | [0x0~0xff]
    FH_UINT8 u08MedBlendRatio;// 中值校正与去坏点校正融合比例，值越大，中值校正比例越大 | [0x0~0xff]
    FH_UINT16 u16MedBlendStr;// 中值校正强度，值越大，中值校正强度也越大 | [0x0~0xff]
    FH_UINT8 u08DpcWTh0Map[12];// 0db白点阈值0 | [0x0~0xff]
    FH_UINT8 u08DpcWTh1Map[12];// 0db白点阈值1 | [0x0~0xff]
    FH_UINT8 u08DpcBTh0Map[12];// 0db黑点阈值0 | [0x0~0xff]
    FH_UINT8 u08DpcBTh1Map[12];// 0db黑点阈值1 | [0x0~0xff]
    FH_UINT8 u08MedBlendRatioMap[12];// 0db中值比例 | [0x0~0xff]
    FH_UINT8 u08MedBlendStrMap[12];// 0db中值强度 | [0x0~0xff]
    FH_UINT8 u08CtrlModeMap[12];// 控制模式（增益0dB时） | [0x0~0x1]
    FH_UINT8 u08SupTwinkleThhMap[12];// 0db闪烁抑制阈值上限 | [0x0~0xff]
    FH_UINT8 u08SupTwinkleThlMap[12];// 0db闪烁抑制阈值下限 | [0x0~0xff]
} ISP_DPC_CFG;

/**|LSC|**/
typedef struct _ISP_LSC_CFG_S_
{
    FH_BOOL   bLscEn; // 镜头阴影矫正控制使能 | [0~1]
    FH_BOOL   bMirrorEn; // lsc表mirror开关使能 | [0~1]
    FH_BOOL   bFlippEn; // lsc表flip开关使能 | [0~1]
    FH_BOOL   bTabMergeEn; // 两张lsc表融合开关，仅FH885XV310支持 | [0~1]
    FH_BOOL   bMergeMode; // 融合模式，当bTabMergeEn=1时有效，0：手动设置融合权重；1：根据色温位置自动设置融合权重 | [0~1]
    FH_UINT8  u08MergeWeight; // 融合权重，(Table * weight + TableEx * (255 - weight))/255, 当bTabMergeEn=1且bMergeMode=0时有效 | [0~255]
    FH_UINT8  u08OriginPattern; // origin bayer pattern | [0~3]
    FH_UINT8  u08InvertPattern; // invert(flip or mirror)后的bayer pattern | [0~3]
    char *    pLscTable; // 补偿系数表0 | [0~0xffffffff]
    char *    pLscTableEx; // 补偿系数表1 | [0~0xffffffff]
    char *    pLscTableEx1; // 补偿系数表2 | [0~0xffffffff]
    char *    pLscTableEx2; // 补偿系数表3 | [0~0xffffffff]
} ISP_LSC_CFG;

typedef struct _ISP_LSC_CROP_CFG_S_
{
    FH_UINT16  u16LscHeight; //<Lsc原图像高度 | [0~0x1fff]
    FH_UINT16  u16LscWidth; //<Lsc原图像宽度 | [0~0x1fff]
    FH_UINT16  u16CropHeight; //<Lsc裁剪图像高度 | [0~0x1fff]
    FH_UINT16  u16CropWidth; //<Lsc裁剪图像宽度 | [0~0x1fff]
    FH_UINT16  u16OffsetY; //<Lsc裁剪垂直偏移 | [0~0xfff]
    FH_UINT16  u16OffsetX; //<Lsc裁剪水平偏移 | [0~0xfff]
} ISP_LSC_CROP_CFG;

/**|NR3D|**/
typedef struct _NR3D_STAT_CFG_S_
{
    FH_BOOL bWinMode;// 统计窗口模式选择： 0:窗口长宽一致 1:窗口根据幅面等比例缩放 | [0x0~0x1]
    FH_UINT8 u08SpaceWinSize;// 统计窗口大小配置，值越大窗口越大，模式由winMode决定 | [0x0~0xff]
    FH_UINT8 u08BinWinSize;// 值域窗口大小配置，值越大窗口越大 | [0x0~0xff]
} NR3D_STAT_CFG;
typedef struct _ISP_NR3D_CFG_S_
{
    FH_BOOL bNr3dEn;// Nr3d控制使能 | [0x0~0x1]
    FH_BOOL bNr3dMode;// NR3D控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_BOOL bInttMappingEn;// NR3D gain mapping控制模式 0: intt mapping关闭 1: intt mapping开启 | [0x0~0x1]
    FH_BOOL bCoeffMode;// 0: 人工痕迹较1少，拖影概率较１高　1:反之
    FH_BOOL bShowCoefEn;// 标定3d时显示使能开关 | [0x0~0x1]
    FH_BOOL bPresetMap;// 0: 采用内置coeff映射表（由coeff_MAP_idx决定） 1：采用自定义coeff映射表 | [0x0~0x1]
    FH_UINT8 u08CoeffMapIdx;// coeff映射曲线: 值越大，斜率越缓, 运动物体越虚，但运动过渡自然 | [0x0~0x7]
    FH_UINT8 u08CoeffYLutIdx;// 噪声估计随亮度LUT映射线，用于低照下控制暗区噪声估计，值越大暗区coeff越大，默认值0 | [0x0~0x3]
    FH_UINT8 u08Nr3dCoeffSel;// nr3d coeff的迭代增益 0时用的是当前帧 值越大迭代越多，拖尾越大 | [0x0~0x1f]
    NR3D_STAT_CFG stNr3dStatCfg;// Nr3d统计窗口配置 | [NR3D_STAT_CFG]
    FH_UINT8 u08RefCoeffRatio;// 参考帧coeff比例因子，值越小收敛越快 | [0x0~0x1f]
    FH_UINT8 u08MdCoeffTh;// 运动coeff门限值，小于此门限认为是静止，与coeff映射线配合使用 | [0x0~0xff]
    FH_UINT16 u16LfSlope;// 长帧噪声估计斜率，主要控制亮出噪声 | [0x0~0xffff]
    FH_UINT8 u08LfSlopeShift;// 长帧噪声估计斜率的小数位移U4 | [0x0~0xf]
    FH_UINT16 u16SfSlope;// 短帧噪声估计斜率，主要控制亮出噪声 | [0x0~0xffff]
    FH_UINT8 u08SfSlopeShift;// 短帧噪声估计斜率的小数位移U4 | [0x0~0xf]
    FH_UINT16 u16LfOffset;// 长帧噪声估计偏移U16 | [0x0~0xffff]
    FH_UINT8 u08LfOffsetShift;// 长帧噪声估计偏移的位移U4 | [0x0~0xf]
    FH_UINT16 u16SfOffset;// 短帧噪声估计偏移U16 | [0x0~0xffff]
    FH_UINT8 u08SfOffsetShift;// 短帧噪声估计偏移的位移U4 | [0x0~0xf]
    FH_UINT16 u16MdGain;// 运动强度增益 | [0x0~0xffff]
    FH_UINT8 u08MdFltCof;// 运动信息滤波比例，值越大越倾向于判断成运动。 | [0x0~0xff]
    FH_UINT8 u08RefCoeffTh;// 参考帧运动信息阈值，将参考帧coeff限制在refCoeffTh之上，值越大，nr3d强度越弱，噪声越大 | [0x0~0xff]
    FH_UINT8 u08Nr3dCofSel[12];// 0db时，3d coeff迭代增益 | [0x0~0xff]
    FH_UINT8 u08RefCoeffRatioMap[12];// 0db时，coeff比例因子 | [0x0~0xff]
    FH_UINT8 u08MdFltCofMap[12];// 0db时，运动信息滤波比例 | [0x0~0xff]
    FH_UINT8 u08RefCoeffThMap[12];// 0db时，参考帧运动coeff最小值 | [0x0~0xff]
    FH_UINT16 u16MdGainInttMap[8];// 1/256 inttMax 时，运动估计强度 | [0x0~0xffff]
    FH_UINT16 u16MdGainMap[12];// 0db，运动估计强度 | [0x0~0xffff]
    FH_UINT16 u16LfSlopeMap[12];// 0db，长帧噪声估计斜率 | [0x0~0xffff]
    FH_UINT16 u16LfOffsetMap[12];// 0db，长帧噪声估计偏移 | [0x0~0xffff]
    FH_UINT16 u16SfSlopeMap[12];// 0db，短帧噪声估计斜率 | [0x0~0xffff]
    FH_UINT16 u16SfOffsetMap[12];// 0db，短帧噪声估计偏移 | [0x0~0xffff]
    FH_UINT8 u08CoeffIdxMap[12];// 不同增益时的coeff映射线 | [0x0-0x1]
    FH_UINT8 u08CoeffModeMap[12];// 不同增益时的coeff处理方式 | [0x0-0x1]
    FH_UINT8 u08CoeffYLutIdxMap[12];// 不同增益时的噪声估计随亮度LUT映射线 | [0x0-0x3]
} ISP_NR3D_CFG;

typedef struct _ISP_NR3D_COEFF_CFG_
{
    FH_UINT16 u16CoeffY[27]; // NR3D Coeff 26段映射线的折点纵坐标。Y0<=Y1...<=Y26 | [0~0x3ff]
} ISP_NR3D_COEFF_CFG;

typedef struct _ISP_NR3D_DC_RATE_
{
    FH_UINT16 u16ActWth;    // NR3D实际输出的一行像素点个数 | [0~0xffff]
    FH_UINT16 u16TarWth;   // NR3D预期一行输出的周期数，NR3D后的模块每行将按照该周期输出,该值和ISP频率有关 | [u16ActWth+1~0xffff]
} ISP_NR3D_DC_RATE;

/**|LTM|**/
typedef struct _ISP_LTM_CFG_S_
{
    FH_BOOL    bLtmEn;               // 线性增益模式:0 手动,1 gainMapping | [0~1]
    CURVE_TYPE eCurveType;           //曲线预设类型 | [1~2]
    FH_UINT32  u32TonemapCurve[128]; // ltm曲线设置表格 | [1~0xffffffff]
    FH_UINT32  u32LtmIdx;            //当mode为normal模式时：	0~14: 内部表系数	15:用户定义 | [0-0xf]
    FH_BOOL bMode;                   // 0：宽动态模式	1：非宽动态模式 | [0~1]
    FH_BOOL  bLtmHwEn;               // Ltm硬件模块开关 | [0-0x1]
    FH_BOOL  bYPreScalerEn;          //高亮时正常亮度和高光处比例系数设置使能 | [0-0x1]
    FH_UINT8 u08HlYScaler;           //高光处的亮度比例系数，值越大越亮，U.5 | [0-0x3f]
    FH_UINT8 u08NlYScaler;           //正常亮度处的亮度比例系数，值越大越亮，U.5， | [0-0x3f]
    FH_UINT8 u08YMaxRatio;           //统计得到的像素最大值的补偿比例因子，U.6 | [0-0xff]
    FH_UINT8 u08YMeanRatio;          //统计得到的像素平均值的补偿比例因子，U.6 | [0-0xff]
    FH_UINT8 u08YMinRatio;           //统计得到的像素最小值的补偿比例因子，U.6 | [0-0xff]
    FH_UINT8 u08NlCtr;   //正常亮度处的局部对比度，值越大对比度越大，但噪声也越大 | [0-0xff]
    FH_UINT8 u08Weight1; //正常亮度处新旧LTM融合比例，值越小，新LTM越多 | [0-0xff]
    FH_UINT8 u08Weight3; //高亮处新旧LTM融合比例，值越小，新LTM越多 | [0-0xff]
    FH_UINT8 u08Weight0; //低亮处新旧LTM融合比例，值越小，新LTM越多 | [0-0xff]
    FH_UINT8 u08Weight2; //较亮处新旧LTM融合比例，值越小，新LTM越多 | [0-0xff]
    FH_UINT8 u08NlYScalerMap[GAIN_NODES]; //正常亮度比例系数 | [0-0xff]
    FH_UINT8 u08HlYScalerMap[GAIN_NODES]; //高光处比例系数 | [0-0xff]
    FH_UINT8 u08NlCtrMap[GAIN_NODES];     //局部对比度 | [0-0xff]
    FH_UINT8 u08LtmWinIdxMap[GAIN_NODES]; //window idx选择 | [0-0xf]
    FH_UINT8 u08EpsWdr;                   //宽动态场景下滤波器参数值 | [0-0xf]
    FH_UINT8 u08EpsLinear;                //普通场景下滤波器参数值 | [0-0xf]
    FH_UINT8 u08NlYPreScaler[4];             //(1/16|1/8|1/4|1/2 inttMax)时正常亮度处的亮度比例系数，值越大越亮，U.5， | [0-0x3f]
    FH_UINT8 u08HlYPreScaler[4];             //(1/16|1/8|1/4|1/2 inttMax)时高亮处的亮度比例系数，值越大越亮，U.5， | [0-0x3f]
} ISP_LTM_CFG;
/**|MD|**/
typedef struct _ISP_MD_STAT_S_
{
    FH_UINT32         u32MdEn;    //模块使能状态，状态为1时获取到的值为有效 | [0~1]
    FH_UINT32         u32MdMove;  //模块运动状态统计 | [0~0x180]
    FH_UINT32         u32MdStill; //模块静止状态统计 | [0~0x180]
} ISP_MD_STAT;
/**|NR2D|**/
typedef struct _ISP_LUT_POINT_S_
{
    FH_UINT16 u16X;// 坐标系中X轴坐标值 | [0x0~0xffff]
    FH_UINT16 u16Y;// 坐标系中Y轴坐标值 | [0x0~0xffff]
} ISP_LUT_POINT;
typedef struct _ISP_NR2D_CFG_S_
{
    FH_BOOL bNr2dEn;// Nr2d控制使能 | [0x0~0x1]
    FH_BOOL bNr2dMode;// NR2D 控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_BOOL bInttMappingEn;// NR2D gain mapping控制模式 0: intt mapping关闭 1: intt mapping开启 | [0x0~0x1]
    FH_UINT8 u08RefCoefRatio;// 参考帧2dcoeff比例因子，值越小收敛越快，拖尾越小 | [0x0~0x1f]
    FH_UINT8 u08Nr2dSfSft;// 2d短帧降噪移位,U4 | [0x0~0xf]
    FH_UINT8 u08Nr2dLfSft;// 2d长帧降噪移位,U4 | [0x0~0xf]
    FH_BOOL bSigLutSet;// 内置的SigLut组别选择： 0：第0组； 1：第1组； | [0x0~0x1]
    FH_BOOL bSigLutMode;// 长短帧siglut的曲线模式： 0：采用内置的SigLut, 根据SigLutIdx确定； 1：自定义模式，用户通过LF/SF_SIGLUT_CONFIG0/1/2设置长短帧的sigLut曲线； Range: 0x0-0x1 | [0x0~0x1]
    FH_UINT8 u08LfSiglutIdx;// 长帧siglut的曲线选择，越大去噪越强 | [0x0~0xf]
    FH_UINT8 u08SfSiglutIdx;// 短帧siglut的曲线选择，越大去噪越强 | [0x0~0xf]
    FH_BOOL bNr2dMenEn;// Nr2d联动使能开关 | [0x0~0x1]
    FH_BOOL bMergeLutIdx;// 线性是写1，只用长帧的参数 其他值用于wdr | [0x0~0x1]
    FH_UINT16 u16LfK1;// Long frame: the slope of “slope K”,U8.8 | [0x0~0xffff]
    FH_UINT8 u08LfK2;// Long frame: the offset of “slope K”,U8 | [0x0~0xff]
    FH_SINT32 s32LfO1;// Long frame: the slope of “offset O”,S20.8 | [-134217728~134217727]
    FH_SINT32 s32LfO2;// Long frame: the offset of “offset O”,S25 | [-16777216~16777215]
    FH_UINT16 u16SfK1;// Short frame: the slope of “slope K”,U8.8 | [0x0~0xffff]
    FH_UINT8 u08SfK2;// Short frame: the offset of “slope K”,U8 | [0x0~0xff]
    FH_SINT32 s32SfO1;// Short frame: the slope of “offset O”,S20.8 | [-134217728~134217727]
    FH_SINT32 s32SfO2;// Short frame: the offset of “offset O”,S25 | [-16777216~16777215]
    FH_UINT8 u08SfSlopeStr;// 短帧噪声斜率估计强度，U4.4 | [0x0~0xff]
    FH_UINT8 u08SfOffsetStr;// 短帧噪声偏移估计强度，U4.4 | [0x0~0xff]
    FH_UINT8 u08LfSlopeStr;// 长帧噪声斜率估计强度，U4.4 | [0x0~0xff]
    FH_UINT8 u08LfOffsetStr;// 长帧噪声偏移估计强度，U4.4 | [0x0~0xff]
    FH_UINT8 u08LfSigLutX[6];// 输入coef x0（0-255） x越小，代表越接近静止区 x越大，代表越偏向运动区 x0固定为0 | [0x0~0xff]
    FH_UINT8 u08LfSigLutY[6];// 输出coef y0,越大去噪越强 | [0x0~0xff]
    FH_UINT8 u08SfSigLutX[6];// 输入coef x0（0-255） x0固定为0 | [0x0~0xff]
    FH_UINT8 u08SfSigLutY[6];// 输出coef y0,越大去噪越强 | [0x0~0xff]
    ISP_LUT_POINT stLfAddLut[4];// 长帧回加LUT横坐标coef x(0~255),从0开始依次增大，值越小，代表越接近静止区 x越大，代表越偏向运动区;纵坐标y(0~255),越大去噪越强 | [ISP_LUT_POINT]
    ISP_LUT_POINT stSfAddLut[4];// 短帧回加LUT横坐标coef x(0~255),从0开始依次增大，值越小，代表越接近静止区 x越大，代表越偏向运动区;纵坐标y(0~255),越大去噪越强 | [ISP_LUT_POINT]
    ISP_LUT_POINT stLfLumaLut[4];// 长帧回加亮度LUT横坐标亮度值x（0-4095）从0开始依次增大;LUT表纵坐标y（0-255）,越小去噪强度越强| [ISP_LUT_POINT]
    ISP_LUT_POINT stSfLumaLut[4];// 短帧回加亮度LUT横坐标亮度值x（0-4095）从0开始依次增大;LUT表纵坐标y（0-255）,越小去噪强度越强| [ISP_LUT_POINT]
    FH_UINT8 u08LfSlopeInttStrMap[4];// 1/16 inttMax时，长帧噪声斜率估计强度，U4.4 | [0x0~0xff]
    FH_UINT8 u08LfSlopeStrMap[12];// 0db时，长帧噪声斜率估计强度，U4.4 | [0x0~0xff]
    FH_UINT8 u08SfSlopeInttStrMap[4];// 1/16 inttMax时，短帧噪声斜率估计强度，U4.4 | [0x0~0xff]
    FH_UINT8 u08SfSlopeStrMap[12];// 0db时，短帧噪声斜率估计强度，U4.4 | [0x0~0xff]
    FH_UINT8 u08LfOffsetStrMap[12];// 0db时，长帧噪声偏移估计强度，U4.4 | [0x0~0xff]
    FH_UINT8 u08SfOffsetStrMap[12];// 0db时，短帧噪声偏移估计强度，U4.4 | [0x0~0xff]
    FH_UINT8 u08RefCoeffRatioInttMap[8];// 1/256 inttMax时，coeff比例因子 | [0x0~0xff]
    FH_UINT8 u08RefCoeffRatioMap[12];// 0db时，coeff比例因子 | [0x0~0xff]
    FH_UINT8 u08LfAddLutIdxMap[12];// 0db, 长帧回加曲线选择，值越大去噪越强 | [0x0~0xf]
    FH_UINT8 u08SfAddLutIdxMap[12];// 0db, 短帧回加曲线选择，值越大去噪越强 | [0x0~0xf]
    FH_UINT8 u08LfLumaLutIdxMap[12];// 0db, 长帧根据亮度去噪曲线选择，值越大去噪越强 | [0x0~0xf]
    FH_UINT8 u08SfLumaLutIdxMap[12];// 0db, 短帧根据亮度去噪曲线选择，值越大去噪越强 | [0x0~0xf]
    FH_UINT8 u08LfAddLutIdX1OffsetMap[12];// 0db, 长帧根据Coef去噪曲线X1偏移值，值越大越偏向运动 | [0x0~0xf]
    FH_UINT8 u08LfAddLutIdX2OffsetMap[12];// 0db, 长帧根据Coef去噪曲线X2偏移值，值越大越偏向运动 | [0x0~0xf]
    FH_UINT8 u08LfSiglutIdxMap[16];// 1/16 inttMax, 长帧siglut的曲线选择，越大去噪越强 | [0x0~0xf]
    FH_UINT8 u08SfSiglutIdxMap[16];// 1/16 inttMax, 短帧siglut的曲线选择，越大去噪越强 | [0x0~0xf]
} ISP_NR2D_CFG;
typedef struct _ISP_NR2D_DPC_CFG_S_
{
    FH_BOOL bNr2dDpcMode;// NR2D DPC控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_UINT8 u08Nr2dDpcEn;// 0:关闭去坏点;1:打开去白点功能;2.打开去黑点功能;3.同时打开去白点黑点功能| [0~3]
    FH_UINT8 u08Str;// 手动去坏点强度,U1.3 | [0-0x7]
    FH_UINT8 u08Wdc;// 手动白点门限值 | [0-0xff]
    FH_UINT8 u08Bdc;// 手动黑点门限值 | [0-0xff]
    FH_UINT8 u08StrMap[12];//不同增益下NR2D DPC强度| [0-0x7]
    FH_UINT8 u08WdcMap[12];// 不同增益下白点门限DC值 | [0-0xff]
    FH_UINT8 u08BdcMap[12];// 不同增益下黑点门限值 | [0-0xff]
} ISP_NR2D_DPC_CFG;

/**|DRC|**/
typedef struct _DRC_STAT_CFG_S_
{
    FH_BOOL bWinMode;// 统计窗口模式选择： 0:窗口长宽一致 1:窗口根据幅面等比例缩放 | [0x0~0x1]
    FH_UINT8 u08SpaceWin0Size;// 统计窗口大小配置，值越大窗口越大，模式由winMode决定 | [0x0~0xff]
    FH_UINT16 u16BinWin0Size;// 值域窗口大小配置，值越大窗口越大 | [0x0~0x1ff]
    FH_UINT8 u08SpaceWin1Size;// 统计窗口大小配置，值越大窗口越大，模式由winMode决定 | [0x0~0xff]
    FH_UINT16 u16BinWin1Size;// 值域窗口大小配置，值越大窗口越大 | [0x0~0x1ff]
} DRC_STAT_CFG;
typedef struct _ISP_DRC_CFG_S_
{
    FH_BOOL bDrcEn;// Drc控制使能 | [0x0~0x1]
    FH_BOOL bDrcHwEn;// Drc硬件模块使能： 0：关闭； 1：打开； | [0x0~0x1]
    FH_BOOL bGainMappingEn;// DRC控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_BOOL bInttMappingEn;// DRC gain mapping控制模式 0: intt mapping关闭 1: intt mapping开启 | [0x0~0x1]
    FH_BOOL bSupPosEdgeEn;// 高亮白边校正使能 | [0x0~0x1]
    FH_BOOL bDrcCurveMode;// 0：采用内置drc表，由drc_curve_idx决定 1：采用自定义drc表 | [0x0~0x1]
    FH_BOOL bDrcCurveDarkEn;// drc曲线暗部控制使能： 0：关闭； 1：打开； | [0x0~0x1]
    FH_UINT8 u08DrcCurveIdx;// drc曲线idx | [0x0~0x7]
    FH_UINT8 u08drcLightStatRatio;// 亮部统计使能打开时有效，亮部统计得到最大值的补偿比例因子，U.6 | [0x0~0xff]
    FH_BOOL bDrcLightStatEn;// 亮部统计使能，为drc曲线亮部控制提供统计信息： 0：关闭； 1：打开； | [0x0~0x1]
    FH_BOOL bDrcCurveLightEn;// drc曲线亮部控制使能： 0：关闭； 1：打开； | [0x0~0x1]
    DRC_STAT_CFG stDrcStatCfg;// Drc统计窗口0和窗口1配置 | [DRC_STAT_CFG]
    ISP_LUT_POINT stFilterWgtLut[2];// 滤波输出权重Lut横坐标x（0-0xfff）; 滤波输出权重纵坐标值y(0x0~0xff)，越大越倾向于使用滤波输出结果，通透性好，y0<=y1 | [ISP_LUT_POINT]
    ISP_LUT_POINT stMergeLut[2];// 两路统计merge横坐标x(0x0~0x3fff); 两路统计merge纵坐标y(0x0~0x1ff)，越大Stat1融合越多 | [ISP_LUT_POINT]
    ISP_LUT_POINT stFwLut[3];// 明暗分割Lut横坐标x（0-0xfff）; 明暗分割Lut纵坐标y(0x0~0xff) | [ISP_LUT_POINT]
    FH_UINT8 u08FilterWgtMin;// 滤波输出权重最小值，越大越倾向于使用滤波输出结果，通透性好，u08FilterWgtMin<=u08FilterWgtMax | [0x0~0xff]
    FH_UINT8 u08FilterWgtMax;// 滤波输出权重最大值，越大越倾向于使用滤波输出结果，通透性好 | [0x0~0xff]
    FH_UINT8 u08SupPosEdgeStr;// 高亮白边校正强度，值越大校正越强，也即高亮白边越弱 | [0x0~0xff]
    FH_UINT8 u08GainN;// 暗处抑制曲线的slope | [0x0~0xff]
    FH_UINT8 u08Offset;// 暗处抑制曲线的offset | [0x0~0xff]
    FH_UINT8 u08Factor;// 暗处抑制曲线的线性控制 | [0x0~0xf]
    FH_UINT16 u16LumaDiffTh;// 控制滤波后亮度差，越大亮处抑制越小  | [0x0~0xffff]
    FH_UINT8 u08LcCtr;// 局部对比度调整，最大为64 | [0x0~0x7f]
    FH_UINT8 u08DrcStr;// DRC强度 | [0x0~0xff]
    FH_UINT8 u08GainY;// DRC强度映射增益 | [0x0~0xff]
    FH_UINT8 u08Bright;// 亮度调整参数，值越大，亮度越亮 | [0x0~0xff]
    FH_UINT8 u08DePosDarkGain;// 正向细节暗区增益 | [0x0~0xff]
    FH_UINT8 u08DePosBrightGain;// 正向细节亮区增益 | [0x0~0xff]
    FH_UINT8 u08DeNegDarkGain;// 负向细节暗区增益 | [0x0~0xff]
    FH_UINT8 u08DeNegBrightGain;// 负向细节亮区增益 | [0x0~0xff]
    FH_UINT8 u08DePosMax;// 控制亮细节最大值 | [0x0~0xff]
    FH_UINT8 u08DePosMin;// 控制亮细节最小值 | [0x0~0xff]
    FH_UINT8 u08DePosTh;// 控制亮细节阈值 | [0x0~0xff]
    FH_UINT8 u08DePosSlope;// 控制亮细节斜率 | [0x0~0x7]
    FH_UINT8 u08DeNegMax;// 控制暗细节最大值 | [0x0~0xff]
    FH_UINT8 u08DeNegMin;// 控制暗细节最小值 | [0x0~0xff]
    FH_UINT8 u08DeNegTh;// 控制暗细节阈值 | [0x0~0xff]
    FH_UINT8 u08DeNegSlope;// 控制暗细节斜率 | [0x0~0x7]
    FH_UINT16 u16SupBrightTh;// 亮区抑制门限 | [0x0~0xfff]
    FH_UINT8 u08SupBrightSlope;// 亮区抑制斜率，值越大抑制越弱 | [0x0~0xff]
    FH_UINT8 u08DarkSat;// 暗区饱和度，值越小，暗区饱和度抑制越强 | [0x0~0xff]
    FH_UINT8 u08BrightSat;// 亮区饱和度，值越小，亮区饱和度抑制越强 | [0x0~0xff]
    FH_UINT8 u08LcCtrMap[12];// 0db 局部对比度  | [0x0~0xff]
    FH_UINT8 u08BrightMap[12];// 0db drc亮度 | [0x0~0xff]
    FH_UINT8 u08DrcInttStrMap[4];// 1/16 inttMax时drc亮度强度 | [0x0~0xff]
    FH_UINT8 u08DrcStrMap[12];// 0db drc亮度强度 | [0x0~0xff]
    FH_UINT8 u08DePosDarkGainMap[12];// 0db正向细节暗区增益 | [0x0~0xff]
    FH_UINT8 u08DePosBrightGainMap[12];// 0db正向细节亮区增益 | [0x0~0xff]
    FH_UINT8 u08DeNegDarkGainMap[12];// 0db负向细节暗区增益 | [0x0~0xff]
    FH_UINT8 u08DeNegBrightMap[12];// 0db负向细节亮区增益 | [0x0~0xff]
    FH_UINT8 u08GainNInttMap[4];// 暗处抑制曲线gainN的intt map 0(1/16 inttMax时) | [0x0~0xff]
    FH_UINT8 u08GainNMap[12];// 暗处抑制曲线gainN的gain map 0(0db) | [0x0~0xff]
    FH_UINT8 u08OffsetInttMap[4];// 暗处抑制曲线offset的intt map 0(1/16 inttMax时) | [0x0~0xff]
    FH_UINT8 u08OffsetMap[12];// 暗处抑制曲线offset的gain map 0(0db) | [0x0~0xff]
    FH_UINT8 u08FactorMap[16];// 暗处抑制曲线factor的gain map 0(1/16 inttMax时) | [0x0~0xf]
    FH_UINT8 u08FilterY1InttMap[4];// 滤波输出权重权重纵坐标值y1的intt map 0(1/16 inttMax时) y1<=y2 | [0x0~0xff]
    FH_UINT8 u08FilterY1Map[12];// 滤波输出权重权重纵坐标值y1的gain map 0(0db) y1<=y2 | [0x0~0xff]
    FH_UINT8 u08FilterY2InttMap[4];// 滤波输出权重权重纵坐标值y2的intt map 0(1/16 inttMax时) | [0x0~0xff]
    FH_UINT8 u08FilterY2Map[12];// 滤波输出权重权重纵坐标值y2的gain map 0(0db) | [0x0~0xff]
    FH_UINT8 u08FwY1InttMap[4];// 亮暗分割曲线y1的intt map 1 | [0x0~0xff]
    FH_UINT8 u08FwY1Map[12];// 亮暗分割曲线y1的gain map 0(0db) | [0x0~0xff]
    FH_UINT16 u16MergeX1InttMap[4];// 两路统计融合映射线横坐标X1点(1/16 inttMax时) | [0x0~0xffff]
    FH_UINT16 u16MergeX1Map[12];// 两路统计融合映射线横坐标X1点(0db时) | [0x0~0xffff]
    FH_UINT16 u16MergeX2InttMap[4];// 两路统计融合映射线横坐标X2点(1/16 inttMax时) | [0x0~0xffff]
    FH_UINT16 u16MergeX2Map[12];// 两路统计融合映射线横坐标X2点(0db时) | [0x0~0xffff]
    FH_UINT8 u08DarkSatMap[12];// 0db时暗区饱和度 | [0x0-0xff]
    FH_UINT8 u08BrightSatMap[12];// 0db时亮区饱和度 | [0x0-0xff]
    FH_UINT16 u16Data[386];// 系数0 | [0x0~0xffff]
} ISP_DRC_CFG;
/**|CFA|**/
typedef struct _ISP_CFA_CFG_S_
{
    FH_BOOL bCfaEn;// Cfa控制使能 | [0x0~0x1]
    FH_BOOL bCfaMode;// CFA控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_BOOL bSmoothHwEn;// Smooth硬件开关使能 | [0x0~0x1]
    FH_BOOL bMedHwEn;// 中值滤波硬件开关使能 | [0x0~0x1]
    FH_BOOL bAvgHwEn;// 均值滤波硬件使能 | [0x0~0x1]
    FH_BOOL bFalColEn;// 去高频伪色使能 | [0x0~0x1]
    FH_BOOL bSmoothMode;// Smooth模式：0:两个方向平滑; 1:四个方向平滑 | [0x0~0x1]
    FH_UINT8 u08SmoothThh;// 平滑上阈值，值越小平滑越明显，smoothThh>smoothThl | [0x0~0xff]
    FH_UINT8 u08SmoothThl;// 平滑下阈值，值越小平滑越明显 | [0x0~0xff]
    FH_UINT8 u08FcStr;// 去伪色强度 | [0x0~0xff]
    FH_UINT8 u08FreqThh;// 边缘梯度上阈值，值越小边缘平滑力度越明显，freqThh>freqThl | [0x0~0xff]
    FH_UINT8 u08FreqThl;// 边缘梯度下阈值，值越小边缘平滑力度越明显 | [0x0~0xff]
    FH_UINT8 u08YFilterThh;// 亮度校正上阈值，值越小平滑越明显，分辨率保护力度越弱，yFilterThh>yFilterThl | [0x0~0xff]
    FH_UINT8 u08YFilterThl;// 亮度校正下阈值，值越小平滑越明显，分辨率保护力度越弱， | [0x0~0xff]
    FH_UINT8 u08ConFilterThh;// 对比度校正上阈值，值越小平滑越明显，分辨率保护力度越弱, conFilterThh > conFilterThl | [0x0~0xff]
    FH_UINT8 u08ConFilterThl;// 对比度校正下阈值，值越小平滑越明显，分辨率保护力度越弱, | [0x0~0xff]
    FH_UINT16 u16OffsetT;// 纹理信息偏色量 | [0x0~0xfff]
    FH_UINT16 u16OffsetY;// 亮度信息偏色量 | [0x0~0xfff]
    FH_UINT8 u08SmoothThhMap[12];// 0db平滑上阈值 | [0x0~0xff]
    FH_UINT8 u08SmoothThlMap[12];// 0db平滑下阈值 | [0x0~0xff]
    FH_UINT8 u08FreqThhMap[12];// 0db边缘梯度上阈值 | [0x0~0xff]
    FH_UINT8 u08FreqThlMap[12];// 0db边缘梯度下阈值 | [0x0~0xff]
    FH_UINT8 u08YFilterThhMap[12];// 0db亮度校正上阈值 | [0x0~0xff]
    FH_UINT8 u08YFilterThlMap[12];// 0db亮度校正下阈值 | [0x0~0xff]
    FH_UINT8 u08ConFilterThhMap[12];// 0db亮度校正上阈值 | [0x0~0xff]
    FH_UINT8 u08ConFilterThlMap[12];// 0db亮度校正下阈值 | [0x0~0xff]
} ISP_CFA_CFG;
/**|HLR|**/
typedef struct _ISP_HLR_CFG_S_
{
    FH_BOOL  bHlrEn;   // 高光恢复控制使能 | [0-1]
    FH_SINT8 s08CompLevel;// Hlr补偿因子 S1.7 hlr_val = hlr_val*(128+comp_level)>>7 | [-128~127]
} ISP_HLR_CFG;
/**|DEHAZE|**/
typedef struct _ISP_DEHAZE_CFG_S_
{
    FH_BOOL bDehazeEn;// Dehaze控制使能 | [0x0~0x1]
    FH_BOOL bGainMappingEn;// 去雾控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_BOOL bDehazeHwEn;// Dehaze硬件模块开关 0: 关 1：开 | [0x0~0x1]
    FH_BOOL bPresetLut;// 0：采用内置lut映射表 1：采用自定义lut映射表 | [0x0~0x1]
    FH_UINT8 u08DehazeStr;// 手动模式下总的去雾强度, 值越大去雾力度越强 | [0x0~0xff]
    FH_UINT8 u08DehazeStrMap[12];// 0dB时，去雾强度等级 | [0x0~0xff]
    FH_UINT16 u16DehazeLutData[66];// 根据不同亮度配置去雾强度映射表，共66项，一般情况下，暗处去雾强度弱，亮处去雾强度强。 | [0x0~0xfff]
} ISP_DEHAZE_CFG;

typedef struct _ISP_DE_CFG_S_
{
    FH_BOOL bDarkEnhanceEn;// 暗部增强开关 0: 关 1: 开 | [0x0~0x1]
    FH_UINT8 u08DeStr;// 暗部增强强度 | [0x0~0xff]
    FH_UINT8 u08GainThl;// 暗部增强作用时增益下限，gain_thl>=1最小为1倍增益 | [0x1~0xff]
    FH_UINT8 u08GainThh;// 暗部增强作用时增益上限，必须满足gain_thl<gainthh,建议最大为4倍增益 | [0x1~0xff]
} ISP_DE_CFG;
/**|LCE|**/
typedef struct _ISP_LCE_CFG_S_
{
    FH_BOOL bLceEn;// Lce控制使能 | [0x0~0x1]
    FH_BOOL bLceHwEn;// 硬件开关 0:关闭 1:打开 | [0x0~0x1]
    FH_BOOL bGainMappingEn;// LCE 控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_BOOL bMergeSel;// 结果是否与原始输入融合： 0：不融合 1：融合 | [0x0~0x1]
    FH_BOOL bMergeCurveMode;// 0：内置MERGE表 1：用户自定义MERGE表 | [0x0~0x1]
    FH_UINT8 u08MergeCurveNum;// 仅内置模式下有0-3 共四条曲线 | [0x0~0x3]
    FH_UINT8 u08Kneex1[4];// 权重LUT的第二个点横坐标, 第一个点固定为0, | [0x0~0xff]
    FH_UINT8 u08Kneey[6];// 权重LUT的第一个点纵坐标,  | [0x0~0xff]
    FH_UINT8 u08LceStr;// 局部对比度强度 | [0x0~0xff]
    FH_UINT8 u08CGain;// 色度补偿因子，值越大，饱和度提升越大，U.6 | [0x0~0xff]
    FH_UINT8 u08COffset;// 色度偏移因子，主要针对暗处，值越大，暗处色度补偿越小，对色噪友好 | [0x0~0xff]
    FH_UINT8 u08StrMap[12];// 0dB时，局部对比度强度 | [0x0~0xff]
    FH_UINT8 u08CGainMap[12];// 0dB时，色度补偿因子U.6 | [0x0~0xff]
    FH_UINT8 u08COffsetMap[12];// 0dB时，色度偏移因子 | [0x0~0xff]
} ISP_LCE_CFG;
/**|IE|**/
typedef struct _ISP_CONTRAST_CFG_S_
{
    FH_BOOL  bYcEn;                 // 亮度控制使能 | [0~1]
    FH_BOOL  bGainMappingEn;        // 0：手动	1：增益映射 | [0-1]
    FH_UINT8 u08Crt;                // 手动模式下对比度 U2.6 | [0-0xff]
    FH_UINT8 u08Mid;                // 手动模式下对比度矫正的中心亮度,暂不支持 | [0-0xff]
    FH_UINT8 u08CrtMap[12];         // 不同噪声强度下对比度值U2.6 | [0~0xff]
} ISP_CONTRAST_CFG;
typedef struct _ISP_BRIGHTNESS_CFG_S_
{
    FH_BOOL  bYcEn;          // 亮度控制使能 | [0~1]
    FH_BOOL  bGainMappingEn; // 0：手动	1：增益映射 | [0~1]
    FH_SINT8 s08Brt;         // 手动模式下亮度S8(-128到127) | [-128~127]
    FH_SINT8 s08BrtMap[12];  // 不同噪声强度下亮度S8 | [-128~127]
} ISP_BRIGHTNESS_CFG;
/**|CE|**/
typedef struct _ISP_SAT_CFG_S_
{
    FH_BOOL bSatEn;// Sat控制使能 | [0x0~0x1]

    FH_BOOL bGainMappingEn;        // 饱和度控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_SINT8 s08RollAngle;          // 色度旋转因子，范围为~64°-63°，最高比特为符号位，S7 | [-64~63]
    FH_UINT8 u08Sat;                // 饱和度增益 U3.5 | [0x0~0xff]
    FH_UINT8 u08BlueSup;           // 蓝色抑制增益 U2.6 | [0x0~0xff]
    FH_UINT8 u08RedSup;            // 红色抑制增益 U2.6 | [0x0~0xff]
    FH_BOOL bGlutEn;                // 亮度增益抑制使能： 0: 关闭 1: 开启 | [0x0~0x1]
    FH_BOOL bPresetLut;            // 预设亮度增益抑制LUT 0:采用内置lut映射表； 1：采用自定义lut映射表 | [0x0~0x1]
    FH_BOOL bThreshEn;              // 阈值抑制使能： 0: 关闭 1: 开启 | [0x0~0x1]
    FH_BOOL bThhMode;              // 抑制阈值控制模式: 0:手动模式;  1:gain mapping | [0x0~0x1]
    FH_UINT8 u08ThhSup;             // 抑制阈值:值越小抑制作用越强, U3.5 | [0x0~0xff]
    FH_UINT16 u16SatParaR1;        // 白色门限值，饱和度小于该值的会被设置为0，即成为白色 | [0x0~0x1ff]
    FH_UINT8 u08CeGainWeight;      // 亮度增益抑制LUT融合权重，值越大抑制作用越强 | [0x0~0xff]
    FH_UINT8 u08SatMap[12];        // 0dB时，饱和度U3.5 | [0x0~0xff]
    FH_UINT8 u08ThhMap[12];        // 0dB时，抑制阈值U3.5 | [0x0~0xff]
    FH_UINT8 u08WeightMap[12];     // 0dB时，融合权重 | [0x0~0xff]
    FH_UINT8 u08Gain[128];         // 亮度增益抑制映射表：128项 | [0x0~0xff]
} ISP_SAT_CFG;
/**|APC|**/
typedef struct _ISP_APC_CFG_S_
{
    FH_BOOL bApcEn;// Apc控制使能 | [0x0~0x1]
    FH_BOOL bGainMappingEn;// 锐化控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_BOOL bInttMappingEn;// APC gain mapping控制模式 0: intt mapping关闭 1: intt mapping开启 | [0x0~0x1]
    FH_BOOL bMergeType;// 细节/边界融合模式 0：选择细节和边界的最大值，detab无效 1：根据detab曲线将细节和边界进行加权 | [0x0~0x1]
    FH_BOOL bLightStrEn;// 锐化强度随亮度变化使能 0：关闭 1：开启 | [0x0~0x1]
    FH_UINT8 u08DetailLutNum;// 细节增强LUT系数选择，范围0~7，由弱到强 | [0x0~0x7]
    FH_BOOL bDetailLutSet;// 细节增强LUT组别选择，范围0~1，噪声抑制由弱到强 | [0x0~0x1]
    FH_UINT8 u08EdgeLutNum;// 边界锐化LUT系数选择，范围0~7，由弱到强 | [0x0~0x7]
    FH_BOOL bEdgeLutSet;// 边界增强LUT组别选择，范围0~1，噪声抑制由弱到强 | [0x0~0x1]
    FH_UINT8 u08DetailFNum;// 细节滤波器选择，范围0~3，由强到弱 | [0x0~0x3]
    FH_UINT8 u08EdgeFNum;// 边界滤波器选择，范围0~3，由强到弱 | [0x0~0x3]
    FH_UINT8 u08DetailGain;// 纹理频率控制系数，逐渐变大，图像细节从圆润变得细碎 | [0x0~0xff]
    FH_UINT8 u08EdgeGain;// 边缘频率控制系数，逐渐变大，图像边缘从圆润变得细碎 | [0x0~0xff]
    FH_UINT8 u08PosDeStr;// 正向细节强度控制u.6 越大细节白边越强 | [0x0~0xff]
    FH_UINT8 u08NegDeStr;// 负向细节强度控制u.6 越大细节黑边越强 | [0x0~0xff]
    FH_UINT8 u08PosEsStr;// 正向边缘强度控制u.6 越大边界白边越强 | [0x0~0xff]
    FH_UINT8 u08NegEsStr;// 负向边缘强度控制u.6 越大边界黑边越强 | [0x0~0xff]
    FH_UINT8 u08Pgain;// 正向全局控制强度u.6 越大全局白点越强 | [0x0~0xff]
    FH_UINT8 u08Ngain;// 负向全局控制强度u.6 越大全局黑点越强 | [0x0~0xff]
    FH_UINT16 u16LocctrlSupsteady;// 强边缘选择阈值，大于该阈值才会判断成强边缘 | [0x0~0x1ff]
    FH_UINT8 u08LocctrlSupstep;// 抑制强度控制参数u.6 越大抑制越弱，黑白边越强 | [0x0~0xff]
    ISP_LUT_POINT stDetabLut[2];// 纹理与边界过度映射Lut横坐标x（0-0xfff）; 映射输出纵坐标值y(0x0~0xff)，越大越倾向于边界输出结果，y0<=y1 | [ISP_LUT_POINT]
    ISP_LUT_POINT stFlatTabLut[2];// 平坦与细节过度映射Lut横坐标x（0-0x3ff）; 映射输出纵坐标值y(0x0~0xff)，越小越倾向于平坦区域越模糊，y0<=y1 | [ISP_LUT_POINT]
    ISP_LUT_POINT stShootTabLut[2];// 强边缘与非强边缘过度映射Lut配合强边抑制使用，横坐标x（0-0x3ff）; 映射输出纵坐标值y(0x0~0xff)，越大越倾向于判断为强边缘越被抑制，y0<=y1 | [ISP_LUT_POINT]
    FH_UINT8 u08DetailGainMap[12];// 0dB时，纹理频率控制系数,越大纹理越细 | [0x0~0xff]
    FH_UINT8 u08EdgeGainMap[12];// 0dB时，边界频率控制系数，越大边界越细 | [0x0~0xff]
    FH_UINT8 u08PosDeInttStrMap[4];// 1/16 inttMax时，正向细节强度控制 | [0x0~0xff]
    FH_UINT8 u08PosDeStrMap[12];// 0dB时，正向细节强度控制 | [0x0~0xff]
    FH_UINT8 u08NegDeInttStrMap[4];// 1/16 inttMax时，负向细节强度控制 | [0x0~0xff]
    FH_UINT8 u08NegDeStrMap[12];// 0dB时，负向细节强度控制 | [0x0~0xff]
    FH_UINT8 u08PosEsInttStrMap[4];// 1/16 inttMax时，正向边缘强度控制 | [0x0~0xff]
    FH_UINT8 u08PosEsStrMap[12];// 0dB时，正向边缘强度控制 | [0x0~0xff]
    FH_UINT8 u08NegEsInttStrMap[4];// 1/16 inttMax时，负向边缘强度控制 | [0x0~0xff]
    FH_UINT8 u08NegEsStrMap[12];// 0dB时，负向边缘强度控制 | [0x0~0xff]
    FH_UINT8 u08PgainMap[12];// 0dB时，白边界强度 | [0x0~0xff]
    FH_UINT8 u08NgainMap[12];// 0dB时，黑边界强度 | [0x0~0xff]
    FH_UINT8 u08FlattabThLYMap[12];// 0db, 平坦区抑制下阈值对应强度 | [0x0~0xff]
    FH_UINT16 u16FlattabThLMap[12];// 0db, 平坦区抑制下阈值 | [0x0~0x3ff]
    FH_UINT16 u16FlattabThHMap[12];// 0db, 平坦区抑制上阈值 | [0x0~0x3ff]
    FH_UINT8 u08DetabThLYMap[12];// 0db, 细节边界过度矩阵下阈值映射梯度值 | [0x0~0xff]
    FH_UINT16 u16DetabThLMap[12];// 0db, 梯度信息下阈值 | [0x0~0xfff]
    FH_UINT16 u16DetabThHMap[12];// 0db, 梯度信息上阈值 | [0x0~0xfff]
    FH_UINT8 u08DetailLutNumMap[12];// Lut系数选择，范围0~7（增益0dB时） | [0x0~0xf]
    FH_UINT8 u08EdgeLutNumMap[12];// Lut系数选择，范围0~7（增益0dB时） | [0x0~0xf]
    FH_UINT8 u08DetailFNumMap[12];// filter系数选择，范围0~3（增益0dB时） | [0x0~0xf]
    FH_UINT8 u08EdgeFNumMap[12];// filter系数选择，范围0~3（增益0dB时） | [0x0~0xf]
    FH_UINT16 u16LightGainThL;// 锐化强度跟随亮度变化增益下阈值，单位为倍数>0 | [0x0~0xffff]
    FH_UINT16 u16LightGainThH;// 锐化强度跟随亮度变化增益上阈值，单位为倍数，>u16LightGainThL | [0x0~0xffff]
    FH_UINT8 u08LightStrThl[33];// 亮度等级0～32在增益不大于gain_thl时锐化强度，U2.6 | [0x0~0xff]
    FH_UINT8 u08LightStrThh[33];// 亮度等级0～32在增益大于gain_thh时锐化强度，U2.6 | [0x0~0xff]
} ISP_APC_CFG;

typedef struct _ISP_APC_MT_CFG_S_ {
    FH_BOOL bApcMtEn;// apc联动使能：0: Off 1: On|[0-0x1]
    FH_BOOL bApcMtMode;// 锐化联动控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_UINT8 u08MotionThL;// 联动coeff下阈值 | [0x0~0xff]
    FH_UINT8 u08MotionThH;// 联动coeff上阈值 | [0x0~0xff]
    FH_UINT8 u08MotionThLY;// 联动coeff下阈值对应权重值 motionThLY>=motionThHY | [0x0~0xff]
    FH_UINT8 u08MotionThHY;// 联动coeff上阈值对应权重值 | [0x0~0xff]
    FH_UINT8 u08MotionThHYMap[12];// 0db，联动coeff上阈值对应权重值 | [0x0~0xff]
    FH_UINT8 u08MotionThLMap[12];// 0db，联动coeff下阈值 | [0x0~0xff]
    FH_UINT8 u08MotionThHMap[12];// 0db，联动coeff上阈值 | [0x0~0xff]
} ISP_APC_MT_CFG;

typedef struct _SHARPEN_STAT_CFG_S{
    FH_UINT16 w;  // 配置的统计窗口宽 | [1, PicW]
    FH_UINT16 h;  // 配置的统计窗口的高 | [1, PicH]
    FH_UINT16 x_offset;  // 统计窗口的水平偏移 | [0, PicW - w]
    FH_UINT16 y_offset;  // 统计窗口的垂直偏移 | [0, PicH - h]
} SHARPEN_STAT_CFG;

typedef struct _SHARPEN_STAT_S{
    FH_UINT32 sum;  // 获取到的锐度统计信息 | [0~0xffffffff]
} SHARPEN_STAT;

/**|YCNR|**/
typedef struct _ISP_YNR_CFG_S_ {
    FH_BOOL bYnrEn;// Ynr控制使能 | [0x0~0x1]
    FH_BOOL bGainMappingEn;// 亮度去噪控制模式 0：手动模式 1：gain mapping | [0x0~0x1]
    FH_BOOL bInttMappingEn;// YNR gain mapping控制模式 0: intt mapping关闭 1: intt mapping开启 | [0x0~0x1]
    FH_BOOL bCtrlMode;// 0：弱模式 1：强模式 | [0x0~0x1]
    FH_UINT8 u08YnrThSlope0;// 0号滤波器映射斜率 U4.4，斜率大于1变锐，斜率小于1变糊 | [0x0~0xff]
    FH_UINT8 u08YnrThSlope1;// 1号滤波器映射斜率 U4.4，斜率大于1变锐，斜率小于1变糊 | [0x0~0xff]
    FH_UINT8 u08YnrThSlope2;// 2号滤波器映射斜率 U4.4，斜率大于1变锐，斜率小于1变糊 | [0x0~0xff]
    FH_UINT8 u08YnrThStr0;// ynr 0号滤波器降噪去噪等级U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrThStr1;// ynr 1号滤波器降噪去噪等级U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrThStr2;// Ynr 2号滤波器降噪去噪等级U4.4 | [0x0~0xff]
    FH_UINT8 u08Thl0[16];// 亮度阈值0（0号滤波器） | [0x0~0xff]
    FH_UINT8 u08Thl1[16];// 亮度阈值0（1号滤波器） | [0x0~0xff]
    FH_UINT8 u08Thl2[16];// 亮度阈值0（2号滤波器） | [0x0~0xff]
    FH_BOOL bYnrMergeEn;// YNR融合使能 0：关 1：开 | [0x0~0x1]
    FH_BOOL bYnrMergeMode; // Ynr联动控制使能 | [0x0~0x1]
    FH_BOOL bYnrMergeInttMappingEn;// YNR Merge gain mapping控制模式 0: intt mapping关闭 1: intt mapping开启 | [0x0~0x1]
    FH_UINT8 u08YnrMergeThl;// 融合下阈值 | [0x0~0xff]
    FH_UINT8 u08YnrMergeThh;// 融合上阈值 thh > thl | [0x0~0xff]
    FH_UINT8 u08YnrMergeThlY;// 融合下阈值对应权重值 | [0x0~0xff]
    FH_UINT8 u08YnrMergeThhY;// 融合上阈值对应权重值 thh_y >= thl_y | [0x0~0xff]
    FH_UINT8 u08YnrThSlope0Map[12];// 0dB时映射斜率（高频分量），U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrThSlope1Map[12];// 0dB时映射斜率（中频分量），U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrThSlope2Map[12];// 0dB时映射斜率（低频分量），U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrThStr0InttMap[8];// 1/256 inttMax 时，去噪强度（高频分量），U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrThStr0Map[12];// 0dB时去噪强度（高频分量），U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrThStr1InttMap[8];// 1/256 inttMax 时，去噪强度（中频分量），U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrThStr1Map[12];// 0dB时去噪强度（中频分量），U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrThStr2Map[12];// 0dB时去噪强度（低频分量），U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrMergeThlYInttMap[4];// (1/16|1/8|1/4|1/2 inttMax)时，ynr联动coeff下阈值对应权重，U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrMergeThlYMap[12];// 0dB时，ynr联动coeff下阈值对应权重，U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrMergeThlMap[12];// 0dB时，ynr联动coeff下阈值，U4.4 | [0x0~0xff]
    FH_UINT8 u08YnrMergeThhMap[12];// 0dB时，ynr联动coeff上阈值，U4.4 | [0x0~0xff]
} ISP_YNR_CFG;

typedef struct _ISP_YNR_MT_CFG_S_ {
    FH_BOOL bCoefEn;// YNR与NR3D联动使能 0：关 1：开 | [0x0~0x1]
    FH_BOOL bGainMappingEn;// Ynr联动控制模式 0：手动模式 1：gain mapping | [0x0~0x1]
    FH_BOOL bInttMappingEn;// YNR MT gain mapping控制模式 0: intt mapping关闭 1: intt mapping开启 | [0x0~0x1]
    FH_UINT8 u08MtCoeff0;// YNR与NR3D联动去噪第一个coeff分界点，小运动 | [0x0~0xff]
    FH_UINT8 u08MtCoeff1;// YNR与NR3D联动去噪第二个coeff分界点，大运动 | [0x0~0xff]
    FH_UINT8 u08YnrMtStr0;// YNR与NR3D联动去噪第一级强度，小运动 | [0x0~0xff]
    FH_UINT8 u08YnrMtStr1;// YNR与NR3D联动去噪第二级强度，大运动 | [0x0~0xff]
    FH_UINT8 u08YnrMtStr0InttMap[8];// 1/256 inttMax 时，小运动去噪等级 | [0x0~0xff]
    FH_UINT8 u08YnrMtStr0Map[12];// 0dB时，小运动去噪等级 | [0x0~0xff]
    FH_UINT8 u08YnrMtStr1InttMap[8];// 1/256 inttMax 时，大运动去噪等级 | [0x0~0xff]
    FH_UINT8 u08YnrMtStr1Map[12];// 0dB时，大运动去噪等级 | [0x0~0xff]
} ISP_YNR_MT_CFG;

typedef struct _ISP_CNR_CFG_S_
{
    FH_BOOL bCnrEn;// Cnr控制使能 | [0x0~0x1]
    FH_BOOL bGainMappingEn;// 色度降噪控制模式 0: 手动模式 1: gain mapping | [0x0~0x1]
    FH_UINT8 u08CnrStr;// 色度降噪强度 | [0x0~0xff]
    FH_UINT8 u08ColThl;// 颜色保护阈值下限。值越小，颜色保护越好，同时去噪效果越弱。 | [0x0~0xff]
    FH_UINT8 u08ColThh;// 颜色保护阈值上限。满足约束：colThH>colThL。值越小，颜色保护保护越好，同时去噪效果越弱。 | [0x0~0xff]
    FH_UINT8 u08StrMap[12];// 0dB时，色度去噪等级 | [0x0~0xff]
    FH_UINT8 u08ColThlMap[12];// 0dB时，颜色阈值下限 | [0x0~0xff]
    FH_UINT8 u08ColThhMap[12];// 0dB时，颜色阈值上限 | [0x0~0xff]
} ISP_CNR_CFG;

typedef struct _ISP_CNR_MT_CFG_S_
{
    FH_BOOL bCnrMtEn;// CNR联动使能 0: 关;   1:开 | [0x0~0x1]
    FH_BOOL bCnrMtStrMode;// CNR联动强度模式 0: 手动模式;    1:gain mapping | [0x0~0x1]
    FH_UINT8 u08CnrMtStr;// cnr联动强度，越大运动区域色度降噪越强 | [0x0~0xff]
    FH_UINT8 u08CnrMtColStr;// cnr联动颜色保护强度 | [0x0~0xf]
    FH_SINT16 s16CnrMtOffset;// cnr联动偏移，越大整体色度去噪越强 | [-256~255]
    FH_UINT8 u08MtStrMap[12];// 0dB时，cnr联动强度等级 | [0x0~0xff]
} ISP_CNR_MT_CFG;

/**|PURPLE|**/
typedef struct _ISP_PURPLEFRI_CFG_S_
{
    FH_BOOL bPurplefriEn;// Purplefri控制使能 | [0x0~0x1]
    FH_BOOL bCtrlMode;// 强弱去紫边模式： 0：弱模式 1：强模式 | [0x0~0x1]
    FH_BOOL bMode;// 去紫边控制模式 0: 手动模式;    1:gain mapping | [0x0~0x1]
    FH_BOOL bInttMappingEn;// Purple gain mapping控制模式 0: intt mapping关闭 1: intt mapping开启 | [0x0~0x1]
    FH_UINT8 u08PurpleStr;// 去紫边强度控制，值越大抑制越强 | [0x0~0x3f]
    FH_UINT8 u08HueExpandCtrl;// 色域浸染抑制强度，值越大抑制越强 | [0x0~0x3f]
    FH_UINT16 u16Ydiffth;// 亮度差阈值，该值越小，判断紫边的范围越宽，=0时，检测紫边不受边缘限制 | [0x0~0xffff]
    FH_SINT8 s08HueThL;// 紫边判断区域下阈值（-128~127），单位为度 | [-128~127]
    FH_SINT8 s08HueThH;// 紫边判断区域上阈值（-128~127），单位为度 | [-128~127]
    FH_SINT8 s08HueThL1;// 绿边判断区域下阈值（-128~127），单位为度 | [-128~127]
    FH_SINT8 s08HueThH1;// 绿边判断区域上阈值（-128~127），单位为度 | [-128~127]
    FH_SINT16 s16HueOff;// 色域偏移量（-135～224），单位为度 | [-135~224]
    FH_UINT16 u16LumaThl;// 紫边判断亮度下阈值，l~h紫边权重线性递增 | [0x0~0x1ff]
    FH_UINT16 u16LumaThh;// 紫边判断亮度上阈值，大于该值则紫边权重最大 LumaThh > lumaThl | [0x0~0x1ff]
    FH_UINT16 u16YDiffThInttMap[4];// 1/16,1/8,1/4,1/2 inttMax时，亮度区域边界检测阈值　| [0x0~0xffff]
    FH_UINT16 u16YDiffThMap[12];// 0dB时，亮度区域边界检测阈值　| [0x0~0xffff]
} ISP_PURPLEFRI_CFG;

/**|LC|**/
typedef struct _ISP_LC_CFG_S_
{
    FH_BOOL   bLc0En;   //第一个色度调整使能 | [0x0-0x1]
    FH_BOOL   bArea0GainMappingEn; //第一个色度中心增益调整模式，0：手动,1：增益映射 | [0x0-0x1]
    FH_BOOL   bArea1GainMappingEn; //第二个色度中心增益调整模式，0：手动,1：增益映射 | [0x0-0x1]
    FH_BOOL   bArea2GainMappingEn; //第三个色度中心增益调整模式，0：手动,1：增益映射 | [0x0-0x1]
    FH_BOOL   bArea0AlphaMappingEn; //第一个色度调整范围角度调整模式,0：手动 1：增益映射 | [0x0-0x1]
    FH_BOOL   bArea1AlphaMappingEn; //第二个色度调整范围角度调整模式,0：手动 1：增益映射 | [0x0-0x1]
    FH_BOOL   bArea2AlphaMappingEn; //第三个色度调整范围角度调整模式,0：手动 1：增益映射 | [0x0-0x1]
    FH_UINT8  u08Gain0; //中心增益：0.6-3.98(0x27-0xff，小于0x27的按0x27处理)U2.6 | [0x0-0xff]
    FH_UINT8  u08Alpha0; //调整范围角度阿尔法：15-89(0x0f-0x59,小于0x0f的按0x0f处理，大于0x59的按0x59处理) | [0x0-0xff]
    FH_UINT16 u16Theta0; //中心线角度西塔：0-359(0x0-0x167，大于0x167的按0x167处理) | [0x0-0x1ff]
    FH_BOOL   bLc1En;    //第二个色度调整使能 | [0x0-0x1]
    FH_UINT8  u08Gain1;  //中心增益：0.6-3.98(0x27-0xff，小于0x27的按0x27处理)U2.6 | [0x0-0xff]
    FH_UINT8  u08Alpha1; //调整范围角度阿尔法：15-89(0x0f-0x59,小于0x0f的按0x0f处理，大于0x59的按0x59处理) | [0x0-0xff]
    FH_UINT16 u16Theta1; //中心线角度西塔：0-359(0x0-0x167，大于0x167的按0x167处理) | [0x0-0x1ff]
    FH_BOOL   bLc2En;    //第三个色度调整使能 | [0x0-0x1]
    FH_UINT8  u08Gain2;  //中心增益：0.6-3.98(0x27-0xff，小于0x27的按0x27处理)U2.6 | [0x0-0xff]
    FH_UINT8  u08Alpha2; //调整范围角度阿尔法：15-89(0x0f-0x59,小于0x0f的按0x0f处理，大于0x59的按0x59处理) | [0x0-0xff]
    FH_UINT16 u16Theta2; //中心线角度西塔：0-359(0x0-0x167，大于0x167的按0x167处理) | [0x0-0x1ff]
    FH_UINT8 u08Area0AlphaMap[12];  // 第一个不同照度下色度调整角度 | [0~0xff]
    FH_UINT8 u08Area1AlphaMap[12];  // 第二个不同照度下色度调整角度 | [0~0xff]
    FH_UINT8 u08Area2AlphaMap[12];  // 第三个不同照度下色度调整角度 | [0~0xff]
    FH_UINT8 u08Area0GainMap[12];  // 第一个不同照度下中心增益调整 | [0~0xff]
    FH_UINT8 u08Area1GainMap[12];  // 第二个不同照度下中心增益调整 | [0~0xff]
    FH_UINT8 u08Area2GainMap[12];  // 第三个不同照度下中心增益调整 | [0~0xff]
} ISP_LC_CFG;

/**|AF|**/
//----------------AF-------------------
typedef struct _ISP_AF_GMAP_S
{
    FH_UINT16 u16X[11];  // gmap折点拐点横坐标 | [0~0xffff]
    FH_SINT16 s16S[12];  // gmap折线直线斜率 | [-2048, 2047]
    FH_UINT16 u16Y[12];  // gmap折点拐点纵坐标 | [0~0xfff]
    FH_UINT8  u8SShift;   // gmap计算移位因子 | [0~0xf]
}AF_GMAP_S;

typedef struct _ISP_AF_GMAP_S_V2
{
    FH_UINT16 u16X[11];  // gmap折点拐点横坐标 | [0~0xfff]
    FH_SINT16 s16S[12];  // gmap折线直线斜率 | [-2048, 2047]
    FH_UINT16 u16Y[12];  // gmap折点拐点纵坐标 | [0~0xfff]
    FH_UINT8  u8SShift;  // gmap计算移位因子 | [0~0xf]
}AF_GMAP_S_V2;

typedef struct _ISP_AF_FILTER_S
{
    FH_UINT8  afStatChannel;       // 统计对象选择，为RGB bayer格式的时候，0~3分别对应R/Gr/Gb/B，代码内部已经做了bayer转换 | [TAB_COLOR_CODE]
    FH_UINT8  afStatHFilterMode;   // 水平滤波模式选择 | [0~0x1]
    AF_GMAP_S stGmapConf;          // gmap配置 | [AF_GMAP_S]
    FH_UINT16 afLumTl;             // 亮度统计亮度門限下阈值 | [0~0xfff]
    FH_UINT16 afLumTh;             // 亮度统计亮度門限上阈值 | [0~0xfff]
    FH_SINT8  afPreFilterCoe[7];   // 预滤波滤波器系数 | [0~0xf]
    FH_UINT16 afPreFilterMul;      // 预滤波滤波器乘系数 | [0-0x3ff]
    FH_UINT16 afPreFilterShift;    // 预滤波滤波器移位因子 | [0~0xf]
    FH_SINT8  afVfilter0[25];      // 垂直滤波器0系数 | [0~0xf]
    FH_UINT16 afVFilter0Shift;     // 垂直滤波器0移位因子 | [0~0xf]
    FH_UINT16 afVFilter0T;         // 垂直滤波器0門限阈值 | [0~0xfff]
    FH_SINT8  afVfilter1[25];      // 垂直滤波器1系数 | [0~0xf]
    FH_UINT16 afVFilter1Shift;     // 垂直滤波器1移位因子 | [0~0xf]
    FH_UINT16 afVFilter1T;         // 垂直滤波器1門限阈值 | [0~0xfff]
    FH_BOOL   afHfilter0En[3];     // 水平滤波器0第1/2/3级滤波器使能 | [0~1]
    FH_UINT8  afHfilter0Gain;      // 水平滤波器0增益 | [0~0xff]
    FH_UINT16 afHFilter0T;         // 水平滤波器0門限阈值 | [0~0xfff]
    FH_SINT16 afHFilter0Offset;    // 水平滤波器0输出偏移 | [0~0x1fff]
    FH_UINT8  afHFilter0Shift[7];  // 水平滤波器0移位因子 | [0~0xf]
    FH_UINT16 afHFilter0C[6];      // 水平滤波器0滤波器系数C | [0~0x7ff]
    FH_UINT16 afHFilter0D[6];      // 水平滤波器0滤波器系数D | [0~0x7ff]
    FH_BOOL   afHfilter1En[3];     // 水平滤波器1第1/2/3级滤波器使能 | [0~1]
    FH_UINT8  afHfilter1Gain;      // 水平滤波器1增益 | [0~0xff]
    FH_UINT16 afHFilter1T;         // 水平滤波器1門限阈值 | [0~0xfff]
    FH_SINT16 afHFilter1Offset;    // 水平滤波器1输出偏移 | [0~0x1fff]
    FH_UINT8  afHFilter1Shift[7];  // 水平滤波器1移位因子 | [0~0xf]
    FH_UINT16 afHFilter1C[6];      // 水平滤波器1滤波器系数C | [0~0x7ff]
    FH_UINT16 afHFilter1D[6];      // 水平滤波器1滤波器系数D | [0~0x7ff]
} ISP_AF_FILTER;

typedef struct _AF_PREFILTER_CFG
{
    FH_BOOL   bPrefilterEn;          // Prefilter使能 | [0~0x1]
    FH_UINT8  u08PrefilterMode;      // Prefilter模式 | [0~0x1]
    FH_SINT8  s08PreFilterCoe[7];    // 预滤波滤波器系数 | [0~0xf]
    FH_UINT8  u08PreFilterShift;     // 预滤波滤波器移位因子 | [0~0xf]
    FH_UINT16 u16PreFilterMul;       // 预滤波滤波器乘系数 | [0-0x3ff]
}AF_PREFILTER_CFG;

typedef struct _AF_VFILTER_CFG
{
    FH_SINT8  s08VFilterCoef[5];    // 垂直滤波器系数 | [0~0x3f]
    FH_UINT8  u08VFilterShift;      // 垂直滤波器移位因子 | [0~0xf]
    FH_UINT16 u16VFilterGain;       // 垂直滤波器增益 | [0~0x3ff]
}AF_VFILTER_CFG;

typedef struct _AF_HFILTER_CFG
{
    FH_UINT8  u08HFilterGain;       // 水平滤波器增益 | [0~0xff]
    FH_UINT8  u08HFilterShift[7];   // 水平滤波器移位因子 | [0~0xf]
    FH_UINT16 u16HFilterC[6];       // 水平滤波器滤波器系数C | [0~0x7ff]
    FH_UINT16 u16HFilterD[6];       // 水平滤波器滤波器系数D | [0~0x7ff]
    FH_BOOL   bHFilterEn[3];        // 水平滤波器第1/2/3级滤波器使能 | [0~1]
}AF_HFILTER_CFG;

typedef struct _AF_H_STAT_CFG_
{
    FH_BOOL bHdsEn;               // 滤波器的HDS使能 | [0~0x1]
    FH_UINT8 u08HdsMode;          // 滤波器的HDS模式 | [0~0x2]
    FH_BOOL bLdgEn;               // 滤波器的LDG使能 | [0~0x1]
    FH_BOOL bCoringEn;            // 滤波器的Coring使能 | [0~0x1]
    FH_BOOL bShapingEn;           // 滤波器的Shaping使能 | [0~0x1]
    FH_UINT8 u08ShapingMode;      // 滤波器的Shaping模式 | [0~0x1]
    AF_HFILTER_CFG afHFilterCfg;  // 水平滤波器配置 | [AF_HFILTER_CFG]
} ISP_AF_H_STAT_CFG;

typedef struct _AF_V_STAT_CFG_
{
    FH_BOOL bLdgEn;               // 滤波器的LDG使能 | [0~0x1]
    FH_BOOL bCoringEn;            // 滤波器的Coring使能 | [0~0x1]
    FH_BOOL bShapingEn;           // 滤波器的Shaping使能 | [0~0x1]
    FH_UINT8 u08ShapingMode;      // 滤波器的Shaping模式 | [0~0x1]
    AF_VFILTER_CFG afVFilterCfg;  // 垂直滤波器配置 | [AF_VFILTER_CFG]
} ISP_AF_V_STAT_CFG;

typedef struct _ISP_AF_FILTER_S_V2
{
    FH_UINT8  u08ChannelMode;        // 统计对象选择，优先选择平均模式
    FH_BOOL bSqrtEN;                 // Sqrt使能 | [0~0x1]
    FH_SINT8 s08SqrtShift;           // Sqrt移位系数 | [-8~7]
    FH_BOOL bMappingEn;              // Mapping使能 | [0~0x1]
    AF_GMAP_S_V2 stGmapConf;           // gmap配置 | [AF_GMAP_S]
    FH_UINT8 u08HfilterStatMode;     // 水平滤波器的统计模式 | [0~0x1]
    FH_UINT16 u16LumTl;              // 亮度统计亮度門限下阈值 | [0~0xfff]
    FH_UINT16 u16LumTh;              // 亮度统计亮度門限上阈值 | [0~0xfff]
    AF_PREFILTER_CFG afPreFilterCfg; // 预滤波滤波器配置 | [AF_PREFILTER_CFG]
    ISP_AF_H_STAT_CFG stAfHStat[2];  // 水平滤波滤波器配置
    ISP_AF_V_STAT_CFG stAfVStat[2];  // 垂直滤波滤波器配置
} ISP_AF_FILTER_V2;

typedef struct _ISP_AF_STAT_ADDR_S
{
    FH_UINT32 startAddr;      // AF统计信息存放地址| [0~0xffffffff]
} ISP_AF_STAT_ADDR;

/**|GME|**/
typedef struct _ISP_GME_PARAM_S_
{
    FH_SINT32 s32x; // x vector | [0~0xffffffff]
    FH_SINT32 s32y; // y vector | [0~0xffffffff]
} ISP_GME_PARAM;

/**|WDR|**/

typedef struct _ISP_WDR_MERGE_CFG_
{
    FH_UINT16 u16MergeX[12]; // Merge曲线的第一～十二个折点横坐标(Xi < X(i+1))。 | [0~0xfff]
    FH_UINT16 u16MergeY[12]; // Merge曲线的第一～十二个折点纵坐标。 | [0~0x400]
} ISP_WDR_MERGE_CFG;

typedef struct _ISP_WDR_CFG_S_
{
    FH_BOOL bWdrEn;// Wdr控制使能 | [0x0~0x1]
    FH_BOOL bMode;// 0：宽动态模式 1：非宽动态模式 | [0x0~0x1]
    FH_BOOL bAutoAdjustEn;// 自动曝光比例调整使能 | [0x0~0x1]
    FH_BOOL bShowHist;// 0：关闭 1：开启 | [0x0~0x1]
    FH_BOOL bSframeCorrMode;// 短帧亮度校正模式选择 0：自动模式，自动配置短帧亮度校正曲线 1：自定义模式，用户通过api设置短帧亮度校正曲线 | [0x0~0x1]
    FH_BOOL bSfCorrMethod;// 短帧亮度校正方法选择，仅当sframe_coor_mode=0有效 0：高亮抑制方式; 1：正常方式，高亮不抑制 | [0x0~0x1]
    FH_UINT16 u16SensorOutputMax;// Sensor最大输出，用于数据通路归一化 | [0x0~0xfff]
    FH_UINT8 u08SframeCorrTiming;// 短帧亮度校正与短帧intt的生效时机控制： 0：同一帧生效 其他：短帧亮度校正推迟1/2/3帧生效 | [0x0~0x3]
    FH_BOOL bMergeCurveMode;// 长短帧merge曲线模式： 0：采用内置merge曲线，根据pre_curve_idx确定 1：自定义模式，用户通过api设置长短帧merge曲线 | [0x0~0x1]
    FH_BOOL bLfHlrEn;// 长帧高光恢复使能： 0：关闭 1：开启 | [0x0~0x1]
    FH_BOOL bSfHlrEn;// 短帧高光恢复使能： 0：关闭 1：开启 | [0x0~0x1]
    FH_UINT8 u08ExposureRatio;// auto_adjust_en为0时，长短帧曝光比例：(6~64倍)  | [0x0~0xff]
    FH_UINT8 u08ExposureRatioMin;// auto_adjust_en为1时，长短帧曝光比例最小值，不能小于6 | [0x0~0xff]
    FH_UINT8 u08ExposureRatioMax;// auto_adjust_en为1时，长短帧曝光比例最大值，不能大于64 | [0x0~0xff]
    FH_UINT8 u08WdrMergeCfg;// WDR merge曲线配置 0:两帧合成输出 1:只出长帧 2:只出短帧 | [0x0~0x3]
    FH_UINT8 u08PreCurveIdx;// Merge预设曲线 | [0x0~0x3]
    FH_UINT16 u16SfOffset;// WDR短帧偏移 | [0x0~0xfff]
    FH_UINT16 u16LfOverexpRatioThh;// 长帧过曝比例(百分数)上阈值U6.6 | [0x0~0xfff]
    FH_UINT16 u16LfOverexpRatioThl;// 长帧过曝比例(百分数)下阈值U6.6 | [0x0~0xfff]
    FH_UINT16 u16SfOverexpRatioTh;// 短帧过曝比例(百分数)阈值U2.10 | [0x0~0xfff]
    FH_UINT16 u16GainTh;// 增益门限值 | [0x0~0xfff]
    FH_UINT8 u08LfHlrTh;// 长帧高光校正阈值 | [0x0~0xff]
    FH_UINT8 u08LfHlrSlope;// 长帧高光校正斜率 | [0x0~0xff]
    FH_UINT8 u08SfHlrTh;// 短帧高光校正阈值 | [0x0~0xff]
    FH_UINT8 u08SfHlrSlope;// 短帧高光校正斜率 | [0x0~0xff]
} ISP_WDR_CFG;

/**|POST_GAIN|**/
typedef struct _ISP_POST_GAIN_CFG_S
{
    FH_UINT16 rGain;        //R分量增益 | [0-0xfff]
    FH_UINT16 gGain;        //G分量增益 | [0-0xfff]
    FH_UINT16 bGain;        //B分量增益 | [0-0xfff]
    FH_UINT16 rOffset;      //R分量黑度补偿 | [0-0xfff]
    FH_UINT16 gOffset;      //G分量黑度补偿 | [0-0xfff]
    FH_UINT16 bOffset;      //B分量黑度补偿 | [0-0xfff]
} ISP_POST_GAIN_CFG;

/**|RGBA|**/
typedef struct _RGBA_ABAL_CFG_S_
{
    FH_UINT8 abalSpeed;          // Abal的调整速度 | [0~0xf]
    FH_UINT8 abalThl;            // Abal不调整的阈值 | [0~0xf]
    FH_UINT8 abalThh;            // Abal开始调整的阈值 | [0~0xf]
    FH_UINT8 abalStabTime;       // Abal的稳定时间 | [0~0xff]
    FH_UINT8 abalStep;           // ABal调整的步长 | [0~0xf]
    FH_UINT8 abalTargetStabTime; // AbalTarget的稳定时间 | [0~0xff]
    FH_UINT8 abalTargetThr;      // AbalTarget的调整阈值 | [0~0xf]
} RGBA_ABAL_CFG;

typedef struct _RGBA_WEIGHT_CFG_S_
{
    FH_UINT8  W2Speed;    // W2的调整速度 | [0~0xf]
    FH_UINT8  W2Thl;      // W2的不调整阈值 | [0~0xf]
    FH_UINT8  W2Thh;      // W2开始调整的阈值 | [0~0xf]
    FH_UINT8  W2StabTime; // W2的稳定时间 | [0~0xf]
    FH_UINT8  W2Step;     // W2的调整步长 | [0~0xf]
    FH_UINT8  W2AThl;     // W2计算时增益的上阈值 | [0~0x7f]
    FH_UINT8  W2AThh;     // W2计算时增益的下阈值 | [0~0x7f]
    FH_UINT16 W2GThl;     // W2计算时红外的下阈值 | [0~0x7f]
    FH_UINT16 W2GThh;     // W2计算时红外的上阈值 | [0~0x7f]
} RGBA_WEIGHT_CFG;

typedef struct _RGBA_CCM_TABLE_S_
{
    FH_UINT16 RgbaCcmTable[2][16]; // rgba ccm矩阵系数 | [-4096~4095]
} RGBA_CCM_TABLE;

typedef struct _RGBA_CONVERT_TH_S_
{
    FH_UINT16 convThl; // 格式转换模块LUT上阈值 | [0~0xffff]
    FH_UINT16 convThh; // 格式转换模块LUT下阈值 | [0~0xffff]
} RGBA_CONVERT_TH;

typedef struct _RGBA_ENHANCE_TH_S_
{
    FH_UINT16 enhThl; // 亮度/锐度增强模块亮度下阈值 | [0~0xffff]
    FH_UINT16 enhThh; // 亮度/锐度增强模块亮度上阈值 | [0~0xffff]
} RGBA_ENHANCE_TH;

typedef struct _RGBA_ETH_S_
{
    FH_UINT16 eThl; // 边界强度下限 | [0~0xffff]
    FH_UINT16 eThh; // 边界强度上限 | [0~0xffff]
} RGBA_ETH;

typedef struct _RGBA_EGAIN_CFG_S_
{
    FH_UINT8 egainR; // 亮度/锐度增强模块R通道增强增益 | [0~0xff]
    FH_UINT8 egainG; // 亮度/锐度增强模块G通道增强增益 | [0~0xff]
    FH_UINT8 egainB; // 亮度/锐度增强模块B通道增强增益 | [0~0xff]
} RGBA_EGAIN_CFG;

typedef struct _RGBA_DEFAULT_CFG_S_
{
    FH_BOOL         bRgbaEn;     // RGBA控制使能 | [0~1]
    FH_BOOL         bIrmode;     // Abal是否使能 | [0~1]
    FH_BOOL         bFirFrm;     // 是否是第一帧标志 | [0~1]
    RGBA_ABAL_CFG   abalParam;   // Abal参数结构体 | [RGBA_ABAL_CFG]
    RGBA_WEIGHT_CFG weightParam; // W2参数结构体 | [RGBA_WEIGHT_CFG]
    RGBA_CCM_TABLE  ccmTab;      // ccm矩阵表 | [RGBA_CCM_TABLE]
    RGBA_CONVERT_TH convTh;      // 格式转换LUT阈值参数结构体 | [RGBA_CONVERT_TH]
    RGBA_ENHANCE_TH enhTh;       // 亮度/锐度阈值参数结构体 | [RGBA_ENHANCE_TH]
    RGBA_ETH        eTh;         // 边界强度参数结构体 | [RGBA_ETH]
    RGBA_EGAIN_CFG  egain;       // 分通道亮度/锐度增强模块增益参数结构体 | [RGBA_EGAIN_CFG]
} RGBA_DEFAULT_CFG;

typedef struct _RGBA_CURR_INFO_S_
{
    FH_UINT16 u08RBalGainPre;   // 上一帧RBalGain | [0~0xffff]
    FH_UINT8  u08RBalCnt;       // 当前RBal稳定时间计数 | [0~0xff]
    FH_UINT8  u08RBalTargetCnt; // 当前RBal目标值的稳定时间计数 | [0~0xff]
    FH_UINT16 u16W2Pre;         // 上一帧CCM2权重 | [0~0x7fff]
    FH_UINT16 u16W2Cnt;         // W2的稳定时间计数 | [0~0xff]
} RGBA_CURR_INFO;

typedef enum _RGBA_CMD_KEY_S_ {
    SET_RGBA_DEFAULT   = 0x0, // 配置RGBA　DRV寄存器
    SET_RGBA_EN        = 0x1, // RGBA策略使能开关
    SET_RGBA_FIRFRM    = 0x2, // 配置RGBA处理第一帧标志
    SET_RGBA_IRMODE    = 0x3, // Abal使能
    SET_ABAL_PARA      = 0x4, // 配置Abal相关DRV参数
    SET_WEIGHT_PARA    = 0x5, // 配置W2相关DRV参数
    SET_RGBA_CCM_TABLE = 0x6, // 配置RGBA的CCM矩阵
    SET_CONVERT_TH     = 0x7, // 格式转换LUT上下阈值配置
    SET_ENHANCE_TH     = 0x8, //  亮度/锐度上下阈值设置
    SET_ETH            = 0x9, // 边界强度上下阈值设置
    SET_EGAIN          = 0xa, // RGB通道亮度/锐度增强模块增益设置
    RGBA_CMD_KEY_DUMMY =0xffffffff,
} RGBA_CMD_KEY;

typedef struct _RGBA_STAT_INFO_S_
{
    struct rgba_blk_info
    {
        FH_UINT32 R_sum;  // RGBA统计值R sum | [0~0xffffffff]
        FH_UINT32 G_sum;  // RGBA统计值G sum | [0~0xffffffff]
        FH_UINT32 B_sum;  // RGBA统计值B sum | [0~0xffffffff]
        FH_UINT32 A_sum;  // RGBA统计值A sum | [0~0xffffffff]
        FH_UINT32 IR_sum; // RGBA统计值IR sum | [0~0xffffffff]
        FH_UINT32 cnt;    // RGBA统计值CNT | [0~0xffffffff]
    } blk_rgba[32];
} RGBA_STAT_INFO;

typedef struct _RGBA_DPC_CFG_S
{
    FH_BOOL   bRGBADpcEn;              // RGBA模式下的dpc控制使能 | [0~1]
    FH_BOOL   bGainMappingEn;          // 0：手动	1：增益映射 | [0~1]
    FH_UINT8  u08DpcMode;              // 0:all close;1:white pixel correct open;2.black pixel correct open;3.all open.
    FH_UINT8  u08ModeIdx;              // 去坏点强度 | [0~2]
    FH_UINT8  u08Str;                  // nr2d去噪强度,U1.3 | [0-0xf]
    FH_UINT8  u08wdc;                  //手动白点门限值 | [0-0xff]
    FH_UINT8  u08bdc;                  //手动黑点门限值 | [0-0xff]
    FH_UINT8  u08ThhIR;                // IR通道比值上阈值 | [0~0x3f]
    FH_UINT8  u08ThlIR;                // IR通道比值下阈值 | [0~0xf]
    FH_UINT8  u08ThhG;                 // G通道比值上阈值 | [0~0x3f]
    FH_UINT8  u08ThlG;                 // G通道比值下阈值 | [0~0xf]
    FH_UINT16 u16DiffMin;              // 变化程度的最小值 | [0~0x3ff]
    FH_UINT8  u08DiffShift;            // 变化程度的移位值 | [0~0xf]
    FH_UINT8  u08Strenght[GAIN_NODES]; // 包括:mode,w_s,b_s
    FH_UINT8  u08WhiteThr[GAIN_NODES]; //白点门限DC值
    FH_UINT8  u08BlackThr[GAIN_NODES]; //黑点门限值
} RGBA_DPC_CFG;

/**|SMART IR|**/
typedef struct _SMART_IR_CFG_S
{
    FH_UINT16 u16gainDay2Night; // 白天切换到夜晚的增益門限，bayer sensor | [0~0xffff]
    FH_UINT16 u16gainNight2Day; // 夜晚切换到白天的增益門限，bayer sensor | [0~0xffff]
    FH_UINT16 u16lumaDay2Night; // 白天切换到夜晚的增益門限，rgbir sensor | [0~0xfff]
    FH_UINT16 u16lumaNight2Day; // 夜晚切换到白天的增益門限，rgbir sensor | [0~0xfff]
} SMART_IR_CFG;

/**|FC|**/

typedef struct _FC_CFG_S
{
    FH_UINT8 str; // 去假色强度 | [0~0xff]
    FH_UINT16 u16OffsetT; // 纹理信息偏移量 | [0~0xffff]
    FH_UINT16 u16OffsetY; // 亮度信息偏移量 | [0~0xffff]
} FC_CFG;

/**|YCGAIN|**/
typedef struct _YC_COEFF_STRUCT_S_
{
    FH_SINT16 s12Rcoeff;    //s2.10 rgb2yuv时r通道系数|[0~0xfff]
    FH_SINT16 s12Gcoeff;    //s2.10 rgb2yuv时g通道系数|[0~0xfff]
    FH_SINT16 s12Bcoeff;    //s2.10 rgb2yuv时b通道系数|[0~0xfff]
    FH_SINT16 s12Offset;    //s12 rgb2yuv时偏移|[0~0xfff]
} YC_COEFF_STRUCT;

typedef struct _ISP_YC_GAIN_STRUCT_S_
{
    YC_COEFF_STRUCT Y;    // Y通道系数 | [YC_COEFF_STRUCT]
    YC_COEFF_STRUCT Cb;   // Cb通道系数 | [YC_COEFF_STRUCT]
    YC_COEFF_STRUCT Cr;   // Cr通道系数 | [YC_COEFF_STRUCT]
} ISP_YC_GAIN_STRUCT;

typedef struct _ISP_YC_GAIN_CFG_S_
{
    FH_UINT8 u08YcGainMode;          // rgb2yuv的转换方式，0：BT601（默认）;1：BT709;2：用户自定义 | [0~2]
    ISP_YC_GAIN_STRUCT strYcGain;   // 用户自定义rgb2yuv的转换矩阵系数 | [YC_COEFF_STRUCT]
} ISP_YC_GAIN_CFG;

/**|STATISTICS|**/
typedef enum _HIST_MODE
{
    Y_MODE = 0,//1024段统计
    RGB_MODE = 1,//4通道256段统计
} HIST_MODE;

typedef enum _STAT_POS_SEL_AWB_ADV
{
    POS_SEL_AWB_ADV_AFTER_DG0 = 0,//ISP_DG0后进行AWB统计
    POS_SEL_AWB_ADV_AFTER_DG3 = 1,//ISP_DG3后进行AWB统计
    POS_SEL_AWB_ADV_AFTER_LUT = 2,//ISP_LUT后进行AWB统计
    POS_SEL_AWB_ADV_AFTER_LSC = 3,//ISP_LSC后进行AWB统计
}STAT_POS_SEL_AWB_ADV;

typedef enum _STAT_POS_SEL_GL
{
    POS_SEL_GL_AFTER_DG2 = 0,//ISP_DG2后进行AE统计
    POS_SEL_GL_AFTER_DG3 = 1,//ISP_DG3后进行AE统计
    POS_SEL_GL_AFTER_WB3 = 2,//ISP_WB3后进行AE统计
}STAT_POS_SEL_GL;

typedef enum _STAT_POS_SEL_AF
{
    POS_SEL_AF_AFTER_LUT  = 0,//ISP_LUT后进行AF统计
    POS_SEL_AF_AFTER_DG2  = 1,//ISP_DG2后进行AF统计
    POS_SEL_AF_AFTER_NR3D = 2,//ISP_DG2后进行AF统计，FH885XV300不支持
    POS_SEL_AF_AFTER_DRC  = 3,//ISP_DG2后进行AF统计，FH885XV300不支持
}STAT_POS_SEL_AF;

typedef enum _STAT_POS_SEL_HIST
{
    POS_SEL_HIST_AFTER_DG2 = 0,//ISP_DG2后进行HIST统计
    POS_SEL_HIST_AFTER_DG3 = 1,//ISP_DG3后进行HIST统计
    POS_SEL_HIST_AFTER_WB3 = 2,//ISP_WB3后进行HIST统计
}STAT_POS_SEL_HIST;

typedef enum _STAT_POS_SEL_MD
{
    POS_SEL_MD_AFTER_DG2 = 0,//ISP_DG2后进行MD统计
    POS_SEL_MD_AFTER_DG3 = 1,//ISP_DG3后进行MD统计
    POS_SEL_MD_AFTER_WB3 = 2,//ISP_WB3后进行MD统计
}STAT_POS_SEL_MD;

typedef enum _STAT_POS_SEL_DRC
{
    POS_SEL_DRC_MATCH_WB3 = 0,//ISP_MATCH_WB3后进行DRC统计
    POS_SEL_DRC_WB3 = 1,//ISP_WB3后进行DRC统计
}STAT_POS_SEL_DRC;

typedef struct _ISP_STAT_WIN_S_
{
    FH_UINT16 winHOffset;  // 统计窗口水平偏移 | [0~PicW]
    FH_UINT16 winVOffset;  // 统计窗口垂直偏移 | [0~PicH]
    FH_UINT16 winHSize;    // 统计窗口宽度 | [0~(PicW-winHOffset)]
    FH_UINT16 winVSize;    // 统计窗口高度 | [0~(PicH-winVOffset)]
} ISP_STAT_WIN;

typedef struct _ISP_STAT_WIN1_S_
{
    FH_UINT16 winHOffset;  // 统计窗水平方向偏移 | [0~0xfff]
    FH_UINT16 winVOffset;  // 统计窗垂直方向偏移 | [0~0xfff]
    FH_UINT16 winHSize;    // 统计窗总宽度 winHOffset+winHSize<=picW | [0~0xfff]
    FH_UINT16 winVSize;    // 统计窗总高度 winHOffset+winHSize<=picH| [0~0xfff]
    FH_UINT8 winHCnt;      // 全局统计水平方向窗口个数 | [1~32]
    FH_UINT8 winVCnt;      // 全局统计垂直方向窗口个数 | [1~32]
} ISP_STAT_WIN1;

typedef struct _ISP_AF_WIN_INFO_S
{
    ISP_STAT_WIN stStatWin;      // 统计窗口0配置,需注意统计窗口0和1不能有交叠 | [ISP_STAT_WIN]
    ISP_STAT_WIN1 stStatWin1;    // 统计窗口1配置,统计窗口1的水平和垂直个数均不大于32，统计窗的宽高均不能大于0x1ff| [ISP_STAT_WIN1]
    STAT_POS_SEL_AF StatPosSel;  // 统计位置 | [0~1]
} ISP_AF_STAT_CFG;

typedef struct _ISP_AF_WIN_INFO_S_V2
{
    ISP_STAT_WIN1 stStatWin1;    // 统计窗口1配置,统计窗口1的水平和垂直个数均不大于32，统计窗的宽高均不能大于0x1ff| [ISP_STAT_WIN1]
    STAT_POS_SEL_AF StatPosSel;  // 统计位置 | [0~3]
} ISP_AF_STAT_CFG_V2;

typedef struct _ISP_AF_STAT_PIXEL_S
{
    FH_UINT32 line_e;    // 偶数行统计结果 | [0~0x3fffffff]
    FH_UINT32 line_o;    // 奇数行统计结果 | [0~0x3fffffff]
    FH_UINT32 column_e;  // 偶数列统计结果 | [0~0x3fffffff]
    FH_UINT32 column_o;  // 奇数列统计结果 | [0~0x3fffffff]
    FH_UINT32 cnt;  // 窗口中大于设定阈值的个数 | [0~0x3ffff]
    FH_UINT32 sum;  // 窗口中大于设定阈值的累加和 | [0~0x3fffffff]
} ISP_AF_STAT_DATA;

typedef struct _ISP_AF_STAT_S
{
    ISP_AF_STAT_DATA stat_win1[1024];  // AF窗口1统计信息 | [ISP_AF_STAT_DATA]
    ISP_AF_STAT_DATA stat_win0;       // AF窗口0统计信息 | [ISP_AF_STAT_DATA]
} ISP_AF_STAT;

typedef struct _AWB_BLOCK_STAT_S_
{
    FH_UINT32 u32AwbBlockR;    // AWB R分量sum统计值 | [0~0xffffffff]
    FH_UINT32 u32AwbBlockG;    // AWB G分量sum统计值 | [0~0xffffffff]
    FH_UINT32 u32AwbBlockB;    // AWB B分量sum统计值 | [0~0xffffffff]
    FH_UINT32 u32AwbBlockCnt;  // AWB 统计值cnt值 | [0~0xffffffff]
} AWB_BLOCK_STAT;

typedef struct _ISP_AWB_STAT_CFG_S_
{
    ISP_STAT_WIN stStatWin;       // 统计窗口配置 | [ISP_STAT_WIN]
    FH_UINT16 u16YHighThreshold;  // awb统计上門限值 | [0~0xfff]
    FH_UINT16 u16YLowThreshold;   // awb统计低門限值 | [0~0xfff]
    STAT_WHITE_POINT stPoint[6];  // awb白框点，A，B，C，D，E，F六个点，Ax=Bx By=Cy Dy=Ey Ex=Fx | [STAT_WHITE_POINT]
} ISP_AWB_STAT_CFG;

typedef struct _ISP_AWB_STAT2_S_
{
    AWB_BLOCK_STAT stBlkStat;  // AWB统计,最大支持32*32,位置可选择在长帧上或合成后,具体可看框图 | [AWB_BLOCK_STAT]
} ISP_AWB_ADV_STAT;



typedef struct _ISP_AWB_ADV_STAT_CFG_S_
{
    STAT_POS_SEL_AWB_ADV StatPosSel; // 统计位置选择,0:合成前长帧位置,1:合成后,默认linear选1,wdr下选0 | [0~3]
    ISP_STAT_WIN1 stStatWin;      // 统计窗口配置 | [ISP_STAT_WIN1]
    FH_UINT16 u16YHighThreshold;  // awb统计上門限值 | [0~0xfff]
    FH_UINT16 u16YLowThreshold;   // awb统计低門限值 | [0~0xfff]
    STAT_WHITE_POINT stPoint[6];  // awb白框点，A，B，C，D，E，F六个点，Ax=Bx By=Cy Dy=Ey Ex=Fx | [STAT_WHITE_POINT]
} ISP_AWB_ADV_STAT_CFG;

typedef struct _ISP_AWB_FRONT_STAT_S_
{
    AWB_BLOCK_STAT stBlkStat[64];  // AWB 8*8统计,位于短帧上 | [AWB_BLOCK_STAT]
} ISP_AWB_FRONT_STAT;

typedef union _ISP_HIST_STAT_S_
{
    struct _hist_bin {
        FH_UINT32 cnt;
    }bin[1024];         // hist统计值，该统计值包含1024个bin
    struct _ch_bin{
        FH_UINT32 cnt[256];
    }ch[4];
} ISP_HIST_STAT;

typedef struct _ISP_HIST_STAT_CFG_S_
{
    ISP_STAT_WIN stStatWin;        // 统计窗口配置 | [ISP_STAT_WIN]
    HIST_MODE StatMode;            // 统计模式 | [0~1]
    STAT_POS_SEL_HIST StatPosSel;  // 统计位置 | [0~2]
} ISP_HIST_STAT_CFG;

typedef struct _GLOBE_STAT_S
{
    struct _Block_gstat
    {
        FH_UINT32 sum;  // 全局统计sum值 | [0~0xffffffff]
        FH_UINT32 cnt;  // 全局统计cnt值 | [0~0xffffffff]
        FH_UINT32 max;  // 统计块内出现的亮度最大值 | [0~0xffffffff]
        FH_UINT32 min;  // 统计块内出现的亮度最小值 | [0~0xffffffff]
    } r, gr, gb, b;
} GLOBE_STAT;

typedef struct _GLOBE_STAT_CFG_S
{
    ISP_STAT_WIN1 stStatWin;  // 统计窗口配置,对于global统计,winHSize和winVSize需小于512,窗口个数较小时需注意 | [ISP_STAT_WIN1]
    FH_UINT16 u16ChnThH;      // 统计像素上阈值,默认0xfff | [0x0~0xfff]
    FH_UINT16 u16ChnThL;      // 统计像素下阈值,默认0 | [0x0~0xfff]
    STAT_POS_SEL_GL StatPosSel;  // 统计位置 | [0~2]
} GLOBE_STAT_CFG;

typedef struct _DEHAZE_STAT_S
{
    FH_UINT32 u32DehazeRGB[2048];  // Dehaze 32*64分区窗共2048个RGB三个通道大气光统计值：B(bit20~29)|G(bit10~19)|R(bit0~9) | [0~0x3fffffff]
    FH_UINT16 u16DehazeIdx;        // 统计最小值中极大值所在窗Index | [0~0x7ff]
} DEHAZE_STAT;

/**|FAST_BOOT|**/
typedef struct _BLC_INIT_CFG_S
{
    FH_UINT16 blc_level; //黑电平值  | [0~0xffff]
} BLC_INIT_CFG;

typedef struct _WB_INIT_CFG_S
{
    FH_UINT16 wbGain[3];  // 白平衡三通道增益值rgain, ggain, bgain; | [0~0x1fff]
} WB_INIT_CFG;

typedef struct _CCM_INIT_CFG_S
{
    FH_UINT8  ccmIdx;     // 选择使用ccm的序号 | [0~1]，此平台只支持一个ccm 0
    FH_SINT16 ccmCfg[9];  // 颜色矫正矩阵 | [-4096~4095]
} CCM_INIT_CFG;

typedef struct _DPC_INIT_CFG_S
{
    FH_UINT8 ctrlMode;  // 去坏点模式 | [0~3]
    FH_UINT8 str;  // 去坏点比例門限值 | [0~7]
} DPC_INIT_CFG;

typedef struct _APC_INIT_CFG_S
{
    FH_UINT8 edgeStr;  // 边界锐化增益，对应drv apc_edge_weight, 低4bits无效 U8| [0x0~0xff]
    FH_UINT8 detailStr;  // 细节增强增益, 对应drv apc_detail_weight, 低4bits无效 U8| [0x0~0xff]
    FH_UINT8 positiveStr;  // 总体正向增益, U8 | [0~0xff]
    FH_UINT8 negativeStr;  // 总体负向增益, U8 | [0~0xff]
} APC_INIT_CFG;

typedef struct _YNR_INIT_CFG_S
{
    FH_UINT8 th_str[3];  // th_str[0] 高频亮度阈值, th_str[1] 中频亮度阈值, th_str[2] 低频亮度阈值，U4.4 | [0~0xff]
} YNR_INIT_CFG;

typedef struct _CTR_INIT_CFG_S
{
    FH_UINT16 ctr;  // 对比度增强因子,U2.6 | [0~255]
} CTR_INIT_CFG;

typedef struct _SAT_INIT_CFG_S
{
    FH_UINT16 sat;  // 饱和度调整因子,U3.5 | [0~0xff]
    FH_UINT16 blue_sup;  // 饱和度蓝色分量,U2.6 | [0~200]
    FH_UINT16 red_sup;  // 饱和度红色分量,U2.6 | [0~200]
} SAT_INIT_CFG;

typedef struct _GAMMA_MODE_INIT_CFG_S
{
    FH_UINT16 gamma_mode;  // 当前gamma运行模式 0：gamma关 1：YC 2：RGB 3：RGBY | [0~3]
} GAMMA_MODE_INIT_CFG;

typedef struct _GAMMA_INIT_CFG_S
{
    FH_UINT32 gamma[512];  // gamma表对应的值 | [0~0xffffffff]
} GAMMA_INIT_CFG;

typedef struct _ISP_GAIN_INIT_CFG_S
{
    FH_UINT32 u32IspGain; // isp增益值 [0~0xffffffff]
} ISP_GAIN_INIT_CFG;

typedef struct _MAP_INTT_GAIN_CFG_S
{
    FH_SINT32 currFormat;  //当前的sensor format | [0~0xffffffff]
    FH_SINT32 desFormat;   //目标的sensor format | [0~0xffffffff]
    FH_UINT32 currIntt;    //当前的曝光时间，单位为行 | [0~0xffffffff]
    FH_UINT32 currTotalGain; //当前的曝光增益，u.6精度 | [0~0xffffffff]
    FH_UINT32 desIntt;     //计算返回的曝光时间，单位为行 | [0~0xffffffff]
    FH_UINT32 desTotalGain; //计算返回的曝光增益，u.6精度 | [0~0xffffffff]
} MAP_INTT_GAIN_CFG;

typedef struct _GPIO_PARAM_CFG_S
{
    FH_BOOL gpio_enable;  //第二阶段是否需要拉高拉低GPIO | [0-0x1]
    FH_UINT32 gpio_time;  //第二阶段拉高拉低需要配置的延时　| [0-0xffffffff]
} GPIO_PARAM_CFG;

typedef struct _BLC_PARAM_CFG_S
{
    FH_UINT32 blc_value;  //配置的第一阶段blc值 | [0~0xffff]
} BLC_PARAM_CFG;

typedef union _ISP_SENSOR_COMMON_CMD_DATA0_U_
{
    BLC_PARAM_CFG blc_param_cfg;        // BLC参数 | [BLC_PARAM_CFG]
    GPIO_PARAM_CFG gpio_param_cfg;      // GPIO相关的参数配置 | [GPIO_PARAM_CFG]
    MAP_INTT_GAIN_CFG mapInttGainCfg;   // 曝光相关的一些参数 | [MAP_INTT_GAIN_CFG]
    unsigned int dw[128];               // 其他的一些保留参数 | [0~0xffffffff]
} ISP_SENSOR_COMMON_CMD_DATA0;

typedef union _ISP_SENSOR_COMMON_CMD_DATA1_
{
    unsigned int dw[128];               // 其他的一些保留参数 | [0~0xffffffff]
} ISP_SENSOR_COMMON_CMD_DATA1;
/*******************************************************************/


/**|DBG|**/
enum _DBG_OUT_DATA_TYPE_
{
    DATA_TYPE_RAW = 0,  // DBG模块的导出的数据格式为RAW格式
    DATA_TYPE_RGB = 1,  // DBG模块的导出的数据格式为RGB格式
    DATA_TYPE_YUV = 2,  // DBG模块的导出的数据格式为YUV422格式
    DATA_TYPE_YUV444 = 3,  // DBG模块的导出的数据格式为YUV444格式
};

typedef struct
{
    FH_CHAR     name[16];   // DBG支持的节点的名字 | [字符串列表]
    const FH_UINT8    u8DumpPos;  // dump数据的节点，不用配置 | [用户不敏感]
    const FH_UINT8    u8DataMode;  // dump数据的格式 | [用户不敏感]
    const FH_UINT8    u8DataWidth;  // dump数据的位宽 | [用户不敏感]
    const FH_UINT8    u8PipeLinePos;  // dump数据的流水线位置 | [用户不敏感]
    FH_UINT16   u16DataStride;  // dump数据的硬件写步长 | [用户不敏感]
    FH_UINT32   u32TotalSize;  // dump数据的总的大小 | [用户不敏感]
}DbgInteralInfoT;

typedef struct
{
    DbgInteralInfoT dbgInfo;  // 模块内部使用的信息 | [DbgInteralInfoT]
    FH_UINT8    u8Enable;  // 模块使能开关 | [0~1]
    FH_PHYADDR  RDataBase;  // 模块使用的r数据地址 | []
    FH_PHYADDR  GDataBase;  // 模块使用的g数据地址 | []
    FH_PHYADDR  BDataBase;  // 模块使用的b数据地址 | []
}DbgDump_T;

/**|RUBBISH|**/
typedef enum _SNS_CLK_S_ {
    SNS_CLK_24_POINT_0   = 0x1,
    SNS_CLK_27_POINT_0   = 0x2,
    SNS_CLK_37_POINT_125 = 0x3,
    SNS_CLK_DUMMY =0xffffffff,
} SNS_CLK;

typedef enum _SNS_DATA_BIT_S_ {
    LINER_DATA_8_BITS  = 0x1,
    LINER_DATA_12_BITS = 0x2,
    LINER_DATA_14_BITS = 0x3,
    WDR_DATA_16_BITS   = 0x4,
    SNS_DATA_BITS_DUMMY =0xffffffff,
} SNS_DATA_BITS;

typedef enum _SIGNAL_POLARITY_S_ {
    ACTIVE_HIGH = 0x0,
    ACTIVE_LOW = 0x1,
    SIGNAL_POLARITY_DUMMY =0xffffffff,
} SIGNAL_POLARITY;

typedef struct _ISP_VI_HW_ATTR_S_
{
    SNS_CLK         eSnsClock;         // 配置的cis时钟 | [SNS_CLK]
    SNS_DATA_BITS   eSnsDataBits;      // 配置的sensor数据位宽 | [SNS_DATA_BITS]
    SIGNAL_POLARITY eHorizontalSignal; // 时钟水平极性 | [0~1]
    SIGNAL_POLARITY eVerticalSignal;   // 时钟垂直极性 | [0~1]
    FH_BOOL         u08Mode16;         //  | [ ]
    FH_UINT32       u32DataMaskHigh;   //  | [ ]
    FH_UINT32       u32DataMaskLow;    //  | [ ]
} ISP_VI_HW_ATTR;

typedef struct _ISP_ALGORITHM_S_
{
    FH_UINT8 u08Name[16];    //  | [ ]
    FH_UINT8 u08AlgorithmId; //  | [ ]
    FH_VOID (*run)(FH_VOID); //  | [ ]
} ISP_ALGORITHM;

typedef struct _STATIC_DPC_CFG_S_
{
    FH_BOOL   bStaticDpcEn;
    char *    u32DpcTable0;
    char *    u32DpcTable1;
} STATIC_DPC_CFG;

typedef struct _ISP_DEBUG_INFO_S_
{
    FH_UINT32 envLuma;
    FH_UINT32 sqrtenvLuma;
    FH_UINT32 sensor_gain;
    FH_UINT32 isp_gain;
} ISP_DEBUG_INFO;

typedef struct _ISP_DEFAULT_PARAM_
{
    ISP_BLC_CFG        stBlcCfg;     //　 | [ ]
    ISP_DRC_CFG        stDRCCfg;     //  | [ ]
    ISP_GAMMA_CFG      stGamma;      //  | [ ]
    ISP_SAT_CFG        stSaturation; //  | [ ]
    ISP_APC_CFG        stApc;        //  | [ ]
    ISP_CONTRAST_CFG   stContrast;   //  | [ ]
    ISP_BRIGHTNESS_CFG stBrt;        //  | [ ]
    ISP_NR3D_CFG       stNr3d;       //  | [ ]
    ISP_NR2D_CFG       stNr2d;       //  | [ ]
    ISP_YNR_CFG        stYnr;        //  | [ ]
    ISP_CNR_CFG        stCnr;        //  | [ ]
    ISP_DPC_CFG        stDpc;        //  | [ ]
    ISP_LSC_CFG        stLscCfg;     //  | [ ]
} ISP_DEFAULT_PARAM;

typedef enum
{
    ISP_RO_OPS_0     = 0,// 旋转0度
    ISP_RO_OPS_90    = 1,// 旋转90度
    ISP_RO_OPS_180   = 2,// 旋转180度
    ISP_RO_OPS_270   = 3,// 旋转270度
    ISP_ROTATE_OPS_DUMMY= 0xffffffff,
}ISP_ROTATE_OPS_E;

typedef struct
{
    FH_BOOL         bLockBuf;//是否将获取的buffer锁住 | [0~1]
    FH_BOOL         bVgsEn;  //是否获取vgs翻转后的数据  | [0~1]

    FH_UINT8        u8BufIdx;//获取到数据的buffer索引,释放数据时使用 | [0~0xff]
    FH_VIDEO_FRAME  stFrm;   //获取到的帧数据 | [FH_VIDEO_FRAME]
}ISP_OUT_STREAM_S;

#pragma pack()
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*_ISP_COMMON_H_*/
