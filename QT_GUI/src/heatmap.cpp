#include "heatmap.h"

#include <QPainter>
#include <QPainterPath>
#include <QDateTime>
#include <QFontMetrics>
#include <QtMath>
#include <climits>

namespace {
constexpr double kTwoPi = 2.0 * M_PI;
/* 起始角度 12 点方向, 顺时针展开, 与传统饼图一致 */
constexpr double kStartDeg = -90.0;
}

Heatmap::Heatmap(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(280, 160);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    setRanges(299, 499, 700); /* 默认区间, 可由 PC 端输入配置 */
}

void Heatmap::setRanges(int b1, int b2, int b3)
{
    /* 4 个距离扇区由 3 个上边界定义, 无效扇区固定在末位;
     * 区间定义变化后原累计失义, 一并清零 */
    sectors_.clear();
    sectors_.append({   0,  b1,      QStringLiteral("0-%1 mm").arg(b1),
                      QStringLiteral("#f38ba8"), 0, 0 });
    sectors_.append({ b1 + 1, b2,      QStringLiteral("%1-%2 mm").arg(b1 + 1).arg(b2),
                      QStringLiteral("#fab387"), 0, 0 });
    sectors_.append({ b2 + 1, b3,      QStringLiteral("%1-%2 mm").arg(b2 + 1).arg(b3),
                      QStringLiteral("#a6e3a1"), 0, 0 });
    sectors_.append({ b3 + 1, INT_MAX, QStringLiteral(">%1 mm").arg(b3),
                      QStringLiteral("#89b4fa"), 0, 0 });
    sectors_.append({  -1,   -1, QStringLiteral("无效"),       QStringLiteral("#6c7086"), 0, 0 });
    invalidIdx_ = sectors_.size() - 1;

    reset();
    update();
}

int Heatmap::sectorIndex(int d_mm) const
{
    for (int i = 0; i < sectors_.size(); ++i) {
        if (d_mm >= sectors_[i].dMin && d_mm <= sectors_[i].dMax) {
            return i;
        }
    }
    /* 负值/异常: 不计入, 落到最后一扇区 (>700) 之前, 选最末一档作兜底 */
    return -1;
}

void Heatmap::setAnalyzing(bool on)
{
    if (on == analyzing_) {
        return;
    }
    analyzing_ = on;
    if (on) {
        reset();
    } else {
        /* 停止时把最后一段 in-flight 时长丢弃, 保持累计与样本到达同步 */
        lastSector_ = -1;
    }
    update();
}

void Heatmap::reset()
{
    for (auto &s : sectors_) {
        s.count  = 0;
        s.timeMs = 0;
    }
    totalCount_   = 0;
    lastSampleMs_ = 0;
    lastSector_   = -1;
}

void Heatmap::accumulate(int idx, qint64 nowMs)
{
    /* 上一段停留时长归入上一扇区, 本样本开启新时段 */
    if (lastSector_ >= 0 && lastSampleMs_ > 0) {
        sectors_[lastSector_].timeMs += qMax<qint64>(0, nowMs - lastSampleMs_);
    }
    sectors_[idx].count += 1;
    totalCount_ += 1;
    lastSector_  = idx;
    lastSampleMs_ = nowMs;
}

void Heatmap::onSample(int d_mm)
{
    if (!analyzing_) {
        return;
    }
    const int idx = sectorIndex(d_mm);
    if (idx < 0) {
        return;
    }
    accumulate(idx, QDateTime::currentMSecsSinceEpoch());
    update();
}

void Heatmap::onInvalidSample()
{
    if (!analyzing_) {
        return;
    }
    accumulate(invalidIdx_, QDateTime::currentMSecsSinceEpoch());
    update();
}

QString Heatmap::formatDuration(qint64 ms) const
{
    if (ms < 0) {
        ms = 0;
    }
    const qint64 s = ms / 1000;
    if (s < 60) {
        return QStringLiteral("%1s").arg(s);
    }
    const qint64 m = s / 60;
    const qint64 r = s % 60;
    if (m < 60) {
        return QStringLiteral("%1m%2s").arg(m).arg(r, 2, 10, QChar('0'));
    }
    const qint64 h = m / 60;
    const qint64 rm = m % 60;
    return QStringLiteral("%1h%2m").arg(h).arg(rm, 2, 10, QChar('0'));
}

void Heatmap::drawSector(QPainter &p, const QRectF &pieRect, int idx, double startDeg)
{
    const Sector &s = sectors_[idx];
    const double frac = (totalCount_ > 0)
                            ? static_cast<double>(s.count) / static_cast<double>(totalCount_)
                            : 0.0;
    const double span = frac * 360.0;

    const QColor base(s.colorHex);
    if (totalCount_ == 0) {
        /* 未分析 / 无数据: 画一圈淡色环提示 */
        p.setPen(QPen(QColor(0x45, 0x47, 0x5a), 1));
        p.setBrush(QColor(0x30, 0x32, 0x44));
        p.drawEllipse(pieRect);
        return;
    }

    /* 扇形主体 (饼内不再画任何文字, 全部信息交给右侧表格) */
    p.setPen(QPen(QColor(0x1e, 0x1e, 0x2e), 1.5));
    p.setBrush(base);
    p.drawPie(pieRect, static_cast<int>(startDeg * 16), static_cast<int>(-span * 16));
}

void Heatmap::drawLegend(QPainter &p, const QRectF &box)
{
    /* 饼图右侧表格: 每行 = 距离范围 + 时长 + 占比 + 占比条 */
    QFont fBase = p.font();
    QFont fRange = fBase;
    QFont fValue = fBase;
    fValue.setPointSizeF(fValue.pointSizeF() - 0.5);
    QFont fPct   = fValue;
    fPct.setBold(true);

    const int rows = sectors_.size();
    if (rows <= 0) {
        return;
    }

    const double rowH = box.height() / rows;
    const double padX = 4.0;
    const double swatchSize = 10.0;
    const double swatchGap  = 8.0;
    const double barH = 5.0;
    const double textRowH = rowH * 0.55;   /* 文字行高 */
    const double barTopOffset = rowH * 0.62;  /* 占比条顶部相对 cell 顶部的偏移 */

    /* 列几何: 左 → 色块 + 区间名, 右 → 时长 + 占比 (右对齐) */
    const double colSwatchX = box.left() + padX;
    const double colRangeX  = colSwatchX + swatchSize + swatchGap;
    const double colBarLeft = colRangeX;
    const double colPctRight = box.right() - padX;
    const double colPctW = 50.0;
    const double colTimeRight = colPctRight - colPctW - 6.0;
    const double colTimeW = 46.0;
    const double colBarRight = colTimeRight - colTimeW - 10.0;
    const double barMaxW = qMax(40.0, colBarRight - colBarLeft);

    for (int i = 0; i < rows; ++i) {
        const Sector &s = sectors_[i];
        const QRectF cell(box.left(), box.top() + i * rowH, box.width(), rowH);
        const double midY = cell.center().y();
        const double frac = (totalCount_ > 0)
                                ? static_cast<double>(s.count) / static_cast<double>(totalCount_)
                                : 0.0;

        /* 色块 */
        QRectF swatch(colSwatchX, midY - swatchSize / 2.0 - 2.0,
                      swatchSize, swatchSize);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(s.colorHex));
        p.drawRoundedRect(swatch, 2, 2);

        /* 区间名 (左) */
        p.setFont(fRange);
        p.setPen(QColor(0xcd, 0xd6, 0xf4));
        p.drawText(QRectF(colRangeX, cell.top(), colBarRight - colRangeX, textRowH),
                   Qt::AlignLeft | Qt::AlignVCenter, s.label);

        /* 时长 (右, 上半行) */
        p.setFont(fValue);
        p.setPen(QColor(0xa6, 0xad, 0xc1));
        p.drawText(QRectF(colTimeRight - colTimeW, cell.top(), colTimeW, textRowH),
                   Qt::AlignRight | Qt::AlignVCenter, formatDuration(s.timeMs));

        /* 占比 (右, 上半行, 粗体高亮) */
        p.setFont(fPct);
        p.setPen(QColor(0xe4, 0xe4, 0xf4));
        const QString pctTxt = QString::number(frac * 100.0, 'f', 1) + QStringLiteral("%");
        p.drawText(QRectF(colPctRight - colPctW, cell.top(), colPctW, textRowH),
                   Qt::AlignRight | Qt::AlignVCenter, pctTxt);

        /* 占比条: 暗底 + 扇区色填充 (按本扇区占比) */
        const QRectF barBg(colBarLeft, cell.top() + barTopOffset, barMaxW, barH);
        p.setBrush(QColor(0x30, 0x32, 0x44));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(barBg, 2, 2);

        if (frac > 0.0) {
            const QRectF barFg(barBg.left(), barBg.top(),
                               barBg.width() * frac, barBg.height());
            p.setBrush(QColor(s.colorHex));
            p.drawRoundedRect(barFg, 2, 2);
        }
    }
}

void Heatmap::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QRectF area = rect().adjusted(8, 8, -8, -8);
    if (area.width() < 80 || area.height() < 60) {
        return;
    }

    /* 左: 圆饼; 右: 范围/时长/占比/占比条 表格
     * 表格列较多, 给到约 50% 宽度 (下限 200, 上限 280) */
    const double legendW = qBound(200.0, area.width() * 0.50, 280.0);
    const QRectF legendBox(area.right() - legendW, area.top(),
                           legendW, area.height());
    const QRectF pieArea(area.left(), area.top(),
                         area.width() - legendW - 8.0, area.height());

    /* 中心正方形 (保证圆不扁) */
    const double side = qMin(pieArea.width(), pieArea.height());
    const QRectF circle(
        pieArea.left() + (pieArea.width() - side) / 2.0,
        pieArea.top()  + (pieArea.height() - side) / 2.0,
        side, side);
    const QRectF circleInset = circle.adjusted(2, 2, -2, -2);

    /* 圆周描边 */
    p.setPen(QPen(QColor(0x45, 0x47, 0x5a), 1));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(circleInset);

    /* 各扇形: 按当前 count 比例展开, 起始 12 点 (饼内无文字) */
    double cursor = kStartDeg;
    if (totalCount_ > 0) {
        for (int i = 0; i < sectors_.size(); ++i) {
            const double frac = static_cast<double>(sectors_[i].count)
                                / static_cast<double>(totalCount_);
            const double span = frac * 360.0;
            if (span <= 0.0) {
                continue;
            }
            drawSector(p, circleInset, i, cursor);
            cursor -= span;
        }
    } else {
        drawSector(p, circleInset, 0, kStartDeg);
    }

    drawLegend(p, legendBox);
}
