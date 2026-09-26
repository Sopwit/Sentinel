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
#if defined(Q_OS_WIN)
#include "WindowsProcessSandbox.h"
#include <vector>
#endif
#if defined(Q_OS_UNIX)
#include <signal.h>
#include <unistd.h>
#endif
#if defined(Q_OS_LINUX)
#include <sys/prctl.h>
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
#if defined(Q_OS_WIN)
    std::shared_ptr<WindowsSandboxState> windows;
    HANDLE nativeProcess = nullptr;
    HANDLE nativeStdin = nullptr;
    HANDLE nativeStdout = nullptr;
    HANDLE nativeStderr = nullptr;
    QTimer* nativePoll = nullptr;
#endif
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
#if defined(Q_OS_WIN)
QString windowsQuote(const QString& value) {
    if (!value.isEmpty() && !value.contains(QLatin1Char(' ')) &&
        !value.contains(QLatin1Char('\t')) && !value.contains(QLatin1Char('"')))
        return value;
    QString result = QStringLiteral("\"");
    int slashes = 0;
    for (const auto ch : value) {
        if (ch == QLatin1Char('\\')) {
            ++slashes;
        } else if (ch == QLatin1Char('"')) {
            result += QString(slashes * 2 + 1, QLatin1Char('\\'));
            result += ch;
            slashes = 0;
        } else {
            result += QString(slashes, QLatin1Char('\\'));
            result += ch;
            slashes = 0;
        }
    }
    result += QString(slashes * 2, QLatin1Char('\\')) + QLatin1Char('"');
    return result;
}
#endif
} // namespace

#if defined(Q_OS_WIN)
bool ProcessExecutor::startWindows(Entry* entry, const SandboxLaunch& launch,
                                   const QString& workingDirectory) {
    auto fail = [this, entry](const QString& category) {
        entry->record.sandbox.enforcement = SandboxEnforcement::Failed;
        entry->record.sandbox.failureCategory = category;
        entry->record.sandbox.processTreeControlled = false;
        entry->record.sandbox.mitigationsApplied = false;
        finish(entry, ProcessState::Failed, QStringLiteral("Windows sandbox launch failed: %1.")
                                             .arg(category));
        return false;
    };
    SECURITY_ATTRIBUTES inheritable{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    HANDLE childStdin = nullptr, childStdout = nullptr, childStderr = nullptr;
    auto closeChildren = [&] {
        if (childStdin) CloseHandle(childStdin);
        if (childStdout) CloseHandle(childStdout);
        if (childStderr) CloseHandle(childStderr);
    };
    if (!CreatePipe(&childStdin, &entry->nativeStdin, &inheritable, 0) ||
        !CreatePipe(&entry->nativeStdout, &childStdout, &inheritable, 0) ||
        !CreatePipe(&entry->nativeStderr, &childStderr, &inheritable, 0)) {
        closeChildren();
        return fail(QStringLiteral("PipeCreationFailed"));
    }
    if (!SetHandleInformation(entry->nativeStdin, HANDLE_FLAG_INHERIT, 0) ||
        !SetHandleInformation(entry->nativeStdout, HANDLE_FLAG_INHERIT, 0) ||
        !SetHandleInformation(entry->nativeStderr, HANDLE_FLAG_INHERIT, 0)) {
        closeChildren();
        return fail(QStringLiteral("PipeIsolationFailed"));
    }
    SIZE_T attributeBytes = 0;
    InitializeProcThreadAttributeList(nullptr, 2, 0, &attributeBytes);
    std::vector<BYTE> attributeStorage(attributeBytes);
    auto* attributes = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(attributeStorage.data());
    if (!InitializeProcThreadAttributeList(attributes, 2, 0, &attributeBytes)) {
        closeChildren();
        return fail(QStringLiteral("LaunchAttributesUnavailable"));
    }
    HANDLE childHandles[] = {childStdin, childStdout, childStderr};
    if (!UpdateProcThreadAttribute(attributes, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                   childHandles, sizeof(childHandles), nullptr, nullptr)) {
        DeleteProcThreadAttributeList(attributes);
        closeChildren();
        return fail(QStringLiteral("HandleIsolationUnavailable"));
    }
    DWORD64 mitigations = PROCESS_CREATION_MITIGATION_POLICY_DEP_ENABLE |
                          PROCESS_CREATION_MITIGATION_POLICY_EXTENSION_POINT_DISABLE_ALWAYS_ON;
    const bool mitigated = UpdateProcThreadAttribute(
        attributes, 0, PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY, &mitigations,
        sizeof(mitigations), nullptr, nullptr);
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    startup.StartupInfo.hStdInput = childStdin;
    startup.StartupInfo.hStdOutput = childStdout;
    startup.StartupInfo.hStdError = childStderr;
    startup.lpAttributeList = attributes;
    QStringList commandParts{windowsQuote(launch.program)};
    for (const auto& argument : launch.arguments) commandParts.append(windowsQuote(argument));
    auto command = commandParts.join(QLatin1Char(' ')).toStdWString();
    std::vector<wchar_t> mutableCommand(command.begin(), command.end());
    mutableCommand.push_back(L'\0');
    auto keys = launch.environment.keys();
    keys.sort(Qt::CaseInsensitive);
    std::wstring environmentBlock;
    for (const auto& key : keys) {
        const auto pair = (key + QLatin1Char('=') + launch.environment.value(key)).toStdWString();
        environmentBlock.append(pair);
        environmentBlock.push_back(L'\0');
    }
    environmentBlock.push_back(L'\0');
    if (keys.isEmpty()) environmentBlock.push_back(L'\0');
    const auto executable = launch.program.toStdWString();
    const auto directory = workingDirectory.toStdWString();
    PROCESS_INFORMATION child{};
    const BOOL created = CreateProcessAsUserW(
        launch.windows->token, executable.c_str(), mutableCommand.data(), nullptr, nullptr,
        TRUE, CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT | EXTENDED_STARTUPINFO_PRESENT,
        environmentBlock.data(), directory.c_str(), &startup.StartupInfo, &child);
    DeleteProcThreadAttributeList(attributes);
    closeChildren();
    if (!created) return fail(QStringLiteral("RestrictedLaunchFailed"));
    if (!AssignProcessToJobObject(launch.windows->job, child.hProcess)) {
        TerminateProcess(child.hProcess, 1);
        CloseHandle(child.hThread);
        CloseHandle(child.hProcess);
        return fail(QStringLiteral("JobAssignmentFailed"));
    }
    if (ResumeThread(child.hThread) == DWORD(-1)) {
        TerminateJobObject(launch.windows->job, 1);
        CloseHandle(child.hThread);
        CloseHandle(child.hProcess);
        return fail(QStringLiteral("ResumeFailed"));
    }
    CloseHandle(child.hThread);
    entry->nativeProcess = child.hProcess;
    entry->record.systemPid = child.dwProcessId;
    entry->record.sandbox.mitigationsApplied = mitigated;
    entry->record.state = ProcessState::Running;
    if (entry->state) entry->state(entry->record);
    entry->process->deleteLater();
    entry->process = nullptr;
    entry->nativePoll = new QTimer(this);
    connect(entry->nativePoll, &QTimer::timeout, this, [this, entry] { pollWindows(entry); });
    entry->nativePoll->start(20);
    return true;
}

void ProcessExecutor::pollWindows(Entry* entry) {
    if (entry->terminal) return;
    auto drain = [entry](HANDLE pipe, ProcessStream stream) {
        if (!pipe) return;
        DWORD available = 0;
        while (PeekNamedPipe(pipe, nullptr, 0, nullptr, &available, nullptr) && available) {
            char bytes[16384];
            DWORD read = 0;
            if (!ReadFile(pipe, bytes, std::min<DWORD>(available, sizeof(bytes)), &read,
                          nullptr) || !read) break;
            if (!entry->cancelling)
                emitOutput(entry->output, entry->record.processId, stream,
                           QByteArray(bytes, qsizetype(read)));
        }
    };
    drain(entry->nativeStdout, ProcessStream::Stdout);
    drain(entry->nativeStderr, ProcessStream::Stderr);
    if (WaitForSingleObject(entry->nativeProcess, 0) != WAIT_OBJECT_0) return;
    if (entry->windows && entry->windows->job)
        TerminateJobObject(entry->windows->job, 1);
    drain(entry->nativeStdout, ProcessStream::Stdout);
    drain(entry->nativeStderr, ProcessStream::Stderr);
    DWORD code = 1;
    GetExitCodeProcess(entry->nativeProcess, &code);
    entry->record.exitCode = int(code);
    finish(entry, entry->record.timedOut ? ProcessState::Failed
                  : entry->cancelling ? ProcessState::Cancelled
                  : ProcessState::Exited);
}
#endif

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
#if defined(Q_OS_WIN)
        constexpr auto pathCase = Qt::CaseInsensitive;
#else
        constexpr auto pathCase = Qt::CaseSensitive;
#endif
        if (allowedRoot.isEmpty() || requestedDirectory.isEmpty() ||
            (requestedDirectory.compare(allowedRoot, pathCase) != 0 &&
             !requestedDirectory.startsWith(allowedRoot + QLatin1Char('/'), pathCase))) {
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
        if (plan.temporaryDirectory.isEmpty()) {
            entry->sandboxTemporaryDirectory = std::make_unique<QTemporaryDir>();
            if (entry->sandboxTemporaryDirectory->isValid())
                plan.temporaryDirectory = entry->sandboxTemporaryDirectory->path();
        }
        const auto paths = environment.value(QStringLiteral("PATH"))
                               .split(QDir::listSeparator(), Qt::SkipEmptyParts);
        const auto resolved = QStandardPaths::findExecutable(program, paths);
        const auto launch = sandbox_.prepare(plan, resolved, arguments, environment);
        entry->record.sandbox = launch.result;
        if (entry->sandboxTemporaryDirectory &&
            !entry->sandboxTemporaryDirectory->isValid()) {
            entry->record.sandbox.enforcement = SandboxEnforcement::Failed;
            entry->record.sandbox.failureCategory = QStringLiteral("TemporaryDirectoryUnavailable");
        }
#if defined(Q_OS_WIN)
        if (!launch.permitted || !launch.windows ||
            (entry->sandboxTemporaryDirectory &&
             !entry->sandboxTemporaryDirectory->isValid())) {
            const QString id = entry->record.processId;
            entries_.insert(id, entry);
            order_.append(id);
            finish(entry, ProcessState::Failed,
                   QStringLiteral("Sandbox unavailable: %1.")
                       .arg(entry->record.sandbox.failureCategory));
            return id;
        }
#endif
        if ((!launch.permitted ||
             (entry->sandboxTemporaryDirectory &&
              !entry->sandboxTemporaryDirectory->isValid())) &&
            request.sandbox->requireEnforcement) {
            const QString id = entry->record.processId;
            entries_.insert(id, entry);
            order_.append(id);
            finish(entry, ProcessState::Failed,
                   QStringLiteral("Sandbox unavailable: %1.")
                       .arg(entry->record.sandbox.failureCategory));
            return id;
        }
        if (launch.permitted &&
            (!entry->sandboxTemporaryDirectory || entry->sandboxTemporaryDirectory->isValid())) {
            program = launch.program;
            arguments = launch.arguments;
        }
        environment = launch.environment;
#if defined(Q_OS_WIN)
        entry->windows = launch.windows;
#endif
    }
#if defined(Q_OS_WIN)
    if (request.sandbox && entry->windows) {
        const QString id = entry->record.processId;
        entries_.insert(id, entry);
        order_.append(id);
        entry->record.state = ProcessState::Starting;
        if (entry->state) entry->state(entry->record);
        SandboxLaunch nativeLaunch;
        nativeLaunch.program = program;
        nativeLaunch.arguments = arguments;
        nativeLaunch.environment = environment;
        nativeLaunch.windows = entry->windows;
        if (!startWindows(entry, nativeLaunch, entry->process->workingDirectory()))
            return id;
        connect(entry->timeout, &QTimer::timeout, this,
                [this, entry] { stop(entry, false, true); });
        if (!entry->terminal && request.timeoutMs > 0)
            entry->timeout->start(request.timeoutMs);
        return id;
    }
#endif
    entry->process->setProcessEnvironment(environment);
#if defined(Q_OS_UNIX)
    if (request.sandbox && entry->record.sandbox.enforcement == SandboxEnforcement::Enforced)
        entry->process->setChildProcessModifier([] {
            if (setsid() < 0) _exit(127);
#if defined(Q_OS_LINUX)
            if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) != 0) _exit(127);
#endif
        });
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
#if defined(Q_OS_WIN)
    if (entry && entry->windows) {
        DWORD written = 0;
        return !entry->terminal && entry->nativeStdin &&
            WriteFile(entry->nativeStdin, bytes.constData(), DWORD(bytes.size()), &written,
                      nullptr) && written == DWORD(bytes.size());
    }
#endif
    return entry && !entry->terminal && entry->process->write(bytes) == bytes.size();
}

bool ProcessExecutor::closeWriteChannel(const QString& id) {
    Q_ASSERT(thread() == QThread::currentThread());
    auto* entry = entries_.value(id, nullptr);
    if (!entry || entry->terminal)
        return false;
#if defined(Q_OS_WIN)
    if (entry->windows) {
        if (entry->nativeStdin) CloseHandle(entry->nativeStdin);
        entry->nativeStdin = nullptr;
        return true;
    }
#endif
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
#if defined(Q_OS_WIN)
    if (entry->windows) {
        if (entry->nativeStdin) CloseHandle(entry->nativeStdin);
        entry->nativeStdin = nullptr;
        if (entry->windows->job) TerminateJobObject(entry->windows->job, 1);
        return;
    }
#endif
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
#if defined(Q_OS_WIN)
    if (entry->nativePoll) {
        entry->nativePoll->stop();
        entry->nativePoll->deleteLater();
        entry->nativePoll = nullptr;
    }
    if (entry->nativeStdin) CloseHandle(entry->nativeStdin);
    if (entry->nativeStdout) CloseHandle(entry->nativeStdout);
    if (entry->nativeStderr) CloseHandle(entry->nativeStderr);
    if (entry->nativeProcess) CloseHandle(entry->nativeProcess);
    entry->nativeStdin = entry->nativeStdout = entry->nativeStderr = entry->nativeProcess = nullptr;
    entry->windows.reset();
#endif
    entry->sandboxTemporaryDirectory.reset();
    if (entry->process) entry->process->deleteLater();
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
#if defined(Q_OS_WIN)
            if (entry->windows && entry->windows->job)
                TerminateJobObject(entry->windows->job, 1);
            if (entry->nativePoll) delete entry->nativePoll;
            if (entry->nativeStdin) CloseHandle(entry->nativeStdin);
            if (entry->nativeStdout) CloseHandle(entry->nativeStdout);
            if (entry->nativeStderr) CloseHandle(entry->nativeStderr);
            if (entry->nativeProcess) CloseHandle(entry->nativeProcess);
            entry->windows.reset();
#endif
            if (entry->process) entry->process->kill();
            // QProcess destruction reaps an active child if the event loop is stopping.
            delete entry->process;
            entry->process = nullptr;
            entry->record.state = ProcessState::Cancelled;
            entry->terminal = true;
        }
    }
}

} // namespace sentinel::core
