// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/ProcessExecutor.h"

#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QUuid>
#include <algorithm>
#include <memory>
#if defined(Q_OS_UNIX)
#include <signal.h>
#include <unistd.h>
#endif

namespace sentinel::core {

struct ProcessExecutor::Entry {
    ProcessRecord record;
    QProcess* process = nullptr;
    QTimer* timeout = nullptr;
    QTimer* grace = nullptr;
    StateCallback state;
    OutputCallback output;
    bool terminal = false;
    bool cancelling = false;
    std::unique_ptr<QTemporaryDir> sandboxTemporaryDirectory;
};

namespace {
void emitOutput(const ProcessExecutor::OutputCallback& callback, const QString& id,
                ProcessStream stream, const QByteArray& bytes) {
    if (!callback)
        return;
    constexpr qsizetype kChunkBytes = 16384;
    for (qsizetype offset = 0; offset < bytes.size(); offset += kChunkBytes)
        callback(id, stream, bytes.mid(offset, kChunkBytes));
}
} // namespace

ProcessExecutor::ProcessExecutor(QObject* parent) : QObject(parent) {}

ProcessExecutor::~ProcessExecutor() {
    shutdown();
    qDeleteAll(entries_);
}

QString ProcessExecutor::start(const ProcessRequest& request, StateCallback state,
                               OutputCallback output) {
    Q_ASSERT(thread() == QThread::currentThread());
    if (shuttingDown_ || request.program.isEmpty())
        return {};
    auto* entry = new Entry;
    entry->record.processId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    entry->record.program = request.program;
    entry->record.workingDirectory = request.workingDirectory;
    entry->record.sessionId = request.sessionId;
    entry->record.toolCallId = request.toolCallId;
    entry->record.startedAt = QDateTime::currentDateTimeUtc();
    entry->state = std::move(state);
    entry->output = std::move(output);
    entry->process = new QProcess(this);
    entry->timeout = new QTimer(this);
    entry->grace = new QTimer(this);
    entry->timeout->setSingleShot(true);
    entry->grace->setSingleShot(true);
    if (!request.sandbox && !request.unconfinedPermitted) {
        const QString id = entry->record.processId;
        entry->record.sandbox.enforcement = SandboxEnforcement::Failed;
        entry->record.sandbox.backend = QStringLiteral("policy");
        entry->record.sandbox.failureCategory = QStringLiteral("SandboxPlanMissing");
        entries_.insert(id, entry);
        order_.append(id);
        finish(entry, ProcessState::Failed,
               QStringLiteral("Process sandbox plan is missing."));
        return id;
    }
    if (!request.sandbox) {
        entry->record.sandbox.backend = QStringLiteral("none");
        entry->record.sandbox.failureCategory = QStringLiteral("ExplicitUnconfined");
    }
    entry->process->setWorkingDirectory(request.workingDirectory);
    QString program = request.program;
    QStringList arguments = request.arguments;
    QProcessEnvironment environment = request.environment.isEmpty()
        ? QProcessEnvironment::systemEnvironment() : request.environment;
    if (request.sandbox) {
        SandboxExecutionPlan plan = *request.sandbox;
        const auto allowedRoot = QFileInfo(plan.workingDirectory).canonicalFilePath();
        const auto requestedDirectory = request.workingDirectory.isEmpty()
            ? allowedRoot : QFileInfo(request.workingDirectory).canonicalFilePath();
        if (allowedRoot.isEmpty() || requestedDirectory.isEmpty() ||
            (requestedDirectory != allowedRoot &&
             !requestedDirectory.startsWith(allowedRoot + QLatin1Char('/')))) {
            const QString id = entry->record.processId;
            entry->record.sandbox.enforcement = SandboxEnforcement::Failed;
            entry->record.sandbox.backend = QStringLiteral("policy");
            entry->record.sandbox.failureCategory = QStringLiteral("WorkingDirectoryOutsidePlan");
            entries_.insert(id, entry);
            order_.append(id);
            finish(entry, ProcessState::Failed,
                   QStringLiteral("Process working directory is outside its sandbox plan."));
            return id;
        }
        entry->process->setWorkingDirectory(requestedDirectory);
        plan.workingDirectory = requestedDirectory;
        entry->sandboxTemporaryDirectory = std::make_unique<QTemporaryDir>();
        if (entry->sandboxTemporaryDirectory->isValid())
            plan.temporaryDirectory = entry->sandboxTemporaryDirectory->path();
        const auto paths = environment.value(QStringLiteral("PATH"))
                               .split(QDir::listSeparator(), Qt::SkipEmptyParts);
        const auto resolved = QStandardPaths::findExecutable(program, paths);
        const auto launch = sandbox_.prepare(plan, resolved, arguments, environment);
        entry->record.sandbox = launch.result;
        if (!entry->sandboxTemporaryDirectory->isValid()) {
            entry->record.sandbox.enforcement = SandboxEnforcement::Failed;
            entry->record.sandbox.failureCategory = QStringLiteral("TemporaryDirectoryUnavailable");
        }
        if ((!launch.permitted || !entry->sandboxTemporaryDirectory->isValid()) &&
            request.sandbox->requireEnforcement) {
            const QString id = entry->record.processId;
            entries_.insert(id, entry);
            order_.append(id);
            finish(entry, ProcessState::Failed,
                   QStringLiteral("Sandbox unavailable: %1.")
                       .arg(entry->record.sandbox.failureCategory));
            return id;
        }
        if (launch.permitted && entry->sandboxTemporaryDirectory->isValid()) {
            program = launch.program;
            arguments = launch.arguments;
        }
        environment = launch.environment;
    }
    entry->process->setProcessEnvironment(environment);
#if defined(Q_OS_UNIX)
    if (request.sandbox && entry->record.sandbox.enforcement == SandboxEnforcement::Enforced)
        entry->process->setChildProcessModifier([] { setsid(); });
#endif
    const QString id = entry->record.processId;
    entries_.insert(id, entry);
    order_.append(id);
    connect(entry->process, &QProcess::started, this, [this, entry] {
        if (entry->terminal)
            return;
        entry->record.systemPid = static_cast<qint64>(entry->process->processId());
        entry->record.state = ProcessState::Running;
        if (entry->state)
            entry->state(entry->record);
    });
    connect(entry->process, &QProcess::readyReadStandardOutput, this, [entry] {
        const auto bytes = entry->process->readAllStandardOutput();
        if (!entry->terminal && !entry->cancelling && !bytes.isEmpty())
            emitOutput(entry->output, entry->record.processId, ProcessStream::Stdout, bytes);
    });
    connect(entry->process, &QProcess::readyReadStandardError, this, [entry] {
        const auto bytes = entry->process->readAllStandardError();
        if (!entry->terminal && !entry->cancelling && !bytes.isEmpty())
            emitOutput(entry->output, entry->record.processId, ProcessStream::Stderr, bytes);
    });
    connect(entry->process, &QProcess::errorOccurred, this,
            [this, entry](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart)
                    finish(entry, ProcessState::Failed, entry->process->errorString());
            });
    connect(
        entry->process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
        [this, entry](int code, QProcess::ExitStatus status) {
            if (entry->terminal)
                return;
            // Drain the final bytes before the terminal state.
            const auto out = entry->process->readAllStandardOutput();
            const auto err = entry->process->readAllStandardError();
            if (!entry->cancelling && entry->output) {
                if (!out.isEmpty())
                    emitOutput(entry->output, entry->record.processId, ProcessStream::Stdout, out);
                if (!err.isEmpty())
                    emitOutput(entry->output, entry->record.processId, ProcessStream::Stderr, err);
            }
            entry->record.exitCode = code;
            finish(entry,
                   entry->record.timedOut          ? ProcessState::Failed
                   : entry->cancelling             ? ProcessState::Cancelled
                   : status == QProcess::CrashExit ? ProcessState::Failed
                                                   : ProcessState::Exited,
                   status == QProcess::CrashExit && !entry->cancelling
                       ? QStringLiteral("Process crashed.")
                       : QString());
        });
    connect(entry->timeout, &QTimer::timeout, this, [this, entry] { stop(entry, false, true); });
    connect(entry->grace, &QTimer::timeout, this, [entry] {
        if (!entry->terminal && entry->process->state() != QProcess::NotRunning) {
#if defined(Q_OS_UNIX)
            if (entry->record.sandbox.enforcement == SandboxEnforcement::Enforced &&
                entry->record.systemPid > 0)
                ::kill(-static_cast<pid_t>(entry->record.systemPid), SIGKILL);
#endif
            entry->process->kill();
        }
    });
    entry->record.state = ProcessState::Starting;
    if (entry->state)
        entry->state(entry->record);
    entry->process->start(program, arguments);
    if (!entry->terminal && request.timeoutMs > 0)
        entry->timeout->start(request.timeoutMs);
    return id;
}

bool ProcessExecutor::write(const QString& id, const QByteArray& bytes) {
    Q_ASSERT(thread() == QThread::currentThread());
    auto* entry = entries_.value(id, nullptr);
    return entry && !entry->terminal && entry->process->write(bytes) == bytes.size();
}

bool ProcessExecutor::closeWriteChannel(const QString& id) {
    Q_ASSERT(thread() == QThread::currentThread());
    auto* entry = entries_.value(id, nullptr);
    if (!entry || entry->terminal)
        return false;
    entry->process->closeWriteChannel();
    return true;
}

void ProcessExecutor::stop(Entry* entry, bool immediate, bool timeout) {
    if (!entry || entry->terminal || entry->cancelling)
        return;
    entry->cancelling = true;
    entry->record.timedOut = timeout;
    entry->record.state = ProcessState::Stopping;
    if (entry->state)
        entry->state(entry->record);
    if (immediate)
    {
#if defined(Q_OS_UNIX)
        if (entry->record.sandbox.enforcement == SandboxEnforcement::Enforced &&
            entry->record.systemPid > 0)
            ::kill(-static_cast<pid_t>(entry->record.systemPid), SIGKILL);
#endif
        entry->process->kill();
    } else {
#if defined(Q_OS_UNIX)
        if (entry->record.sandbox.enforcement == SandboxEnforcement::Enforced &&
            entry->record.systemPid > 0)
            ::kill(-static_cast<pid_t>(entry->record.systemPid), SIGTERM);
#endif
        entry->process->terminate();
        entry->grace->start(1000);
    }
    if (entry->process->state() == QProcess::NotRunning)
        finish(entry, timeout ? ProcessState::Failed : ProcessState::Cancelled,
               timeout ? QStringLiteral("Process timed out.") : QString());
}

bool ProcessExecutor::terminate(const QString& id) {
    Q_ASSERT(thread() == QThread::currentThread());
    auto* entry = entries_.value(id, nullptr);
    if (!entry || entry->terminal)
        return false;
    stop(entry, false);
    return true;
}

bool ProcessExecutor::kill(const QString& id) {
    Q_ASSERT(thread() == QThread::currentThread());
    auto* entry = entries_.value(id, nullptr);
    if (!entry || entry->terminal)
        return false;
    stop(entry, true);
    return true;
}

void ProcessExecutor::finish(Entry* entry, ProcessState state, const QString& error) {
    if (entry->terminal)
        return;
    entry->terminal = true;
    entry->timeout->stop();
    entry->grace->stop();
    entry->record.state = state;
    entry->record.error = entry->record.timedOut ? QStringLiteral("Process timed out.") : error;
    entry->sandboxTemporaryDirectory.reset();
    entry->process->deleteLater();
    entry->process = nullptr;
    entry->timeout->deleteLater();
    entry->grace->deleteLater();
    if (entry->state)
        entry->state(entry->record);
    // Keep a bounded set of terminal records, but never evict active processes.
    while (order_.size() > 128) {
        auto it = std::find_if(order_.cbegin(), order_.cend(),
                               [this](const QString& id) { return entries_.value(id)->terminal; });
        if (it == order_.cend())
            break;
        const QString oldest = *it;
        order_.removeAll(oldest);
        delete entries_.take(oldest);
    }
}

ProcessRecord ProcessExecutor::record(const QString& id) const {
    Q_ASSERT(thread() == QThread::currentThread());
    auto* entry = entries_.value(id, nullptr);
    return entry ? entry->record : ProcessRecord{};
}

QList<ProcessRecord> ProcessExecutor::records() const {
    Q_ASSERT(thread() == QThread::currentThread());
    QList<ProcessRecord> result;
    for (const auto& id : order_)
        result.append(entries_.value(id)->record);
    return result;
}

void ProcessExecutor::shutdown() {
    Q_ASSERT(thread() == QThread::currentThread());
    shuttingDown_ = true;
    for (auto* entry : entries_) {
        if (!entry->terminal) {
            entry->cancelling = true;
            entry->timeout->stop();
            entry->grace->stop();
#if defined(Q_OS_UNIX)
            if (entry->record.sandbox.enforcement == SandboxEnforcement::Enforced &&
                entry->record.systemPid > 0)
                ::kill(-static_cast<pid_t>(entry->record.systemPid), SIGKILL);
#endif
            entry->process->kill();
            // QProcess destruction reaps an active child if the event loop is stopping.
            delete entry->process;
            entry->process = nullptr;
            entry->record.state = ProcessState::Cancelled;
            entry->terminal = true;
        }
    }
}

} // namespace sentinel::core
