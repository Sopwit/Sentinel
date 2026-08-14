// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/flock/FlockFileLock.h"
#ifdef Q_OS_UNIX
#include <sys/file.h>
#include <utime.h>
#include <unistd.h>
#endif
#include <QFileInfo>

namespace sentinel::core {

FlockFileLock::Config::Config()
    : heartbeatIntervalMs(20000)
    , stalenessTimeoutMs(60000)
    , enableHeartbeat(true) {}

FlockFileLock::FlockFileLock(const QString& filePath, Config config)
    : m_filePath(filePath), m_config(std::move(config)) {}

FlockFileLock::~FlockFileLock() { unlock(); }

bool FlockFileLock::lock() {
    QMutexLocker locker(&m_mutex);
    if (m_locked) return true;

    m_lockFile = new QFile(m_filePath);
    if (m_lockFile->open(QIODevice::ReadWrite | QIODevice::Append)) {
#ifdef Q_OS_UNIX
        int fd = m_lockFile->handle();
        if (flock(fd, LOCK_EX) == 0) {
            m_locked = true;
            m_lockTimer.start();
            if (m_config.enableHeartbeat) {
                startHeartbeat();
            }
            return true;
        }
#else
        m_locked = true;
        m_lockTimer.start();
        return true;
#endif
    }
    delete m_lockFile;
    m_lockFile = nullptr;
    return false;
}

bool FlockFileLock::tryLock() {
    QMutexLocker locker(&m_mutex);
    if (m_locked) return true;

    m_lockFile = new QFile(m_filePath);
    if (m_lockFile->open(QIODevice::ReadWrite | QIODevice::Append)) {
#ifdef Q_OS_UNIX
        int fd = m_lockFile->handle();
        if (flock(fd, LOCK_EX | LOCK_NB) == 0) {
            m_locked = true;
            m_lockTimer.start();
            if (m_config.enableHeartbeat) {
                startHeartbeat();
            }
            return true;
        }
        delete m_lockFile;
        m_lockFile = nullptr;
        return false;
#else
        m_locked = true;
        m_lockTimer.start();
        return true;
#endif
    }
    delete m_lockFile;
    m_lockFile = nullptr;
    return false;
}

void FlockFileLock::unlock() {
    QMutexLocker locker(&m_mutex);
    if (m_locked && m_lockFile) {
        stopHeartbeat();
#ifdef Q_OS_UNIX
        flock(m_lockFile->handle(), LOCK_UN);
#endif
        m_lockFile->close();
        delete m_lockFile;
        m_lockFile = nullptr;
        m_locked = false;
    }
}

bool FlockFileLock::isLocked() const {
    QMutexLocker locker(&m_mutex);
    return m_locked;
}

bool FlockFileLock::isStale() const {
    QMutexLocker locker(&m_mutex);
    return checkStaleness();
}

QString FlockFileLock::filePath() const { return m_filePath; }

qint64 FlockFileLock::lockAgeMs() const {
    QMutexLocker locker(&m_mutex);
    if (!m_locked) return 0;
    return m_lockTimer.elapsed();
}

void FlockFileLock::startHeartbeat() {
    if (m_heartbeatTimer) return;
    m_heartbeatTimer = new QTimer();
    QObject::connect(m_heartbeatTimer, &QTimer::timeout, [this]() {
        updateHeartbeat();
    });
    m_heartbeatTimer->start(m_config.heartbeatIntervalMs);
}

void FlockFileLock::stopHeartbeat() {
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
        delete m_heartbeatTimer;
        m_heartbeatTimer = nullptr;
    }
}

void FlockFileLock::updateHeartbeat() {
#ifdef Q_OS_UNIX
    if (m_lockFile && m_locked) {
        utime(m_filePath.toUtf8().constData(), nullptr);
    }
#endif
}

bool FlockFileLock::checkStaleness() const {
    QFileInfo info(m_filePath);
    if (!info.exists()) return false;

    qint64 age = info.lastModified().msecsTo(QDateTime::currentDateTime());
    return age > m_config.stalenessTimeoutMs;
}

} // namespace sentinel::core
