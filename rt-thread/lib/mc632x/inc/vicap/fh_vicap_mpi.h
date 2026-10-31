#ifndef __FH_VICAP_MPI_H__
#define __FH_VICAP_MPI_H__

#include "types/type_def.h"

typedef FH_UINT32 VI_DEV;
#define VICAP_MAX_SENSOR 2
typedef FH_SINT32 (*VicapIntCallback)(FH_SINT32);
#pragma pack(4)

/**|VICAP|**/
typedef enum
{
    VICAP_SNS0 = 0, // sensor0的序号
    VICAP_SNS1 = 1, // sensor1的序号
    VICAP_SNS2 = 2, // sensor2的序号
}FH_VICAP_SNS_IDX_E;

typedef enum MIPI_PHY_RESET_MODE
{
    FH_MIPI_PHY_SUSPEND = 0,
    FH_MIPI_PHY_RESUME = 1,
}FH_MIPI_PHY_RESET_E;

enum __VICAP_IDX__
{
    VICAP_PIPELINE_MIPI0 = 0,   // MIPI0序号，本项目对应VICAP_SNS0
    VICAP_PIPELINE_MIPI1 = 1,   // MIPI1序号，本项目对应VICAP_SNS1
    VICAP_PIPELINE_DVP = 2,     // DVP序号，本项目对应VICAP_SNS2
    VICAP_PIPELINE_TESTMODE = 3,    // 在线模式VICAP测试模式，离线模式下不支持
};

typedef enum __VICAP_STITCH_MODE__
{
    VICAP_STITCH_FREE_RUN     = 0,     // sns非同步模式时的拼接
    VICAP_STITCH_SYNC_MODE     = 1,     // sns同步模式时的拼接
    VICAP_STITCH_LARGE_PIC_MODE = 2,     // 硬件大图处理模式
    VICAP_STITCH_BUTT,
}FH_VICAP_STITCH_MODE_E;

typedef enum __VICAP_WORK_MODE__
{
    VICAP_WORK_MODE_ONLINE             = 0,     // vicap工作在在线模式
    VICAP_WORK_MODE_OFFLINE            = 1,     // vicap工作在离线模式
    VICAP_WORK_MODE_SWITCH             = 2,     // vicap工作在switch模式
    VICAP_WORK_MODE_SWITCH_OFFLINE     = 3,     // vicap工作在switch_off模式
    VICAP_WORK_MODE_BUTT,
}FH_VICAP_WORK_MODE_E;

typedef enum __VICAP_WDR_CHAN__
{
    VICAP_SFRAME = 0,   // 短帧通道号
    VICAP_LFRAME = 1,   // 长帧通道号
}FH_VICAP_WDR_CHAN_E;


typedef enum __VICAP_BAYER_TYPE__ {
    VICAP_BAYER_RGGB = 0x0, // 色彩模式RGGB
    VICAP_BAYER_GRBG = 0x1, // 色彩模式GRBG
    VICAP_BAYER_BGGR = 0x2, // 色彩模式BGGR
    VICAP_BAYER_GBRG = 0x3, // 色彩模式GBRG
    VICAP_BAYER_TYPE_DUMMY=0xffffffff,
} FH_VICAP_BAYER_TYPE_E;

typedef enum __VICAP_SYNC_TYPE__ {
    FH_VICAP_HSYNC = 0,  // 水平同步信号
    FH_VICAP_VSYNC = 1,  // 垂直同步信号
} FH_VICAP_SYNC_TYPE_E;

typedef enum __VICAP_INPUT_DATA_TYPE {
    VICAP_DATA_IN_RAW12 = 0,      // 输入的数据格式为RAW12
    VICAP_DATA_IN_RAW16 = 1,      // 输入的数据格式为RAW16
    VICAP_DATA_IN_YUV16 = 2,      // 输入的数据格式为YUV16
    VICAP_DATA_IN_BUTT,
} FH_VICAP_INPUT_DATA_TYPE_E;

typedef enum _VICAP_DVP_IN_BIT_MODE_ {
    FH_VICAP_DVP_BIT_MODE_16BIT_YUV = 0,    // DVP输入格式为YUV16
    FH_VICAP_DVP_BIT_MODE_12BIT_RAW = 1,    // DVP输入格式为RAW12
    FH_VICAP_DVP_BIT_MODE_10BIT_RAW = 2,    // DVP输入格式为RAW10
}FH_VICAP_DVP_IN_BIT_MODE_E;

typedef enum _VICAP_DVP_SYNC_POL_
{
    FH_VICAP_DVP_SYNC_POL_HIGH = 0,     // 同步信号高有效
    FH_VICAP_DVP_SYNC_POL_LOW  = 1,     // 同步信号低有效
}FH_VICAP_SYNC_POL_E;

typedef enum __CALLBACK_POS__
{
    FH_VICAP_POS_PIC_START = 0, //callback函数位于pic start
    FH_VICAP_POS_PIC_END   = 1, //callback函数位于pic end
}FH_VICAP_CALLBACK_POS_E;

typedef enum __MIPI_SNS_MOUNT_E__
{
    SENSOR_ON_MIPI0 = 0, // sensor挂载于mipi0
    SENSOR_ON_MIPI1 = 1, // sensor挂载于mipi1
    SENSOR_ON_MIPI2,     // 后期预留
    SENSOR_ON_MIPI3,     // 后期预留
}FH_VICAP_SNS_MOUNT_E;

typedef struct
{
    FH_UINT16    u16Width;   // 图像的宽度 | [0~0xffff]
    FH_UINT16    u16Height;  // 图像的高度 | [0~0xffff]
}FH_VICAP_SIZE_S;

typedef struct
{
    FH_UINT16    u16X;       // 图像裁剪的水平偏移值 | [0~0xffff]
    FH_UINT16    u16Y;       // 图像裁剪的垂直偏移值 | [0~0xffff]
    FH_UINT16    u16Width;   // 图像裁剪后的宽度 | [0~0xffff]
    FH_UINT16    u16Height;  // 图像裁剪后的高度 | [0~0xffff]
}FH_VICAP_RECT_S;

typedef struct
{
    FH_BOOL            bCutEnable;  // 裁剪的使能寄存器 | [0~1]
    FH_VICAP_RECT_S    stRect;      // 裁剪的偏移及裁剪后的宽高 | [FH_VICAP_RECT_S]
}FH_VICAP_CROP_CFG_S;

typedef struct
{
    FH_UINT8 u8Priority;    // 离线遍历优先级 | [0~0xf]
}FH_VICAP_OFFLINE_CFG_S;

typedef struct
{
    FH_VICAP_BAYER_TYPE_E     enBayerType;   // bayer pattern | [FH_VICAP_BAYER_TYPE_E]
    FH_VICAP_WORK_MODE_E      enWorkMode;    // 配置当前vicap的工作模式 | [FH_VICAP_WORK_MODE_E]
    FH_UINT8                  u8WdrMode;     // 输入sensor的线性还是wdr配置 | [0~1]
    FH_UINT8                  bitwidth;      // 输入的pixel宽度 | [FH_UINT8]
    FH_VICAP_SIZE_S           stInSize;      // 输入幅面 | [FH_VICAP_SIZE_S]
    FH_VICAP_CROP_CFG_S       stCropSize;    // 裁剪后幅面 | [FH_VICAP_CROP_CFG_S]
    FH_VICAP_OFFLINE_CFG_S    stOfflineCfg;  // 离线下的配置 | [FH_VICAP_OFFLINE_CFG_S]
}FH_VICAP_VI_ATTR_S;

typedef struct
{
    FH_BOOL                     bUsingVb;       // 是否使用VB缓存逻辑 | [0~1]
    FH_VICAP_INPUT_DATA_TYPE_E  enDataTypeIn;   // SENSOR输入该模块的数据格式，本芯片仅DVP有效 |  [FH_VICAP_INPUT_DATA_TYPE_E]
    FH_VICAP_WORK_MODE_E        enWorkMode;     // 配置当前vicap的工作模式，影响内存分配 | [FH_VICAP_WORK_MODE_E]
    FH_VICAP_SIZE_S             stSize;         // 图像幅面, 会影响内存分配, 故需按应用需要的最大幅面配置 | [FH_VICAP_SIZE_S]
}FH_VICAP_DEV_ATTR_S;

typedef struct
{
    FH_VICAP_DVP_IN_BIT_MODE_E    enBitMode;        // 配置当前输出模式是raw还是yuv等 | [FH_VICAP_DVP_IN_BIT_MODE_E]
    FH_VICAP_SYNC_POL_E           enHsyncPolarity;  // DVP水平同步信号极性 | [FH_VICAP_SYNC_POL_E]
    FH_VICAP_SYNC_POL_E           enVsyncPolarity;  // DVP垂直同步信号极性 | [FH_VICAP_SYNC_POL_E]
    FH_UINT8                      u8IoRemap[16];    // 对应的16个管脚的映射关系 | [0~0xf]
}FH_VICAP_DVP_CFG_S;

typedef struct
{
    FH_VICAP_SNS_MOUNT_E          enSnsMount;        // sensor挂载的mipi设备 | [FH_VICAP_DVP_IN_BIT_MODE_E]
    FH_UINT16                     u16Height;         // sensor的输入幅面高 | [0~0xffff]
}FH_VICAP_SNS_MOUNT_CFG_S;

typedef struct
{
    FH_UINT8    u8GroupId;     // 分组拼接的序号 | [0~0xf]
    FH_UINT32   u32DevId;      // 该路的通道号 | [0~3]
    FH_UINT8    u8StitchMode;  // 该路的拼接模式 | [0~2]
    FH_BOOL     bMainDev;      // 该路是否为mainPipe | [0~1]
    FH_BOOL     bStitchEn;     // 该路是否开启拼接时能 | [0~1]
}FH_VICAP_STITCH_ATTR_S;


typedef struct
{
    FH_UINT8 u8DevNum;         // 总共的vicap通路 | [0~VICAP_MAX_SENSOR]
    struct _buf_size_cfg
    {
        FH_UINT8            u8DevId;    // 设备号 | [0~VICAP_MAX_SENSOR]
        FH_VICAP_DEV_ATTR_S stDevAttr;  // 设备参数 | [FH_VICAP_DEV_ATTR_S]
        FH_UINT32           u32BufNum;  // 获取到的缓存块数 | [0~0xffffffff]
        FH_UINT32           u32BufSize;  // 获取到的缓存总大小 | [0~0xffffffff]
    } astBufCfg[VICAP_MAX_SENSOR];
}FH_VICAP_BUF_SIZE_ATTR_S;

typedef struct
{
    FH_VICAP_SYNC_TYPE_E enSyncType;    // 选择配置hsync还是vsync
    FH_UINT16   u16Period;              // 触发信号的一个周期的cycle数 | [0~0x1fff]
    FH_UINT16   u16Duty;                // 触发信号的有效电平cycle数   | [0~0x1fff]
    FH_BOOL     bPolatiry;              // 有效电平的极性 | [0~1]
}FH_VICAP_FRAME_SEQ_CFG_S;

typedef struct
{
    FH_UINT32  addrPhy;     // 数据的物理地址 | [FH_PHYADDR]
    FH_UINT32  u32VirAddr;  // 应用层的虚拟地址，同一个进程空间可直接使用 | [FH_UINT32]
    FH_UINT32  u32Crc;      // 数据校验值   | [0~0xffffffff]
    FH_UINT64  u64TimeStamp;// 数据获取的时间戳 | [0~0xffffffffffffffff]
    FH_UINT32  u32BufSize;  // 获取到的离线数据的缓存大小 | [0~0xffffffff]
    FH_UINT32  u32LineStride;   // 一行的数据量，单位是字节 | [0~0xffffffff]
}FH_VICAP_STREAM_COM_S;

typedef struct
{
    FH_UINT8   u8DevId;     // 设备号 | [0~2]
    FH_BOOL    bLock;       // 是否加锁 | [0~1]
    FH_BOOL    bBlock;      // 是否使用阻塞，阻塞支持线程调度 | [0~1]
    FH_UINT32  u32TimeOut;  // 超时时间，阻塞和非阻塞都可以配置超时时间，单位是ms | [0~0xffffffff]
    FH_UINT32  u32PollId;   // 释放FIFO所需要的Id号 | [0~0xffffffff]
    FH_UINT64  u64FrmCnt;   // 帧号 | [0~0xffffffff]
    FH_VICAP_SIZE_S stPic;      // 图像幅面相关的配置 | [FH_VICAP_SIZE_S]
    FH_VICAP_STREAM_COM_S stSf;  // 短帧的数据流 | [FH_VICAP_STREAM_COM_S]
    FH_VICAP_STREAM_COM_S stLf;  // 长帧的数据流 | [FH_VICAP_STREAM_COM_S]
}FH_VICAP_STREAM_S;

typedef struct
{
    FH_BOOL     bEnable;        // 当前通道是否使能 | [0~1]
    FH_UINT32   u32SofCnt;      // 帧开始信号计数 | [0~0xffffffff]
    FH_UINT32   u32EofCnt;      // 帧结束信号计数 | [0~0xffffffff]
    FH_UINT32   u32WrDoneCnt;   // 帧写完计数 | [0~0xffffffff]
    FH_UINT16   u16FrameRate;   // sensor输出帧率 | [0~0xffff]
}FH_VICAP_STATUS_S;

typedef struct MIPI_ERR_INFO
{
    FH_UINT32 SotErr; // sot错误统计 | [0-0xffffffff]
    FH_UINT32 CrcErr; // crc错误统计 | [0-0xffffffff]
    FH_UINT32 bndry_match; // 帧开始结束不匹配错误统计 | [0-0xffffffff]
    FH_UINT32 Seq_Err; // 帧时序传输错误统计 | [0-0xffffffff]
    FH_UINT32 FrameCrcErr; // 一帧中至少一个crc错误 | [0-0xffffffff]
    FH_UINT32 SotSyncErr; // data lane sot错误 依旧可以同步 | [0-0xffffffff]
    FH_UINT32 EscErr; // data lane escape命令序列错误 | [0-0xffffffff]
    FH_UINT32 VccErr; //　channel上存在无法识别的数据 | [0-0xffffffff]
    FH_UINT32 EccCorrErr; // 可校正的ecc错误 | [0-0xffffffff]
}FH_MIPI_ERRINFO_S;

typedef struct mipi_debug_info
{
    FH_UINT32 u32DevId; // 设备号 | [0-2]
    FH_MIPI_ERRINFO_S stErrInfo; // mipi错误信息统计 | [FH_MIPI_ERRINFO_S]
}FH_MIPI_DEBUGINFO_S;

/** VICAP **/
/*
*   Name: FH_VICAP_GetBufSize
*            获取入参模式下控制的VICAP模块使用的内存大小
*
*   Parameters:
*
*       [OUT]  FH_VICAP_DEV_ATTR_S* pstDevAttr
*            初始化VICAP设备的参数
*
*   Return:
*           小于0(失败)
*           其他值(缓存的大小)
*   Note:
*      1. u8DevNum决定了获取的是多个还是一个VICAP设备使用的缓存大小，并且决定了使用astBufCfg数组中的前多少个结构体变量
*      2. 例如u8DevNum等于2时，只会获取到astBufCfg[0]/astBufCfg[1]对应的u8DevId设备的使用的缓存的大小
*/
FH_SINT32 FH_VICAP_GetBufSize(FH_VICAP_BUF_SIZE_ATTR_S* stBufSizeAttr);
/*
*   Name: FH_VICAP_InitViDev
*            初始化VICAP使用的内存
*
*   Parameters:
*
*       [IN] VI_DEV devId
*            初始化的VI的设备号
*
*       [IN]  FH_VICAP_DEV_ATTR_S* pstDevAttr
*            初始化VICAP设备的参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_InitViDev(VI_DEV devId, FH_VICAP_DEV_ATTR_S* pstDevAttr);
/*
*   Name: FH_VICAP_SetViAttr
*            初始化VICAP硬件模块工作属性
*
*   Parameters:
*
*       [IN] VI_DEV devId
*            初始化的VI的设备号
*
*       [IN]  FH_VICAP_VI_ATTR_S* pstViAttr
*            初始化VICAP硬件参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_SetViAttr(VI_DEV devId, FH_VICAP_VI_ATTR_S* pstViAttr);
/*
*   Name: FH_VICAP_GetViAttr
*            获取当前VICAP硬件初始化属性
*
*   Parameters:
*
*       [IN] VI_DEV devId
*            初始化的VI的设备号
*
*       [OUT]  FH_VICAP_VI_ATTR_S* pstViAttr
*            获取当前VICAP硬件初始化参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_GetViAttr(VI_DEV devId, FH_VICAP_VI_ATTR_S* pstViAttr);
/*
*   Name: FH_VICAP_SetPipeCrop
*            配置VICAP通道参见属性
*
*   Parameters:
*
*       [IN] VI_DEV devId
*            VICAP裁剪通路的设备号
*
*       [IN]  FH_VICAP_WDR_CHAN_E enWdrChan
*            VICAP裁剪通道号
*
*       [IN]  FH_VICAP_CROP_CFG_S* pstCropCfg
*            VICAP裁剪配置
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_SetPipeCrop(VI_DEV devId, FH_VICAP_WDR_CHAN_E enWdrChan, FH_VICAP_CROP_CFG_S* pstCropCfg);
/*
*   Name: FH_VICAP_GetPipeCrop
*            获取VICAP通道裁剪的配置
*
*   Parameters:
*
*       [IN] VI_DEV devId
*            VICAP裁剪通路的设备号
*
*       [IN] FH_VICAP_WDR_CHAN_E enWdrChan
*            VICAP裁剪通道号
*
*       [OUT]  FH_VICAP_CROP_CFG_S* pstCropCfg
*            获取到的VICAP的裁剪配置
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_GetPipeCrop(VI_DEV devId, FH_VICAP_WDR_CHAN_E enWdrChan, FH_VICAP_CROP_CFG_S* pstCropCfg);
/*
*   Name: FH_VICAP_SetDvpCfg
*            配置DVP Sensor接入的配置
*
*   Parameters:
*
*       [IN] FH_VICAP_DVP_CFG_S* pstDvpCfg
*            DVP SENSOR接入配置
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      本项目不支持DVP功能
*/
FH_SINT32 FH_VICAP_SetDvpCfg(FH_VICAP_DVP_CFG_S* pstDvpCfg);
/*
*   Name: FH_VICAP_SetDvpCfg
*            获取DVP Sensor接入的配置
*
*   Parameters:
*
*       [OUT] FH_VICAP_DVP_CFG_S* pstDvpCfg
*            DVP SENSOR接入配置
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      本项目不支持DVP功能
*/
FH_SINT32 FH_VICAP_GetDvpCfg(FH_VICAP_DVP_CFG_S* pstDvpCfg);
// 注册需要拼接的通道号，注册之后，该路ISP支持拼接，注册后即开启拼接，不想开启拼接清调用注销接口
/*
*   Name: FH_VICAP_RegisterStitchDev
*            注册拼接的设备到拼接组里面
*
*   Parameters:
*
*       [IN] FH_VICAP_STITCH_ATTR_S* pstStithAttr
*            拼接相关的属性
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      本项目不支持拼接功能
*/
FH_SINT32 FH_VICAP_RegisterStitchDev(FH_VICAP_STITCH_ATTR_S* pstStithAttr);
/*
*   Name: FH_VICAP_UnreisterStitchDev
*            注销掉需要拼接的通道号，注销掉之后，该路通道不参与拼接
*
*   Parameters:
*
*       [IN] VI_DEV devId
*            目标注销掉的设备号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      本项目不支持拼接功能
*/
FH_SINT32 FH_VICAP_UnregisterStitchDev(VI_DEV devId);
/*
*   Name: FH_VICAP_Exit
*            VICAP设备退出
*
*   Parameters:
*
*       [IN] VI_DEV devId
*            需要退出的VICAP设备号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_Exit(VI_DEV devId);
/*
*   Name: FH_VICAP_SetSnsClk
*            配置目标sensor的工作时钟
*
*   Parameters:
*
*       [IN] FH_VICAP_SNS_IDX_E enSnsIdx
*            sensor序号
*
*       [IN]  FH_UINT32 u32Clock
*            sensor的工作时钟
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      FPGA环境下未测试
*/
FH_SINT32 FH_VICAP_SetSnsClk(FH_VICAP_SNS_IDX_E enSnsIdx, FH_UINT32 u32Clock);
/*
*   Name: FH_VICAP_SetSnsClkEn
*            配置目标sensor的工作时钟使能
*
*   Parameters:
*
*       [IN] FH_VICAP_SNS_IDX_E enSnsIdx
*            sensor序号
*
*       [IN]  FH_BOOL bEn
*            sensor工作时钟使能开关
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      FPGA环境下未测试
*/
FH_SINT32 FH_VICAP_SetSnsClkEn(FH_VICAP_SNS_IDX_E enSnsIdx, FH_BOOL bEn);
/*
*   Name: FH_VICAP_SetFrameSeqGenCfg
*            配置产生触发信号模块的参数
*
*   Parameters:
*
*       [IN] FH_VICAP_SNS_IDX_E enSnsIdx
*            sensor序号
*
*       [IN]  FH_VICAP_FRAME_SEQ_CFG_S* pstFrameSeqCfg
*            目标配置的触发信号参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      1. hsync的实际的cycle的单位时间单位为: 32 / vicap频率
*      2. vsync的实际输cycle的单位时间单位为: hsync总的时间 / vicap频率
*      3. vsync在５个vsync单位cycle之后，开始输出
*      4. vsync实际输出波形的频率等于:vicap工作频率 / vsync配置的cycle值
*      5. hsync实际输出波形的频率等于:vicap工作频率 / hsync配置的cycle值
*/
FH_SINT32 FH_VICAP_SetFrameSeqGenCfg(FH_VICAP_SNS_IDX_E enSnsIdx, FH_VICAP_FRAME_SEQ_CFG_S* pstFrameSeqCfg);
/*
*   Name: FH_VICAP_GetFrameSeqGenCfg
*            配置特定的触发信号模块的参数
*
*   Parameters:
*
*       [IN] FH_VICAP_SNS_IDX_E enSnsIdx
*            sensor序号
*
*       [IN/OUT]  FH_VICAP_FRAME_SEQ_CFG_S* pstFrameSeqCfg
*            获取目标配置的触发信号参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      1. hsync的实际的cycle的单位时间单位为: 32 / vicap频率
*      2. vsync的实际输cycle的单位时间单位为: hsync总的时间 / vicap频率
*      3. vsync在５个vsync单位cycle之后，开始输出
*      4. vsync实际输出波形的频率等于:vicap工作频率 / vsync配置的cycle值
*      5. hsync实际输出波形的频率等于:vicap工作频率 / hsync配置的cycle值
*/
FH_SINT32 FH_VICAP_GetFrameSeqGenCfg(FH_VICAP_SNS_IDX_E enSnsIdx, FH_VICAP_FRAME_SEQ_CFG_S* pstFrameSeqCfg);
/*
*   Name: FH_VICAP_SetFrameSeqGenEn
*            配置触发信号模块输出使能
*
*   Parameters:
*
*       [IN] FH_UINT16 u16EnCfg
*            触发信号使能，一次性配置多个
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      1.u16EnCfg = 1，打开通道0触发信号输出，关闭其他通道
*      2.u16EnCfg = 0x3，打开通道0/1触发信号输出
*/
FH_SINT32 FH_VICAP_SetFrameSeqGenEn(FH_UINT16 u16EnCfg);
/*
*   Name: FH_VICAP_GetFrameSeqGenEn
*            获取当前配置的触发信号模块输出使能
*
*   Parameters:
*
*       [OUT] FH_UINT16 *u16EnCfg
*            当前配置使能开关值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_GetFrameSeqGenEn(FH_UINT16 *u16EnCfg);
/*
*   Name: FH_VICAP_GetStream
*            获取指定通道的一帧数据流
*
*   Parameters:
*
*       [IN/OUT] FH_VICAP_STREAM_S *stStream
*            配置的信息，以及返回获取到的数据地址
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_GetStream(FH_VICAP_STREAM_S *stStream);
/*
*   Name: FH_VICAP_ReleaseStream
*            获取指定通道的一帧数据流
*
*   Parameters:
*
*       [IN/OUT] FH_VICAP_STREAM_S *stStream
*            FH_VICAP_GetStream配置/获取到的指针信息
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_ReleaseStream(FH_VICAP_STREAM_S *pstStream);
/*
*   Name: FH_VICAP_GetStatus
*            获取VICAP当前工作状态
*
*   Parameters:
*       [IN] FH_VICAP_SNS_IDX_E enSnsIdx
*            sensor序号
*       [OUT] FH_VICAP_STATUS_S *stStatus
*            获取到的VICAP的工作状态信息，具体数据参看结构体定义
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        无
*/
FH_UINT32 FH_VICAP_GetStatus(FH_VICAP_SNS_IDX_E enSnsIdx, FH_VICAP_STATUS_S *stStatus);
/*
*   Name: FH_VICAP_Pause
*            暂停VICAP的输出
*
*   Parameters:
*       [IN] VI_DEV devId
*            mipi设备号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        暂停mipi wrap给vicap的输入 在线模式与离线模式均有效
*/
FH_SINT32 FH_VICAP_Pause(VI_DEV devId);
/*
*   Name: FH_VICAP_Resume
*            暂停VICAP的输出
*
*   Parameters:
*       [IN] VI_DEV devId
*            mipi设备号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        恢复mipi wrap给vicap的输入 在线模式与离线模式均有效
*/
FH_SINT32 FH_VICAP_Resume(VI_DEV devId);
/*
*   Name: FH_VICAP_Open
*            打开VICAP设备
*
*   Parameters:
*       [IN] 无
*
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        无
*/
FH_SINT32 FH_VICAP_Open(FH_VOID);
/*
*   Name: FH_VICAP_Close
*           关闭VICAP设备
*
*   Parameters:
*       [IN] 无
*
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        无
*/
FH_SINT32 FH_VICAP_Close(FH_VOID);
/*
*   Name: FH_VICAP_RegisterCallBack
*           注册vicap的回调函数
*
*   Parameters:
*       [IN] FH_UINT32 u32DevId
*            设备号
*       [IN] VicapIntCallback cb
*            回调函数
*       [IN] FH_VICAP_CALLBACK_POS_E enPos
*            回调函数调用时机
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        在rtos环境下并且只在aov环境下使用
*/
FH_SINT32 FH_VICAP_RegisterCallBack(FH_UINT32 u32DevId, VicapIntCallback cb, FH_VICAP_CALLBACK_POS_E enPos);
/*
*   Name: FH_VICAP_SetSnsMount
*           设置sensor与对应的mipi口的挂载关系
*
*   Parameters:
*       [IN] VI_DEV devId
*            mipi设备号
*
*       [IN] FH_VICAP_SNS_MOUNT_CFG_S* pstSnsMountCfg
*            对应的devId对应的sensor的挂载点
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        根据的硬件特性 仅mipi0 / mipi1需要执行该函数 默认mipi0挂载两个 需要特殊需求 在mipi1上挂载两个sensor时可调用该接口
*/
FH_SINT32 FH_VICAP_SetSnsMount(VI_DEV devId, FH_VICAP_SNS_MOUNT_CFG_S* pstSnsMountCfg);
/*
*   Name: FH_VICAP_GetDebugInfo
*            获取mipi对应的统计信息
*
*   Parameters:
*
*       [IN/OUT] FH_MIPI_DEBUGINFO_S *pstDebugInfo
*            用于获取
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_GetDebugInfo(FH_MIPI_DEBUGINFO_S *pstDebugInfo);
/*
*   Name: FH_MIPI_PhyReset
*            执行mipiphy模块的reset
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            mipi设备号
*
*       [IN] FH_MIPI_PHY_RESET_E enReset
*            对应的复位的模式
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_MipiReset(FH_UINT32 u32DevId, FH_MIPI_PHY_RESET_E enReset);
/*
*   Name: FH_VICAP_Reset
*            从外部将整个vicap模块执行复位
*
*   Parameters:
*       [IN] 无
*
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FH_VICAP_Reset(FH_VOID);
#pragma pack()
#endif // __FH_VICAP_MPI_H__
