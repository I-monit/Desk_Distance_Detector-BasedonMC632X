#include "logpanel.h"

#include <QPlainTextEdit>
#include <QToolButton>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QDateTime>
#include <QTextCursor>

LogPanel::LogPanel(QWidget *parent)
    : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    body_ = new QFrame(this);
    body_->setObjectName(QStringLiteral("logBody"));
    auto *bodyLayout = new QVBoxLayout(body_);
    bodyLayout->setContentsMargins(0, 0, 0, 0);
    bodyLayout->setSpacing(0);

    auto *toolbar = new QHBoxLayout;
    toolbar->setContentsMargins(0, 0, 0, 0);
    toolbar->setSpacing(6);
    auto *hint = new QLabel(QStringLiteral("最近事件 · 自动滚到底部"), body_);
    hint->setObjectName(QStringLiteral("logHint"));
    clearBtn_ = new QToolButton(body_);
    clearBtn_->setText(QStringLiteral("清空"));
    clearBtn_->setObjectName(QStringLiteral("logClearBtn"));
    connect(clearBtn_, &QToolButton::clicked, this, &LogPanel::clearLog);
    toolbar->addWidget(hint);
    toolbar->addStretch(1);
    toolbar->addWidget(clearBtn_);
    bodyLayout->addLayout(toolbar);

    edit_ = new QPlainTextEdit(body_);
    edit_->setObjectName(QStringLiteral("logEdit"));
    edit_->setReadOnly(true);
    edit_->setMaximumBlockCount(kMaxBlocks);
    edit_->setLineWrapMode(QPlainTextEdit::NoWrap);
    bodyLayout->addWidget(edit_, 1);

    root->addWidget(body_);
}

void LogPanel::setBodyVisible(bool visible)
{
    body_->setVisible(visible);
}

bool LogPanel::isBodyVisible() const
{
    return body_->isVisible();
}

void LogPanel::clearLog()
{
    edit_->clear();
}

void LogPanel::appendInfo(const QString &msg)
{
    appendLine(QStringLiteral("INFO"), QStringLiteral("#a6e3a1"), msg);
}

void LogPanel::appendWarn(const QString &msg)
{
    appendLine(QStringLiteral("WARN"), QStringLiteral("#fab387"), msg);
}

void LogPanel::appendError(const QString &msg)
{
    appendLine(QStringLiteral("ERR "), QStringLiteral("#f38ba8"), msg);
}

void LogPanel::appendLine(const QString &levelTag, const QString &color, const QString &msg)
{
    const QString ts = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss"));
    const QString line = QStringLiteral("<span style=\"color:%1;\">[%2] %3</span> %4")
                             .arg(color, ts, levelTag, msg.toHtmlEscaped());
    edit_->appendHtml(line);
    trimToMax();
}

void LogPanel::trimToMax()
{
    /* QPlainTextEdit::setMaximumBlockCount 已自动截断头部, 这里留作未来扩展 */
}
