// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/ProcessExecutor.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace sentinel::core;

class ProcessExecutorTest : public QObject {
    Q_OBJECT
signals:
    void completed();
private slots:
    void outputAndExit();
    void failedStart();
    void timeoutAndCancellation();
    void concurrentOwnership();
    void cancellationAndShutdown();
    void macHostContainmentPreventsChildren();
    void macHostContainmentRestrictsFilesystem();
};

void ProcessExecutorTest::macHostContainmentRestrictsFilesystem() {
#ifndef Q_OS_MACOS
    QSKIP("macOS containment probe.");
#else
    QTemporaryDir authorized, external;
    const auto allowed = authorized.filePath("allowed.txt");
    const auto denied = external.filePath("denied.txt");
    for (const auto& path : {allowed, denied}) {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("CONTAINMENT_OK");
    }
    for (const auto& path : {allowed, denied}) {
        ProcessExecutor executor;
        ProcessRequest request;
        request.program = "/bin/cat";
        request.arguments = {path};
        request.workingDirectory = authorized.path();
        SandboxExecutionPlan plan;
        plan.workingDirectory = authorized.path();
        plan.readablePaths = {QFileInfo(authorized.path()).canonicalFilePath()};
        plan.forbidDetachedChildren = true;
        request.sandbox = plan;
        QByteArray out, error;
        const auto id = executor.start(
            request, {}, [&](const QString&, ProcessStream stream, const QByteArray& bytes) {
                (stream == ProcessStream::Stdout ? out : error) += bytes;
            });
        QTRY_VERIFY_WITH_TIMEOUT(executor.record(id).state == ProcessState::Exited ||
                                     executor.record(id).state == ProcessState::Failed,
                                 5000);
        QCOMPARE(executor.record(id).sandbox.enforcement, SandboxEnforcement::Enforced);
        if (path == allowed) {
            QCOMPARE(executor.record(id).exitCode, 0);
            QCOMPARE(out, QByteArray("CONTAINMENT_OK"));
        } else {
            QVERIFY(executor.record(id).exitCode != 0);
            QVERIFY(out.isEmpty());
            QVERIFY(error.contains("Operation not permitted"));
        }
    }
#endif
}

void ProcessExecutorTest::macHostContainmentPreventsChildren() {
#ifndef Q_OS_MACOS
    QSKIP("macOS containment probe.");
#else
    QTemporaryDir directory;
    ProcessExecutor executor;
    ProcessRequest request;
    request.program = "/bin/sh";
    request.arguments = {"-c", "/bin/sleep 1 & wait"};
    request.workingDirectory = directory.path();
    SandboxExecutionPlan plan;
    plan.workingDirectory = directory.path();
    plan.writablePaths = {directory.path()};
    plan.forbidDetachedChildren = true;
    request.sandbox = plan;
    QByteArray error;
    const auto id = executor.start(
        request, {}, [&](const QString&, ProcessStream stream, const QByteArray& bytes) {
            if (stream == ProcessStream::Stderr)
                error += bytes;
        });
    QTRY_VERIFY_WITH_TIMEOUT(executor.record(id).state == ProcessState::Exited ||
                                 executor.record(id).state == ProcessState::Failed,
                             5000);
    const auto record = executor.record(id);
    QCOMPARE(record.sandbox.enforcement, SandboxEnforcement::Enforced);
    QVERIFY(record.sandbox.processTreeControlled);
    QVERIFY(record.exitCode != 0);
    QVERIFY(error.contains("Operation not permitted"));
#endif
}

void ProcessExecutorTest::outputAndExit() {
#ifdef Q_OS_WIN
    QSKIP("The shell fixture is Unix-only.");
#endif
    ProcessExecutor executor;
    QByteArray out, err;
    QStringList transitions;
    ProcessRequest request;
    request.unconfinedPermitted = true;
    request.program = QStringLiteral("/bin/sh");
    request.arguments = {QStringLiteral("-c"),
                         QStringLiteral("printf 'line1\nline2\n'; printf 'problem\n' >&2; exit 7")};
    QString id;
    id = executor.start(
        request,
        [&](const ProcessRecord& record) {
            transitions.append(QString::number(static_cast<int>(record.state)));
            if (record.state == ProcessState::Exited)
                emit completed();
        },
        [&](const QString& processId, ProcessStream stream, const QByteArray& bytes) {
            QCOMPARE(processId, id);
            (stream == ProcessStream::Stdout ? out : err).append(bytes);
        });
    QSignalSpy spy(this, &ProcessExecutorTest::completed);
    QVERIFY(spy.wait(5000));
    QCOMPARE(out, QByteArray("line1\nline2\n"));
    QCOMPARE(err, QByteArray("problem\n"));
    QCOMPARE(executor.record(id).exitCode, 7);
    QCOMPARE(executor.record(id).state, ProcessState::Exited);
    QCOMPARE(transitions.last(), QString::number(static_cast<int>(ProcessState::Exited)));
}

void ProcessExecutorTest::failedStart() {
    ProcessExecutor executor;
    ProcessRequest request;
    request.unconfinedPermitted = true;
    request.program = QStringLiteral("/nonexistent/sentinel-process-test");
    const auto id = executor.start(request, [&](const ProcessRecord& record) {
        if (record.state == ProcessState::Failed)
            emit completed();
    });
    QSignalSpy spy(this, &ProcessExecutorTest::completed);
    QVERIFY(spy.wait(5000));
    QCOMPARE(executor.record(id).state, ProcessState::Failed);
}

void ProcessExecutorTest::timeoutAndCancellation() {
#ifdef Q_OS_WIN
    QSKIP("The shell fixture is Unix-only.");
#endif
    ProcessExecutor executor;
    ProcessRequest request;
    request.unconfinedPermitted = true;
    request.program = QStringLiteral("/bin/sh");
    request.arguments = {QStringLiteral("-c"), QStringLiteral("sleep 30")};
    request.timeoutMs = 50;
    const auto id = executor.start(request, [&](const ProcessRecord& record) {
        if (record.state == ProcessState::Failed)
            emit completed();
    });
    QSignalSpy spy(this, &ProcessExecutorTest::completed);
    QVERIFY(spy.wait(3000));
    QVERIFY(executor.record(id).timedOut);
    QCOMPARE(executor.record(id).state, ProcessState::Failed);
    QVERIFY(!executor.terminate(id));
}

void ProcessExecutorTest::concurrentOwnership() {
#ifdef Q_OS_WIN
    QSKIP("The shell fixture is Unix-only.");
#endif
    ProcessExecutor executor;
    int completedCount = 0;
    QStringList ids;
    for (const auto& session : {QStringLiteral("one"), QStringLiteral("two")}) {
        ProcessRequest request;
        request.unconfinedPermitted = true;
        request.program = QStringLiteral("/bin/sh");
        request.arguments = {QStringLiteral("-c"), QStringLiteral("printf ready")};
        request.sessionId = session;
        ids.append(executor.start(request, [&](const ProcessRecord& record) {
            if (record.state == ProcessState::Exited && ++completedCount == 2)
                emit completed();
        }));
    }
    QSignalSpy spy(this, &ProcessExecutorTest::completed);
    QVERIFY(spy.wait(5000));
    QVERIFY(ids[0] != ids[1]);
    QCOMPARE(executor.record(ids[0]).sessionId, QStringLiteral("one"));
    QCOMPARE(executor.record(ids[1]).sessionId, QStringLiteral("two"));
}

void ProcessExecutorTest::cancellationAndShutdown() {
#ifdef Q_OS_WIN
    QSKIP("The shell fixture is Unix-only.");
#endif
    ProcessExecutor executor;
    ProcessRequest request;
    request.unconfinedPermitted = true;
    request.program = QStringLiteral("/bin/sh");
    request.arguments = {QStringLiteral("-c"), QStringLiteral("exec sleep 30")};
    request.timeoutMs = 60000;
    int terminalCount = 0;
    const auto id = executor.start(request, [&](const ProcessRecord& record) {
        if (record.state == ProcessState::Cancelled)
            ++terminalCount;
    });
    QTRY_COMPARE_WITH_TIMEOUT(executor.record(id).state, ProcessState::Running, 3000);
    QVERIFY(executor.terminate(id));
    QTRY_COMPARE_WITH_TIMEOUT(executor.record(id).state, ProcessState::Cancelled, 3000);
    QCOMPARE(terminalCount, 1);
    QVERIFY(!executor.terminate(id));
    const auto second = executor.start(request, {});
    QTRY_COMPARE_WITH_TIMEOUT(executor.record(second).state, ProcessState::Running, 3000);
    executor.shutdown();
    QCOMPARE(executor.record(second).state, ProcessState::Cancelled);
}

QTEST_MAIN(ProcessExecutorTest)
#include "test_process_executor.moc"
