#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "algstate.h"

#include <QMainWindow>
#include <QElapsedTimer>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class BoardLink;
class VideoPanel;
class DistancePlot;
class LogPanel;
class Heatmap;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onAlgStateChanged(const AlgState &state);
    void onConnStateChanged(ConnState s);
    void onLinkError(const QString &msg);
    void onCalibrateClicked();
    void onConnectClicked();
    void onVideoError(const QString &msg);
    void onVideoFrameTick(qint64 ptsUs);
    void onAlertCfgChanged();
    void onBinEdgesChanged();
    void onLogToggleClicked(bool checked);
    void onAnalyzeToggled(bool checked);

private:
    void setupStateLabels();
    void applyStateStyle(const QString &state);
    void applyConnProperty(const QString &tag);
    void applyBannerRole();
    void configureSplitters();
    void sendAlertCfg();
    void setAlertActive(bool on);
    void logInfo(const QString &msg);
    void logWarn(const QString &msg);
    void logError(const QString &msg);
    void showDImmediate(int d, int w);   /* 直显路径 (门控退化/flush 共用) */
    void flushPendingAll();              /* 队列全部直显清空 (异常退化用) */

    Ui::MainWindow *ui;
    BoardLink      *boardLink_    = nullptr;
    VideoPanel     *videoPanel_   = nullptr;
    DistancePlot   *distancePlot_ = nullptr;
    LogPanel       *logPanel_     = nullptr;
    Heatmap        *heatmap_      = nullptr;

    QString currentBoardIp_ = QStringLiteral("192.168.1.16");

    /* ---- 帧级门控 (Time_Stamp Phase①): D 随视频帧时刻上屏, 数字永不超前画面 ----
     * D {值, ts} 入队; 每帧上屏时按时钟桥公式 rt_of = ats + (帧PTS_ms − pts) 换算,
     * flush 所有 ts <= rt_of 的排队项 (PROTOCOL.md 帧级配对规则)。
     * 锚点缺失(旧固件) / PTS 跳变 时退化为即到即显, 行为与旧版一致 */
    struct PendingD { int D; int W; qint64 ts; qint64 enqMs; };
    QList<PendingD> pendingD_;        /* 待上屏 D 队列 (cap 8, 超限丢最旧保流动) */
    qint64 lastPts_     = 0;          /* 时钟桥锚点: 最近编码帧 VENC 时刻 | [ms] */
    qint64 lastAts_     = 0;          /* 时钟桥锚点: 采样 pts 同一时刻的 rt_tick | [ms] */
    qint64 lastEnqTs_   = 0;          /* 最近入队 D 的 ts (检测板端重启 ts 回绕) */
    qint64 lastFlushLogMs_ = 0;       /* 配对延迟日志节流 (qInfo, 5s 一行) */

    QElapsedTimer wall_;                  /* 本机单调时钟 (告警计时复用) */
    int           shownD_ = INT_MIN;      /* 当前已上屏 D (去重) */

    /* ---- 过近提醒 (PC 本地滞回判定为最终方案; CFG 下发为板端预留接口, 当前不实现) ----
     * 触发: READY 且 D < 阈值 持续 hold 秒; 解除: D >= 阈值+30mm 滞回或离开 READY/断线 */
    qint64 nearSinceMs_  = 0;     /* wall_ 单调时钟, 0 = 当前无持续过近 */
    bool   alertActive_  = false; /* 横幅当前是否显示 */

    /* ---- 饼图区间边界 (3 个上边界定义 4 个距离扇区, 改动即持久化并重建扇区/清零累计) ---- */
    bool updatingBins_ = false;   /* 防止递增约束触发的级联 valueChanged 重入 */
};

#endif
