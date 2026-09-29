// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/app/FileLogger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>

#if defined(Q_OS_MACOS) || defined(__APPLE__)
#include <os/log.h>
#endif

namespace sentinel::core {

FileLogger& FileLogger::instance() {
    static FileLogger inst;
    return inst;
}

FileLogger::~FileLogger() {
    if (logFile_.isOpen()) {
        logStream_.flush();
        logFile_.close();
    }
}

void FileLogger::initialize(const QString& logDir, int retentionDays) {
    QMutexLocker lock(&mutex_);
    logDir_ = QDir(logDir);
    retentionDays_ = retentionDays;

    if (!logDir_.exists()) {
        logDir_.mkpath(QStringLiteral("."));
        QFile::setPermissions(logDir_.absolutePath(),
                              QFileDevice::ReadOwner | QFileDevice::WriteOwner |
                                  QFileDevice::ExeOwner);
    }

    rotateLog();
    cleanOldLogs();
    initialized_ = true;
}

void FileLogger::rotateLog() {
    const QDate today = QDate::currentDate();
    if (currentLogDate_ == today && logFile_.isOpen()) {
        return;
    }

    if (logFile_.isOpen()) {
        logStream_.flush();
        logFile_.close();
    }

    currentLogDate_ = today;
    const QString fileName =
        QStringLiteral("sentinel-%1.log").arg(today.toString(QStringLiteral("yyyy-MM-dd")));
    const QString filePath = logDir_.filePath(fileName);

    logFile_.setFileName(filePath);
    if (!logFile_.open(QIODevice::Append | QIODevice::Text)) {
        qWarning().noquote() << "FileLogger: cannot open log file:" << filePath;
        return;
    }
    QFile::setPermissions(filePath, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    logStream_.setDevice(&logFile_);
}

int FileLogger::cleanOldLogs() {
    if (retentionDays_ <= 0) return 0;
    const QDate cutoff = QDate::currentDate().addDays(-retentionDays_);
    int removed = 0;
    bool failed = false;
    QDirIterator it(logDir_.absolutePath(), QStringList{QStringLiteral("sentinel-*.log")},
                    QDir::Files);
    while (it.hasNext()) {
        it.next();
        const QFileInfo info = it.fileInfo();
        static const QRegularExpression name(QStringLiteral("^sentinel-\\d{4}-\\d{2}-\\d{2}\\.log$"));
        if (!name.match(info.fileName()).hasMatch()) continue;
        if (!info.isSymLink() && info.absoluteFilePath() != logFile_.fileName() &&
            info.lastModified().date() < cutoff) {
            if (QFile::remove(info.absoluteFilePath())) ++removed;
            else failed = true;
        }
    }
    return failed ? -1 : removed;
}

int FileLogger::applyRetention(int days) {
    QMutexLocker lock(&mutex_);
    if (!initialized_ || days < 0 || days > 3650) return -1;
    retentionDays_ = days;
    return cleanOldLogs();
}

static const char* levelPrefix(QtMsgType type) {
    switch (type) {
    case QtDebugMsg:
        return "DBG";
    case QtInfoMsg:
        return "INF";
    case QtWarningMsg:
        return "WRN";
    case QtCriticalMsg:
        return "CRT";
    case QtFatalMsg:
        return "FTL";
    }
    return "???";
}

void FileLogger::handleMessage(QtMsgType type, const QMessageLogContext& ctx, const QString& msg) {
    QMutexLocker lock(&mutex_);
    rotateLog();

    const QString timestamp =
        QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd hh:mm:ss.zzz"));
    const QString category =
        ctx.category ? QString::fromLatin1(ctx.category) : QStringLiteral("default");
    const QString line = QStringLiteral("%1 [%2] [%3] %4")
                             .arg(timestamp, QString::fromLatin1(levelPrefix(type)), category, msg);

    // Write to file
    if (logFile_.isOpen()) {
        logStream_ << line << Qt::endl;
    }

#if defined(Q_OS_MACOS) || defined(__APPLE__)
    static os_log_t osLog = os_log_create("dev.sentinel.Sentinel", "app");
    os_log_type_t osLogType = OS_LOG_TYPE_DEFAULT;
    switch (type) {
    case QtDebugMsg:
        osLogType = OS_LOG_TYPE_DEBUG;
        break;
    case QtInfoMsg:
        osLogType = OS_LOG_TYPE_INFO;
        break;
    case QtWarningMsg:
        osLogType = OS_LOG_TYPE_DEFAULT;
        break;
    case QtCriticalMsg:
        osLogType = OS_LOG_TYPE_ERROR;
        break;
    case QtFatalMsg:
        osLogType = OS_LOG_TYPE_FAULT;
        break;
    }
    os_log_with_type(osLog, osLogType, "%{public}s", msg.toUtf8().constData());
#endif
}

QString FileLogger::currentLogFilePath() const {
    QMutexLocker lock(&mutex_);
    return logFile_.fileName();
}

} // namespace sentinel::core
