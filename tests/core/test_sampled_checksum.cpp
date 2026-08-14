// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/util/SampledChecksum.h"

#include <QtTest>

using Sentinel::SampledChecksum;

class SampledChecksumTest final : public QObject {
    Q_OBJECT

private slots:
    void smallData();
    void emptyData();
    void hasChanged();
    void largeData();
};

void SampledChecksumTest::smallData() {
    QByteArray data = "hello world";
    auto result = SampledChecksum::compute(data);
    QVERIFY(result.hash != 0);
    QCOMPARE(result.fileSize, static_cast<qint64>(data.size()));
}

void SampledChecksumTest::emptyData() {
    QByteArray data;
    auto result = SampledChecksum::compute(data);
    QVERIFY(result.hash != 0);
    QCOMPARE(result.fileSize, static_cast<qint64>(0));
}

void SampledChecksumTest::hasChanged() {
    QByteArray a = "hello";
    QByteArray b = "world";
    QVERIFY(SampledChecksum::hasChanged(a, b));
    QVERIFY(!SampledChecksum::hasChanged(a, a));
}

void SampledChecksumTest::largeData() {
    QByteArray largeData(100000, 'x');
    auto result = SampledChecksum::compute(largeData);
    QVERIFY(result.hash != 0);
    QCOMPARE(result.fileSize, static_cast<qint64>(100000));
}

QTEST_MAIN(SampledChecksumTest)
#include "test_sampled_checksum.moc"
