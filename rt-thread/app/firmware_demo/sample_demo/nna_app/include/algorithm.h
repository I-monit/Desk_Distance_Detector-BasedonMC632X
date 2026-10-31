#ifndef __ALGORITHM_H__
#define __ALGORITHM_H__

#include "type_def.h" /* FH_UINT32/FH_SINT32/FH_FLOAT/FH_VOID 等基础类型 (lib/inc/types) */

/*
 * 针孔模型个性化测距算法 (TOF锚定 + 视觉推算, 标定与检测一体)
 *
 * 针孔公式: P(脸部像素宽度) = F(焦距) * D(脸部真实宽度) / W(人到电脑距离)
 *           即 W = F*D / P; 令 C = F*D = W基准*P基准, 则 W = C / P
 * (2026-09-28 上板修正: 原方案 W = k*P 把P放在分子, 与针孔成像"越近像素越大"相反,
 *  实测人靠近时P增大而输出距离暴涨, 已反转为 W = C/P)
 *
 * 状态机:
 *   IDLE --alg_calib_start--> CALIBRATING --3s配对成功--> READY
 *                                 |                       |
 *                          alg_input_tof             alg_input_nn
 *                          alg_input_nn           -> W = C/P 实时推算
 *                                 |                       |
 *                            配对失败回IDLE         重新标定回CALIBRATING
 *
 * 基准标定: 按键触发3秒测量, 期间大模型检测目标平均算出像素宽度,
 *           与TOF距离按时间戳配对, 得到针孔常数 C = W基准×P基准 | [mm*pixel]
 * 检测过程: 每帧大模型输出经同一宽度定义换算P, 由W = C/P实时推算距离
 *
 * 数据接口:
 *   接口1 alg_input_nn : 大模型bbox输出(归一化), 标定期采样/检测期推算, 单一入口
 *   接口2 alg_input_tof: TOF测距结果(mm)输入, 仅标定期消费
 * 输出: C (alg_get_k, 返回字段名保留k为API兼容), 实时距离 (alg_get_distance)
 *
 * 注意: 两个接口的timestamp必须使用同一时钟基准(单位ms), 否则配对无效
 */

#define ALG_CALIB_DEFAULT_MS 4000 /* 基准测量默认时长4s (Time_Stamp: 560ms事务+100ms间隔, 3s仅4~5样本对5对临界) */
#define ALG_PAIR_TOLERANCE_MS 70  /* 配对容差: |NN帧采集时刻 − TOF曝光中点| (Time_Stamp:
                                   * 半帧33ms+中点模型余量; 原NN侧最近邻的200ms含两侧完成延迟偏差) */
#define ALG_MIN_PAIR_NUM 5        /* 标定成功所需最少配对样本数 */
#define ALG_MIN_FACE_CONF 0.50f   /* 目标框最低置信度 */
#define ALG_SAMPLE_MAX 128        /* 标定期采样环形缓冲容量 */

/* 返回码 */
#define ALG_OK 0
#define ALG_ERR_PARAM -1
#define ALG_ERR_NOT_READY -2 /* 未标定或标定中 */
#define ALG_ERR_NO_DATA -3   /* 尚无有效结果 */
#define ALG_ERR_CALIB_FAIL -4

typedef enum
{
    ALG_STATE_IDLE = 0,    /* 未标定, k无效 */
    ALG_STATE_CALIBRATING, /* 基准测量中 */
    ALG_STATE_READY,       /* k有效, 实时推算距离 */
} ALG_STATE;

/* 大模型输出的归一化bbox, 与NN_RESULT_DET_INFO的box对应 */
typedef struct
{
    FH_FLOAT x;    /* 矩形框x坐标 | [0.0 - 1.0] */
    FH_FLOAT y;    /* 矩形框y坐标 | [0.0 - 1.0] */
    FH_FLOAT w;    /* 矩形框宽度 | [0.0 - 1.0] */
    FH_FLOAT h;    /* 矩形框高度 | [0.0 - 1.0] */
    FH_FLOAT conf; /* 置信度 | [0.0 - 1.0] */
} ALG_BBOX;

/* 算法状态信息 */
typedef struct
{
    ALG_STATE state;      /* 当前状态 */
    FH_FLOAT k;           /* 针孔常数 C = W基准×P基准 = F×D | [mm*pixel], 字段名保留k为API兼容 */
    FH_UINT32 calib_ms;   /* 本次标定窗口时长 | [ms] */
    FH_UINT32 nn_samples; /* 标定期NN采样数 */
    FH_UINT32 tof_samples;/* 标定期TOF采样数 */
    FH_UINT32 pair_num;   /* 时间戳配对成功的样本对数 */
    FH_FLOAT face_width_px; /* 最近一次由bbox换算的脸部像素宽度 */
    FH_FLOAT distance_mm; /* 最近一次推算的人与电脑距离 | [mm] */
    FH_UINT64 update_ts;  /* 距离结果对应的模型帧时间戳 | [ms] (Time_Stamp Phase②: 帧采集时刻) */
    FH_UINT64 update_fid; /* 距离结果对应的模型帧帧号 (与RTSP帧同源计数) */
    FH_UINT32 face_num;   /* 最近一帧NNA检测到的目标数 (含低置信度框, 0=无人脸;
                           * PC端 faces != 1 时判定 D/W 为陈旧无效数据) */

    /* Time_Stamp Phase④: 最近一次成功配对的观测质量 (tcp_ctrl 组包 / PC 端显示用;
     * 无成功配对时为 0, 与 ts/fid 无效约定一致) */
    FH_UINT64 pair_last_tdev; /* |NN帧采集时刻 − TOF曝光中点| | [ms] */
    FH_UINT64 pair_last_tfid; /* 配对成功的 NN 帧 VPU 帧号 */
    FH_UINT64 pair_last_tts;  /* 配对的 TOF 样本曝光中点 | [ms] */
    FH_UINT32 pair_last_win;  /* TOF 曝光区间宽度 [trig, first] | [ms] */
} ALG_INFO;

/* 滤波/死区内部状态快照 (msh alg_dbg: 观测 W 抖动抑制链路 中值->EMA->自适应死区) */
typedef struct
{
    FH_UINT32 w_med_num;     /* 中值窗口已填充点数 (0~5) */
    FH_FLOAT  w_ema;         /* EMA 平滑后的 W | [px] */
    FH_UINT32 w_ema_ok;      /* EMA 是否已预热 */
    FH_UINT64 static_acc_ms; /* 静止信号累计时长 | [ms] */
    FH_UINT32 is_static;     /* 当前是否静止态 (死区收窄中) */
    FH_FLOAT  deadband_mm;   /* 最近一帧使用的输出死区 | [mm] (静止2/运动4) */
    FH_UINT32 nn_win;        /* 当前标定窗口内 NN 样本数 */
    FH_UINT32 tof_win;       /* 当前标定窗口内 TOF 样本数 */
    FH_UINT64 last_ts;       /* 最近一次输入时间戳 | [ms] */
} ALG_DBG;

/* 启动基准标定(按键触发), duration_ms为0时使用ALG_CALIB_DEFAULT_MS, 可重复调用重新标定 */
FH_SINT32 alg_calib_start(FH_UINT32 duration_ms);

/* 手动注入针孔常数C, 跳过5对采样直入 READY (msh alg_set_c, 会话级不持久化);
 * 合法量级 1000 ~ 500000 | [mm*px], 超范围返回 ALG_ERR_PARAM */
FH_SINT32 alg_calib_set(FH_FLOAT c_mm_px);

/* 接口1: 大模型输出. boxes为归一化bbox数组, frame_w/frame_h为检测通道像素尺寸,
 * frame_id为该帧VPU帧号(用于PC端与RTSP画面配对);
 * 内部选取置信度达标的最大目标换算像素宽度(标定/检测共用同一宽度定义);
 * 标定期采样配对, READY期每帧推算W = C / P */
FH_SINT32 alg_input_nn(FH_UINT64 timestamp, const ALG_BBOX *boxes, FH_SINT32 box_num,
                       FH_UINT32 frame_w, FH_UINT32 frame_h, FH_UINT64 frame_id);

/* 接口2: TOF测距结果输入, dist_mm为TOF距离 | [mm]; 仅标定期采样参与配对
 * (兼容保留: 零宽区间, ts 即观测时刻) */
FH_SINT32 alg_input_tof(FH_UINT64 timestamp, FH_FLOAT dist_mm);

/* 接口2扩展 (Time_Stamp Phase③): t_mid = 曝光区间中点, win_ms = 区间宽度
 * [trig, first] (atk_ms53l1m_get_last_interval); 内部展开 ts_b/ts_e 参与区间配对 */
FH_SINT32 alg_input_tof_ex(FH_UINT64 t_mid, FH_UINT32 win_ms, FH_FLOAT dist_mm);

/* 配对算法切换 (Time_Stamp Phase③ 影子对比): 0 = TOF侧区间配对(新), 1 = NN侧最近邻(旧);
 * 返回当前模式; msh alg_pair_mode 无参翻转/带参设置 */
FH_SINT32 alg_pair_legacy_mode(FH_SINT32 set);

/* 输出: 获取针孔常数C | [mm*pixel], 未完成标定返回ALG_ERR_NOT_READY */
FH_SINT32 alg_get_k(FH_FLOAT *k);

/* 输出: 最近一次实时推算的人与电脑距离 | [mm] */
FH_SINT32 alg_get_distance(FH_FLOAT *dist_mm);

/* 手动按针孔模型推算: W = C / P | [mm] */
FH_SINT32 alg_estimate_distance(FH_FLOAT face_width_px, FH_FLOAT *dist_mm);

/* 获取最近一次由bbox换算的脸部像素宽度 */
FH_SINT32 alg_get_face_width(FH_FLOAT *width_px);

/* 查询算法状态信息 */
FH_SINT32 alg_get_info(ALG_INFO *info);

/* 查询滤波/死区内部状态 (快照语义, 不改变算法状态) */
FH_SINT32 alg_get_dbg(ALG_DBG *dbg);

/* 兼容别名: 保持k_algorithm/distance_algorithm时期的调用面源码兼容 */
#define K_ALG_BBOX ALG_BBOX
#define k_alg_calib_start alg_calib_start
#define k_alg_input_tof alg_input_tof
#define k_alg_input_nn alg_input_nn
#define k_alg_get_k alg_get_k
#define dist_alg_get_distance alg_get_distance

#endif /* __ALGORITHM_H__ */
