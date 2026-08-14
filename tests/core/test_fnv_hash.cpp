// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/util/FnvHash.h"

#include <QtTest>

using Sentinel::FnvHash;

class FnvHashTest final : public QObject {
    Q_OBJECT

private slots:
    void fnv1a32Basic();
    void fnv1a32Consistent();
    void fnv1a32Different();
    void fnv1a32Hex();
    void fnv1a32Base36();
    void fnv1a32Combine();
};

void FnvHashTest::fnv1a32Basic() {
    QVERIFY(FnvHash::fnv1a32(QByteArray("")) != 0);
}

void FnvHashTest::fnv1a32Consistent() {
    QCOMPARE(FnvHash::fnv1a32(QStringLiteral("hello")), FnvHash::fnv1a32(QStringLiteral("hello")));
}

void FnvHashTest::fnv1a32Different() {
    QVERIFY(FnvHash::fnv1a32(QStringLiteral("hello")) != FnvHash::fnv1a32(QStringLiteral("world")));
}

void FnvHashTest::fnv1a32Hex() {
    QString hex = FnvHash::fnv1a32Hex(QByteArray("test"));
    QVERIFY(!hex.isEmpty());
}

void FnvHashTest::fnv1a32Base36() {
    QString b36 = FnvHash::fnv1a32Base36(QByteArray("test"));
    QVERIFY(!b36.isEmpty());
}

void FnvHashTest::fnv1a32Combine() {
    uint32_t h1 = FnvHash::fnv1a32(QByteArray("a"));
    uint32_t h2 = FnvHash::fnv1a32(QByteArray("b"));
    uint32_t combined = FnvHash::fnv1a32Combine(h1, h2);
    QVERIFY(combined != h1);
    QVERIFY(combined != h2);
}

QTEST_MAIN(FnvHashTest)
#include "test_fnv_hash.moc"
