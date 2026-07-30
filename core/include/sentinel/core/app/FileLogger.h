// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDir>
#include <QFile>
#include <QString>
#include <QMutex>
#include <QTextStream>
#include <memory>
#include <QtGlobal>

namespace sentinel::core {

class FileLogger final {
public:
    static FileLogger& instance();

    void initialize(const QString& logDir, int retentionDays = 30);
    void handleMessage(QtMsgType type, const QMessageLogContext& ctx, const QString& msg);
    QString currentLogFilePath() const;

private:
    Q_DISABLE_COPY(FileLogger)
    FileLogger() = default;
    ~FileLogger();
    void rotateLog();
    void cleanOldLogs();

    QDir logDir_;
    QFile logFile_;
    QTextStream logStream_;
    QDate currentLogDate_;
    int retentionDays_ = 30;
    mutable QMutex mutex_;
    bool initialized_ = false;
};

} // namespace sentinel::core
