// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/flock/FlockFileLock.h"

#include <QtTest>
#include <QTemporaryDir>

using sentinel::core::FlockFileLock;

class FlockFileLockTest final : public QObject {
    Q_OBJECT

private slots:
    void lockAndUnlock();
    void tryLock();
    void isStale();
    void lockAge();
};

void FlockFileLockTest::lockAndUnlock() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FlockFileLock lock(dir.filePath("test.lock"));
    QVERIFY(lock.lock());
    QVERIFY(lock.isLocked());
    lock.unlock();
    QVERIFY(!lock.isLocked());
}

void FlockFileLockTest::tryLock() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FlockFileLock lock(dir.filePath("test.lock"));
    QVERIFY(lock.tryLock());
    QVERIFY(lock.isLocked());
    lock.unlock();
}

void FlockFileLockTest::isStale() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FlockFileLock::Config config;
    config.enableHeartbeat = false;
    FlockFileLock lock(dir.filePath("test.lock"), config);
    QVERIFY(!lock.isStale());
}

void FlockFileLockTest::lockAge() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    FlockFileLock::Config config;
    config.enableHeartbeat = false;
    FlockFileLock lock(dir.filePath("test.lock"), config);
    lock.lock();
    QVERIFY(lock.lockAgeMs() >= 0);
    lock.unlock();
}

QTEST_MAIN(FlockFileLockTest)
#include "test_flock_file_lock.moc"
