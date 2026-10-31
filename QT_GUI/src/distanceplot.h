#ifndef DISTANCEPLOT_H
#define DISTANCEPLOT_H

#include <QWidget>
#include <QVector>

/* D-t 实时曲线: y 轴为距离 D (mm) 固定量程 [200, 2000], x 轴为墙钟时间 t (HH:mm:ss), 滚动窗口 30s */
class DistancePlot : public QWidget
{
    Q_OBJECT
public:
    explicit DistancePlot(QWidget *parent = nullptr);

    void appendSample(int d_mm);   /* READY 期有效样本 (faces == 1, D >= 0) */
    void appendInvalid();          /* READY 但 faces != 1: 无效时段, 红色竖直色带 + 曲线断开 */
    void notifyGap();              /* 断线 / 非 READY: 曲线断开 (无色带) */

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void pushPoint(double tMs, double dMm, bool invalid);

    QVector<double> tMs_;          /* epoch ms */
    QVector<double> dMm_;          /* NaN 表示断点 */
    QVector<bool>   invalid_;      /* 与 tMs_ 同步: true = 无效时段样本 (红色色带) */
};

#endif // DISTANCEPLOT_H
