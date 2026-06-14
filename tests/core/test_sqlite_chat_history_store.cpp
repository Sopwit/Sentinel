#include "sentinel/core/SQLiteChatHistoryStore.h"

#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using sentinel::core::ChatMessage;
using sentinel::core::ChatMessageStatus;
using sentinel::core::ChatRole;
using sentinel::core::SQLiteChatHistoryStore;

class SQLiteChatHistoryStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void startsEmpty();
    void appendsAndLoadsMessages();
    void clearsPersistedMessages();
    void preservesDeterministicOrdering();
    void persistsMessagesAcrossInstances();
    void persistsExecutionMetadata();
    void initializesSchemaVersion();
    void safelyNoOpsForUnopenableDatabasePath();
    void createsParentDirectories();
};

static QString databasePath(QTemporaryDir& dir,
                            const QString& fileName = QStringLiteral("chat_history.sqlite3")) {
    return dir.filePath(fileName);
}

static ChatMessage message(int id, ChatRole role, const QString& content,
                           ChatMessageStatus status = ChatMessageStatus::Received) {
    return ChatMessage{
        id,
        role,
        content,
        QDateTime::fromString(QStringLiteral("2026-05-15T12:00:00.000Z"), Qt::ISODateWithMs),
        status,
    };
}

void SQLiteChatHistoryStoreTest::startsEmpty() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteChatHistoryStore store(databasePath(dir));

    QVERIFY(store.loadMessages().isEmpty());
}

void SQLiteChatHistoryStoreTest::appendsAndLoadsMessages() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteChatHistoryStore store(databasePath(dir));

    store.appendMessage(message(1, ChatRole::System, QStringLiteral("online")));
    store.appendMessage(
        message(2, ChatRole::User, QStringLiteral("status"), ChatMessageStatus::Sent));
    store.appendMessage(message(3, ChatRole::Assistant, QStringLiteral("ready")));

    const auto messages = store.loadMessages();

    QCOMPARE(messages.size(), 3);
    QCOMPARE(messages.at(0).id, 1);
    QCOMPARE(messages.at(0).role, ChatRole::System);
    QCOMPARE(messages.at(1).role, ChatRole::User);
    QCOMPARE(messages.at(1).status, ChatMessageStatus::Sent);
    QCOMPARE(messages.at(2).role, ChatRole::Assistant);
    QCOMPARE(messages.at(2).content, QStringLiteral("ready"));
}

void SQLiteChatHistoryStoreTest::clearsPersistedMessages() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteChatHistoryStore store(databasePath(dir));

    store.appendMessage(message(1, ChatRole::System, QStringLiteral("online")));
    store.appendMessage(message(2, ChatRole::User, QStringLiteral("status")));
    store.clear();

    QVERIFY(store.loadMessages().isEmpty());
}

void SQLiteChatHistoryStoreTest::preservesDeterministicOrdering() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteChatHistoryStore store(databasePath(dir));

    store.appendMessage(message(3, ChatRole::Assistant, QStringLiteral("third")));
    store.appendMessage(message(1, ChatRole::System, QStringLiteral("first")));
    store.appendMessage(message(2, ChatRole::User, QStringLiteral("second")));

    const auto messages = store.loadMessages();

    QCOMPARE(messages.size(), 3);
    QCOMPARE(messages.at(0).id, 1);
    QCOMPARE(messages.at(1).id, 2);
    QCOMPARE(messages.at(2).id, 3);
}

void SQLiteChatHistoryStoreTest::persistsMessagesAcrossInstances() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = databasePath(dir);

    {
        SQLiteChatHistoryStore store(path);
        store.appendMessage(message(1, ChatRole::System, QStringLiteral("online")));
        store.appendMessage(
            message(2, ChatRole::User, QStringLiteral("status"), ChatMessageStatus::Sent));
    }

    SQLiteChatHistoryStore reloaded(path);
    const auto messages = reloaded.loadMessages();

    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages.at(0).content, QStringLiteral("online"));
    QCOMPARE(messages.at(1).content, QStringLiteral("status"));
}

void SQLiteChatHistoryStoreTest::persistsExecutionMetadata() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteChatHistoryStore store(databasePath(dir));

    auto assistant = message(1, ChatRole::Assistant, QStringLiteral("ready"));
    assistant.providerUsed = QStringLiteral("ollama");
    assistant.modelUsed = QStringLiteral("llama3.2");
    assistant.roleUsed = QStringLiteral("primary");
    assistant.responseDurationMs = 1200;
    assistant.firstTokenLatencyMs = 300;
    assistant.approximateTokensPerSecond = 12.5;
    store.appendMessage(assistant);

    const auto messages = store.loadMessages();

    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages.first().providerUsed, QStringLiteral("ollama"));
    QCOMPARE(messages.first().modelUsed, QStringLiteral("llama3.2"));
    QCOMPARE(messages.first().roleUsed, QStringLiteral("primary"));
    QCOMPARE(messages.first().responseDurationMs, 1200);
    QCOMPARE(messages.first().firstTokenLatencyMs, 300);
    QCOMPARE(messages.first().approximateTokensPerSecond, 12.5);
}

void SQLiteChatHistoryStoreTest::initializesSchemaVersion() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteChatHistoryStore store(databasePath(dir));

    QCOMPARE(store.schemaVersion(), 2);
}

void SQLiteChatHistoryStoreTest::safelyNoOpsForUnopenableDatabasePath() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteChatHistoryStore store(dir.path());

    QVERIFY(!store.isAvailable());
    QVERIFY(!store.lastError().isEmpty());

    store.appendMessage(message(1, ChatRole::User, QStringLiteral("ignored")));
    store.clear();

    QVERIFY(store.loadMessages().isEmpty());
    QCOMPARE(store.schemaVersion(), 0);
}

void SQLiteChatHistoryStoreTest::createsParentDirectories() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = databasePath(dir, QStringLiteral("nested/storage/chat_history.sqlite3"));

    SQLiteChatHistoryStore store(path);
    store.appendMessage(message(1, ChatRole::System, QStringLiteral("online")));

    QVERIFY(QFile::exists(path));
}

QTEST_MAIN(SQLiteChatHistoryStoreTest)

#include "test_sqlite_chat_history_store.moc"
