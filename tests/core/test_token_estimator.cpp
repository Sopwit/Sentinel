// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/util/TokenEstimator.h"

#include <QtTest>

using Sentinel::TokenEstimator;

class TokenEstimatorTest final : public QObject {
    Q_OBJECT

private slots:
    void estimateEmpty();
    void estimateShort();
    void estimateLong();
    void estimateFromChars();
};

void TokenEstimatorTest::estimateEmpty() {
    QCOMPARE(TokenEstimator::estimateTokenCount(""), 0);
}

void TokenEstimatorTest::estimateShort() {
    QCOMPARE(TokenEstimator::estimateTokenCount("1234"), 1);
    QCOMPARE(TokenEstimator::estimateTokenCount("12345678"), 2);
}

void TokenEstimatorTest::estimateLong() {
    QString text(100, 'a');
    QCOMPARE(TokenEstimator::estimateTokenCount(text), 25);
}

void TokenEstimatorTest::estimateFromChars() {
    QCOMPARE(TokenEstimator::estimateFromChars(0), 0);
    QCOMPARE(TokenEstimator::estimateFromChars(4), 1);
    QCOMPARE(TokenEstimator::estimateFromChars(8), 2);
}

QTEST_MAIN(TokenEstimatorTest)
#include "test_token_estimator.moc"
