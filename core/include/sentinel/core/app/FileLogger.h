// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDir>
#include <QFile>
#include <QMutex>
#include <QString>
#include <QTextStream>
#include <QtGlobal>
#include <memory>

namespace sentinel::core {

class FileLogger final {
public:
    static FileLogger& instance();

    void initialize(const QString& logDir, int retentionDays = 0);
    void handleMessage(QtMsgType type, const QMessageLogContext& ctx, const QString& msg);
    QString currentLogFilePath() const;
    int applyRetention(int days);

private:
    Q_DISABLE_COPY(FileLogger)
    FileLogger() = default;
    ~FileLogger();
    void rotateLog();
    int cleanOldLogs();

    QDir logDir_;
    QFile logFile_;
    QTextStream logStream_;
    QDate currentLogDate_;
    int retentionDays_ = 0;
    mutable QMutex mutex_;
    bool initialized_ = false;
};

} // namespace sentinel::core
