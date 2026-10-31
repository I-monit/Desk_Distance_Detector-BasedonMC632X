#ifndef _ISP_COMMON_H_
#define _ISP_COMMON_H_

#include "types/type_def.h"
#include "types/bufCtrl.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
#pragma pack(4)

#define MALLOCED_MEM_BASE_ISP (0)

#define MALLOCED_MEM_ISPPPKG (0)
#define MALLOCED_MEM_ISPFPKG (1)
#define MALLOCED_MEM_ISPFPKG_L (2)
#define MALLOCED_MEM_ISPFPKG_L2 (4)
#define MALLOCED_MEM_ISPFPKG_L3 (5)

#define MALLOCED_MEM_DRV (3)

#define GAIN_NODES 12
#define INTT_NODES 8

typedef int (*ispInitCfgCallback)(FH_UINT32 u32DevId);
typedef int (*ispIntCallback)(void);
void ispTrace_FunctionName(FH_UINT32 u32DevId, const char* func);

/**|SYSTEM_CONTROL|**/
typedef enum _ISP_BAYER_TYPE_S_ {
    BAYER_RGGB = 0x0, // 色彩模式RGGB
    BAYER_GRBG = 0x1, // 色彩模式GRBG
    BAYER_BGGR = 0x2, // 色彩模式BGGR
    BAYER_GBRG = 0x3, // 色彩模式GBRG
    ISP_BAYER_TYPE_DUMMY=0xffffffff,
} ISP_BAYER_TYPE;

typedef enum _TAB_COLOR_CODE {
    CC_R  = 0,
    CC_GR = 1,
    CC_B  = 2,
    CC_GB = 3,
    TAB_COLOR_CODE_DUMMY =0xffffffff,
} TAB_COLOR_CODE;

typedef enum _CURVE_TYPE_ {
    CURVE_BUILTIN     = 0x1, // 选用内定曲线
    CURVE_USER_DEFINE = 0x2, // 选用用户自定义曲线
    CURVE_TYPE_DUMMY  =0xffffffff,
} CURVE_TYPE;

typedef enum __ISP_STITCH_MODE__
{
    ISP_STITCH_FREE_RUN     = 0,     // sns非同步模式时的拼接
    ISP_STITCH_SYNC_MODE    = 1,     // sns同步模式时的拼接
    ISP_SYNC_AE_AWB         = 2,     // sns同步多路的效果
    ISP_STITCH_BUTT,                 // sns同步模式最大值
}ISP_STITCH_MODE_E;

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

typedef enum _ISP_MOD_TYPE_E_
{
    ISP_MOD_NR3D_COEF = 0,  // nr3d coef
    ISP_MOD_NR2D_COEF = 1,  // nr2d coef
    ISP_MOD_YC3D_YCOEF = 2,  // yc3d ycoef
    ISP_MOD_YC3D_CCOEF = 3,  // yc3d ycoef
    ISP_MOD_CANCEL_NR3D_COEF = 4,  // cancel nr3d coef
    ISP_MOD_CANCEL_NR2D_COEF = 5,  // cancel nr2d coef
    ISP_MOD_BUTT,
}ISP_MOD_TYPE_E;

typedef enum __ISP_SENSOR_MODE_E__
{
    SENSOR_ENTER_SELFSYNC_MASTER = 0,
    SENSOR_LEAVE_SELFSYNC_MASTER = 1,
    SENSOR_ENTER_SELFSYNC_SLAVE = 2,
    SENSOR_LEAVE_SELFSYNC_SLAVE = 3,
    SENSOR_MOD_BUTT = 0xffffffff,
}ISP_SENSOR_MODE_E;

typedef struct _ISP_PIC_SIZE_S_
{
    FH_UINT32 u32Width;     // 输入幅面宽度 | [0~0xffff]
    FH_UINT32 u32Height;    // 输入幅面高度 | [0~0xffff]
} ISP_PIC_SIZE;

typedef struct _FH_ISP_PIC_SIZE_S_
{
    FH_UINT16 u16Width;     // 输入幅面宽度 | [0~0xffff]
    FH_UINT16 u16Height;    // 输入幅面高度 | [0~0xffff]
} FH_ISP_PIC_SIZE;

typedef struct _ISP_BUF_INFO_S
{
    FH_UINT32 u32Addr;  // 地址信息 | [0~0xffffffff]
    FH_UINT32 u32BufSize;  // buf大小 | [0~0xffffffff]
}ISP_BUF_INFO_S;
typedef struct _ISP_VI_ATTR_
{
    FH_UINT16      u16WndHeight;   ///<sensor幅面高度 | [0~0xffff]
    FH_UINT16      u16WndWidth;    /// <sensor幅面宽度 | [0~0xffff]
    FH_UINT16      u16InputHeight; ///<sensor输入图像高度 | [0~0x1fff]
    FH_UINT16      u16InputWidth;  ///<sensor输入图像宽度 | [0~0x1fff]
    FH_UINT16      u16OffsetX;     ///<裁剪水平偏移 | [0~0xfff]
    FH_UINT16      u16OffsetY;     ///<裁剪垂直偏移 | [0~0xfff]
    FH_UINT16      u16PicHeight;   ///<处理的图像高度 | [0~0x1fff]
    FH_UINT16      u16PicWidth;    ///<处理的图像宽度 | [0~0x1fff]
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

typedef struct _ISP_VIGEN_CFG_S
{
    FH_BOOL   bViGenEn;     // VIGEN使能 0：关闭 1：打开。| [0~1]
    FH_UINT32 u32phyAddr;   //线性图像（或者wdr合成后的图像）导入到ddr空间的起始地址,大小计算方式：1.非合成的短（长）帧为长×宽×1.5;2合成后的为长x宽×2| [0~0xffffffff]
    FH_UINT32 u32PicWidth;  // 图像宽度 | [0~0xffffffff]
    FH_UINT32 u32PicHeight; // 图像高度 | [0~0xffffffff]
    FH_UINT32 u32FrameWidth;// 框架宽度 | [0~0xffffffff]
    FH_UINT32 u32FrameHeight;// 框架高度 | [0~0xffffffff]
    FH_UINT8  u08ViGenMode; // 导入模式 0.多张图循环切换模式（暂不支持） 1.一张图连续出图模式  | [0~1]
    FH_UINT8  u08FrameCnt;  // 多图模式的帧数 | [0~0xff]
} ISP_VIGEN_CFG;

typedef struct _ISP_VI_INFO_S
{
    FH_UINT32 u32DevId;     // 设备号 | [0~0xffffffff]
    FH_UINT16 u16PicWidth;  // 图像宽度 | [0~0xffff]
    FH_UINT16 u16PicHeight; // 图像高度 | [0~0xffff]
    FH_UINT32 u32phyAddr;   // 线性图像（或者wdr合成后的图像）导入到ddr空间的起始地址,大小计算方式：1.非合成的短（长）帧为长×宽×1.5;2合成后的为长x宽×2| [0~0xffffffff]
    FH_UINT64 u64FrameCnt;  // [0~0xffffffffffffffff]
    FH_UINT32 u32FrameSize; // 帧宽度 | [0~0xffffffff]
    FH_UINT64 u64TimeStamp; // [0~0xffffffffffffffff]
    FH_UINT64 u64FrmIdx; // [0~0xffffffffffffffff]
} ISP_VI_INFO;

typedef struct
{
    FH_UINT32  addrPhy;     // 数据的物理地址 | [FH_PHYADDR]
    FH_UINT32  u32VirAddr;  // 应用层的虚拟地址，同一个进程空间可直接使用 | [FH_UINT32]
    FH_UINT32  u32Crc;      // 数据校验值   | [0~0xffffffff]
    FH_UINT64  u64TimeStamp;// 数据获取的时间戳 | [0~0xffffffffffffffff]
    FH_UINT32  u32BufSize;  // 获取到的离线数据的缓存大小 | [0~0xffffffff]
    FH_UINT32  u32LineStride;   // 一行的数据量，单位是字节 | [0~0xffffffff]
}ISP_STREAM_COM_S;

typedef struct
{
    FH_UINT8   u8DevId;     // 设备号 | [0~2]
    FH_BOOL    bLock;       // 是否加锁 | [0~1]
    FH_BOOL    bBlock;      // 是否使用阻塞，阻塞支持线程调度 | [0~1]
    FH_UINT32  u32TimeOut;  // 超时时间，阻塞和非阻塞都可以配置超时时间，单位是ms | [0~0xffffffff]
    FH_UINT32  u32PollId;   // 释放FIFO所需要的Id号 | [0~0xffffffff]
    FH_UINT64  u64FrmCnt;   // 帧号 | [0~0xffffffff]
    FH_ISP_PIC_SIZE stPic;      // 图像幅面相关的配置 | [ISP_PIC_SIZE]
    ISP_STREAM_COM_S stSf;  // 短帧的数据流 | [ISP_STREAM_COM_S]
    ISP_STREAM_COM_S stLf;  // 长帧的数据流 | [ISP_STREAM_COM_S]
}ISP_STREAM_S;

typedef struct _ISP_MOD_ADDR_S_
{
    ISP_MOD_TYPE_E enMod; // 选择的模块类型 | [ISP_MOD_TYPE_E]
    FH_UINT32 u32Addr;  // 预期要配置的模块地址 | [合法的内存地址]
    FH_UINT32 u32Stride; // 预期要配置的行间距 | [合法的行间距大小]
}ISP_MOD_ADDR_S;

typedef enum _ISP_SEM_TYPE_E_ {
    ISP_WAIT_PRE_START = 0,    // ISP第一段流水开始信号
    ISP_WAIT_POST_START = 1,    // ISP第二段流水开始信号
    ISP_WAIT_PRE_END   = 2,    // ISP第一段流水完成信号
    ISP_WAIT_POST_END   = 3,    // ISP第二段流水完成信号
    ISP_WAIT_BUTT,    // 
} ISP_SEM_TYPE_E;

typedef struct _ISP_RAW_BUF_S
{
    struct mem_desc raw_mem;     // 　raw_mem | [-]
    FH_UINT32 frameCnt;  //　分配缓存的块数，实际最大取值受到可利用空闲空间的限制 | [0~0xffffffff]
} ISP_RAW_BUF;

typedef struct _ISP_PARAM_CONFIG_S_
{
    FH_UINT32 u32BinAddr;// ISP参数存放的当前地址 | [0~0xffffffff]
    FH_UINT32 u32BinSize;// ISP参数的大小 | [0~0xffffffff]
} ISP_PARAM_CONFIG;

typedef struct _ISP_STITCH_CFG_S_
{
    FH_UINT8             u8GroupId;     // 拼接组的序号 | [0~1]
    ISP_STITCH_MODE_E    u8StitchMode;  // 拼接模式 | [ISP_STITCH_MODE_E]
    FH_BOOL              bStitchEn;     // 是否拼接 | [0~1]
    FH_BOOL              bGrpTimeEn;    // 是否时间同步 | [0~1]
    FH_UINT32            u32MainDevId;  // 主通路的devId | [0~3]
    FH_UINT32            u32DevNum;     // 拼接组的通路个数，按照实际初始化填写 | [0~3]
    FH_UINT32            u32TimeThreshold; // 时间间隔单位是us，同步拼接模式下，超过该时间间隔会失同步 | [0~0xffffffff]
    FH_UINT32            au32DevId[4];  // 各个通路的id | [0~3]
} ISP_STITCH_CFG_S;

typedef struct __ISP_AOV_MODE_CFG_S
{
    FH_BOOL bEn;                // aov模式开启使能
    FH_UINT32 u32RunCnt;        // 运行多少帧之后停止流水
}ISP_AOV_MODE_CFG_S;

/**|MIRROR|**/
typedef struct mirror_cfg_s
{
    FH_BOOL             bEN;  // 镜像使能开关 | [0~1]
    ISP_BAYER_TYPE      normal_bayer;  // 无效成员变量 | [0~3]
    ISP_BAYER_TYPE      mirror_bayer;  //　无效成员变量 | [0~3]
}MIRROR_CFG_S;


typedef enum _OFFLINE_MODE_{
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
}ISP_OUT_FORMAT_E;

typedef enum _LUT2D_WORK_MODE_E_
{
    ISP_LUT2D_BYPASS  = 0,// 2dlut 未启用
    ISP_LUT2D_ONLINE  = 1,// 2dlut 在线模式
    ISP_LUT2D_OFFLINE = 2,// 2dlut 离线模式
}LUT2D_WORK_MODE_E;

typedef enum _ISP_RUN_POS_SEL_E
{
    ISP_RUN_POS_START = 0,
    ISP_RUN_POS_END = 1,
}ISP_RUN_POS_SEL_E;

typedef struct _ISP_MEM_INIT_S_
{
    ISP_OFFLINE_MODE  enOfflineWorkMode; // 当前ISP工作模式，不支持 | [OFFLINE_MODE]
    ISP_OUT_MODE_E    enIspOutMode;      // ISP输出的方式，不支持 | [ISP_OUT_MODE_E]
    ISP_OUT_FORMAT_E  enIspOutFmt;       // ISP离线输出的格式，不支持 | [ISP_OUT_FORMAT]
    LUT2D_WORK_MODE_E enLut2dWorkMode;   // Lut缓存分配控制，仅影响内存分配，不支持 | [0~1]
    ISP_PIC_SIZE      stPicConf;         // 幅面配置 | [ISP_PIC_SIZE]
} ISP_MEM_INIT;

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
    FH_UINT8 u08SmartOpStatus;         // 当前智能曝光状态 | [0x0-0x2]
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
    FH_BOOL bAntiFlickerEn;  // 自动抗闪使能   0：关闭抗闪模式 1：打开抗闪模式 | [0x0-0x1]
    FH_BOOL bAdaptiveEn;     // 自适应参考亮度    0：关闭自适应参考亮度，亮度只由lumaRef决定      1：打开自适应参考亮度，亮度跟随环境光线变化 | [0x0-0x1]
    FH_BOOL bARouteEn;       // 自动曝光路线使能（更新时需置位refresh位）   0：使用默认曝光路线     1：使用手动曝光路线 | [0x0-0x1]
    FH_BOOL bInttUnitSel;    // 曝光时间单位选择：       0: 曝光时间单位使用us   1: 曝光时间单位使用行 | [0x0-0x1]
    FH_BOOL bInttFineMode;   // Intt内插模式选择 0：使用dGain内插 1：使用aGain内插 | [0x0-0x1]
    FH_BOOL bSensUpMode;     // 降帧模式选择 0：无极降帧 1：根据降帧精度选择 | [0x0-0x1]
    FH_UINT8 u08SensUpPrec;  // 降帧精度 | [0x0-0xf]
    FH_BOOL bTestDelayEn;    // 测试intt/again/dgain生效时序 | [0x0-0x1]
    FH_UINT8 u08TestModeSel; // 测试参数项选择： 0：测试intt时序 1：测试again时序 2：测试dgain时序 | [0x0-0x2]
} AE_ENABLE_CFG;

typedef struct _AE_INIT_CFG_S_
{
    FH_BOOL bInitAeFlag;       // 快速启动AE开关控制：0：关闭快速启动AE 1：开启快速启动AE | [0x0-0x1]
    FH_UINT8 u08InitSpeed;     // 初始化曝光速度 | [0x0~0xff]
    FH_UINT8 u08InitTolerance; // 初始化曝光的稳定区间，U.4精度 | [0x0-0xff]
    FH_UINT32 u32InitExpTime;  // 初始化曝光时间，单位由inttUnitSel决定，暂不支持 | [0x0~0xfffff]
    FH_UINT16 u16InitAgain;    // 初始化sensor增益，U.6，暂不支持 | [0x40~0xffff]
    FH_UINT16 u16InitDgain;    // 初始化isp增益，U.6，暂不支持 | [0x40~0x3ff]
} AE_INIT_CFG;

typedef struct _AE_SPEED_CFG_S_
{
    FH_UINT8 u08GreatChangeZone;  // 较大亮度变化区间 | [0x0-0xff]
    FH_UINT8 u08Tolerance;        // 一般不超过lumaRef的1%,U.4精度 | [0x0-0xff]
    FH_UINT8 u08LightChangeZone;  // 较小亮度变化区间 | [0x0-0xff]
    FH_UINT8 u08OverExpLDlyFrm;   // 过曝较小调整时延时调整的帧数 | [0x0-0xff]
    FH_UINT8 u08UnderExpGDlyFrm;  // 欠曝较大调整时延时调整的帧数 | [0x0-0xff]
    FH_UINT8 u08UnderExpLDlyFrm;  // 欠曝较小调整时延时调整的帧数 | [0x0-0xff]
    FH_UINT8 u08OverExpGDlyFrm;   // 过曝较大调整时延时调整的帧数 | [0x0-0xff]
    FH_UINT8 u08UExpSpeedBias;    // 欠曝调整时速度偏移量 | [0x0-0xff]
    FH_UINT8 u08RunInterval;      // AE策略运行间隔，值为0时表示每帧运行，值为1时表示隔1帧运行一次，依次类推，一般配成0 | [0x0-0xf]
    FH_UINT8 u08Speed;            // 曝光速度，值越小速率越快，0是最快速率 | [0x0-0xff]
} AE_SPEED_CFG;

typedef struct _AE_METERING_CFG_S_
{
    FH_UINT8 u08StatSel;         // 统计模式：  0：3*3分块统计  1：16*16分块统计    2：hist统计 | [0x0-0x2]
    FH_UINT8 u08LightMode;       // 亮度测光模式(statSel=1或2时生效)：  0：正常模式 1：高光优先 2：低光优先 | [0x0-0x2]
    FH_UINT8 u08BlockMode;       // 分块测光模式(statSel=0或1时生效)：  0：全局测光 1：前光补偿(仅3*3统计生效) 2：背光补偿(仅3*3统计生效) 3：用户自定义 | [0x0-0x3]
    FH_UINT8 u08MeteringBlock;   // 当blockMode = 1/2时生效，表示   0：最强，8最弱 | [0x0-0x8]
    FH_UINT8 u08LumaBoundary;    // 高光或低光亮度截断值(仅在lightMode下有效) | [0x0-0xff]
    FH_UINT8 u08RoiWeight;       // 高光或低光区域对平均亮度影响程度，值越大，对平均亮度影响越大，值为0时不影响(仅在lightMode下有效) | [0x0-0xff]
    FH_UINT16 u16Sensitivity;    // 亮度敏感度，值越大，对高光或低光区域越敏感, U.7(仅在lightMode下有效) | [0x0-0xffff]
    FH_UINT8 u08WeightBlk[256];  // 最大支持256个窗口权值配置（仅meteringMode=5或7，更新时需置位refresh位） | [0x0-0xf]
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
    FH_UINT8 u08LumaRefHigh;  // 高光状态下的参考亮度，仅当adaptiveEn为1且aRouteEn为0时生效，归一化至0~255 | [0x0-0xff]
    FH_UINT8 u08LumaRefLow;   // 低光状态下的参考亮度，仅当adaptiveEn为1且aRouteEn为0时生效，归一化至0~255 | [0x0-0xff]
    FH_UINT8 u08LumaRef;      // 参考亮度，归一化至0~255 | [0x0-0xff]
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
    FH_UINT16 u16SensUpEnableAgain;         // 降帧启动增益，U.6 (aRouteEn为0时生效，更新时需置位refresh位） | [0x40-0xffff]
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

typedef struct _ISP_AE_INFO_3_S_
{
    FH_UINT32   u32InttUs;        // sensor曝光时间us | [0~0xffffffff]
    FH_UINT32   u32IspGain;       // isp增益值U4.8, 0x100表示一倍 | [0~0xfff]
    FH_UINT32   u32SensorGain;    // sensor增益值U.6, 0x40表示一倍 |[0~0xffffffff]
    FH_UINT32   u32TotalGain;     // 总增益U.6 , 0x40表示一倍 | [0~0xffffffff]
} ISP_AE_INFO_3;

typedef struct _ISP_AE_STATUS_S_
{
    FH_UINT8 u08TarLuma;           // 目标亮度 | [0x0-0xff]
    FH_UINT8 u08CurrLuma;          // 当前亮度 | [0x0-0xff]
    FH_UINT16 u16ErrorLuma;        // 当前帧亮度与目标值差异，S.4，tar-curr | [0x0-0x1fff]
    FH_UINT32 u32CurrIntt;         // 当前曝光时间，单位固定是μs | [0x0-0xfffff]
    FH_UINT16 u16CurrPiris;        // 当前光圈值 | [0x0-0xfff]
    FH_UINT16 u16CurrDgain;        // 当前isp增益，U.6 | [0x0-0xffff]
    FH_UINT16 u16CurrAgain;        // 当前sensor增益，U.6 | [0x0-0xffff]
    FH_UINT32 u32CurrTotalGain;    // 当前总增益，U.6 | [0x0-0xffffffff]
    FH_UINT16 u16CurrInttLine;     // 当前曝光时间，单位固定为行 | [0x0-0xffff]
    FH_UINT16 u16CurrFrameHeight;  // 当前帧高度 | [0x0-0xffff]
    FH_UINT16 u16Ev;               // 当前环境光强 | [0x0-0xffff]
    FH_UINT8 u08CurrNodeId;        // 当前节点ID(仅aRouteEn为1时有效) | [0x0-0xf]
    FH_BOOL bOpStatus;             // Ae当前运行状态    0：停止 1：运行 | [0x0-0x1]
    FH_UINT32 u32ExpParam;         // 当前总曝光参数，曝光时间以行数计算，增益均以.6精度 | [0x0-0xffffffff]
    FH_UINT16 u16Piris[16];        // 该节点对应光圈参数 | [0x0-0xfff]
    FH_UINT32 u32Intt[16];         // 该节点对应曝光时间参数，单位由inttUnitSel决定 | [0x0-0xfffff]
    FH_UINT16 u16DGain[16];        // 该节点对应数字增益，U.6 | [0x0-0xffff]
    FH_UINT16 u16AGain[16];        // 该节点对应模拟增益，U.6 | [0x0-0xffff]
    FH_UINT8 u08LumaRef[64];       // 该节点对应参考亮度 | [0x0-0xff]
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
    FH_UINT8 u08TarLuma;         // 当前亮度 | [0x0-0xff]
    FH_UINT32 u32CurrTotalGain;  // 当前总增益，U.6 | [0x0-0xffffffff]
    FH_BOOL bOpStatus;           // Ae当前运行状态  0：停止 1：运行 | [0x0-0x1]
    FH_UINT32 u32CurrIntt;       // 当前曝光时间，单位固定是μs | [0x0-0xfffff]
    FH_UINT16 u16CurrInttLine;   // 当前曝光时间，单位固定为行 | [0x0-0xffff]
    FH_UINT16 u16ActInttMax;     // 策略运行生效的inttmax，以行为单位,非慢快门下不大于u16SensorInttMax | [0x0-0xffff]
    FH_UINT16 u16SensorInttMax;  // Sensor支持的inttmax，以行为单位，不考虑慢快门情况,表示的是一帧之内的最大曝光值 | [0x0-0xffff]
} AE_USER_STATUS;

/**|AWB|**/
typedef struct _AWB_ENABLE_CFG_S_
{
    FH_BOOL bAwbEn;          // Awb总使能 | [0x0-0x1]
    FH_BOOL bMwbEn;          // 手动wb使能，当为1时配置手动R,G,B增益值，当为0时根据统计实时调整 | [0x0-0x1]
    FH_BOOL bPureColorEn;    // 单色物体判断使能 | [0x0-0x1]
    FH_BOOL bCompEn;         // 增益补偿使能 | [0x0-0x1]
    FH_BOOL bInOutEn;        // 室内外场景判断使能 | [0x0-0x1]
    FH_BOOL bLightWeightEn;  // 亮度是否影响权重使能 | [0x0-0x1]
    FH_BOOL bMultiCTEn;      // 混合色温使能 | [0x0-0x1]
    FH_BOOL bAddWPEn;        // 附加色温点使能 | [0x0-0x1]
    FH_BOOL bFineTuneEn;     // 精细调整使能，目前主要用于肤色保护 | [0x0-0x1]
    FH_BOOL bMinMaxSel;      // 0：增益计算采用最小值   1：增益计算采用最大值 | [0x0-0x1]
    FH_BOOL bMinNormEn;      // 增益计算采用最小值时，是否做亮度归一化的使能 | [0x0-0x1]
    FH_UINT8 u08AwbAlgType;  // 0：Gray World   1：Global Alg 2:Advanced Alg| [0x0-0x2]
} AWB_ENABLE_CFG;

typedef struct _AWB_WHITE_POINT_S_
{
    FH_UINT16 u16BOverG;  // 标定白点的B/G分量 | [0-0xffff]
    FH_UINT16 u16ROverG;  // 标定白点的R/G分量 | [0-0xffff]
} AWB_WHITE_POINT;

typedef struct _STAT_WHITE_POINT_S_
{
    FH_UINT16 u16Coordinate_w;  // awb白框横坐标U3.7 | [0-0x3ff]
    FH_UINT16 u16Coordinate_h;  // awb白框纵坐标U3.7 | [0-0x3ff]
} STAT_WHITE_POINT;

typedef struct _AWB_CT_CFG_S_
{
    FH_UINT8 u08ColorTempNum;         // 色温框的个数，范围4~12 | [0x4-0xc]
    FH_UINT8 u08CtWidthThl;           // 每个色温框斜向宽度下阈值，U.7，斜向指垂直于标定色温连线方向,值越大被判定为白点的可能性越高，当前增益小于config21中的ctWidthGainThl时，ctWidth采用ctWidthThl | [0x0-0xff]
    FH_UINT8 u08CtWidthThh;           // 每个色温框斜向宽度上阈值，U.7，斜向指垂直于标定色温连线方向,值越大被判定为白点的可能性越高，当前增益大于config21中的ctWidthGainThh时，ctWidth采用ctWidthThh,当前增益在两个值中间时，线性内插出ctWidth | [0x0-0xff]
    FH_UINT16 u16CtWidthGainThh;      // 色温框斜向宽度的增益上阈值(U12.4) | [0x0-0xffff]
    FH_UINT16 u16CtWidthGainThl;      // 色温框斜向宽度的增益下阈值(U12.4) | [0x0-0xffff]
    AWB_WHITE_POINT stWhitePoint[4];  // 标定白点的结构体 | [AWB_WHITE_POINT]
    STAT_WHITE_POINT stPoint[6];      //  awb白框点,A，B，C，D，E，F六个点，Ax=Bx By=Cy Dy=Ey Ex=Fx | [STAT_WHITE_POINT]
} AWB_CT_CFG;

typedef struct _AWB_LIGHT_CFG_S_
{
    FH_BOOL bLightWeightThMode;    // 0：内置门限值表   1：用户自定义门限值 | [0x0-0x1]
    FH_UINT8 u08LightWeightTh[6];  // 亮度阈值, 单调递增 | [0x0-0xff]
    FH_UINT8 u08LightWeight[6];    // 对应亮度阈值的亮度权重 | [0x0-0xff]
} AWB_LIGHT_CFG;

typedef struct _AWB_SCENE_CFG_S_
{
    FH_UINT8 u08SingleCTRatioThl;   // 单色温比例门限下阈值，原来处于单色温状态时，比例小于该值认为是混合色温。单位是百分比，0~100% | [0x0-0x64]
    FH_UINT8 u08SingleCTRatioThh;   // 单色温比例门限上阈值，原来处于混合色温状态时，比例大于该值认为是单色温。单位是百分比，0~100% | [0x0-0x64]
    FH_UINT8 u08MultiSatScaler;     // 混合色温下饱和度调整比例，增益系数计算为multiSatScaler/0xff,值越小，混合色温下饱和度越低。（暂不支持） | [0x0-0xff]
    FH_UINT8 u08FineTuneStr;        // 精细调整强度，值越大保护越强，，但过大也可能导致不稳定 | [0x0-0xff]
    FH_UINT8 u08MultiCTWeight[12];  // 混合色温下各色温点的权重 | [0x0-0xff]
    FH_UINT8 u08LumaDiffRatioTh;    // 单色亮度差异比例门限值，差异值与平均值的比例小于此值则亮度符合纯色判断，反之则不符合 | [0x0-0x64]
    FH_UINT8 u08ChromaDiffRatioTh;  // 单色色度差异比例门限值，差异值与平均值的比例小于此值则色度符合纯色判断，反之则不符合 | [0x0-0x64]
    FH_BOOL bInOutType;             // 室内外判断类型： 0：自动 1：手动 | [0x0-0x1]
    FH_BOOL bManualType;            // 手动模式下室内外类型：   0：室内 1：室外 | [0x0-0x1]
    FH_UINT16 u16InDoorThresh;      // 室内场景判断曝光时间的阈值，单位是us，大于该曝光时间认为是室内的概率是100% | [0x0-0xffff]
    FH_UINT16 u16OutDoorThresh;     // 室外场景判断曝光时间的阈值，单位是us，不大于该曝光时间认为是室外的概率是100% | [0x0-0xffff]
    FH_UINT8 u08OutWeight[12];      // 室外各色温点的权重 | [0x0-0xff]
} AWB_SCENE_CFG;

typedef struct _AWB_ADD_WHITE_AREA_S_
{
    FH_BOOL bAddWPEn;     // 新增白点使能 | [0x0-0x1]
    FH_UINT8 u08ByWidth;  // 新增白点的色温框B-Y的宽度，U.7 | [0x0-0xff]
    FH_UINT8 u08RyWidth;  // 新增白点的色温框R-Y的宽度，U.7 | [0x0-0xff]
} AWB_ADD_WHITE_AREA;

typedef struct _AWB_ADD_CT_CFG_S_
{
    AWB_ADD_WHITE_AREA stAddWhiteArea[4];  // 新增白点使能及色温框配置 | [AWB_ADD_WHITE_AREA]
    AWB_WHITE_POINT stAddWhitePoint[4];    // 标定新增白点的结构体 | [AWB_WHITE_POINT]
} AWB_ADD_CT_CFG;

typedef struct _ISP_AWB_GAIN_S_ {
    FH_UINT16 u16Rgain;  // 手动R通道增益，U.9 | [0x0-0x1fff]
    FH_UINT16 u16Ggain;  // 手动G通道增益，U.9 | [0x0-0x1fff]
    FH_UINT16 u16Bgain;  // 手动B通道增益，U.9 | [0x0-0x1fff]
} ISP_AWB_GAIN;

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

typedef struct _AWB_DEFAULT_CFG_S_
{
    AWB_ENABLE_CFG stAwbEnableCfg;  // awb使能配置 | [AWB_ENABLE_CFG]
    FH_UINT16 u16Speed;             // 值越大，调整速度越快 | [0x0-0xfff]
    FH_UINT8 u08StatThh;            // 统计像素亮度阈值上限，亮度按255归一化 | [0x0-0xff]
    FH_UINT8 u08StatThl;            // 统计像素亮度阈值下限，亮度按255归一化 | [0x0-0xff]
    FH_BOOL bAwbFlowPre;            // awb流水提前至3d之前 | [0x0-0x1]
    AWB_CT_CFG stAwbCtCfg;          // awb色温相关参数配置 | [AWB_CT_CFG]
    AWB_LIGHT_CFG stAwbLightCfg;    // awb亮度阈值及权重配置 | [AWB_LIGHT_CFG]
    AWB_SCENE_CFG stAwbSceneCfg;    // awb场景判断参数配置 | [AWB_SCENE_CFG]
    AWB_ADD_CT_CFG stAwbAddCtCfg;   // awb新增色温参数配置 | [AWB_ADD_CT_CFG]
    AWB_MANUAL_CFG stAwbManualCfg;  // awb手动增益配置 | [AWB_MANUAL_CFG]
    AWB_COMP_CFG stAwbCompCfg;      // awb补偿增益配置 | [AWB_COMP_CFG]
} AWB_DEFAULT_CFG;

typedef struct _ISP_AWB_STATUS_S_
{
    FH_UINT16 u16RGain;            // 红色增益（U4.6） | [0x0-0x3ff]
    FH_UINT16 u16GGain;            // 绿色增益（U4.6） | [0x0-0x3ff]
    FH_UINT16 u16BGain;            // 蓝色增益（U4.6） | [0x0-0x3ff]
    FH_UINT16 u16CurrBOverG;       // 当前B/G(水平坐标，U4.12) | [0x0-0xffff]
    FH_UINT16 u16CurrROverG;       // 当前R/G(垂直坐标，U4.12) | [0x0-0xffff]
    FH_UINT16 u16RAvg;             // 当前统计红色平均值（U10） | [0x0-0x3ff]
    FH_UINT16 u16GAvg;             // 当前统计绿色平均值（U10） | [0x0-0x3ff]
    FH_UINT16 u16BAvg;             // 当前统计蓝色平均值（U10） | [0x0-0x3ff]
    FH_UINT8 u08Pos;               // 当前色温位置   0：最靠近左侧统计点 255：最靠近右侧统计点 | [0x0-0xff]
    FH_UINT8 u08CurrLeft;          // 当前色温左侧统计点index | [0x0-0xf]
    FH_UINT8 u08CurrRight;         // 当前色温右侧统计点index | [0x0-0xf]
    FH_UINT8 u08UnknownProb;       // 当前为不确定场景的概率，单位为百分比 | [0x0-0x64]
    FH_UINT8 u08PureColorProb;     // 当前为纯色场景的概率，单位为百分比 | [0x0-0x64]
    FH_UINT8 u08OutdoorProb;       // 当前为室外场景的概率，单位为百分比 | [0x0-0x64]
    FH_UINT8 u08IndoorSingleProb;  // 当前为室内单色温场景的概率，单位为百分比 | [0x0-0x64]
    FH_UINT8 u08IndoorMultiProb;   // 当前为室内混合色温场景的概率，单位为百分比 | [0x0-0x64]
    FH_UINT8 u08CtNum[12];         // 各色温的blk数量 | [0x0-0xff]
    FH_UINT8 u08CtAddNum[4];       // 新增白点的blk数量 | [0x0-0xff]
} ISP_AWB_STATUS;

/**|CCM|**/
typedef struct _CCM_TABLE_S_
{
    FH_BOOL bCcmEn;// ccm控制使能 | [0x0-0x1]
    FH_SINT16 s16CcmTable[4][12];  // ccm矩阵表 | [-4096~0xfff]
} CCM_TABLE;

typedef struct _ISP_CCM_CFG_S_ {
    FH_SINT16 s16CcmTable[12];  // ccm矩阵表 | [-4096~0xfff]
} ISP_CCM_CFG;

typedef struct _ISP_ACR_CFG_S_
{
    FH_BOOL bAcrEn;// Acr控制使能 | [0x0-0x1]
    FH_UINT16 u16AcrGainL;// 色彩矫正弱化控制增益下限U.6 | [0x0-0xffff]
    FH_UINT16 u16AcrGainH;// 色彩矫正弱化控制增益上限U.6 | [0x0-0xffff]
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
    IPB_GAMMA_MODE u8GammaMode; // GAMMA模块输入选择 0: gamma off 1: YC mode 2: RGB mode 3: RGBY mode |[0-0x3]
    CURVE_TYPE        eCCurveType;       // c gamma控制模式:1 内置gamma表;2 用户自定义gamma表 | [1~2]
    GAMMA_BUILTIN_IDX eCGammaBuiltInIdx; // c gamma控制模式等于0时有效，内置cgamma表索引 | [0~0xf]
    FH_UINT16         u16CGamma[160];    // 用户自定义cgamma表 | [0~0xffff]
    CURVE_TYPE        eYCurveType;       // y gamma控制模式:1 内置gamma表;2 用户自定义gamma表 | [1~2]
    GAMMA_BUILTIN_IDX eYGammaBuiltInIdx; // y gamma控制模式等于0时有效，内置ygamma表索引 | [0~0xf]
    FH_UINT16         u16YGamma[160];    // 用户自定义ygamma表 | [0~0xffff]
} ISP_GAMMA_CFG;
/**|CFA|**/
typedef struct _ISP_CFA_CFG_S_ {
    FH_BOOL bCfaEn;// CFA控制使能 | [0~1]
    FH_UINT8 u08Str;// 去假色强度 | [0x0-0xff]
    FH_BOOL bSmoothMode;// SMOOTH 控制模式 0: 手动模式 1: gain mapping | [0x0-0x1]
    FH_BOOL bSmoothHwEn;// Smooth硬件开关使能 | [0x0-0x1]
    FH_UINT8 u08SmoothThh;// 平滑上阈值，值越小平滑越明显，smoothThh>smoothThl | [0x0-0xff]
    FH_UINT8 u08SmoothThl;// 平滑下阈值，值越小平滑越明显 | [0x0-0xff]
    FH_UINT8 u08SmoothThhMap[GAIN_NODES];// 6db平滑上阈值 | [0x0-0xff]
    FH_UINT8 u08SmoothThlMap[GAIN_NODES];// 0db平滑下阈值 | [0x0-0xff]
} ISP_CFA_CFG;

/**|BLC|**/
typedef struct _ISP_BLC_ATTR_S_ {
    FH_BOOL bBlcEn;// blc控制使能 | [0x0-0x1]
    FH_BOOL bMode;// 0: manual 1: gain mapping | [0x0-0x1]
    FH_BOOL bBlcDeltaModeEn;// 0:不分通道减dc 1: 分通道减dc | [0x0-0x1]
    FH_UINT16 u16BlcLevel;// 手动黑电平值 | [0x0-0xfff]
    FH_SINT8 s08BlcDeltaR;// 手动红通道dc偏移 | [-128~127]
    FH_SINT8 s08BlcDeltaG;// 手动绿通道dc偏移 | [-128~127]
    FH_SINT8 s08BlcDeltaB;// 手动蓝通道dc偏移 | [-128~127]
    FH_UINT16 u16BlcMapMap[GAIN_NODES];// 增益为0dB的BLC值 | [0x0-0xfff]
    FH_SINT8 s08BlcDeltaRMapMap[GAIN_NODES];// 增益为0dB的R通道BLC值偏移 | [-128~127]
    FH_SINT8 s08BlcDeltaGMapMap[GAIN_NODES];// 增益为0dB的G通道BLC值偏移 | [-128~127]
    FH_SINT8 s08BlcDeltaBMapMap[GAIN_NODES];// 增益为0dB的B通道BLC值偏移 | [-128~127]
} ISP_BLC_ATTR;
/**|GB|**/
typedef struct _ISP_GB_CFG_S_ {
    FH_BOOL bGbEn;// 绿平衡控制使能 | [0x0-0x1]
    FH_BOOL bMode;// GB控制模式 0: 手动模式 1: gain mapping | [0x0-0x1]
    FH_UINT8 u08SensiThl;// GB敏感度下阈值，值越大表示边界上的校正强度也越大U8，sensiThl<sensiThh | [0x0-0xff]
    FH_UINT8 u08SensiThh;// GB敏感度上阈值，值越大表示边界上的校正强度也越大U8 | [0x0-0xff]
    FH_UINT8 u08GbTh;// G通道差异阈值，值越小整体校正强度越大U8 | [0x0-0xff]
    FH_UINT8 u08GbDiffTh0;// GB阈值0 用于坏点和GB同时存在，坏点周围点G校正门限，越小校正力度越大U8 | [0x0-0xff]
    FH_UINT8 u08GbDiffTh1;// GB阈值1 用于坏点和GB同时存在，坏点周围点G校正门限，越小校正力度越大U8，一般gbDiffTh1>gbDiffTh0 | [0x0-0xff]
    FH_UINT8 u08SensiThhMap[GAIN_NODES];// 6db阈值上限U8 | [0x0-0xff]
    FH_UINT8 u08SensiThlMap[GAIN_NODES];// 0db阈值下限U8 | [0x0-0xff]
    FH_UINT8 u08GbThMap[GAIN_NODES];// 0db G通道差异阈值U8 | [0x0-0xff]
} ISP_GB_CFG;
/**|DPC|**/
typedef struct _ISP_DPC_CFG_S_ {
    FH_BOOL bDpcEn;  // 坏点消除控制使能 | [0x0-0x1]
    FH_BOOL bMode1;// 0: 手动模式 1: gain mapping | [0x0-0x1]
    FH_BOOL bCtrlMode;// 去坏点控制模式： 0：单个坏点校正 1：多个坏点校正 | [0x0-0x1]
    FH_UINT8 u08Enable;// 0：关闭去坏点 1：打开去白点功能 2：打开去黑点功能 3：同时打开去白点黑点功能 | [0x0-0x3]
    FH_BOOL bMedBlendEn;// 平滑使能 | [0x0-0x1]
    FH_BOOL bSupTwinkleEn;// 闪烁抑制使能 | [0x0-0x1]
    FH_UINT8 u08DpcWTh0;// 白点阈值0，用于单个及多个坏点模式，值越小，去坏点强度越大 | [0x0-0xff]
    FH_UINT8 u08DpcWTh1;// 白点阈值1，仅在多个坏点下生效，值越小，去坏点强度越大 | [0x0-0xff]
    FH_UINT8 u08DpcBTh0;// 黑点阈值0，用于单个及多个坏点模式，值越小，去坏点强度越大 | [0x0-0xff]
    FH_UINT8 u08DpcBTh1;// 黑点阈值1，用于单个及多个坏点模式，值越小，去坏点强度越大 | [0x0-0xff]
    FH_UINT8 u08SupTwinkleThl;// 闪烁抑制阈值下限，值越大，抑制强度越大，去坏点也越弱，supTwinkleThl<supTwinkleThh | [0x0-0xff]
    FH_UINT8 u08SupTwinkleThh;// 闪烁抑制阈值上限值越大，抑制强度越大，去坏点也越弱 | [0x0-0xff]
    FH_UINT8 u08MedBlendRatio;// 中值校正与去坏点校正融合比例，值越大，中值校正比例越大 | [0x0-0xff]
    FH_UINT16 u16MedBlendStr;// 中值校正强度，值越大，中值校正强度也越大 | [0x0-0xff]
    FH_UINT8 u08DpcWTh0Map[GAIN_NODES];// 0db白点阈值0 | [0x0-0xff]
    FH_UINT8 u08DpcWTh1Map[GAIN_NODES];// 0db白点阈值1 | [0x0-0xff]
    FH_UINT8 u08DpcBTh0Map[GAIN_NODES];// 0db黑点阈值0 | [0x0-0xff]
    FH_UINT8 u08DpcBTh1Map[GAIN_NODES];// 0db黑点阈值1 | [0x0-0xff]
    FH_UINT8 u08MedBlendRatioMap[GAIN_NODES];// 0db中值比例 | [0x0-0xff]
    FH_UINT8 u08MedBlendStrMap[GAIN_NODES];// 0db中值强度 | [0x0-0xff]
    FH_UINT8 u08CtrlModeMap[GAIN_NODES];// 控制模式（增益0dB时） | [0x0-0x1]
    FH_UINT8 u08SupTwinkleThhMap[GAIN_NODES];// 0db闪烁抑制阈值上限 | [0x0-0xff]
    FH_UINT8 u08SupTwinkleThlMap[GAIN_NODES];// 0db闪烁抑制阈值下限 | [0x0-0xff]
} ISP_DPC_CFG;
/**|LSC|**/
typedef struct _ISP_LSC_CFG_S_
{
    FH_BOOL   bLscEn;       // 镜头阴影矫正控制使能 | [0~1]
    FH_UINT32 u32Coff[299]; // 补偿系数 | [0~0xffffffff]
} ISP_LSC_CFG;
/**|NR3D|**/
typedef struct _ISP_NR3D_CFG_S_ {
    FH_BOOL bNr3dEn;// 3d降噪控制使能 | [0x0-0x1]
    FH_BOOL bGainMappingEn;// NR3D控制模式 0: 手动模式 1: gain mapping | [0x0-0x1]
    FH_BOOL bCoeffMappingEn;// NR3Dcoeff映射线以及处理方式控制模式　0: 手动模式 1: gain mapping | [0x0-0x1]
    FH_BOOL bBinWinSizeMappingEn;// NR3D值域窗控制模式 0: 手动模式 1: gain mapping | [0x0-0x1]
    FH_BOOL bRefDitherEn;// 参考帧扰动模式使能 | [0x0-0x1]
    FH_BOOL bPresetMap;// 0: 采用内置coeff映射表（由coeff_MAP_idx决定）    1：采用自定义coeff映射表 | [0x0-0x1]
    FH_UINT8 u08CoeffMapIdx;// coeff映射曲线: | [0x0-0x7]
    FH_BOOL bCoeffMode;// 0: 人工痕迹较1少，拖影概率较１高　1:反之
    FH_UINT8 u08RefCoeffRatio;// 参考帧coeff比例因子，值越小收敛越快 | [0x0-0x1f]
    FH_BOOL bWinMode;// 统计窗口模式选择： 0:窗口长宽一致 1:窗口根据幅面等比例缩放 | [0x0-0x1]
    FH_BOOL bWinRefresh;// 刷新窗口配置到硬件 | [0x0-0x1]
    FH_UINT16 u16StatThh;// 统计信息阈值 | [0x0-0x3ff]
    FH_UINT8 u08SpaceWinSize;// 统计窗口大小配置，值越大窗口越大，模式由winMode决定 | [0x0-0xff]
    FH_UINT8 u08BinWinSize;// 值域窗口大小配置，值越大窗口越大 | [0x0-0xff]
    FH_UINT8 u08Nr3dSlopeStr;// nr3d噪声斜率估计强度,U4.4 | [0x0-0xff]
    FH_UINT8 u08Nr3dOffsetStr;// nr3d噪声偏移估计强度,U4.4 | [0x0-0xff]
    FH_UINT8 u08MdCoeffTh;// 运动coeff门限值，小于此门限认为是静止，与coeff映射线配合使用 | [0x0-0xff]
    FH_UINT16 u16K1;// the slope of “slope K”,U10.4 | [0x0-0x3fff]
    FH_UINT16 u16K2;// the offset of “slope K”,U10.4 | [0x0-0x3fff]
    FH_SINT16 s16O1;// the slope of “offset O”,S16 | [-32768~32767]
    FH_SINT16 s16O2;// the offset of “offset O”,S16 | [-32768~32767]
    FH_UINT16 u16MdGain;// 运动强度增益 | [0x0-0xffff]
    FH_UINT8 u08MdFltCof;// 运动信息滤波比例，值越大越倾向于判断成运动。 | [0x0-0xff]
    FH_UINT8 u08RefCoeffTh;// 运动coeff门限值，小于此门限认为是静止，与coeff映射线配合使用 | [0x0-0xff]
    FH_UINT8 u08DitherGainThh;// 开启扰动模式增益阈值U8， 1表示1倍增益, ref_dither_en=1时生效 | [0x0-0xff]
    FH_UINT8 u08DitherGainThl;// 关闭扰动模式增益阈值U8，1表示1倍增益, ref_dither_en=1时生效 | [0x0-0xff]
    FH_UINT8 u08Nr3dSlopeStrMap[GAIN_NODES];//  不同增益时的噪声斜率估计强度，U4.4 | [0x0-0xff]
    FH_UINT8 u08Nr3dOffsetStrMap[GAIN_NODES];//  不同增益时的噪声偏移估计强度，U4.4 | [0x0-0xff]
    FH_UINT16 u16MdGainInttMap[INTT_NODES];//  不同曝光行数下的运动估计强度 | [0x0-0xffff]
    FH_UINT16 u16MdGainMap[GAIN_NODES];//  不同增益时的运动估计强度 | [0x0-0xffff]
    FH_UINT8 u08RefCoeffRatioMap[GAIN_NODES];//  不同增益时的coeff比例因子 | [0x0-0xff]
    FH_UINT8 u08MdFltCofMap[GAIN_NODES];//  不同增益时的运动信息滤波比例(描述增加下效果) | [0x0-0xff]
    FH_UINT8 u08RefCoeffThMap[GAIN_NODES];//  不同增益时的参考帧运动coeff最小值 | [0x0-0xff]
    FH_UINT8 u08BinWinSizeMap[GAIN_NODES];//  不同增益时的值域窗口大小| [0x0-0xff]
    FH_UINT8 u08CoeffModeMap[GAIN_NODES];//  不同增益时的coeff处理方式| [0x0-0x1]
    FH_UINT8 u08CoeffIdxMap[GAIN_NODES];//  不同增益时的coeff映射线| [0x0-0x7]
} ISP_NR3D_CFG;

/**|NR2D|**/

typedef struct _ISP_NR2D_CFG_S_ {
    FH_BOOL bNr2dEn;// 2D降噪控制使能 | [0x0-0x1]
    FH_BOOL bGainMappingEn;// NR2D 控制模式 0: 手动模式 1: gain mapping | [0x0-0x1]
    FH_BOOL bNr2dMtEn;// nr2d联动使能 | [0x0-0x1]
    FH_BOOL bBlendingValMode;// nr2d所占比例控制模式 0: 手动模式 1: gain mapping | [0x0-0x1]
    FH_UINT8 u08BlendingVal;// nr2d所占比例 | [0x0-0xff]
    FH_UINT8 u08RefCoeffRatio;// nr2d coeff比例因子，值越小收敛越快 | [0x0-0xff]
    FH_UINT8 u08Nr2dS;// 2D 降噪强度 U4.4 | [0x0-0xff]
    FH_UINT8 u08K1;// “slope K”斜率, U2.6  | [0x0-0xff]
    FH_UINT8 u08K2;// “slope K”偏移值, U8 | [0x0-0xff]
    FH_SINT16 s16O1;// “offset O”斜率,S6.10 | [-32768~32767]
    FH_SINT16 s16O2;// “offset O”偏移值,S16 | [-32768~32767]
    FH_UINT8 u08Nr2dSft;// 2d降噪移位,U4 | [0x0-0xf]
    FH_SINT8 s08CoeffOffset;// nr2d coeff偏移值 | [-128~127]
    FH_UINT8 u08Nr2dStrMap[GAIN_NODES];// NR2D等级（不同增益时） | [0x0-0xff]
    FH_SINT8 s08Nr2dCoeffOffsetInttMap[INTT_NODES];// 不同曝光行数下的nr2d coeff偏移值 | [-128~127]
    FH_SINT8 s08Nr2dCoeffOffsetMap[GAIN_NODES];// 不同增益时的nr2d coeff偏移值 | [-128~127]
    FH_UINT8 u08RefCoeffRatioInttMap[INTT_NODES];// 不同曝光行数下的coeff比例因子 | [0x0-0xff]
    FH_UINT8 u08RefCoeffRatioMap[GAIN_NODES];// 不同增益时的coeff比例因子 | [0x0-0xff]
    FH_UINT8 u08BlendingValMap[GAIN_NODES];// 不同增益时的nr2d比例因子 | [0x0-0xff]
} ISP_NR2D_CFG;


/**|HLR|**/
typedef struct _ISP_HLR_CFG_S_
{
    FH_BOOL  bHlrEn;   // 高光恢复控制使能 | [0-1]
    FH_SINT8 s08Level; // Hlr补偿因子 S1.7 hlr_val = hlr_val*(128+comp_level)>>7 |  [-128~127]
    FH_UINT16 u16SensorOutputMax;// Sensor最大输出，用于数据通路归一化 | [0x0-0xfff]
} ISP_HLR_CFG;
/**|IE|**/
typedef struct _ISP_CONTRAST_CFG_S_
{
    FH_BOOL  bYcEn;                 // 亮度控制使能 | [0~1]
    FH_BOOL  bGainMappingEn;        // 0：手动 1：增益映射 | [0-1]
    FH_UINT8 u08Crt;                // 手动模式下对比度 U2.6 | [0-0xff]
    FH_UINT8 u08Mid;                // 手动模式下对比度矫正的中心亮度,暂不支持 | [0-0xff]
    FH_UINT8 u08CrtMap[GAIN_NODES]; // 不同噪声强度下对比度值U2.6 | [0~0xff]
} ISP_CONTRAST_CFG;
typedef struct _ISP_BRIGHTNESS_CFG_S_
{
    FH_BOOL  bYcEn;          // 亮度控制使能 | [0~1]
    FH_BOOL  bGainMappingEn; // 0：手动 1：增益映射 | [0~1]
    FH_SINT8 s08Brt;         // 手动模式下亮度S8(-128到127) | [-128~127]
    FH_SINT8 s08BrtMap[GAIN_NODES]; // 不同噪声强度下亮度S8 | [-128~127]
} ISP_BRIGHTNESS_CFG;
/**|CE|**/
typedef struct _ISP_SAT_CFG_S_
{
    FH_BOOL bSatEn;                 //色彩增强调整控制使能 | [0x0-0x1]
    FH_BOOL  bGainMappingEn;        // 0：手动 1：增益映射 | [0~1]
    FH_SINT8 s08RollAngle;          // 色度旋转因子 | [-64,63]
    FH_UINT8 u08Sat;                //饱和度调整因子 U3.5 | [0-0xff]
    FH_UINT8 u08BlueSup;           //饱和度蓝色分量 U2.6 | [0-0xff]
    FH_UINT8 u08RedSup;            //饱和度红色分量 U2.6 | [0-0xff]
    FH_BOOL bPresetLut;            //预设亮度增益抑制LUT 0:采用内置lut映射表；1：采用自定义lut映射表|[0~1]
    FH_BOOL bGlutEn;               //亮度增益抑制使能：0: 关闭 1: 开启|[0~1]
    FH_UINT8 u08ThhSup;            //抑制阈值:值越小抑制作用越强|[0~0xff]
    FH_BOOL bThhMode;              //抑制阈值控制模式:0:手动; 1:映射|[0~1]
    FH_BOOL bThreshEn;             //阈值抑制使能：0: 关闭 1: 开启 |[0~1]
    FH_UINT8 u08SatParaR1;         //白色门限制，饱和度小于该值的会被置0 | [0-0xff]
    FH_UINT8 u08CeGainWeight;      //亮度增益抑制LUT融合权重 | [0-0xff]
    FH_UINT8 u08Gain[32][4];       //亮度增益抑制映射表：128项| [0-0xf]
    FH_UINT8 u08SatMap[GAIN_NODES]; // 不同噪声强度下亮度值U2.6 | [0~0xff]
    FH_UINT8 u08ThhMap[GAIN_NODES]; // 不同噪声强度下抑制阈值|[0~0xff]
    FH_UINT8 u08GainWeightMap[GAIN_NODES]; // 不同噪声强度下亮度增益抑制LUT融合权重|[0~0xff]
} ISP_SAT_CFG;
/**|APC|**/
typedef struct _ISP_APC_CFG_S_
{
    FH_BOOL bApcEn;//锐度控制使能 | [0x0-0x1]
    FH_BOOL bGainMappingEn;// apc control mode 0: manual 1: gain mapping | [0x0-0x1]
    FH_BOOL bLutMappingEn;// lut control mode 0: manual 1: gain mapping | [0x0-0x1]
    FH_BOOL bFilterMappingEn;// filter control mode 0: manual 1: gain mapping | [0x0-0x1]
    FH_BOOL bMergeSel;// Detail Enhancer和Edge Sharpener合并模式选择 0：相加 1：取绝对最大值 | [0x0-0x1]
    FH_BOOL bLutMode;// LUT 系数选择模式： 0：系数表选择模式 1：手动配置系数 | [0x0-0x1]
    FH_BOOL bFilterMode;// 滤波器选择模式: 0：系数表选择模式 1：手动配置系数 | [0x0-0x1]
    FH_UINT8 u08DetailLutNum;// 细节增强LUT系数选择 | [0x0-0x7]
    FH_UINT8 u08EdgeLutNum;// 边界锐化LUT系数选择 | [0x0-0x7]
    FH_UINT8 u08DetailFNum;// 细节滤波器选择 | [0x0-0x4]
    FH_UINT8 u08EdgeFNum;// 边界滤波器选择 | [0x0-0x3]
    FH_UINT8 u08DetailLutGroup;//细节增强LUT组别选择 | [0x0-0x1]
    FH_UINT8 u08EdgeLutGroup;//边界增强LUT组别选择 | [0x0-0x1]
    FH_UINT8 u08Pgain;// 总体APC 正向增益 | [0x0-0xff]
    FH_UINT8 u08Ngain;// 总体APC 负向增益 | [0x0-0xff]
    FH_UINT8 u08DetailLv;// 细节锐化等级 | [0x0-0xff]
    FH_UINT8 u08EdgeLv;// 边界锐化等级 | [0x0-0xff]
    FH_UINT16 u16DetailThl;// 细节增强下门限值 | [0x0-0x3ff]
    FH_UINT16 u16DetailThh;// 细节增强上门限值 | [0x0-0x3ff]
    FH_UINT16 u16EdgeThl;// 边界增强下门限值 | [0x0-0x3ff]
    FH_UINT16 u16EdgeThh;// 边界增强上门限值 | [0x0-0x3ff]
    FH_UINT8 u08EdgeMap[GAIN_NODES];// 不同增益时的边界锐化强度 | [0x0-0xff]
    FH_UINT8 u08DetailMap[GAIN_NODES];// 不同增益时的细节锐化强度 | [0x0-0xff]
    FH_UINT8 u08PgainMap[GAIN_NODES];// 不同增益时的白边界强度 | [0x0-0xff]
    FH_UINT8 u08NgainMap[GAIN_NODES];// 不同增益时的黑边界强度 | [0x0-0xff]
    FH_UINT8 u08DetailLutNumMap[GAIN_NODES];// LUT系数选择（不同增益时） | [0x0-0xf]
    FH_UINT8 u08EdgeLutNumMap[GAIN_NODES];// LUT系数选择（不同增益时） | [0x0-0xf]
    FH_UINT8 u08DetailFNumMap[GAIN_NODES];// filter系数选择（不同增益时） | [0x0-0xf]
    FH_UINT8 u08EdgeFNumMap[GAIN_NODES];// filter系数选择（不同增益时） | [0x0-0xf]
} ISP_APC_CFG;

typedef struct _ISP_APC_MT_CFG_S_ {
    FH_BOOL bApcMtEn;// apc联动使能： 0: Off 1: On | [0x0-0x1]
    FH_BOOL bApcMtMode;// apc联动模式： 0: manual 1: gain mapping | [0x0-0x1]
    FH_UINT8 u08MtStr;// 联动强度，越大运动区域越模糊 | [0x0-0xff]
    FH_SINT16 s16MtOffset;// 联动偏移，越大运动区域越模糊 | [-256~255]
    FH_UINT8 u08MtStrMap[GAIN_NODES];// 不同增益时的联动强度 | [0x0-0xff]
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
    FH_BOOL bYnrEn;// ynr控制使能 | [0x0-0x1]
    FH_BOOL bGainMappingEn;// 亮度去噪控制模式 0：手动模式 1：gain mapping | [0x0-0x1]
    FH_BOOL bCtrlMode;// 0：弱模式 1：强模式 | [0x0-0x1]
    FH_UINT8 u08YnrThSlope0;// 0号滤波器映射斜率 U4.4，斜率大于1变锐，斜率小于1变糊 | [0x0-0xff]
    FH_UINT8 u08YnrThSlope1;// 1号滤波器映射斜率 U4.4，斜率大于1变锐，斜率小于1变糊 | [0x0-0xff]
    FH_UINT8 u08YnrThSlope2;// 2号滤波器映射斜率 U4.4，斜率大于1变锐，斜率小于1变糊 | [0x0-0xff]
    FH_UINT8 u08MergeWeight;// 融合权重，值越大，原始输入权重越大 | [0x0-0xff]
    FH_UINT8 u08YnrThStr0;// ynr 0号滤波器降噪去噪等级U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThStr1;// ynr 1号滤波器降噪去噪等级U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThStr2;// Ynr 2号滤波器降噪去噪等级U4.4 | [0x0-0xff]
    FH_UINT8 u08Thl0[16];// 亮度阈值0~15（0号滤波器） | [0x0-0xff]
    FH_UINT8 u08Thl1[16];// 亮度阈值0~15（1号滤波器） | [0x0-0xff]
    FH_UINT8 u08Thl2[16];// 亮度阈值0~15（2号滤波器） | [0x0-0xff]
    FH_UINT8 u08YnrMergeWeightMap[GAIN_NODES];// 不同增益时的融合权重 | [0x0-0xff]
    FH_UINT8 u08YnrThSlope0Map[GAIN_NODES];// 不同增益时的映射斜率（高频分量），U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThSlope1Map[GAIN_NODES];// 不同增益时的映射斜率（中频分量），U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThSlope2Map[GAIN_NODES];// 不同增益时的映射斜率（低频分量），U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThStr0InttMap[INTT_NODES];// 不同曝光行数下的去噪强度（高频分量），U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThStr0Map[GAIN_NODES];// 不同增益时的去噪强度（高频分量），U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThStr1InttMap[INTT_NODES];// 不同曝光行数下的去噪强度（中频分量），U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThStr1Map[GAIN_NODES];// 不同增益时的去噪强度（中频分量），U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThStr2Map[GAIN_NODES];// 不同增益时的去噪强度（低频分量），U4.4 | [0x0-0xff]
} ISP_YNR_CFG;

typedef struct _ISP_YNR_MT_CFG_S_ {
    FH_BOOL bCoefEn;// YNR与NR3D联动使能 0：关 1：开 | [0x0-0x1]
    FH_BOOL bYnrMtMode;// Ynr联动控制模式 0：手动模式 1：gain mapping | [0x0-0x1]
    FH_UINT8 u08MtCoeff0;// YNR与NR3D联动去噪第一个coeff分界点，小运动 | [0x0-0xff]
    FH_UINT8 u08MtCoeff1;// YNR与NR3D联动去噪第二个coeff分界点，大运动 | [0x0-0xff]
    FH_UINT8 u08YnrMtStr0;// YNR与NR3D联动去噪第一级强度，小运动 | [0x0-0xff]
    FH_UINT8 u08YnrMtStr1;// YNR与NR3D联动去噪第二级强度，大运动 | [0x0-0xff]
    FH_UINT8 u08YnrMtStr0InttMap[INTT_NODES];// 不同曝光行数下的小运动去噪等级 | [0x0-0xff]
    FH_UINT8 u08YnrMtStr0Map[GAIN_NODES];// 不同增益时的小运动去噪等级 | [0x0-0xff]
    FH_UINT8 u08YnrMtStr1InttMap[INTT_NODES];// 不同曝光行数下的大运动去噪等级 | [0x0-0xff]
    FH_UINT8 u08YnrMtStr1Map[GAIN_NODES];// 不同增益时的大运动去噪等级 | [0x0-0xff]
} ISP_YNR_MT_CFG;

typedef struct _ISP_CNR_CFG_S_ {
    FH_BOOL  bCnrEn;           //色度降噪控制使能 | [0-0x1]
    FH_BOOL bGainMappingEn;// 色度降噪控制模式 0: 手动模式 1: gain mapping | [0x0-0x1]
    FH_UINT8 u08CnrStr;// 色度降噪强度 | [0x0-0xff]
    FH_UINT8 u08ColThl;// 颜色保护阈值下限。值越小，颜色保护越好，同时去噪效果越弱。 | [0x0-0xff]
    FH_UINT8 u08ColThh;// 颜色保护阈值上限。满足约束：colThH>colThL。值越小，颜色保护保护越好，同时去噪效果越弱。 | [0x0-0xff]
    FH_UINT8 u08StrMap[GAIN_NODES];// 6dB时，色度去噪等级 | [0x0-0xff]
    FH_UINT8 u08ColThlMap[GAIN_NODES];// 0dB时，颜色阈值下限 | [0x0-0xff]
    FH_UINT8 u08ColThhMap[GAIN_NODES];// 0dB时，颜色阈值上限 | [0x0-0xff]
} ISP_CNR_CFG;

typedef struct _ISP_CNR_MT_CFG_S_
{
    FH_BOOL bCnrMtEn;        //CNR联动使能0: 关;   1:开|[0-0x1]
    FH_BOOL bCnrMtStrMode;   //CNR联动强度模式 0: manual;   1:gain mapping|[0-0x1]
    FH_UINT8 u08CnrMtColStr; //cnr联动饱和度保护强度|[0~0xf]
    FH_UINT8 u08CnrMtStr;    //cnr联动强度，越大运动区域色度降噪越强| [0-0xff]
    FH_SINT16 s16CnrMtOffset;//cnr联动偏移，越大整体色度去噪越强|[-254 - 255]
    FH_UINT8 u08MtStrMap[GAIN_NODES]; //cnr联动强度等级| [0-0xff]
} ISP_CNR_MT_CFG;

/**|PURPLE|**/
typedef struct _ISP_PURPLEFRI_CFG_S_
{
    FH_BOOL bPurpleEn; //去紫边控制使能 | [0-0x1]
    FH_SINT8 s08Shiftbits; //校正力度调整单元，通过移位增大或减小饱和度（当为正数时表示左移，当为负数时表示右移），范围为[-8,3]。| [-8,3]
    FH_BOOL   bSatcorrenable; //饱和度校正使能 | [0~1]
    FH_UINT16 u16Rollangle;   //旋转因子，单位为度° | [0-0x1ff]
    FH_UINT16 u16Adjangle;    //调整区域因子，单位为度° | [0-0x1ff]
    FH_UINT16 u16Ygapth; // 7x7窗亮度分量最大值和最小值之差的最小阈值，YMax-YMin>=YGapTh | [0-0x3ff]
    FH_UINT16 u16Yth;    // 7x7窗亮度分量最大值的最小阈值，YMax >= YTh | [0-0x3ff]
    FH_UINT16 u16Expoth; //亮度分量的最大阈值，Y<ExpoTh | [0-0x3ff]
    FH_SINT16 s16Ulth;   //旋转后色度分量U’最小阈值，U’>ULth | [-512-511]
    FH_SINT16 s16Uhth;   //旋转后色度分量U’最大阈值，U’<UHth | [-512-511]
    FH_UINT16 u16Satthh; //饱和度最大阈值， Sat(YMax)< SatThH | [0-0x3ff]
    FH_UINT16 u16Satthl; //饱和度最小阈值，Sat> SatThL | [0-0x3ff]
} ISP_PURPLEFRI_CFG;

/**|DRC|**/
typedef struct _ISP_DRC_CFG_S_ {
    FH_BOOL bDrcEn; //DRC控制使能 | [0-0x1]
    FH_BOOL bGainMappingEn;// DRC控制模式 0: 手动模式 1: gain mapping | [0x0-0x1]
    FH_BOOL bSupPosEdgeEn;// 高亮白边校正使能 | [0x0-0x1]
    FH_BOOL bDrcCurveMode;// 0：采用内置drc表，由drc_curve_idx决定 1：采用自定义drc表 | [0x0-0x1]
    FH_UINT8 u08DrcCurveIdx;// drc曲线idx | [0x0-0x7]
    FH_BOOL bDrcHwEn;// DRC硬件模块开关 | [0-0x1]
    FH_BOOL bWinMode;// 统计窗口模式选择： 0:窗口长宽一致 1:窗口根据幅面等比例缩放 | [0x0-0x1]
    FH_BOOL bWinRefresh;// 刷新窗口配置到硬件 | [0x0-0x1]
    FH_BOOL bRefresh;// 设置1刷新drc表至寄存器 | [0x0-0x1]
    FH_UINT8 u08SpaceWinSize;// 统计窗口大小配置，值越大窗口越大，模式由winMode决定 | [0x0-0xff]
    FH_UINT16 u16BinWinSize;// 值域窗口大小配置，值越大窗口越大 | [0x0-0x1ff]
    FH_UINT16 u16FilterWgtMin;// 滤波输出权重最小值，值越大越倾向于使用滤波输出结果，通透性好，filterWgtMin<=filterWgtMax | [0x0-0x1fff]
    FH_UINT16 u16FilterWgtMax;// 滤波输出权重最大值 | [0x0-0x1fff]
    FH_UINT8 u08FilterWgtSlope;// 滤波输出权重斜率 | [0x0-0xff]
    FH_UINT8 u08SupPosEdgeStr;// 高亮白边校正强度，值越大校正越强，也即高亮白边越弱 | [0x0-0xff]
    FH_UINT16 u16LumaDiffTh;// 控制滤波后亮度差，调试确认效果后加描述 | [0x0-0x1fff]
    FH_UINT8 u08LcCtr;// 局部对比度调整 | [0x0-0x7f]
    FH_UINT8 u08DrcStr;// DRC强度 | [0x0-0xff]
    FH_UINT8 u08Bright;// 亮度调整参数，值越大，亮度越亮 | [0x0-0xff]
    FH_UINT8 u08DePosDarkGain;// 正向细节暗区增益 | [0x0-0xff]
    FH_UINT8 u08DePosBrightGain;// 正向细节亮区增益 | [0x0-0xff]
    FH_UINT8 u08DeNegDarkGain;// 负向细节暗区增益 | [0x0-0xff]
    FH_UINT8 u08DeNegBrightGain;// 负向细节亮区增益 | [0x0-0xff]
    FH_UINT8 u08DePosMax;// 控制亮细节最大值 | [0x0-0xff]
    FH_UINT8 u08DePosMin;// 控制亮细节最小值 | [0x0-0xff]
    FH_UINT8 u08DePosTh;// 控制亮细节阈值 | [0x0-0xff]
    FH_UINT8 u08DePosSlope;// 控制亮细节斜率 | [0x0-0x7]
    FH_UINT8 u08DeNegMax;// 控制暗细节最大值 | [0x0-0xff]
    FH_UINT8 u08DeNegMin;// 控制暗细节最小值 | [0x0-0xff]
    FH_UINT8 u08DeNegTh;// 控制暗细节阈值 | [0x0-0xff]
    FH_UINT8 u08DeNegSlope;// 控制暗细节斜率 | [0x0-0x7]
    FH_UINT16 u16SupBrightTh;// 亮区抑制门限，值越大抑制越强 | [0x0-0xfff]
    FH_UINT8 u08SupBrightSlope;// 亮区抑制斜率，值越大抑制越强 | [0x0-0xff]
    FH_UINT8 u08DarkSat;// 暗区饱和度，值越大，饱和度抑制越强 | [0x0-0xff]
    FH_UINT8 u08BrightSat;// 亮区饱和度，值越大，饱和度抑制越强 | [0x0-0xff]
    FH_UINT8 u08LcCtrMap[GAIN_NODES];// 6db局部对比度 | [0x0-0xff]
    FH_UINT8 u08BrightMap[GAIN_NODES];// 0db drc亮度 | [0x0-0xff]
    FH_UINT8 u08DrcStrMap[GAIN_NODES];// 0db drc亮度强度 | [0x0-0xff]
    FH_UINT8 u08DePosDarkGainMap[GAIN_NODES];// 0db正向细节暗区增益 | [0x0-0xff]
    FH_UINT8 u08DePosBrightGainMap[GAIN_NODES];// 0db正向细节亮区增益 | [0x0-0xff]
    FH_UINT8 u08DeNegDarkGainMap[GAIN_NODES];// 0db负向细节暗区增益 | [0x0-0xff]
    FH_UINT8 u08DeNegBrightMap[GAIN_NODES];// 0db负向细节亮区增益 | [0x0-0xff]
    FH_UINT8 u08DarkSatMap[GAIN_NODES];// 0db暗区饱和度 | [0x0-0xff]
    FH_UINT8 u08BrightSatMap[GAIN_NODES];// 0db亮区饱和度 | [0x0-0xff]
    FH_UINT32 u32Data[129];// drc曲线系数 | [0x0-0xffffffff]
} ISP_DRC_CFG;

/**|SMART_IR|**/
typedef struct _SMART_IR_CFG_S
{
    FH_UINT16 u16gainDay2Night; // 白天切换到夜晚的增益門限，bayer sensor | [0~0xffff]
    FH_UINT16 u16gainNight2Day; // 夜晚切换到白天的增益門限，bayer sensor | [0~0xffff]
    FH_UINT16 u16lumaDay2Night; // 白天切换到夜晚的增益門限，rgbir sensor | [0~0xfff]
    FH_UINT16 u16lumaNight2Day; // 夜晚切换到白天的增益門限，rgbir sensor | [0~0xfff]
} SMART_IR_CFG;



/**|STATISTICS|**/
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

typedef struct _AWB_BLOCK_STAT_S_
{
    FH_UINT32 u32AwbBlockR;    // AWB R分量sum统计值 | [0~0xffffffff]
    FH_UINT32 u32AwbBlockG;    // AWB G分量sum统计值 | [0~0xffffffff]
    FH_UINT32 u32AwbBlockB;    // AWB B分量sum统计值 | [0~0xffffffff]
    FH_UINT32 u32AwbBlockCnt;  // AWB 统计值cnt值 | [0~0xffffffff]
} AWB_BLOCK_STAT;

typedef struct _ISP_AWB_STAT1_S_
{
    AWB_BLOCK_STAT stBlkStat[9];  // AWB 3*3统计 | [AWB_BLOCK_STAT]
} ISP_AWB_STAT;

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

typedef struct
{
    FH_UINT32        u32IspDevId;        //通道号 | [0~2]
    ISP_AWB_ADV_STAT astAwbAdvStat[256]; //256是默认窗口配置,若改动的话通过API_ISP_GetAwbAdvStatCfg获取当前统计窗个数 | [ISP_AWB_ADV_STAT]
} GRP_CH_AWB_ADV_STAT;
typedef struct
{
    FH_UINT32            u32GrpChanNum;       //拼接组的通道数,本项目为2 | [2]
    GRP_CH_AWB_ADV_STAT  astGrpChanStat[2];   //所有拼接通路的统计 | [GRP_CH_AWB_ADV_STAT]
    FH_UINT8             u8AwbId;             // 用来选择当前使用的是awb0还是awb1(FH8866适用) | [0~1]
} GROUP_AWB_ADV_STAT;

typedef struct _ISP_AWB_ADV_STAT_CFG_S_
{
    ISP_STAT_WIN1 stStatWin;      // 统计窗口配置 | [ISP_STAT_WIN1]
    FH_UINT16 u16YHighThreshold;  // awb统计上門限值 | [0~0xfff]
    FH_UINT16 u16YLowThreshold;   // awb统计低門限值 | [0~0xfff]
    STAT_WHITE_POINT stPoint[6];  // awb白框点，A，B，C，D，E，F六个点，Ax=Bx By=Cy Dy=Ey Ex=Fx | [STAT_WHITE_POINT]
} ISP_AWB_ADV_STAT_CFG;

/**|MD|**/
typedef struct _ISP_MD_STAT_S
{
    FH_UINT32         u32MdEn;    //模块使能状态，状态为1时获取到的值为有效 | [0~1]
    FH_UINT32         u32MdMove;  //模块运动状态统计 | [0~0x180]
    FH_UINT32         u32MdStill; //模块静止状态统计 | [0~0x180]
} ISP_MD_STAT;

/**|GLOBAL_STAT|**/
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

typedef struct
{
    FH_UINT32  u32IspDevId;       //通道号 | [0~2]
    GLOBE_STAT astGlobeStat[256]; // 256是默认窗口配置,若改动的话通过API_ISP_GetGlobeStatCfg获取当前统计窗个数 | [GLOBE_STAT]
} GRP_CH_GL_STAT;
typedef struct
{
    FH_UINT32      u32GrpChanNum;      //拼接组的通道数,本项目为2 | [2]
    GRP_CH_GL_STAT astGrpChanStat[2];  //所有拼接通路的统计 | [GRP_CH_GL_STAT]
} GROUP_GLOBE_STAT;

typedef struct _GLOBE_STAT_CFG_S
{
    ISP_STAT_WIN1 stStatWin;  // 统计窗口配置,对于global统计,winHSize和winVSize需小于512,窗口个数较小时需注意 | [ISP_STAT_WIN1]
    FH_UINT16 u16ChnThH;      // 统计像素上阈值,默认0xfff | [0x0~0x3ff]
    FH_UINT16 u16ChnThL;      // 统计像素下阈值,默认0 | [0x0~0x3ff]
} GLOBE_STAT_CFG;
/**|FAST_BOOT|**/
/******************************************************************/
/******************isp init cfg structure**************************/
/******************************************************************/
/******************************************************************/
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
    FH_SINT16 ccmCfg[9];  // 颜色矫正矩阵 | [-4096~4095]
} CCM_INIT_CFG;

typedef struct _DPC_INIT_CFG_S
{
    FH_BOOL bCtrlMode;  // 去坏点控制模式： 0：单个坏点校正 1：多个坏点校正 | [0x0-0x1]
    FH_BOOL bSmoothEn;// 平滑使能 | [0x0-0x1]
    FH_UINT16 u16MedStr;// 中值校正强度，值越大，中值校正强度也越大 | [0x0-0xff]
    FH_UINT8 u08Enable;// 0：关闭去坏点 1：打开去白点功能 2：打开去黑点功能 3：同时打开去白点黑点功能 | [0x0-0x3]
    FH_UINT8 u08WTh0;// 白点阈值0，用于单个及多个坏点模式，值越小，去坏点强度越大 | [0x0-0xff]
    FH_UINT8 u08WTh1;// 白点阈值1，仅在多个坏点下生效，值越小，去坏点强度越大 | [0x0-0xff]
    FH_UINT8 u08BTh0;// 黑点阈值0，用于单个及多个坏点模式，值越小，去坏点强度越大 | [0x0-0xff]
    FH_UINT8 u08BTh1;// 黑点阈值1，用于单个及多个坏点模式，值越小，去坏点强度越大 | [0x0-0xff]
} DPC_INIT_CFG;

typedef struct _APC_INIT_CFG_S
{
    FH_UINT8 u08DetailLutNum;// 细节增强LUT系数选择 | [0x0-0x7]
    FH_UINT8 u08EdgeLutNum;// 边界锐化LUT系数选择 | [0x0-0x7]
    FH_UINT8 u08DetailFNum;// 细节滤波器选择 | [0x0-0x4]
    FH_UINT8 u08EdgeFNum;// 边界滤波器选择 | [0x0-0x3]
    FH_UINT8 u08DetailLutGroup;//细节增强LUT组别选择 | [0x0-0x1]
    FH_UINT8 u08EdgeLutGroup;//边界增强LUT组别选择 | [0x0-0x2]
    FH_UINT8 u08Pgain;// 总体APC 正向增益 | [0x0-0xff]
    FH_UINT8 u08Ngain;// 总体APC 负向增益 | [0x0-0xff]
    FH_UINT8 u08DetailLv;// 细节锐化等级 | [0x0-0xff]
    FH_UINT8 u08EdgeLv;// 边界锐化等级 | [0x0-0xff]
    FH_UINT16 u16DetailThl;// 细节增强下门限值 | [0x0-0x3ff]
    FH_UINT16 u16DetailThh;// 细节增强上门限值 | [0x0-0x3ff]
    FH_UINT16 u16EdgeThl;// 边界增强下门限值 | [0x0-0x3ff]
    FH_UINT16 u16EdgeThh;// 边界增强上门限值 | [0x0-0x3ff]
} APC_INIT_CFG;

typedef struct _YNR_INIT_CFG_S
{
    FH_UINT8 u08YnrThSlope0;// 0号滤波器映射斜率 U4.4，斜率大于1变锐，斜率小于1变糊 | [0x0-0xff]
    FH_UINT8 u08YnrThSlope1;// 1号滤波器映射斜率 U4.4，斜率大于1变锐，斜率小于1变糊 | [0x0-0xff]
    FH_UINT8 u08YnrThSlope2;// 2号滤波器映射斜率 U4.4，斜率大于1变锐，斜率小于1变糊 | [0x0-0xff]
    FH_UINT8 u08MergeWeight;// 融合权重，值越大，原始输入权重越大 | [0x0-0xff]
    FH_UINT8 u08YnrThStr0;// ynr 0号滤波器降噪去噪等级U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThStr1;// ynr 1号滤波器降噪去噪等级U4.4 | [0x0-0xff]
    FH_UINT8 u08YnrThStr2;// Ynr 2号滤波器降噪去噪等级U4.4 | [0x0-0xff]
    FH_UINT8 u08Thl0[16];// 亮度阈值0~15（0号滤波器） | [0x0-0xff]
    FH_UINT8 u08Thl1[16];// 亮度阈值0~15（1号滤波器） | [0x0-0xff]
    FH_UINT8 u08Thl2[16];// 亮度阈值0~15（2号滤波器） | [0x0-0xff]
    FH_UINT8 u08MtCoeff0;// YNR与NR3D联动去噪第一个coeff分界点，小运动 | [0x0-0xff]
    FH_UINT8 u08MtCoeff1;// YNR与NR3D联动去噪第二个coeff分界点，大运动 | [0x0-0xff]
    FH_UINT8 u08YnrMtStr0;// YNR与NR3D联动去噪第一级强度，小运动 | [0x0-0xff]
    FH_UINT8 u08YnrMtStr1;// YNR与NR3D联动去噪第二级强度，大运动 | [0x0-0xff]
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
    FH_UINT8 u08ThhSup;  //抑制阈值:值越小抑制作用越强|[0~0xff]
} SAT_INIT_CFG;

typedef struct _GAMMA_MODE_INIT_CFG_S
{
    FH_UINT16 gamma_mode;  // 当前gamma运行模式 | [0~3]
} GAMMA_MODE_INIT_CFG;

typedef struct _GAMMA_INIT_CFG_S
{
    FH_UINT32 gamma[80];  // gamma表对应的值 | [0~0xffffffff]
} GAMMA_INIT_CFG;

typedef struct _ISP_GAIN_INIT_CFG_S
{
    FH_UINT32 u32IspGain; // isp增益值 [0~0xffffffff]
} ISP_GAIN_INIT_CFG;

/**|SENSOR_CONTROL|**/
/******************************************************************/
/******************end of isp init cfg structure*******************/
/******************************************************************/
/******************************************************************/

/******************isp sensor common_if interface******************/
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

typedef struct _NEW_SNS_ADDR_S_
{
    FH_UINT32 u32newI2cAddr; //新配置的i2c地址 | [0~0xffffffff]
    FH_UINT32 u32SnsId; // 设备号 | [0xffffffff]
} NEW_SNS_ADDR_S;

typedef union
{
    BLC_PARAM_CFG blc_param_cfg;
    GPIO_PARAM_CFG gpio_param_cfg;
    MAP_INTT_GAIN_CFG mapInttGainCfg;
    NEW_SNS_ADDR_S new_sns_addr;
    FH_UINT32 u32Rate;
    unsigned int dw[140];
} ISP_SENSOR_COMMON_CMD_DATA0;

typedef union
{
    unsigned int dw[128]; //  | [ ]
} ISP_SENSOR_COMMON_CMD_DATA1;
/*******************************************************************/

/**|OTHERS|**/
typedef enum _INTERRUPT_CFG_CMD_S_ {
    WAIT_SLOT_LINE_ENDP_CMD = 0x1,
    WAIT_PIC_START_CMD = 0x02,
    WAIT_PIC_END_CMD = 0x03,
    INTERRUPT_CFG_CMD_DUMMY = 0xffffffff,
} INTERRUPT_CFG_CMD;

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
    FH_BOOL   bStaticDccEn;
    FH_BOOL   bStaticDpcEn;
    FH_UINT32 u32DpcTable[1024];
    FH_UINT16 u16DpcCol[32];
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
    ISP_BLC_ATTR       stBlcCfg;     //　 | [ ]
    ISP_GAMMA_CFG      stGamma;      //  | [ ]
    ISP_SAT_CFG        stSaturation; //  | [ ]
    ISP_APC_CFG        stApc;        //  | [ ]
    ISP_CONTRAST_CFG   stContrast;   //  | [ ]
    ISP_BRIGHTNESS_CFG stBrt;        //  | [ ]
    ISP_NR3D_CFG       stNr3d;       //  | [ ]
    ISP_NR2D_CFG       stNr2d;       //  | [ ]
    ISP_YNR_CFG        stYnr;        //  | [ ]
    ISP_CNR_CFG        stCnr;        //  | [ ]
    ISP_LSC_CFG        stLscCfg;     //  | [ ]
} ISP_DEFAULT_PARAM;

typedef struct _ISP_AE_STAT_CFG_S_
{
    ISP_STAT_WIN stStatWin;  // 统计窗口配置 | [ISP_STAT_WIN]
} ISP_AE_STAT_CFG;

typedef struct _ISP_AE_STAT_S_
{
    FH_UINT32 u32SumLuma[9];  // ae统计sum值 | [0~0xffffffff]
    FH_UINT32 u32Cnt[9];      // ae统计cnt值 | [0~0xffffffff]
} ISP_AE_STAT;

typedef struct _ISP_AWB_FRONT_STAT_S_
{
    AWB_BLOCK_STAT stBlkStat[64];  // AWB 8*8统计,位于短帧上 | [AWB_BLOCK_STAT]
} ISP_AWB_FRONT_STAT;
#pragma pack()
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*_ISP_COMMON_H_*/
