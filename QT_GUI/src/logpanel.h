#ifndef LOGPANEL_H
#define LOGPANEL_H

#include <QWidget>

class QPlainTextEdit;
class QToolButton;
class QFrame;

/* 系统日志面板: 业务事件 (连接/算法/告警/错误) 与 qInfo/qWarning 镜像统一展示.
 * 折叠由 logGroup 标题栏的切换按钮驱动 (setBodyVisible), 这里只负责内容呈现 */
class LogPanel : public QWidget
{
    Q_OBJECT
public:
    explicit LogPanel(QWidget *parent = nullptr);

    void appendInfo(const QString &msg);
    void appendWarn(const QString &msg);
    void appendError(const QString &msg);
    void clearLog();

    /* 折叠/展开内容区 (由 GroupBox 标题栏按钮调用) */
    void setBodyVisible(bool visible);
    bool isBodyVisible() const;

private:
    void appendLine(const QString &levelTag, const QString &color, const QString &msg);
    void trimToMax();

    QPlainTextEdit *edit_  = nullptr;
    QFrame         *body_  = nullptr;
    QToolButton    *clearBtn_ = nullptr;

    static constexpr int kMaxBlocks = 2000;
};

#endif // LOGPANEL_H
