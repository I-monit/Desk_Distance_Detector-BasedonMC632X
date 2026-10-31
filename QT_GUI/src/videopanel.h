#ifndef VIDEOPANEL_H
#define VIDEOPANEL_H

#include <QWidget>
#include <QUrl>

class QVideoWidget;
class RtspStreamer;

class VideoPanel : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPanel(QWidget *parent = nullptr);
    ~VideoPanel() override;

    void setUrl(const QUrl &url);
    void start();
    void stop();

    bool isPlaying() const;

signals:
    void errorOccurred(const QString &msg);
    /* 每帧上屏节拍 (帧级配对用): ptsUs = 该帧 VENC 时刻 (µs, 经 RTP PTS 90kHz 还原,
     * rtspstreamer toVideoFrame 里 setStartTime 塞入), 提交至 sink 时刻发出 */
    void videoFrameTick(qint64 ptsUs);

private slots:
    void onFrameReady();
    void onStreamError(const QString &msg);

private:
    void scheduleRetry();
    void openStream();
    void closeStream();

    QVideoWidget *videoWidget_  = nullptr;
    RtspStreamer *streamer_     = nullptr;
    QUrl          url_;
    bool          wantPlaying_  = false;
    bool          retryPending_ = false;
};

#endif // VIDEOPANEL_H
