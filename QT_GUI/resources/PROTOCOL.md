# 协议契约（TCP 6000）

本文件为板端 `tcp_ctrl` 与 PC 端 Qt 应用之间的**唯一契约**。任何字段变更必须同步两边，并更新本文档。

## 字段定义

| 字段 | 类型 | 含义 | 对应算法状态 |
|---|---|---|---|
| `state` | string | `IDLE` / `CALIBRATING` / `READY` | 状态机 |
| `D` | int | 人到摄像头距离（mm） | `READY` 时有效 |
| `C` | int | 针孔常数（mm·px） | `READY` 时有效 |
| `W` | int | 人脸宽度（px） | `READY` 时有效 |
| `pairs` | int | 已配对样本数（0~5） | `CALIBRATING` 时有效 |
| `faces` | int | 最近一帧 NNA 检测目标数 | **`faces != 1` 时 `D`/`W` 为陈旧数据，PC 端判无效**（显示 `--`、曲线红色色带、饼图无效区） |
| `ts` | uint64 | `D` 对应模型帧的 rt_tick 时间戳（ms） | `READY` 时有效，其余 `0` |
| `fid` | uint64 | `D` 对应模型帧的 VPU 帧号 | `READY` 时有效，其余 `0` |
| `pts` | uint64 | 时钟桥锚点：最近编码帧 VENC 时钟（ms） | 恒有效（无编码帧时 `0`） |
| `ats` | uint64 | 时钟桥锚点：采样 `pts` 同一时刻的 rt_tick（ms） | 恒有效（无编码帧时 `0`） |
| `alert` | int | 板端过近告警：`0`/`1` | **预留字段，板端不实现**（提醒由 PC 本地判定完成） |
| `tts` | uint64 | 标定期最近一次配对：配对 TOF 样本的曝光中点 `t_mid`（rt_tick ms） | `CALIBRATING` 期间有配对时有效，其余状态 `0` |
| `tfid` | uint64 | 标定期最近一次配对：配对成功的 NN 帧 VPU 帧号 | `CALIBRATING` 期间有配对时有效，其余状态 `0` |
| `tdev` | uint64 | 标定期最近一次配对：`|NN 帧采集时刻 − TOF 曝光中点|`（ms） | `CALIBRATING` 期间有配对时有效，其余状态 `0`；越小越同步 |
| `twin` | uint32 | 标定期最近一次配对：TOF 曝光区间宽度 `first − trig`（ms） | `CALIBRATING` 期间有配对时有效，其余状态 `0`；供现场判断测距稳定度 |

无效的数值字段填 `-1`；`ts/fid/pts/ats/tts/tfid/tdev` 无效时填 `0`（u64 无符号），`twin` 无效时填 `0`（u32）。

### 帧级配对（时钟桥用法）

RTSP 流中每帧的 PTS（90kHz，源自板端 `VENC time_stamp(ms) × 9 / 100`）与 `ts` 不同时钟域。
用锚点对换算到同一时间轴后即可配对：

```
帧的算法时刻 rt_of(帧PTS_ms) = ats + (帧PTS_ms - pts)
显示规则: 当 rt_of(当前上屏帧) >= ts 时, 该 D 应上屏  → D 数字与画面严格同帧
```

### 标定配对质量（Time_Stamp Phase④）

`CALIBRATING` 期间板端每完成一次有效配对（同一次测距 + 同一次人脸检测），
随状态 JSON 同步上送最近一对样本的观测质量，PC 端用于显示配对可信度：

- `tts` / `tfid` / `tdev`：本次配对所选 TOF 曝光中点、NN 帧号、时刻偏差；
- `twin`：TOF 曝光区间宽度（实测稳定在 ~500ms 上下，单帧 560ms 事务 + UART 轮询抖动）。

PC 端展示建议：`配对 N/5 · 偏差 X ms · 帧 Y`（偏差 < 50ms 视为同帧合格；越大说明标定时人在动）。

## 消息格式

| 方向 | 格式 | 说明 |
|---|---|---|
| 板→PC | `{"state":"...","D":n,"C":n,"W":n,"faces":n,"pairs":n,"ts":n,"fid":n,"tts":n,"tfid":n,"tdev":n,"twin":n,"pts":n,"ats":n}\n` | 100ms 一帧，UTF-8，行终止符 `\n` |
| PC→板 | `KEY1\n` | 等效按下物理 SW2 |
| PC→板 | `CFG <dmin_mm> <hold_ms> <0|1>\n` | 过近提醒参数下发（如 `CFG 300 5000 1`）。**预留接口，板端不实现**（当前固件忽略未知行；提醒为 PC 本地判定） |
| PC→板 | `PING\n` | 心跳探测 |
| 板→PC | `PONG\n` | 心跳应答 |

PC 端解析必须容忍未知/缺失字段（向后兼容旧固件与 mock）。

## 示例

```json
{"state":"READY","D":1234,"C":56789,"W":96,"faces":1,"pairs":3,"ts":123456,"fid":7734,"tts":0,"tfid":0,"tdev":0,"twin":0,"pts":234567,"ats":234500}
{"state":"READY","D":1234,"C":56789,"W":96,"faces":0,"pairs":3,"ts":123456,"fid":7734,"tts":0,"tfid":0,"tdev":0,"twin":0,"pts":234567,"ats":234500}
{"state":"CALIBRATING","D":-1,"C":-1,"W":-1,"faces":1,"pairs":2,"ts":0,"fid":0,"tts":120003,"tfid":7301,"tdev":33,"twin":560,"pts":234567,"ats":234500}
{"state":"IDLE","D":-1,"C":-1,"W":-1,"faces":0,"pairs":0,"ts":0,"fid":0,"tts":0,"tfid":0,"tdev":0,"twin":0,"pts":0,"ats":0}
```

第 2 行示例：`faces=0`（无人脸），`D`/`W`/`ts`/`fid` 为冻结的陈旧值，PC 端必须按无效数据处理。
第 3 行示例：标定第 2 对样本（`pairs=2`），配对 `t_mid=120003ms` 的 TOF 与 `fid=7301` 的 NN 帧，
时刻偏差 33ms（半帧内，合格），TOF 曝光区间 560ms。

## 状态机

```
IDLE ──KEY1──▶ CALIBRATING ──pairs>=5──▶ READY
                  │                          │
                  └───── 3s 超时无新增 ────────┘
```

## 重连与心跳

- PC 端断线 3s 退避重连
- PC 每 5s 发 `PING\n`；10s 未收到 `PONG` 或状态数据判定死链，主动断开
