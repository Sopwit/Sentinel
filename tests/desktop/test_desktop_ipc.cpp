// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../protocol/QtIpcContract.generated.h"
#include "../support/DeterministicChatFixture.h"
#include "sentinel/core/agent/IAgentStepPlanner.h"
#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/app/OnboardingService.h"
#include "sentinel/core/memory/InMemorySettingsStore.h"
#include "sentinel/core/memory/InMemoryStore.h"
#include "sentinel/core/model/ModelLibrary.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"
#include "sentinel/desktop/DaemonClient.h"
#include "sentinel/desktop/DesktopControllerBridge.h"
#include "sentinel/desktop/DesktopRuntimeClient.h"
#include "sentinel/desktop/DesktopSettingsStore.h"
#include "sentinel/desktop/QuickPanelController.h"
#include "service/DaemonIpcServer.h"

#include <QDir>
#include <QJsonDocument>
#include <QLocalServer>
#include <QSignalSpy>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

using sentinel::desktop::DaemonClient;
namespace {
class CountingCredentials final : public sentinel::core::ICredentialBackend {
public:
    sentinel::core::CredentialStoreBackend backend() const override {
        return delegate.backend();
    }
    sentinel::core::CredentialStoreSummary summary() const override {
        return delegate.summary();
    }
    sentinel::core::CredentialBackendResult
    storeCredential(const sentinel::core::CredentialKey& key, const QString& secret) override {
        return delegate.storeCredential(key, secret);
    }
    sentinel::core::CredentialReadResult
    readCredential(const sentinel::core::CredentialKey& key) const override {
        ++reads;
        return delegate.readCredential(key);
    }
    sentinel::core::CredentialBackendResult
    deleteCredential(const sentinel::core::CredentialKey& key) override {
        return delegate.deleteCredential(key);
    }
    sentinel::core::CredentialBackendResult
    containsCredential(const sentinel::core::CredentialKey& key) const override {
        ++reads;
        return delegate.containsCredential(key);
    }
    mutable int reads = 0;
    sentinel::core::InMemoryCredentialBackend delegate;
};
QJsonObject frame(const QString& type, const QString& id, const QString& name,
                  const QJsonObject& payload, int major = 1, int minor = 0) {
    return {{"version", QJsonObject{{"major", major}, {"minor", minor}}},
            {"type", type},
            {"id", id},
            {"name", name},
            {"payload", payload}};
}
void send(QLocalSocket* socket, const QJsonObject& object) {
    socket->write(QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n');
}
QJsonObject read(QLocalSocket* socket) {
    QElapsedTimer timer;
    timer.start();
    while (!socket->canReadLine() && timer.elapsed() < 2000) {
        QTest::qWait(1);
    }
    return QJsonDocument::fromJson(socket->readLine()).object();
}
class DictationStt final : public sentinel::core::ISpeechToTextRuntime {
public:
    sentinel::core::SpeechProviderInfo info() const override {
        sentinel::core::SpeechProviderInfo result;
        result.readiness = sentinel::core::AudioRuntimeReadiness::Ready;
        return result;
    }
    sentinel::core::SpeechTranscript transcribeFile(const QString&, const QString&,
                                                    std::shared_ptr<std::atomic_bool>) override {
        return {.finalText = "Private dictation"};
    }
};
class MetadataClient final : public sentinel::core::IOllamaRuntimeClient {
public:
    sentinel::core::OllamaConfig config() const override {
        return {};
    }
    sentinel::core::OllamaHealthCheckResult healthCheck() const override {
        sentinel::core::OllamaHealthCheckResult result;
        result.connectionStatus = sentinel::core::OllamaConnectionStatus::Connected;
        result.healthStatus = sentinel::core::OllamaHealthStatus::Healthy;
        return result;
    }
    QList<sentinel::core::OllamaModelSummary> installedModels() const override {
        return {{QStringLiteral("sentinel-test-model"), {}, 0}};
    }
};
class ApprovalPlanner final : public sentinel::core::IAgentStepPlanner {
public:
    sentinel::core::AgentStepDecision
    nextStep(const QString&, const QList<sentinel::core::AgentStepRecord>& history) const override {
        sentinel::core::AgentStepDecision step;
        if (history.isEmpty()) {
            step.kind = sentinel::core::AgentStepDecision::Kind::ToolCall;
            step.toolId = "run-command";
            step.toolName = "run-command";
            step.riskLevel = sentinel::core::ToolRiskLevel::High;
            step.arguments.append({QStringLiteral("command"), QStringLiteral("pwd")});
        } else {
            step.kind = sentinel::core::AgentStepDecision::Kind::FinalAnswer;
            step.answer = "Fixture finished";
        }
        return step;
    }
};
class FixtureExecutor final : public sentinel::core::IToolExecutor {
public:
    sentinel::core::ToolExecutionResult
    execute(const sentinel::core::ToolExecutionRequest&) const override {
        return {sentinel::core::ToolExecutionStatus::Succeeded, QStringLiteral("Fixture result")};
    }
};
struct Peer {
    QTemporaryDir directory{QDir::tempPath() + "/sd-XXXXXX"};
    QLocalServer server;
    QLocalSocket* socket = nullptr;
    QString path;
    Peer() : path(directory.filePath("fixture.sock")) {
        server.listen(path);
    }
    bool connect(DaemonClient& client, int major = 1, bool generation = true) {
        client.connectToDaemon();
        QElapsedTimer timer;
        timer.start();
        while (!server.hasPendingConnections() && timer.elapsed() < 2000) {
            QTest::qWait(1);
        }
        socket = server.nextPendingConnection();
        if (!socket) {
            return false;
        }
        const auto hello = read(socket);
        if (hello.value("name") != "hello") {
            return false;
        }
        QJsonObject payload{{"major", major},
                            {"minor", 0},
                            {"daemon_version", "fixture"},
                            {"capabilities", QJsonArray{}}};
        if (generation) {
            payload.insert("server_generation", "fixture-generation");
        }
        send(socket, frame("response", hello.value("id").toString(), "hello", payload, major));
        return true;
    }
    void status() {
        const auto request = read(socket);
        send(socket, frame("response", request.value("id").toString(), "daemon.status",
                           {{"running", true},
                            {"daemon_version", "fixture"},
                            {"uptime_ms", 0},
                            {"active_runs", 0},
                            {"sessions", 0}}));
    }
};
} // namespace
class DesktopIpcTest final : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName("sentinel-phase3-test-" +
                                             QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    void cleanupTestCase() {
        for (auto location : {QStandardPaths::AppDataLocation, QStandardPaths::AppConfigLocation}) {
            const auto path = QStandardPaths::writableLocation(location);
            if (QDir(path).exists()) {
                QVERIFY(QDir(path).removeRecursively());
            }
        }
    }
    void onboardingProgressPersistsWithoutDaemon() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        DaemonClient client(directory.filePath("absent.sock"), 1000, 1000);
        sentinel::desktop::DesktopRuntimeClient runtime(client);
        const auto path = directory.filePath("preferences.json");
        {
            sentinel::core::AppSettings settings(
                std::make_unique<sentinel::desktop::DesktopSettingsStore>(runtime, path));
            sentinel::core::OnboardingService onboarding(settings, nullptr);
            QCOMPARE(onboarding.snapshot().step, sentinel::core::OnboardingStep::Welcome);
            QVERIFY(onboarding.advance().accepted);
            QCOMPARE(onboarding.snapshot().step, sentinel::core::OnboardingStep::ProcessingMode);
        }
        sentinel::core::AppSettings reopened(
            std::make_unique<sentinel::desktop::DesktopSettingsStore>(runtime, path));
        sentinel::core::OnboardingService onboarding(reopened, nullptr);
        QCOMPARE(onboarding.snapshot().step, sentinel::core::OnboardingStep::ProcessingMode);
        QVERIFY(onboarding.back().accepted);
        QCOMPARE(onboarding.snapshot().step, sentinel::core::OnboardingStep::Welcome);
        QVERIFY(!client.daemonReachable());
    }
    void handshakeStatusAndDisconnect() {
        Peer peer;
        DaemonClient client(peer.path, 1000, 1000);
        QSignalSpy status(&client, &DaemonClient::statusReceived);
        QVERIFY(peer.connect(client));
        QTRY_VERIFY(client.daemonReachable());
        QCOMPARE(client.serverGeneration(), QString("fixture-generation"));
        QCOMPARE(client.connectionEpoch(), quint64(1));
        peer.status();
        QTRY_COMPARE(status.size(), 1);
        client.disconnectFromDaemon();
        QCOMPARE(client.connectionState(), DaemonClient::ConnectionState::Disconnected);
        QVERIFY(!client.daemonReachable());
    }
    void olderServerWithoutGeneration() {
        Peer peer;
        DaemonClient client(peer.path, 1000, 1000);
        QVERIFY(peer.connect(client, 1, false));
        QTRY_VERIFY(client.daemonReachable());
        QVERIFY(client.serverGeneration().isEmpty());
        QCOMPARE(client.connectionEpoch(), quint64(1));
        peer.status();
    }
    void invalidRequestIsRejectedLocally() {
        Peer peer;
        DaemonClient client(peer.path, 1000, 1000);
        QVERIFY(peer.connect(client));
        QTRY_VERIFY(client.daemonReachable());
        peer.status();
        QSignalSpy errors(&client, &DaemonClient::requestFailed);
        const auto id =
            client.request(DaemonClient::Command::chat_send, {{"session_id", "s"}, {"text", 123}});
        QTRY_VERIFY(!errors.isEmpty());
        QCOMPARE(errors.last().at(0).toString(), id);
        QCOMPARE(errors.last().at(1).value<DaemonClient::Error>(),
                 DaemonClient::Error::InvalidRequest);
        QVERIFY(!peer.socket->canReadLine());
    }
    void unavailable() {
        QTemporaryDir directory{QDir::tempPath() + "/sd-XXXXXX"};
        DaemonClient client(directory.filePath("absent.sock"), 1000, 1000);
        QTRY_COMPARE(client.connectionState(), DaemonClient::ConnectionState::Unavailable);
        QSignalSpy errors(&client, &DaemonClient::requestFailed);
        const auto id = client.request(DaemonClient::Command::model_current);
        QTRY_COMPARE(errors.size(), 1);
        QCOMPARE(errors.at(0).at(0).toString(), id);
        QCOMPARE(errors.at(0).at(1).value<DaemonClient::Error>(),
                 DaemonClient::Error::DaemonUnavailable);
        client.disconnectFromDaemon();
    }
    void incompatibleMajor() {
        Peer peer;
        DaemonClient client(peer.path, 1000, 10);
        QVERIFY(peer.connect(client, 2));
        QTRY_COMPARE(client.connectionState(), DaemonClient::ConnectionState::VersionMismatch);
        QVERIFY(!client.daemonReachable());
        QTest::qWait(50);
        QVERIFY(!peer.server.hasPendingConnections());
    }
    void requestCorrelationAndDuplicateSuppression() {
        Peer peer;
        DaemonClient client(peer.path, 1000, 1000);
        QVERIFY(peer.connect(client));
        QTRY_VERIFY(client.daemonReachable());
        peer.status();
        QSignalSpy replies(&client, &DaemonClient::responseReceived);
        const auto first = client.request(DaemonClient::Command::model_current);
        const auto second = client.request(DaemonClient::Command::session_list);
        read(peer.socket);
        read(peer.socket);
        send(peer.socket, frame("response", second, "session.list", {{"sessions", QJsonArray{}}}));
        const auto model = frame("response", first, "model.current",
                                 {{"provider_id", "fixture"}, {"model_id", "m"}});
        send(peer.socket, model);
        send(peer.socket, model);
        QTRY_VERIFY(replies.size() >= 2);
        int firstCount = 0, secondCount = 0;
        for (const auto& reply : replies) {
            firstCount += reply.at(0).toString() == first;
            secondCount += reply.at(0).toString() == second;
        }
        QCOMPARE(firstCount, 1);
        QCOMPARE(secondCount, 1);
    }
    void fragmentedFrame() {
        Peer peer;
        DaemonClient client(peer.path, 1000, 1000);
        QVERIFY(peer.connect(client));
        QTRY_VERIFY(client.daemonReachable());
        peer.status();
        QSignalSpy events(&client, &DaemonClient::eventReceived);
        const auto bytes =
            QJsonDocument(frame("event", {}, "output.delta",
                                {{"run_id", "r"}, {"session_id", "s"}, {"text", "hello"}}))
                .toJson(QJsonDocument::Compact) +
            '\n';
        peer.socket->write(bytes.left(10));
        QTest::qWait(10);
        QCOMPARE(events.size(), 0);
        peer.socket->write(bytes.mid(10));
        QTRY_COMPARE(events.size(), 1);
        QCOMPARE(events.at(0).at(1).toJsonObject().value("text").toString(), QString("hello"));
    }
    void malformedEvent_data() {
        QTest::addColumn<QByteArray>("bytes");
        QTest::newRow("bad-json") << QByteArray("not-json\n");
        QTest::newRow("wrong-payload")
            << (QJsonDocument(frame("event", {}, "output.delta",
                                    {{"run_id", "r"}, {"session_id", "s"}, {"text", 1}}))
                    .toJson(QJsonDocument::Compact) +
                '\n');
        QTest::newRow("oversized-frame") << QByteArray(sentinel::ipc::max_frame_bytes, 'x');
    }
    void malformedEvent() {
        QFETCH(QByteArray, bytes);
        Peer peer;
        DaemonClient client(peer.path, 1000, 1000);
        QVERIFY(peer.connect(client));
        QTRY_VERIFY(client.daemonReachable());
        peer.status();
        QSignalSpy errors(&client, &DaemonClient::requestFailed);
        peer.socket->write(bytes);
        QTRY_VERIFY(!client.daemonReachable());
        bool malformed = false;
        for (const auto& error : errors) {
            malformed |=
                error.at(1).value<DaemonClient::Error>() == DaemonClient::Error::MalformedResponse;
        }
        QVERIFY(malformed);
    }
    void timeoutDoesNotReplayMutation() {
        Peer peer;
        DaemonClient client(peer.path, 150, 1000);
        QVERIFY(peer.connect(client));
        QTRY_VERIFY(client.daemonReachable());
        peer.status();
        QSignalSpy errors(&client, &DaemonClient::requestFailed);
        const auto id = client.request(DaemonClient::Command::session_create, {{"title", "test"}});
        read(peer.socket);
        QTRY_VERIFY(!errors.isEmpty());
        bool timedOut = false;
        for (const auto& error : errors) {
            timedOut |= error.at(0).toString() == id && error.at(1).value<DaemonClient::Error>() ==
                                                            DaemonClient::Error::RequestTimeout;
        }
        QVERIFY(timedOut);
        QVERIFY(!peer.socket->canReadLine());
    }
    void reconnectInvalidatesPendingIds() {
        Peer peer;
        DaemonClient client(peer.path, 1000, 10);
        QVERIFY(peer.connect(client));
        QTRY_VERIFY(client.daemonReachable());
        peer.status();
        const auto oldId = client.request(DaemonClient::Command::model_current);
        read(peer.socket);
        peer.socket->abort();
        QTRY_VERIFY(!client.daemonReachable());
        QVERIFY(peer.connect(client));
        QTRY_VERIFY(client.daemonReachable());
        peer.status();
        QCOMPARE(client.connectionEpoch(), quint64(2));
        QSignalSpy replies(&client, &DaemonClient::responseReceived);
        send(peer.socket, frame("response", oldId, "model.current",
                                {{"provider_id", "stale"}, {"model_id", "stale"}}));
        QTest::qWait(20);
        for (const auto& reply : replies) {
            QVERIFY(reply.at(0).toString() != oldId);
        }
    }
    void rejectedSendRemainsVisibleAcrossPollingAndRecovers() {
        QTemporaryDir directory{QDir::tempPath() + "/se-XXXXXX"};
        QVERIFY(QFile::setPermissions(directory.path(), QFileDevice::ReadOwner |
                                         QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        sentinel::test::DeterministicModelServiceFixture models;
        sentinel::core::ApplicationControllerBuilder builder;
        auto controller = builder.withModelService(models.takeModelService())
                                 .withOllamaRuntimeClient(std::make_unique<MetadataClient>())
                                 .withMemoryStore(std::make_unique<sentinel::core::InMemoryStore>())
                                 .build();
        sentinel::daemon::DaemonIpcServer server(controller.get());
        QVERIFY(server.startServer(directory.filePath("send.sock")));
        DaemonClient transport(directory.filePath("send.sock"), 3000, 10);
        sentinel::desktop::DesktopRuntimeClient adapter(transport);
        sentinel::desktop::DesktopControllerBridge bridge({nullptr, &adapter});
        QTRY_VERIFY(adapter.ready());
        controller->modelService()->registerProvider("ollama", [](const sentinel::core::ModelBinding&) {
            return std::shared_ptr<sentinel::core::IChatProvider>{};
        });
        QVERIFY(bridge.sendMessage(QStringLiteral("The server stopped after discovery")));
        QTRY_COMPARE(bridge.chatSendLifecycleState(), QString("failed"));
        QCOMPARE(bridge.chatErrorCategory(), QString("provider-unavailable"));
        QVERIFY(bridge.chatSendLifecycleSummary().contains("server"));
        QVERIFY(!bridge.chatGenerationActive());
        // Explicit refresh includes session.list; a prior idle session cannot hide the error.
        adapter.refresh();
        QTest::qWait(2200);
        QCOMPARE(bridge.chatSendLifecycleState(), QString("failed"));
        QCOMPARE(bridge.chatErrorCategory(), QString("provider-unavailable"));
        controller->modelService()->registerProvider("ollama", [state = models.state](const sentinel::core::ModelBinding&) {
            return std::make_shared<sentinel::test::DeterministicChatProvider>(state);
        });
        QVERIFY(bridge.sendMessage(QStringLiteral("The provider is available again")));
        QTRY_COMPARE(bridge.chatSendLifecycleState(), QString("completed"));
        QVERIFY(bridge.chatErrorCategory().isEmpty());
        QVERIFY(adapter.messages().last().content.contains("SENTINEL_TEST_RESPONSE"));
    }
    void backgroundConversationTitleIsDurableAndProjected_data() {
        QTest::addColumn<bool>("manualRename");
        QTest::newRow("generated-title") << false;
        QTest::newRow("manual-title-wins") << true;
    }
    void backgroundConversationTitleIsDurableAndProjected() {
        QFETCH(bool, manualRename);
        QTemporaryDir directory{QDir::tempPath() + "/st-XXXXXX"};
        QVERIFY(directory.isValid());
        QVERIFY(QFile::setPermissions(directory.path(), QFileDevice::ReadOwner |
                                         QFileDevice::WriteOwner | QFileDevice::ExeOwner));
        auto gate = std::make_shared<QSemaphore>();
        auto started = std::make_shared<std::atomic_bool>(false);
        const auto releaseOnExit = qScopeGuard([gate] { gate->release(); });
        class Provider final : public sentinel::core::IChatProvider {
        public:
            std::shared_ptr<QSemaphore> gate;
            std::shared_ptr<std::atomic_bool> started;
            QString name() const override { return "title-fixture"; }
            sentinel::core::ChatProviderStatus status() const override {
                return sentinel::core::ChatProviderStatus::Ready;
            }
            sentinel::core::ChatProviderReply sendMessage(const QString& prompt) override {
                if (prompt.startsWith("Create a concise conversation title")) {
                    started->store(true);
                    if (!gate->tryAcquire(1, 5000)) return {false, {}, "title timeout"};
                    return {true, "Qt Interface Design", {}};
                }
                return {true, "A useful answer about Qt.", {}};
            }
        };
        sentinel::test::DeterministicModelServiceFixture fixture;
        auto models = fixture.takeModelService();
        models->registerProvider("ollama", [gate, started](const sentinel::core::ModelBinding&) {
            auto provider = std::make_shared<Provider>();
            provider->gate = gate;
            provider->started = started;
            return provider;
        });
        sentinel::core::ApplicationControllerBuilder builder;
        auto controller = builder.withModelService(std::move(models))
                                 .withOllamaRuntimeClient(std::make_unique<MetadataClient>())
                                 .withMemoryStore(std::make_unique<sentinel::core::InMemoryStore>())
                                 .build();
        sentinel::daemon::DaemonIpcServer server(controller.get());
        QVERIFY(server.startServer(directory.filePath("title.sock")));
        DaemonClient transport(directory.filePath("title.sock"), 3000, 10);
        sentinel::desktop::DesktopRuntimeClient adapter(transport);
        sentinel::desktop::DesktopControllerBridge bridge({nullptr, &adapter});
        sentinel::desktop::QuickPanelController panel(adapter);
        QTRY_VERIFY(adapter.ready());
        QVERIFY(!bridge.createConversation(QStringLiteral("New Chat")).isEmpty());
        QTRY_COMPARE(adapter.value("activeConversationTitle").toString(), QString("New Chat"));
        const auto sid = adapter.sessionId();
        QVERIFY(panel.ask(QStringLiteral("Help me design a Qt interface")));
        QTRY_COMPARE(bridge.chatSendLifecycleState(), QString("completed"));
        QTRY_VERIFY(started->load());
        // The answer finished and IPC remains usable while title generation waits.
        if (manualRename)
            QVERIFY(controller->renameConversation(sid, QStringLiteral("My chosen title")));
        gate->release();
        const auto expected = manualRename ? QStringLiteral("My chosen title")
                                           : QStringLiteral("Qt Interface Design");
        QTRY_COMPARE(adapter.value("activeConversationTitle").toString(), expected);
        QTRY_COMPARE(adapter.value("conversationListCurrentTitle").toString(), expected);
        QVERIFY(adapter.value("conversationTitles").toStringList().contains(expected));
        for (const auto& record : controller->conversationStore()->listConversations())
            if (record.id == sid) QCOMPARE(record.title, expected);
    }
    void compatibleFutureEvent() {
        Peer peer;
        DaemonClient client(peer.path, 1000, 1000);
        QVERIFY(peer.connect(client));
        QTRY_VERIFY(client.daemonReachable());
        peer.status();
        QSignalSpy events(&client, &DaemonClient::eventReceived);
        send(peer.socket, frame("event", {}, "future.event", {}, 1, sentinel::ipc::minor + 1));
        QTest::qWait(20);
        QVERIFY(client.daemonReachable());
        QCOMPARE(events.size(), 0);
    }
    void quickPanelRejectsUnboundedLinks_data() {
        QTest::addColumn<QString>("url");
        for (const auto& url :
             {"https://settings", "sentinel://session/../settings", "sentinel://session/a/b",
              "sentinel://user@session/a", "sentinel://session/a?run=1", "sentinel://session/a#x",
              "sentinel://session/%2fetc", "sentinel://run/a", "sentinel://session/",
              "sentinel://settings:42"})
            QTest::newRow(url) << QString::fromLatin1(url);
    }
    void quickPanelRejectsUnboundedLinks() {
        QFETCH(QString, url);
        QVERIFY(sentinel::desktop::QuickPanelController::parseLink(url).isEmpty());
    }
    void modelLibraryProjectionDoesNotReadCloudCredentials() {
        auto credentials = std::make_shared<CountingCredentials>();
        sentinel::core::AppSettings settings(
            std::make_unique<sentinel::core::InMemorySettingsStore>(),
            sentinel::core::CredentialStore(credentials));
        sentinel::core::ModelService models(&settings);
        sentinel::core::ModelLibraryService library(models);
        const auto before = credentials->reads;
        QVERIFY(!library.providerStates().isEmpty());
        library.entries();
        QCOMPARE(credentials->reads, before);
        // Real binding configuration still reads through the authorized credential backend.
        models.providerConfig(sentinel::core::ModelBinding{"gemini", "test-model"});
        QVERIFY(credentials->reads > before);
    }
    void connectedDoesNotMeanReadyUntilSettingsAndSessionArrive() {
        Peer peer;
        DaemonClient transport(peer.path, 5000, 1000);
        sentinel::desktop::DesktopRuntimeClient adapter(transport);
        QVERIFY(peer.connect(transport));
        QTRY_COMPARE(transport.connectionState(), DaemonClient::ConnectionState::Connected);
        QVERIFY(!adapter.ready());
        QHash<QString, QJsonObject> pending;
        for (int i = 0; i < 4; ++i) {
            const auto request = read(peer.socket);
            pending[request.value("name").toString()] = request;
        }
        const auto reply = [&](const QString& name, QJsonObject payload) {
            send(peer.socket,
                 frame("response", pending.value(name).value("id").toString(), name, payload));
            QTest::qWait(2);
        };
        QJsonObject properties;
        const auto fields = sentinel::ipc::desktopProjectionFields();
        for (auto it = fields.begin(); it != fields.end(); ++it) {
            const auto type = it.value().toObject().value("type").toString();
            properties[it.key()] = type == "string"    ? QJsonValue(QString{})
                                   : type == "boolean" ? QJsonValue(false)
                                   : type == "array"   ? QJsonValue(QJsonArray{})
                                   : type == "object"  ? QJsonValue(QJsonObject{})
                                                       : QJsonValue(0);
        }
        reply("desktop.projection", {{"page", 0}, {"pages", 1}, {"properties", properties}});
        QVERIFY(!adapter.ready());
        reply("desktop.settings", {{"values", QJsonObject{}}});
        QVERIFY(!adapter.ready());
        reply("session.list",
              {{"sessions", QJsonArray{QJsonObject{{"session_id", "readiness-session"}}}}});
        QVERIFY(!adapter.ready());
        auto attach = read(peer.socket);
        QCOMPARE(attach.value("name").toString(), QString("session.attach"));
        send(peer.socket, frame("response", attach.value("id").toString(), "session.attach",
                                {{"session_id", "readiness-session"},
                                 {"state", "idle"},
                                 {"output", ""},
                                 {"run_id", ""},
                                 {"event_sequence", 0},
                                 {"properties", QJsonObject{}}}));
        QTRY_VERIFY(adapter.ready());
        peer.socket->abort();
        QTRY_VERIFY(!adapter.ready());
        QVERIFY(adapter.messages().isEmpty());
    }
    void voicePcmLeaseBoundsPrivacyAndConfigurationChange() {
        QTemporaryDir directory{QDir::tempPath() + "/sv-XXXXXX"};
        QVERIFY(QFile::setPermissions(directory.path(), QFileDevice::ReadOwner |
                                                            QFileDevice::WriteOwner |
                                                            QFileDevice::ExeOwner));
        sentinel::core::AppSettings settings(
            std::make_unique<sentinel::core::InMemorySettingsStore>(),
            sentinel::core::inMemoryTestCredentialStore());
        sentinel::test::DeterministicModelServiceFixture models(
            sentinel::test::DeterministicChatReply::Final, &settings);
        sentinel::core::ApplicationControllerBuilder builder;
        auto controller = builder.withModelService(models.takeModelService())
                              .withOllamaRuntimeClient(std::make_unique<MetadataClient>())
                              .withMemoryStore(std::make_unique<sentinel::core::InMemoryStore>())
                              .build();
        sentinel::daemon::DaemonIpcServer server(controller.get());
        QVERIFY(server.startServer(directory.filePath("voice.sock")));
        QLocalSocket owner, other;
        const auto connectClient = [&](QLocalSocket& socket) {
            socket.connectToServer(directory.filePath("voice.sock"));
            QVERIFY(socket.waitForConnected(2000));
            send(&socket, frame("request", "hello", "hello",
                                {{"client_id", "voice-test"},
                                 {"major", 1},
                                 {"minor", 1},
                                 {"capabilities", QJsonArray{}}}));
            QCOMPARE(read(&socket).value("type").toString(), QString("response"));
        };
        const auto request = [&](QLocalSocket& socket, const QString& name,
                                 QJsonObject payload = {}) {
            send(&socket, frame("request", name, name, payload));
            return read(&socket);
        };
        connectClient(owner);
        connectClient(other);
        QCOMPARE(request(owner, "voice.action", {{"action", "start"}}).value("type").toString(),
                 QString("error"));
        controller->audioSession()->setSttRuntime(std::make_shared<DictationStt>());
        QCOMPARE(request(owner, "voice.action", {{"action", "start"}}).value("type").toString(),
                 QString("response"));
        QCOMPARE(request(other, "voice.action", {{"action", "start"}})
                     .value("payload")
                     .toObject()
                     .value("code")
                     .toString(),
                 QString("voice-busy"));
        const QJsonObject chunk{{"pcm", "AAA="}, {"final", false}, {"speech", true}};
        QCOMPARE(request(other, "voice.audio", chunk).value("type").toString(), QString("error"));
        QCOMPARE(request(owner, "voice.audio",
                         {{"pcm", "not base64!"}, {"final", true}, {"speech", true}})
                     .value("type")
                     .toString(),
                 QString("error"));
        request(owner, "voice.action", {{"action", "start"}});
        controller->audioSession()->setSttRuntime(std::make_shared<DictationStt>());
        QCOMPARE(request(owner, "voice.audio", chunk)
                     .value("payload")
                     .toObject()
                     .value("code")
                     .toString(),
                 QString("voice-configuration-changed"));
        request(owner, "voice.action", {{"action", "start"}});
        QCOMPARE(request(owner, "voice.audio", {{"pcm", "AAA="}, {"final", true}, {"speech", true}})
                     .value("payload")
                     .toObject()
                     .value("accepted")
                     .toBool(),
                 true);
        QTRY_COMPARE(controller->audioSession()->state(),
                     sentinel::core::VoiceInteractionState::Completed);
        QCOMPARE(request(owner, "voice.state")
                     .value("payload")
                     .toObject()
                     .value("transcript")
                     .toString(),
                 QString("Private dictation"));
        QVERIFY(request(other, "voice.state")
                    .value("payload")
                    .toObject()
                    .value("transcript")
                    .toString()
                    .isEmpty());
        request(owner, "voice.action", {{"action", "start"}});
        request(owner, "voice.audio", chunk);
        owner.abort();
        QTest::qWait(20);
        QCOMPARE(request(other, "voice.action", {{"action", "start"}}).value("type").toString(),
                 QString("response"));
        QVERIFY(request(other, "voice.state")
                    .value("payload")
                    .toObject()
                    .value("transcript")
                    .toString()
                    .isEmpty());
        QVERIFY(!controller->audioSession()->devices()->isCapturing());
        QVERIFY(!controller->audioSession()->privacy().retainRawRecordings);
    }
    void quickPanelConnectionAndLinkProjection() {
        QTemporaryDir directory;
        DaemonClient transport(directory.filePath("absent.sock"), 500, 1000);
        sentinel::desktop::DesktopRuntimeClient adapter(transport);
        sentinel::desktop::QuickPanelController panel(adapter);
        QTRY_COMPARE(transport.connectionState(), DaemonClient::ConnectionState::Unavailable);
        QCOMPARE(panel.state().value("connection").toString(), QString("Unavailable"));
        QVERIFY(!panel.state().value("ready").toBool());
        QCOMPARE(panel.state().value("approvalCount").toInt(), 0);
        QCOMPARE(panel.state().value("model").toString(), QString("Unavailable"));
        QVERIFY(!panel.ask("No fallback"));
        QVERIFY(!panel.agent("No fallback"));
        QVERIFY(!panel.error().isEmpty());
        QVERIFY(!panel.voiceAction("start"));
        QVERIFY(!panel.state().value("voiceAvailable").toBool());
        QVERIFY(!panel.cancel());
        QVERIFY(!panel.approve(true));
        QSignalSpy open(&panel, &sentinel::desktop::QuickPanelController::openRequested);
        QVERIFY(panel.openLink("sentinel://settings"));
        QCOMPARE(open.last().first().toString(), QString("Settings"));
        QVERIFY(!panel.openLink("sentinel://session/abc-123"));
        QCOMPARE(sentinel::desktop::QuickPanelController::parseLink("sentinel://session/abc-123")
                     .value("sessionId")
                     .toString(),
                 QString("abc-123"));
    }
    void presentationAdapterNoFallback() {
        QTemporaryDir directory{QDir::tempPath() + "/sd-XXXXXX"};
        DaemonClient transport(directory.filePath("absent.sock"), 500, 1000);
        sentinel::desktop::DesktopRuntimeClient adapter(transport);
        sentinel::desktop::DesktopControllerBridge bridge({nullptr, &adapter});
        QTRY_COMPARE(transport.connectionState(), DaemonClient::ConnectionState::Unavailable);
        QVERIFY(bridge.isRemote());
        QVERIFY(!bridge.modelService());
        QVERIFY(!bridge.sendMessage(QStringLiteral("Must not run locally")));
        QVERIFY(!bridge.runAgentRequest(QStringLiteral("Must not run locally")));
        QVERIFY(adapter.messages().isEmpty());
        QVERIFY(!bridge.localChatSendAvailable());
    }
    void presentationAdapterChatSessionsCancelRecover() {
        QTemporaryDir directory{QDir::tempPath() + "/sd-XXXXXX"};
        sentinel::core::AppSettings settings(
            std::make_unique<sentinel::core::InMemorySettingsStore>(),
            sentinel::core::inMemoryTestCredentialStore());
        sentinel::test::DeterministicModelServiceFixture models(
            sentinel::test::DeterministicChatReply::Delayed, &settings);
        auto completionGate = std::make_shared<QSemaphore>();
        models.state->completionGate = completionGate;
        auto releaseOnExit = qScopeGuard([completionGate] { completionGate->release(); });
        sentinel::core::ApplicationControllerBuilder builder;
        auto controller = builder.withModelService(models.takeModelService())
                              .withOllamaRuntimeClient(std::make_unique<MetadataClient>())
                              .withMemoryStore(std::make_unique<sentinel::core::InMemoryStore>())
                              .build();
        sentinel::daemon::DaemonIpcServer server(controller.get());
        const auto path = directory.filePath("daemon.sock");
        QVERIFY(server.openSessionProjectionStore(directory.filePath("sessions.sqlite3")));
        QVERIFY(server.startServer(path));
        DaemonClient transport(path, 3000, 10);
        sentinel::desktop::DesktopRuntimeClient adapter(transport);
        sentinel::desktop::DesktopControllerBridge bridge({nullptr, &adapter});
        sentinel::desktop::QuickPanelController panel(adapter);
        QSignalSpy failures(&adapter, &sentinel::desktop::DesktopRuntimeClient::operationFailed);
        QSignalSpy responses(&transport, &DaemonClient::responseReceived);
        QTRY_VERIFY2(
            adapter.ready(),
            qPrintable(transport.statusSummary() + " / " + QString::number(responses.size()) +
                       " responses / " + QString::number(failures.size()) + " failures / " +
                       (failures.isEmpty() ? QString{} : failures.last().first().toString())));
        QTRY_VERIFY(!adapter.sessionId().isEmpty());
        QCOMPARE(bridge.selectedLocalModel(), QString("sentinel-test-model"));
        QCOMPARE(bridge.selectedRuntimeProvider(), QString("ollama"));
        QTRY_VERIFY(!adapter.voiceState().isEmpty());
        QVERIFY(!adapter.voiceState().value("available").toBool());
        QVERIFY(!panel.voiceAction("invalid"));
        const auto first = adapter.sessionId();
        QSignalSpy open(&panel, &sentinel::desktop::QuickPanelController::openRequested);
        panel.continueConversation();
        QCOMPARE(open.last().at(1).toString(), first);
        QVERIFY(panel.state().value("ready").toBool());
        QVERIFY(!panel.state().value("network").toString().isEmpty());
        QVERIFY(panel.ask(QStringLiteral("First turn")));
        QTRY_VERIFY(bridge.chatGenerationActive() && !adapter.runId().isEmpty());
        bridge.setSelectedRuntimeProvider(QStringLiteral("llama-cpp-server"));
        QTRY_COMPARE(bridge.selectedRuntimeProvider(), QString("llama-cpp-server"));
        QCOMPARE(adapter.value("activeChatProviderId").toString(), QString("ollama"));
        bridge.setSelectedRuntimeProvider(QStringLiteral("ollama"));
        QTRY_COMPARE(bridge.selectedRuntimeProvider(), QString("ollama"));
        QVERIFY(panel.cancel());
        QTRY_COMPARE(bridge.chatSendLifecycleState(), QString("cancelled"));
        QVERIFY(!bridge.chatGenerationActive());
        completionGate->release();
        models.state->completionGate.reset();
        models.state->reply = sentinel::test::DeterministicChatReply::Streaming;
        QVERIFY(panel.ask(QStringLiteral("Recovery turn")));
        QTRY_COMPARE(bridge.chatSendLifecycleState(), QString("completed"));
        QTRY_VERIFY(adapter.messages().size() >= 4);
        QCOMPARE(adapter.messages().last().content, QString("SENTINEL_TEST_RESPONSE"));
        QVERIFY(panel.ask(QStringLiteral("Next turn")));
        QTRY_COMPARE(bridge.chatSendLifecycleState(), QString("completed"));
        QTRY_VERIFY(adapter.messages().size() >= 6);
        QVERIFY(!bridge.createConversation(QStringLiteral("Second session")).isEmpty());
        QTRY_VERIFY(adapter.sessionId() != first);
        QVERIFY(bridge.switchConversation(first));
        QTRY_COMPARE(adapter.sessionId(), first);
        QTRY_VERIFY(adapter.messages().size() >= 6);
        transport.disconnectFromDaemon();
        QVERIFY(!panel.ask(QStringLiteral("No fallback")));
        transport.connectToDaemon();
        QTRY_VERIFY(adapter.ready());
        QCOMPARE(adapter.sessionId(), first);
        QVERIFY(!bridge.chatGenerationActive());
        const auto finishedRun = adapter.runId();
        transport.disconnectFromDaemon();
        server.stopServer();
        sentinel::daemon::DaemonIpcServer recovered(controller.get());
        QVERIFY(recovered.openSessionProjectionStore(directory.filePath("sessions.sqlite3")));
        QVERIFY(recovered.startServer(path));
        transport.connectToDaemon();
        QTRY_VERIFY(adapter.ready());
        QTRY_COMPARE(adapter.runId(), finishedRun);
        QCOMPARE(adapter.value("conversationState").toString(), QString("completed"));
    }
    void presentationAdapterAgentApproval_data() {
        QTest::addColumn<QString>("action");
        QTest::newRow("allow") << QString("allow");
        QTest::newRow("deny") << QString("deny");
        QTest::newRow("cancel") << QString("cancel");
    }
    void presentationAdapterAgentApproval() {
        QFETCH(QString, action);
        QTemporaryDir directory{QDir::tempPath() + "/sd-XXXXXX"};
        sentinel::core::AppSettings settings(
            std::make_unique<sentinel::core::InMemorySettingsStore>(),
            sentinel::core::inMemoryTestCredentialStore());
        sentinel::test::DeterministicModelServiceFixture models(
            sentinel::test::DeterministicChatReply::Final, &settings);
        sentinel::core::ApplicationControllerBuilder builder;
        auto controller =
            builder.withModelService(models.takeModelService())
                .withOllamaRuntimeClient(std::make_unique<MetadataClient>())
                .withMemoryStore(std::make_unique<sentinel::core::InMemoryStore>())
                .withAgentRuntime(std::make_unique<sentinel::core::NullAgentRuntime>(
                    sentinel::core::NullAgentRuntime::standardTools()))
                .withAgentStepPlanner(std::make_unique<ApprovalPlanner>())
                .withToolExecutor(std::make_unique<FixtureExecutor>())
                .withSandboxPolicy(std::make_unique<sentinel::core::StaticSandboxPolicy>())
                .build();
        sentinel::daemon::DaemonIpcServer server(controller.get());
        server.setSettings(&settings);
        const auto path = directory.filePath("daemon.sock");
        QVERIFY(server.startServer(path));
        DaemonClient transport(path, 3000, 10);
        sentinel::desktop::DesktopRuntimeClient adapter(transport);
        sentinel::desktop::DesktopControllerBridge bridge({nullptr, &adapter});
        sentinel::desktop::QuickPanelController panel(adapter);
        QSignalSpy notices(&panel, &sentinel::desktop::QuickPanelController::notificationRequested);
        QTRY_VERIFY(adapter.ready());
        QTRY_VERIFY(!adapter.sessionId().isEmpty());
        DaemonClient observerTransport(path, 3000, 10);
        sentinel::desktop::DesktopRuntimeClient observer(observerTransport);
        sentinel::desktop::QuickPanelController observerPanel(observer);
        QTRY_VERIFY(observer.ready() && !observer.sessionId().isEmpty());
        const auto actorSession = adapter.sessionId();
        QVERIFY(!observer.dispatch("createConversation", {QStringLiteral("Observer session")})
                     .toString()
                     .isEmpty());
        QTRY_VERIFY(observer.sessionId() != actorSession);
        QVERIFY(panel.agent(QStringLiteral("Execute fixture action")));
        QTRY_VERIFY(!adapter.pendingApproval().value("approval_id").toString().isEmpty());
        QCOMPARE(panel.state().value("approvalCount").toInt(), 1);
        QCOMPARE(notices.size(), 1);
        observer.refresh();
        QTRY_COMPARE(observerPanel.state().value("approvalCount").toInt(), 1);
        QCOMPARE(observerPanel.state().value("approvalSession").toString(), actorSession);
        const auto approval = adapter.pendingApproval();
        const auto run = adapter.runId();
        transport.eventReceived("output.delta", {{"session_id", adapter.sessionId()},
                                                 {"run_id", run},
                                                 {"text", "INTERNAL_PLAN_MUST_NOT_APPEAR"}});
        QCOMPARE(panel.state().value("preview").toString(), QString{});
        QVERIFY(bridge.agentAwaitingApproval());
        // A resumed run snapshot may carry an old payload, but no live approval ID.
        transport.eventReceived("run.started", {{"session_id", adapter.sessionId()},
                                                {"run_id", run},
                                                {"run_type", "agent"},
                                                {"state", "running"},
                                                {"approval", approval}});
        QVERIFY(adapter.pendingApproval().isEmpty());
        QVERIFY(!panel.approve(true));
        adapter.refresh();
        QTRY_COMPARE(adapter.pendingApproval().value("approval_id"), approval.value("approval_id"));
        transport.disconnectFromDaemon();
        QVERIFY(adapter.pendingApproval().isEmpty());
        QVERIFY(!panel.approve(true));
        transport.connectToDaemon();
        QTRY_VERIFY(adapter.ready());
        QTRY_COMPARE(adapter.pendingApproval().value("approval_id"), approval.value("approval_id"));
        QCOMPARE(adapter.runId(), run);
        QCOMPARE(notices.size(), 1);
        QVERIFY(!bridge.latestAgentActivitySummary().isEmpty());
        if (action == "cancel")
            QVERIFY(panel.cancel());
        else if (action == "allow")
            QVERIFY(observerPanel.approve(true));
        else
            QVERIFY(panel.approve(false));
        QTRY_VERIFY(!bridge.agentLoopActive());
        QTRY_VERIFY(adapter.pendingApproval().isEmpty());
        QCOMPARE(adapter.value("conversationState").toString(),
                 action == "allow"    ? QString("completed")
                 : action == "cancel" ? QString("cancelled")
                                      : QString("failed"));
        if (action == "allow")
            QCOMPARE(bridge.lastAgentResponse(), QString("Fixture finished"));
        QCOMPARE(notices.size(), 2);
        QVERIFY(!panel.approve(true));
        // A late completion from this closed run cannot turn cancellation/denial into success.
        const auto terminal = adapter.value("conversationState");
        transport.eventReceived(QStringLiteral("run.completed"),
                                {{"run_id", run},
                                 {"session_id", adapter.sessionId()},
                                 {"state", "completed"},
                                 {"text", "Late result"},
                                 {"detail", ""},
                                 {"server_generation", transport.serverGeneration()},
                                 {"event_sequence", 99999}});
        QCOMPARE(adapter.value("conversationState"), terminal);
        QVERIFY(panel.agent(QStringLiteral("Next fixture action")));
        QTRY_VERIFY(!adapter.pendingApproval().value("approval_id").toString().isEmpty());
        QVERIFY(panel.cancel());
        QTRY_VERIFY(!bridge.agentLoopActive());
    }
    void daemonSettingsRejectMalformedAndUnknownActions() {
        QTemporaryDir directory{QDir::tempPath() + "/sd-XXXXXX"};
        sentinel::core::AppSettings settings(
            std::make_unique<sentinel::core::InMemorySettingsStore>(),
            sentinel::core::inMemoryTestCredentialStore());
        sentinel::test::DeterministicModelServiceFixture models(
            sentinel::test::DeterministicChatReply::Final, &settings);
        sentinel::core::ApplicationControllerBuilder builder;
        auto controller = builder.withModelService(models.takeModelService())
                              .withMemoryStore(std::make_unique<sentinel::core::InMemoryStore>())
                              .build();
        sentinel::daemon::DaemonIpcServer server(controller.get());
        server.setSettings(&settings);
        const auto path = directory.filePath("daemon.sock");
        QVERIFY(server.startServer(path));
        DaemonClient client(path, 2000, 10);
        QTRY_VERIFY(client.daemonReachable());
        QSignalSpy errors(&client, &DaemonClient::requestFailed);
        QSignalSpy responses(&client, &DaemonClient::responseReceived);
        client.request(DaemonClient::Command::desktop_settings_service,
                       {{"action", "arbitraryInvocation"}, {"arguments", QJsonArray{}}});
        QTRY_COMPARE(errors.size(), 1);
        client.request(
            DaemonClient::Command::desktop_settings_service,
            {{"action", "setProductSetting"}, {"arguments", QJsonArray{true, "Trusted"}}});
        QTRY_COMPARE(errors.size(), 2);
        QCOMPARE(settings.defaultPermissionPolicyState(), QString("Disabled"));
        const auto request = client.request(
            DaemonClient::Command::desktop_settings_service,
            {{"action", "setProductSetting"},
             {"arguments", QJsonArray{"privacy.permission-policy", "Ask Every Time"}}});
        QTRY_VERIFY(([&] {
            for (const auto& response : responses)
                if (response.at(0) == request)
                    return response.at(2)
                        .toJsonObject()
                        .value("result")
                        .toObject()
                        .value("accepted")
                        .toBool();
            return false;
        })());
        QCOMPARE(settings.defaultPermissionPolicyState(), QString("Ask Every Time"));
    }
    void realChatCancelAndRecover() {
        QTemporaryDir directory{QDir::tempPath() + "/sd-XXXXXX"};
        sentinel::test::DeterministicModelServiceFixture models(
            sentinel::test::DeterministicChatReply::Delayed);
        models.state->delayMs = 500;
        sentinel::core::ApplicationControllerBuilder builder;
        auto controller = builder.withModelService(models.takeModelService())
                              .withMemoryStore(std::make_unique<sentinel::core::InMemoryStore>())
                              .build();
        sentinel::daemon::DaemonIpcServer server(controller.get());
        const auto path = directory.filePath("daemon.sock");
        QVERIFY(server.startServer(path));
        DaemonClient client(path, 2000, 10);
        QTRY_VERIFY(client.daemonReachable());
        QSignalSpy replies(&client, &DaemonClient::responseReceived);
        const auto createId =
            client.request(DaemonClient::Command::session_create, {{"title", "Cancel/recover"}});
        QString session;
        QTRY_VERIFY(([&] {
            for (const auto& reply : replies) {
                if (reply.at(0).toString() == createId) {
                    session = reply.at(2).toJsonObject().value("session_id").toString();
                }
            }
            return !session.isEmpty();
        })());
        QSignalSpy events(&client, &DaemonClient::eventReceived);
        const auto sendId = client.request(DaemonClient::Command::chat_send,
                                           {{"session_id", session}, {"text", "cancel this turn"}});
        QString run;
        QTRY_VERIFY(([&] {
            for (const auto& reply : replies) {
                if (reply.at(0).toString() == sendId) {
                    run = reply.at(2).toJsonObject().value("run_id").toString();
                }
            }
            return !run.isEmpty();
        })());
        client.request(DaemonClient::Command::run_cancel, {{"run_id", run}});
        QTRY_VERIFY(([&] {
            for (const auto& event : events) {
                if (event.at(0) == "run.cancelled") {
                    return true;
                }
            }
            return false;
        })());
        QTest::qWait(600);
        for (const auto& event : events) {
            QVERIFY(event.at(0) != "run.completed");
        }
        events.clear();
        client.request(DaemonClient::Command::chat_send,
                       {{"session_id", session}, {"text", "post-cancel turn"}});
        QTRY_VERIFY(([&] {
            for (const auto& event : events) {
                if (event.at(0) == "run.completed") {
                    return true;
                }
            }
            return false;
        })());
        client.disconnectFromDaemon();
        server.stopServer();
    }
    void realAgentApproval_data() {
        QTest::addColumn<QString>("action");
        QTest::newRow("allow") << QString("allow");
        QTest::newRow("deny") << QString("deny");
        QTest::newRow("cancel") << QString("cancel");
    }
    void realAgentApproval() {
        QFETCH(QString, action);
        QTemporaryDir directory{QDir::tempPath() + "/sd-XXXXXX"};
        sentinel::test::DeterministicModelServiceFixture models;
        sentinel::core::ApplicationControllerBuilder builder;
        auto controller =
            builder.withModelService(models.takeModelService())
                .withMemoryStore(std::make_unique<sentinel::core::InMemoryStore>())
                .withAgentRuntime(std::make_unique<sentinel::core::NullAgentRuntime>(
                    sentinel::core::NullAgentRuntime::standardTools()))
                .withAgentStepPlanner(std::make_unique<ApprovalPlanner>())
                .withToolExecutor(std::make_unique<FixtureExecutor>())
                .withSandboxPolicy(std::make_unique<sentinel::core::StaticSandboxPolicy>())
                .build();
        sentinel::daemon::DaemonIpcServer server(controller.get());
        const auto path = directory.filePath("daemon.sock");
        QVERIFY(server.startServer(path));
        DaemonClient desktop(path, 2000, 10);
        QTRY_VERIFY(desktop.daemonReachable());
        QSignalSpy replies(&desktop, &DaemonClient::responseReceived);
        const auto createId =
            desktop.request(DaemonClient::Command::session_create, {{"title", "Agent IPC"}});
        QString session;
        QTRY_VERIFY(([&] {
            for (const auto& reply : replies) {
                if (reply.at(0).toString() == createId) {
                    session = reply.at(2).toJsonObject().value("session_id").toString();
                }
            }
            return !session.isEmpty();
        })());
        QSignalSpy events(&desktop, &DaemonClient::eventReceived);
        desktop.request(DaemonClient::Command::agent_start,
                        {{"session_id", session}, {"text", "perform action"}});
        QJsonObject approval;
        QTRY_VERIFY(([&] {
            for (const auto& event : events) {
                if (event.at(0) == "approval.requested") {
                    approval = event.at(1).toJsonObject();
                }
            }
            return !approval.isEmpty();
        })());
        QVERIFY(!approval.value("approval_id").toString().isEmpty());
        // Reattach with a fresh Desktop client while the daemon retains the pending approval.
        desktop.disconnectFromDaemon();
        DaemonClient reattached(path, 2000, 10);
        QTRY_VERIFY(reattached.daemonReachable());
        QSignalSpy snapshots(&reattached, &DaemonClient::responseReceived);
        const auto attachId =
            reattached.request(DaemonClient::Command::session_attach, {{"session_id", session}});
        QJsonObject snapshot;
        QTRY_VERIFY(([&] {
            for (const auto& reply : snapshots) {
                if (reply.at(0).toString() == attachId) {
                    snapshot = reply.at(2).toJsonObject();
                }
            }
            return !snapshot.isEmpty();
        })());
        QCOMPARE(snapshot.value("state").toString(), QString("approval"));
        QCOMPARE(snapshot.value("approval_id"), approval.value("approval_id"));
        QSignalSpy terminal(&reattached, &DaemonClient::eventReceived);
        QSignalSpy errors(&reattached, &DaemonClient::requestFailed);
        if (action == "cancel") {
            reattached.request(DaemonClient::Command::run_cancel,
                               {{"run_id", approval.value("run_id")}});
        } else {
            reattached.request(DaemonClient::Command::approval_respond,
                               {{"run_id", approval.value("run_id")},
                                {"approval_id", approval.value("approval_id")},
                                {"allow", action == "allow"}});
        }
        QTRY_VERIFY(([&] {
            for (const auto& event : terminal) {
                if (event.at(0) == "run.completed" || event.at(0) == "run.failed" ||
                    event.at(0) == "run.cancelled") {
                    return true;
                }
            }
            return false;
        })());
        if (action == "allow") {
            bool completed = false;
            for (const auto& event : terminal) {
                if (event.at(0) == "run.completed") {
                    completed = true;
                    QCOMPARE(event.at(1).toJsonObject().value("text").toString(),
                             QString("Fixture finished"));
                }
            }
            QVERIFY(completed);
        } else {
            for (const auto& event : terminal) {
                QVERIFY(event.at(0) != "run.completed");
            }
        }
        reattached.request(DaemonClient::Command::approval_respond,
                           {{"run_id", approval.value("run_id")},
                            {"approval_id", approval.value("approval_id")},
                            {"allow", true}});
        QTRY_VERIFY(!errors.isEmpty());
        QCOMPARE(errors.last().at(1).value<DaemonClient::Error>(),
                 DaemonClient::Error::ApprovalExpired);
        reattached.disconnectFromDaemon();
        server.stopServer();
    }
    void realDaemonServiceChatAndMultiClient() {
        QTemporaryDir directory{QDir::tempPath() + "/sd-XXXXXX"};
        sentinel::test::DeterministicModelServiceFixture models(
            sentinel::test::DeterministicChatReply::Streaming);
        sentinel::core::ApplicationControllerBuilder builder;
        auto controller = builder.withModelService(models.takeModelService())
                              .withMemoryStore(std::make_unique<sentinel::core::InMemoryStore>())
                              .build();
        sentinel::daemon::DaemonIpcServer server(controller.get());
        const auto path = directory.filePath("daemon.sock");
        QVERIFY(server.startServer(path));
        DaemonClient desktop(path, 2000, 10), second(path, 2000, 10);
        QTRY_VERIFY(desktop.daemonReachable());
        QTRY_VERIFY(second.daemonReachable());
        QVERIFY(!desktop.serverGeneration().isEmpty());
        QCOMPARE(desktop.serverGeneration(), second.serverGeneration());
        QSignalSpy replies(&desktop, &DaemonClient::responseReceived);
        const auto createId =
            desktop.request(DaemonClient::Command::session_create, {{"title", "IPC desktop"}});
        QString session;
        QTRY_VERIFY(([&] {
            for (const auto& reply : replies) {
                if (reply.at(0).toString() == createId) {
                    session = reply.at(2).toJsonObject().value("session_id").toString();
                }
            }
            return !session.isEmpty();
        })());
        QSignalSpy events(&desktop, &DaemonClient::eventReceived);
        QSignalSpy otherEvents(&second, &DaemonClient::eventReceived);
        desktop.request(DaemonClient::Command::chat_send,
                        {{"session_id", session}, {"text", "fixture prompt"}});
        QTRY_VERIFY(([&] {
            for (const auto& event : events) {
                if (event.at(0) == "run.completed") {
                    return true;
                }
            }
            return false;
        })());
        bool delta = false;
        for (const auto& event : events) {
            delta |= event.at(0) == "output.delta";
            if (event.at(0) == "run.completed") {
                QCOMPARE(event.at(1).toJsonObject().value("text").toString(),
                         QString("SENTINEL_TEST_RESPONSE"));
            }
        }
        QVERIFY(delta);
        QCOMPARE(otherEvents.size(), 0);
        const auto generation = desktop.serverGeneration();
        desktop.disconnectFromDaemon();
        QVERIFY(second.daemonReachable());
        server.stopServer();
        QTRY_VERIFY(!second.daemonReachable());
        QVERIFY(server.startServer(path));
        QTRY_VERIFY(second.daemonReachable());
        QVERIFY(second.serverGeneration() != generation);
        second.disconnectFromDaemon();
        server.stopServer();
        QVERIFY(!QFileInfo::exists(path));
    }
};
QTEST_GUILESS_MAIN(DesktopIpcTest)
#include "test_desktop_ipc.moc"
