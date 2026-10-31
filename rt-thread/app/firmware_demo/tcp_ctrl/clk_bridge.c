/**
 ****************************************************************************************************
 * @file        clk_bridge.c
 * @brief       VENC PTS <-> rt_tick 时钟桥锚点实现 (Qt_GUI 分支)
 ****************************************************************************************************
 */

#include <rtthread.h>
#include <rthw.h> /* rt_hw_interrupt_disable/enable */
#include <rttshell.h> /* SHELL_CMD_EXPORT (clk_verify / clk_print) */
#include <stdio.h>
#include <stdlib.h> /* atoi (clk_print 参数解析) */
#include "clk_bridge.h"

static FH_UINT64 g_venc_ms = 0;
static FH_UINT64 g_rt_ms   = 0;
static volatile int g_valid = 0;
volatile rt_uint32_t g_media_hb = 0;   /* 媒体心跳计数, guard 读取 (单调递增不清零) */

/* 单位自检: 前 11 帧各打印一次原始 us 增量, 15fps 下相邻帧差应约 66700us(=66.7ms);
 * 2026-10-07 实测 delta~66695us, 证实 time_stamp 单位为 us, 已在此转 ms 存储。 */
static FH_UINT64 g_dbg_last = 0;
static int g_dbg_cnt = 0;

/* msh clk_print: 逐帧锚点对打印开关 (联调比对每编码帧 pts/ats 用, 15fps 一行/帧,
 * 用完及时关避免刷屏; 打印与锚点更新同线程 (enc get_stream), 无并发问题) */
static volatile rt_uint8_t g_clk_print = 0;

void clk_bridge_update(FH_UINT64 venc_us)
{
    rt_base_t level;

    g_media_hb++;   /* 媒体管线心跳: 每个编码帧 +1, guard 据此判定 VICAP->ISP->VENC 链路死活 */

    if (g_dbg_cnt < 11)
    {
        printf("[clk_bridge] frame#%d venc_us=%llu delta_us=%llu\n",
               g_dbg_cnt, (unsigned long long)venc_us,
               (unsigned long long)(venc_us - g_dbg_last));
        g_dbg_last = venc_us;
        g_dbg_cnt++;
    }

    level = rt_hw_interrupt_disable();
    g_venc_ms = venc_us / 1000; /* us -> ms, 与 rt_tick 同单位, 供 JSON 导出 */
    g_rt_ms   = rt_tick_get_millisecond();
    g_valid   = 1;
    rt_hw_interrupt_enable(level);

    if (g_clk_print) /* 本帧锚点已采样, 打印即所见 (PC 端配对公式 rt=ats+(pts-pts_anchor)) */
    {
        printf("[clk] pts_us=%llu anchor(venc_ms=%llu, ats_rt_ms=%llu) hb=%lu\n",
               (unsigned long long)venc_us,
               (unsigned long long)g_venc_ms,
               (unsigned long long)g_rt_ms, (unsigned long)g_media_hb);
    }
}

FH_SINT32 clk_bridge_get(FH_UINT64 *venc_ms, FH_UINT64 *rt_ms)
{
    rt_base_t level;
    int valid;

    if (venc_ms == NULL || rt_ms == NULL)
        return -1;

    level = rt_hw_interrupt_disable();
    *venc_ms = g_venc_ms;
    *rt_ms   = g_rt_ms;
    valid    = g_valid;
    rt_hw_interrupt_enable(level);

    return valid ? 0 : -1;
}

/* 帧时刻换算: rt = rt_anchor + (pts_us/1000 - venc_ms_anchor)
 * 锚点在 15fps 下每 66.7ms 刷新一次, 帧与锚点间隔 ≤ 数百 ms, 时钟漂移(µs/s 级)
 * 引入的误差 < 1ms; 无锚点返回 -1 由调用方退化处理 */
FH_SINT64 clk_bridge_pts_to_rt(FH_UINT64 pts_us)
{
    FH_UINT64 venc_ms, rt_ms;

    if (clk_bridge_get(&venc_ms, &rt_ms) != 0)
        return -1;

    return (FH_SINT64)rt_ms + (FH_SINT64)(pts_us / 1000) - (FH_SINT64)venc_ms;
}

/* 媒体管线心跳读取 (软件看门狗 tof/guard/guard.c 用) */
rt_uint32_t clk_bridge_hb(void)
{
    return g_media_hb;
}

/* ---- msh 联调命令: 时钟桥健康检查 ----
 * 3 轮 x 2s 采样, 每轮取锚点对差分, 输出:
 *   - VENC 时钟对 rt_tick 的相对漂移 | [us/s] (时钟桥线性映射的成立前提)
 *   - 心跳增量折算的实际编码帧率 (15fps 实证, 可复现)
 * 现象定位: drift 异常大 => 锚点不可信, PC 端帧级配对会漂 */
static void clk_verify(void)
{
    FH_UINT64 v0, r0, v1, r1;
    rt_uint32_t hb0, hb1;
    int i;

    if (clk_bridge_get(&v0, &r0) != 0)
    {
        rt_kprintf("[clk_verify] no anchor yet (media pipeline not streaming)\n");
        return;
    }
    rt_kprintf("[clk_verify] anchor: venc_ms=%llu rt_ms=%llu\n",
               (unsigned long long)v0, (unsigned long long)r0);

    for (i = 0; i < 3; i++)
    {
        long long dv, dr, drift_us_s;
        rt_uint32_t fps_x100;

        hb0 = clk_bridge_hb();
        clk_bridge_get(&v0, &r0);
        rt_thread_mdelay(2000);
        hb1 = clk_bridge_hb();
        clk_bridge_get(&v1, &r1);

        dv = (long long)(v1 - v0);   /* VENC 时钟增量 | [ms] */
        dr = (long long)(r1 - r0);   /* rt_tick 增量 | [ms] */
        drift_us_s = (dr > 0) ? ((dv - dr) * 1000000 / dr) : 0;
        fps_x100 = (rt_uint32_t)(((rt_uint64_t)(hb1 - hb0)) * 100000 / 2000);

        rt_kprintf("[clk_verify] round%d: dvenc=%lldms drt=%lldms hb+%u "
                   "-> fps=%u.%02u, drift=%lldus/s\n",
                   i + 1, dv, dr, hb1 - hb0,
                   fps_x100 / 100, fps_x100 % 100, drift_us_s);
    }

    rt_kprintf("[clk_verify] done (fps = real encode rate; drift = venc clock skew vs rt_tick)\n");
}
SHELL_CMD_EXPORT(clk_verify, verify clock bridge (drift + real encode fps));

/* ---- msh 联调命令: 逐帧打印时钟桥锚点对采样 ----
 * clk_print [0|1]: 无参翻转, 带参设置; 打开后每个编码帧一行
 *   [clk] pts_us=... anchor(venc_ms=..., ats_rt_ms=...) hb=...
 * 与 nna_output 的时间戳列配套: NN 帧打印 t_cap_ms 即由这些锚点对换算而来,
 * 两路打印对照即可验证时钟桥线性映射 (帧间隔差应 < 1ms 量级) */
static void clk_print(int argc, char *argv[])
{
    if (argc >= 2)
        g_clk_print = atoi(argv[1]) ? 1 : 0;
    else
        g_clk_print = !g_clk_print;

    rt_kprintf("[clk_print] per-frame anchor print %s\n",
               g_clk_print ? "ON" : "OFF");
}
SHELL_CMD_EXPORT(clk_print, toggle per-frame VENC pts + anchor pair print);
