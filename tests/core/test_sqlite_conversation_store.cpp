#include "sentinel/core/SQLiteChatHistoryStore.h"
#include "sentinel/core/SQLiteConversationStore.h"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>

using sentinel::core::ChatMessage;
using sentinel::core::ChatMessageStatus;
using sentinel::core::ChatRole;
using sentinel::core::ConversationMessageRecord;
using sentinel::core::ConversationStoreErrorCode;
using sentinel::core::ConversationStoreStatus;
using sentinel::core::SQLiteChatHistoryStore;
using sentinel::core::SQLiteConversationStore;

class SQLiteConversationStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void startsEmptyAndInitializesSchema();
    void createsListsAndLoadsConversation();
    void appendsAndLoadsMessagesInDeterministicOrder();
    void persistsConversationsAcrossInstances();
    void blocksAppendingToArchivedConversation();
    void softDeleteHidesConversation();
    void softDeleteMarksOnlyAndKeepsMessagesInDatabase();
    void safelyNoOpsForUnopenableDatabasePath();
    void doesNotMigrateOrClearSingleTranscriptStore();
};

static QString databasePath(QTemporaryDir& dir,
                            const QString& fileName = QStringLiteral("conversations.sqlite3")) {
    return dir.filePath(fileName);
}

static ConversationMessageRecord message(const QString& conversationId, int messageId,
                                         ChatRole role, const QString& content) {
    return ConversationMessageRecord{
        conversationId,
        messageId,
        role,
        content,
        QDateTime::fromString(QStringLiteral("2026-05-15T12:00:00.000Z"), Qt::ISODateWithMs),
        ChatMessageStatus::Received,
    };
}

static ChatMessage chatMessage(int id, ChatRole role, const QString& content) {
    return ChatMessage{
        id,
        role,
        content,
        QDateTime::fromString(QStringLiteral("2026-05-15T12:00:00.000Z"), Qt::ISODateWithMs),
        ChatMessageStatus::Received,
    };
}

void SQLiteConversationStoreTest::startsEmptyAndInitializesSchema() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteConversationStore store(databasePath(dir));

    QCOMPARE(store.status(), ConversationStoreStatus::Ready);
    QCOMPARE(store.schemaVersion(), 1);
    QVERIFY(store.listConversations().isEmpty());
}

void SQLiteConversationStoreTest::createsListsAndLoadsConversation() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteConversationStore store(databasePath(dir));

    const auto first = store.createConversation(QStringLiteral("Alpha"));
    const auto second = store.createConversation(QStringLiteral("Beta"));
    const auto conversations = store.listConversations();

    QCOMPARE(conversations.size(), 2);
    QCOMPARE(conversations.at(0).id, first.id);
    QCOMPARE(conversations.at(0).title, QStringLiteral("Alpha"));
    QCOMPARE(conversations.at(1).id, second.id);
    QCOMPARE(conversations.at(1).title, QStringLiteral("Beta"));
    QCOMPARE(store.lastError().code, ConversationStoreErrorCode::None);
}

void SQLiteConversationStoreTest::appendsAndLoadsMessagesInDeterministicOrder() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteConversationStore store(databasePath(dir));
    const auto conversation = store.createConversation(QStringLiteral("Ordered"));

    QVERIFY(store.appendMessage(
        message(conversation.id, 3, ChatRole::Assistant, QStringLiteral("third"))));
    QVERIFY(store.appendMessage(
        message(conversation.id, 1, ChatRole::System, QStringLiteral("first"))));
    QVERIFY(
        store.appendMessage(message(conversation.id, 2, ChatRole::User, QStringLiteral("second"))));

    const auto messages = store.loadMessages(conversation.id);
    QCOMPARE(messages.size(), 3);
    QCOMPARE(messages.at(0).messageId, 1);
    QCOMPARE(messages.at(1).messageId, 2);
    QCOMPARE(messages.at(2).messageId, 3);
    QCOMPARE(messages.at(1).content, QStringLiteral("second"));
    QCOMPARE(store.listConversations().first().messageCount, 3);
}

void SQLiteConversationStoreTest::persistsConversationsAcrossInstances() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = databasePath(dir);
    QString conversationId;

    {
        SQLiteConversationStore store(path);
        const auto conversation = store.createConversation(QStringLiteral("Persisted"));
        conversationId = conversation.id;
        QVERIFY(store.appendMessage(
            message(conversationId, 1, ChatRole::User, QStringLiteral("saved"))));
    }

    SQLiteConversationStore reloaded(path);
    const auto conversations = reloaded.listConversations();
    const auto messages = reloaded.loadMessages(conversationId);

    QCOMPARE(conversations.size(), 1);
    QCOMPARE(conversations.first().title, QStringLiteral("Persisted"));
    QCOMPARE(conversations.first().messageCount, 1);
    QCOMPARE(messages.size(), 1);
    QCOMPARE(messages.first().content, QStringLiteral("saved"));
}

void SQLiteConversationStoreTest::blocksAppendingToArchivedConversation() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteConversationStore store(databasePath(dir));
    const auto conversation = store.createConversation(QStringLiteral("Archived"));

    QVERIFY(store.archiveConversation(conversation.id));
    QVERIFY(!store.appendMessage(
        message(conversation.id, 1, ChatRole::User, QStringLiteral("blocked"))));
    QCOMPARE(store.lastError().code, ConversationStoreErrorCode::UnsupportedOperation);
    QVERIFY(store.loadMessages(conversation.id).isEmpty());
}

void SQLiteConversationStoreTest::softDeleteHidesConversation() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteConversationStore store(databasePath(dir));
    const auto first = store.createConversation(QStringLiteral("First"));
    const auto second = store.createConversation(QStringLiteral("Second"));

    QVERIFY(store.deleteConversation(first.id));

    const auto conversations = store.listConversations();
    QCOMPARE(conversations.size(), 1);
    QCOMPARE(conversations.first().id, second.id);
}

void SQLiteConversationStoreTest::softDeleteMarksOnlyAndKeepsMessagesInDatabase() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto path = databasePath(dir);
    QString conversationId;

    {
        SQLiteConversationStore store(path);
        const auto conversation = store.createConversation(QStringLiteral("Soft Delete"));
        conversationId = conversation.id;
        QVERIFY(store.appendMessage(
            message(conversationId, 1, ChatRole::System, QStringLiteral("system"))));
        QVERIFY(store.appendMessage(
            message(conversationId, 2, ChatRole::User, QStringLiteral("kept"))));

        QVERIFY(store.deleteConversation(conversationId));
        QVERIFY(store.listConversations().isEmpty());
        QVERIFY(store.loadMessages(conversationId).isEmpty());
        QCOMPARE(store.lastError().code, ConversationStoreErrorCode::InvalidConversationId);
    }

    const auto connectionName = QStringLiteral("sentinel_soft_delete_test_%1")
                                    .arg(QUuid::createUuid().toString(QUuid::Id128));
    auto database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), connectionName);
    database.setDatabaseName(path);
    QVERIFY(database.open());

    {
        QSqlQuery query(database);
        query.prepare(QStringLiteral("SELECT archived, deleted FROM conversations WHERE id = ?"));
        query.addBindValue(conversationId);
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QVERIFY(query.value(0).toBool());
        QVERIFY(query.value(1).toBool());
    }

    {
        QSqlQuery query(database);
        query.prepare(
            QStringLiteral("SELECT COUNT(*) FROM conversation_messages WHERE conversation_id = ?"));
        query.addBindValue(conversationId);
        QVERIFY(query.exec());
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 2);
    }

    database.close();
    database = {};
    QSqlDatabase::removeDatabase(connectionName);
}

void SQLiteConversationStoreTest::safelyNoOpsForUnopenableDatabasePath() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    SQLiteConversationStore store(dir.path());

    QCOMPARE(store.status(), ConversationStoreStatus::Unavailable);
    QVERIFY(!store.lastError().summary.isEmpty());
    QVERIFY(store.createConversation(QStringLiteral("ignored")).id.isEmpty());
    QVERIFY(!store.appendMessage(
        message(QStringLiteral("missing"), 1, ChatRole::User, QStringLiteral("ignored"))));
    QVERIFY(store.listConversations().isEmpty());
    QCOMPARE(store.schemaVersion(), 0);
}

void SQLiteConversationStoreTest::doesNotMigrateOrClearSingleTranscriptStore() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto chatPath = databasePath(dir, QStringLiteral("chat_history.sqlite3"));
    const auto conversationPath = databasePath(dir, QStringLiteral("conversations.sqlite3"));

    {
        SQLiteChatHistoryStore chatStore(chatPath);
        chatStore.appendMessage(chatMessage(1, ChatRole::System, QStringLiteral("online")));
        chatStore.appendMessage(chatMessage(2, ChatRole::User, QStringLiteral("existing")));
    }

    {
        SQLiteConversationStore conversationStore(conversationPath);
        QCOMPARE(conversationStore.schemaVersion(), 1);
        QVERIFY(conversationStore.listConversations().isEmpty());
    }

    SQLiteChatHistoryStore reloadedChatStore(chatPath);
    const auto messages = reloadedChatStore.loadMessages();
    QCOMPARE(messages.size(), 2);
    QCOMPARE(messages.at(1).content, QStringLiteral("existing"));
    QVERIFY(QFile::exists(chatPath));
    QVERIFY(QFile::exists(conversationPath));
}

QTEST_MAIN(SQLiteConversationStoreTest)

#include "test_sqlite_conversation_store.moc"
