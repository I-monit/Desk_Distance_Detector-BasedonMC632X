/**
 ****************************************************************************************************
 * @file        alg_glue.c
 * @brief       TOF 按键单次测距与 k 算法接线 (firmware_demo 联调 glue, 不属于任何模块分支)
 *
 * 取数路径: RT-Thread sensor 框架设备 "tof_ms53l1m" (tof/sensor/ 适配层),
 *           alg 层经 rt_device_read 取数, 与 ATK vendor 层解耦
 *
 * 行为 (KEY1 = TCP KEY1 命令 / msh alg_key, 无物理按键, 单次触发自动标定):
 *   IDLE/READY   按下 -> alg_calib_start(3s 窗口) + 窗口内自动连续 TOF 采样
 *                  (无需多次按键, Qt 端单击一次即可; 3s 窗口约采 4~5 个 TOF 样本)
 *   CALIBRATING  按下 -> 并入自动采样流程 (采样期间到达的按键事件结束后被丢弃)
 *   READY 期距离      由 nn 帧推算 (alg_input_nn, 接在 nna 检测回调), TOF 不参与
 *
 * 时间戳: rt_tick_get_millisecond() | 与 alg_input_nn 侧必须同一时钟基准
 * 启动:   user_main() 末尾显式调用 (demo_entry_rtt.c) —— fh BSP 链接脚本无 .rti_fn 段,
 *         INIT_APP_EXPORT 不会被执行, 仅保留作标准 RT-Thread 环境兼容;
 *         也可 msh 命令 "alg_start" 手动拉起
 ****************************************************************************************************
 */

#include <rtthread.h>
#include <rttshell.h>
#include <stdlib.h>                          /* atof (alg_set_c 参数解析) */
#include "tof_sensor.h"                      /* sensor 框架设备 "tof_ms53l1m" */
#include "ATK_MS53L1M/atk_ms53l1m_uart.h"    /* tof_probe 自检用 */
#include "KEY/key.h"
#include "nna_app/include/algorithm.h"

#define TOF_ALG_THREAD_STACK    4096
#define TOF_ALG_THREAD_PRIO     25
#define TOF_ALG_KEY_DEBOUNCE_MS 60  /* 按键去抖: 处理一次事件后丢弃残余抖动事件 */
#define TOF_ALG_AUTO_GAP_MS   100  /* 自动采样间隔: 单次 modbus 测距约 560ms, 3s 窗口约采 4~5 个样本 */
#define TOF_ALG_AUTO_TIMEOUT_MS (ALG_CALIB_DEFAULT_MS + 2000) /* 硬超时: 防 TOF 失效且无 NN 锚定时死循环 */

static rt_thread_t g_tof_alg_thread = RT_NULL;
static rt_device_t g_tof_dev        = RT_NULL;  /* sensor 框架设备 "tof_ms53l1m" 句柄 (惰性获取) */
static volatile rt_uint32_t g_tof_alg_hb = 0;  /* 心跳: 主循环每轮 +1, guard 冻结检测用 */

/* 惰性获取 sensor 设备并按轮询模式打开 (设备由 tof_sensor_init 注册, 永不注销) */
static rt_device_t tof_alg_device(void)
{
    if (g_tof_dev == RT_NULL)
    {
        g_tof_dev = rt_device_find("tof_ms53l1m");
        if (g_tof_dev != RT_NULL)
            rt_device_open(g_tof_dev, RT_DEVICE_FLAG_RDONLY);
    }
    return g_tof_dev;
}

/* 单次测距 (sensor 默认 TIMEBUDGET=140ms), 经 sensor 框架设备读取, 结果喂 k 算法
 * Time_Stamp Phase③: 时间戳 = 曝光区间中点 (t_trig+t_first)/2, 附带区间宽度,
 * 替代原"读取完成时刻"(偏晚 ~560ms); 影子日志供上板验证中点模型 */
static int tof_alg_measure_once(void)
{
    struct rt_sensor_data data;
    rt_device_t dev = tof_alg_device();
    rt_uint32_t trig_ms = 0, first_ms = 0;
    rt_uint64_t t_mid;
    rt_uint32_t win_ms;

    if (dev == RT_NULL)
    {
        rt_kprintf("[tof_alg] sensor device 'tof_ms53l1m' not found\n");
        return -1;
    }

    if (rt_device_read(dev, 0, &data, 1) != 1)
    {
        rt_kprintf("[tof_alg] measure failed\n");
        return -1;
    }

    tof_sensor_last_interval(&trig_ms, &first_ms);
    if (trig_ms == 0 || first_ms < trig_ms)
    {
        /* 区间异常 (vendor 层未打点): 退化为读完成时刻, 行为同旧版 */
        t_mid  = rt_tick_get_millisecond();
        win_ms = 0;
    }
    else
    {
        t_mid  = ((rt_uint64_t)trig_ms + (rt_uint64_t)first_ms) / 2;
        win_ms = first_ms - trig_ms;
    }

    alg_input_tof_ex(t_mid, win_ms, (float)data.data.proximity);
    rt_kprintf("[tof_alg] %d mm fed (t_mid=%llu win=%u [%u,%u])\n",
               data.data.proximity,
               (unsigned long long)t_mid, win_ms, trig_ms, first_ms);
    return 0;
}

static void tof_alg_thread_entry(void *param)
{
    ALG_INFO info;

    (void)param;

    if (tof_sensor_init() != 0)   /* vendor 层 uart1 init + sensor 设备注册 */
    {
        rt_kprintf("[tof_alg] ATK-MS53L1M init failed (check UART1 & pinmux)\n");
        rt_kprintf("[tof_alg] hint: pinctrl -d UART1 && alg_start\n");
        g_tof_alg_thread = RT_NULL;  /* 允许 alg_start 重试 (线程已退出) */
        return;
    }
    rt_kprintf("[tof_alg] TOF ready id=0x%04x (sensor device 'tof_ms53l1m')\n",
               tof_sensor_id());

    key_init();
    rt_kprintf("[tof_alg] KEY1 -> open %dms calib window + auto sample (single press)\n",
               ALG_CALIB_DEFAULT_MS);

    while (1)
    {
        g_tof_alg_hb++;    /* 心跳: 标定期内层采样最长阻塞约 5s, guard 阈值 10s 已规避误报 */

        uint8_t key = key_scan(0);

        if (key == KEY1_PRES)
        {
            if (alg_get_info(&info) == ALG_OK && info.state != ALG_STATE_CALIBRATING)
            {
                alg_calib_start(ALG_CALIB_DEFAULT_MS);
                rt_kprintf("[tof_alg] calib window open, auto sampling...\n");
            }

            /* 单击自动标定: 窗口内连续 TOF 采样, 直到窗口关闭 (READY/IDLE) 或硬超时.
             * 一次触发即可完成整次标定, 无需多次按键 */
            {
                rt_int64_t t0 = rt_tick_get_millisecond();

                while (alg_get_info(&info) == ALG_OK && info.state == ALG_STATE_CALIBRATING &&
                       (rt_tick_get_millisecond() - t0) < TOF_ALG_AUTO_TIMEOUT_MS)
                {
                    tof_alg_measure_once();
                    rt_thread_mdelay(TOF_ALG_AUTO_GAP_MS);
                }
            }

            /* 消抖: 丢弃标定期间到达的残余按键事件 (避免结束后立刻重开标定) */
            rt_thread_mdelay(TOF_ALG_KEY_DEBOUNCE_MS);
            key_scan(0);
        }

        rt_thread_mdelay(10);
    }
}

int alg_start(void)
{
    if (g_tof_alg_thread != RT_NULL)
    {
        rt_kprintf("[tof_alg] already running\n");
        return 0;
    }

    g_tof_alg_thread = rt_thread_create("tof_alg", tof_alg_thread_entry, RT_NULL,
                                        TOF_ALG_THREAD_STACK, TOF_ALG_THREAD_PRIO, 20);
    if (g_tof_alg_thread == RT_NULL)
    {
        rt_kprintf("[tof_alg] create thread failed\n");
        return -1;
    }

    rt_thread_startup(g_tof_alg_thread);
    return 0;
}

SHELL_CMD_EXPORT(alg_start, start tof + k-algorithm daemon);

/* ---- 软件看门狗 (tof/guard/guard.c) 读取接口 ---- */
rt_bool_t tof_alg_alive(void)   /* 线程句柄判活 (失败退出路径已置 NULL, alg_glue.c:62) */
{
    return (g_tof_alg_thread != RT_NULL) ? RT_TRUE : RT_FALSE;
}

rt_uint32_t tof_alg_hb(void)
{
    return g_tof_alg_hb;
}

INIT_APP_EXPORT(alg_start);

/* ============================ 联调调试命令 ============================
 * monitor_help : 列出全部项目调试/监控 msh 命令 (便于无 Qt 现场排障)
 * alg_state    : 直接 dump k 算法状态机 (不依赖串口打印时序)
 * alg_key      : 无物理按键时注入 KEY1 事件, 隔离 "GPIO 中断" 与 "状态机" 故障
 * tof_probe    : uart1 设备 find/open 自检 (结果码见打印; 用后建议复位再跑正式流程)
 * alg_set_c    : 手动注入针孔常数 C, 跳过 5 对采样直入 READY (会话级)
 * alg_dbg      : dump W 抖动抑制链路内部 (中值/EMA/自适应死区)
 * ==================================================================== */

/* monitor_help: 自建命令索引 (本文件仅是其中一部分, 跨文件汇总)
 * 注: 不含 RT-Thread 内建命令 (help/ps/free/list_thread 等), 那些走 msh help */
typedef struct { const char *name; const char *desc; } msh_cmd_help_t;

static const msh_cmd_help_t g_monitor_help[] = {
    /* tof / k 算法 (tof/alg_glue.c) */
    {"alg_start",      "start tof + k-algorithm daemon"},
    {"alg_state",      "dump k-algorithm state machine"},
    {"alg_key",        "inject KEY1 press event (full calib via key queue)"},
    {"alg_set_c",      "set pinhole constant C manually (skip sampling)"},
    {"alg_dbg",        "dump k-algorithm filter internals (median/EMA/deadband)"},
    {"alg_pair_mode",  "pairing algo: 0=TOF-side interval(new) 1=NN-side nearest(old)"},
    {"tof_probe",      "probe uart1 device find/open"},

    /* 时钟桥 (tcp_ctrl/clk_bridge.c) */
    {"clk_verify",     "verify clock bridge drift and real encode fps"},
    {"clk_print",      "toggle per-frame VENC pts + anchor pair print"},

    /* TCP 状态服务 (tcp_ctrl/tcp_ctrl.c) */
    {"svc_qt",         "start/stop TCP 6000 Qt GUI state service"},

    /* 软件看门狗 (tof/guard/guard.c) */
    {"guard_stat",     "show tof_alg/tcp_ctrl guard status"},
    {"guard_retry",    "manual retry: guard_retry [alg|tcp|media|all]"},

    /* NNA 检测 (sample_demo/nna_app/src/sample_nna_face_feature_locate.c) */
    {"nna_output",     "toggle nna detect print (fid/pts/t_cap, default off)"},

    /* 出流与开机 (startup/demo_entry_rtt.c) */
    {"svc_rtsp",       "start/stop RTSP pipeline (-p port)"},
    {"bootrun",        "autostart: ifconfig e0 + svc_rtsp -p 554 + svc_qt"},

    /* 设备/线程列表 (shell/cmd.c, 第6406eee 提交解锁 msh 可见) */
    {"list_device",    "list devices in system"},
    {"list_thread",    "list threads"},
};

static void monitor_help(void)
{
    const rt_uint32_t n = (rt_uint32_t)(sizeof(g_monitor_help) / sizeof(g_monitor_help[0]));
    rt_kprintf("---- monitor commands (project debug/monitor; msh help for builtins) ----\n");
    for (rt_uint32_t i = 0; i < n; i++)
    {
        rt_kprintf("  %-15s %s\n", g_monitor_help[i].name, g_monitor_help[i].desc);
    }
    rt_kprintf("---- end (total %u) ----\n", n);
}
SHELL_CMD_EXPORT(monitor_help, list project debug/monitor msh commands);
static void alg_state(void)
{
    ALG_INFO info;

    if (alg_get_info(&info) != ALG_OK)
    {
        rt_kprintf("[alg_state] no info\n");
        return;
    }
    /* C = W基准×P基准 (mm*px), 数值量级 6万, 整数显示即可 */
    rt_kprintf("[alg_state] state=%d pair=%u/%u nn=%u tof=%u C=%d D=%dmm W=%dpx\n",
               info.state, info.pair_num, (unsigned)ALG_MIN_PAIR_NUM,
               info.nn_samples, info.tof_samples,
               (int)(info.k + 0.5f),
               (int)(info.distance_mm + 0.5f),
               (int)(info.face_width_px + 0.5f));
}
SHELL_CMD_EXPORT(alg_state, dump k-algorithm state machine);

static void alg_key(void)
{
    key_push_event(KEY1_PRES);
    rt_kprintf("[alg_key] KEY1 event injected\n");
}
SHELL_CMD_EXPORT(alg_key, inject KEY1 press event for k-algorithm);

/* alg_pair_mode [0|1]: Time_Stamp Phase③ 配对算法影子切换
 * 0 (默认) = TOF 侧区间配对 (一对一, 采集时刻落在曝光区间内)
 * 1        = 旧 NN 侧最近邻 (一对多); 标定结束时两种结果都会打印对比 */
static void alg_pair_mode(int argc, char *argv[])
{
    int mode;

    if (argc >= 2)
        mode = alg_pair_legacy_mode(atoi(argv[1]));
    else
        mode = alg_pair_legacy_mode(-1);

    rt_kprintf("[alg_pair_mode] mode=%d (%s) -- calib finish prints both for compare\n",
               mode, mode ? "NN-side nearest (legacy)" : "TOF-side interval (new)");
}
SHELL_CMD_EXPORT(alg_pair_mode, alg_pair_mode);

static void tof_probe(void)
{
    int ret = atk_ms53l1m_uart_probe(115200);

    rt_kprintf("[tof_probe] result=%d (0=ok | -1=dev_not_found(check FH_USING_UART1) "
               "| -2=open_fail)\n", ret);
}
SHELL_CMD_EXPORT(tof_probe, probe uart1 device find/open);

/* alg_set_c <C_mm_px>: 联调时反复试不同 C 值, 免走 KEY1 完整标定流程 */
static void alg_set_c(int argc, char *argv[])
{
    ALG_INFO info;

    if (argc < 2)
    {
        if (alg_get_info(&info) == ALG_OK && info.k > 0.0f)
            rt_kprintf("[alg_set_c] state=%d C=%d (current)\n",
                       (int)info.state, (int)(info.k + 0.5f));
        else
            rt_kprintf("[alg_set_c] C not calibrated yet\n");
        rt_kprintf("[alg_set_c] usage: alg_set_c <C_mm_px>  (1000 ~ 500000, session-only)\n");
        return;
    }

    if (alg_calib_set((FH_FLOAT)atof(argv[1])) == ALG_OK)
        rt_kprintf("[alg_set_c] C=%s mm*px injected, state -> READY "
                   "(reset or KEY1 recalib to clear)\n", argv[1]);
    else
        rt_kprintf("[alg_set_c] invalid C '%s' (valid range 1000 ~ 500000 mm*px)\n",
                   argv[1]);
}
SHELL_CMD_EXPORT(alg_set_c, set pinhole constant C manually (skip sampling));

/* alg_dbg: W 抖动抑制链路 (中值->EMA->突变重置->自适应死区) 首次可直接观测 */
static void alg_dbg(void)
{
    ALG_INFO info;
    ALG_DBG dbg;

    if (alg_get_info(&info) != ALG_OK || alg_get_dbg(&dbg) != ALG_OK)
    {
        rt_kprintf("[alg_dbg] no info\n");
        return;
    }

    rt_kprintf("[alg_dbg] state=%d C=%d D=%dmm W=%dpx fid=%llu\n",
               (int)info.state, (int)(info.k + 0.5f),
               (int)(info.distance_mm + 0.5f), (int)(info.face_width_px + 0.5f),
               (unsigned long long)info.update_fid);
    rt_kprintf("[alg_dbg] med=%u/5 ema=%dpx warm=%u static=%u acc=%llums band=%dmm\n",
               dbg.w_med_num, (int)(dbg.w_ema + 0.5f), dbg.w_ema_ok,
               dbg.is_static, (unsigned long long)dbg.static_acc_ms,
               (int)dbg.deadband_mm);
    rt_kprintf("[alg_dbg] calib win: nn=%u tof=%u last_ts=%llums\n",
               dbg.nn_win, dbg.tof_win, (unsigned long long)dbg.last_ts);
}
SHELL_CMD_EXPORT(alg_dbg, dump k-algorithm filter internals);
