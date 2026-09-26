// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/ProcessSandbox.h"

#include <QDateTime>
#include <QHash>
#include <QObject>
#include <QProcessEnvironment>
#include <QStringList>
#include <functional>
#include <optional>

namespace sentinel::core {

enum class ProcessState { Created, Starting, Running, Stopping, Exited, Failed, Cancelled };
enum class ProcessStream { Stdout, Stderr };

struct ProcessRequest {
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment;
    int timeoutMs = 30000;
    QString sessionId;
    QString toolCallId;
    std::optional<SandboxExecutionPlan> sandbox;
    bool unconfinedPermitted = false;
};

struct ProcessRecord {
    QString processId;
    qint64 systemPid = 0;
    QString program;
    QString workingDirectory;
    QString sessionId;
    QString toolCallId;
    QDateTime startedAt;
    ProcessState state = ProcessState::Created;
    int exitCode = -1;
    bool timedOut = false;
    QString error;
    SandboxExecutionResult sandbox;
};

// All methods and callbacks run on the owning Qt event-loop thread. Callers on
// other threads must use queued invocation; QProcess never crosses threads.
class ProcessExecutor final : public QObject {
    Q_OBJECT
public:
    using StateCallback = std::function<void(const ProcessRecord&)>;
    using OutputCallback = std::function<void(const QString&, ProcessStream, const QByteArray&)>;

    explicit ProcessExecutor(QObject* parent = nullptr);
    ~ProcessExecutor() override;
    QString start(const ProcessRequest& request, StateCallback state, OutputCallback output = {});
    bool write(const QString& processId, const QByteArray& bytes);
    bool closeWriteChannel(const QString& processId);
    bool terminate(const QString& processId);
    bool kill(const QString& processId);
    ProcessRecord record(const QString& processId) const;
    QList<ProcessRecord> records() const;
    void shutdown();

private:
    struct Entry;
    void finish(Entry* entry, ProcessState state, const QString& error = {});
    void stop(Entry* entry, bool immediate, bool timeout = false);
    QHash<QString, Entry*> entries_;
    QStringList order_;
    bool shuttingDown_ = false;
    PlatformProcessSandbox sandbox_;
};

} // namespace sentinel::core
