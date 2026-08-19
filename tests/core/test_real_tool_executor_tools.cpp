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

using namespace sentinel::core;

namespace {

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
};

QTEST_MAIN(RealToolExecutorToolsTest)
#include "test_real_tool_executor_tools.moc"
