/**
 ****************************************************************************************************
 * @file        clk_bridge.h
 * @brief       VENC PTS 时钟域 <-> rt_tick 时钟域 换算桥 (Qt_GUI 分支)
 *
 * 目的: PC 端把 RTSP 流里任意帧的 PTS 换算到算法时间轴(rt_tick ms), 实现 D 值与画面帧级配对。
 * 原理: VENC 每帧 time_stamp(实测单位 us, 经 libpes x9/100 -> 90kHz PTS 烧进流)与编码器时钟同源,
 *       与 rt_tick 不同源; 本模块在每个编码帧到达时同步采样 (venc_ms[us/1000], rt_ms) 锚点对,
 *       tcp_ctrl 随状态 JSON 广播, PC 端多锚点回归得到线性映射。
 ****************************************************************************************************
 */

#ifndef __CLK_BRIDGE_H__
#define __CLK_BRIDGE_H__

#include <rtthread.h>   /* rt_uint32_t (clk_bridge_hb) */
#include "type_def.h"

/* 取自 getAllEncStreamToDmc 的每帧 VENC time_stamp (编码器时钟, 实测单位 us,
 * 2026-10-07 上板自检: 15fps 下相邻帧 delta~66700us=66.7ms 证实; 内部转 ms 存储)。
 * 由编码取流线程调用, 15fps 下开销可忽略。 */
void clk_bridge_update(FH_UINT64 venc_us);

/* 读取最近一次锚点对; 无采样过返回 -1, 否则 0 */
FH_SINT32 clk_bridge_get(FH_UINT64 *venc_ms, FH_UINT64 *rt_ms);

/* VPU/VENC 帧时间戳(us, PTS 时钟域) -> rt_tick(ms): 按最近锚点线性映射 (双向可外推);
 * 无锚点返回 -1。NN 帧真实采集时刻换算用 (Time_Stamp Phase②: ts 语义 = 采集时刻) */
FH_SINT64 clk_bridge_pts_to_rt(FH_UINT64 pts_us);

/* 媒体管线心跳计数 (每个编码帧 +1, 单调递增); 软件看门狗判定媒体链路死活用 */
rt_uint32_t clk_bridge_hb(void);

#endif /* __CLK_BRIDGE_H__ */
