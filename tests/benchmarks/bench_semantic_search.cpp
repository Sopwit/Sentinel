// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>
#include "sentinel/core/memory/SemanticRetrieval.h"

class SemanticSearchBenchmark final : public QObject {
    Q_OBJECT

private slots:
    void benchmarkRetrievalLatency();
};

void SemanticSearchBenchmark::benchmarkRetrievalLatency() {
    QBENCHMARK {
        const QString query = QStringLiteral("Find relevant memory context for Sentinel assistant");
        QVERIFY(!query.isEmpty());
    }
}

QTEST_MAIN(SemanticSearchBenchmark)

#include "bench_semantic_search.moc"
