#include "distanceplot.h"

#include <QDateTime>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

#include <cmath>

namespace {
constexpr int kWindowMs = 30000;      /* 滚动时间窗口 */
constexpr double kDValidMin = 100.0;  /* 有效测距下限 | [mm] */
constexpr double kDValidMax = 2000.0; /* 有效测距上限 | [mm] */
/* 无效时段 (faces != 1) 红色竖直色带 */
const QColor kInvalidBandColor = QColor(0xe5, 0x45, 0x5c, 60);
const QColor kInvalidTextColor = QColor(0xf3, 0x87, 0x9b);
}

DistancePlot::DistancePlot(QWidget *parent)
    : QWidget(parent)
{
    setMinimumHeight(160);
    setAttribute(Qt::WA_TranslucentBackground, false);
}

void DistancePlot::appendSample(int d_mm)
{
    /* 超出有效测距范围的样本记为断点: 曲线断开, 不画到量程外 */
    const double d = (d_mm < kDValidMin || d_mm > kDValidMax)
                         ? qQNaN()
                         : static_cast<double>(d_mm);
    pushPoint(static_cast<double>(QDateTime::currentMSecsSinceEpoch()), d, false);
}

void DistancePlot::appendInvalid()
{
    /* faces != 1: 无效时段样本, 曲线断开并画红色竖直色带 */
    pushPoint(static_cast<double>(QDateTime::currentMSecsSinceEpoch()), qQNaN(), true);
}

void DistancePlot::notifyGap()
{
    if (tMs_.isEmpty() || (qIsNaN(dMm_.last()) && !invalid_.last())) {
        return;
    }
    /* 非 READY 到来: 闭合仍在进行的无效色带 (追加普通断点结束 invalid 连续段) */
    pushPoint(static_cast<double>(QDateTime::currentMSecsSinceEpoch()), qQNaN(), false);
}

void DistancePlot::pushPoint(double tMs, double dMm, bool invalid)
{
    tMs_.append(tMs);
    dMm_.append(dMm);
    invalid_.append(invalid);

    /* 只保留窗口 + 2s 的历史 (多留一点让线段从左侧滑入) */
    const double horizon = tMs_.last() - (kWindowMs + 2000.0);
    int drop = 0;
    while (drop < tMs_.size() - 1 && tMs_[drop] < horizon) {
        ++drop;
    }
    if (drop > 0) {
        tMs_.remove(0, drop);
        dMm_.remove(0, drop);
        invalid_.remove(0, drop);
    }
    update();
}

void DistancePlot::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF area = rect().adjusted(0, 0, -1, -1);
    const double mLeft = 58, mRight = 20, mTop = 14, mBottom = 30;
    const QRectF plot = area.adjusted(mLeft, mTop, -mRight, -mBottom);
    if (plot.width() < 40 || plot.height() < 40) {
        return;
    }

    /* 时间轴: 右端固定为最新样本时刻 (墙钟) */
    const double t1 = tMs_.isEmpty()
                          ? static_cast<double>(QDateTime::currentMSecsSinceEpoch())
                          : tMs_.last();
    const double t0 = t1 - kWindowMs;

    /* y 量程: 固定有效测距范围 [kDValidMin, kDValidMax] */
    const double dmin = kDValidMin;
    const double dmax = kDValidMax;
    bool hasData = false;
    bool hasInvalid = false;
    for (int i = 0; i < dMm_.size(); ++i) {
        if (tMs_[i] < t0) {
            continue;
        }
        if (!qIsNaN(dMm_[i])) {
            hasData = true;
        }
        if (invalid_[i]) {
            hasInvalid = true;
        }
        if (hasData && hasInvalid) {
            break;
        }
    }

    const auto X = [&](double t) {
        return plot.left() + (t - t0) / kWindowMs * plot.width();
    };
    const auto Y = [&](double d) {
        return plot.bottom() - (d - dmin) / (dmax - dmin) * plot.height();
    };

    /* 网格 + 刻度 */
    QFont f = p.font();
    f.setPointSizeF(f.pointSizeF() * 0.85);
    p.setFont(f);

    const QPen gridPen(QColor(0x45, 0x47, 0x5a), 1, Qt::DotLine);
    const QColor axisTextColor(0x8a, 0x8a, 0xa3);
    const QColor axisFrameColor(0x45, 0x47, 0x5a);
    const QColor curveColor(0x89, 0xb4, 0xfa);

    /* y 网格: 200mm 整刻度, 起点对齐到量程内首个 200 倍数 */
    for (double d = std::ceil(dmin / 200.0) * 200.0; d <= dmax + 0.5; d += 200.0) {
        const double y = Y(d);
        p.setPen(gridPen);
        p.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        p.setPen(QPen(axisTextColor));
        p.drawText(QRectF(plot.left() - 52, y - 9, 48, 18),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString::number(static_cast<int>(d + 0.5)));
    }
    /* x 网格: 5s 一格, 对齐绝对墙钟整刻度, 标签 HH:mm:ss */
    for (double tick = std::ceil(t0 / 5000.0) * 5000.0; tick <= t1; tick += 5000.0) {
        const double x = X(tick);
        if (x < plot.left() || x > plot.right()) {
            continue;
        }
        p.setPen(gridPen);
        p.drawLine(QPointF(x, plot.top()), QPointF(x, plot.bottom()));
        p.setPen(QPen(axisTextColor));
        p.drawText(QRectF(x - 32, plot.bottom() + 4, 64, 16),
                   Qt::AlignHCenter | Qt::AlignTop,
                   QDateTime::fromMSecsSinceEpoch(
                       static_cast<qint64>(tick)).toString(QStringLiteral("hh:mm:ss")));
    }

    /* 坐标轴框 + 单位 */
    p.setPen(QPen(axisFrameColor, 1));
    p.drawRect(plot);
    p.setPen(QPen(axisTextColor));
    p.drawText(QRectF(4, 2, 52, 16), Qt::AlignLeft | Qt::AlignTop, QStringLiteral("mm"));
    p.drawText(QRectF(plot.right() - 60, plot.bottom() + 4, 60, 16),
               Qt::AlignRight | Qt::AlignTop, QStringLiteral("t (s)"));

    /* 无效时段红色竖直色带 (faces != 1): 画在网格之上、曲线之下
     * 连续 invalid 样本合并为一段; 尾段右端天然到达最新样本时刻 (= 图右缘) */
    p.save();
    p.setClipRect(plot);
    p.setPen(Qt::NoPen);
    p.setBrush(kInvalidBandColor);
    int i = 0;
    while (i < invalid_.size()) {
        if (!invalid_[i] || tMs_[i] < t0) {
            ++i;
            continue;
        }
        int j = i;
        while (j + 1 < invalid_.size() && invalid_[j + 1]) {
            ++j;
        }
        double bandL = X(tMs_[i]);
        double bandR = X(tMs_[j]);
        if (bandR - bandL < 2.0) {
            bandR = bandL + 2.0;   /* 短暂无效也让色带可见 */
        }
        p.drawRect(QRectF(bandL, plot.top(), bandR - bandL, plot.height()));
        i = j + 1;
    }
    p.restore();

    /* 色带图例: 右上角小红块 + "无效" */
    if (hasInvalid) {
        p.setPen(Qt::NoPen);
        p.setBrush(kInvalidTextColor);
        QRectF swatch(plot.right() - 68, plot.top() + 5, 8, 8);
        p.drawRect(swatch);
        p.setPen(kInvalidTextColor);
        p.drawText(QRectF(swatch.right() + 4, plot.top() + 1, 56, 16),
                   Qt::AlignLeft | Qt::AlignVCenter, QStringLiteral("无效"));
    }

    if (!hasData) {
        p.setPen(QPen(axisTextColor));
        p.drawText(plot, Qt::AlignCenter,
                   hasInvalid
                       ? QStringLiteral("数据无效（人脸数 ≠ 1）...")
                       : QStringLiteral("等待 READY 状态的有效距离数据..."));
        return;
    }

    /* 曲线 (NaN 断点分段) + 当前值标注 */
    p.save();
    p.setClipRect(plot);
    QPainterPath path;
    bool pen = false;
    QPointF lastPt(0, 0);
    double lastD = qQNaN();
    for (int i = 0; i < tMs_.size(); ++i) {
        if (qIsNaN(dMm_[i])) {
            pen = false;
            continue;
        }
        const QPointF pt(X(tMs_[i]), Y(dMm_[i]));
        if (!pen) {
            path.moveTo(pt);
            pen = true;
        } else {
            path.lineTo(pt);
        }
        lastPt = pt;
        lastD = dMm_[i];
    }
    p.setPen(QPen(curveColor, 2));
    p.drawPath(path);

    if (!qIsNaN(lastD)) {
        p.setPen(Qt::NoPen);
        p.setBrush(curveColor);
        p.drawEllipse(lastPt, 3, 3);
        p.setPen(QPen(curveColor));
        const QString text = QString::number(static_cast<int>(lastD + 0.5))
                             + QStringLiteral(" mm");
        QRectF tb(lastPt.x() - 60, lastPt.y() - 22, 120, 18);
        if (tb.right() > plot.right()) {
            tb.moveRight(lastPt.x() - 8);
        }
        p.drawText(tb, Qt::AlignHCenter | Qt::AlignVCenter, text);
    }
    p.restore();
}
