// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/util/MemoMap.h"

#include <QtTest>

using Sentinel::MemoMap;
using Sentinel::SharedMemoMap;

class MemoMapTest final : public QObject {
    Q_OBJECT

private slots:
    void basicGet();
    void memoizes();
    void hasAndInvalidate();
    void invalidateAll();
    void sharedMemoMap();
};

void MemoMapTest::basicGet() {
    MemoMap<QString, int> memo([](const QString &key) {
        return key.length();
    });
    QCOMPARE(memo.get("hello"), 5);
}

void MemoMapTest::memoizes() {
    int callCount = 0;
    MemoMap<QString, int> memo([&callCount](const QString &key) {
        callCount++;
        return key.length();
    });
    memo.get("hello");
    QCOMPARE(callCount, 1);
    memo.get("hello");
    QCOMPARE(callCount, 1);
    memo.get("world");
    QCOMPARE(callCount, 2);
}

void MemoMapTest::hasAndInvalidate() {
    MemoMap<QString, int> memo([](const QString &key) {
        return key.length();
    });
    memo.get("test");
    QVERIFY(memo.has("test"));
    memo.invalidate("test");
    QVERIFY(!memo.has("test"));
}

void MemoMapTest::invalidateAll() {
    MemoMap<QString, int> memo([](const QString &key) {
        return key.length();
    });
    memo.get("a");
    memo.get("b");
    QCOMPARE(memo.size(), 2);
    memo.invalidateAll();
    QCOMPARE(memo.size(), 0);
}

void MemoMapTest::sharedMemoMap() {
    SharedMemoMap<QString, int> memo([](const QString &key) {
        return key.length();
    });
    auto ptr1 = memo.get("hello");
    auto ptr2 = memo.get("hello");
    QCOMPARE(*ptr1, 5);
    QVERIFY(ptr1 == ptr2);
}

QTEST_MAIN(MemoMapTest)
#include "test_memo_map.moc"
