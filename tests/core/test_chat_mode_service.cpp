// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/app/IClock.h"
#include "sentinel/core/chat/ChatModeService.h"
#include "sentinel/core/chat/ChatSession.h"
#include "sentinel/core/chat/InMemoryConversationStore.h"
#include "sentinel/core/chat/SQLiteConversationStore.h"
#include "sentinel/core/interfaces/IChatProvider.h"
#include "sentinel/core/model/ModelService.h"

#include <QSignalSpy>
#include <QTemporaryDir>
#include <QThread>
#include <QtTest>

using namespace sentinel::core;

namespace {

class FixtureProvider final : public IChatProvider {
public:
    enum class ReplyMode { FinalOnly, StreamWithEmptyFinal, Failure, IgnoresCancellation };

    explicit FixtureProvider(ReplyMode mode, std::shared_ptr<std::atomic_int> finished = {})
        : mode_(mode), finished_(std::move(finished)) {}

    QString name() const override {
        return QStringLiteral("chat-fixture");
    }
    ChatProviderStatus status() const override {
        return ChatProviderStatus::Ready;
    }
    ChatProviderConcurrency concurrency() const override {
        return ChatProviderConcurrency::Supported;
    }
    bool supportsStreaming() const override {
        return mode_ == ReplyMode::StreamWithEmptyFinal;
    }

    ChatProviderReply sendMessage(const QString&) override {
        return {};
    }
    ChatProviderReply sendRequest(const QString&, const ChatRequestOptions&) override {
        if (mode_ == ReplyMode::Failure) {
            ChatProviderReply result;
            result.errorMessage = QStringLiteral("Fixture transport failure.");
            result.category = ChatProviderErrorCategory::ConnectionFailed;
            result.lifecycle = ChatRequestLifecycle::Failed;
            return result;
        }
        if (mode_ == ReplyMode::IgnoresCancellation)
            QThread::msleep(100);
        ChatProviderReply result;
        result.success = true;
        result.message = QStringLiteral("SENTINEL_CHAT_PIPELINE_OK");
        result.lifecycle = ChatRequestLifecycle::Completed;
        if (finished_)
            ++*finished_;
        return result;
    }
    ChatProviderReply sendMessageStreaming(const QString&,
                                           const std::function<void(const QString&)>& onDelta,
                                           const std::shared_ptr<std::atomic_bool>&) override {
        onDelta(QStringLiteral("SENTINEL_"));
        onDelta(QStringLiteral("CHAT_PIPELINE_OK"));
        ChatProviderReply result;
        result.success = true;
        result.lifecycle = ChatRequestLifecycle::Completed;
        return result;
    }

private:
    ReplyMode mode_;
    std::shared_ptr<std::atomic_int> finished_;
};

struct Harness {
    explicit Harness(FixtureProvider::ReplyMode mode) : session(std::make_unique<SystemClock>()) {
        models.registerProvider(QStringLiteral("ollama"), [mode](const ModelBinding&) {
            return std::make_shared<FixtureProvider>(mode);
        });
        conversationId = store.createConversation(QStringLiteral("Chat fixture")).id;
    }

    ModelService models;
    ChatSession session;
    InMemoryConversationStore store;
    QString conversationId;
};

} // namespace

class ChatModeServiceTest final : public QObject {
    Q_OBJECT

private slots:
    void finalOnlyReplyUpdatesPersistedPlaceholder();
    void emptyStreamingFinalDoesNotEraseDeltas();
    void providerFailureDoesNotLeaveEmptyAssistantMessage();
    void bindingFailureDoesNotLeaveEmptyAssistantMessage();
    void lateSuccessfulCallbackAfterCancellationStaysCancelled();
    void shutdownJoinsCancelledAndReplacementWorkers();
    void sqliteStoreAcceptsInitiallyEmptyAssistantPlaceholder();
    void geminiPreservesAuthoritativeJsonSchema();
};

void ChatModeServiceTest::geminiPreservesAuthoritativeJsonSchema() {
    ToolDescriptor tool;
    tool.id = QStringLiteral("list-directory");
    tool.description = QStringLiteral("List actual entries");
    tool.inputSchema = QJsonObject{
        {QStringLiteral("type"), QStringLiteral("object")},
        {QStringLiteral("additionalProperties"), false},
        {QStringLiteral("properties"),
         QJsonObject{{QStringLiteral("path"),
                      QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}}}};
    const auto declaration = geminiFunctionDeclaration(tool);
    QCOMPARE(declaration.value(QStringLiteral("name")).toString(), tool.id);
    QVERIFY(!declaration.contains(QStringLiteral("parameters")));
    QCOMPARE(declaration.value(QStringLiteral("parametersJsonSchema")).toObject(),
             tool.inputSchema);
}

void ChatModeServiceTest::finalOnlyReplyUpdatesPersistedPlaceholder() {
    Harness harness(FixtureProvider::ReplyMode::FinalOnly);
    ChatModeService service(harness.models, harness.session, harness.store);
    QSignalSpy changed(&service, &ChatModeService::messagesChanged);

    QVERIFY(service.send(harness.conversationId, QStringLiteral("fixture prompt"), {},
                         {QStringLiteral("ollama"), QStringLiteral("fixture")}, false));
    QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 3000);
    QCOMPARE(harness.session.messages().last().content,
             QStringLiteral("SENTINEL_CHAT_PIPELINE_OK"));
    QCOMPARE(harness.session.messages().last().status, ChatMessageStatus::Completed);
    QCOMPARE(harness.store.loadMessages(harness.conversationId).last().content,
             QStringLiteral("SENTINEL_CHAT_PIPELINE_OK"));
    QVERIFY(changed.count() >= 2);
}

void ChatModeServiceTest::emptyStreamingFinalDoesNotEraseDeltas() {
    Harness harness(FixtureProvider::ReplyMode::StreamWithEmptyFinal);
    ChatModeService service(harness.models, harness.session, harness.store);

    QVERIFY(service.send(harness.conversationId, QStringLiteral("fixture prompt"), {},
                         {QStringLiteral("ollama"), QStringLiteral("fixture")}, false));
    QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 3000);
    QCOMPARE(harness.session.messages().last().content,
             QStringLiteral("SENTINEL_CHAT_PIPELINE_OK"));
    QCOMPARE(harness.session.messages().last().status, ChatMessageStatus::Completed);
    QVERIFY(!harness.session.messages().last().partial);
}

void ChatModeServiceTest::providerFailureDoesNotLeaveEmptyAssistantMessage() {
    Harness harness(FixtureProvider::ReplyMode::Failure);
    ChatModeService service(harness.models, harness.session, harness.store);

    QVERIFY(service.send(harness.conversationId, QStringLiteral("fixture prompt"), {},
                         {QStringLiteral("ollama"), QStringLiteral("fixture")}, false));
    QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 3000);
    const auto& message = harness.session.messages().last();
    QCOMPARE(message.status, ChatMessageStatus::Failed);
    QVERIFY(!message.content.trimmed().isEmpty());
    QCOMPARE(message.errorCategory, ChatProviderErrorCategory::ConnectionFailed);
}

void ChatModeServiceTest::bindingFailureDoesNotLeaveEmptyAssistantMessage() {
    Harness harness(FixtureProvider::ReplyMode::FinalOnly);
    ChatModeService service(harness.models, harness.session, harness.store);

    QVERIFY(service.send(harness.conversationId, QStringLiteral("fixture prompt"), {},
                         {QStringLiteral("unknown-provider"), QStringLiteral("fixture")}, false));
    QVERIFY(!service.busy());
    const auto& message = harness.session.messages().last();
    QCOMPARE(message.status, ChatMessageStatus::Failed);
    QVERIFY(!message.content.trimmed().isEmpty());
    QCOMPARE(message.errorCategory, ChatProviderErrorCategory::ProviderUnavailable);
}

void ChatModeServiceTest::lateSuccessfulCallbackAfterCancellationStaysCancelled() {
    Harness harness(FixtureProvider::ReplyMode::IgnoresCancellation);
    ChatModeService service(harness.models, harness.session, harness.store);

    QVERIFY(service.send(harness.conversationId, QStringLiteral("fixture prompt"), {},
                         {QStringLiteral("ollama"), QStringLiteral("fixture")}, false));
    QVERIFY(service.stop());
    // A provider may return late even after it receives cancellation.  The service
    // must close the visible request immediately, then ignore that late success.
    QVERIFY(!service.busy());
    QTest::qWait(150);
    const auto& message = harness.session.messages().last();
    QCOMPARE(message.status, ChatMessageStatus::Cancelled);
    QCOMPARE(message.errorCategory, ChatProviderErrorCategory::Cancelled);
    QVERIFY(!message.content.trimmed().isEmpty());
}

void ChatModeServiceTest::shutdownJoinsCancelledAndReplacementWorkers() {
    Harness harness(FixtureProvider::ReplyMode::IgnoresCancellation);
    auto finished = std::make_shared<std::atomic_int>(0);
    harness.models.registerProvider(QStringLiteral("ollama"), [finished](const ModelBinding&) {
        return std::make_shared<FixtureProvider>(FixtureProvider::ReplyMode::IgnoresCancellation,
                                                 finished);
    });
    auto service =
        std::make_unique<ChatModeService>(harness.models, harness.session, harness.store);
    QVERIFY(service->send(harness.conversationId, QStringLiteral("First cancelled turn"), {},
                          {QStringLiteral("ollama"), QStringLiteral("fixture")}));
    QVERIFY(service->stop());
    QVERIFY(service->send(harness.conversationId, QStringLiteral("Replacement turn"), {},
                          {QStringLiteral("ollama"), QStringLiteral("fixture")}));
    service.reset();
    QCOMPARE(finished->load(), 2);
}

void ChatModeServiceTest::sqliteStoreAcceptsInitiallyEmptyAssistantPlaceholder() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    SQLiteConversationStore store(directory.filePath(QStringLiteral("conversations.sqlite3")));
    QCOMPARE(store.status(), ConversationStoreStatus::Ready);
    const auto conversation = store.createConversation(QStringLiteral("SQLite fixture"));
    QVERIFY(!conversation.id.isEmpty());

    ModelService models;
    models.registerProvider(QStringLiteral("ollama"), [](const ModelBinding&) {
        return std::make_shared<FixtureProvider>(FixtureProvider::ReplyMode::FinalOnly);
    });
    ChatSession session(std::make_unique<SystemClock>());
    ChatModeService service(models, session, store);

    QVERIFY(service.send(conversation.id, QStringLiteral("fixture prompt"), {},
                         {QStringLiteral("ollama"), QStringLiteral("fixture")}, false));
    QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 3000);
    const auto persisted = store.loadMessages(conversation.id);
    QCOMPARE(persisted.size(), 2);
    QCOMPARE(persisted.last().role, ChatRole::Assistant);
    QCOMPARE(persisted.last().content, QStringLiteral("SENTINEL_CHAT_PIPELINE_OK"));
    QCOMPARE(persisted.last().status, ChatMessageStatus::Completed);
}

QTEST_MAIN(ChatModeServiceTest)
#include "test_chat_mode_service.moc"
