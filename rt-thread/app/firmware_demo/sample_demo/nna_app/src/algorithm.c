#include "nna_app/include/algorithm.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>

#define ALG_TAG "alg"

/* W 抖动抑制 (READY 期): 静止时NNA检测框宽抖动±2~4px经 D=K/W 反比放大 -> D 波动10~20mm
 * 链路: 5点中值(滤脉冲) -> EMA(降方差) -> 突变重置(保快速移动响应) -> 输出死区(消尾抖)
 * 预期: 稳态D波动 < 5mm */
#define ALG_W_MED_WIN     5      /* 中值窗口点数 */
#define ALG_W_EMA_ALPHA   0.35f  /* EMA系数: 小=平稳, 大=灵敏 */
#define ALG_W_JUMP_PX     8.0f   /* 中值与EMA偏差超此值视为快速移动, 重置EMA立即跟上 */
/* 自适应死区: 静止时数字真不动; 运动时死区4mm跟手
 * 静止判据: |ΔW_ema| < 1.5px 持续 ≥ 1s  -> 死区收到 2mm
 * 退出判据: 单帧 |ΔW_ema| > 3.0px       -> 立即回 4mm
 * (1.5~3.0 灰区保持当前态, 避免边界抖动来回切) */
#define ALG_W_STATIC_PX          1.5f   /* 进入静止阈值 | [px] */
#define ALG_W_MOVE_PX            3.0f   /* 退出静止阈值, 高于进入阈值构成磁滞 | [px] */
#define ALG_STATIC_HOLD_MS       1000   /* 持续静止多久切静止态 | [ms] */
#define ALG_D_DEADBAND_QUIET_MM  2.0f   /* 静止态死区 | [mm] */
#define ALG_D_DEADBAND_MOVE_MM   4.0f   /* 运动态死区 | [mm] */

/* 手动注入标定 (msh alg_set_c) 的合法量级: 典型针孔常数 ~6e4 mm*px */
#define ALG_CALIB_MANUAL_MIN     1000.0f
#define ALG_CALIB_MANUAL_MAX     500000.0f

typedef struct
{
    FH_UINT64 ts;    /* 采样时间戳 | [ms]: NN=帧采集时刻(Phase②), TOF=曝光区间中点(Phase③) */
    FH_UINT64 ts_b;  /* TOF专用: 曝光区间下界 trig | [ms] (NN样本恒等于 ts) */
    FH_UINT64 ts_e;  /* TOF专用: 曝光区间上界 first | [ms] (NN样本恒等于 ts) */
    FH_UINT64 fid;   /* NN专用: VPU 帧号 (配对质量上报 tcp_ctrl) */
    FH_UINT32 used;  /* NN专用: 本窗口内已被某 TOF 样本配走 (一对一约束, 新配对用) */
    FH_FLOAT  val;   /* 采样值: 像素宽度或TOF距离mm */
} ALG_SAMPLE;

static pthread_mutex_t g_alg_lock = PTHREAD_MUTEX_INITIALIZER;
static ALG_STATE g_state = ALG_STATE_IDLE;
static FH_FLOAT g_k = 0.0f;
static FH_FLOAT g_face_width_px = 0.0f;
static FH_FLOAT g_distance_mm = 0.0f;
static FH_UINT64 g_update_ts = 0;
static FH_UINT64 g_update_fid = 0;
static FH_UINT32 g_face_num = 0; /* 最近一帧NNA检测目标数 (无人脸=0, PC端判无效) */
static FH_UINT64 g_calib_start_ts = 0;
static FH_UINT32 g_calib_ms = ALG_CALIB_DEFAULT_MS;
static FH_SINT32 g_calib_anchored = 0;
static FH_UINT64 g_last_ts = 0;
static ALG_SAMPLE g_nn_buf[ALG_SAMPLE_MAX];
static FH_UINT32 g_nn_num = 0;
static ALG_SAMPLE g_tof_buf[ALG_SAMPLE_MAX];
static FH_UINT32 g_tof_num = 0;
static FH_UINT32 g_stat_nn = 0;
static FH_UINT32 g_stat_tof = 0;
static FH_UINT32 g_stat_pair = 0;

/* Time_Stamp Phase③: 配对模式与最近一次成功配对质量 (alg_pair_legacy_mode 切换) */
static volatile FH_SINT32 g_pair_legacy = 0; /* 0=TOF侧区间配对(新,默认) 1=NN侧最近邻(旧) */
static FH_UINT64 g_pair_last_tdev = 0;       /* |NN采集时刻 − TOF曝光中点| | [ms] */
static FH_UINT64 g_pair_last_tfid = 0;       /* 配对成功的 NN 帧 VPU 帧号 */
static FH_UINT64 g_pair_last_tts  = 0;       /* 配对的 TOF 样本曝光中点 | [ms] */
static FH_UINT32 g_pair_last_win  = 0;       /* TOF 曝光区间宽度 | [ms] */
static FH_FLOAT  g_w_med_buf[ALG_W_MED_WIN]; /* W 中值窗口 */
static FH_UINT32 g_w_med_num = 0;
static FH_FLOAT  g_w_ema = 0.0f;             /* W EMA 平滑值 */
static FH_UINT32 g_w_ema_ok = 0;             /* EMA 是否已预热 */
static FH_FLOAT  g_w_ema_prev = 0.0f;        /* 上一帧 W 滤波值 (算 ΔW 判动静) */
static FH_UINT64 g_static_acc_ms = 0;        /* 静止信号累计时长 | [ms] */
static FH_UINT32 g_is_static = 0;            /* 当前是否静止态 (死区收窄中) */
static FH_UINT64 g_dband_last_ts = 0;        /* 自适应死区上一帧时间戳 (算 dt) */
static FH_FLOAT  g_dband_last_mm = ALG_D_DEADBAND_MOVE_MM; /* 最近一帧使用的死区 (alg_dbg 观测) */

/* 标定/检测共用的目标选取: 置信度过滤后取面积最大目标(最近者),
 * 归一化宽度换算像素宽度, 保证两个阶段脸部宽度定义一致 */
static FH_SINT32 alg_pick_target(const ALG_BBOX *boxes, FH_SINT32 box_num,
                                 FH_UINT32 frame_w, FH_FLOAT *width_px)
{
    FH_SINT32 i;
    const ALG_BBOX *best = NULL;
    FH_FLOAT best_area = 0.0f;
    FH_FLOAT area;

    for (i = 0; i < box_num; i++)
    {
        if (boxes[i].conf < ALG_MIN_FACE_CONF)
            continue;

        area = boxes[i].w * boxes[i].h;
        if (area > best_area)
        {
            best_area = area;
            best = &boxes[i];
        }
    }

    if (best == NULL)
        return -1;

    *width_px = best->w * (FH_FLOAT)frame_w;

    return 0;
}

static FH_FLOAT alg_absf(FH_FLOAT v)
{
    return v >= 0.0f ? v : -v;
}

/* READY 期 W 抖动抑制: 5点中值 + EMA + 突变重置 (窗口未满时直接通过, ~5帧预热) */
static FH_FLOAT alg_w_filter(FH_FLOAT w_now)
{
    FH_FLOAT tmp[ALG_W_MED_WIN];
    FH_UINT32 i, j;
    FH_FLOAT med, t;

    if (g_w_med_num < ALG_W_MED_WIN)
    {
        g_w_med_buf[g_w_med_num++] = w_now;
        return w_now;
    }

    memmove(&g_w_med_buf[0], &g_w_med_buf[1], (ALG_W_MED_WIN - 1) * sizeof(FH_FLOAT));
    g_w_med_buf[ALG_W_MED_WIN - 1] = w_now;

    memcpy(tmp, g_w_med_buf, sizeof(tmp));
    for (i = 0; i < ALG_W_MED_WIN - 1; i++)
        for (j = 0; j < ALG_W_MED_WIN - 1 - i; j++)
            if (tmp[j] > tmp[j + 1])
            {
                t = tmp[j];
                tmp[j] = tmp[j + 1];
                tmp[j + 1] = t;
            }
    med = tmp[ALG_W_MED_WIN / 2];

    if (!g_w_ema_ok)
    {
        g_w_ema = med;
        g_w_ema_ok = 1;
    }
    else if (alg_absf(med - g_w_ema) > ALG_W_JUMP_PX)
    {
        g_w_ema = med; /* 快速移动: 重置EMA立即跟上, 不平滑 */
    }
    else
    {
        g_w_ema += ALG_W_EMA_ALPHA * (med - g_w_ema);
    }

    return g_w_ema;
}

/* 自适应死区状态机: 每帧调, 返回当前应使用的死区阈值 | [mm]
 * 进入静止需持续 ALG_STATIC_HOLD_MS, 退出静止单帧立即 (磁滞防抖) */
static FH_FLOAT alg_deadband_get(FH_FLOAT w_f, FH_UINT64 ts, FH_UINT32 *is_static_out)
{
    FH_FLOAT dw = alg_absf(w_f - g_w_ema_prev);
    FH_UINT64 dt = 66; /* 默认一帧 66ms @15fps */

    if (g_dband_last_ts && ts > g_dband_last_ts)
        dt = ts - g_dband_last_ts;
    g_dband_last_ts = ts;

    if (dw > ALG_W_MOVE_PX)
    {
        g_static_acc_ms = 0; /* 真人明显移动: 立即退出静止 */
        g_is_static = 0;
    }
    else if (dw < ALG_W_STATIC_PX)
    {
        g_static_acc_ms += dt; /* 稳定信号: 累计静止时长 */
        if (g_static_acc_ms >= ALG_STATIC_HOLD_MS)
            g_is_static = 1;
    }
    /* else: 1.5~3.0 灰区, 保持当前态与累计值, 避免边界抖动来回切 */

    g_w_ema_prev = w_f;
    *is_static_out = g_is_static;
    g_dband_last_mm = g_is_static ? ALG_D_DEADBAND_QUIET_MM : ALG_D_DEADBAND_MOVE_MM;
    return g_dband_last_mm;
}

static FH_VOID alg_sample_push(ALG_SAMPLE *buf, FH_UINT32 *num, FH_UINT64 ts, FH_FLOAT val)
{
    if (*num >= ALG_SAMPLE_MAX)
    {
        memmove(&buf[0], &buf[1], (ALG_SAMPLE_MAX - 1) * sizeof(ALG_SAMPLE));
        *num = ALG_SAMPLE_MAX - 1;
    }

    buf[*num].ts   = ts;
    buf[*num].ts_b = ts;   /* 区间字段默认零宽, TOF 侧 push 后覆写 */
    buf[*num].ts_e = ts;
    buf[*num].fid  = 0;
    buf[*num].used = 0;
    buf[*num].val  = val;
    (*num)++;
}

/* 在TOF采样缓冲中查找与ts时间戳最近的样本, 偏差超过容差视为未配对 */
static FH_SINT32 alg_tof_nearest(FH_UINT64 ts, FH_FLOAT *val)
{
    FH_UINT32 i;
    FH_UINT64 diff;
    FH_UINT64 best_diff = (FH_UINT64)-1;
    FH_SINT32 best_idx = -1;

    for (i = 0; i < g_tof_num; i++)
    {
        diff = g_tof_buf[i].ts > ts ? (g_tof_buf[i].ts - ts) : (ts - g_tof_buf[i].ts);
        if (diff < best_diff)
        {
            best_diff = diff;
            best_idx = (FH_SINT32)i;
        }
    }

    if (best_idx < 0 || best_diff > ALG_PAIR_TOLERANCE_MS)
        return -1;

    *val = g_tof_buf[best_idx].val;
    return best_idx;
}

/* Time_Stamp Phase③: TOF 侧区间配对 —— 对每条 TOF 样本, 在 NN 缓冲中回溯选取
 * "采集时刻落在曝光区间 [ts_b, ts_e] 内、离中点 ts 最近且未被配走"的帧 (一对一);
 * 区间内无帧时 (人脸丢失帧间隙) 按 ALG_PAIR_TOLERANCE_MS 容差最近邻回退。
 * 与旧 NN 侧最近邻的本质区别: 配对时刻约束在真实观测时刻上, 且一条 TOF 读数只配一帧 */
static FH_SINT32 alg_nn_pick_for_tof(FH_UINT32 tof_idx, FH_UINT64 *tdev)
{
    FH_UINT32 j;
    FH_SINT32 best = -1;
    FH_UINT64 best_diff = (FH_UINT64)-1;
    const ALG_SAMPLE *t = &g_tof_buf[tof_idx];

    for (j = 0; j < g_nn_num; j++)
    {
        FH_UINT64 diff;

        if (g_nn_buf[j].used)
            continue;
        if (g_nn_buf[j].ts < t->ts_b || g_nn_buf[j].ts > t->ts_e)
            continue;
        diff = g_nn_buf[j].ts > t->ts ? (g_nn_buf[j].ts - t->ts) : (t->ts - g_nn_buf[j].ts);
        if (diff < best_diff)
        {
            best_diff = diff;
            best = (FH_SINT32)j;
        }
    }

    if (best < 0) /* 区间内无帧: 容差回退 */
    {
        for (j = 0; j < g_nn_num; j++)
        {
            FH_UINT64 diff;

            if (g_nn_buf[j].used)
                continue;
            diff = g_nn_buf[j].ts > t->ts ? (g_nn_buf[j].ts - t->ts) : (t->ts - g_nn_buf[j].ts);
            if (diff <= ALG_PAIR_TOLERANCE_MS && diff < best_diff)
            {
                best_diff = diff;
                best = (FH_SINT32)j;
            }
        }
    }

    if (best >= 0)
        *tdev = best_diff;
    return best;
}

/* 标定窗口结束: 时间戳配对后取均值, 针孔常数 C = 平均TOF距离 × 平均像素宽度
 * Time_Stamp Phase③: 默认走 TOF 侧区间配对 (一对一); 旧 NN 侧最近邻影子并算并打印,
 * msh alg_pair_mode 1 可切回旧行为 (联调对比期保险) */
static FH_VOID alg_calib_finish(FH_VOID)
{
    FH_UINT32 i;
    FH_UINT32 pair = 0, pair_l = 0;
    FH_FLOAT dist;
    FH_FLOAT width_sum = 0.0f, dist_sum = 0.0f;
    FH_FLOAT width_sum_l = 0.0f, dist_sum_l = 0.0f;
    FH_FLOAT c_new, c_legacy;
    FH_UINT32 use_pair;
    FH_FLOAT use_c;

    g_stat_nn = g_nn_num;
    g_stat_tof = g_tof_num;

    /* 新配对 (默认): 每个 TOF 样本回溯选一帧, 记录配对质量 */
    for (i = 0; i < g_tof_num; i++)
    {
        FH_UINT64 tdev = 0;
        FH_SINT32 idx = alg_nn_pick_for_tof(i, &tdev);

        if (idx >= 0)
        {
            g_nn_buf[idx].used = 1;
            width_sum += g_nn_buf[idx].val;
            dist_sum  += g_tof_buf[i].val;
            g_pair_last_tdev = tdev;
            g_pair_last_tfid = g_nn_buf[idx].fid;
            g_pair_last_tts  = g_tof_buf[i].ts;
            g_pair_last_win  = (FH_UINT32)(g_tof_buf[i].ts_e - g_tof_buf[i].ts_b);
            pair++;
        }
    }
    c_new = (pair > 0) ? (dist_sum / (FH_FLOAT)pair) * (width_sum / (FH_FLOAT)pair) : 0.0f;

    /* 旧配对 (影子): NN 侧最近邻一对多; 时间戳同为采集时刻/曝光中点, 差异在配对方向 */
    for (i = 0; i < g_nn_num; i++)
    {
        if (alg_tof_nearest(g_nn_buf[i].ts, &dist) >= 0)
        {
            width_sum_l += g_nn_buf[i].val;
            dist_sum_l += dist;
            pair_l++;
        }
    }
    c_legacy = (pair_l > 0) ? (dist_sum_l / (FH_FLOAT)pair_l) * (width_sum_l / (FH_FLOAT)pair_l) : 0.0f;

    printf("[%s] calib finish: new pairs=%u C=%.1f | legacy pairs=%u C=%.1f (mode=%s)\n",
           ALG_TAG, pair, c_new, pair_l, c_legacy,
           g_pair_legacy ? "legacy" : "new");

    use_pair = g_pair_legacy ? pair_l : pair;
    use_c    = g_pair_legacy ? c_legacy : c_new;
    g_stat_pair = use_pair;

    if (use_pair >= ALG_MIN_PAIR_NUM && use_c > 0.0f)
    {
        g_k = use_c;
        g_distance_mm = 0.0f; /* 等待READY期首帧模型输出 */
        g_state = ALG_STATE_READY;
        printf("[%s] calib ok: C = %f mm*px, pairs = %u\n", ALG_TAG, g_k, use_pair);
    }
    else
    {
        g_k = 0.0f;
        g_state = ALG_STATE_IDLE;
        printf("[%s] calib failed: pairs = %u < %d\n", ALG_TAG, use_pair, ALG_MIN_PAIR_NUM);
    }
}

static FH_VOID alg_check_expire(FH_UINT64 ts)
{
    if (g_state == ALG_STATE_CALIBRATING && g_calib_anchored &&
        ts >= g_calib_start_ts && (ts - g_calib_start_ts) >= g_calib_ms)
    {
        alg_calib_finish();
    }
}

FH_SINT32 alg_calib_start(FH_UINT32 duration_ms)
{
    if (duration_ms == 0)
        duration_ms = ALG_CALIB_DEFAULT_MS;

    pthread_mutex_lock(&g_alg_lock);

    g_calib_ms = duration_ms;
    g_calib_start_ts = 0;
    g_calib_anchored = 0;
    g_nn_num = 0;
    g_tof_num = 0;
    g_k = 0.0f;
    g_distance_mm = 0.0f;
    g_w_med_num = 0; /* 滤波状态随标定重置, 从当前W重新收敛 (~5帧预热) */
    g_w_ema_ok = 0;
    g_w_ema_prev = 0.0f; /* 自适应死区状态同样重置, 新周期从运动态起步 */
    g_static_acc_ms = 0;
    g_is_static = 0;
    g_dband_last_ts = 0;
    g_state = ALG_STATE_CALIBRATING;

    pthread_mutex_unlock(&g_alg_lock);

    printf("[%s] calib start: %u ms\n", ALG_TAG, duration_ms);

    return ALG_OK;
}

/* 手动注入针孔常数 (msh alg_set_c): 跳过 5 对采样直入 READY, 会话级不持久化
 * (重启或重新 KEY1 标定即恢复); 滤波状态复位集与 alg_calib_start 一致 */
FH_SINT32 alg_calib_set(FH_FLOAT c_mm_px)
{
    if (c_mm_px < ALG_CALIB_MANUAL_MIN || c_mm_px > ALG_CALIB_MANUAL_MAX)
        return ALG_ERR_PARAM;

    pthread_mutex_lock(&g_alg_lock);

    g_k = c_mm_px;
    g_distance_mm = 0.0f; /* 等待READY期首帧模型输出 */
    g_w_med_num = 0;      /* 滤波状态随注入重置, 从当前W重新收敛 (~5帧预热) */
    g_w_ema_ok = 0;
    g_w_ema_prev = 0.0f;
    g_static_acc_ms = 0;
    g_is_static = 0;
    g_dband_last_ts = 0;
    g_state = ALG_STATE_READY;

    pthread_mutex_unlock(&g_alg_lock);

    printf("[%s] calib set: C = %f mm*px (manual, session-only)\n", ALG_TAG, c_mm_px);

    return ALG_OK;
}

FH_SINT32 alg_input_nn(FH_UINT64 timestamp, const ALG_BBOX *boxes, FH_SINT32 box_num,
                       FH_UINT32 frame_w, FH_UINT32 frame_h, FH_UINT64 frame_id)
{
    FH_FLOAT width_px;

    /* 人脸计数先于一切早退路径记录: 无人脸帧(0)后 D/W 不再更新,
     * PC 端依据 faces != 1 判定随后推送的 D/W 为陈旧无效数据 */
    if (box_num < 0)
        box_num = 0;
    pthread_mutex_lock(&g_alg_lock);
    g_face_num = (FH_UINT32)box_num;
    pthread_mutex_unlock(&g_alg_lock);

    if (box_num > 0)
    {
        if (boxes == NULL || frame_w == 0 || frame_h == 0)
            return ALG_ERR_PARAM;

        if (alg_pick_target(boxes, box_num, frame_w, &width_px) < 0)
            return ALG_OK; /* 本帧无有效目标, 不采样不推算 */
    }
    else
    {
        return ALG_OK;
    }

    pthread_mutex_lock(&g_alg_lock);

    g_face_width_px = width_px;
    if (timestamp > g_last_ts)
        g_last_ts = timestamp;

    if (g_state == ALG_STATE_CALIBRATING)
    {
        if (!g_calib_anchored)
        {
            g_calib_start_ts = timestamp;
            g_calib_anchored = 1;
        }

        if (timestamp >= g_calib_start_ts && (timestamp - g_calib_start_ts) < g_calib_ms)
        {
            alg_sample_push(g_nn_buf, &g_nn_num, timestamp, width_px);
            g_nn_buf[g_nn_num - 1].fid = frame_id; /* Time_Stamp Phase③: 配对质量上报 */
        }

        alg_check_expire(timestamp);
    }
    else if (g_state == ALG_STATE_READY)
    {
        if (width_px >= 1.0f) /* 防0除: 针孔模型 W = C / P, P过小无意义 */
        {
            FH_FLOAT w_f = alg_w_filter(width_px);
            FH_FLOAT d_raw = g_k / w_f;
            FH_UINT32 is_static;
            FH_FLOAT deadband = alg_deadband_get(w_f, timestamp, &is_static);

            g_face_width_px = w_f; /* 对外W与D同源, PC曲线/alg_state打印同步变稳 */
            g_update_ts = timestamp;
            g_update_fid = frame_id;

            /* 自适应死区: 静止时2mm数字真不动; 运动时4mm跟手 */
            if (g_distance_mm <= 0.0f || alg_absf(d_raw - g_distance_mm) >= deadband)
                g_distance_mm = d_raw;
        }
    }

    pthread_mutex_unlock(&g_alg_lock);

    return ALG_OK;
}

FH_SINT32 alg_input_tof(FH_UINT64 timestamp, FH_FLOAT dist_mm)
{
    return alg_input_tof_ex(timestamp, 0, dist_mm); /* 兼容包装: 零宽区间 */
}

/* Time_Stamp Phase③: t_mid = 曝光区间中点, win_ms = [trig, first] 宽度;
 * 区间展开存入 ts_b/ts_e 供 TOF 侧区间配对 (alg_nn_pick_for_tof) */
FH_SINT32 alg_input_tof_ex(FH_UINT64 t_mid, FH_UINT32 win_ms, FH_FLOAT dist_mm)
{
    if (dist_mm <= 0.0f)
        return ALG_ERR_PARAM;

    pthread_mutex_lock(&g_alg_lock);

    if (t_mid > g_last_ts)
        g_last_ts = t_mid;

    if (g_state == ALG_STATE_CALIBRATING)
    {
        if (!g_calib_anchored)
        {
            g_calib_start_ts = t_mid;
            g_calib_anchored = 1;
        }

        if (t_mid >= g_calib_start_ts && (t_mid - g_calib_start_ts) < g_calib_ms)
        {
            alg_sample_push(g_tof_buf, &g_tof_num, t_mid, dist_mm);
            g_tof_buf[g_tof_num - 1].ts_b = t_mid - win_ms / 2;
            g_tof_buf[g_tof_num - 1].ts_e = g_tof_buf[g_tof_num - 1].ts_b + win_ms;
        }

        alg_check_expire(t_mid);
    }

    pthread_mutex_unlock(&g_alg_lock);

    return ALG_OK;
}

/* Time_Stamp Phase③: 配对模式切换 (msh alg_pair_mode); set<0 仅查询 */
FH_SINT32 alg_pair_legacy_mode(FH_SINT32 set)
{
    if (set >= 0)
        g_pair_legacy = set ? 1 : 0;
    return g_pair_legacy;
}

FH_SINT32 alg_get_k(FH_FLOAT *k)
{
    FH_SINT32 ret = ALG_OK;

    if (k == NULL)
        return ALG_ERR_PARAM;

    pthread_mutex_lock(&g_alg_lock);

    alg_check_expire(g_last_ts);

    if (g_state != ALG_STATE_READY || g_k <= 0.0f)
        ret = ALG_ERR_NOT_READY;
    else
        *k = g_k;

    pthread_mutex_unlock(&g_alg_lock);

    return ret;
}

FH_SINT32 alg_get_distance(FH_FLOAT *dist_mm)
{
    FH_SINT32 ret = ALG_OK;

    if (dist_mm == NULL)
        return ALG_ERR_PARAM;

    pthread_mutex_lock(&g_alg_lock);

    alg_check_expire(g_last_ts);

    if (g_state != ALG_STATE_READY)
        ret = ALG_ERR_NOT_READY;
    else if (g_distance_mm <= 0.0f)
        ret = ALG_ERR_NO_DATA;
    else
        *dist_mm = g_distance_mm;

    pthread_mutex_unlock(&g_alg_lock);

    return ret;
}

FH_SINT32 alg_estimate_distance(FH_FLOAT face_width_px, FH_FLOAT *dist_mm)
{
    FH_SINT32 ret;

    if (dist_mm == NULL || face_width_px <= 0.0f)
        return ALG_ERR_PARAM;

    ret = alg_get_k(dist_mm);
    if (ret != ALG_OK)
        return ret;

    *dist_mm /= face_width_px;

    return ALG_OK;
}

FH_SINT32 alg_get_face_width(FH_FLOAT *width_px)
{
    if (width_px == NULL)
        return ALG_ERR_PARAM;

    pthread_mutex_lock(&g_alg_lock);
    *width_px = g_face_width_px;
    pthread_mutex_unlock(&g_alg_lock);

    return ALG_OK;
}

FH_SINT32 alg_get_info(ALG_INFO *info)
{
    if (info == NULL)
        return ALG_ERR_PARAM;

    pthread_mutex_lock(&g_alg_lock);

    alg_check_expire(g_last_ts);

    info->state = g_state;
    info->k = g_k;
    info->calib_ms = g_calib_ms;
    info->nn_samples = g_stat_nn;
    info->tof_samples = g_stat_tof;
    info->pair_num = g_stat_pair;
    info->face_width_px = g_face_width_px;
    info->distance_mm = g_distance_mm;
    info->update_ts = g_update_ts;
    info->update_fid = g_update_fid;
    info->face_num = g_face_num;
    info->pair_last_tdev = g_pair_last_tdev; /* Time_Stamp Phase④ */
    info->pair_last_tfid = g_pair_last_tfid;
    info->pair_last_tts  = g_pair_last_tts;
    info->pair_last_win  = g_pair_last_win;

    pthread_mutex_unlock(&g_alg_lock);

    return ALG_OK;
}

FH_SINT32 alg_get_dbg(ALG_DBG *dbg)
{
    if (dbg == NULL)
        return ALG_ERR_PARAM;

    pthread_mutex_lock(&g_alg_lock);

    dbg->w_med_num = g_w_med_num;
    dbg->w_ema = g_w_ema;
    dbg->w_ema_ok = g_w_ema_ok;
    dbg->static_acc_ms = g_static_acc_ms;
    dbg->is_static = g_is_static;
    dbg->deadband_mm = g_dband_last_mm;
    dbg->nn_win = g_nn_num;
    dbg->tof_win = g_tof_num;
    dbg->last_ts = g_last_ts;

    pthread_mutex_unlock(&g_alg_lock);

    return ALG_OK;
}
