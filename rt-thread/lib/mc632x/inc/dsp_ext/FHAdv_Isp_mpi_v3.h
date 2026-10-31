/**
 * @file     FHAdv_Isp_mpi_v3.h
 * @brief    FHAdv ISP moduel interface
 * @version  V1.0.0
 * @date     11-8-2020
 * @author   Software Team
 *
 * @note
 * Copyright (C) 2016 Shanghai Fullhan Microelectronics Co., Ltd.
 * All rights reserved.
 *
 * @par
 * Fullhan is supplying this software which provides customers with programming
 * information regarding the products. Fullhan has no responsibility or
 * liability for the use of the software. Fullhan not guarantee the correctness
 * of this software. Fullhan reserves the right to make changes in the software
 * without notification.
 *
 */

#ifndef _FHADV_ISP_MPI_H
#define _FHADV_ISP_MPI_H

#include "FH_typedef.h"
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**| ADV_ISP |**/
enum FH_SINT32t
{
    FH_SINT32t_1_3      = 333333,   // 曝光时间333.333ms
    FH_SINT32t_1_6      = 166666,   // 曝光时间166.666ms
    FH_SINT32t_1_12     = 83333,    // 曝光时间83.33ms
    FH_SINT32t_1_25     = 40000,    // 曝光时间40ms
    FH_SINT32t_1_50     = 20000,    // 曝光时间20ms
    FH_SINT32t_1_100    = 10000,    // 曝光时间10ms
    FH_SINT32t_1_150    = 6666,     // 曝光时间6.666ms
    FH_SINT32t_1_200    = 5000,     // 曝光时间5ms
    FH_SINT32t_1_250    = 4000,     // 曝光时间4ms
    FH_SINT32t_1_500    = 2000,     // 曝光时间2ms
    FH_SINT32t_1_750    = 1333,     // 曝光时间1.333ms
    FH_SINT32t_1_1000   = 1000,     // 曝光时间1ms
    FH_SINT32t_1_2000   = 500,      // 曝光时间500us
    FH_SINT32t_1_4000   = 250,      // 曝光时间250us
    FH_SINT32t_1_10000  = 100,      // 曝光时间100us
    FH_SINT32t_1_100000 = 10,      // 曝光时间10us
};
typedef enum FH_SINT32t FHADV_ISP_AE_TIME_LEVEL;

/** ADV_ISP **/
/*
*   Name: FHAdv_Isp_SensorInit
*            sensor初始化
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  struct isp_sensor_if* sns_if
*            传入sensor结构体指针
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_SensorInit(FH_UINT32 u32DevId, struct isp_sensor_if* sns_if);
/*
*   Name: FHAdv_Isp_Init
*            ISP参数初始化
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      为了能正确读取初始参数，请务必在程序初始化时调用FHAdv_Isp_Init接口,调用位置必须在配置完成sensor幅面和加载完成效果参数之后。
*/
FH_SINT32 FHAdv_Isp_Init(FH_UINT32 u32DevId);
/*
*   Name: FHAdv_Isp_SetAwbGain
*            设置自动白平衡增益
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN] FH_UINT32 mode
*            设置模式：0:自动白平衡 1:锁定白平衡 2:白炽灯 3:暖光灯 4:自然光 5:日光灯
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      模式2/3/4/5必须sensor库实现get_user_awb_gain函数
*/
FH_SINT32 FHAdv_Isp_SetAwbGain(FH_UINT32 u32DevId,FH_UINT32 mode);
/*
*   Name: FHAdv_Isp_GetAwbGain
*            获取自动白平衡增益
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT] AwbGain
*            数组首地址
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      使用方法参考：FH_UINT32 awbgain[3];
*                 FHAdv_Isp_GetAwbGain(FH_UINT32 u32DevId,awbgain);
*              结果保存在awbgain数组中,分别对应rgain,ggain,bgain
*/
FH_SINT32 FHAdv_Isp_GetAwbGain(FH_UINT32 u32DevId,FH_UINT32 *AwbGain);
/*
*   Name: FHAdv_Isp_SetAEMode
*            设置自动曝光模式
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 mode
*            设置模式
*
*       [IN]  FH_UINT32 intt_us
*            可以选择FHADV_ISP_AE_TIME_LEVEL枚举类型中的一个, 也可以直接输入参数, 单位是微秒(us)
*
*       [IN]  FH_UINT32 gain_level
*            增益等级，0 ~ 100, 0代表增益1倍, 100代表增益最大值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*          mode 0:自动, intt和gain的最大值由intt_us和gain_level决定
*          mode 1:曝光时间手动, 手动曝光由intt_us决定,gain最大值由gain_level决定
*          mode 2:曝光增益手动, 手动增益由gain_level决定,intt最大值由intt_us决定
*          mode 3:均手动, 手动曝光由intt_us决定,手动增益由gain_level决定
*/
FH_SINT32 FHAdv_Isp_SetAEMode(FH_UINT32 u32DevId, FH_UINT32 mode, FH_UINT32 intt_us, FH_UINT32 gain_level);
/*
*   Name: FHAdv_Isp_GetAEMode
*            获取自动曝光模式
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT]  &intt_us
*            实际曝光时间,单位为微秒(us)
*
*       [OUT]  &gain_value
*            曝光增益等级(0~100),值越大增益越大,由于量化误差,获取值与输入值之间可能会有差异
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*            使用方法参考：
*                 FH_SINT32 intt_us, gain_value;
*                 FHAdv_Isp_GetAEMode(FH_UINT32 u32DevId, &intt_us, &gain_value);
*/
FH_SINT32 FHAdv_Isp_GetAEMode(FH_UINT32 u32DevId, FH_UINT32 *intt_us, FH_UINT32 *gain_level);
/*
*   Name: FHAdv_Isp_SetLtmCfg
*            设置宽动态参数
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 mode
*            0:关闭, 1:开启
*
*       [IN]  FH_UINT32 level
*            0~10 值越大，动态范围越大, 必须sensor库实现get_user_ltm_curve函数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_SetLtmCfg(FH_UINT32 u32DevId, FH_UINT32 mode, FH_UINT32 level);
/*
*   Name: FHAdv_Isp_GetLtmCfg
*            设置宽动态参数
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT]  FH_UINT32 *mode
*            0:关闭, 1:开启
*
*       [OUT]  FH_UINT32 *level
*            0~10 值越大，动态范围越大, 必须sensor库实现get_user_ltm_curve函数
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      V3 不支持
*/
FH_SINT32 FHAdv_Isp_GetLtmCfg(FH_UINT32 u32DevId, FH_UINT32 *mode, FH_UINT32 *level);
/*
*   Name: FHAdv_Isp_SetBrightness
*            设置亮度等级
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 value
*            范围0-255, 128表示采用默认值,即FHAdv_Isp_Init获取得到的值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      V3 不支持
*/
FH_SINT32 FHAdv_Isp_SetBrightness(FH_UINT32 u32DevId, FH_UINT32 value);
/*
*   Name: FHAdv_Isp_GetBrightness
*            获取当前亮度等级
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 *pValue
*            范围0-255, 由于量化误差,获取值与输入值之间可能会有差异
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_GetBrightness(FH_UINT32 u32DevId, FH_UINT32 *pValue);
/*
*   Name: FHAdv_Isp_SetContrast
*            设置对比度等级
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 mode
*            0:手动模式，1:gainmapping模式，根据mapping选择并可以手动做整体mapping调整
*
*       [IN]  FH_UINT32 value
*            范围0-255, 128表示采用默认值,即FHAdv_Isp_Init获取得到的值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_SetContrast(FH_UINT32 u32DevId, FH_UINT32 mode, FH_UINT32 value);
/*
*   Name: FHAdv_Isp_GetContrast
*            获取当前对比度等级
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT]  FH_UINT32 *mode
*            0:手动模式，1:gainmapping模式，根据mapping选择并可以手动做整体mapping调整
*
*       [OUT]  FH_UINT32 *pValue
*            范围0-255, 由于量化误差,获取值与输入值之间可能会有差异
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_GetContrast(FH_UINT32 u32DevId, FH_UINT32 *mode, FH_UINT32 *pValue);
/*
*   Name: FHAdv_Isp_SetSaturation
*            设置饱和度等级
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 mode
*            0:手动模式，1:gainmapping模式，根据mapping选择并可以手动做整体mapping调整
*
*       [IN]  FH_UINT32 value
*            范围0-255 128表示采用默认值,即FHAdv_Isp_Init获取得到的值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_SetSaturation(FH_UINT32 u32DevId, FH_UINT32 mode, FH_UINT32 value);
/*
*   Name: FHAdv_Isp_GetSaturation
*            获取当前饱和度等级
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT]  FH_UINT32 *mode
*            0:手动模式，1:gainmapping模式，根据mapping选择并可以手动做整体mapping调整
*
*       [OUT]  FH_UINT32 *pValue
*            范围0-255, 由于量化误差,获取值与输入值之间可能会有差异
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_GetSaturation(FH_UINT32 u32DevId, FH_UINT32 *mode, FH_UINT32 *pValue);
/*
*   Name: FHAdv_Isp_SetSharpeness
*            设置锐度
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 mode
*            0:手动模式，1:gainmapping模式，根据mapping选择并可以手动做整体mapping调整
*
*       [IN]  FH_UINT32 value
*            范围0~255 128表示采用默认值,即FHAdv_Isp_Init获取得到的值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_SetSharpeness(FH_UINT32 u32DevId, FH_UINT32 mode, FH_UINT32 value);
/*
*   Name: FHAdv_Isp_GetSharpeness
*            获取当前锐度
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT]  FH_UINT32 *mode
*            0:手动模式，1:gainmapping模式，根据mapping选择并可以手动做整体mapping调整
*
*       [OUT]  FH_UINT32 *pValue
*            范围0-255, 由于量化误差,获取值与输入值之间可能会有差异
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_GetSharpeness(FH_UINT32 u32DevId, FH_UINT32 *mode, FH_UINT32 *pValue);
/*
*   Name: FHAdv_Isp_SetChroma
*            设置色度
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 value
*            色度值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      暂未实现
*/
FH_SINT32 FHAdv_Isp_SetChroma(FH_UINT32 u32DevId, FH_UINT32 value);
/*
*   Name: FHAdv_Isp_GetChroma
*            获取色度
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT]  FH_UINT32 *pValue
*            色度值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      暂未实现
*/
FH_SINT32 FHAdv_Isp_GetChroma(FH_UINT32 u32DevId, FH_UINT32 *pValue);
/*
*   Name: FHAdv_Isp_SetMirrorAndflip
*            设置镜像和翻转
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 grpidx
*            无
*
*       [IN]  FH_UINT32 mirror
*            0:恢复镜像，1:使能镜像
*
*       [IN]  FH_UINT32 flip
*            0:恢复翻转，1:使能翻转
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_SetMirrorAndflip(FH_UINT32 u32DevId, FH_UINT32 grpidx, FH_UINT32 mirror, FH_UINT32 flip);
/*
*   Name: FHAdv_Isp_GetMirrorAndflip
*            获取镜像和翻转的状态
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT]  FH_UINT32 *pMirror
*            0:恢复镜像，1:使能镜像
*
*       [OUT]  FH_UINT32 *pFlip
*            0:恢复翻转，1:使能翻转
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_GetMirrorAndflip(FH_UINT32 u32DevId, FH_UINT32 *pMirror, FH_UINT32 *pFlip);
/*
*   Name: FHAdv_Isp_SetColorMode
*            设置颜色模式
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 mode
*            0:饱和度为0，1:使用默认饱和度
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_SetColorMode(FH_UINT32 u32DevId, FH_UINT32 mode);
/*
*   Name: FHAdv_Isp_GetColorMode
*            获取颜色模式
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT]  FH_UINT32 *pMode
*            0:黑白模式, 1:彩色模式
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_SINT32 FHAdv_Isp_GetColorMode(FH_UINT32 u32DevId, FH_UINT32 *pMode);
/*
*   Name: FH_ADV_ISP_Version
*            获取打印库版本信息
*
*   Parameters:
*
*       [IN] FH_UINT32 print_enable
*            打印使能
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      无
*/
FH_CHAR* FH_ADV_ISP_Version(FH_UINT32 print_enable);
/*
*   Name: FHAdv_Isp_SetDenoise3D
*            设置NR3D降噪等级
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 mode
*            0:手动模式，1:gainmapping模式，根据mapping选择并可以手动做整体mapping调整
*
*       [IN]  FH_UINT32 value
*            范围0~255 128表示采用默认值,即FHAdv_Isp_Init获取得到的值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      仅MC635x适用
*/
FH_SINT32 FHAdv_Isp_SetDenoise3D(FH_UINT32 u32DevId, FH_UINT32 mode, FH_UINT32 value);
/*
*   Name: FHAdv_Isp_GetDenoise3D
*            获取当前NR3D降噪等级
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT]  FH_UINT32 *mode
*            0:手动模式，1:gainmapping模式，根据mapping选择并可以手动做整体mapping调整
*
*       [OUT]  FH_UINT32 *pvalue
*            范围0-255, 由于量化误差,获取值与输入值之间可能会有差异
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      仅MC635x适用
*/
FH_SINT32 FHAdv_Isp_GetDenoise3D(FH_UINT32 u32DevId, FH_UINT32 *mode, FH_UINT32 *pvalue);
/*
*   Name: FHAdv_Isp_SetDenoise2D
*            设置NR2D降噪等级
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [IN]  FH_UINT32 mode
*            0:手动模式，1:gainmapping模式，根据mapping选择并可以手动做整体mapping调整
*
*       [IN]  FH_UINT32 value
*            范围0~255 128表示采用默认值,即FHAdv_Isp_Init获取得到的值
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      仅MC635x适用
*/
FH_SINT32 FHAdv_Isp_SetDenoise2D(FH_UINT32 u32DevId, FH_UINT32 mode, FH_UINT32 value);
/*
*   Name: FHAdv_Isp_GetDenoise2D
*            获取当前NR2D降噪等级
*
*   Parameters:
*
*       [IN] FH_UINT32 u32DevId
*            ISP的设备号
*
*       [OUT]  FH_UINT32 *mode
*            0:手动模式，1:gainmapping模式，根据mapping选择并可以手动做整体mapping调整
*
*       [OUT]  FH_UINT32 *pvalue
*            范围0-255, 由于量化误差,获取值与输入值之间可能会有差异
*
*   Return:
*           0(成功)
*          非0(失败，详见错误码)
*
*   Note:
*      仅MC635x适用
*/
FH_SINT32 FHAdv_Isp_GetDenoise2D(FH_UINT32 u32DevId, FH_UINT32 *mode, FH_UINT32 *pvalue);

/*@} end of group Video_ISP */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif