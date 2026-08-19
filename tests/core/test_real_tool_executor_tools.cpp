// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QtTest>

#include <QClipboard>
#include <QCoreApplication>
#include <QGuiApplication>

#include "sentinel/core/agent/NullAgentRuntime.h"
#include "sentinel/core/runtime/AlarmStore.h"
#include "sentinel/core/runtime/RealToolExecutor.h"

using namespace sentinel::core;namespace {

class FakeMcpService final : public IMcpService {
public:
    bool addServer(const McpServerConfig& config) override {
        servers_.append(config);
        return true;
    }
    bool removeServer(const QString& serverName) override {
        for (int i = 0; i < servers_.size(); ++i) {
            if (servers_.at(i).name == serverName) {
                servers_.removeAt(i);
                return true;
            }
        }
        return false;
    }
    QList<McpServerConfig> servers() const override { return servers_; }
    McpServerConfig serverConfig(const QString& serverName) const override {
        for (const auto& server : servers_) {
            if (server.name == serverName) {
                return server;
            }
        }
        return {};
    }
    bool connectToServer(const QString& serverName) override { return hasServer(serverName); }
    bool disconnectFromServer(const QString& serverName) override {
        Q_UNUSED(serverName)
        return true;
    }
    McpConnectionState connectionState(const QString& serverName) const override {
        return hasServer(serverName) ? McpConnectionState::Connected
                                     : McpConnectionState::Disconnected;
    }
    QList<McpToolDefinition> tools(const QString& serverName) const override {
        QList<McpToolDefinition> result;
        if (serverName.isEmpty() || serverName == QStringLiteral("weather")) {
            result.append(McpToolDefinition{
                QStringLiteral("get_forecast"),
                QStringLiteral("Returns the weather forecast for a city."),
                QStringLiteral("weather"),
                QJsonObject(),
            });
        }
        return result;
    }
    QJsonObject callTool(const QString& serverName, const QString& toolName,
                         const QJsonObject& arguments) override {
        lastServer = serverName;
        lastTool = toolName;
        lastArguments = arguments;
        if (toolName == QStringLiteral("boom")) {
            return QJsonObject{{"error", QJsonObject{{"message", "server exploded"}}}};
        }
        QJsonObject contentItem{{"type", "text"}, {"text", "sunny, 24C"}};
        return QJsonObject{{"result", QJsonObject{{"content", QJsonArray{contentItem}}}}};
    }
    bool connectToAll() override { return true; }
    void disconnectFromAll() override {}

    // IMcpService declares its notification hooks as pure virtual "signals";
    // the test double keeps them as no-ops.
    void serverConnected(const QString&) override {}
    void serverDisconnected(const QString&) override {}
    void serverError(const QString&, const QString&) override {}
    void toolsUpdated(const QString&) override {}

    bool hasServer(const QString& name) const {
        for (const auto& server : servers_) {
            if (server.name == name) {
                return true;
            }
        }
        return false;
    }

    QList<McpServerConfig> servers_;
    QString lastServer;
    QString lastTool;
    QJsonObject lastArguments;
};

ToolInvocationPlan approvedPlan(const QString& toolId,
                                const QList<ToolInvocationArgument>& arguments,
                                ToolRiskLevel risk = ToolRiskLevel::Low) {
    ToolInvocationPlan plan;
    plan.status = ToolInvocationPlanStatus::Planned;
    plan.summary = QStringLiteral("test plan");
    plan.invocations.append(PlannedToolInvocation{
        toolId, toolId, QStringLiteral("test"), QStringLiteral("test"), risk,
        ToolExecutionMode::Local, arguments, {}});
    return plan;
}

ToolExecutionResult runTool(const RealToolExecutor& executor, const QString& toolId,
                            const QList<ToolInvocationArgument>& arguments,
                            const QStringList& knownToolIds) {
    return executor.execute(ToolExecutionRequest{
        approvedPlan(toolId, arguments),
        ApprovalDecision{ApprovalStatus::Approved, QStringLiteral("test"), {}},
        SandboxEvaluationResult{SandboxStatus::Allowed, QStringLiteral("test"), {}},
        knownToolIds,
    });
}

QStringList allToolIds() {
    QStringList ids;
    for (const auto& tool : NullAgentRuntime::standardTools()) {
        ids.append(tool.id);
    }
    return ids;
}

} // namespace

class RealToolExecutorToolsTest final : public QObject {
    Q_OBJECT

private slots:
    void readFileReturnsNumberedLinesAndPagingFooter() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("sample.txt"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("alpha\nbeta\ngamma\n");
        file.close();

        // The executor scopes reads to the current working directory.
        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result = runTool(executor, QStringLiteral("read-file"),
                                    {ToolInvocationArgument{QStringLiteral("path"),
                                                            QStringLiteral("sample.txt")}},
                                    allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("1: alpha")));
        QVERIFY(result.summary.contains(QStringLiteral("3: gamma")));
        QVERIFY(result.summary.contains(QStringLiteral("total 3 lines")));
    }

    void readFileOffsetOutOfRangeFailsGracefully() {
        QTemporaryDir dir;
        QFile file(dir.filePath(QStringLiteral("tiny.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("only line\n");
        file.close();

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("read-file"),
            {ToolInvocationArgument{QStringLiteral("path"), QStringLiteral("tiny.txt")},
             ToolInvocationArgument{QStringLiteral("offset"), QStringLiteral("42")}},
            allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("out of range")));
    }

    void readDirectoryListsEntries() {
        QTemporaryDir dir;
        QVERIFY(QDir(dir.filePath(QStringLiteral("nested"))).mkpath(QStringLiteral(".")));

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result =
            runTool(executor, QStringLiteral("read-file"),
                    {ToolInvocationArgument{QStringLiteral("path"), QStringLiteral(".")}},
                    allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("nested/")));
    }

    void editFileAppliesReplacement() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("code.cpp"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("int main() {\n    return 1;\n}\n");
        file.close();

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("edit-file"),
            {ToolInvocationArgument{QStringLiteral("path"), QStringLiteral("code.cpp")},
             ToolInvocationArgument{QStringLiteral("oldString"), QStringLiteral("return 1;")},
             ToolInvocationArgument{QStringLiteral("newString"), QStringLiteral("return 0;")}},
            allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("edit-file: Edited")));

        QFile updated(path);
        QVERIFY(updated.open(QIODevice::ReadOnly));
        const QString content = QString::fromUtf8(updated.readAll());
        QVERIFY(content.contains(QStringLiteral("return 0;")));
        QVERIFY(!content.contains(QStringLiteral("return 1;")));
    }

    void editFileCreatesNewFileWithEmptyOldString() {
        QTemporaryDir dir;
        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("edit-file"),
            {ToolInvocationArgument{QStringLiteral("path"), QStringLiteral("fresh.md")},
             ToolInvocationArgument{QStringLiteral("oldString"), QString()},
             ToolInvocationArgument{QStringLiteral("newString"), QStringLiteral("# hello")}},
            allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(QFile::exists(dir.filePath(QStringLiteral("fresh.md"))));
    }

    void grepFindsMatchesUnderDirectory() {
        QTemporaryDir dir;
        QFile a(dir.filePath(QStringLiteral("a.txt")));
        QVERIFY(a.open(QIODevice::WriteOnly | QIODevice::Text));
        a.write("keep this line\nnothing here\n");
        a.close();
        QFile b(dir.filePath(QStringLiteral("b.txt")));
        QVERIFY(b.open(QIODevice::WriteOnly | QIODevice::Text));
        b.write("also keep this one\n");
        b.close();

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result = runTool(executor, QStringLiteral("grep"),
                                    {ToolInvocationArgument{QStringLiteral("pattern"),
                                                            QStringLiteral("keep this")}},
                                    allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("a.txt:")));
        QVERIFY(result.summary.contains(QStringLiteral("Line 1: keep this line")));
        QVERIFY(result.summary.contains(QStringLiteral("2 match")));
    }

    void globListsMatchingFiles() {
        QTemporaryDir dir;
        QVERIFY(QFile(dir.filePath(QStringLiteral("one.cpp"))).open(QIODevice::WriteOnly));
        QVERIFY(QFile(dir.filePath(QStringLiteral("two.cpp"))).open(QIODevice::WriteOnly));
        QVERIFY(QFile(dir.filePath(QStringLiteral("three.txt"))).open(QIODevice::WriteOnly));

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result =
            runTool(executor, QStringLiteral("glob"),
                    {ToolInvocationArgument{QStringLiteral("pattern"), QStringLiteral("*.cpp")}},
                    allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("one.cpp")));
        QVERIFY(result.summary.contains(QStringLiteral("two.cpp")));
        QVERIFY(!result.summary.contains(QStringLiteral("three.txt")));
    }

    void todoWriteAndReadRoundTrip() {
        RealToolExecutor executor;
        const QString todos = QStringLiteral(
            "[{\"content\":\"find files\",\"status\":\"completed\",\"priority\":\"high\"},"
            "{\"content\":\"edit config\",\"status\":\"in_progress\",\"priority\":\"medium\"}]");

        const auto writeResult = runTool(
            executor, QStringLiteral("todo-write"),
            {ToolInvocationArgument{QStringLiteral("todos"), todos}}, allToolIds());
        QCOMPARE(writeResult.status, ToolExecutionStatus::Succeeded);
        QVERIFY(writeResult.summary.contains(QStringLiteral("2 todo")));

        const auto readResult =
            runTool(executor, QStringLiteral("todo-read"), {}, allToolIds());
        QCOMPARE(readResult.status, ToolExecutionStatus::Succeeded);
        QVERIFY(readResult.summary.contains(QStringLiteral("find files")));
        QVERIFY(readResult.summary.contains(QStringLiteral("in_progress")));
    }

    void todoWriteRejectsInvalidStatus() {
        RealToolExecutor executor;
        const auto result =
            runTool(executor, QStringLiteral("todo-write"),
                    {ToolInvocationArgument{QStringLiteral("todos"),
                                            QStringLiteral("[{\"content\":\"x\",\"status\":"
                                                           "\"banana\"}]")}},
                    allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("Invalid status 'banana'")));
    }

    void setAlarmAndListAlarmsRoundTrip() {
        QTemporaryDir dir;
        auto alarmStore = std::make_shared<AlarmStore>(dir.filePath(QStringLiteral("alarms.json")));

        RealToolExecutor executor;
        executor.setAlarmStore(alarmStore);

        const auto setResult = runTool(
            executor, QStringLiteral("set-alarm"),
            {ToolInvocationArgument{QStringLiteral("time"), QStringLiteral("23:59")},
             ToolInvocationArgument{QStringLiteral("label"), QStringLiteral("test alarm")}},
            allToolIds());

        QCOMPARE(setResult.status, ToolExecutionStatus::Succeeded);
        QVERIFY(setResult.summary.contains(QStringLiteral("Alarm scheduled")));

        const auto listResult =
            runTool(executor, QStringLiteral("list-alarms"), {}, allToolIds());
        QCOMPARE(listResult.status, ToolExecutionStatus::Succeeded);
        QVERIFY(listResult.summary.contains(QStringLiteral("test alarm")));
    }

    void setAlarmRejectsUnparseableTime() {
        RealToolExecutor executor;
        executor.setAlarmStore(std::make_shared<AlarmStore>(QString()));

        const auto result =
            runTool(executor, QStringLiteral("set-alarm"),
                    {ToolInvocationArgument{QStringLiteral("time"), QStringLiteral("next tuesday")},
                     ToolInvocationArgument{QStringLiteral("label"), QStringLiteral("x")}},
                    allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("Could not parse time")));
    }

    void alarmStorePersistsAndTakesDue() {
        QTemporaryDir dir;
        const QString path = dir.filePath(QStringLiteral("alarms.json"));
        {
            AlarmStore store(path);
            store.schedule(QDateTime::currentDateTime().addSecs(-10), QStringLiteral("past"));
            store.schedule(QDateTime::currentDateTime().addSecs(3600), QStringLiteral("future"));
        }

        AlarmStore reloaded(path);
        QCOMPARE(reloaded.active().size(), 2);

        const auto due = reloaded.takeDue(QDateTime::currentDateTime());
        QCOMPARE(due.size(), 1);
        QCOMPARE(due.first().label, QStringLiteral("past"));
        QCOMPARE(reloaded.active().size(), 1);
    }

    void rejectsPathOutsideWorkspace() {
        QTemporaryDir dir;
        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("read-file"),
            {ToolInvocationArgument{QStringLiteral("path"), QStringLiteral("/etc/passwd")}},
            allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("outside the approved workspace")));
    }

    void deleteFileRemovesFileAndRefusesDirectories() {
        QTemporaryDir dir;
        QVERIFY(QFile(dir.filePath(QStringLiteral("doomed.txt"))).open(QIODevice::WriteOnly));
        QVERIFY(QDir(dir.filePath(QStringLiteral("keepdir"))).mkpath(QStringLiteral(".")));

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto fileResult =
            runTool(executor, QStringLiteral("delete-file"),
                    {ToolInvocationArgument{QStringLiteral("path"),
                                            QStringLiteral("doomed.txt")}},
                    allToolIds());
        const auto dirResult =
            runTool(executor, QStringLiteral("delete-file"),
                    {ToolInvocationArgument{QStringLiteral("path"), QStringLiteral("keepdir")}},
                    allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(fileResult.status, ToolExecutionStatus::Succeeded);
        QVERIFY(fileResult.summary.contains(QStringLiteral("delete-file: Deleted")));
        QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("doomed.txt"))));
        QVERIFY(dirResult.summary.contains(QStringLiteral("Refusing to delete a directory")));
        QVERIFY(QDir(dir.filePath(QStringLiteral("keepdir"))).exists());
    }

    void deleteFileRefusesPathOutsideWorkspace() {
        QTemporaryDir dir;
        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result =
            runTool(executor, QStringLiteral("delete-file"),
                    {ToolInvocationArgument{QStringLiteral("path"), QStringLiteral("/etc/passwd")}},
                    allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("outside the approved workspace")));
        QVERIFY(QFile::exists(QStringLiteral("/etc/passwd")));
    }

    void moveFileRenamesWithinWorkspace() {
        QTemporaryDir dir;
        const QString source = dir.filePath(QStringLiteral("old-name.txt"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("payload\n");
        file.close();

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("move-file"),
            {ToolInvocationArgument{QStringLiteral("source"), QStringLiteral("old-name.txt")},
             ToolInvocationArgument{QStringLiteral("destination"),
                                    QStringLiteral("new-name.txt")}},
            allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("move-file: Moved")));
        QVERIFY(!QFile::exists(source));
        QFile moved(dir.filePath(QStringLiteral("new-name.txt")));
        QVERIFY(moved.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(moved.readAll()), QStringLiteral("payload\n"));
    }

    void moveFileRefusesExistingDestination() {
        QTemporaryDir dir;
        QVERIFY(QFile(dir.filePath(QStringLiteral("a.txt"))).open(QIODevice::WriteOnly));
        QVERIFY(QFile(dir.filePath(QStringLiteral("b.txt"))).open(QIODevice::WriteOnly));

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("move-file"),
            {ToolInvocationArgument{QStringLiteral("source"), QStringLiteral("a.txt")},
             ToolInvocationArgument{QStringLiteral("destination"), QStringLiteral("b.txt")}},
            allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("Destination already exists")));
        QVERIFY(QFile::exists(dir.filePath(QStringLiteral("a.txt"))));
        QVERIFY(QFile::exists(dir.filePath(QStringLiteral("b.txt"))));
    }

    void cancelAlarmRemovesScheduledAlarm() {
        QTemporaryDir dir;
        auto alarmStore = std::make_shared<AlarmStore>(dir.filePath(QStringLiteral("alarms.json")));
        const auto entry = alarmStore->schedule(
            QDateTime::currentDateTime().addSecs(3600), QStringLiteral("will cancel"));

        RealToolExecutor executor;
        executor.setAlarmStore(alarmStore);

        const auto cancelResult =
            runTool(executor, QStringLiteral("cancel-alarm"),
                    {ToolInvocationArgument{QStringLiteral("id"), entry.id}}, allToolIds());
        QCOMPARE(cancelResult.status, ToolExecutionStatus::Succeeded);
        QVERIFY(cancelResult.summary.contains(QStringLiteral("cancel-alarm: Alarm %1 cancelled")
                                                   .arg(entry.id)));
        QCOMPARE(alarmStore->active().size(), 0);

        const auto missingResult =
            runTool(executor, QStringLiteral("cancel-alarm"),
                    {ToolInvocationArgument{QStringLiteral("id"), QStringLiteral("nope")}},
                    allToolIds());
        QVERIFY(missingResult.summary.contains(
            QStringLiteral("No active alarm with id nope")));
    }

    void openUrlRejectsNonHttpScheme() {
        RealToolExecutor executor;
        const auto result =
            runTool(executor, QStringLiteral("open-url"),
                    {ToolInvocationArgument{QStringLiteral("url"),
                                            QStringLiteral("file:///etc/passwd")}},
                    allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("Only http and https URLs can be opened")));
    }

    void currentTimeReportsUtcAndEpoch() {
        RealToolExecutor executor;
        const auto result =
            runTool(executor, QStringLiteral("current-time"), {}, allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("current-time:")));
        QVERIFY(result.summary.contains(QStringLiteral("UTC:")));
        QVERIFY(result.summary.contains(QStringLiteral("Epoch seconds:")));
        QVERIFY(result.summary.contains(
            QString::number(QDateTime::currentDateTime().date().year())));
    }

    void systemInfoReportsPlatform() {
        RealToolExecutor executor;
        const auto result = runTool(executor, QStringLiteral("system-info"), {}, allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("OS:")));
        QVERIFY(result.summary.contains(QStringLiteral("CPU architecture:")));
        QVERIFY(result.summary.contains(QStringLiteral("Hostname:")));
    }

    void processListReportsProcessLines() {
        RealToolExecutor executor;
        const auto result = runTool(executor, QStringLiteral("process-list"), {}, allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("process-list:")));
    }

    void clipboardToolsFailGracefullyWithoutGuiSession() {
        // With a GUI session present the tools round-trip through the real
        // clipboard; without one they must report unavailability instead of
        // crashing. The original clipboard content is restored afterwards.
        const auto* guiApp = qobject_cast<const QGuiApplication*>(QCoreApplication::instance());
        if (!guiApp) {
            RealToolExecutor executor;
            const auto readResult =
                runTool(executor, QStringLiteral("clipboard-read"), {}, allToolIds());
            QCOMPARE(readResult.status, ToolExecutionStatus::Succeeded);
            QVERIFY(readResult.summary.contains(
                QStringLiteral("unavailable without a GUI session")));
            return;
        }

        QClipboard* clipboard = QGuiApplication::clipboard();
        const QString original = clipboard->text();
        const QString sample = QStringLiteral("sentinel-clipboard-test-42");

        RealToolExecutor executor;
        const auto writeResult = runTool(
            executor, QStringLiteral("clipboard-write"),
            {ToolInvocationArgument{QStringLiteral("text"), sample}}, allToolIds());
        QCOMPARE(writeResult.status, ToolExecutionStatus::Succeeded);
        QVERIFY(writeResult.summary.contains(QStringLiteral("Copied 26 character(s)")));

        const auto readResult =
            runTool(executor, QStringLiteral("clipboard-read"), {}, allToolIds());
        QCOMPARE(readResult.status, ToolExecutionStatus::Succeeded);
        QVERIFY(readResult.summary.contains(sample));

        clipboard->setText(original);
    }

    void memorySearchFindsSnapshotEntries() {
        RealToolExecutor executor;
        executor.setMemorySnapshot({{QStringLiteral("user_name"), QStringLiteral("Ahmet")},
                                    {QStringLiteral("shopping"), QStringLiteral("buy oat milk")}});

        const auto result =
            runTool(executor, QStringLiteral("memory-search"),
                    {ToolInvocationArgument{QStringLiteral("query"), QStringLiteral("milk")}},
                    allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("1 match(es) for 'milk'")));
        QVERIFY(result.summary.contains(QStringLiteral("shopping: buy oat milk")));

        const auto noMatch =
            runTool(executor, QStringLiteral("memory-search"),
                    {ToolInvocationArgument{QStringLiteral("query"), QStringLiteral("yzk")}},
                    allToolIds());
        QVERIFY(noMatch.summary.contains(QStringLiteral("No memory entries match 'yzk'")));
    }

    void memorySearchWithoutSnapshotIsGraceful() {
        RealToolExecutor executor;
        const auto result =
            runTool(executor, QStringLiteral("memory-search"),
                    {ToolInvocationArgument{QStringLiteral("query"), QStringLiteral("anything")}},
                    allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(
            QStringLiteral("No memory entries are available for this session")));
    }

    void appLaunchRedirectsDomainNamesToOpenUrl() {
        // Domains must never be launched as applications; the executor guides
        // the caller to open-url instead of spawning anything.
        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("app-launch"),
            {ToolInvocationArgument{QStringLiteral("app"), QStringLiteral("sahibinden.com")}},
            allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("is a website address")));
        QVERIFY(result.summary.contains(QStringLiteral("open-url")));
    }

    void applyPatchUpdatesExistingFile() {
        QTemporaryDir dir;
        QFile file(dir.filePath(QStringLiteral("app.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("alpha\nbeta\ngamma\n");
        file.close();

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        const QString patch = QStringLiteral("--- a/app.txt\n"
                                             "+++ b/app.txt\n"
                                             "@@ -1,3 +1,3 @@\n"
                                             " alpha\n"
                                             "-beta\n"
                                             "+BETA\n"
                                             " gamma\n");

        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("apply-patch"),
            {ToolInvocationArgument{QStringLiteral("patch"), patch}}, allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("updated")));
        QVERIFY(result.summary.contains(QStringLiteral("1 hunk")));

        QFile updated(dir.filePath(QStringLiteral("app.txt")));
        QVERIFY(updated.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(updated.readAll()), QStringLiteral("alpha\nBETA\ngamma"));
    }

    void applyPatchAddsAndDeletesFiles() {
        QTemporaryDir dir;
        QVERIFY(QFile(dir.filePath(QStringLiteral("old.txt"))).open(QIODevice::WriteOnly));

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        const QString patch = QStringLiteral(
            "--- /dev/null\n"
            "+++ b/new.txt\n"
            "@@ -0,0 +1,2 @@\n"
            "+first\n"
            "+second\n"
            "--- a/old.txt\n"
            "+++ /dev/null\n"
            "@@ -1 +0,0 @@\n"
            "-content\n");

        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("apply-patch"),
            {ToolInvocationArgument{QStringLiteral("patch"), patch}}, allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("added")));
        QVERIFY(result.summary.contains(QStringLiteral("deleted")));
        QVERIFY(!QFile::exists(dir.filePath(QStringLiteral("old.txt"))));

        QFile created(dir.filePath(QStringLiteral("new.txt")));
        QVERIFY(created.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(created.readAll()), QStringLiteral("first\nsecond"));
    }

    void applyPatchReportsContextMismatch() {
        QTemporaryDir dir;
        QFile file(dir.filePath(QStringLiteral("app.txt")));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("one\ntwo\nthree\n");
        file.close();

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        const QString patch = QStringLiteral("--- a/app.txt\n"
                                             "+++ b/app.txt\n"
                                             "@@ -1,3 +1,3 @@\n"
                                             " wrong-context\n"
                                             "-two\n"
                                             "+TWO\n"
                                             " three\n");

        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("apply-patch"),
            {ToolInvocationArgument{QStringLiteral("patch"), patch}}, allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("did not match")));

        // The file must remain untouched.
        QFile unchanged(dir.filePath(QStringLiteral("app.txt")));
        QVERIFY(unchanged.open(QIODevice::ReadOnly));
        QCOMPARE(QString::fromUtf8(unchanged.readAll()), QStringLiteral("one\ntwo\nthree\n"));
    }

    void applyPatchRejectsInvalidPatchText() {
        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("apply-patch"),
            {ToolInvocationArgument{QStringLiteral("patch"),
                                    QStringLiteral("this is not a patch")}},
            allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("No file sections found")));
    }

    void listCodeDefinitionsExtractsSymbols() {
        QTemporaryDir dir;
        QFile file(dir.filePath(QStringLiteral("sample.py")));
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("class Greeter:\n"
                   "    def greet(self):\n"
                   "        return 1\n"
                   "\n"
                   "def main():\n"
                   "    pass\n");
        file.close();

        const auto oldCwd = QDir::currentPath();
        QVERIFY(QDir::setCurrent(dir.path()));

        RealToolExecutor executor;
        const auto result =
            runTool(executor, QStringLiteral("list-code-definitions"),
                    {ToolInvocationArgument{QStringLiteral("path"), QStringLiteral("sample.py")}},
                    allToolIds());

        QVERIFY(QDir::setCurrent(oldCwd));

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("class Greeter")));
        QVERIFY(result.summary.contains(QStringLiteral("def greet")));
        QVERIFY(result.summary.contains(QStringLiteral("def main")));
    }

    void historySearchFindsSnapshotEntries() {
        RealToolExecutor executor;
        executor.setHistorySnapshot({QStringLiteral("[user] bisiklet tamir etmem lazım"),
                                     QStringLiteral("[assistant] hangi parça sorunlu?")});

        const auto result = runTool(
            executor, QStringLiteral("history-search"),
            {ToolInvocationArgument{QStringLiteral("query"), QStringLiteral("bisiklet")}},
            allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("1 match(es) for 'bisiklet'")));
        QVERIFY(result.summary.contains(QStringLiteral("[user] bisiklet")));

        const auto missing =
            runTool(executor, QStringLiteral("history-search"),
                    {ToolInvocationArgument{QStringLiteral("query"), QStringLiteral("xyzzy")}},
                    allToolIds());
        QVERIFY(missing.summary.contains(QStringLiteral("No history entries match 'xyzzy'")));
    }

    void historySearchWithoutSnapshotIsGraceful() {
        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("history-search"),
            {ToolInvocationArgument{QStringLiteral("query"), QStringLiteral("anything")}},
            allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(
            result.summary.contains(QStringLiteral("No chat history is available")));
    }

    void askQuestionReturnsGuidanceToFinishRun() {
        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("ask-question"),
            {ToolInvocationArgument{QStringLiteral("question"),
                                    QStringLiteral("Hangi dosyayı düzenleyelim?")},
             ToolInvocationArgument{QStringLiteral("options"),
                                    QStringLiteral("config.json\nsettings.ini")}},
            allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("Hangi dosyayı düzenleyelim?")));
        QVERIFY(result.summary.contains(QStringLiteral("1. config.json")));
        QVERIFY(result.summary.contains(QStringLiteral("final answer")));
    }

    void mcpToolsWithoutServiceAreGraceful() {
        RealToolExecutor executor;
        const auto listResult = runTool(executor, QStringLiteral("mcp-list"), {}, allToolIds());
        QCOMPARE(listResult.status, ToolExecutionStatus::Succeeded);
        QVERIFY(listResult.summary.contains(QStringLiteral("No MCP servers are configured")));

        const auto callResult = runTool(
            executor, QStringLiteral("mcp-call"),
            {ToolInvocationArgument{QStringLiteral("server"), QStringLiteral("weather")},
             ToolInvocationArgument{QStringLiteral("tool"), QStringLiteral("get_forecast")}},
            allToolIds());
        QCOMPARE(callResult.status, ToolExecutionStatus::Succeeded);
        QVERIFY(callResult.summary.contains(QStringLiteral("No MCP servers are configured")));
    }

    void mcpListShowsServersAndTools() {
        RealToolExecutor executor;
        auto service = std::make_shared<FakeMcpService>();
        McpServerConfig config;
        config.name = QStringLiteral("weather");
        config.type = QStringLiteral("local");
        config.command = QStringLiteral("weather-mcp");
        service->addServer(config);
        executor.setMcpService(service);

        const auto result = runTool(executor, QStringLiteral("mcp-list"), {}, allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("weather (local, connected)")));
        QVERIFY(result.summary.contains(QStringLiteral("get_forecast")));
    }

    void mcpCallInvokesServerTool() {
        RealToolExecutor executor;
        auto service = std::make_shared<FakeMcpService>();
        executor.setMcpService(service);

        const auto result = runTool(
            executor, QStringLiteral("mcp-call"),
            {ToolInvocationArgument{QStringLiteral("server"), QStringLiteral("weather")},
             ToolInvocationArgument{QStringLiteral("tool"), QStringLiteral("get_forecast")},
             ToolInvocationArgument{QStringLiteral("arguments"),
                                    QStringLiteral("{\"city\": \"Ankara\"}")}},
            allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(QStringLiteral("weather/get_forecast result:")));
        QVERIFY(result.summary.contains(QStringLiteral("sunny, 24C")));
        QCOMPARE(service->lastServer, QStringLiteral("weather"));
        QCOMPARE(service->lastTool, QStringLiteral("get_forecast"));
        QCOMPARE(service->lastArguments.value(QStringLiteral("city")).toString(),
                 QStringLiteral("Ankara"));
    }

    void mcpCallReportsServerErrorAndBadArguments() {
        RealToolExecutor executor;
        auto service = std::make_shared<FakeMcpService>();
        executor.setMcpService(service);

        const auto errorResult = runTool(
            executor, QStringLiteral("mcp-call"),
            {ToolInvocationArgument{QStringLiteral("server"), QStringLiteral("weather")},
             ToolInvocationArgument{QStringLiteral("tool"), QStringLiteral("boom")}},
            allToolIds());
        QVERIFY(errorResult.summary.contains(QStringLiteral("server exploded")));

        const auto badArgs = runTool(
            executor, QStringLiteral("mcp-call"),
            {ToolInvocationArgument{QStringLiteral("server"), QStringLiteral("weather")},
             ToolInvocationArgument{QStringLiteral("tool"), QStringLiteral("get_forecast")},
             ToolInvocationArgument{QStringLiteral("arguments"), QStringLiteral("not json")}},
            allToolIds());
        QVERIFY(badArgs.summary.contains(QStringLiteral("must be a JSON object")));
    }

    void spawnAgentRunsInjectedSubagent() {
        RealToolExecutor executor;
        QStringList receivedTasks;
        executor.setSubagentRunner([&receivedTasks](const QString& task) {
            receivedTasks.append(task);
            return QStringLiteral("subagent answer for: %1").arg(task);
        });

        const auto result = runTool(
            executor, QStringLiteral("spawn-agent"),
            {ToolInvocationArgument{QStringLiteral("task"),
                                    QStringLiteral("count the TODOs")}},
            allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QCOMPARE(receivedTasks.size(), 1);
        QCOMPARE(receivedTasks.first(), QStringLiteral("count the TODOs"));
        QVERIFY(result.summary.contains(QStringLiteral("subagent finished")));
        QVERIFY(result.summary.contains(QStringLiteral("subagent answer for: count the TODOs")));
    }

    void spawnAgentWithoutRunnerIsGraceful() {
        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("spawn-agent"),
            {ToolInvocationArgument{QStringLiteral("task"), QStringLiteral("anything")}},
            allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(
            QStringLiteral("No subagent runner is configured")));
    }

    void spawnAgentRequiresTask() {
        RealToolExecutor executor;
        const auto result =
            runTool(executor, QStringLiteral("spawn-agent"), {}, allToolIds());
        QVERIFY(result.summary.contains(QStringLiteral("No task argument provided")));
    }

    void runCommandDockerSandboxReportsMissingDocker() {
        // On machines without docker the tool must degrade to clear guidance
        // instead of failing the whole plan. (Docker installs run the same
        // code path with a real container.)
        const bool hasDocker =
            QProcess::execute(QStringLiteral("docker"),
                              {QStringLiteral("--version")}) == 0;
        if (hasDocker) {
            QSKIP("docker is installed on this machine; the missing-docker branch cannot run.");
        }

        RealToolExecutor executor;
        const auto result = runTool(
            executor, QStringLiteral("run-command"),
            {ToolInvocationArgument{QStringLiteral("command"), QStringLiteral("ls")},
             ToolInvocationArgument{QStringLiteral("sandbox"), QStringLiteral("docker")}},
            allToolIds());

        QCOMPARE(result.status, ToolExecutionStatus::Succeeded);
        QVERIFY(result.summary.contains(
            QStringLiteral("docker CLI is not available")));
    }

    void browserToolsReportMissingNodeGracefully() {
        // Playwright tools require Node.js (npx). Without it they must return
        // install guidance rather than crash.
        const bool hasNode =
            QProcess::execute(QStringLiteral("npx"), {QStringLiteral("--version")}) == 0;
        if (hasNode) {
            QSKIP("npx is installed on this machine; the missing-node branch cannot run.");
        }

        RealToolExecutor executor;
        const auto screenshot = runTool(
            executor, QStringLiteral("browser-screenshot"),
            {ToolInvocationArgument{QStringLiteral("url"), QStringLiteral("https://example.com")}},
            allToolIds());
        QCOMPARE(screenshot.status, ToolExecutionStatus::Succeeded);
        QVERIFY(screenshot.summary.contains(QStringLiteral("Node.js")));

        const auto pdf = runTool(
            executor, QStringLiteral("browser-pdf"),
            {ToolInvocationArgument{QStringLiteral("url"), QStringLiteral("https://example.com")}},
            allToolIds());
        QCOMPARE(pdf.status, ToolExecutionStatus::Succeeded);
        QVERIFY(pdf.summary.contains(QStringLiteral("Node.js")));
    }

    void browserToolsRequireUrl() {
        RealToolExecutor executor;
        const auto screenshot =
            runTool(executor, QStringLiteral("browser-screenshot"), {}, allToolIds());
        QVERIFY(screenshot.summary.contains(QStringLiteral("No url argument provided")));

        const auto pdf = runTool(executor, QStringLiteral("browser-pdf"), {}, allToolIds());
        QVERIFY(pdf.summary.contains(QStringLiteral("No url argument provided")));
    }
};

QTEST_MAIN(RealToolExecutorToolsTest)
#include "test_real_tool_executor_tools.moc"
