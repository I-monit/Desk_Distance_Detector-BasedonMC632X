#ifndef __FH_VICAP_MPI_H__
#define __FH_VICAP_MPI_H__

#include "types/type_def.h"

typedef FH_UINT32 VI_DEV;
#define VICAP_MAX_SENSOR_NUM 2
#define FUNC_DEP __attribute__((deprecated))
/**|VICAP|**/
typedef enum
{
    VICAP_SNS0 = 0, // sensor0的序号
    VICAP_SNS1 = 1, // sensor1的序号
    VICAP_SNS2 = 2, // sensor2的序号
}FH_VICAP_SNS_IDX_E;

enum __VICAP_IDX__
{
    VICAP_PIPELINE_MIPI0 = 0,   // MIPI0序号
    VICAP_PIPELINE_MIPI1 = 1,   // MIPI1序号
};

typedef enum
{
    VICAP_IN_TYPE_MIPI0  = 0,   // MIPI0输入
    VICAP_IN_TYPE_MIPI1  = 1,   // MIPI1输入
    VICAP_IN_TYPE_MIPI2  = 2,   // MIPI2输入
    VICAP_IN_TYPE_ISP    = 3,   // 在线模式VI给ISP的输入，离线模式下无用
}FH_VICAP_VI_TYPE_E;

typedef enum __VICAP_WORK_MODE__
{
    VICAP_WORK_MODE_ONLINE     = 0,     // vicap工作在在线模式
    VICAP_WORK_MODE_OFFLINE    = 1,     // vicap工作在离线模式
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

typedef struct _VICAP_SIZE_S_
{
    FH_UINT16    u16Width;   // 图像的宽度 | [0~0xffff]
    FH_UINT16    u16Height;  // 图像的高度 | [0~0xffff]
}FH_VICAP_SIZE_S;

typedef struct _VICAP_RECT_S_
{
    FH_UINT16    u16X;       // 图像裁剪的水平偏移值 | [0~0xffff]
    FH_UINT16    u16Y;       // 图像裁剪的垂直偏移值 | [0~0xffff]
    FH_UINT16    u16Width;   // 图像裁剪后的宽度 | [0~0xffff]
    FH_UINT16    u16Height;  // 图像裁剪后的高度 | [0~0xffff]
}FH_VICAP_RECT_S;

typedef struct _VICAP_CROP_CFG_S_
{
    FH_BOOL            bCutEnable;  // 裁剪的使能寄存器 | [0~1]
    FH_VICAP_RECT_S    stRect;      // 裁剪的偏移及裁剪后的宽高 | [FH_VICAP_RECT_S]
}FH_VICAP_CROP_CFG_S;

typedef struct _VICAP_OFFLINE_CFG_S_
{
    FH_UINT8 u8Priority;    // 离线遍历优先级 | [0~0xf]
}FH_VICAP_OFFLINE_CFG_S;

typedef struct _VICAP_VI_ATTR_S_
{
    FH_VICAP_BAYER_TYPE_E     enBayerType;   // bayer pattern | [FH_VICAP_BAYER_TYPE_E]
    FH_VICAP_WORK_MODE_E      enWorkMode;    // 配置当前vicap的工作模式 | [FH_VICAP_WORK_MODE_E]
    FH_UINT8                  u8WdrMode;     // 输入sensor的线性还是wdr配置 | [0~1]
    FH_VICAP_SIZE_S           stInSize;      // 输入幅面 | [FH_VICAP_SIZE_S]
    FH_VICAP_CROP_CFG_S       stCropSize;    // 裁剪后幅面 | [FH_VICAP_CROP_CFG_S]
    FH_VICAP_OFFLINE_CFG_S    stOfflineCfg;  // 离线下的配置 | [FH_VICAP_OFFLINE_CFG_S]
}FH_VICAP_VI_ATTR_S;

typedef struct _VICAP_DEV_ATTR_S_
{
    FH_BOOL                     bUsingVb;       // 是否使用VB缓存逻辑 | [0~1]
    FH_VICAP_WORK_MODE_E        enWorkMode;     // 配置当前vicap的工作模式，影响内存分配 | [FH_VICAP_WORK_MODE_E]
    FH_VICAP_SIZE_S             stSize;         // 图像幅面, 会影响内存分配, 故需按应用需要的最大幅面配置 | [FH_VICAP_SIZE_S]
}FH_VICAP_DEV_ATTR_S;

typedef struct _VICAP_BUF_SIZE_ATTR_S_
{
    FH_UINT8 u8DevNum;         // 总共的vicap通路 | [0~VICAP_MAX_SENSOR_NUM]
    struct _buf_size_cfg
    {
        FH_UINT8            u8DevId;    // 设备号 | [0~VICAP_MAX_SENSOR_NUM]
        FH_VICAP_DEV_ATTR_S stDevAttr;  // 设备参数 | [FH_VICAP_DEV_ATTR_S]
        FH_UINT32           u32BufSize;  // 缓存的总大小 | [0~0xffffffff]
        FH_UINT32           u32BufNum;   // 缓存块数,0表示使用了循环缓冲 | [0~0xffffffff]
    } astBufCfg[VICAP_MAX_SENSOR_NUM];
}FH_VICAP_BUF_SIZE_ATTR_S;

typedef struct _VICAP_FRAME_SEQ_CFG_S_
{
    FH_VICAP_SYNC_TYPE_E enSyncType;    // 选择配置hsync还是vsync
    FH_UINT16   u16Period;              // 触发信号的一个周期的cycle数 | [0~0x1fff]
    FH_UINT16   u16Duty;                // 触发信号的有效电平cycle数   | [0~0x1fff]
    FH_BOOL     bPolatiry;              // 有效电平的极性 | [0~1]

    //VICAP_VSYNC适用
    FH_UINT16   u16VStart;    // 触发信号起始时刻偏移,htotal的个数 | [0~0xfff]
    FH_UINT8    u8TrigSel;    // 触发使能控制,0表示由使能开关控制FH_VICAP_SetFrameSeqGenEn;1表示另一路结束后自动触发,不受使能开关控制  | [0~1]
    FH_UINT8    u8TrigMode;   // 触发方式,0表示连续触发,1表示单次触发,如果u8TrigSel配1,那么u8TrigMode也需要配1  | [0~1]
}FH_VICAP_FRAME_SEQ_CFG_S;

typedef struct _VICAP_FRAME_SEQ_CFG_ADV_S_
{
    FH_BOOL     bSeqTrigMode; // 同步信号触发模式,0为软件设置框架双边触发，1为硬件mipiend后vbtime时间触发 | [0~1]
    FH_UINT16   u16Fps;       // sensor帧率   | [0~0x1fff]
    //VICAP_VSYNC适用
    FH_UINT16   u16VStart;    // 触发信号起始时刻偏移,0为周期的一半，1为周期的四分之一 | [0~1]
}FH_VICAP_FRAME_SEQ_CFG_ADV_S;

typedef struct _VICAP_STREAM_COM_S_
{
    FH_PHYADDR addrPhy;     // 数据的物理地址 | [FH_PHYADDR]
    FH_UINT32  u32VirAddr;  // 应用层的虚拟地址，同一个进程空间可直接使用 | [FH_UINT32]
    FH_UINT32  u32Crc;      // 数据校验值，仅压缩模式下有用   | [0~0xffffffff]
    FH_UINT64  u64TimeStamp;// 数据获取的时间戳 | [0~0xffffffffffffffff]
    FH_UINT32  u32BufSize;  // 获取到的离线数据的缓存大小 | [0~0xffffffff]
}FH_VICAP_STREAM_COM_S;

typedef struct _VICAP_STREAM_S_
{
    FH_UINT8   u8DevId;         // 设备号 | [0~2]
    FH_BOOL    bLock;           // 是否加锁 | [0~1]
    FH_UINT32  u32FrmCnt;       // 帧号 | [0~0xffffffff]
    FH_UINT32  u32LineStride;   // 一行的数据量，单位是字节，压缩模式下该变量无用 | [0~0xffffffff]
    FH_VICAP_SIZE_S stPic;      // 图像幅面相关的配置 | [FH_VICAP_SIZE_S]
    FH_VICAP_STREAM_COM_S stSf;  // 短帧的数据流 | [FH_VICAP_STREAM_COM_S]
    FH_VICAP_STREAM_COM_S stLf;  // 长帧的数据流 | [FH_VICAP_STREAM_COM_S]
}FH_VICAP_STREAM_S;

typedef struct _VICAP_VI_EN_CFG_S_
{
    FH_VICAP_VI_TYPE_E enType;  //  VI类型 | [FH_VICAP_VI_TYPE_E]
    FH_BOOL            bEn;     //  是否使能 | [0~1]
}FH_VICAP_VI_EN_CFG_S;
/** VICAP **/
/*
*   Name: FH_VICAP_GetBufSize
*            获取入参模式下控制的VICAP模块使用的内存大小
*
*   Parameters:
*
*       [IN/OUT]  FH_VICAP_BUF_SIZE_ATTR_S* pstDevAttr
*            初始化VICAP设备的参数和获取到的结果
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*   Note:
*      0. 是否可以动态调用:是
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
*      是否可以动态调用:否
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
*      是否可以动态调用:否
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
*       [IN]  FH_VICAP_VI_ATTR_S* pstViAttr
*            获取当前VICAP硬件初始化参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      是否可以动态调用:否
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
*      不支持
*/
FH_SINT32 FUNC_DEP FH_VICAP_SetPipeCrop(VI_DEV devId, FH_VICAP_WDR_CHAN_E enWdrChan, FH_VICAP_CROP_CFG_S* pstCropCfg);
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
*      不支持
*/
FH_SINT32 FUNC_DEP FH_VICAP_GetPipeCrop(VI_DEV devId, FH_VICAP_WDR_CHAN_E enWdrChan, FH_VICAP_CROP_CFG_S* pstCropCfg);
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
*      是否可以动态调用:否
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
*      是否可以动态调用:是
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
*      是否可以动态调用:是
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
*      0. 是否可以动态调用:是
*      1. hsync的实际的cycle的单位时间单位为: 32 / vicap频率
*      2. vsync的实际输cycle的单位时间单位为: hsync总的时间 / vicap频率
*      3. vsync在５个vsync单位cycle之后，开始输出
*      4. vsync实际输出波形的频率等于:vicap工作频率 / vsync配置的cycle值
*      5. hsync实际输出波形的频率等于:vicap工作频率 / hsync配置的cycle值
*/
FH_SINT32 FH_VICAP_SetFrameSeqGenCfg(FH_VICAP_SNS_IDX_E enSnsIdx, FH_VICAP_FRAME_SEQ_CFG_S* pstFrameSeqCfg);
/*
*   Name: FH_VICAP_SetFrameSeqGenCfgNew
*            配置产生触发信号模块的参数
*
*   Parameters:
*
*       [IN] FH_VICAP_SNS_IDX_E enSnsIdx
*            sensor序号
*
*       [IN]  FH_VICAP_FRAME_SEQ_CFG_ADV_S* pstFrameSeqCfgAdv
*            目标配置的触发信号参数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      0. 是否可以动态调用:是
*      1. 如果配置无法满足需求，请调用原先接口
*/
FH_SINT32 FH_VICAP_SetFrameSeqGenCfgAdv(FH_VICAP_SNS_IDX_E enSnsIdx, FH_VICAP_FRAME_SEQ_CFG_ADV_S* pstFrameSeqCfgAdv);
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
*      0. 是否可以动态调用:是
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
*      0.是否可以动态调用:是
*      1.u16EnCfg = 1，打开通道0触发信号输出，关闭其他通道
*      2.u16EnCfg = 0x6，打开通道1/2触发信号输出，关闭其他通道
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
*      是否可以动态调用:是
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
*        不支持
*
*/
FH_SINT32 FUNC_DEP FH_VICAP_GetStream(FH_VICAP_STREAM_S *stStream);
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
*        不支持
*
*/
FH_SINT32 FUNC_DEP FH_VICAP_ReleaseStream(FH_VICAP_STREAM_S *pstStream);
/*
*   Name: FH_VICAP_SetViEn
*            配置VI或在线模式下的ISP使能开关
*
*   Parameters:
*
*       [IN] FH_VICAP_VI_EN_CFG_S *pstCfg
*            控制VI输入使能参数的结构体指针
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        是否可以动态调用:是
*/
FH_SINT32 FH_VICAP_SetViEn(FH_VICAP_VI_EN_CFG_S* pstCfg);
/*
*   Name: FH_VICAP_GetViEn
*            获取当前VI或在线模式下的ISP使能开关配置状态
*
*   Parameters:
*
*       [IN/OUT] FH_VICAP_VI_EN_CFG_S *pstCfg
*            获取VI输入使能参数的结构体指针
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        是否可以动态调用:是
*/
FH_SINT32 FH_VICAP_GetViEn(FH_VICAP_VI_EN_CFG_S* pstCfg);
/*
*   Name: FH_VICAP_Pause
*            暂停VICAP对图像的处理与输出
*
*   Parameters:
*
*       [IN] VI_DEV devId
*            需要暂停的VICAP设备号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        是否可以动态调用:是
*/
FH_SINT32 FH_VICAP_Pause(VI_DEV devId);
/*
*   Name: FH_VICAP_Resume
*            恢复VICAP对图像的处理与输出
*
*   Parameters:
*
*       [IN] VI_DEV devId
*            需要恢复的VICAP设备号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        是否可以动态调用:是
*/
FH_SINT32 FH_VICAP_Resume(VI_DEV devId);
/*
*   Name: FH_VICAP_GetClk
*            获取VICAP的时钟
*
*   Parameters:
*
*       [IN/OUT] FH_UINT32 u32VicapClk
*            获取VICAP的时钟
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*        获取VICAP的时钟
*/
FH_SINT32 FH_VICAP_GetClk(FH_UINT32 *u32VicapClk);
#pragma pack()
#endif // __FH_VICAP_MPI_H__
