#include "videopanel.h"
#include "rtspstreamer.h"

#include <QVideoWidget>
#include <QVideoSink>
#include <QVideoFrame>
#include <QVBoxLayout>
#include <QTimer>
#include <QDebug>

VideoPanel::VideoPanel(QWidget *parent)
    : QWidget(parent)
    , videoWidget_(new QVideoWidget(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(videoWidget_);
}

VideoPanel::~VideoPanel()
{
    wantPlaying_ = false;
    closeStream();
}

void VideoPanel::setUrl(const QUrl &url)
{
    url_ = url;
}

void VideoPanel::start()
{
    wantPlaying_ = true;
    openStream();
}

void VideoPanel::stop()
{
    wantPlaying_ = false;
    closeStream();
    videoWidget_->videoSink()->setVideoFrame(QVideoFrame()); /* 清空画面 */
}

bool VideoPanel::isPlaying() const
{
    return streamer_ && streamer_->isRunning();
}

void VideoPanel::openStream()
{
    closeStream();
    if (!url_.isValid())
        return;

    streamer_ = new RtspStreamer(url_, this);
    connect(streamer_, &RtspStreamer::frameReady,
            this, &VideoPanel::onFrameReady);
    connect(streamer_, &RtspStreamer::streamError,
            this, &VideoPanel::onStreamError);
    streamer_->start();
}

void VideoPanel::closeStream()
{
    if (!streamer_)
        return;
    streamer_->disconnect(this);
    streamer_->shutdown();
    streamer_->deleteLater();
    streamer_ = nullptr;
}

void VideoPanel::onFrameReady()
{
    if (!streamer_)
        return;
    QVideoFrame f;
    if (streamer_->takeLatest(f)) {   /* 只取最新帧, 旧帧已在上屏途中被覆盖丢弃 */
        videoWidget_->videoSink()->setVideoFrame(f);
        emit videoFrameTick(f.startTime());
    }
}

void VideoPanel::onStreamError(const QString &msg)
{
    qDebug() << "[video] stream error:" << msg;
    emit errorOccurred(msg);
    scheduleRetry();
}

void VideoPanel::scheduleRetry()
{
    if (retryPending_)
        return;
    retryPending_ = true;
    qDebug() << "[video] stream ended, retry in 2s ...";
    QTimer::singleShot(2000, this, [this]{
        retryPending_ = false;
        if (wantPlaying_) {
            qDebug() << "[video] retry open";
            openStream();
        }
    });
}
