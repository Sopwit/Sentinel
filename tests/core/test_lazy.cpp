// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/util/Lazy.h"

#include <QtTest>

using Sentinel::Lazy;
using Sentinel::LazyPtr;

class LazyTest final : public QObject {
    Q_OBJECT

private slots:
    void basicGet();
    void isLoaded();
    void reset();
    void lazyPtr();
};

void LazyTest::basicGet() {
    int callCount = 0;
    Lazy<int> lazy([&callCount]() {
        callCount++;
        return 42;
    });
    QCOMPARE(lazy.get(), 42);
    QCOMPARE(callCount, 1);
    QCOMPARE(lazy.get(), 42);
    QCOMPARE(callCount, 1);
}

void LazyTest::isLoaded() {
    Lazy<int> lazy([]() { return 42; });
    QVERIFY(!lazy.isLoaded());
    lazy.get();
    QVERIFY(lazy.isLoaded());
}

void LazyTest::reset() {
    int callCount = 0;
    Lazy<int> lazy([&callCount]() {
        callCount++;
        return 42;
    });
    lazy.get();
    QCOMPARE(callCount, 1);
    lazy.reset();
    QVERIFY(!lazy.isLoaded());
    lazy.get();
    QCOMPARE(callCount, 2);
}

void LazyTest::lazyPtr() {
    LazyPtr<int> lazy([]() {
        return std::make_shared<int>(42);
    });
    auto ptr = lazy.get();
    QCOMPARE(*ptr, 42);
    QVERIFY(lazy.isLoaded());
    auto ptr2 = lazy.get();
    QVERIFY(ptr == ptr2);
}

QTEST_MAIN(LazyTest)
#include "test_lazy.moc"
