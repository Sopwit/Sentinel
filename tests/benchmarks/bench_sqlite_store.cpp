// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>
#include "sentinel/core/memory/SQLiteMemoryStore.h"

#include <QTemporaryDir>

using namespace sentinel::core;

namespace {
constexpr int kWriteCount = 500;

QTemporaryDir makeTempDir() {
    QTemporaryDir dir;
    Q_ASSERT(dir.isValid());
    return dir;
}
} // namespace

class SqliteStoreBenchmark final : public QObject {
    Q_OBJECT

private slots:
    void benchmarkKeyValueWrites();
    void benchmarkKeyValueReads();
};

void SqliteStoreBenchmark::benchmarkKeyValueWrites() {
    const auto dir = makeTempDir();
    SQLiteMemoryStore store{dir.filePath(QStringLiteral("bench_memory.sqlite3"))};
    QVERIFY(store.isAvailable());

    QBENCHMARK {
        for (int i = 0; i < kWriteCount; ++i) {
            store.put(QStringLiteral("key-%1").arg(i), QStringLiteral("value-%1").arg(i));
        }
        store.clear();
    }
}

void SqliteStoreBenchmark::benchmarkKeyValueReads() {
    const auto dir = makeTempDir();
    SQLiteMemoryStore store{dir.filePath(QStringLiteral("bench_memory.sqlite3"))};
    for (int i = 0; i < kWriteCount; ++i) {
        store.put(QStringLiteral("key-%1").arg(i), QStringLiteral("value-%1").arg(i));
    }

    QBENCHMARK {
        const auto entries = store.entries();
        QVERIFY(entries.size() == kWriteCount);
    }
}

QTEST_MAIN(SqliteStoreBenchmark)

#include "bench_sqlite_store.moc"
