// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QElapsedTimer>
#include <QFile>
#include <QMutex>
#include <QString>
#include <QTimer>

namespace sentinel::core {

class FlockFileLock {
public:
    struct Config {
        int heartbeatIntervalMs;
        int stalenessTimeoutMs;
        bool enableHeartbeat;

        Config();
    };

    explicit FlockFileLock(const QString& filePath, Config config = Config());
    ~FlockFileLock();

    bool lock();
    bool tryLock();
    void unlock();
    bool isLocked() const;
    bool isStale() const;
    QString filePath() const;
    qint64 lockAgeMs() const;

private:
    void startHeartbeat();
    void stopHeartbeat();
    void updateHeartbeat();
    bool checkStaleness() const;

    QString m_filePath;
    Config m_config;
    QFile* m_lockFile{nullptr};
    bool m_locked{false};
    QElapsedTimer m_lockTimer;
    QTimer* m_heartbeatTimer{nullptr};
    mutable QMutex m_mutex;
};

} // namespace sentinel::core
