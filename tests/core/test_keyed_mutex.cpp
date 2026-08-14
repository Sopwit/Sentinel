// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/sync/KeyedMutex.h"

#include <QtTest>

using Sentinel::KeyedMutex;

class KeyedMutexTest final : public QObject {
    Q_OBJECT

private slots:
    void lockAndUnlock();
    void tryLockDifferentKeys();
    void lockerRAII();
};

void KeyedMutexTest::lockAndUnlock() {
    KeyedMutex km;
    km.lock("a");
    QVERIFY(km.tryLock("b", 100));
    km.unlock("a");
    km.unlock("b");
}

void KeyedMutexTest::tryLockDifferentKeys() {
    KeyedMutex km;
    QVERIFY(km.tryLock("x", 100));
    QVERIFY(km.tryLock("y", 100));
    km.unlock("x");
    km.unlock("y");
}

void KeyedMutexTest::lockerRAII() {
    KeyedMutex km;
    {
        KeyedMutex::Locker lock1(km, "a");
        QVERIFY(km.tryLock("b", 100));
    }
    QVERIFY(km.tryLock("a", 100));
    km.unlock("a");
}

QTEST_MAIN(KeyedMutexTest)
#include "test_keyed_mutex.moc"
