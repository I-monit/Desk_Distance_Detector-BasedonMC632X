#ifndef ALGSTATE_H
#define ALGSTATE_H

#include <QString>
#include <QMetaType>
#include <QtGlobal>

enum class ConnState {
    Disconnected,
    Connecting,
    Connected
};

struct AlgState {
    QString state = QStringLiteral("IDLE");
    int     D     = -1;
    int     C     = -1;
    int     W     = -1;
    int     pairs = 0;
    int     fps   = 0;
    int     faces = 0;  /* 最近一帧 NNA 检测目标数 (板端新固件 "faces" 字段)
                         * faces != 1 时 D/W 为陈旧数据, 判定无效: 显示 -- / 曲线红色竖直色带 / 饼图无效区
                         * 缺省 0 = 无效 (两端同步升级, 不做旧固件兼容) */

    /* 帧级配对字段 (板端 Qt_GUI 固件提供, 旧固件/mock 缺省为 0)
     * ts/fid = D 对应模型帧的 rt_tick 时间戳(ms)与帧号
     * pts/ats = VENC时钟(ms) <-> rt_tick(ms) 时钟桥锚点对:
     *          任意 RTSP 帧的算法时刻 rt_of(PTS) = ats + (PTS_ms - pts) */
    quint64 ts  = 0;
    quint64 fid = 0;
    quint64 pts = 0;
    quint64 ats = 0;

    /* 标定期配对质量 (板端 Time_Stamp 固件 CALIBRATING 期提供, 其余状态/旧固件缺省 0)
     * tts  = 最近配对 TOF 样本曝光中点 | [ms]
     * tfid = 配对成功的 NN 帧 VPU 帧号
     * tdev = |NN 帧采集时刻 − TOF 曝光中点| | [ms] (配对可信度, 越小越同步)
     * twin = TOF 曝光区间宽度 | [ms] */
    quint64 tts  = 0;
    quint64 tfid = 0;
    quint64 tdev = 0;
    quint32 twin = 0;

    /* 板端过近告警标志 (新固件 JSON "alert" 字段; 旧固件/mock 缺省 0)
     * PC 横幅为本地判定不依赖此字段, alert 仅作板端状态回显 */
    int     alert = 0;

    bool hasPairingInfo() const { return ts > 0; }
    bool isFaceValid() const { return faces == 1; }

    bool isValid() const { return !state.isEmpty(); }
    bool isCalibrating() const { return state == QStringLiteral("CALIBRATING"); }
    bool isReady() const { return state == QStringLiteral("READY"); }
    bool isIdle() const { return state == QStringLiteral("IDLE"); }
};

Q_DECLARE_METATYPE(AlgState)

#endif
