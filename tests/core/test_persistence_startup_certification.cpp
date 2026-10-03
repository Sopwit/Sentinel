// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/chat/SQLiteConversationStore.h"
#include "sentinel/core/memory/SQLiteMemoryStore.h"
#include "sentinel/core/agent/SQLiteAgentRunStore.h"
#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/chat/InMemoryConversationStore.h"
#include "sentinel/core/memory/InMemorySettingsStore.h"
#include "sentinel/core/memory/InMemoryStore.h"
#include <QProcess>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QStandardPaths>
#include <QUuid>
#include <QtConcurrent>
#include <QTcpServer>
#include <QtTest>

using namespace sentinel::core;
static bool recordRun(SQLiteAgentRunStore& store, const QString& id, AgentEventType end) {
    AgentEvent event;
    event.id = id + "-start"; event.sessionId = id; event.turnId = id;
    event.timestamp = QDateTime::currentDateTimeUtc();
    event.type = AgentEventType::RunStarted; event.payload = AgentRunStartedEvent{};
    if (!store.record(event)) return false;
    if (end == AgentEventType::RunStarted) return true;
    event.id = id + "-end"; event.type = end; event.payload = AgentTextEvent{"terminal fixture"};
    return store.record(event);
}
static bool seed(const QString& root) {
    SQLiteMemoryStore memory(root + "/memory.db");
    SQLiteConversationStore chat(root + "/chat.db");
    SQLiteAgentRunStore runs(root + "/runs.db");
    memory.put("certification", "keep");
    const auto conversation = chat.createConversation("certification");
    auto message = ConversationMessageRecord{conversation.id, 1, ChatRole::Assistant,
        "completed", QDateTime::currentDateTimeUtc(), ChatMessageStatus::Completed};
    if (!chat.appendMessage(message)) return false;
    message.messageId = 2; message.content = "partial"; message.status = ChatMessageStatus::Streaming;
    return chat.appendMessage(message) && recordRun(runs, "completed", AgentEventType::AgentCompleted)
        && recordRun(runs, "cancelled", AgentEventType::AgentCancelled)
        && recordRun(runs, "active", AgentEventType::RunStarted) && memory.lastError().isEmpty();
}

class PersistenceStartupCertificationTest final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { QStandardPaths::setTestModeEnabled(true); }
    void startupReadinessGetterDoesNotEnterNetworkEventLoop() {
        QTcpServer slow;
        QVERIFY(slow.listen(QHostAddress::LocalHost, 0));
        AppSettings settings(std::make_unique<InMemorySettingsStore>(), inMemoryTestCredentialStore());
        auto config = OllamaConfig::fromEndpoint(QStringLiteral("http://127.0.0.1:%1").arg(slow.serverPort()));
        config.healthCheckTimeoutMs = 100;
        config.modelDiscoveryTimeoutMs = 100;
        ApplicationControllerBuilder builder;
        auto controller = builder.withMemoryStore(std::make_unique<InMemoryStore>())
            .withConversationStore(std::make_unique<InMemoryConversationStore>())
            .withModelService(std::make_unique<ModelService>(&settings))
            .withOllamaRuntimeClient(std::make_unique<OllamaHttpRuntimeClient>(config)).build();
        bool nestedLoopEntered = false;
        QTimer::singleShot(0, controller.get(), [&] { nestedLoopEntered = true; });
        controller->ollamaDiscoveryStatus();
        QVERIFY2(!nestedLoopEntered, "Startup readiness getter performed synchronous network I/O");
    }
    void forcedChildTerminationRecoversCommittedStores() {
        QTemporaryDir dir;
        QProcess child;
        child.start(QCoreApplication::applicationFilePath(), {"--unclean-fixture", dir.path()});
        QVERIFY(child.waitForStarted());
        QTRY_VERIFY_WITH_TIMEOUT(child.canReadLine(), 5000);
        QCOMPARE(child.readLine().trimmed(), QByteArray("READY"));
        QVERIFY(QFileInfo::exists(dir.filePath("chat.db-wal")));
        QVERIFY(QFileInfo::exists(dir.filePath("chat.db-shm")));
        child.kill();
        QVERIFY(child.waitForFinished());
        SQLiteConversationStore chat(dir.filePath("chat.db"));
        QCOMPARE(chat.status(), ConversationStoreStatus::Ready);
        const auto conversations = chat.listConversations();
        QCOMPARE(conversations.size(), 1);
        const auto messages = chat.loadMessages(conversations.first().id);
        QCOMPARE(messages.size(), 2);
        QCOMPARE(messages[0].content, QStringLiteral("completed"));
        QCOMPARE(messages[0].status, ChatMessageStatus::Completed);
        QCOMPARE(messages[1].status, ChatMessageStatus::Interrupted);
        SQLiteMemoryStore memory(dir.filePath("memory.db"));
        QCOMPARE(memory.get("certification"), QStringLiteral("keep"));
        SQLiteAgentRunStore runs(dir.filePath("runs.db"));
        QCOMPARE(runs.runById("completed").state, QStringLiteral("Completed"));
        QCOMPARE(runs.runById("cancelled").state, QStringLiteral("Cancelled"));
        QCOMPARE(runs.runById("active").state, QStringLiteral("Interrupted"));
    }
    void walCheckpointAndReopenPreserveRows() {
        QTemporaryDir dir;
        QVERIFY(seed(dir.path()));
        const auto name = QUuid::createUuid().toString();
        {
            auto db = QSqlDatabase::addDatabase("QSQLITE", name);
            db.setDatabaseName(dir.filePath("chat.db"));
            QVERIFY(db.open());
            QSqlQuery query(db);
            QVERIFY(query.exec("PRAGMA journal_mode"));
            QVERIFY(query.next()); QCOMPARE(query.value(0).toString(), QStringLiteral("wal"));
            QVERIFY(query.exec("PRAGMA wal_checkpoint(TRUNCATE)"));
            QVERIFY(query.next()); QCOMPARE(query.value(0).toInt(), 0);
            QVERIFY(query.exec("PRAGMA integrity_check"));
            QVERIFY(query.next()); QCOMPARE(query.value(0).toString(), QStringLiteral("ok"));
        }
        QSqlDatabase::removeDatabase(name);
        SQLiteConversationStore reopened(dir.filePath("chat.db"));
        QCOMPARE(reopened.listConversations().size(), 1);
        QCOMPARE(reopened.loadMessages(reopened.listConversations().first().id).size(), 2);
    }
    void concurrentIndependentOwnersAndSerializedAgentWrites() {
        QTemporaryDir dir;
        SQLiteConversationStore chat(dir.filePath("chat.db"));
        SQLiteAgentRunStore runs(dir.filePath("runs.db"));
        const auto conversation = chat.createConversation("concurrent");
        auto memoryJob = QtConcurrent::run([root = dir.path()] {
            SQLiteMemoryStore memory(root + "/memory.db");
            memory.put("remember", "value");
            return memory.get("remember") == "value" && memory.lastError().isEmpty();
        });
        auto agentJob = QtConcurrent::run([&] { return recordRun(runs, "agent", AgentEventType::AgentCompleted); });
        QVERIFY(chat.appendMessage({conversation.id, 1, ChatRole::Assistant, "chat",
            QDateTime::currentDateTimeUtc(), ChatMessageStatus::Completed}));
        QVERIFY(memoryJob.result()); QVERIFY(agentJob.result());
        QCOMPARE(chat.loadMessages(conversation.id).size(), 1);
        QCOMPARE(runs.runById("agent").state, QStringLiteral("Completed"));
        QVERIFY(runs.lastError().isEmpty());
    }
    void primaryCorruptionPreservesUnrelatedStore() {
        QTemporaryDir dir;
        SQLiteMemoryStore healthy(dir.filePath("memory.db"));
        healthy.put("keep", "value");
        QFile corrupt(dir.filePath("chat.db"));
        QVERIFY(corrupt.open(QIODevice::WriteOnly)); corrupt.write("malformed primary DB"); corrupt.close();
        SQLiteConversationStore chat(corrupt.fileName());
        QVERIFY(chat.status() != ConversationStoreStatus::Ready);
        QVERIFY(corrupt.open(QIODevice::ReadOnly));
        QCOMPARE(corrupt.readAll(), QByteArray("malformed primary DB"));
        QCOMPARE(healthy.get("keep"), QStringLiteral("value"));
    }
};
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().size() == 3 && app.arguments()[1] == "--unclean-fixture") {
        // Hold all production owners open so WAL/SHM really exist at termination.
        const auto root = app.arguments()[2];
        SQLiteMemoryStore memory(root + "/memory.db");
        SQLiteConversationStore chat(root + "/chat.db");
        SQLiteAgentRunStore runs(root + "/runs.db");
        if (!seed(root)) return 2;
        fputs("READY\n", stdout); fflush(stdout);
        return app.exec();
    }
    PersistenceStartupCertificationTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "test_persistence_startup_certification.moc"
