// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/chat/SQLiteChatHistoryStore.h"
#include "sentinel/core/chat/SQLiteConversationStore.h"
#include "sentinel/core/memory/SQLiteMemoryStore.h"

#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QtTest>

using namespace sentinel::core;

class StartupTimeTest final : public QObject {
    Q_OBJECT

private slots:
    void measureColdStartStorageBoundary();
};

void StartupTimeTest::measureColdStartStorageBoundary() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QElapsedTimer timer;
    timer.start();

    SQLiteMemoryStore memoryStore{dir.filePath(QStringLiteral("memory.sqlite3"))};
    SQLiteChatHistoryStore chatStore{dir.filePath(QStringLiteral("chat_history.sqlite3"))};
    SQLiteConversationStore conversationStore{
        dir.filePath(QStringLiteral("conversations.sqlite3"))};

    const qint64 elapsedMs = timer.elapsed();

    QVERIFY(memoryStore.isAvailable());
    QVERIFY(chatStore.isAvailable());
    QVERIFY(conversationStore.status() == ConversationStoreStatus::Ready);

    qInfo().noquote() << "Cold-start storage initialization:" << elapsedMs << "ms";

    QVERIFY(elapsedMs < 5000);
}

QTEST_MAIN(StartupTimeTest)

#include "test_startup_time.moc"
