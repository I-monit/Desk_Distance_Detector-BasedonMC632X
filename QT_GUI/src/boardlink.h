#ifndef BOARDLINK_H
#define BOARDLINK_H

#include "algstate.h"

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QDateTime>

class BoardLink : public QObject
{
    Q_OBJECT
public:
    explicit BoardLink(QObject *parent = nullptr);
    ~BoardLink() override;

    void connectTo(const QString &host, quint16 port = 6000);
    void disconnectFromBoard();

    bool isConnected() const;

    void sendCommand(const QString &cmd);

signals:
    void algStateChanged(const AlgState &state);
    void connStateChanged(ConnState s);
    void linkError(const QString &msg);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError err);
    void onHeartbeatTick();
    void onReconnectTick();

private:
    void parseLine(const QByteArray &line);
    void scheduleReconnect(int delayMs);
    void resetHeartbeatClock();

    QTcpSocket *socket_ = nullptr;
    QByteArray  buffer_;
    QTimer     *reconnectTimer_ = nullptr;
    QTimer     *heartbeatTimer_ = nullptr;
    QString     host_;
    quint16     port_ = 6000;

    ConnState   connState_ = ConnState::Disconnected;
    qint64      lastRxMs_  = 0;
    qint64      lastTxPingMs_ = 0;
};

#endif
