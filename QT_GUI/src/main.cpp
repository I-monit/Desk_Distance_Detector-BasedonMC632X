#include "mainwindow.h"
#include "algstate.h"

#include <QApplication>
#include <QIcon>
#include <QMetaType>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QtGlobal>
#include <cstdio>

static void logToFile(QtMsgType type, const QMessageLogContext &ctx, const QString &msg)
{
    static QFile logFile(QApplication::applicationDirPath() + "/qt_monitor.log");
    static bool opened = logFile.open(QIODevice::Append | QIODevice::Text);
    const QString line = QStringLiteral("[%1] %2: %3 (%4:%5)\n")
        .arg(QDateTime::currentDateTime().toString("HH:mm:ss.zzz"))
        .arg([&]{ switch(type){case QtDebugMsg:return "DEBUG";case QtWarningMsg:return "WARN";case QtCriticalMsg:return "CRIT";case QtFatalMsg:return "FATAL";case QtInfoMsg:return "INFO";} return "?"; }())
        .arg(msg).arg(ctx.function).arg(ctx.line);
    if (opened) {
        QTextStream ts(&logFile);
        ts << line;
        ts.flush();
    }
    fprintf(stderr, "%s", qUtf8Printable(line));
    if (type == QtFatalMsg) abort();
}

int main(int argc, char *argv[])
{
    // 方案B起改为 FFmpeg 自主拉流 (rtspstreamer.cpp), TCP 由拉流参数指定,
    // 不再使用 QMediaPlayer, QT_FFMPEG_RTSP_TRANSPORT 环境变量已无作用。
    qInstallMessageHandler(logToFile);
    qRegisterMetaType<AlgState>("AlgState");

    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("MC632X办公人员桌面测距仪"));
    app.setApplicationDisplayName(QStringLiteral("MC632X办公人员桌面测距仪"));
    app.setOrganizationName(QStringLiteral("MC632X"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/qt_monitor")));

    MainWindow w;
    w.setWindowIcon(QIcon(QStringLiteral(":/icons/qt_monitor")));
    w.show();
    return app.exec();
}
