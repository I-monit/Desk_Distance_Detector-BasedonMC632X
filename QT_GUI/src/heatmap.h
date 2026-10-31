#ifndef HEATMAP_H
#define HEATMAP_H

#include <QWidget>
#include <QVector>

/* 距离分布饼图: 4 个距离扇区 + 1 个无效扇区 (faces != 1 / 非 READY 时段) 按样本数与时长累积.
 * 默认未开启分析, 调用 onSample/onInvalidSample 不会计入; setAnalyzing(true) 后开始累计
 * 重新开启会清空历史. 区间边界可通过 setRanges 配置 (重建扇区并清零累计). */
class Heatmap : public QWidget
{
    Q_OBJECT
public:
    explicit Heatmap(QWidget *parent = nullptr);

    bool isAnalyzing() const { return analyzing_; }
    void setAnalyzing(bool on);

    void setRanges(int b1, int b2, int b3); /* 4 距离扇区 = [0,b1] [b1+1,b2] [b2+1,b3] [b3+1,∞), 清零累计 */
    void onSample(int d_mm);       /* 有效样本 (faces == 1) */
    void onInvalidSample();        /* 无效样本 (faces != 1 / 非 READY): 计入无效扇区 */
    void reset();

    int  sampleCount() const { return static_cast<int>(totalCount_); }

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    struct Sector {
        int      dMin;
        int      dMax;        /* INT_MAX 表示 ">xxx" */
        QString  label;       /* 距离区间, 例 "0-299 mm" */
        QString  colorHex;
        qint64   count  = 0;  /* 落入此扇区的样本数 */
        qint64   timeMs = 0;  /* 累计停留时长 (ms) */
    };

    int  sectorIndex(int d_mm) const;
    void accumulate(int idx, qint64 nowMs);  /* 时长归段 + 计数公共路径 */
    void drawSector(QPainter &p, const QRectF &circle, int idx, double startDeg);
    void drawLegend(QPainter &p, const QRectF &box);
    QString formatDuration(qint64 ms) const;

    QVector<Sector> sectors_;
    bool    analyzing_   = false;
    qint64  totalCount_  = 0;
    qint64  lastSampleMs_ = 0;
    int     lastSector_  = -1;
    int     invalidIdx_  = -1;   /* 无效扇区下标 (构造时确定) */
};

#endif // HEATMAP_H
