#include "nna_app/include/algorithm.h"
#include <pthread.h>
#include <string.h>
#include <stdio.h>

#define ALG_TAG "alg"

typedef struct
{
    FH_UINT64 ts; /* 采样时间戳 | [ms] */
    FH_FLOAT val; /* 采样值: 像素宽度或TOF距离mm */
} ALG_SAMPLE;

static pthread_mutex_t g_alg_lock = PTHREAD_MUTEX_INITIALIZER;
static ALG_STATE g_state = ALG_STATE_IDLE;
static FH_FLOAT g_k = 0.0f;
static FH_FLOAT g_face_width_px = 0.0f;
static FH_FLOAT g_distance_mm = 0.0f;
static FH_UINT64 g_update_ts = 0;
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

static FH_VOID alg_sample_push(ALG_SAMPLE *buf, FH_UINT32 *num, FH_UINT64 ts, FH_FLOAT val)
{
    if (*num >= ALG_SAMPLE_MAX)
    {
        memmove(&buf[0], &buf[1], (ALG_SAMPLE_MAX - 1) * sizeof(ALG_SAMPLE));
        *num = ALG_SAMPLE_MAX - 1;
    }

    buf[*num].ts = ts;
    buf[*num].val = val;
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

/* 标定窗口结束: 时间戳配对后取均值, 针孔常数 C = 平均TOF距离 × 平均像素宽度 */
static FH_VOID alg_calib_finish(FH_VOID)
{
    FH_UINT32 i, pair = 0;
    FH_FLOAT dist;
    FH_FLOAT width_sum = 0.0f;
    FH_FLOAT dist_sum = 0.0f;

    g_stat_nn = g_nn_num;
    g_stat_tof = g_tof_num;

    for (i = 0; i < g_nn_num; i++)
    {
        if (alg_tof_nearest(g_nn_buf[i].ts, &dist) >= 0)
        {
            width_sum += g_nn_buf[i].val;
            dist_sum += dist;
            pair++;
        }
    }

    g_stat_pair = pair;

    if (pair >= ALG_MIN_PAIR_NUM && width_sum > 0.0f)
    {
        g_k = (dist_sum / (FH_FLOAT)pair) * (width_sum / (FH_FLOAT)pair);
        g_distance_mm = 0.0f; /* 等待READY期首帧模型输出 */
        g_state = ALG_STATE_READY;
        printf("[%s] calib ok: C = %f mm*px, pairs = %u\n", ALG_TAG, g_k, pair);
    }
    else
    {
        g_k = 0.0f;
        g_state = ALG_STATE_IDLE;
        printf("[%s] calib failed: pairs = %u < %d\n", ALG_TAG, pair, ALG_MIN_PAIR_NUM);
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
    g_state = ALG_STATE_CALIBRATING;

    pthread_mutex_unlock(&g_alg_lock);

    printf("[%s] calib start: %u ms\n", ALG_TAG, duration_ms);

    return ALG_OK;
}

FH_SINT32 alg_input_nn(FH_UINT64 timestamp, const ALG_BBOX *boxes, FH_SINT32 box_num,
                       FH_UINT32 frame_w, FH_UINT32 frame_h)
{
    FH_FLOAT width_px;

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
            alg_sample_push(g_nn_buf, &g_nn_num, timestamp, width_px);

        alg_check_expire(timestamp);
    }
    else if (g_state == ALG_STATE_READY)
    {
        if (width_px >= 1.0f) /* 防0除: 针孔模型 W = C / P, P过小无意义 */
        {
            g_distance_mm = g_k / width_px;
            g_update_ts = timestamp;
        }
    }

    pthread_mutex_unlock(&g_alg_lock);

    return ALG_OK;
}

FH_SINT32 alg_input_tof(FH_UINT64 timestamp, FH_FLOAT dist_mm)
{
    if (dist_mm <= 0.0f)
        return ALG_ERR_PARAM;

    pthread_mutex_lock(&g_alg_lock);

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
            alg_sample_push(g_tof_buf, &g_tof_num, timestamp, dist_mm);

        alg_check_expire(timestamp);
    }

    pthread_mutex_unlock(&g_alg_lock);

    return ALG_OK;
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

    pthread_mutex_unlock(&g_alg_lock);

    return ALG_OK;
}
