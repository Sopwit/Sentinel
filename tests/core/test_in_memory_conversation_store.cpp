#include "sentinel/core/InMemoryConversationStore.h"

#include <QtTest>

using sentinel::core::ChatMessageStatus;
using sentinel::core::ChatRole;
using sentinel::core::ConversationMessageRecord;
using sentinel::core::ConversationStoreErrorCode;
using sentinel::core::ConversationStoreStatus;
using sentinel::core::InMemoryConversationStore;

class InMemoryConversationStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void createsListsAndLoadsConversation();
    void appendsAndLoadsMessagesInDeterministicOrder();
    void pinsAndUnpinsConversationMetadata();
    void rejectsMissingConversation();
    void archivesAndDeletesWithoutRemovingOtherConversations();
};

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

void InMemoryConversationStoreTest::createsListsAndLoadsConversation() {
    InMemoryConversationStore store;

    QCOMPARE(store.status(), ConversationStoreStatus::Ready);

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

void InMemoryConversationStoreTest::appendsAndLoadsMessagesInDeterministicOrder() {
    InMemoryConversationStore store;
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
    QCOMPARE(store.listConversations().first().messageCount, 3);
}

void InMemoryConversationStoreTest::pinsAndUnpinsConversationMetadata() {
    InMemoryConversationStore store;
    const auto conversation = store.createConversation(QStringLiteral("Pinned"));

    QVERIFY(store.pinConversation(conversation.id));
    QCOMPARE(store.listConversations().first().pinned, true);

    QVERIFY(store.unpinConversation(conversation.id));
    QCOMPARE(store.listConversations().first().pinned, false);
}

void InMemoryConversationStoreTest::rejectsMissingConversation() {
    InMemoryConversationStore store;

    QVERIFY(!store.appendMessage(
        message(QStringLiteral("missing"), 1, ChatRole::User, QStringLiteral("ignored"))));
    QCOMPARE(store.lastError().code, ConversationStoreErrorCode::InvalidConversationId);
    QVERIFY(store.loadMessages(QStringLiteral("missing")).isEmpty());
    QCOMPARE(store.lastError().code, ConversationStoreErrorCode::InvalidConversationId);
}

void InMemoryConversationStoreTest::archivesAndDeletesWithoutRemovingOtherConversations() {
    InMemoryConversationStore store;
    const auto first = store.createConversation(QStringLiteral("First"));
    const auto second = store.createConversation(QStringLiteral("Second"));

    QVERIFY(store.archiveConversation(first.id));
    QVERIFY(!store.appendMessage(message(first.id, 1, ChatRole::User, QStringLiteral("blocked"))));
    QCOMPARE(store.lastError().code, ConversationStoreErrorCode::UnsupportedOperation);

    QVERIFY(store.deleteConversation(first.id));
    const auto conversations = store.listConversations();
    QCOMPARE(conversations.size(), 1);
    QCOMPARE(conversations.first().id, second.id);
}

QTEST_MAIN(InMemoryConversationStoreTest)

#include "test_in_memory_conversation_store.moc"
