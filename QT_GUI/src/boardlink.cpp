#include "boardlink.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QHostAddress>
#include <QDebug>

BoardLink::BoardLink(QObject *parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
    , reconnectTimer_(new QTimer(this))
    , heartbeatTimer_(new QTimer(this))
{
    buffer_.reserve(4096);

    reconnectTimer_->setSingleShot(true);
    heartbeatTimer_->setInterval(5000);

    connect(socket_, &QTcpSocket::connected,    this, &BoardLink::onConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &BoardLink::onDisconnected);
    connect(socket_, &QTcpSocket::readyRead,    this, &BoardLink::onReadyRead);
    connect(socket_, &QAbstractSocket::errorOccurred,
            this, &BoardLink::onSocketError);

    connect(reconnectTimer_, &QTimer::timeout, this, &BoardLink::onReconnectTick);
    connect(heartbeatTimer_, &QTimer::timeout, this, &BoardLink::onHeartbeatTick);
}

BoardLink::~BoardLink()
{
    disconnectFromBoard();
}

void BoardLink::connectTo(const QString &host, quint16 port)
{
    host_ = host;
    port_ = port;
    buffer_.clear();

    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->abort();
    }

    connState_ = ConnState::Connecting;
    emit connStateChanged(connState_);

    socket_->connectToHost(QHostAddress(host_), port_);
}

void BoardLink::disconnectFromBoard()
{
    reconnectTimer_->stop();
    heartbeatTimer_->stop();
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->abort();
    }
    connState_ = ConnState::Disconnected;
    emit connStateChanged(connState_);
}

bool BoardLink::isConnected() const
{
    return socket_->state() == QAbstractSocket::ConnectedState;
}

void BoardLink::sendCommand(const QString &cmd)
{
    if (!isConnected()) {
        emit linkError(QStringLiteral("链路未连接，丢弃命令: %1").arg(cmd));
        return;
    }
    QByteArray payload = cmd.toUtf8();
    payload.append('\n');
    socket_->write(payload);
}

void BoardLink::onConnected()
{
    connState_ = ConnState::Connected;
    buffer_.clear();
    resetHeartbeatClock();
    heartbeatTimer_->start();
    emit connStateChanged(connState_);
}

void BoardLink::onDisconnected()
{
    qDebug() << "[link] disconnected, reconnect in 3s ->" << host_ << port_;
    heartbeatTimer_->stop();
    connState_ = ConnState::Disconnected;
    emit connStateChanged(connState_);
    if (!host_.isEmpty()) {
        scheduleReconnect(3000);
    }
}

void BoardLink::onSocketError(QAbstractSocket::SocketError err)
{
    qDebug() << "[link] socket error:" << int(err) << socket_->errorString();
    emit linkError(socket_->errorString());
    // 连接从未建立（如板端未开 6000 端口，ConnectionRefused）时 Qt 不发 disconnected，
    // 需在这里补状态流转，否则 UI 卡在"连接中"
    if (connState_ == ConnState::Connecting
            && socket_->state() == QAbstractSocket::UnconnectedState) {
        heartbeatTimer_->stop();
        connState_ = ConnState::Disconnected;
        emit connStateChanged(connState_);
        if (!host_.isEmpty()) {
            scheduleReconnect(3000);
        }
    }
}

void BoardLink::onHeartbeatTick()
{
    if (!isConnected()) {
        return;
    }
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (lastRxMs_ > 0 && (now - lastRxMs_) > 10000) {
        emit linkError(QStringLiteral("心跳超时 (>10s 无响应)，主动断开"));
        socket_->abort();
        return;
    }
    if (lastTxPingMs_ == 0 || (now - lastTxPingMs_) >= 5000) {
        sendCommand(QStringLiteral("PING"));
        lastTxPingMs_ = now;
    }
}

void BoardLink::onReconnectTick()
{
    connectTo(host_, port_);
}

void BoardLink::onReadyRead()
{
    buffer_.append(socket_->readAll());
    int idx;
    while ((idx = buffer_.indexOf('\n')) >= 0) {
        QByteArray line = buffer_.left(idx);
        buffer_.remove(0, idx + 1);
        line = line.trimmed();
        if (line.isEmpty()) continue;
        parseLine(line);
    }
    resetHeartbeatClock();
}

void BoardLink::parseLine(const QByteArray &line)
{
    if (line == "PONG") {
        return;
    }
    QJsonParseError perr{};
    QJsonDocument doc = QJsonDocument::fromJson(line, &perr);
    if (perr.error != QJsonParseError::NoError) {
        emit linkError(QStringLiteral("JSON 解析失败: %1").arg(perr.errorString()));
        return;
    }
    if (!doc.isObject()) {
        emit linkError(QStringLiteral("JSON 根非对象"));
        return;
    }
    QJsonObject obj = doc.object();
    AlgState s;
    s.state = obj.value("state").toString(QStringLiteral("IDLE"));
    s.D     = obj.value("D").toInt(-1);
    s.C     = obj.value("C").toInt(-1);
    s.W     = obj.value("W").toInt(-1);
    s.pairs = obj.value("pairs").toInt(0);
    s.fps   = obj.value("fps").toInt(0);
    s.alert = obj.value("alert").toInt(0);
    s.faces = obj.value("faces").toInt(0); /* 缺省 0 = 无效 (faces != 1 判无效) */
    /* 帧级配对字段 (新固件提供; 缺省 0 表示无效, 详见 algstate.h) */
    s.ts  = static_cast<quint64>(obj.value("ts").toDouble(0));
    s.fid = static_cast<quint64>(obj.value("fid").toDouble(0));
    s.pts = static_cast<quint64>(obj.value("pts").toDouble(0));
    s.ats = static_cast<quint64>(obj.value("ats").toDouble(0));
    /* 标定期配对质量字段 (Time_Stamp Phase④; 缺省 0) */
    s.tts  = static_cast<quint64>(obj.value("tts").toDouble(0));
    s.tfid = static_cast<quint64>(obj.value("tfid").toDouble(0));
    s.tdev = static_cast<quint64>(obj.value("tdev").toDouble(0));
    s.twin = static_cast<quint32>(obj.value("twin").toDouble(0));
    emit algStateChanged(s);
}

void BoardLink::scheduleReconnect(int delayMs)
{
    reconnectTimer_->stop();
    reconnectTimer_->setInterval(delayMs);
    reconnectTimer_->start();
}

void BoardLink::resetHeartbeatClock()
{
    lastRxMs_ = QDateTime::currentMSecsSinceEpoch();
    lastTxPingMs_ = 0;
}
