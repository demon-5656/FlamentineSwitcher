#include "flamentine_switcher/utils/logging.h"

#include <cstdio>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>

namespace {

QMutex g_logMutex;
QFile* g_logFile = nullptr;
FlamentineSwitcher::Core::LogLevel g_minimumLevel = FlamentineSwitcher::Core::LogLevel::Info;

int levelRank(const QtMsgType type) {
    switch (type) {
    case QtDebugMsg:
        return 1;
    case QtInfoMsg:
        return 2;
    case QtWarningMsg:
        return 3;
    case QtCriticalMsg:
    case QtFatalMsg:
        return 4;
    }

    return 2;
}

int levelRank(const FlamentineSwitcher::Core::LogLevel level) {
    switch (level) {
    case FlamentineSwitcher::Core::LogLevel::Trace:
    case FlamentineSwitcher::Core::LogLevel::Debug:
        return 1;
    case FlamentineSwitcher::Core::LogLevel::Info:
        return 2;
    case FlamentineSwitcher::Core::LogLevel::Warning:
        return 3;
    case FlamentineSwitcher::Core::LogLevel::Error:
        return 4;
    }

    return 2;
}

void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message) {
    if (levelRank(type) < levelRank(g_minimumLevel)) {
        return;
    }

    const QString typeName = [type]() {
        switch (type) {
        case QtDebugMsg:
            return QStringLiteral("DEBUG");
        case QtInfoMsg:
            return QStringLiteral("INFO");
        case QtWarningMsg:
            return QStringLiteral("WARN");
        case QtCriticalMsg:
            return QStringLiteral("ERROR");
        case QtFatalMsg:
            return QStringLiteral("FATAL");
        }

        return QStringLiteral("INFO");
    }();

    const QString rendered = QStringLiteral("%1 [%2] %3 (%4:%5, %6)\n")
                                 .arg(QDateTime::currentDateTime().toString(Qt::ISODate),
                                      typeName,
                                      message,
                                      QString::fromUtf8(context.file ? context.file : "?"),
                                      QString::number(context.line),
                                      QString::fromUtf8(context.function ? context.function : "?"));

    {
        QMutexLocker locker(&g_logMutex);
        std::fputs(rendered.toLocal8Bit().constData(), stderr);
        std::fflush(stderr);

        if (g_logFile != nullptr && g_logFile->isOpen()) {
            g_logFile->write(rendered.toUtf8());
            g_logFile->flush();
        }
    }

    if (type == QtFatalMsg) {
        std::abort();
    }
}

}  // namespace

namespace FlamentineSwitcher::Utils::Logging {

QString logFilePath() {
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return baseDir + QStringLiteral("/flamentine-switcher.log");
}

void initialize(const FlamentineSwitcher::Core::LoggingConfig& config) {
    QMutexLocker locker(&g_logMutex);
    g_minimumLevel = config.level;

    if (g_logFile != nullptr) {
        if (g_logFile->isOpen()) {
            g_logFile->close();
        }
        delete g_logFile;
        g_logFile = nullptr;
    }

    if (config.fileEnabled) {
        const QString path = logFilePath();
        QFileInfo info(path);
        QDir().mkpath(info.dir().absolutePath());

        g_logFile = new QFile(path);
        if (!g_logFile->open(QIODevice::Append | QIODevice::Text)) {
            delete g_logFile;
            g_logFile = nullptr;
        }
    }

    qSetMessagePattern(QStringLiteral("%{time yyyy-MM-dd hh:mm:ss.zzz} [%{type}] %{message}"));
    qInstallMessageHandler(messageHandler);
}

}  // namespace FlamentineSwitcher::Utils::Logging

