#include "mainwindow.h"
#include "boardlink.h"
#include "videopanel.h"
#include "distanceplot.h"
#include "logpanel.h"
#include "heatmap.h"
#include "ui_mainwindow.h"

#include <QMessageBox>
#include <QApplication>
#include <QTimer>
#include <QHostAddress>
#include <QDateTime>
#include <QDebug>
#include <QSettings>
#include <QSplitter>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , boardLink_(new BoardLink(this))
    , videoPanel_(new VideoPanel(this))
{
    ui->setupUi(this);

    wall_.start(); /* 告警计时复用 */

    applyBannerRole();
    configureSplitters();

    ui->videoContainer->layout()->addWidget(videoPanel_);

    /* 距离曲线 / 分布分析: 左半图表, 右半热力图 + 分析按钮 */
    distancePlot_ = new DistancePlot(this);
    ui->chartLeft->layout()->addWidget(distancePlot_);

    heatmap_ = new Heatmap(this);
    ui->heatmapContainer->layout()->addWidget(heatmap_);

    /* 饼图区间边界可输入 (默认 299/499/700), QSettings 记忆, 改动即重建扇区并清零累计 */
    {
        QSettings cfg;
        int b1 = cfg.value(QStringLiteral("binEdge1"), 299).toInt();
        int b2 = cfg.value(QStringLiteral("binEdge2"), 499).toInt();
        int b3 = cfg.value(QStringLiteral("binEdge3"), 700).toInt();
        b1 = qBound(100, b1, 1996); /* 容错: 越界/乱序历史配置收敛到合法严格递增 */
        b2 = qBound(b1 + 1, b2, 1997);
        b3 = qBound(b2 + 1, b3, 1998);
        updatingBins_ = true;
        ui->binEdge1Spin->setValue(b1);
        ui->binEdge2Spin->setValue(b2);
        ui->binEdge3Spin->setValue(b3);
        updatingBins_ = false;
        heatmap_->setRanges(b1, b2, b3);
    }
    connect(ui->binEdge1Spin, &QSpinBox::valueChanged,
            this, &MainWindow::onBinEdgesChanged);
    connect(ui->binEdge2Spin, &QSpinBox::valueChanged,
            this, &MainWindow::onBinEdgesChanged);
    connect(ui->binEdge3Spin, &QSpinBox::valueChanged,
            this, &MainWindow::onBinEdgesChanged);

    connect(ui->analyzeBtn, &QPushButton::toggled,
            this, &MainWindow::onAnalyzeToggled);

    /* chartGroup 内部左右 1:1 等分 */
    ui->chartLayout->setStretch(0, 1);
    ui->chartLayout->setStretch(1, 1);

    logPanel_ = new LogPanel(this);
    ui->logContainer->layout()->addWidget(logPanel_);

    /* 日志默认折叠, 按钮显示 "展开"; 用户点击后切到展开态再切回 */
    ui->logToggleBtn->setChecked(false);
    ui->logToggleBtn->setText(QStringLiteral("展开"));
    ui->logContainer->setVisible(false);
    logPanel_->setBodyVisible(false);
    connect(ui->logToggleBtn, &QToolButton::toggled,
            this, &MainWindow::onLogToggleClicked);

    /* 距离分布分析: 标定完成 (READY, 针孔常数 C 有值) 前禁止开启,
     * 避免无 C 时段混入统计; onAlgStateChanged 随状态实时切换 */
    ui->analyzeBtn->setEnabled(false);

    ui->boardIpEdit->setText(currentBoardIp_);
    ui->boardPortEdit->setText(QStringLiteral("6000"));

    /* 过近提醒配置记忆 (默认 300mm 持续 5s), 改动即持久化并即时下发板端 */
    QSettings cfg;
    ui->alertChk->setChecked(cfg.value(QStringLiteral("alertEnabled"), true).toBool());
    ui->alertDminSpin->setValue(cfg.value(QStringLiteral("alertDminMm"), 300).toInt());
    ui->alertHoldSpin->setValue(cfg.value(QStringLiteral("alertHoldS"), 5).toInt());
    connect(ui->alertChk, &QCheckBox::toggled,
            this, &MainWindow::onAlertCfgChanged);
    connect(ui->alertDminSpin, &QSpinBox::valueChanged,
            this, &MainWindow::onAlertCfgChanged);
    connect(ui->alertHoldSpin, &QSpinBox::valueChanged,
            this, &MainWindow::onAlertCfgChanged);

    setupStateLabels();

    connect(boardLink_, &BoardLink::algStateChanged,
            this, &MainWindow::onAlgStateChanged);
    connect(boardLink_, &BoardLink::connStateChanged,
            this, &MainWindow::onConnStateChanged);
    connect(boardLink_, &BoardLink::linkError,
            this, &MainWindow::onLinkError);

    connect(videoPanel_, &VideoPanel::errorOccurred,
            this, &MainWindow::onVideoError);
    connect(videoPanel_, &VideoPanel::videoFrameTick,
            this, &MainWindow::onVideoFrameTick);

    connect(ui->calibrateBtn, &QPushButton::clicked,
            this, &MainWindow::onCalibrateClicked);
    connect(ui->connectBtn, &QPushButton::clicked,
            this, &MainWindow::onConnectClicked);

    statusBar()->showMessage(QStringLiteral("未连接"));
    logInfo(QStringLiteral("界面就绪 · 等待连接板端"));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::configureSplitters()
{
    /* 主分隔: 左侧自适应伸展, 右侧仪表盘固定 (min/max 由 .ui 决定) */
    ui->mainSplitter->setStretchFactor(0, 1);
    ui->mainSplitter->setStretchFactor(1, 0);
    ui->mainSplitter->setSizes({900, 340});

    /* 垂直分隔: 视频 / 曲线 / 日志, 默认 5:3:2 */
    ui->vertSplitter->setStretchFactor(0, 5);
    ui->vertSplitter->setStretchFactor(1, 3);
    ui->vertSplitter->setStretchFactor(2, 2);
}

void MainWindow::applyBannerRole()
{
    /* 顶部横幅样式由 QSS 选择器 QLabel[role="alert"] 接管, 不再内联色块 */
    ui->alertBanner->setProperty("role", QStringLiteral("alert"));
    ui->alertBanner->style()->unpolish(ui->alertBanner);
    ui->alertBanner->style()->polish(ui->alertBanner);
}

void MainWindow::onLogToggleClicked(bool checked)
{
    ui->logContainer->setVisible(checked);
    logPanel_->setBodyVisible(checked);
    ui->logToggleBtn->setText(checked ? QStringLiteral("收起") : QStringLiteral("展开"));
}

void MainWindow::onAnalyzeToggled(bool checked)
{
    if (heatmap_) {
        heatmap_->setAnalyzing(checked);
    }
    ui->analyzeBtn->setText(checked ? QStringLiteral("停止分析")
                                    : QStringLiteral("开启分析"));
    if (checked) {
        logInfo(QStringLiteral("开启距离分布分析"));
    } else {
        logInfo(QStringLiteral("停止距离分布分析 (累计 %1 个样本)")
                    .arg(heatmap_ ? heatmap_->sampleCount() : 0));
    }
}

void MainWindow::setupStateLabels()
{
    ui->stateDot->setText(QStringLiteral("\u25CF"));
    ui->stateText->setText(QStringLiteral("IDLE"));
    applyStateStyle(QStringLiteral("IDLE"));
    ui->dValue->setText(QStringLiteral("--"));
    ui->cValue->setText(QStringLiteral("--"));
    ui->wValue->setText(QStringLiteral("--"));
    ui->calibBar->setValue(0);
    ui->calibBar->setVisible(false);
    ui->calibLabel->setVisible(false);
}

void MainWindow::applyStateStyle(const QString &state)
{
    /* 颜色由 QSS QLabel[state="..."] 选择器统一控制, 这里只驱动属性 */
    ui->stateDot->setProperty("state", state);
    ui->stateDot->style()->unpolish(ui->stateDot);
    ui->stateDot->style()->polish(ui->stateDot);
    ui->stateText->setText(state);
}

void MainWindow::applyConnProperty(const QString &tag)
{
    ui->connDot->setProperty("state", tag);
    ui->connDot->style()->unpolish(ui->connDot);
    ui->connDot->style()->polish(ui->connDot);
}

void MainWindow::onAlgStateChanged(const AlgState &state)
{
    applyStateStyle(state.state);

    /* 时钟桥锚点刷新 (pts/ats 恒有效字段): 板端重启 ats 回落时旧队列直显清空, 防死等 */
    if (state.pts > 0 && state.ats > 0) {
        if (lastAts_ > 0 && (qint64)state.ats + 1000 < lastAts_)
            flushPendingAll();
        lastPts_ = (qint64)state.pts;
        lastAts_ = (qint64)state.ats;
    }

    /* faces != 1 时板端 D/W 不再更新 (陈旧数据), 一律判无效 */
    const bool faceInvalid = state.isReady() && !state.isFaceValid();
    const bool ready = state.isReady() && state.D >= 0 && state.isFaceValid();

    /* 分析按钮: 仅标定完成后允许开启 (开启后中途重新标定不强制关) */
    ui->analyzeBtn->setEnabled(state.isReady());

    if (state.isReady()) {
        ui->cValue->setText(QString::number(state.C)); /* C 恒定, 立即显示无碍 */
    }

    if (ready) {
        /* 帧级门控: 锚点齐备时 D/W 入队, 由视频帧节拍 flush 上屏 (数字不超前画面);
         * 无锚点 (旧固件) 退化为即到即显 */
        if (state.hasPairingInfo() && lastPts_ > 0 && lastAts_ > 0) {
            if (lastEnqTs_ > 0 && (qint64)state.ts + 1000 < lastEnqTs_) {
                flushPendingAll();      /* ts 回绕 (板端重启): 旧队列直显清空 */
            }
            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            pendingD_.append({state.D, state.W, (qint64)state.ts, nowMs});
            lastEnqTs_ = (qint64)state.ts;
            if (pendingD_.size() > 8) { /* 视频停滞保护: 丢最旧保流动 */
                const PendingD p = pendingD_.takeFirst();
                showDImmediate(p.D, p.W);
            }
        } else {
            showDImmediate(state.D, state.W);
        }
    } else {
        ui->dValue->setText(QStringLiteral("--"));
        ui->wValue->setText(QStringLiteral("--"));
        if (!state.isReady()) {
            ui->cValue->setText(QStringLiteral("--"));  /* C 仅 READY 期有效 */
        }
        pendingD_.clear();                              /* D 失效: 排队数据一并作废 */
        if (faceInvalid) {
            shownD_ = INT_MIN;                          /* "--" 后恢复同值 D 也能重新上屏 */
            distancePlot_->appendInvalid();             /* faces != 1: 红色竖直色带 */
        } else {
            distancePlot_->notifyGap();                 /* 断线 / 非 READY: 普通断点 */
        }
        if (heatmap_) {
            heatmap_->onInvalidSample();                /* 分析期无有效数据时段计入无效扇区 */
        }
    }

    const bool calibrating = state.isCalibrating();
    ui->calibBar->setVisible(calibrating);
    ui->calibLabel->setVisible(calibrating);
    if (calibrating) {
        ui->calibBar->setRange(0, 5);
        ui->calibBar->setValue(state.pairs);
        /* Time_Stamp Phase④: 配对质量显示 (板端仅 CALIBRATING 期上送, 旧固件/缺字段默认 0) */
        ui->calibLabel->setText(QStringLiteral("配对 %1/5 · 偏差%2ms · 帧%3")
            .arg(state.pairs)
            .arg(state.tdev)
            .arg(state.tfid));
    }
    ui->calibrateBtn->setEnabled(!calibrating);

    if (state.fps > 0) {
        statusBar()->showMessage(
            QStringLiteral("已连接  fps=%1").arg(state.fps));
    }

    /* 数据通联状态摘要日志: 每 5s 记一行 (DEBUG 排查, 默认被 qInfo 落到 qt_monitor.log) */
    static qint64 lastSummaryMs = 0;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - lastSummaryMs >= 5000) {
        lastSummaryMs = now;
        if (ready) {
            qInfo("[data] state=%s D=%d C=%d W=%d fps=%d faces=%d",
                  qPrintable(state.state), state.D, state.C, state.W, state.fps, state.faces);
            logInfo(QStringLiteral("数据 state=%1 D=%2mm C=%3 W=%4px fps=%5 faces=%6")
                        .arg(state.state)
                        .arg(state.D)
                        .arg(state.C)
                        .arg(state.W)
                        .arg(state.fps)
                        .arg(state.faces));
        } else if (faceInvalid) {
            qInfo("[data] state=%s faces=%d (数据无效: 人脸数 != 1)",
                  qPrintable(state.state), state.faces);
            logInfo(QStringLiteral("数据 state=%1 faces=%2 (数据无效: 人脸数 != 1)")
                        .arg(state.state)
                        .arg(state.faces));
        } else {
            qInfo("[data] state=%s (非 READY)", qPrintable(state.state));
            logInfo(QStringLiteral("数据 state=%1 (非 READY)").arg(state.state));
        }
    }

    /* 过近提醒 (10Hz 状态节拍驱动): 滞回判定 → 横幅 + 提示音
     * D < 阈值持续 hold 秒触发; D >= 阈值+30mm 滞回解除; 阈值~滞回带间维持现状 */
    if (ui->alertChk->isChecked() && ready) {
        const int dmin = ui->alertDminSpin->value();
        const qint64 holdMs = ui->alertHoldSpin->value() * 1000;
        const qint64 nowMs = wall_.elapsed();
        if (state.D < dmin) {
            if (nearSinceMs_ == 0) {
                nearSinceMs_ = nowMs;               /* 开始计时 */
            } else if (!alertActive_ && nowMs - nearSinceMs_ >= holdMs) {
                setAlertActive(true);               /* 持续过近成立 */
            }
        } else if (state.D >= dmin + 30) {
            nearSinceMs_ = 0;                       /* 恢复 (含 30mm 滞回) */
            setAlertActive(false);
        }
    } else {
        nearSinceMs_ = 0;                           /* 离开 READY/数据无效/关闭开关: 复位 */
        setAlertActive(false);
    }
}

void MainWindow::onAlertCfgChanged()
{
    QSettings s;
    s.setValue(QStringLiteral("alertEnabled"), ui->alertChk->isChecked());
    s.setValue(QStringLiteral("alertDminMm"), ui->alertDminSpin->value());
    s.setValue(QStringLiteral("alertHoldS"), ui->alertHoldSpin->value());
    sendAlertCfg();                     /* 预留: 板端实现 CFG 后即同步 (当前固件忽略未知行) */
    logInfo(QStringLiteral("提醒配置更新: 启用=%1 阈值=%2mm 持续=%3s")
                .arg(ui->alertChk->isChecked() ? QStringLiteral("是") : QStringLiteral("否"))
                .arg(ui->alertDminSpin->value())
                .arg(ui->alertHoldSpin->value()));
}

void MainWindow::sendAlertCfg()
{
    if (!boardLink_ || !boardLink_->isConnected())
        return;
    boardLink_->sendCommand(QStringLiteral("CFG %1 %2 %3")  /* 板端预留接口, 当前不实现 */
        .arg(ui->alertDminSpin->value())
        .arg(ui->alertHoldSpin->value() * 1000)
        .arg(ui->alertChk->isChecked() ? 1 : 0));
}

void MainWindow::onBinEdgesChanged()
{
    if (updatingBins_) {
        return;
    }
    updatingBins_ = true;
    /* 级联约束: 三边界严格递增 (后框下限 = 前框 + 1), 嵌套 valueChanged 被标志拦截 */
    if (ui->binEdge1Spin->value() >= ui->binEdge2Spin->value()) {
        ui->binEdge2Spin->setValue(ui->binEdge1Spin->value() + 1);
    }
    if (ui->binEdge2Spin->value() >= ui->binEdge3Spin->value()) {
        ui->binEdge3Spin->setValue(ui->binEdge2Spin->value() + 1);
    }
    updatingBins_ = false;

    const int b1 = ui->binEdge1Spin->value();
    const int b2 = ui->binEdge2Spin->value();
    const int b3 = ui->binEdge3Spin->value();

    QSettings s;
    s.setValue(QStringLiteral("binEdge1"), b1);
    s.setValue(QStringLiteral("binEdge2"), b2);
    s.setValue(QStringLiteral("binEdge3"), b3);

    if (heatmap_) {
        const bool hadSamples = heatmap_->sampleCount() > 0;
        heatmap_->setRanges(b1, b2, b3);
        logInfo(QStringLiteral("饼图区间更新: 0-%1 / %2-%3 / %4-%5 / >%6 mm%7")
                    .arg(b1).arg(b1 + 1).arg(b2).arg(b2 + 1).arg(b3).arg(b3)
                    .arg(hadSamples ? QStringLiteral(" (累计已清零)") : QString()));
    }
}

void MainWindow::setAlertActive(bool on)
{
    if (on == alertActive_)
        return;
    alertActive_ = on;
    if (on) {
        ui->alertBanner->setText(QStringLiteral("距离过近（< %1 mm），请保持健康用眼距离")
                                     .arg(ui->alertDminSpin->value()));
        QApplication::beep();                       /* 触发沿提示音两声 */
        QTimer::singleShot(300, this, [] { QApplication::beep(); });
        logWarn(QStringLiteral("过近提醒触发 (阈值 %1 mm)").arg(ui->alertDminSpin->value()));
    } else {
        logInfo(QStringLiteral("过近提醒解除"));
    }
    ui->alertBanner->setVisible(on);
}

void MainWindow::onConnStateChanged(ConnState s)
{
    switch (s) {
    case ConnState::Disconnected:
        nearSinceMs_ = 0;               /* 数据停更, 告警状态一并复位 */
        setAlertActive(false);
        applyConnProperty(QStringLiteral("offline"));
        ui->connText->setText(QStringLiteral("离线"));
        ui->calibrateBtn->setEnabled(false);
        ui->connectBtn->setText(QStringLiteral("连接"));
        statusBar()->showMessage(
            QStringLiteral("状态通道离线（板端未跑 qtserver 时正常），视频独立运行"));
        logWarn(QStringLiteral("状态通道断开"));
        break;
    case ConnState::Connecting:
        applyConnProperty(QStringLiteral("connecting"));
        ui->connText->setText(QStringLiteral("连接中"));
        ui->connectBtn->setText(QStringLiteral("取消"));
        statusBar()->showMessage(QStringLiteral("正在连接..."));
        logInfo(QStringLiteral("正在连接状态通道..."));
        break;
    case ConnState::Connected:
        applyConnProperty(QStringLiteral("online"));
        ui->connText->setText(QStringLiteral("在线"));
        ui->connectBtn->setText(QStringLiteral("断开"));
        statusBar()->showMessage(QStringLiteral("已连接"));
        sendAlertCfg();                 /* 连接即下发提醒配置 (旧固件忽略未知行, 无害) */
        logInfo(QStringLiteral("状态通道已连接, 已下发提醒配置"));
        break;
    }
}

void MainWindow::onLinkError(const QString &msg)
{
    statusBar()->showMessage(QStringLiteral("[链路] ") + msg, 5000);
    logError(QStringLiteral("[链路] ") + msg);
}

void MainWindow::onVideoError(const QString &msg)
{
    statusBar()->showMessage(QStringLiteral("[视频] ") + msg, 5000);
    logError(QStringLiteral("[视频] ") + msg);
}

/* 帧级门控 (Time_Stamp Phase①): 每帧上屏节拍回调, ptsUs = 该帧 VENC 时刻 (µs)。
 * 时钟桥公式换算到板端 rt_tick 轴: rt_of = ats + (帧PTS_ms − pts)  (PROTOCOL.md),
 * flush 所有 ts <= rt_of 的排队 D —— 数字只描述"画面已经演到"的过去帧, 永不超前 */
void MainWindow::onVideoFrameTick(qint64 ptsUs)
{
    if (lastPts_ <= 0 || lastAts_ <= 0 || pendingD_.isEmpty())
        return;

    const qint64 framePtsMs = ptsUs / 1000;
    const qint64 deltaMs = framePtsMs - lastPts_;
    if (qAbs(deltaMs) > 10000) {         /* PTS 回绕/跳变 (RTP 32bit ~13.25h): 退化直显 */
        flushPendingAll();
        return;
    }
    const qint64 rtOfFrame = lastAts_ + deltaMs;

    qint64 flushLagMs = -1;
    while (!pendingD_.isEmpty() && pendingD_.first().ts <= rtOfFrame) {
        const PendingD p = pendingD_.takeFirst();
        showDImmediate(p.D, p.W);
        flushLagMs = QDateTime::currentMSecsSinceEpoch() - p.enqMs;
    }
    if (flushLagMs >= 0) {               /* 配对延迟观测: 5s 一行, 仅 qInfo 落日志文件 */
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        if (now - lastFlushLogMs_ >= 5000) {
            lastFlushLogMs_ = now;
            qInfo("[pair] flushed lag=%lldms queue=%u rt_of=%lld",
                  flushLagMs, pendingD_.size(), rtOfFrame);
        }
    }
}

/* 直显路径: 行为与原"即到即显"一致 (setText 去重 + 曲线/饼图累计) */
void MainWindow::showDImmediate(int d, int w)
{
    if (d != shownD_) {
        shownD_ = d;
        ui->dValue->setText(QString::number(d));
        ui->wValue->setText(QString::number(w));
    }
    distancePlot_->appendSample(d);
    if (heatmap_) {
        heatmap_->onSample(d);           /* 仅在分析开启时真正累计 */
    }
}

/* 队列全部直显清空 (异常退化: 板端重启 / PTS 跳变) */
void MainWindow::flushPendingAll()
{
    while (!pendingD_.isEmpty()) {
        const PendingD p = pendingD_.takeFirst();
        showDImmediate(p.D, p.W);
    }
    lastEnqTs_ = 0;
}

void MainWindow::onCalibrateClicked()
{
    if (boardLink_ && boardLink_->isConnected()) {
        boardLink_->sendCommand(QStringLiteral("KEY1"));
        statusBar()->showMessage(QStringLiteral("已发送 KEY1，等待标定..."), 3000);
        logInfo(QStringLiteral("发送 KEY1, 等待板端进入标定"));
    } else {
        QMessageBox::warning(this, QStringLiteral("未连接"),
                             QStringLiteral("请先连接板端"));
    }
}

void MainWindow::onConnectClicked()
{
    if (boardLink_ && boardLink_->isConnected()) {
        boardLink_->disconnectFromBoard();
        return;
    }
    const QString ip   = ui->boardIpEdit->text().trimmed();
    quint16 port = ui->boardPortEdit->text().toUShort();

    // 防呆：560/562 是 RTSP 视频端口（程序内部自动拉流），状态通道端口是 6000
    if (port == 560 || port == 562) {
        QMessageBox::information(this, QStringLiteral("端口已纠正"),
            QStringLiteral("560/562 是 RTSP 视频端口（视频区自动使用，无需填写）。\n"
                           "状态通道端口应为 6000，已自动改用 6000。"));
        port = 6000;
        ui->boardPortEdit->setText(QStringLiteral("6000"));
    }
    if (port == 0) {
        port = 6000;
    }
    if (ip.isEmpty()) {
        QMessageBox::warning(this, QStringLiteral("参数错误"),
                             QStringLiteral("请填写板端 IP"));
        return;
    }
    currentBoardIp_ = ip;

    // 视频只依赖 RTSP 560，立即启动；状态通道(TCP 6000)独立连接，失败不影响视频
    const QUrl videoUrl(QStringLiteral("rtsp://%1:560").arg(ip));
    videoPanel_->setUrl(videoUrl);
    videoPanel_->start();
    logInfo(QStringLiteral("启动 RTSP 拉流 rtsp://%1:560").arg(ip));

    boardLink_->connectTo(ip, port);
}

void MainWindow::logInfo(const QString &msg)
{
    if (logPanel_) logPanel_->appendInfo(msg);
}

void MainWindow::logWarn(const QString &msg)
{
    if (logPanel_) logPanel_->appendWarn(msg);
}

void MainWindow::logError(const QString &msg)
{
    if (logPanel_) logPanel_->appendError(msg);
}
