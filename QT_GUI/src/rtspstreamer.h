#ifndef RTSPSTREAMER_H
#define RTSPSTREAMER_H

#include <QThread>
#include <QUrl>
#include <QVideoFrame>
#include <QElapsedTimer>
#include <atomic>
#include <mutex>

/* FFmpeg 自主拉流+软解线程: 解出即上屏(最新帧策略), 无播放引擎缓冲
 * - RTP over TCP interleaved (板端 librtsp 仅支持 TCP)
 * - NOBUFFER + 小probe + 单线程解码, 最小化流水线延迟
 * - UI 线程通过 frameReady + takeLatest 取"最新帧"(旧帧自动丢弃) */
class RtspStreamer : public QThread
{
    Q_OBJECT
public:
    explicit RtspStreamer(const QUrl &url, QObject *parent = nullptr);
    ~RtspStreamer() override;

    void shutdown();                    /* 请求退出并等待线程结束 */
    bool takeLatest(QVideoFrame &out);  /* 取最新解码帧, 无新帧返回 false */

signals:
    void frameReady();                  /* 有新帧可取 (配合 takeLatest 合并上屏) */
    void streamError(const QString &msg);

protected:
    void run() override;

private:
    static int interruptCb(void *opaque);

    QUrl url_;
    std::atomic_bool abort_{false};

    QElapsedTimer clock_;
    std::atomic<qint64> lastReadMs_{0};

    std::mutex mtx_;
    QVideoFrame latest_;
    bool hasLatest_ = false;
};

#endif // RTSPSTREAMER_H
