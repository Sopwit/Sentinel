// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/DeterministicChatFixture.h"
#include "sentinel/core/agent/AgentRuntime.h"
#include "sentinel/core/agent/IAgentStepPlanner.h"
#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/memory/InMemorySettingsStore.h"
#include "sentinel/core/memory/InMemoryStore.h"
#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/security/StaticSandboxPolicy.h"
#include "service/DaemonIpcServer.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>
#ifdef Q_OS_UNIX
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif
using namespace sentinel;
namespace {
QJsonObject message(const QString& name, const QJsonObject& payload = {}, int major = 1) {
    return {{"version", QJsonObject{{"major", major}, {"minor", 0}}},
            {"type", "request"},
            {"id", "test-id"},
            {"name", name},
            {"payload", payload}};
}
QJsonObject receive(QLocalSocket& socket, int timeoutMs = 2000) {
    QElapsedTimer timer;
    timer.start();
    while (!socket.canReadLine() && timer.elapsed() < timeoutMs)
        QTest::qWait(1);
    return QJsonDocument::fromJson(socket.readLine()).object();
}
QJsonObject request(QLocalSocket& socket, const QString& name, QJsonObject payload = {},
                    int major = 1, int timeoutMs = 2000) {
    socket.write(QJsonDocument(message(name, payload, major)).toJson(QJsonDocument::Compact) +
                 '\n');
    socket.flush();
    return receive(socket, timeoutMs);
}
void hello(QLocalSocket& socket, const QString& path) {
    socket.connectToServer(path);
    QVERIFY(socket.waitForConnected(1000));
    QCOMPARE(
        request(socket, "hello",
                {{"client_id", "test"}, {"major", 1}, {"minor", 0}, {"capabilities", QJsonArray{}}})
            .value("type")
            .toString(),
        QString("response"));
}
class ApprovalPlanner final : public core::IAgentStepPlanner {
public:
    core::AgentStepDecision nextStep(const QString&,
                                     const QList<core::AgentStepRecord>& history) const override {
        core::AgentStepDecision step;
        if (history.isEmpty()) {
            step.kind = core::AgentStepDecision::Kind::ToolCall;
            step.toolId = "run-command";
            step.toolName = "run-command";
            step.riskLevel = core::ToolRiskLevel::High;
            step.arguments.append({QStringLiteral("command"), QStringLiteral("pwd")});
        } else {
            step.kind = core::AgentStepDecision::Kind::FinalAnswer;
            step.answer = "Fixture finished";
        }
        return step;
    }
};
class FixtureExecutor final : public core::IToolExecutor {
public:
    core::ToolExecutionResult execute(const core::ToolExecutionRequest&) const override {
        return {core::ToolExecutionStatus::Succeeded, QStringLiteral("Fixture result")};
    }
};
struct Harness {
    QTemporaryDir directory{QDir::tempPath() + "/sentinel-ipc-XXXXXX"};
    test::DeterministicModelServiceFixture models;
    std::unique_ptr<core::ApplicationController> controller;
    std::unique_ptr<daemon::DaemonIpcServer> server;
    QString path;
    explicit Harness(test::DeterministicChatReply reply = test::DeterministicChatReply::Final,
                     bool agent = false, core::AppSettings* settings = nullptr,
                     bool realFiles = false)
        : models(reply, settings) {
        core::ApplicationControllerBuilder builder;
        if (agent || realFiles) {
            builder
                .withAgentRuntime(std::make_unique<core::NullAgentRuntime>(
                    core::NullAgentRuntime::standardTools()))
                .withAgentStepPlanner(std::make_unique<ApprovalPlanner>())
                .withToolExecutor(realFiles ? std::unique_ptr<core::IToolExecutor>(
                                                  std::make_unique<core::RealToolExecutor>())
                                            : std::unique_ptr<core::IToolExecutor>(
                                                  std::make_unique<FixtureExecutor>()))
                .withSandboxPolicy(std::make_unique<core::StaticSandboxPolicy>());
        }
        controller = builder.withModelService(models.takeModelService())
                         .withMemoryStore(std::make_unique<core::InMemoryStore>())
                         .build();
        server = std::make_unique<daemon::DaemonIpcServer>(controller.get());
        path = directory.filePath("daemon.sock");
    }
};
} // namespace
class DaemonIpcServerTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        QStandardPaths::setTestModeEnabled(true);
        QCoreApplication::setApplicationName("sentinel-phase2-ipc-" +
                                             QUuid::createUuid().toString(QUuid::WithoutBraces));
    }
    void cleanupTestCase() {
        for (const auto location :
             {QStandardPaths::AppDataLocation, QStandardPaths::AppConfigLocation}) {
            const auto path = QStandardPaths::writableLocation(location);
            if (QDir(path).exists())
                QVERIFY(QDir(path).removeRecursively());
        }
    }
    void respondsToPing() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
        QCOMPARE(request(s, "daemon.status")["payload"].toObject()["running"].toBool(), true);
    }
    void respondsToStatusWithControllerData() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
        QCOMPARE(request(s, "model.current")["payload"].toObject()["model_id"].toString(),
                 QString("sentinel-test-model"));
    }
    void terminalDiagnosticsAreSafeAndAuthoritative() {
        core::AppSettings settings(std::make_unique<core::InMemorySettingsStore>(),
                                   core::inMemoryTestCredentialStore());
        Harness h(test::DeterministicChatReply::Final, false, &settings);
        settings.setOpenAiApiKey("fixture-secret-never-print");
        const auto cached = h.controller->modelService()->providerStatusSnapshot("openai");
        QCOMPARE(cached.catalog, core::ProviderCatalogState::Unverified);
        h.server->setSettings(&settings);
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket socket;
        hello(socket, h.path);
        const auto reply = request(socket, "terminal.state", {}, 1, 10000);
        QCOMPARE(reply["type"].toString(), QString("response"));
        const auto properties = reply["payload"].toObject()["properties"].toObject();
        QVERIFY(properties.contains("permission_service_available"));
        QVERIFY(properties.contains("availableToolIds"));
        QVERIFY(!properties.contains("contextReasoningDeveloperTraces"));
        QVERIFY(!properties.contains("apiKey"));
        QVERIFY(reply["payload"].toObject()["providers"].isArray());
        QVERIFY(!QJsonDocument(reply).toJson().contains("fixture-secret-never-print"));
        const QJsonValue sid =
            request(socket, "session.create", {{"title", "safe attach"}})["payload"]
                .toObject()
                .value("session_id");
        const auto attached = request(socket, "terminal.attach", {{"session_id", sid}});
        QCOMPARE(attached["type"].toString(), QString("response"));
        QVERIFY(!attached["payload"].toObject()["properties"].toObject().contains(
            "contextReasoningDeveloperTraces"));
        QVERIFY(!QJsonDocument(attached).toJson().contains("fixture-secret-never-print"));
        QCOMPARE(properties["activeRuntimeModelLabel"].toString(), QString("sentinel-test-model"));
    }
    void workspaceSelectionAndRootAreValidated() {
        core::AppSettings settings(std::make_unique<core::InMemorySettingsStore>(),
                                   core::inMemoryTestCredentialStore());
        Harness h(test::DeterministicChatReply::Final, false, &settings);
        h.server->setSettings(&settings);
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket socket;
        hello(socket, h.path);
        QCOMPARE(request(socket, "workspace.select", {{"workspace_id", "missing"}})["payload"]
                     .toObject()["code"]
                     .toString(),
                 QString("unknown-workspace"));
        QCOMPARE(request(socket, "workspace.select", {{"workspace_id", "coding"}})["payload"]
                     .toObject()["workspace_id"]
                     .toString(),
                 QString("coding"));
        QCOMPARE(settings.selectedWorkspaceId(), QString("coding"));
        QCOMPARE(request(socket, "workspace.root",
                         {{"workspace_id", "coding"}, {"path", h.directory.path()}})["payload"]
                     .toObject()["code"]
                     .toString(),
                 QString("workspace-root-rejected"));
        const QJsonValue created =
            request(socket, "workspace.create",
                    {{"name", "Terminal fixture"}, {"template", "Coding"}})["payload"]
                .toObject()
                .value("workspace_id");
        QVERIFY(!created.toString().isEmpty());
        QCOMPARE(request(socket, "workspace.root",
                         {{"workspace_id", created}, {"path", h.directory.path()}})["type"]
                     .toString(),
                 QString("response"));
        QCOMPARE(
            request(socket, "workspace.root",
                    {{"workspace_id", "coding"}, {"path", "/nonexistent/sentinel-test"}})["payload"]
                .toObject()["code"]
                .toString(),
            QString("workspace-root-rejected"));
    }
    void terminalFilesRespectWorkspaceAndToolBoundaries() {
        core::AppSettings settings(std::make_unique<core::InMemorySettingsStore>(),
                                   core::inMemoryTestCredentialStore());
        Harness h(test::DeterministicChatReply::Final, false, &settings, true);
        core::WorkspaceService workspaces;
        const auto created = workspaces.createWorkspace({}, "File fixture", "Coding");
        const auto rooted = workspaces.setWorkspaceRoot(
            created.catalogJson, created.selectedWorkspaceId, h.directory.path());
        QVERIFY(rooted.success);
        settings.setWorkspaceCatalogJson(rooted.catalogJson);
        settings.setSelectedWorkspaceId(created.selectedWorkspaceId);
        h.controller->attachControlledTaskSettings(settings);
        h.server->setSettings(&settings);
        QFile greeting(h.directory.filePath("greeting.txt"));
        QVERIFY(greeting.open(QIODevice::WriteOnly));
        greeting.write("hello\n");
        greeting.close();
        QFile secret(h.directory.filePath(".env"));
        QVERIFY(secret.open(QIODevice::WriteOnly));
        secret.write("PRIVATE_FIXTURE=hidden\n");
        secret.close();
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket socket;
        hello(socket, h.path);
        const QJsonValue sid =
            request(socket, "session.create", {{"title", "files"}})["payload"].toObject().value(
                "session_id");
        auto* runtime = dynamic_cast<core::AgentRuntime*>(h.controller->agentRuntime());
        QVERIFY(runtime);
        const auto inspected = runtime->inspectWorkspace(sid.toString(), h.directory.path(), "glob",
                                                         h.directory.path());
        QVERIFY2(inspected.status == core::ToolExecutionStatus::Succeeded,
                 qPrintable(inspected.summary));
        const auto listing = request(socket, "workspace.files", {{"session_id", sid}}, 1, 10000);
        QCOMPARE(listing["type"].toString(), QString("response"));
        const auto files = listing["payload"].toObject()["files"].toArray();
        QVERIFY(files.contains(QFileInfo(greeting).canonicalFilePath()));
        QVERIFY(!files.contains(QFileInfo(secret).canonicalFilePath()));
        QCOMPARE(runtime
                     ->inspectWorkspace(sid.toString(), h.directory.path(), "read-file",
                                        secret.fileName())
                     .status,
                 core::ToolExecutionStatus::Blocked);
        QCOMPARE(runtime
                     ->inspectWorkspace(sid.toString(), h.directory.path(), "write-file",
                                        greeting.fileName())
                     .status,
                 core::ToolExecutionStatus::Blocked);
        QTemporaryDir outside;
        QVERIFY(outside.isValid());
        QCOMPARE(
            runtime->inspectWorkspace(sid.toString(), h.directory.path(), "glob", outside.path())
                .status,
            core::ToolExecutionStatus::Blocked);
        settings.setWorkspaceProfilesJson(workspaces.updateProfile(
            {}, created.selectedWorkspaceId, {}, {{"tools", QJsonObject{{"glob", false}}}}));
        QCOMPARE(request(socket, "workspace.files", {{"session_id", sid}})["payload"]
                     .toObject()["code"]
                     .toString(),
                 QString("permission-denied"));
    }
    void activeBindingCannotBeSwitched() {
        Harness h(test::DeterministicChatReply::Delayed);
        h.models.state->delayMs = 300;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket stream, control;
        hello(stream, h.path);
        hello(control, h.path);
        const QJsonValue sid =
            request(stream, "session.create", {{"title", "binding"}})["payload"].toObject().value(
                "session_id");
        QCOMPARE(request(stream, "chat.send", {{"session_id", sid}, {"text", "hello"}})["type"]
                     .toString(),
                 QString("response"));
        QCOMPARE(request(control, "model.select",
                         {{"provider_id", "anything"}, {"model_id", "anything"}})["payload"]
                     .toObject()["code"]
                     .toString(),
                 QString("runtime-busy"));
        h.controller->stopChatGeneration();
        QTest::qWait(350);
    }
    void rejectsUnknownCommand() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
        QCOMPARE(request(s, "bogus")["payload"].toObject()["code"].toString(),
                 QString("unknown-command"));
    }
    void incompatibleMajor() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        s.connectToServer(h.path);
        QVERIFY(s.waitForConnected());
        QCOMPARE(
            request(s, "hello", {{"client_id", "test"}, {"major", 2}, {"minor", 0}}, 2)["payload"]
                .toObject()["code"]
                .toString(),
            QString("incompatible-major"));
    }
    void handshakeRequired() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        s.connectToServer(h.path);
        QVERIFY(s.waitForConnected());
        QCOMPARE(request(s, "daemon.status")["payload"].toObject()["code"].toString(),
                 QString("handshake-required"));
    }
    void multipleClientsAndReconnect() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket a, b;
        hello(a, h.path);
        hello(b, h.path);
        const auto created =
            request(a, "session.create", {{"title", "shared"}})["payload"].toObject();
        const auto attached = request(b, "session.attach", {{"session_id", created["session_id"]}});
        QCOMPARE(attached["type"].toString(), QString("response"));
        a.abort();
        QLocalSocket c;
        hello(c, h.path);
        QCOMPARE(request(c, "session.attach", {{"session_id", created["session_id"]}})["payload"]
                     .toObject()["session_id"],
                 created["session_id"]);
    }
    void collisionDoesNotUnlinkOwner() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        daemon::DaemonIpcServer second;
        QVERIFY(!second.startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
        QCOMPARE(request(s, "daemon.status")["type"].toString(), QString("response"));
    }
    void malformedAndInvalidPayload() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
        s.write("{bad}\n");
        s.flush();
        QCOMPARE(receive(s)["type"].toString(), QString("error"));
        QCOMPARE(request(s, "approval.respond",
                         {{"run_id", "x"}, {"approval_id", "x"}, {"allow", "yes"}})["payload"]
                     .toObject()["code"]
                     .toString(),
                 QString("invalid-payload"));
    }
    void framingAcrossChunks() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
        const auto bytes = QJsonDocument(message("daemon.status")).toJson(QJsonDocument::Compact);
        s.write(bytes.left(10));
        s.flush();
        QTest::qWait(10);
        QVERIFY(!s.canReadLine());
        s.write(bytes.mid(10) + '\n');
        s.flush();
        QCOMPARE(receive(s)["id"].toString(), QString("test-id"));
    }
    void oversizedFrame() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
        s.write(QByteArray(262145, 'a'));
        s.flush();
        QCOMPARE(receive(s)["payload"].toObject()["code"].toString(), QString("message-too-large"));
    }
    void rejectsUnknownSessionAndApprovalReplay() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
        QCOMPARE(request(s, "session.attach", {{"session_id", "missing"}})["payload"]
                     .toObject()["code"]
                     .toString(),
                 QString("unknown-session"));
        QCOMPARE(request(s, "approval.respond",
                         {{"run_id", "old"}, {"approval_id", "old"}, {"allow", true}})["payload"]
                     .toObject()["code"]
                     .toString(),
                 QString("invalid-approval"));
    }
    void deterministicChatEvents() {
        Harness h(test::DeterministicChatReply::Streaming);
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
        auto created = request(s, "session.create", {{"title", "chat"}})["payload"].toObject();
        auto started =
            request(s, "chat.send", {{"session_id", created["session_id"]}, {"text", "hello"}});
        QCOMPARE(started["type"].toString(), QString("response"));
        QString output;
        bool completed = false;
        for (int i = 0; i < 8 && !completed; ++i) {
            auto e = receive(s);
            if (e["name"] == "output.delta")
                output += e["payload"].toObject()["text"].toString();
            completed = e["name"] == "run.completed";
        }
        QVERIFY(completed);
        QCOMPARE(output, QString("SENTINEL_TEST_RESPONSE"));
    }
    void cancellationHasNoLateCompletion() {
        Harness h(test::DeterministicChatReply::Delayed);
        h.models.state->delayMs = 300;
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s, c;
        hello(s, h.path);
        hello(c, h.path);
        auto created = request(s, "session.create", {{"title", "cancel"}})["payload"].toObject();
        auto started =
            request(s, "chat.send",
                    {{"session_id", created["session_id"]}, {"text", "hello"}})["payload"]
                .toObject();
        QCOMPARE(request(c, "run.cancel", {{"run_id", started["run_id"]}})["type"].toString(),
                 QString("response"));
        bool cancelled = false;
        for (int i = 0; i < 8 && !cancelled; ++i) {
            auto e = receive(s);
            QVERIFY(e["name"] != "run.completed");
            cancelled = e["name"] == "run.cancelled";
        }
        QVERIFY(cancelled);
        QTest::qWait(400);
        QVERIFY(!s.canReadLine());
    }
    void agentApprovalRoundTrip() {
        Harness h(test::DeterministicChatReply::Final, true);
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s, control;
        hello(s, h.path);
        hello(control, h.path);
        const auto created =
            request(s, "session.create", {{"title", "Agent"}})["payload"].toObject();
        QCOMPARE(
            request(s, "agent.start",
                    {{"session_id", created["session_id"]}, {"text", "perform action"}})["type"]
                .toString(),
            QString("response"));
        QJsonObject approval;
        for (int i = 0; i < 10 && approval.isEmpty(); ++i) {
            const auto e = receive(s);
            if (e["name"] == "approval.requested")
                approval = e["payload"].toObject();
        }
        QVERIFY(!approval.isEmpty());
        QCOMPARE(approval["state"].toString(), QString("approval"));
        QCOMPARE(request(control, "approval.respond",
                         {{"run_id", approval["run_id"]},
                          {"approval_id", approval["approval_id"]},
                          {"allow", true}})["type"]
                     .toString(),
                 QString("response"));
        QCOMPARE(request(control, "approval.respond",
                         {{"run_id", approval["run_id"]},
                          {"approval_id", approval["approval_id"]},
                          {"allow", true}})["payload"]
                     .toObject()["code"]
                     .toString(),
                 QString("invalid-approval"));
        bool completed = false;
        QElapsedTimer completionTimer;
        completionTimer.start();
        while (!completed && completionTimer.elapsed() < 5000) {
            const auto e = receive(s, 100);
            completed = e["name"] == "run.completed";
            if (completed)
                QCOMPARE(e["payload"].toObject()["text"].toString(), QString("Fixture finished"));
        }
        QVERIFY(completed);
    }
    void disconnectDoesNotCancelChat() {
        Harness h(test::DeterministicChatReply::Delayed);
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
        const auto id = request(s, "session.create", {{"title", "Reconnect"}})["payload"]
                            .toObject()["session_id"]
                            .toString();
        QCOMPARE(
            request(s, "chat.send", {{"session_id", id}, {"text", "hello"}})["type"].toString(),
            QString("response"));
        s.abort();
        QTest::qWait(500);
        QLocalSocket reconnected;
        hello(reconnected, h.path);
        const auto snapshot =
            request(reconnected, "session.attach", {{"session_id", id}})["payload"].toObject();
        QCOMPARE(snapshot["state"].toString(), QString("completed"));
        QVERIFY(!snapshot["output"].toString().isEmpty());
    }
    void agentCancellationDuringApproval() {
        Harness h(test::DeterministicChatReply::Final, true);
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s, control;
        hello(s, h.path);
        hello(control, h.path);
        const auto id = request(s, "session.create", {{"title", "Cancel agent"}})["payload"]
                            .toObject()["session_id"]
                            .toString();
        request(s, "agent.start", {{"session_id", id}, {"text", "perform action"}});
        QJsonObject approval;
        for (int i = 0; i < 10 && approval.isEmpty(); ++i) {
            const auto e = receive(s);
            if (e["name"] == "approval.requested")
                approval = e["payload"].toObject();
        }
        QVERIFY(!approval.isEmpty());
        QCOMPARE(approval["state"].toString(), QString("approval"));
        QCOMPARE(
            request(control, "run.cancel", {{"run_id", approval["run_id"]}})["type"].toString(),
            QString("response"));
        bool cancelled = false;
        for (int i = 0; i < 10 && !cancelled; ++i) {
            const auto e = receive(s);
            QVERIFY(e["name"] != "run.completed");
            cancelled = e["name"] == "run.cancelled";
        }
        QVERIFY(cancelled);
        QTest::qWait(100);
        QVERIFY(!s.canReadLine());
        QCOMPARE(request(control, "approval.respond",
                         {{"run_id", approval["run_id"]},
                          {"approval_id", approval["approval_id"]},
                          {"allow", true}})["type"]
                     .toString(),
                 QString("error"));
    }
    void refusesRegularFileAtEndpoint() {
        Harness h;
        QFile file(h.path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("owned data");
        file.close();
        QVERIFY(!h.server->startServer(h.path));
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.readAll(), QByteArray("owned data"));
    }
    void staleSocketRecovered() {
#ifdef Q_OS_UNIX
        Harness h;
        const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
        QVERIFY(fd >= 0);
        sockaddr_un address{};
        address.sun_family = AF_UNIX;
        const auto path = h.path.toLocal8Bit();
        QVERIFY(path.size() < static_cast<int>(sizeof(address.sun_path)));
        std::memcpy(address.sun_path, path.constData(), path.size() + 1);
        QCOMPARE(::bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)), 0);
        ::close(fd);
        QVERIFY(QFileInfo::exists(h.path));
        QVERIFY(h.server->startServer(h.path));
        QLocalSocket s;
        hello(s, h.path);
#else
        QSKIP("Stale Unix socket recovery is a Unix platform contract.");
#endif
    }
    void ownedSocketRemovedOnStop() {
        Harness h;
        QVERIFY(h.server->startServer(h.path));
        h.server->stopServer();
        QVERIFY(!QFileInfo::exists(h.path));
        QVERIFY(h.server->startServer(h.path));
    }
};
QTEST_GUILESS_MAIN(DaemonIpcServerTest)
#include "test_daemon_ipc.moc"
