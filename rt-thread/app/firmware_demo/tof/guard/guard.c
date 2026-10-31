/**
 ****************************************************************************************************
 * @file        guard.c
 * @brief       软件看门狗守护线程 (tof_alg + tcp_ctrl 自动重启, media 检测+告警; 纯软件层)
 *
 * 监护对象:
 *   tof_alg   tof/alg_glue.c        TOF 采样 + KEY1 标定 + k 算法喂料 (线程句柄 NULL 判死)
 *   tcp_ctrl  tcp_ctrl/tcp_ctrl.c   Qt GUI 状态/标定服务 (TCP 6000, 线程句柄 NULL 判死)
 *   media     媒体管线 VICAP->ISP->VENC->RTSP (心跳判死: clk_bridge 每编码帧 +1,
 *                                     冻结 >=10s -> ERROR 告警, **不自动重启**)
 *
 * 行为:
 *   服务死亡       -> 自动重启 (两次自动重启间隔 >= 30s 冷却, 防重启风暴)
 *   重启成功判据   -> 重启后存活 >= 60s, 连续失败计数清零
 *   连续失败 >= 5  -> GIVE UP: 停止自动重启, ERROR 日志 30s 一条提醒,
 *                     等待人工介入 (msh guard_retry 清计数并重新拉起, 不做硬件复位)
 *   心跳冻结       -> (线程活着但计数 >=10s 不涨) WARN 日志告警, 不强杀 (RTOS 无安全 kill)
 *                     注: tof_alg 标定期主循环最长阻塞约 5s, 阈值 2 轮(10s) 已规避误报
 *
 * media 为何只告警不自动重启 (2026-10-09 上板实测, 降级决策):
 *   svc_rtsp -q -> svc_rtsp -p 554 热重启后, 闭源媒体库触发 rt_sem_release 悬空句柄断言,
 *   编码恢复 (心跳继续) 但 RTSP 送达路径半死 (客户端握手成功收不到流) —— 心跳指标
 *   原理性覆盖不到"送达"; 自动重启只会把"干净死"变成"半死不活"。
 *   恢复手段: msh guard_retry media 手动重启 (实验性, 部分故障模式可能有效) 或断电。
 *
 * 不做: 硬件看门狗 (rtconfig 未启用 RT_USING_WDT) / 卡死线程强杀
 *
 * msh 命令: guard_stat 状态一览 | guard_retry [alg|tcp|media|all] 手动恢复
 * 启动:     user_main() 末尾显式调用 (demo_entry_rtt.c, 同 .rti_fn 段缺失原因)
 ****************************************************************************************************
 */

#include <rtthread.h>
#include <rttshell.h>
#include <string.h>

#include "tcp_ctrl.h"

extern int         alg_start(void);   /* tof/alg_glue.c */
extern rt_bool_t   tof_alg_alive(void);
extern rt_uint32_t tof_alg_hb(void);
extern rt_uint32_t clk_bridge_hb(void);   /* tcp_ctrl/clk_bridge.c: 媒体心跳 */
extern int msh_exec(char *cmd, rt_size_t length); /* shell/msh.c: FSymTab 查表执行 */

#define GUARD_PERIOD_MS        5000   /* 巡检周期 */
#define GUARD_RESTART_COOL_MS  30000  /* 两次自动重启最小间隔 */
#define GUARD_STABLE_MS        60000  /* 存活该时长才算重启成功, 清零失败计数 */
#define GUARD_MAX_FAIL         5      /* 连续失败上限, 超限 GIVE UP */
#define GUARD_ERR_EVERY_MS     30000  /* GIVE UP 后 ERROR 提醒间隔 */
#define GUARD_FROZEN_TICKS     2      /* 心跳连续 N 轮不涨才告警 (>=10s, 规避标定阻塞) */
#define GUARD_THREAD_STACK     2048
#define GUARD_THREAD_PRIO      28     /* 低于被监护服务 (tof_alg/tcp_ctrl 均 25) */

static int tcp_ctrl_start_default(void)
{
    return tcp_ctrl_start(TCP_CTRL_DEFAULT_PORT);   /* bootrun 同款默认端口 */
}

struct guard_svc
{
    const char   *name;
    rt_bool_t   (*alive)(void);
    rt_uint32_t (*hb)(void);
    int         (*start)(void);
    /* 运行状态 */
    rt_uint32_t   last_hb;
    rt_int64_t    last_restart_ms;
    rt_int64_t    alive_since_ms;   /* 死->活 沿时刻, 稳定判据起点 */
    rt_int64_t    last_err_log_ms;
    int           fail_cnt;         /* 连续失败 (重启后未活满 60s 即死 累计) */
    int           restart_cnt;      /* 累计重启次数 */
    int           frozen_cnt;       /* 心跳连续冻结轮数 */
    rt_bool_t     given_up;
    rt_bool_t     prev_alive;
};

static struct guard_svc g_svcs[] =
{
    { "tof_alg",  tof_alg_alive,  tof_alg_hb,  alg_start         },
    { "tcp_ctrl", tcp_ctrl_alive, tcp_ctrl_hb, tcp_ctrl_start_default},
};

#define GUARD_SVC_NUM ((int)(sizeof(g_svcs) / sizeof(g_svcs[0])))

static rt_thread_t g_guard_thread = RT_NULL;

/* ============================ 媒体管线监护 (心跳判死, 检测+告警) ============================
 * 心跳源: clk_bridge_update() 在每个 VENC 编码帧被调 (enc_config.c 取流循环),
 *         心跳冻结 = VICAP->ISP->VENC 全链路任一环节卡死 (如 TOF 热插拔电源毛刺)。
 * 只告警不自动重启: 2026-10-09 实测热重启触发闭源库 rt_sem 悬空释放断言,
 *         编码恢复但 RTSP 送达半死 (心跳覆盖不到送达路径), 自动重启反而掩盖故障。
 * 手动通道: guard_retry media 在独立线程跑 msh "svc_rtsp -q" + "svc_rtsp -p 554"
 *         (实验性; svc_rtsp_exit 在管线挂死时可能阻塞, 绝不能在 guard 线程内直接调)。 */

static volatile rt_uint8_t g_media_restarting = 0;   /* 手动重启序列已发出 */
static rt_uint32_t g_media_hb_at_restart   = 0;
static rt_uint32_t g_media_last_hb         = 0;
static rt_int64_t  g_media_last_err_log_ms = 0;
static int         g_media_frozen_ticks    = 0;
static int         g_media_restart_cnt     = 0;      /* 手动重启累计 */

#define MEDIA_RESTART_STACK 16384  /* msh_exec 内跑 svc_rtsp 完整初始化, 从宽 */

static void media_restart_thread_entry(void *param)
{
    static const char q[]  = "svc_rtsp -q";
    static const char st[] = "svc_rtsp -p 554";

    (void)param;
    rt_kprintf("[guard] media: restarting pipeline (svc_rtsp -q -> svc_rtsp -p 554)\n");
    msh_exec((char *)q, rt_strlen(q));
    rt_thread_mdelay(1000);
    msh_exec((char *)st, rt_strlen(st));
    rt_kprintf("[guard] media: restart sequence issued\n");
}

static int media_spawn_restart(void)
{
    rt_thread_t t = rt_thread_create("media_rst", media_restart_thread_entry, RT_NULL,
                                     MEDIA_RESTART_STACK, 25, 20);
    if (t == RT_NULL)
    {
        rt_kprintf("[guard] media: create restart thread failed\n");
        return -1;
    }
    rt_thread_startup(t);
    return 0;
}

static void guard_check_media(rt_int64_t now)
{
    rt_uint32_t hb = clk_bridge_hb();

    /* 手动重启后心跳恢复流动 -> 恢复正常 */
    if (g_media_restarting && hb != g_media_hb_at_restart)
    {
        g_media_restarting = 0;
        rt_kprintf("[guard] media: pipeline recovered\n");
    }

    if (hb == 0)
    {
        return;   /* 从未出帧: svc_rtsp 未启动或手动 -q 调试中, 不抢跑 (bootrun 负责拉起) */
    }

    /* 冻结计数: 连续 2 轮(>=10s) 不涨才判死, 单轮抖动不误报 */
    if (hb == g_media_last_hb)
    {
        if (g_media_frozen_ticks < GUARD_FROZEN_TICKS)
        {
            g_media_frozen_ticks++;
        }
    }
    else
    {
        g_media_frozen_ticks = 0;
    }
    g_media_last_hb = hb;

    if (g_media_frozen_ticks < GUARD_FROZEN_TICKS)
    {
        return;
    }

    /* 冻结告警 (30s 节流): 不自动重启, 恢复交给人工 —— 见文件头降级决策说明 */
    if ((now - g_media_last_err_log_ms) >= GUARD_ERR_EVERY_MS)
    {
        g_media_last_err_log_ms = now;
        rt_kprintf("[guard] ERROR: media pipeline frozen (no encoded frame >=%ds). "
                   "try 'guard_retry media' (experimental) or power cycle\n",
                   GUARD_FROZEN_TICKS * GUARD_PERIOD_MS / 1000);
    }
}

/* ============================ 单服务巡检 ============================ */

static void guard_check_svc(struct guard_svc *svc, rt_int64_t now)
{
    if (svc->alive())
    {
        rt_uint32_t hb = svc->hb();

        if (!svc->prev_alive)               /* 死->活 沿: 记稳定判据起点 */
        {
            svc->alive_since_ms = now;
            rt_kprintf("[guard] %s alive\n", svc->name);
        }

        /* 存活满稳定窗: 上一次重启判定成功, 连续失败清零 */
        if (svc->fail_cnt > 0 && !svc->given_up &&
            (now - svc->alive_since_ms) >= GUARD_STABLE_MS)
        {
            rt_kprintf("[guard] %s stable %ds, fail count cleared\n",
                       svc->name, GUARD_STABLE_MS / 1000);
            svc->fail_cnt = 0;
        }

        /* 心跳冻结告警: 只记录不强杀 */
        if (hb != 0 && hb == svc->last_hb)
        {
            if (++svc->frozen_cnt == GUARD_FROZEN_TICKS)
            {
                rt_kprintf("[guard] WARN: %s heartbeat frozen (thread stuck?)\n",
                           svc->name);
            }
        }
        else
        {
            svc->frozen_cnt = 0;
        }
        svc->last_hb   = hb;
        svc->prev_alive = RT_TRUE;
    }
    else
    {
        svc->frozen_cnt = 0;

        if (svc->prev_alive)                /* 活->死 沿 */
        {
            rt_kprintf("[guard] %s dead\n", svc->name);
            svc->prev_alive = RT_FALSE;
            svc->last_hb    = 0;
        }

        if (svc->given_up)                  /* 放弃后只提醒, 等人工 guard_retry */
        {
            if ((now - svc->last_err_log_ms) >= GUARD_ERR_EVERY_MS)
            {
                svc->last_err_log_ms = now;
                rt_kprintf("[guard] ERROR: %s still dead after %d restarts, "
                           "use 'guard_retry %s' to recover\n",
                           svc->name, GUARD_MAX_FAIL, svc->name);
            }
            return;
        }

        if (svc->last_restart_ms != 0 &&    /* 冷却窗内不重复重启 */
            (now - svc->last_restart_ms) < GUARD_RESTART_COOL_MS)
        {
            return;
        }

        rt_kprintf("[guard] %s dead, restarting (%d/%d)\n",
                   svc->name, svc->fail_cnt + 1, GUARD_MAX_FAIL);
        if (svc->start() == 0)
        {
            svc->restart_cnt++;
        }
        else
        {
            rt_kprintf("[guard] %s start() failed\n", svc->name);
        }
        svc->last_restart_ms = now;
        svc->fail_cnt++;

        if (svc->fail_cnt >= GUARD_MAX_FAIL)
        {
            svc->given_up        = RT_TRUE;
            svc->last_err_log_ms = now;
            rt_kprintf("[guard] ERROR: %s failed %d restarts, auto-recovery GAVE UP. "
                       "msh 'guard_retry %s' to recover manually\n",
                       svc->name, GUARD_MAX_FAIL, svc->name);
        }
    }
}

/* ============================ 守护线程 ============================ */

static void guard_thread_entry(void *param)
{
    int i;

    (void)param;

    rt_kprintf("[guard] watching tof_alg + tcp_ctrl + media (%dms period; tof/tcp auto-restart "
               "max %d then guard_retry; media detect+alert only)\n",
               GUARD_PERIOD_MS, GUARD_MAX_FAIL);

    while (1)
    {
        rt_int64_t now;

        rt_thread_mdelay(GUARD_PERIOD_MS);
        now = rt_tick_get_millisecond();
        for (i = 0; i < GUARD_SVC_NUM; i++)
        {
            guard_check_svc(&g_svcs[i], now);
        }
        guard_check_media(now);
    }
}

int guard_start(void)
{
    if (g_guard_thread != RT_NULL)
    {
        rt_kprintf("[guard] already running\n");
        return 0;
    }

    g_guard_thread = rt_thread_create("guard", guard_thread_entry, RT_NULL,
                                      GUARD_THREAD_STACK, GUARD_THREAD_PRIO, 20);
    if (g_guard_thread == RT_NULL)
    {
        rt_kprintf("[guard] create thread failed\n");
        return -1;
    }

    rt_thread_startup(g_guard_thread);
    return 0;
}

/* ============================ msh 命令 ============================ */

static void guard_stat(void)
{
    int i;
    rt_int64_t now = rt_tick_get_millisecond();

    for (i = 0; i < GUARD_SVC_NUM; i++)
    {
        struct guard_svc *s = &g_svcs[i];

        rt_kprintf("[guard] %-8s alive=%-3s hb=%-6u restarts=%-3d fails=%d/%d%s\n",
                   s->name,
                   s->alive() ? "yes" : "NO",
                   s->hb(),
                   s->restart_cnt,
                   s->fail_cnt, GUARD_MAX_FAIL,
                   s->given_up ? "  << GAVE UP, use guard_retry" : "");
    }
    rt_kprintf("[guard] %-8s hb=%-6u manual_rsts=%-3d%s\n",
               "media", clk_bridge_hb(), g_media_restart_cnt,
               (g_media_frozen_ticks >= GUARD_FROZEN_TICKS)
                   ? "  << FROZEN, guard_retry media / power cycle"
                   : (g_media_restarting ? "  (recovering)" : ""));
    rt_kprintf("[guard] uptime=%d s\n", (int)(now / 1000));
}
SHELL_CMD_EXPORT(guard_stat, show guard status of watched services);

static rt_bool_t svc_match(struct guard_svc *s, const char *key)
{
    if (!strcmp(key, "all"))                                  return RT_TRUE;
    if (!strcmp(key, s->name))                                return RT_TRUE;
    if (!strcmp(key, "alg") && !strcmp(s->name, "tof_alg"))   return RT_TRUE;
    if (!strcmp(key, "tcp") && !strcmp(s->name, "tcp_ctrl"))  return RT_TRUE;
    return RT_FALSE;
}

static void guard_retry(int argc, char *argv[])
{
    const char *key = (argc >= 2) ? argv[1] : "all";
    rt_int64_t now_ms = rt_tick_get_millisecond();
    int i;

    for (i = 0; i < GUARD_SVC_NUM; i++)
    {
        struct guard_svc *s = &g_svcs[i];

        if (!svc_match(s, key))
        {
            continue;
        }

        s->given_up = RT_FALSE;
        s->fail_cnt = 0;
        s->last_restart_ms = now_ms;   /* 手动重启后重起冷却计时 */

        if (!s->alive())
        {
            rt_kprintf("[guard] retry: restarting %s\n", s->name);
            if (s->start() == 0)
            {
                s->restart_cnt++;
            }
        }
        else
        {
            rt_kprintf("[guard] retry: %s already alive, counters cleared\n",
                       s->name);
        }
    }

    /* 媒体管线: 降级为检测+告警, guard_retry 提供实验性手动重启通道 */
    if (!strcmp(key, "all") || !strcmp(key, "media"))
    {
        rt_uint32_t hb = clk_bridge_hb();

        if (hb == 0 || hb == g_media_last_hb)   /* 未启动或冻结 -> 手动重启管线 */
        {
            rt_kprintf("[guard] retry: restarting media pipeline (experimental, "
                       "SDK hot-restart has known flaw)\n");
            if (media_spawn_restart() == 0)
            {
                g_media_restart_cnt++;
            }
            g_media_restarting      = 1;
            g_media_hb_at_restart   = hb;
            g_media_last_err_log_ms = now_ms;   /* 重置告警节流, 给重启留时间窗 */
        }
        else
        {
            rt_kprintf("[guard] retry: media running, skipped\n");
        }
    }
}
SHELL_CMD_EXPORT(guard_retry, manual retry: guard_retry [alg|tcp|media|all]);
