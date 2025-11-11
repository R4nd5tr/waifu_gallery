#include "gui/main_window.h"
#include "utils/logger.h"
#include <QApplication>
#include <QDateTime>
#include <QFile>
#include <QLoggingCategory>
#include <QTextStream>

const qint64 MAX_LOG_FILE_SIZE = 5 * 1024 * 1024; // 5 MB
const int MAX_LOG_FILES = 5;

QString logFileName = "";

void setLogFile() {
    logFileName = "waifu_gallery.log";

    QFile logFile(logFileName);
    if (logFile.exists() && logFile.size() >= MAX_LOG_FILE_SIZE) {
        logFile.close();
        for (int i = MAX_LOG_FILES - 1; i >= 1; --i) {
            QString oldFileName = QString("waifu_gallery.log.%1").arg(i);
            QString newFileName = QString("waifu_gallery.log.%1").arg(i + 1);
            if (QFile::exists(oldFileName)) {
                QFile::rename(oldFileName, newFileName);
            }
        }
        QFile::rename(logFileName, "waifu_gallery.log.1");
    }
}

void customMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    QString logLevel;
    switch (type) {
    case QtDebugMsg:
        logLevel = "DEBUG";
        break;
    case QtInfoMsg:
        logLevel = "INFO";
        break;
    case QtWarningMsg:
        logLevel = "WARN";
        break;
    case QtCriticalMsg:
        logLevel = "ERROR";
        break;
    case QtFatalMsg:
        logLevel = "FATAL";
        break;
    }

    QString logMsg = QString("[%1] [%2] %3").arg(timestamp, logLevel, msg);
#ifdef QT_NO_DEBUG_OUTPUT // for release builds
    // 输出到文件
    if (logFileName.isEmpty()) {
        setLogFile();
    }
    QFile file(logFileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&file);
        stream << logMsg << "\n";
        file.close();
    }
#else
    // 输出到控制台
    fprintf(stderr, "%s\n", logMsg.toLocal8Bit().data());
#endif
}

int main(int argc, char* argv[]) {
    QLoggingCategory::setFilterRules("qt.gui.imageio=false\n"
                                     "qt.gui.icc=false\n"
                                     "qt.text.font.db=false\n"
                                     "qt.qpa.fonts=false");
    qInstallMessageHandler(customMessageHandler);
    Info() << "Application started.";
    QApplication app(argc, argv);
    MainWindow window;
    window.show();
    return app.exec();
}
