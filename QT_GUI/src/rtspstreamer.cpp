#include "rtspstreamer.h"

#include <QVideoFrameFormat>
#include <QSize>
#include <QDebug>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

namespace {

constexpr int kStallTimeoutMs = 5000;   /* 网络停滞判定, 对齐原 networkTimeout 语义 */

QString errText(int err)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(err, buf, sizeof(buf));
    return QString::fromLatin1(buf);
}

/* AVFrame -> QVideoFrame: YUV420P/NV12 直接平面拷贝, 其它格式经 sws 兜底转 NV12 */
QVideoFrame toVideoFrame(const AVFrame *f, qint64 ptsUs, SwsContext **sws)
{
    const AVPixelFormat srcFmt = static_cast<AVPixelFormat>(f->format);
    bool needSws = false;
    QVideoFrameFormat::PixelFormat qFmt = QVideoFrameFormat::Format_Invalid;

    if (srcFmt == AV_PIX_FMT_YUV420P || srcFmt == AV_PIX_FMT_YUVJ420P) {
        qFmt = QVideoFrameFormat::Format_YUV420P;
    } else if (srcFmt == AV_PIX_FMT_NV12) {
        qFmt = QVideoFrameFormat::Format_NV12;
    } else {
        qFmt = QVideoFrameFormat::Format_NV12;
        needSws = true;
    }

    AVFrame conv;
    memset(&conv, 0, sizeof(conv));
    if (needSws) {
        *sws = sws_getCachedContext(*sws, f->width, f->height, srcFmt,
                                    f->width, f->height, AV_PIX_FMT_NV12,
                                    SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!*sws)
            return {};
        conv.format = AV_PIX_FMT_NV12;
        conv.width  = f->width;
        conv.height = f->height;
        if (av_frame_get_buffer(&conv, 32) < 0)
            return {};
        sws_scale(*sws, f->data, f->linesize, 0, f->height, conv.data, conv.linesize);
        f = &conv;
    }

    QVideoFrame out(QVideoFrameFormat(QSize(f->width, f->height), qFmt));
    if (out.isValid()) {
        out.setStartTime(ptsUs);
        const int planes = (qFmt == QVideoFrameFormat::Format_NV12) ? 2 : 3;
        if (out.map(QVideoFrame::ReadWrite)) {
            for (int p = 0; p < planes; ++p) {
                const int h = (p == 0) ? f->height : (f->height + 1) / 2;
                const int srcStride = f->linesize[p];
                const int dstStride = out.bytesPerLine(p);
                const uchar *src = f->data[p];
                uchar *dst = out.bits(p);
                const int rowBytes = qMin(srcStride, dstStride);
                for (int y = 0; y < h; ++y)
                    memcpy(dst + (size_t)y * dstStride, src + (size_t)y * srcStride, rowBytes);
            }
            out.unmap();
        }
    }

    if (needSws)
        av_frame_unref(&conv);
    return out;
}

} // namespace

RtspStreamer::RtspStreamer(const QUrl &url, QObject *parent)
    : QThread(parent)
    , url_(url)
{
}

RtspStreamer::~RtspStreamer()
{
    shutdown();
}

void RtspStreamer::shutdown()
{
    abort_ = true;
    if (isRunning())
        wait(3000);
}

bool RtspStreamer::takeLatest(QVideoFrame &out)
{
    std::lock_guard<std::mutex> lk(mtx_);
    if (!hasLatest_)
        return false;
    out = latest_;
    hasLatest_ = false;
    return true;
}

int RtspStreamer::interruptCb(void *opaque)
{
    auto *self = static_cast<RtspStreamer *>(opaque);
    if (self->abort_.load())
        return 1;
    if (self->clock_.elapsed() - self->lastReadMs_.load(std::memory_order_relaxed)
            > kStallTimeoutMs)
        return 1; /* 网络停滞超时, 中断阻塞 IO */
    return 0;
}

void RtspStreamer::run()
{
    AVFormatContext *fc = avformat_alloc_context();
    AVCodecContext   *cc = nullptr;
    AVPacket         *pkt = av_packet_alloc();
    AVFrame          *frame = av_frame_alloc();
    SwsContext       *sws = nullptr;
    QString failMsg;

    auto cleanup = [&]() {
        if (fc) avformat_close_input(&fc);
        if (cc) avcodec_free_context(&cc);
        if (pkt) av_packet_free(&pkt);
        if (frame) av_frame_free(&frame);
        if (sws) sws_freeContext(sws);
    };

    if (!fc || !pkt || !frame) {
        cleanup();
        emit streamError(QStringLiteral("alloc failed"));
        return;
    }

    clock_.start();
    lastReadMs_ = 0;
    fc->interrupt_callback.callback = &RtspStreamer::interruptCb;
    fc->interrupt_callback.opaque = this;
    fc->flags |= AVFMT_FLAG_NOBUFFER;    /* 禁用 demuxer 侧整帧缓存 */

    AVDictionary *opts = nullptr;
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);       /* 板端仅支持 TCP interleaved */
    av_dict_set(&opts, "buffer_size", "131072", 0);       /* socket 缓冲 ~128KB (2Mbps 下 <0.5s) */
    av_dict_set(&opts, "reorder_queue_size", "0", 0);     /* 无 RTP 重排缓存 */
    av_dict_set(&opts, "probesize", "65536", 0);          /* 最小流探测 */
    av_dict_set(&opts, "analyzeduration", "0", 0);
    av_dict_set(&opts, "timeout", "5000000", 0);          /* socket IO 超时 5s */
    av_dict_set(&opts, "stimeout", "5000000", 0);         /* 旧版本参数名兼容 */

    qDebug() << "[video] opening" << url_.toString();
    int ret = avformat_open_input(&fc, url_.toString().toUtf8().constData(),
                                  nullptr, &opts);
    av_dict_free(&opts);
    if (ret < 0) {
        cleanup();
        emit streamError(QStringLiteral("open failed: %1").arg(errText(ret)));
        return;
    }
    if (abort_) {
        cleanup();
        return;
    }

    avformat_find_stream_info(fc, nullptr); /* probesize/analyzeduration 已限制, 快速返回 */
    lastReadMs_ = clock_.elapsed();

    int vIdx = -1;
    for (unsigned i = 0; i < fc->nb_streams; ++i) {
        if (fc->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            vIdx = static_cast<int>(i);
            break;
        }
    }
    if (vIdx < 0) {
        cleanup();
        emit streamError(QStringLiteral("no video stream"));
        return;
    }

    const AVCodec *dec = avcodec_find_decoder(fc->streams[vIdx]->codecpar->codec_id);
    if (!dec) {
        cleanup();
        emit streamError(QStringLiteral("decoder not found"));
        return;
    }
    cc = avcodec_alloc_context3(dec);
    if (!cc || avcodec_parameters_to_context(cc, fc->streams[vIdx]->codecpar) < 0) {
        cleanup();
        emit streamError(QStringLiteral("codec param failed"));
        return;
    }
    cc->flags  |= AV_CODEC_FLAG_LOW_DELAY;  /* 禁解码器重排缓冲 */
    cc->flags2 |= AV_CODEC_FLAG2_FAST;
    cc->thread_count = 1;                   /* 单线程: 无帧级流水延迟 */
    if (avcodec_open2(cc, dec, nullptr) < 0) {
        cleanup();
        emit streamError(QStringLiteral("open decoder failed"));
        return;
    }

    const AVRational tb = fc->streams[vIdx]->time_base;

    while (!abort_) {
        ret = av_read_frame(fc, pkt);
        if (ret < 0) {
            failMsg = QStringLiteral("read: %1").arg(errText(ret));
            break;
        }
        lastReadMs_ = clock_.elapsed();

        if (pkt->stream_index != vIdx) {
            av_packet_unref(pkt);
            continue;
        }

        if (avcodec_send_packet(cc, pkt) >= 0) {
            while (!abort_ && avcodec_receive_frame(cc, frame) == 0) {
                int64_t p = (frame->pts != AV_NOPTS_VALUE)
                                ? frame->pts
                                : frame->best_effort_timestamp;
                qint64 ptsUs = -1;
                if (p != AV_NOPTS_VALUE)
                    ptsUs = static_cast<qint64>(double(p) * tb.num * 1e6 / tb.den);

                QVideoFrame vf = toVideoFrame(frame, ptsUs, &sws);
                if (vf.isValid()) {
                    {
                        std::lock_guard<std::mutex> lk(mtx_);
                        latest_ = vf;      /* 覆盖旧帧: 始终只保留最新 */
                        hasLatest_ = true;
                    }
                    emit frameReady();
                }
                av_frame_unref(frame);
            }
        }
        av_packet_unref(pkt);
    }

    const bool aborted = abort_.load();
    cleanup();
    if (!aborted && !failMsg.isEmpty())
        emit streamError(failMsg);
}
