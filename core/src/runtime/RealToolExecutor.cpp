// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/chat/IChatHistoryStore.h"
#include "sentinel/core/editor/FuzzyEditor.h"
#include "sentinel/core/mcp/McpService.h"
#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include "sentinel/core/runtime/FileSystemPatch.h"
#include "sentinel/core/runtime/ToolArgumentValidator.h"
#include "sentinel/core/security/ExternalDirectoryGate.h"
#include "sentinel/core/security/PathGuard.h"

#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QSettings>
#include <QStorageInfo>
#include <QSysInfo>
#include <QTimer>
#include <QUrl>

#include <algorithm>
#include <functional>
#include <optional>

#if defined(Q_OS_UNIX)
#include <unistd.h>
#elif defined(Q_OS_WIN)
#define NOMINMAX
#include <windows.h>
#endif

namespace sentinel::core {

RealToolExecutor::RealToolExecutor() = default;

RealToolExecutor::RealToolExecutor(std::shared_ptr<AlarmStore> alarmStore)
    : alarmStore_(std::move(alarmStore)) {}

void RealToolExecutor::setAlarmStore(std::shared_ptr<AlarmStore> alarmStore) {
    alarmStore_ = std::move(alarmStore);
}

void RealToolExecutor::configureMcpServers(const QList<McpServerConfig>& configs) {
    auto service = std::make_shared<McpService>();
    for (const auto& config : configs) {
        if (config.name.trimmed().isEmpty()) {
            continue;
        }
        service->addServer(config);
    }
    service->connectToAll();
    mcpService_ = std::move(service);
}

void RealToolExecutor::setMcpService(std::shared_ptr<IMcpService> service) {
    mcpService_ = std::move(service);
}

void RealToolExecutor::setSubagentRunner(std::function<QString(const QString& task)> runner) {
    subagentRunner_ = std::move(runner);
}

void RealToolExecutor::configureWebSearch(const QString& provider, const QString& apiKey,
                                          int maxResults) {
    webSearchTool_.setSearchProvider(provider);
    webSearchTool_.setApiKey(apiKey);
    webSearchTool_.setMaxResults(maxResults);
}

WebSearchResponse RealToolExecutor::searchWeb(const QString& query) const {
    return webSearchTool_.search(query);
}

namespace {
QString scopedPath(const QString& workingDirectory, const QString& rawPath);
QString osascriptEscape(const QString& text);
bool looksLikeDomainName(const QString& text);
} // namespace

namespace {
ToolExecutionResult validateCompatibilityRequest(ToolExecutionRequest& request) {
    if (request.plan.status != ToolInvocationPlanStatus::Planned)
        return {ToolExecutionStatus::Succeeded, {}};
    const auto descriptors = BuiltInToolProvider::descriptors();
    for (auto& invocation : request.plan.invocations) {
        const auto found =
            std::find_if(descriptors.begin(), descriptors.end(),
                         [&](const ToolDescriptor& tool) { return tool.id == invocation.toolId; });
        if (found == descriptors.end())
            return {ToolExecutionStatus::UnknownTool,
                    QStringLiteral("Unknown built-in tool: %1").arg(invocation.toolId)};
        const auto validation = ToolArgumentValidator::validate(*found, invocation.arguments);
        if (!validation.valid) {
            QStringList details;
            for (const auto& issue : validation.errors.mid(0, 8))
                details.append(
                    QStringLiteral("%1 [%2]: %3").arg(issue.path, issue.keyword, issue.message));
            return {ToolExecutionStatus::InvalidArguments, details.join(QLatin1Char('\n'))};
        }
        invocation.arguments =
            ToolArgumentValidator::toInvocationArguments(validation.normalizedArguments);
    }
    return {ToolExecutionStatus::Succeeded, {}};
}
} // namespace

IToolExecutor::Cancel RealToolExecutor::executeAsync(const ToolExecutionRequest& originalRequest,
                                                     const QString& sessionId,
                                                     const QString& toolCallId, Output output,
                                                     Completion completion) const {
    ToolExecutionRequest normalizedRequest = originalRequest;
    const auto validation = validateCompatibilityRequest(normalizedRequest);
    if (validation.status != ToolExecutionStatus::Succeeded) {
        completion(validation);
        return {};
    }
    const auto& request = normalizedRequest;
    if (request.plan.status == ToolInvocationPlanStatus::Planned &&
        request.plan.invocations.size() > 1 &&
        (request.approval.status == ApprovalStatus::Approved ||
         request.approval.status == ApprovalStatus::NotRequired) &&
        request.sandbox.status != SandboxStatus::Denied &&
        request.sandbox.status != SandboxStatus::BlockedByApproval) {
        struct Batch {
            ToolExecutionRequest request;
            Output output;
            Completion completion;
            Cancel currentCancel;
            QStringList summaries;
            std::function<void()> next;
            int index = 0;
            bool finished = false;
            bool cancelled = false;
            bool active = false;
        };
        auto batch = std::make_shared<Batch>();
        batch->request = request;
        batch->output = std::move(output);
        batch->completion = std::move(completion);
        std::weak_ptr<Batch> weak = batch;
        batch->next = [this, weak, sessionId, toolCallId] {
            auto current = weak.lock();
            if (!current || current->finished)
                return;
            if (current->cancelled) {
                current->finished = true;
                current->completion(
                    {ToolExecutionStatus::Blocked, QStringLiteral("Tool execution cancelled.")});
                current->next = {};
                return;
            }
            if (current->index >= current->request.plan.invocations.size()) {
                current->finished = true;
                current->completion({ToolExecutionStatus::Succeeded,
                                     current->summaries.join(QStringLiteral("\n\n"))});
                current->next = {};
                return;
            }
            ToolExecutionRequest one = current->request;
            one.plan.invocations = {current->request.plan.invocations.at(current->index++)};
            current->active = true;
            current->currentCancel = executeAsync(
                one, sessionId, toolCallId, current->output, [weak](ToolExecutionResult result) {
                    auto state = weak.lock();
                    if (!state || state->finished)
                        return;
                    state->active = false;
                    state->currentCancel = {};
                    if (state->cancelled) {
                        state->finished = true;
                        state->completion({ToolExecutionStatus::Blocked,
                                           QStringLiteral("Tool execution cancelled.")});
                        state->next = {};
                        return;
                    }
                    if (result.status != ToolExecutionStatus::Succeeded &&
                        result.status != ToolExecutionStatus::PlaceholderSucceeded) {
                        state->finished = true;
                        state->completion(std::move(result));
                        state->next = {};
                        return;
                    }
                    state->summaries.append(std::move(result.summary));
                    QTimer::singleShot(0, [weak] {
                        if (auto next = weak.lock(); next && next->next)
                            next->next();
                    });
                });
            if (!current->active)
                current->currentCancel = {};
        };
        batch->next();
        return [batch] {
            if (batch->finished)
                return;
            batch->cancelled = true;
            if (batch->active && batch->currentCancel) {
                batch->currentCancel();
            } else {
                batch->finished = true;
                batch->completion(
                    {ToolExecutionStatus::Blocked, QStringLiteral("Tool execution cancelled.")});
                batch->next = {};
            }
        };
    }
    if (request.plan.invocations.size() != 1 ||
        request.plan.status != ToolInvocationPlanStatus::Planned ||
        !request.knownToolIds.contains(request.plan.invocations.first().toolId) ||
        (request.approval.status != ApprovalStatus::Approved &&
         request.approval.status != ApprovalStatus::NotRequired) ||
        request.sandbox.status == SandboxStatus::Denied ||
        request.sandbox.status == SandboxStatus::BlockedByApproval) {
        return IToolExecutor::executeAsync(request, sessionId, toolCallId, std::move(output),
                                           std::move(completion));
    }

    const auto& invocation = request.plan.invocations.first();
    auto argument = [&invocation](const QString& id) {
        for (const auto& item : invocation.arguments) {
            if (item.id == id)
                return item.value;
        }
        return QString{};
    };
    const QString toolId = invocation.toolId;
    if (toolId != QLatin1String("run-command")) {
        static const QSet<QString> processTools{
            QStringLiteral("process-list"),  QStringLiteral("app-quit"),
            QStringLiteral("system-notify"), QStringLiteral("browser-screenshot"),
            QStringLiteral("browser-pdf"),   QStringLiteral("voice-transcribe"),
            QStringLiteral("voice-speak"),   QStringLiteral("app-launch")};
        if (!processTools.contains(toolId))
            return IToolExecutor::executeAsync(request, sessionId, toolCallId, std::move(output),
                                               std::move(completion));
        ProcessRequest process;
        process.sessionId = sessionId;
        process.toolCallId = toolCallId;
        process.timeoutMs = 15000;
        std::function<QString(const ProcessRecord&, const QString&, const QString&)> format;
        QByteArray input;
        if (toolId == QLatin1String("process-list")) {
#if defined(Q_OS_WIN)
            process.program = QStringLiteral("tasklist");
#else
            process.program = QStringLiteral("ps");
            process.arguments = {QStringLiteral("-eo"), QStringLiteral("pid,pcpu,comm")};
#endif
            format = [](const ProcessRecord& record, const QString& out, const QString& err) {
                if (record.timedOut || record.state == ProcessState::Failed)
                    return QStringLiteral("process-list: %1")
                        .arg(record.timedOut          ? QStringLiteral("Timed out.")
                             : record.error.isEmpty() ? err
                                                      : record.error);
                if (out.isEmpty())
                    return QStringLiteral("process-list: No process output was returned.");
                const auto lines = out.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                QStringList shown;
                for (int i = 0; i < lines.size() && i < 40; ++i)
                    shown.append(lines.at(i).left(2000).trimmed());
                return QStringLiteral("process-list: %1 process line(s) (showing first %2):\n%3")
                    .arg(lines.size())
                    .arg(shown.size())
                    .arg(shown.join(QLatin1Char('\n')));
            };
        } else if (toolId == QLatin1String("app-quit")) {
            const QString app = argument(QStringLiteral("app")).trimmed();
            if (app.isEmpty()) {
                completion({ToolExecutionStatus::Succeeded,
                            QStringLiteral("app-quit: No app argument provided.")});
                return {};
            }
#if defined(Q_OS_MACOS)
            process.program = QStringLiteral("osascript");
            process.arguments = {QStringLiteral("-e"),
                                 QStringLiteral("quit app \"%1\"").arg(osascriptEscape(app))};
#elif defined(Q_OS_WIN)
            process.program = QStringLiteral("taskkill");
            process.arguments = {QStringLiteral("/IM"),
                                 app.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)
                                     ? app
                                     : app + QStringLiteral(".exe"),
                                 QStringLiteral("/F")};
#else
            process.program = QStringLiteral("pkill");
            process.arguments = {QStringLiteral("-f"), QStringLiteral("-i"), app};
#endif
            format = [app](const ProcessRecord& record, const QString&, const QString& err) {
                const QString error = record.timedOut ? QStringLiteral("Timed out.")
                                      : record.state == ProcessState::Failed ? record.error
                                      : record.exitCode != 0                 ? err
                                                                             : QString();
                return error.isEmpty()
                           ? QStringLiteral("app-quit: Quit requested for '%1'.").arg(app)
                           : QStringLiteral("app-quit: %1 (%2)").arg(error, app);
            };
        } else if (toolId == QLatin1String("app-launch")) {
            const QString app = argument(QStringLiteral("app")).trimmed();
            if (app.isEmpty() || looksLikeDomainName(app)) {
                QString cwd = QDir::currentPath();
                completion(executeAppLaunch(invocation, cwd));
                return {};
            }
#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
            // Linux launch uses the existing nonblocking desktop-entry path.
            QString cwd = QDir::currentPath();
            completion(executeAppLaunch(invocation, cwd));
            return {};
#else
            const QString extra = argument(QStringLiteral("args")).trimmed();
#if defined(Q_OS_MACOS)
            process.program = QStringLiteral("open");
            process.arguments = {QStringLiteral("-a"), app};
            if (!extra.isEmpty())
                process.arguments.append(extra);
#else
            process.program = QStringLiteral("cmd.exe");
            process.arguments = {QStringLiteral("/c"), QStringLiteral("start"), QString(), app};
            if (!extra.isEmpty())
                process.arguments.append(extra);
#endif
            format = [app](const ProcessRecord& record, const QString&, const QString& err) {
                const QString error = record.timedOut ? QStringLiteral("Timed out.")
                                      : record.state == ProcessState::Failed ? record.error
                                      : record.exitCode != 0                 ? err
                                                                             : QString();
                return error.isEmpty()
                           ? QStringLiteral("app-launch: Launch requested for '%1'.").arg(app)
                           : QStringLiteral("app-launch: %1 (%2)").arg(error, app);
            };
#endif
        } else if (toolId == QLatin1String("system-notify")) {
            const QString title = argument(QStringLiteral("title")).trimmed();
            const QString message = argument(QStringLiteral("message")).trimmed();
            if (message.isEmpty()) {
                completion({ToolExecutionStatus::Succeeded,
                            QStringLiteral("system-notify: No message argument provided.")});
                return {};
            }
#if defined(Q_OS_MACOS)
            process.program = QStringLiteral("osascript");
            process.arguments = {
                QStringLiteral("-e"),
                QStringLiteral("display notification \"%1\" with title \"%2\"")
                    .arg(osascriptEscape(message),
                         osascriptEscape(title.isEmpty() ? QStringLiteral("Sentinel") : title))};
#elif defined(Q_OS_WIN)
            process.program = QStringLiteral("powershell");
            QString escapedTitle = title.isEmpty() ? QStringLiteral("Sentinel") : title;
            escapedTitle.replace(QLatin1Char('\''), QStringLiteral("''"));
            QString cleanMessage = message;
            cleanMessage.replace(QLatin1Char('\''), QStringLiteral("''"));
            const QString script = QStringLiteral(
                "[Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, "
                "ContentType = WindowsRuntime] | Out-Null;"
                "[Windows.Data.Xml.Dom.XmlDocument, Windows.Data.Xml.Dom.XmlDocument, ContentType "
                "= WindowsRuntime] | Out-Null;"
                "$template = '<toast><visual><binding template=\"ToastGeneric\"><text "
                "id=\"1\">%1</text><text id=\"2\">%2</text></binding></visual></toast>';"
                "$xml = New-Object Windows.Data.Xml.Dom.XmlDocument;$xml.LoadXml($template);"
                "$toast = New-Object Windows.UI.Notifications.ToastNotification($xml);"
                "[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier('"
                "Sentinel').Show($toast);");
            process.arguments = {QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                                 QStringLiteral("-Command"),
                                 script.arg(escapedTitle, cleanMessage)};
#else
            process.program = QStringLiteral("notify-send");
            process.arguments = {title.isEmpty() ? QStringLiteral("Sentinel") : title, message};
#endif
            format = [message](const ProcessRecord& record, const QString&, const QString& err) {
                const QString error = record.timedOut ? QStringLiteral("Timed out.")
                                      : record.state == ProcessState::Failed ? record.error
                                      : record.exitCode != 0                 ? err
                                                                             : QString();
                return error.isEmpty()
                           ? QStringLiteral("system-notify: Notification sent: %1").arg(message)
                           : QStringLiteral("system-notify: %1").arg(error);
            };
        } else if (toolId == QLatin1String("browser-screenshot") ||
                   toolId == QLatin1String("browser-pdf")) {
            const QString mode = toolId == QLatin1String("browser-pdf")
                                     ? QStringLiteral("pdf")
                                     : QStringLiteral("screenshot");
            const QString url = argument(QStringLiteral("url")).trimmed();
            if (url.isEmpty()) {
                completion({ToolExecutionStatus::Succeeded,
                            QStringLiteral("browser-%1: No url argument provided.").arg(mode)});
                return {};
            }
            QString path = argument(QStringLiteral("path")).trimmed();
            if (path.isEmpty())
                path =
                    QDir(QDir::tempPath())
                        .filePath(QStringLiteral("sentinel_browser_%1.%2")
                                      .arg(QDateTime::currentMSecsSinceEpoch())
                                      .arg(mode == QLatin1String("pdf") ? QStringLiteral("pdf")
                                                                        : QStringLiteral("png")));
            else {
                path = scopedPath(QDir::currentPath(), path);
                if (path.isEmpty()) {
                    completion(
                        {ToolExecutionStatus::Succeeded,
                         QStringLiteral("browser-%1: Path is outside the approved workspace.")
                             .arg(mode)});
                    return {};
                }
            }
            process.program = QStringLiteral("npx");
            process.arguments = {QStringLiteral("-y"), QStringLiteral("playwright"), mode};
            if (mode == QLatin1String("screenshot"))
                process.arguments.append(
                    {QStringLiteral("--wait-for-timeout"), QStringLiteral("2500")});
            process.arguments.append({url, path});
            process.timeoutMs = 300000;
            format = [mode, path](const ProcessRecord& record, const QString&, const QString& err) {
                if (record.timedOut)
                    return QStringLiteral("browser-%1: Playwright timed out after 300 s.")
                        .arg(mode);
                if (record.state == ProcessState::Failed && record.systemPid == 0)
                    return QStringLiteral("browser-%1: Node.js (npx) is required for Playwright "
                                          "browser tools. Install Node.js and run 'npx playwright "
                                          "install chromium' once.")
                        .arg(mode);
                if (record.exitCode != 0 || !QFile::exists(path))
                    return QStringLiteral("browser-%1: Playwright failed: %2")
                        .arg(mode,
                             err.isEmpty() ? QStringLiteral("no output file was produced") : err);
                return QStringLiteral("browser-%1: saved '%2'.").arg(mode, path);
            };
        } else if (toolId == QLatin1String("voice-transcribe")) {
            const QString path = argument(QStringLiteral("path"));
            if (path.isEmpty()) {
                completion({ToolExecutionStatus::Succeeded,
                            QStringLiteral("voice-transcribe: No audio path provided.")});
                return {};
            }
#if defined(Q_OS_WIN)
            process.program = QStringLiteral("whisper.exe");
#else
            process.program = QStringLiteral("whisper");
#endif
            process.arguments = {path};
            format = [path](const ProcessRecord& record, const QString& out, const QString&) {
                return record.timedOut
                           ? QStringLiteral("voice-transcribe: Whisper timed out for '%1'")
                                 .arg(path)
                           : QStringLiteral("voice-transcribe: OK\n%1").arg(out);
            };
        } else if (toolId == QLatin1String("voice-speak")) {
            const QString content = argument(QStringLiteral("text"));
            if (content.isEmpty()) {
                completion({ToolExecutionStatus::Succeeded,
                            QStringLiteral("voice-speak: No text argument provided.")});
                return {};
            }
#if defined(Q_OS_WIN)
            process.program = QStringLiteral("piper.exe");
#else
            process.program = QStringLiteral("piper");
#endif
            const QString path =
                QDir(QDir::tempPath()).filePath(QStringLiteral("sentinel_tts.wav"));
            process.arguments = {QStringLiteral("--model"),
                                 QStringLiteral("en_US-lessac-medium.onnx"),
                                 QStringLiteral("--output_file"), path};
            process.timeoutMs = 10000;
            input = content.toUtf8();
            format = [path](const ProcessRecord& record, const QString&, const QString&) {
                return record.timedOut
                           ? QStringLiteral("voice-speak: Piper TTS timed out.")
                           : QStringLiteral("voice-speak: TTS synthesis OK → %1").arg(path);
            };
        }
        struct Active {
            std::shared_ptr<ProcessExecutor> executor;
            QString id;
            QByteArray out;
            QByteArray err;
            Output output;
            Completion completion;
            std::function<QString(const ProcessRecord&, const QString&, const QString&)> format;
        };
        auto active = std::make_shared<Active>();
        active->executor = std::shared_ptr<ProcessExecutor>(
            new ProcessExecutor, [](ProcessExecutor* service) { service->deleteLater(); });
        active->output = std::move(output);
        active->completion = std::move(completion);
        active->format = std::move(format);
        std::weak_ptr<Active> weak = active;
        active->id = active->executor->start(
            process,
            [weak](const ProcessRecord& record) {
                auto current = weak.lock();
                if (!current ||
                    (record.state != ProcessState::Exited && record.state != ProcessState::Failed &&
                     record.state != ProcessState::Cancelled))
                    return;
                current->completion(
                    {ToolExecutionStatus::Succeeded,
                     current->format(record, QString::fromUtf8(current->out).trimmed(),
                                     QString::fromUtf8(current->err).trimmed())});
            },
            [weak](const QString& id, ProcessStream stream, const QByteArray& bytes) {
                auto current = weak.lock();
                if (!current)
                    return;
                auto& buffer = stream == ProcessStream::Stdout ? current->out : current->err;
                if (buffer.size() < 65536)
                    buffer.append(bytes.left(65536 - buffer.size()));
                if (current->output)
                    current->output(id, stream, bytes);
            });
        if (!input.isEmpty()) {
            active->executor->write(active->id, input);
            active->executor->closeWriteChannel(active->id);
        }
        return [active] {
            if (active->executor && !active->id.isEmpty())
                active->executor->terminate(active->id);
        };
    }
    const QString command = argument(QStringLiteral("command"));
    if (command.isEmpty()) {
        completion({ToolExecutionStatus::Failed,
                    QStringLiteral("run-command: No command argument provided.")});
        return {};
    }
    bool timeoutOk = false;
    const int suppliedTimeout = argument(QStringLiteral("timeout")).trimmed().toInt(&timeoutOk);
    int timeoutMs = qBound(1000, timeoutOk ? suppliedTimeout : 60000, 600000);
    const QString root = QDir::currentPath();
    QString workdir = root;
    const QString requestedWorkdir = argument(QStringLiteral("workdir")).trimmed();
    if (!requestedWorkdir.isEmpty()) {
        workdir = scopedPath(root, requestedWorkdir);
        if (workdir.isEmpty() || !QDir(workdir).exists()) {
            completion({ToolExecutionStatus::Blocked,
                        QStringLiteral("run-command: workdir is outside the approved workspace.")});
            return {};
        }
    }
    const bool docker =
        argument(QStringLiteral("sandbox")).trimmed().toLower() == QLatin1String("docker");
    ProcessRequest process;
    process.workingDirectory = workdir;
    process.sessionId = sessionId;
    process.toolCallId = toolCallId;
    process.timeoutMs = timeoutMs;
    if (docker) {
        process.program = QStringLiteral("docker");
        process.arguments = {
            QStringLiteral("run"),       QStringLiteral("--rm"),
            QStringLiteral("--network"), QStringLiteral("none"),
            QStringLiteral("--memory"),  QStringLiteral("2g"),
            QStringLiteral("--cpus"),    QStringLiteral("2"),
            QStringLiteral("-v"),        QStringLiteral("%1:/workspace").arg(workdir),
            QStringLiteral("-w"),        QStringLiteral("/workspace")};
        if (timeoutMs > 60000)
            process.timeoutMs += 120000;
        process.arguments.append(
            {QStringLiteral("alpine:3.20"), QStringLiteral("sh"), QStringLiteral("-c"), command});
    } else {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString pathEnv = env.value(QStringLiteral("PATH"));
#if defined(Q_OS_MACOS)
        if (!pathEnv.contains(QStringLiteral("/opt/homebrew/bin")))
            pathEnv = QStringLiteral("/opt/homebrew/bin:/usr/local/bin:") + pathEnv;
#elif defined(Q_OS_UNIX)
        if (!pathEnv.contains(QStringLiteral("/usr/local/bin")))
            pathEnv = QStringLiteral("/usr/local/bin:") + pathEnv;
#endif
        env.insert(QStringLiteral("PATH"), pathEnv);
        process.environment = env;
#if defined(Q_OS_WIN)
        process.program = QStringLiteral("cmd.exe");
        process.arguments = {QStringLiteral("/c"), command};
#else
        process.program = QStringLiteral("/bin/sh");
        process.arguments = {QStringLiteral("-c"), command};
#endif
    }

    struct ActiveCommand {
        std::shared_ptr<ProcessExecutor> executor;
        QString id;
        QByteArray stdoutBytes;
        QByteArray stderrBytes;
        Output output;
        Completion completion;
        bool docker = false;
        QString command;
        int timeoutMs = 0;
    };
    auto active = std::make_shared<ActiveCommand>();
    active->executor = std::shared_ptr<ProcessExecutor>(
        new ProcessExecutor, [](ProcessExecutor* service) { service->deleteLater(); });
    active->output = std::move(output);
    active->completion = std::move(completion);
    active->docker = docker;
    active->command = command;
    active->timeoutMs = timeoutMs;
    std::weak_ptr<ActiveCommand> weak = active;
    active->id = active->executor->start(
        process,
        [weak](const ProcessRecord& record) {
            auto current = weak.lock();
            if (!current ||
                (record.state != ProcessState::Exited && record.state != ProcessState::Failed &&
                 record.state != ProcessState::Cancelled))
                return;
            const QString out = QString::fromUtf8(current->stdoutBytes).trimmed();
            const QString err = QString::fromUtf8(current->stderrBytes).trimmed();
            QString summary;
            if (record.timedOut) {
                summary =
                    current->docker
                        ? QStringLiteral(
                              "run-command: docker sandbox terminated command after timeout %1 ms.")
                              .arg(current->timeoutMs)
                        : QStringLiteral(
                              "run-command: Shell tool terminated command after exceeding timeout "
                              "%1 ms. If this command is expected to take longer, retry it with a "
                              "larger timeout value in milliseconds.\nCommand: %2")
                              .arg(current->timeoutMs)
                              .arg(current->command);
            } else if (record.state == ProcessState::Cancelled) {
                summary = QStringLiteral("run-command: Command cancelled.");
            } else if (record.state == ProcessState::Failed && record.systemPid == 0) {
                summary =
                    current->docker
                        ? QStringLiteral(
                              "run-command: docker sandbox requested but the docker CLI is not "
                              "available. Install Docker or retry without sandbox=docker.")
                        : QStringLiteral("run-command: Failed to start: %1").arg(current->command);
            } else if (current->docker) {
                if (record.exitCode == 0)
                    summary =
                        out.isEmpty() && err.isEmpty()
                            ? QStringLiteral("Command executed in docker sandbox. (exit=0)")
                            : QStringLiteral(
                                  "Command executed in docker sandbox. (exit=0)\n\n[STDOUT]:\n%1")
                                  .arg(out);
                else if (err.contains(QStringLiteral("failed to connect to the docker API")) ||
                         err.contains(QStringLiteral("Is the docker daemon running")))
                    summary =
                        QStringLiteral("run-command: the docker CLI is installed but the daemon is "
                                       "not running. Start Docker (or OrbStack) and retry, or run "
                                       "without sandbox=docker.\nDetails: %1")
                            .arg(err);
                else
                    summary = QStringLiteral("Docker sandbox command failed. "
                                             "(exit=%1)\n\n[STDOUT]:\n%2\n\n[STDERR]:\n%3")
                                  .arg(record.exitCode)
                                  .arg(out, err);
            } else if (record.exitCode == 0) {
                summary =
                    out.isEmpty() && err.isEmpty()
                        ? QStringLiteral("Command executed successfully. (exit=0)")
                        : QStringLiteral("Command executed successfully. (exit=0)\n\n[STDOUT]:\n%1")
                              .arg(out);
                if (!err.isEmpty())
                    summary += QStringLiteral("\n\n[STDERR]:\n%1").arg(err);
            } else {
                summary =
                    QStringLiteral(
                        "Command execution failed. (exit=%1)\n\n[STDOUT]:\n%2\n\n[STDERR]:\n%3")
                        .arg(record.exitCode)
                        .arg(out, err);
            }
            current->completion(
                {record.state == ProcessState::Exited && !record.timedOut && record.exitCode == 0
                     ? ToolExecutionStatus::Succeeded
                     : ToolExecutionStatus::Failed,
                 summary});
        },
        [weak](const QString& id, ProcessStream stream, const QByteArray& bytes) {
            auto current = weak.lock();
            if (!current)
                return;
            auto& buffer =
                stream == ProcessStream::Stdout ? current->stdoutBytes : current->stderrBytes;
            if (buffer.size() < 65536)
                buffer.append(bytes.left(65536 - buffer.size()));
            if (current->output)
                current->output(id, stream, bytes);
        });
    return [active] {
        if (active->executor && !active->id.isEmpty())
            active->executor->terminate(active->id);
    };
}

namespace {

constexpr int kDefaultReadLimit = 2000;
constexpr int kMaxLineLength = 2000;
constexpr int kMaxReadBytes = 50 * 1024;
constexpr int kGrepGlobLimit = 100;
constexpr int kDirListLimit = 100;
constexpr int kWebFetchPreviewChars = 8000;

bool isSensitiveToolPath(const QString& canonicalPath);

QString getArgument(const PlannedToolInvocation& invocation, const QString& argId) {
    for (const auto& arg : invocation.arguments) {
        if (arg.id == argId) {
            return arg.value;
        }
    }
    return QString();
}

int getIntArgument(const PlannedToolInvocation& invocation, const QString& argId, int fallback) {
    bool ok = false;
    const int value = getArgument(invocation, argId).trimmed().toInt(&ok);
    return ok ? value : fallback;
}

QString scopedPath(const QString& workingDirectory, const QString& rawPath) {
    QString path = rawPath.trimmed();
    if (path.isEmpty()) {
        return QString();
    }
    if (path == QLatin1String("~"))
        path = QDir::homePath();
    else if (path.startsWith(QStringLiteral("~/")))
        path = QDir::home().absoluteFilePath(path.mid(2));
    QFileInfo fileInfo(path);
    if (fileInfo.isRelative()) {
        path = QDir(workingDirectory).absoluteFilePath(path);
    }
    const QString scoped = PathGuard::safePath(workingDirectory, path);
    if (scoped.isEmpty() || isSensitiveToolPath(scoped)) {
        return QString();
    }
    return scoped;
}

bool isSensitiveToolPath(const QString& canonicalPath) {
    static const QStringList sensitiveDirs{
        QStringLiteral(".ssh"),  QStringLiteral(".gnupg"),          QStringLiteral(".aws"),
        QStringLiteral(".kube"), QStringLiteral(".password-store"),
    };
    static const QStringList sensitiveFiles{
        QStringLiteral("id_rsa"),  QStringLiteral("id_ed25519"),       QStringLiteral("id_ecdsa"),
        QStringLiteral("id_dsa"),  QStringLiteral(".git-credentials"), QStringLiteral(".npmrc"),
        QStringLiteral(".pypirc"), QStringLiteral(".netrc"),
    };

    const QFileInfo info(canonicalPath);
    if (sensitiveFiles.contains(info.fileName())) {
        return true;
    }

    const QStringList parts =
        QDir::cleanPath(canonicalPath).split(QDir::separator(), Qt::SkipEmptyParts);
    for (const auto& part : parts) {
        if (sensitiveDirs.contains(part)) {
            return true;
        }
    }
    return false;
}

bool isFilesystemRootPath(const QString& canonicalPath) {
    const QString clean = QDir::cleanPath(canonicalPath);
#ifdef Q_OS_WIN
    if (clean.size() == 2 && clean.endsWith(QLatin1Char(':'))) {
        return true;
    }
    if (clean.size() == 3 && clean.at(1) == QLatin1Char(':') &&
        (clean.at(2) == QLatin1Char('/') || clean.at(2) == QLatin1Char('\\'))) {
        return true;
    }
#endif
    return clean == QStringLiteral("/") || clean.isEmpty();
}

bool isForbiddenWorkspacePath(const QString& canonicalPath) {
    if (isFilesystemRootPath(canonicalPath)) {
        return true;
    }

    const QString clean = QDir::cleanPath(canonicalPath);
    static const QStringList systemRoots{
        QStringLiteral("/etc"),
        QStringLiteral("/usr"),
        QStringLiteral("/bin"),
        QStringLiteral("/sbin"),
        QStringLiteral("/lib"),
        QStringLiteral("/lib64"),
        QStringLiteral("/var"),
        QStringLiteral("/boot"),
        QStringLiteral("/root"),
        QStringLiteral("/System"),
        QStringLiteral("/Library/Keychains"),
        QStringLiteral("/Applications"),
        QStringLiteral("C:\\Windows"),
        QStringLiteral("C:\\Program Files"),
        QStringLiteral("C:\\Program Files (x86)"),
    };
    for (const auto& systemRoot : systemRoots) {
        const QString root = QDir::cleanPath(systemRoot);
        if (clean == root || clean.startsWith(root + QLatin1Char('/'))) {
            return true;
        }
    }

    const QString home = QDir::cleanPath(QDir::homePath());
    if (home != clean && home != QStringLiteral("/") && home.startsWith(clean + QLatin1Char('/'))) {
        return true;
    }

    return isSensitiveToolPath(clean);
}

QString runSynchronousProcess(const QString& program, const QStringList& args,
                              const QString& workingDirectory, int timeoutMs,
                              QString* errorOut = nullptr) {
    QProcess process;
    if (!workingDirectory.isEmpty() && QDir(workingDirectory).exists()) {
        process.setWorkingDirectory(workingDirectory);
    }
    process.start(program, args);
    if (!process.waitForStarted(5000)) {
        if (errorOut) {
            *errorOut = QStringLiteral("Failed to start '%1'.").arg(program);
        }
        return QString();
    }
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(3000);
        if (errorOut) {
            *errorOut = QStringLiteral("Timed out after %1 ms.").arg(QString::number(timeoutMs));
        }
        return QString();
    }
    const QString out = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString err = QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (process.exitCode() != 0 && errorOut && out.isEmpty()) {
        *errorOut =
            err.isEmpty() ? QStringLiteral("Exited with code %1.").arg(process.exitCode()) : err;
    }
    return out;
}

QString osascriptEscape(const QString& text) {
    QString escaped = text;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return escaped;
}

QString humanReadableBytes(quint64 bytes) {
    const double gib = static_cast<double>(bytes) / (1024.0 * 1024.0 * 1024.0);
    if (gib >= 1.0) {
        return QStringLiteral("%1 GiB").arg(QString::number(gib, 'f', 1));
    }
    const double mib = static_cast<double>(bytes) / (1024.0 * 1024.0);
    return QStringLiteral("%1 MiB").arg(QString::number(mib, 'f', 0));
}

QClipboard* activeClipboard() {
    const auto* guiApp = qobject_cast<const QGuiApplication*>(QCoreApplication::instance());
    return guiApp ? QGuiApplication::clipboard() : nullptr;
}

QString systemInfoReport() {
    QStringList lines;
    lines.append(QStringLiteral("OS: %1").arg(QSysInfo::prettyProductName()));
    lines.append(QStringLiteral("Kernel: %1").arg(QSysInfo::kernelVersion()));
    lines.append(QStringLiteral("CPU architecture: %1").arg(QSysInfo::currentCpuArchitecture()));
    lines.append(QStringLiteral("Hostname: %1").arg(QSysInfo::machineHostName()));
    QString user = qEnvironmentVariable("USER");
    if (user.isEmpty()) {
        user = qEnvironmentVariable("USERNAME");
    }
    if (!user.isEmpty()) {
        lines.append(QStringLiteral("User: %1").arg(user));
    }

    quint64 totalRamBytes = 0;
#if defined(Q_OS_UNIX)
    const long pages = sysconf(_SC_PHYS_PAGES);
    const long pageSize = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && pageSize > 0) {
        totalRamBytes = static_cast<quint64>(pages) * static_cast<quint64>(pageSize);
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX memoryStatus{};
    memoryStatus.dwLength = sizeof(memoryStatus);
    if (GlobalMemoryStatusEx(&memoryStatus)) {
        totalRamBytes = memoryStatus.ullTotalPhys;
    }
#endif
    if (totalRamBytes > 0) {
        lines.append(QStringLiteral("Total RAM: %1").arg(humanReadableBytes(totalRamBytes)));
    }

    const QStorageInfo root = QStorageInfo::root();
    if (root.isValid() && root.isReady()) {
        lines.append(QStringLiteral("Root volume (%1): %2 free of %3")
                         .arg(QDir::toNativeSeparators(root.rootPath()),
                              humanReadableBytes(static_cast<quint64>(root.bytesFree())),
                              humanReadableBytes(static_cast<quint64>(root.bytesTotal()))));
    }
    lines.append(
        QStringLiteral("Workspace: %1").arg(QDir::toNativeSeparators(QDir::currentPath())));
    return lines.join(QLatin1Char('\n'));
}

QString processListReport() {
    QString output;
    QString error;
#if defined(Q_OS_WIN)
    output = runSynchronousProcess(QStringLiteral("tasklist"), {}, QString(), 15000, &error);
#else
    output = runSynchronousProcess(QStringLiteral("ps"),
                                   {QStringLiteral("-eo"), QStringLiteral("pid,pcpu,comm")},
                                   QString(), 15000, &error);
#endif
    if (output.isEmpty()) {
        return error.isEmpty() ? QStringLiteral("process-list: No process output was returned.")
                               : QStringLiteral("process-list: %1").arg(error);
    }

    const auto lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    QStringList shown;
    const int maxLines = 40;
    for (int i = 0; i < lines.size() && shown.size() < maxLines; ++i) {
        QString line = lines.at(i).trimmed();
        if (line.size() > kMaxLineLength) {
            line = line.left(kMaxLineLength) + QStringLiteral("... (line truncated)");
        }
        shown.append(line);
    }
    return QStringLiteral("process-list: %1 process line(s) (showing first %2):\n%3")
        .arg(QString::number(lines.size()), QString::number(shown.size()),
             shown.join(QLatin1Char('\n')));
}

// Freedesktop .desktop entry resolution (Linux/BSD app-launch support).
#if !defined(Q_OS_MACOS) && !defined(Q_OS_WIN)
struct DesktopEntry {
    QString desktopId;
    QString name;
    QString exec;
    QString icon;
};

// Locates a freedesktop .desktop entry for an application name such as
// "Spotify" or "org.kde.dolphin". Searches the standard data directories.
std::optional<DesktopEntry> findDesktopEntry(const QString& appName) {
    const QString needle = appName.trimmed();
    if (needle.isEmpty()) {
        return std::nullopt;
    }
    const QString lowered = needle.toLower();

    QStringList searchRoots;
    const QString dataHome = qEnvironmentVariable("XDG_DATA_HOME").isEmpty()
                                 ? QDir::home().filePath(QStringLiteral(".local/share"))
                                 : qEnvironmentVariable("XDG_DATA_HOME");
    searchRoots.append(dataHome);
    const QString dataDirsEnv = qEnvironmentVariable("XDG_DATA_DIRS");
    const QStringList dataDirs =
        dataDirsEnv.isEmpty()
            ? QStringList{QStringLiteral("/usr/share"), QStringLiteral("/usr/local/share")}
            : dataDirsEnv.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    searchRoots.append(dataDirs);

    std::optional<DesktopEntry> best;
    for (const auto& root : searchRoots) {
        QDirIterator it(QDir(root).filePath(QStringLiteral("applications")),
                        {QStringLiteral("*.desktop")}, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
            QSettings desktopFile(path, QSettings::IniFormat);
            desktopFile.beginGroup(QStringLiteral("Desktop Entry"));

            const QString type =
                desktopFile.value(QStringLiteral("Type"), QString()).toString().trimmed();
            const bool hidden = desktopFile.value(QStringLiteral("Hidden"), false).toBool();
            if (type != QStringLiteral("Application") || hidden) {
                continue;
            }

            const QString name =
                desktopFile.value(QStringLiteral("Name"), QString()).toString().trimmed();
            const QString desktopId = QFileInfo(path).completeBaseName();
            const bool matches =
                desktopId.compare(lowered, Qt::CaseInsensitive) == 0 ||
                desktopId.endsWith(QStringLiteral(".") + lowered, Qt::CaseInsensitive) ||
                name.compare(needle, Qt::CaseInsensitive) == 0;
            if (!matches) {
                continue;
            }

            DesktopEntry entry;
            entry.desktopId = desktopId;
            entry.name = name;
            entry.exec = desktopFile.value(QStringLiteral("Exec"), QString()).toString();
            entry.icon = desktopFile.value(QStringLiteral("Icon"), QString()).toString();
            desktopFile.endGroup();

            // Prefer exact id matches (e.g. "spotify") over suffix matches
            // (e.g. "org.kde.dolphin" for "dolphin").
            if (!best || (best->desktopId.toLower() != lowered && desktopId.toLower() == lowered)) {
                best = entry;
            }
        }
    }
    return best;
}

// Strips freedesktop field codes (%f, %u, %F, %U) from an Exec= value.
QString desktopExecToCommand(const QString& exec) {
    static const QRegularExpression fieldCodes(QStringLiteral("%[fFuUdDnNickvm]"));
    QString command = exec;
    command.remove(fieldCodes);
    return command.simplified();
}
#endif // !Q_OS_MACOS && !Q_OS_WIN

// True when the text looks like a website address ("sahibinden.com",
// "www.x.com", "https://x.com") rather than an application name.
bool looksLikeDomainName(const QString& text) {
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(QLatin1Char(' '))) {
        return false;
    }
    const QString lowered = trimmed.toLower();
    if (lowered.startsWith(QStringLiteral("http://")) ||
        lowered.startsWith(QStringLiteral("https://")) ||
        lowered.startsWith(QStringLiteral("www."))) {
        return true;
    }
    const int dot = trimmed.lastIndexOf(QLatin1Char('.'));
    if (dot < 1 || dot >= trimmed.size() - 2) {
        return false;
    }
    const QString suffix = trimmed.mid(dot + 1);
    if (suffix.size() > 10) {
        return false;
    }
    for (const QChar c : suffix) {
        if (!c.isLetter()) {
            return false;
        }
    }
    return true;
}

bool dockerAvailable() {
    return !runSynchronousProcess(QStringLiteral("docker"), {QStringLiteral("--version")},
                                  QString(), 5000)
                .isEmpty();
}

// Runs a shell command inside a throwaway docker container with the workspace
// mounted read-write at /workspace. Ported from openclaw's Docker sandbox
// pattern; degrades to a clear error when docker is missing.
QString runCommandInDocker(const QString& command, const QString& workspace, int timeoutMs) {
    if (!dockerAvailable()) {
        return QStringLiteral(
            "run-command: docker sandbox requested but the docker CLI is not available. "
            "Install Docker or retry without sandbox=docker.");
    }
    const QString image = QStringLiteral("alpine:3.20");
    QStringList args{QStringLiteral("run"),       QStringLiteral("--rm"),
                     QStringLiteral("--network"), QStringLiteral("none"),
                     QStringLiteral("--memory"),  QStringLiteral("2g"),
                     QStringLiteral("--cpus"),    QStringLiteral("2"),
                     QStringLiteral("-v"),        QStringLiteral("%1:/workspace").arg(workspace),
                     QStringLiteral("-w"),        QStringLiteral("/workspace")};
    if (timeoutMs > 60000) {
        // Leave time for image pulls on first use.
        timeoutMs += 120000;
    }
    args.append(image);
    args.append(QStringLiteral("sh"));
    args.append(QStringLiteral("-c"));
    args.append(command);

    QProcess process;
    process.start(QStringLiteral("docker"), args);
    if (!process.waitForStarted(10000)) {
        return QStringLiteral("run-command: failed to start docker (%1).").arg(command);
    }
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(3000);
        return QStringLiteral("run-command: docker sandbox terminated command after timeout %1 ms.")
            .arg(QString::number(timeoutMs));
    }
    const QString out = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
    const QString err = QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (process.exitCode() == 0) {
        return out.isEmpty() && err.isEmpty()
                   ? QStringLiteral("Command executed in docker sandbox. (exit=0)")
                   : QStringLiteral("Command executed in docker sandbox. (exit=0)\n\n[STDOUT]:\n%1")
                         .arg(out);
    }
    if (err.contains(QStringLiteral("failed to connect to the docker API")) ||
        err.contains(QStringLiteral("Is the docker daemon running"))) {
        return QStringLiteral(
                   "run-command: the docker CLI is installed but the daemon is not running. "
                   "Start Docker (or OrbStack) and retry, or run without sandbox=docker.\n"
                   "Details: %1")
            .arg(err);
    }
    return QStringLiteral(
               "Docker sandbox command failed. (exit=%1)\n\n[STDOUT]:\n%2\n\n[STDERR]:\n%3")
        .arg(QString::number(process.exitCode()), out, err);
}

// Runs `npx playwright <mode>` for browser-screenshot / browser-pdf. Real
// Playwright CLI integration (ported from SWE-agent's browser bundle idea);
// requires Node.js and npx on PATH, fails gracefully otherwise.
QString runPlaywrightCli(const QString& mode, const QString& url, const QString& outputPath,
                         const QString& extraWait) {
    if (runSynchronousProcess(QStringLiteral("npx"), {QStringLiteral("--version")}, QString(), 5000)
            .isEmpty()) {
        return QStringLiteral(
                   "browser-%1: Node.js (npx) is required for Playwright browser tools. Install "
                   "Node.js and run 'npx playwright install chromium' once.")
            .arg(mode);
    }

    QStringList args{QStringLiteral("-y"), QStringLiteral("playwright"), mode};
    if (!extraWait.isEmpty()) {
        args.append(QStringLiteral("--wait-for-timeout"));
        args.append(extraWait);
    }
    args.append(url);
    args.append(outputPath);

    QProcess process;
    process.start(QStringLiteral("npx"), args);
    if (!process.waitForStarted(10000)) {
        return QStringLiteral("browser-%1: failed to start npx.").arg(mode);
    }
    // npx may fetch the playwright package on first use; allow extra time.
    if (!process.waitForFinished(300000)) {
        process.kill();
        process.waitForFinished(3000);
        return QStringLiteral("browser-%1: Playwright timed out after 300 s.").arg(mode);
    }
    const QString err = QString::fromUtf8(process.readAllStandardError()).trimmed();
    if (process.exitCode() != 0 || !QFile::exists(outputPath)) {
        return QStringLiteral("browser-%1: Playwright failed: %2")
            .arg(mode, err.isEmpty() ? QStringLiteral("no output file was produced") : err);
    }
    return QStringLiteral("browser-%1: saved '%2'.").arg(mode, outputPath);
}

// Extracts code symbols (classes, functions, methods) with line numbers for
// common language families. Ported concept from Cline's
// list_code_definition_names tool.
QStringList extractCodeDefinitions(const QString& path, const QByteArray& bytes,
                                   const FileSystemOperationContext& context,
                                   bool& cancelled, bool& truncated) {
    const QString content = QString::fromUtf8(bytes);
    const QStringList lines = content.split(QLatin1Char('\n'));
    const QString suffix = QFileInfo(path).suffix().toLower();

    static const QRegularExpression cxxPattern(QStringLiteral(
        "^\\s*(?:template\\s*<[^>]*>\\s*)?(?:class|struct|enum\\s+class|enum|namespace)\\s+"
        "([A-Za-z_]\\w*)"));
    static const QRegularExpression cxxFunction(
        QStringLiteral("^\\s*(?:[A-Za-z_][\\w:<>,\\*\\&\\s]*)\\s+([A-Za-z_]\\w*)\\s*\\("));
    static const QRegularExpression pythonPattern(
        QStringLiteral("^\\s*(?:async\\s+)?def\\s+([A-Za-z_]\\w*)|^\\s*class\\s+([A-Za-z_]\\w*)"));
    static const QRegularExpression jsPattern(
        QStringLiteral("^\\s*(?:export\\s+)?(?:default\\s+)?(?:async\\s+)?(?:function\\*?|class|"
                       "interface|type|enum)\\s+([A-Za-z_$][\\w$]*)"));
    static const QRegularExpression rustPattern(QStringLiteral(
        "^\\s*(?:pub(?:\\(\\w+\\))?\\s+)?(?:async\\s+)?(?:fn|struct|enum|trait|mod)\\s+"
        "([A-Za-z_]\\w*)"));

    const bool isPython = suffix == QStringLiteral("py");
    const bool isJsLike = suffix == QStringLiteral("js") || suffix == QStringLiteral("jsx") ||
                          suffix == QStringLiteral("ts") || suffix == QStringLiteral("tsx") ||
                          suffix == QStringLiteral("mjs");
    const bool isRust = suffix == QStringLiteral("rs");
    const bool isCxx = suffix == QStringLiteral("c") || suffix == QStringLiteral("cc") ||
                       suffix == QStringLiteral("cpp") || suffix == QStringLiteral("cxx") ||
                       suffix == QStringLiteral("h") || suffix == QStringLiteral("hpp") ||
                       suffix == QStringLiteral("java") || suffix == QStringLiteral("cs");

    QStringList definitions;
    for (int i = 0; i < lines.size(); ++i) {
        if ((i & 127) == 0 && context.isCancelled()) { cancelled = true; break; }
        const QString& line = lines.at(i);
        auto add = [&](const QString& kind, const QString& name) {
            definitions.append(
                QStringLiteral("%1:%2 %3 %4").arg(QString::number(i + 1), kind, name));
        };
        if (isPython) {
            const auto m = pythonPattern.match(line);
            if (m.hasMatch()) {
                if (!m.captured(1).isEmpty()) {
                    add(QStringLiteral("def"), m.captured(1));
                } else {
                    add(QStringLiteral("class"), m.captured(2));
                }
            }
        } else if (isJsLike) {
            const auto m = jsPattern.match(line);
            if (m.hasMatch()) {
                add(QStringLiteral("symbol"), m.captured(1));
            }
        } else if (isRust) {
            const auto m = rustPattern.match(line);
            if (m.hasMatch()) {
                add(QStringLiteral("item"), m.captured(1));
            }
        } else if (isCxx) {
            const auto m = cxxPattern.match(line);
            if (m.hasMatch()) {
                add(QStringLiteral("type"), m.captured(1));
                continue;
            }
            const auto f = cxxFunction.match(line);
            if (f.hasMatch() && f.captured(1) != QStringLiteral("return") &&
                f.captured(1) != QStringLiteral("if") && f.captured(1) != QStringLiteral("for") &&
                f.captured(1) != QStringLiteral("while") &&
                f.captured(1) != QStringLiteral("switch")) {
                add(QStringLiteral("function"), f.captured(1));
            }
        }
        if (definitions.size() >= 100) {
            definitions.append(QStringLiteral("(truncated at 100 definitions)"));
            truncated = true;
            break;
        }
    }
    return definitions;
}

QDateTime parseAlarmTime(const QString& raw) {
    const auto trimmed = raw.trimmed();
    QDateTime parsed = QDateTime::fromString(trimmed, Qt::ISODateWithMs);
    if (!parsed.isValid()) {
        parsed = QDateTime::fromString(trimmed, Qt::ISODate);
    }
    if (parsed.isValid()) {
        return parsed;
    }

    const QRegularExpression timeOnly(QStringLiteral("^(\\d{1,2}):(\\d{2})(?::(\\d{2}))?$"));
    const auto match = timeOnly.match(trimmed);
    if (match.hasMatch()) {
        const auto now = QDateTime::currentDateTime();
        QTime time(match.captured(1).toInt(), match.captured(2).toInt(), match.captured(3).toInt());
        QDateTime candidate(now.date(), time);
        if (candidate <= now) {
            candidate = candidate.addDays(1);
        }
        return candidate;
    }

    for (const char* format : {"yyyy-MM-dd HH:mm", "dd.MM.yyyy HH:mm", "HH:mm:ss"}) {
        parsed = QDateTime::fromString(trimmed, QString::fromLatin1(format));
        if (parsed.isValid()) {
            return parsed;
        }
    }
    return QDateTime();
}

} // namespace

ToolExecutionResult RealToolExecutor::execute(const ToolExecutionRequest& request) const {
    Q_UNUSED(request)
    return {ToolExecutionStatus::Blocked,
            QStringLiteral("Registered tools must execute through ToolExecutionGateway.")};
}

ToolExecutionResult
RealToolExecutor::executeLocalPlanSummary(const PlannedToolInvocation& invocation,
                                          QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        logs.append(QStringLiteral("Executed: Local Plan Summary"));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

QString RealToolExecutor::resolveToolPath(const QString& cwd, const QString& raw, bool write) const {
    const auto result = fileSystemService_.resolve(raw, cwd, write ? FileSystemAccess::Write : FileSystemAccess::Read);
    return result.ok() ? result.value->canonicalPath : QString{};
}
namespace {
FileSystemOperationContext operationContext(const PlannedToolInvocation& invocation) {
    return {invocation.cancellation, invocation.toolCancellation};
}
QJsonArray traversalIssues(const TraversalStatus& status) {
    QJsonArray issues;
    for (const auto& issue : status.issues)
        issues.append(QJsonObject{{QStringLiteral("resource"), issue.resource},
                                  {QStringLiteral("failure"), static_cast<int>(issue.failure)}});
    return issues;
}
template <typename T> ToolExecutionResult fileToolFailure(const QString& tool, const FileSystemResult<T>& result) {
    auto observation = std::make_shared<StructuredObservation>();
    observation->kind = StructuredObservationKind::FileSystemFailure;
    observation->fileSystemFailure = result.failure;
    observation->fileSystemOperation = result.operation;
    if (tool == QLatin1String("grep")) observation->fileSystemOperation = FileSystemOperation::Grep;
    else if (tool == QLatin1String("glob")) observation->fileSystemOperation = FileSystemOperation::Glob;
    else if (result.operation == FileSystemOperation::Stat) {
        if (tool == QLatin1String("list-directory")) observation->fileSystemOperation = FileSystemOperation::ListDirectory;
        else if (tool == QLatin1String("read-file")) observation->fileSystemOperation = FileSystemOperation::ReadFile;
        else if (tool == QLatin1String("write-file")) observation->fileSystemOperation = FileSystemOperation::WriteFile;
        else if (tool == QLatin1String("edit-file")) observation->fileSystemOperation = FileSystemOperation::EditFile;
        else if (tool == QLatin1String("delete-file")) observation->fileSystemOperation = FileSystemOperation::Delete;
        else if (tool == QLatin1String("move-file")) observation->fileSystemOperation = FileSystemOperation::Move;
        else if (tool == QLatin1String("glob")) observation->fileSystemOperation = FileSystemOperation::Glob;
        else if (tool == QLatin1String("grep")) observation->fileSystemOperation = FileSystemOperation::Grep;
    }
    observation->failureResource = result.resource;
    observation->data = {{QStringLiteral("resource"), result.resource}};
    const auto status = result.failure == FileSystemFailure::PermissionDenied ? ToolExecutionStatus::Blocked
                      : result.failure == FileSystemFailure::InvalidPath ? ToolExecutionStatus::InvalidArguments
                      : ToolExecutionStatus::Failed;
    QString detail = result.diagnostic;
    if (detail.isEmpty()) {
        if (result.failure == FileSystemFailure::PermissionDenied)
            detail = QStringLiteral("Access denied: %1").arg(result.resource);
        else if (result.failure == FileSystemFailure::InvalidPath)
            detail = QStringLiteral("Invalid path");
        else if (result.failure == FileSystemFailure::NotFound)
            detail = QStringLiteral("Not found: %1").arg(result.resource);
        else if (result.failure == FileSystemFailure::AlreadyExists)
            detail = QStringLiteral("Already exists: %1").arg(result.resource);
        else detail = QStringLiteral("Filesystem operation failed: %1").arg(result.resource);
    }
    return {status, QStringLiteral("%1: %2").arg(tool, detail), observation};
}
FileSystemResult<AuthorizedPath> authorizedFilePath(
    const IFileSystemService& service, const PlannedToolInvocation& invocation,
    const QString& argument, AccessMode access, const QString& cwd) {
    if (invocation.resourceSnapshot && invocation.resourceSnapshot->authorized &&
        invocation.resourceSnapshot->workingDirectory == PathGuard::canonicalPath(cwd)) {
        for (const auto& resource : invocation.resourceSnapshot->files) {
            if (resource.argument == argument && resource.patchAction.isEmpty() &&
                resource.access == access)
                return service.revalidateAuthorized(resource.path, cwd);
        }
    }
    FileSystemResult<AuthorizedPath> denied;
    denied.failure = FileSystemFailure::PermissionDenied;
    denied.resource = argument;
    denied.diagnostic = QStringLiteral("Pre-execution filesystem authorization is missing.");
    return denied;
}
}
ToolExecutionResult RealToolExecutor::executeListDirectory(const PlannedToolInvocation& invocation, QString& cwd) const {
    const auto path = authorizedFilePath(fileSystemService_, invocation, QStringLiteral("path"), AccessMode::Read, cwd);
    if (!path.ok()) return fileToolFailure(QStringLiteral("list-directory"), path);
    const auto listing = fileSystemService_.listDirectory(*path.value,
        getArgument(invocation, QStringLiteral("includeHidden")) == QLatin1String("true"),
        500, operationContext(invocation));
    if (!listing.ok()) return fileToolFailure(QStringLiteral("list-directory"), listing);
    QJsonArray entries;
    for (const auto& entry : listing.value->entries)
        entries.append(QJsonObject{{QStringLiteral("name"), entry.name},
            {QStringLiteral("type"), entry.directory ? QStringLiteral("directory")
                : entry.regularFile ? QStringLiteral("file") : QStringLiteral("other")}});
    const QJsonObject data{{QStringLiteral("path"), listing.resource}, {QStringLiteral("entries"), entries},
        {QStringLiteral("recursive"), false},
        {QStringLiteral("includeHidden"), getArgument(invocation, QStringLiteral("includeHidden")) == QLatin1String("true")},
        {QStringLiteral("complete"), listing.value->status.complete},
        {QStringLiteral("truncated"), listing.value->status.truncated},
        {QStringLiteral("cancelled"), listing.value->status.cancelled},
        {QStringLiteral("issues"), traversalIssues(listing.value->status)}};
    const QString summary = listing.value->status.cancelled
        ? QStringLiteral("Directory listing stopped after %1 entries. Scope not fully inspected.\n%2")
              .arg(entries.size()).arg(QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact)))
        : QString::fromUtf8(QJsonDocument(data).toJson(QJsonDocument::Compact));
    return {listing.value->status.cancelled ? ToolExecutionStatus::Cancelled : ToolExecutionStatus::Succeeded,
            summary,
        std::make_shared<StructuredObservation>(StructuredObservation{StructuredObservationKind::DirectoryListing, data})};
}
ToolExecutionResult RealToolExecutor::executeReadFile(const PlannedToolInvocation& invocation, QString& cwd) const {
    const auto path = authorizedFilePath(fileSystemService_, invocation, QStringLiteral("path"), AccessMode::Read, cwd);
    if (!path.ok()) return fileToolFailure(QStringLiteral("read-file"), path);
    const auto read = fileSystemService_.readFile(*path.value, 64 * 1024);
    if (!read.ok()) return fileToolFailure(QStringLiteral("read-file"), read);
    if (read.value->binary) {
        FileSystemResult<FileRead> rejected;
        rejected.resource = read.resource;
        rejected.failure = FileSystemFailure::ReadFailed;
        rejected.diagnostic = QStringLiteral("Cannot read binary file: %1").arg(read.resource);
        return fileToolFailure(QStringLiteral("read-file"), rejected);
    }
    const int offset = getIntArgument(invocation, QStringLiteral("offset"), 1);
    const int limit = getIntArgument(invocation, QStringLiteral("limit"), kDefaultReadLimit);
    const auto lines = QString::fromUtf8(read.value->content).split(QLatin1Char('\n'));
    QStringList output;
    for (int i = qMax(0, offset - 1); i < lines.size() && output.size() < limit; ++i)
        output.append(QStringLiteral("%1: %2").arg(i + 1).arg(lines.at(i).left(kMaxLineLength)));
    const QJsonObject data{{QStringLiteral("path"), read.resource},
        {QStringLiteral("content"), QString::fromUtf8(read.value->content)},
        {QStringLiteral("complete"), read.value->complete && offset == 1},
        {QStringLiteral("truncated"), read.value->truncated || offset != 1}};
    return {ToolExecutionStatus::Succeeded, output.join(QLatin1Char('\n')),
        std::make_shared<StructuredObservation>(StructuredObservation{StructuredObservationKind::FileContent, data})};
}
ToolExecutionResult RealToolExecutor::executeWriteFile(const PlannedToolInvocation& invocation, QString& cwd) const {
    const auto path = authorizedFilePath(fileSystemService_, invocation, QStringLiteral("path"), AccessMode::Write, cwd);
    if (!path.ok()) return fileToolFailure(QStringLiteral("write-file"), path);
    const auto written = fileSystemService_.writeFile(*path.value, getArgument(invocation, QStringLiteral("content")).toUtf8());
    if (!written.ok()) return fileToolFailure(QStringLiteral("write-file"), written);
    return {ToolExecutionStatus::Succeeded,
        QStringLiteral("write-file: Wrote %1 bytes to '%2'.").arg(written.value->bytesWritten).arg(written.resource),
        {}, written.mutations};
}
ToolExecutionResult RealToolExecutor::executeEditFile(const PlannedToolInvocation& invocation, QString& cwd) const {
    const auto path = authorizedFilePath(fileSystemService_, invocation, QStringLiteral("path"), AccessMode::Write, cwd);
    if (!path.ok()) return fileToolFailure(QStringLiteral("edit-file"), path);
    const auto oldText = getArgument(invocation, QStringLiteral("oldString"));
    const auto newText = getArgument(invocation, QStringLiteral("newString"));
    const auto stat = fileSystemService_.stat(*path.value);
    if (!stat.ok() && !(stat.failure == FileSystemFailure::NotFound && oldText.trimmed().isEmpty()))
        return fileToolFailure(QStringLiteral("edit-file"), stat);
    if (stat.ok() && stat.value->directory) {
        FileSystemResult<bool> rejected;
        rejected.resource = path.value->canonicalPath;
        rejected.failure = FileSystemFailure::NotFile;
        return fileToolFailure(QStringLiteral("edit-file"), rejected);
    }
    if (!stat.ok()) {
        const auto written = fileSystemService_.writeFile(*path.value, newText.toUtf8(), true);
        if (!written.ok()) return fileToolFailure(QStringLiteral("edit-file"), written);
        return {ToolExecutionStatus::Succeeded,
            QStringLiteral("edit-file: Created '%1'.").arg(written.resource), {}, written.mutations};
    }
    if (oldText == newText)
        return {ToolExecutionStatus::Failed, QStringLiteral("edit-file: oldString and newString are identical.")};
    const auto read = fileSystemService_.readFile(*path.value, 16 * 1024 * 1024);
    if (!read.ok()) return fileToolFailure(QStringLiteral("edit-file"), read);
    if (!read.value->complete) {
        FileSystemResult<bool> rejected;
        rejected.resource = path.value->canonicalPath;
        rejected.failure = FileSystemFailure::ReadFailed;
        rejected.diagnostic = QStringLiteral("File is too large to edit safely");
        return fileToolFailure(QStringLiteral("edit-file"), rejected);
    }
    if (read.value->content.isEmpty()) {
        FileSystemResult<bool> rejected;
        rejected.resource = path.value->canonicalPath;
        rejected.failure = FileSystemFailure::ReadFailed;
        rejected.diagnostic = QStringLiteral("Cannot edit an empty file with the current edit contract");
        return fileToolFailure(QStringLiteral("edit-file"), rejected);
    }
    FuzzyEditRequest request;
    request.filePath = path.value->canonicalPath;
    request.oldString = oldText;
    request.newString = newText;
    request.replaceAll = getArgument(invocation, QStringLiteral("replaceAll")) == QLatin1String("true");
    QString output;
    const auto edited = FuzzyEditor{}.transform(QString::fromUtf8(read.value->content), request, output);
    if (!edited.success) {
        FileSystemResult<bool> rejected;
        rejected.resource = path.value->canonicalPath;
        rejected.failure = FileSystemFailure::WriteFailed;
        rejected.diagnostic = edited.error;
        return fileToolFailure(QStringLiteral("edit-file"), rejected);
    }
    const auto written = fileSystemService_.writeFile(*path.value, output.toUtf8());
    if (!written.ok()) return fileToolFailure(QStringLiteral("edit-file"), written);
    return {ToolExecutionStatus::Succeeded,
        QStringLiteral("edit-file: Edited %1 line(s) in '%2'.").arg(edited.linesChanged).arg(path.value->canonicalPath),
        {}, written.mutations};
}
ToolExecutionResult RealToolExecutor::executeDeleteFile(const PlannedToolInvocation& invocation, QString& cwd) const {
    const auto path = authorizedFilePath(fileSystemService_, invocation, QStringLiteral("path"), AccessMode::Delete, cwd);
    if (!path.ok()) return fileToolFailure(QStringLiteral("delete-file"), path);
    const auto deleted = fileSystemService_.deleteFile(*path.value);
    if (!deleted.ok()) return fileToolFailure(QStringLiteral("delete-file"), deleted);
    return {ToolExecutionStatus::Succeeded, QStringLiteral("delete-file: Deleted '%1'.").arg(deleted.resource), {}, deleted.mutations};
}
ToolExecutionResult RealToolExecutor::executeMoveFile(const PlannedToolInvocation& invocation, QString& cwd) const {
    const auto source = authorizedFilePath(fileSystemService_, invocation, QStringLiteral("source"), AccessMode::Delete, cwd);
    if (!source.ok()) return fileToolFailure(QStringLiteral("move-file"), source);
    const auto destination = authorizedFilePath(fileSystemService_, invocation, QStringLiteral("destination"), AccessMode::Write, cwd);
    if (!destination.ok()) return fileToolFailure(QStringLiteral("move-file"), destination);
    const auto moved = fileSystemService_.moveFile(*source.value, *destination.value);
    if (!moved.ok()) return fileToolFailure(QStringLiteral("move-file"), moved);
    return {ToolExecutionStatus::Succeeded, QStringLiteral("move-file: Moved '%1' to '%2'.").arg(moved.value->source, moved.value->destination),
        {}, moved.mutations};
}
ToolExecutionResult RealToolExecutor::executeApplyPatch(const PlannedToolInvocation& invocation, QString& cwd) const {
    const QString patch = getArgument(invocation, QStringLiteral("patch"));
    if (patch.trimmed().isEmpty())
        return {ToolExecutionStatus::InvalidArguments, QStringLiteral("apply-patch: No patch argument provided.")};
    return applyPatchWithFileSystem(patch, cwd, fileSystemService_, operationContext(invocation),
                                    invocation.resourceSnapshot.get());
}

ToolExecutionResult RealToolExecutor::executeListCodeDefinitions(const PlannedToolInvocation& invocation,
                                                                 QString& cwd) const {
    const auto path = authorizedFilePath(fileSystemService_, invocation, QStringLiteral("path"), AccessMode::Read, cwd);
    if (!path.ok()) return fileToolFailure(QStringLiteral("list-code-definitions"), path);
    const auto read = fileSystemService_.readFile(*path.value, 2 * 1024 * 1024);
    if (!read.ok()) return fileToolFailure(QStringLiteral("list-code-definitions"), read);
    bool cancelled = false;
    bool truncated = read.value->truncated;
    const auto definitions = extractCodeDefinitions(read.resource, read.value->content,
                                                     operationContext(invocation), cancelled, truncated);
    const bool complete = read.value->complete && !cancelled && !truncated;
    const QJsonObject data{{QStringLiteral("path"), read.resource},
        {QStringLiteral("definitions"), QJsonArray::fromStringList(definitions)},
        {QStringLiteral("complete"), complete}, {QStringLiteral("truncated"), truncated},
        {QStringLiteral("cancelled"), cancelled}};
    const QString summary = cancelled
        ? QStringLiteral("Definition scan stopped after %1 result(s); file not fully inspected.\n%2")
              .arg(definitions.size()).arg(definitions.join(QLatin1Char('\n')))
        : truncated
            ? QStringLiteral("Definition scan returned %1 result(s); file not fully inspected.\n%2")
                  .arg(definitions.size()).arg(definitions.join(QLatin1Char('\n')))
        : definitions.isEmpty()
            ? QStringLiteral("list-code-definitions: No definitions found in '%1'.").arg(read.resource)
            : QStringLiteral("list-code-definitions: %1 definition(s) in '%2':\n%3")
                  .arg(definitions.size()).arg(read.resource, definitions.join(QLatin1Char('\n')));
    return {cancelled ? ToolExecutionStatus::Cancelled : ToolExecutionStatus::Succeeded,
            summary,
            std::make_shared<StructuredObservation>(StructuredObservation{StructuredObservationKind::CodeDefinitions, data})};
}

ToolExecutionResult RealToolExecutor::executeGrep(const PlannedToolInvocation& invocation, QString& cwd) const {
    const QString pattern = getArgument(invocation, QStringLiteral("pattern"));
    if (pattern.trimmed().isEmpty())
        return {ToolExecutionStatus::InvalidArguments, QStringLiteral("grep: No pattern argument provided.")};
    const QRegularExpression regex(pattern);
    if (!regex.isValid())
        return {ToolExecutionStatus::InvalidArguments,
                QStringLiteral("grep: Invalid regular expression: %1").arg(regex.errorString())};
    const auto path = authorizedFilePath(fileSystemService_, invocation, QStringLiteral("path"), AccessMode::Read, cwd);
    if (!path.ok()) return fileToolFailure(QStringLiteral("grep"), path);
    const auto context = operationContext(invocation);
    const bool includeHidden = getArgument(invocation, QStringLiteral("includeHidden")) == QLatin1String("true");
    const QString include = getArgument(invocation, QStringLiteral("include")).trimmed();
    int count = 0;
    QStringList output;
    QJsonArray readIssues;
    bool filesComplete = true;
    bool outputCapped = false;
    bool cancelled = false;
    auto onFile = [&](const FileSystemEntry& entry) {
        if (context.isCancelled()) { cancelled = true; return false; }
        if (!include.isEmpty() && !QDir::match(include, entry.name)) return true;
        auto issue = [&](FileSystemFailure failure) {
            filesComplete = false;
            if (readIssues.size() < 16)
                readIssues.append(QJsonObject{{QStringLiteral("resource"), entry.path},
                    {QStringLiteral("failure"), static_cast<int>(failure)}});
        };
        if (entry.size > 2 * 1024 * 1024) { issue(FileSystemFailure::Unavailable); return true; }
        const auto child = fileSystemService_.resolve(entry.path, cwd, FileSystemAccess::Read);
        if (!child.ok()) { issue(child.failure); return true; }
        const auto read = fileSystemService_.readFile(*child.value, 2 * 1024 * 1024);
        if (!read.ok() || !read.value->complete || read.value->binary) {
            issue(read.ok() ? FileSystemFailure::Unavailable : read.failure);
            return true;
        }
        const auto lines = QString::fromUtf8(read.value->content).split(QLatin1Char('\n'));
        for (int i = 0; i < lines.size(); ++i) {
            if ((i & 127) == 0 && context.isCancelled()) { cancelled = true; return false; }
            if (!regex.match(lines.at(i)).hasMatch()) continue;
            if (count >= kGrepGlobLimit) { outputCapped = true; return false; }
            output.append(QStringLiteral("%1:%2: %3").arg(entry.path).arg(i + 1)
                          .arg(lines.at(i).left(kMaxLineLength)));
            ++count;
        }
        return true;
    };
    const auto tree = fileSystemService_.traverseFiles(*path.value, includeHidden, 5000,
        [this, &cwd](const QString& child) { return !resolveToolPath(cwd, child).isEmpty(); },
        context, onFile);
    if (!tree.ok()) return fileToolFailure(QStringLiteral("grep"), tree);
    cancelled = cancelled || tree.value->status.cancelled;
    const bool truncated = outputCapped || tree.value->status.truncated;
    const bool complete = tree.value->status.complete && filesComplete && !cancelled && !outputCapped;
    QJsonArray issues = traversalIssues(tree.value->status);
    for (const auto& issue : readIssues)
        if (issues.size() < 16) issues.append(issue);
    const QJsonObject data{{QStringLiteral("scope"), path.value->canonicalPath},
        {QStringLiteral("query"), pattern}, {QStringLiteral("matchCount"), count},
        {QStringLiteral("recursive"), true}, {QStringLiteral("includeHidden"), includeHidden},
        {QStringLiteral("maxDepth"), 128}, {QStringLiteral("followSymlinks"), false},
        {QStringLiteral("complete"), complete}, {QStringLiteral("truncated"), truncated},
        {QStringLiteral("cancelled"), cancelled}, {QStringLiteral("issues"), issues}};
    const QString summary = cancelled
        ? QStringLiteral("Search stopped before the full scope was inspected. %1 match(es) found.\n%2")
              .arg(count).arg(output.join(QLatin1Char('\n')))
        : output.isEmpty()
            ? QStringLiteral("grep: No matches found for '%1' under '%2'.").arg(pattern, path.value->canonicalPath)
            : output.join(QLatin1Char('\n'));
    return {cancelled ? ToolExecutionStatus::Cancelled : ToolExecutionStatus::Succeeded,
            summary,
            std::make_shared<StructuredObservation>(StructuredObservation{StructuredObservationKind::TextSearch, data})};
}
ToolExecutionResult RealToolExecutor::executeGlob(const PlannedToolInvocation& invocation, QString& cwd) const {
    const QString pattern = getArgument(invocation, QStringLiteral("pattern"));
    if (pattern.trimmed().isEmpty())
        return {ToolExecutionStatus::InvalidArguments, QStringLiteral("glob: No pattern argument provided.")};
    const auto path = authorizedFilePath(fileSystemService_, invocation, QStringLiteral("path"), AccessMode::Read, cwd);
    if (!path.ok()) return fileToolFailure(QStringLiteral("glob"), path);
    const bool includeHidden = getArgument(invocation, QStringLiteral("includeHidden")) == QLatin1String("true");
    const auto tree = fileSystemService_.traverseFiles(*path.value, includeHidden, 5000,
        [this, &cwd](const QString& child) { return !resolveToolPath(cwd, child).isEmpty(); },
        operationContext(invocation));
    if (!tree.ok()) return fileToolFailure(QStringLiteral("glob"), tree);
    QJsonArray matches;
    QStringList output;
    bool truncated = tree.value->status.truncated;
    bool cancelled = tree.value->status.cancelled;
    const auto context = operationContext(invocation);
    for (const auto& entry : tree.value->files) {
        // Matching already inspected paths is cheap and preserves positive partial evidence.
        if (context.isCancelled() && !tree.value->status.cancelled) { cancelled = true; break; }
        if (!QDir::match(pattern, entry.name) && !QDir::match(pattern, entry.path)) continue;
        if (matches.size() >= kGrepGlobLimit) { truncated = true; break; }
        matches.append(entry.path);
        output.append(entry.path);
    }
    const bool complete = tree.value->status.complete && !cancelled;
    const QJsonObject data{{QStringLiteral("root"), path.value->canonicalPath},
        {QStringLiteral("pattern"), pattern}, {QStringLiteral("matches"), matches},
        {QStringLiteral("recursive"), true}, {QStringLiteral("includeHidden"), includeHidden},
        {QStringLiteral("maxDepth"), 128}, {QStringLiteral("followSymlinks"), false},
        {QStringLiteral("complete"), complete}, {QStringLiteral("truncated"), truncated},
        {QStringLiteral("cancelled"), cancelled}, {QStringLiteral("issues"), traversalIssues(tree.value->status)}};
    return {cancelled ? ToolExecutionStatus::Cancelled : ToolExecutionStatus::Succeeded,
        cancelled ? QStringLiteral("Search stopped before the full scope was inspected. %1 match(es) found.\n%2")
                        .arg(matches.size()).arg(output.join(QLatin1Char('\n'))) : output.isEmpty() ? QStringLiteral("glob: No files matching '%1' under '%2'.").arg(pattern, path.value->canonicalPath)
                         : output.join(QLatin1Char('\n')),
        std::make_shared<StructuredObservation>(StructuredObservation{StructuredObservationKind::PathMatches, data})};
}

ToolExecutionResult RealToolExecutor::executeRunCommand(const PlannedToolInvocation& invocation,
                                                        QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString command = getArgument(invocation, QStringLiteral("command"));
        if (command.isEmpty()) {
            logs.append(QStringLiteral("run-command: No command argument provided."));
            continue;
        }
        int timeoutMs = getIntArgument(invocation, QStringLiteral("timeout"), 60000);
        timeoutMs = qBound(1000, timeoutMs, 600000);
        const QString workdirArg = getArgument(invocation, QStringLiteral("workdir")).trimmed();
        QString workingDirectory = currentWorkingDirectory;
        if (!workdirArg.isEmpty()) {
            const QString scopedWorkdir = resolveToolPath(currentWorkingDirectory, workdirArg);
            if (scopedWorkdir.isEmpty() || !QDir(scopedWorkdir).exists()) {
                logs.append(
                    QStringLiteral("run-command: workdir is outside the approved workspace."));
                continue;
            }
            workingDirectory = scopedWorkdir;
        }

        const QString sandbox =
            getArgument(invocation, QStringLiteral("sandbox")).trimmed().toLower();
        if (sandbox == QStringLiteral("docker")) {
            logs.append(runCommandInDocker(command, workingDirectory, timeoutMs));
            continue;
        }

        QProcess process;
        process.setWorkingDirectory(workingDirectory);

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString pathEnv = env.value(QStringLiteral("PATH"));
#if defined(Q_OS_MACOS)
        if (!pathEnv.contains(QStringLiteral("/opt/homebrew/bin"))) {
            pathEnv = QStringLiteral("/opt/homebrew/bin:/usr/local/bin:") + pathEnv;
        }
#elif defined(Q_OS_UNIX)
        if (!pathEnv.contains(QStringLiteral("/usr/local/bin"))) {
            pathEnv = QStringLiteral("/usr/local/bin:") + pathEnv;
        }
#endif
        env.insert(QStringLiteral("PATH"), pathEnv);
        process.setProcessEnvironment(env);

#if defined(Q_OS_WIN)
        process.start(QStringLiteral("cmd.exe"), QStringList{QStringLiteral("/c"), command});
#else
        process.start(QStringLiteral("/bin/sh"), QStringList{QStringLiteral("-c"), command});
#endif

        if (!process.waitForFinished(timeoutMs)) {
            const bool started = process.state() != QProcess::NotRunning;
            process.kill();
            process.waitForFinished(3000);
            if (started) {
                logs.append(
                    QStringLiteral(
                        "run-command: Shell tool terminated command after exceeding timeout "
                        "%1 ms. If this command is expected to take longer, retry it with a "
                        "larger timeout value in milliseconds.\nCommand: %2")
                        .arg(QString::number(timeoutMs), command));
            } else {
                logs.append(QStringLiteral("run-command: Failed to start: %1").arg(command));
            }
            continue;
        }
        const QString stdoutContent = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        const QString stderrContent = QString::fromUtf8(process.readAllStandardError()).trimmed();
        const int exitCode = process.exitCode();
        if (exitCode == 0) {
            if (stdoutContent.isEmpty() && stderrContent.isEmpty()) {
                logs.append(QStringLiteral("Command executed successfully. (exit=0)"));
            } else {
                logs.append(
                    QStringLiteral("Command executed successfully. (exit=0)\n\n[STDOUT]:\n%1")
                        .arg(stdoutContent));
                if (!stderrContent.isEmpty()) {
                    logs.append(QStringLiteral("\n\n[STDERR]:\n%1").arg(stderrContent));
                }
            }
        } else {
            logs.append(QStringLiteral(
                            "Command execution failed. (exit=%1)\n\n[STDOUT]:\n%2\n\n[STDERR]:\n%3")
                            .arg(QString::number(exitCode), stdoutContent, stderrContent));
        }

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeAppLaunch(const PlannedToolInvocation& invocation,
                                                       QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString app = getArgument(invocation, QStringLiteral("app")).trimmed();
        if (app.isEmpty()) {
            logs.append(QStringLiteral("app-launch: No app argument provided."));
            continue;
        }
        // Safety net: domains are websites, not applications. Guide the
        // caller to open-url instead of trying to launch a browser as an
        // app.
        if (looksLikeDomainName(app)) {
            logs.append(QStringLiteral("app-launch: '%1' is a website address, not an application. "
                                       "Retry with the open-url tool and url=%1 to open it in the "
                                       "browser.")
                            .arg(app));
            continue;
        }
        const QString extraArgs = getArgument(invocation, QStringLiteral("args")).trimmed();

        QString error;
#if defined(Q_OS_MACOS)
        QStringList args{QStringLiteral("-a"), app};
        if (!extraArgs.isEmpty()) {
            args.append(extraArgs);
        }
        runSynchronousProcess(QStringLiteral("open"), args, QString(), 15000, &error);
#elif defined(Q_OS_WIN)
        QStringList args{QStringLiteral("/c"), QStringLiteral("start"), QStringLiteral(""), app};
        if (!extraArgs.isEmpty()) {
            args.append(extraArgs);
        }
        runSynchronousProcess(QStringLiteral("cmd.exe"), args, QString(), 15000, &error);
#else
        // Linux/BSD: resolve the application through its freedesktop
        // .desktop entry so names like "Files" launch org.kde.dolphin.
        const auto entry = findDesktopEntry(app);
        if (entry) {
            QString command = desktopExecToCommand(entry->exec);
            QStringList args;
            if (!extraArgs.isEmpty()) {
                args.append(extraArgs);
            }
            if (command.isEmpty()) {
                command = entry->desktopId;
            }
            const bool started = QProcess::startDetached(command, args);
            logs.append(
                started ? QStringLiteral("app-launch: Launched '%1' (%2) via desktop entry.")
                              .arg(app, entry->desktopId)
                        : QStringLiteral("app-launch: Found desktop entry '%1' but failed to start "
                                         "'%2'.")
                              .arg(entry->desktopId, command));
            continue;
        }
        QProcess::startDetached(app, extraArgs.isEmpty() ? QStringList{} : QStringList{extraArgs});
#endif
        logs.append(error.isEmpty()
                        ? QStringLiteral("app-launch: Launch requested for '%1'.").arg(app)
                        : QStringLiteral("app-launch: %1 (%2)").arg(error, app));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeAppQuit(const PlannedToolInvocation& invocation,
                                                     QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString app = getArgument(invocation, QStringLiteral("app")).trimmed();
        if (app.isEmpty()) {
            logs.append(QStringLiteral("app-quit: No app argument provided."));
            continue;
        }

        QString error;
#if defined(Q_OS_MACOS)
        runSynchronousProcess(
            QStringLiteral("osascript"),
            QStringList{QStringLiteral("-e"),
                        QStringLiteral("quit app \"%1\"").arg(osascriptEscape(app))},
            QString(), 15000, &error);
#elif defined(Q_OS_WIN)
        runSynchronousProcess(QStringLiteral("taskkill"),
                              QStringList{QStringLiteral("/IM"),
                                          app.endsWith(QStringLiteral(".exe"), Qt::CaseInsensitive)
                                              ? app
                                              : app + QStringLiteral(".exe"),
                                          QStringLiteral("/F")},
                              QString(), 15000, &error);
#else
        runSynchronousProcess(QStringLiteral("pkill"),
                              QStringList{QStringLiteral("-f"), QStringLiteral("-i"), app},
                              QString(), 15000, &error);
#endif
        logs.append(error.isEmpty() ? QStringLiteral("app-quit: Quit requested for '%1'.").arg(app)
                                    : QStringLiteral("app-quit: %1 (%2)").arg(error, app));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeOpenUrl(const PlannedToolInvocation& invocation,
                                                     QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString url = getArgument(invocation, QStringLiteral("url")).trimmed();
        if (url.isEmpty()) {
            logs.append(QStringLiteral("open-url: No url argument provided."));
            continue;
        }
        const QUrl parsed = QUrl::fromUserInput(url);
        if (!parsed.isValid() || (parsed.scheme() != QStringLiteral("http") &&
                                  parsed.scheme() != QStringLiteral("https"))) {
            logs.append(QStringLiteral("open-url: Only http and https URLs can be opened. Got: %1")
                            .arg(url));
            continue;
        }
        const QString target = parsed.toString(QUrl::FullyEncoded);
        bool launched = false;
#if defined(Q_OS_MACOS)
        launched = QProcess::startDetached(QStringLiteral("open"), {target});
#elif defined(Q_OS_WIN)
        launched = QProcess::startDetached(
            QStringLiteral("cmd.exe"),
            {QStringLiteral("/c"), QStringLiteral("start"), QString(), target});
#else
        launched = QProcess::startDetached(QStringLiteral("xdg-open"), {target});
#endif
        logs.append(launched ? QStringLiteral("open-url: Opened '%1' in the default browser.")
                                   .arg(parsed.toString())
                             : QStringLiteral("open-url: Failed to open '%1' in the default "
                                              "browser.")
                                   .arg(parsed.toString()));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeSpawnAgent(const PlannedToolInvocation& invocation,
                                                        QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString task = getArgument(invocation, QStringLiteral("task")).trimmed();
        if (task.isEmpty()) {
            logs.append(QStringLiteral("spawn-agent: No task argument provided."));
            continue;
        }
        if (subagentActive_) {
            logs.append(
                QStringLiteral("spawn-agent: subagents cannot spawn further subagents; finish this "
                               "subtask directly."));
            continue;
        }
        if (!subagentRunner_) {
            logs.append(
                QStringLiteral("spawn-agent: No subagent runner is configured in this session."));
            continue;
        }
        subagentActive_ = true;
        const QString answer = subagentRunner_(task);
        subagentActive_ = false;
        logs.append(QStringLiteral("spawn-agent: subagent finished the task '%1'.\n\n%2")
                        .arg(task.left(200), answer));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult
RealToolExecutor::executeBrowserScreenshot(const PlannedToolInvocation& invocation,
                                           QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString url = getArgument(invocation, QStringLiteral("url")).trimmed();
        if (url.isEmpty()) {
            logs.append(QStringLiteral("browser-screenshot: No url argument provided."));
            continue;
        }
        QString path = getArgument(invocation, QStringLiteral("path")).trimmed();
        if (path.isEmpty()) {
            path = QDir(QDir::tempPath())
                       .filePath(QStringLiteral("sentinel_browser_%1.png")
                                     .arg(QDateTime::currentMSecsSinceEpoch()));
        } else {
            const QString scoped = resolveToolPath(currentWorkingDirectory, path);
            if (scoped.isEmpty()) {
                logs.append(
                    QStringLiteral("browser-screenshot: Path is outside the approved workspace."));
                continue;
            }
            path = scoped;
        }
        logs.append(
            runPlaywrightCli(QStringLiteral("screenshot"), url, path, QStringLiteral("2500")));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeBrowserPdf(const PlannedToolInvocation& invocation,
                                                        QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString url = getArgument(invocation, QStringLiteral("url")).trimmed();
        if (url.isEmpty()) {
            logs.append(QStringLiteral("browser-pdf: No url argument provided."));
            continue;
        }
        QString path = getArgument(invocation, QStringLiteral("path")).trimmed();
        if (path.isEmpty()) {
            path = QDir(QDir::tempPath())
                       .filePath(QStringLiteral("sentinel_browser_%1.pdf")
                                     .arg(QDateTime::currentMSecsSinceEpoch()));
        } else {
            const QString scoped = resolveToolPath(currentWorkingDirectory, path);
            if (scoped.isEmpty()) {
                logs.append(QStringLiteral("browser-pdf: Path is outside the approved workspace."));
                continue;
            }
            path = scoped;
        }
        logs.append(runPlaywrightCli(QStringLiteral("pdf"), url, path, QString()));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeSystemNotify(const PlannedToolInvocation& invocation,
                                                          QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString title = getArgument(invocation, QStringLiteral("title")).trimmed();
        const QString message = getArgument(invocation, QStringLiteral("message")).trimmed();
        if (message.isEmpty()) {
            logs.append(QStringLiteral("system-notify: No message argument provided."));
            continue;
        }

        QString error;
#if defined(Q_OS_MACOS)
        runSynchronousProcess(
            QStringLiteral("osascript"),
            QStringList{
                QStringLiteral("-e"),
                QStringLiteral("display notification \"%1\" with title \"%2\"")
                    .arg(osascriptEscape(message),
                         osascriptEscape(title.isEmpty() ? QStringLiteral("Sentinel") : title))},
            QString(), 15000, &error);
#elif defined(Q_OS_WIN)
        // PowerShell toast via the Windows Runtime projection; works on
        // Windows 10/11 without any extra dependencies.
        QString escapedTitle = title.isEmpty() ? QStringLiteral("Sentinel") : title;
        escapedTitle.replace(QLatin1Char('\''), QStringLiteral("''"));
        QString cleanMessage = message;
        cleanMessage.replace(QLatin1Char('\''), QStringLiteral("''"));
        const QString psScript = QStringLiteral(
            "[Windows.UI.Notifications.ToastNotificationManager, Windows.UI.Notifications, "
            "ContentType = WindowsRuntime] | Out-Null;"
            "[Windows.Data.Xml.Dom.XmlDocument, Windows.Data.Xml.Dom.XmlDocument, "
            "ContentType = WindowsRuntime] | Out-Null;"
            "$template = "
            "'<toast><visual><binding template=\"ToastGeneric\">"
            "<text id=\"1\">%1</text><text id=\"2\">%2</text></binding></visual></toast>';"
            "$xml = New-Object Windows.Data.Xml.Dom.XmlDocument;"
            "$xml.LoadXml($template);"
            "$toast = New-Object Windows.UI.Notifications.ToastNotification($xml);"
            "[Windows.UI.Notifications.ToastNotificationManager]::CreateToastNotifier("
            "'Sentinel').Show($toast);");
        runSynchronousProcess(
            QStringLiteral("powershell"),
            QStringList{QStringLiteral("-NoProfile"), QStringLiteral("-NonInteractive"),
                        QStringLiteral("-Command"), psScript.arg(escapedTitle, cleanMessage)},
            QString(), 15000, &error);
#else
        runSynchronousProcess(
            QStringLiteral("notify-send"),
            QStringList{title.isEmpty() ? QStringLiteral("Sentinel") : title, message}, QString(),
            15000, &error);
#endif
        logs.append(error.isEmpty()
                        ? QStringLiteral("system-notify: Notification sent: %1").arg(message)
                        : QStringLiteral("system-notify: %1").arg(error));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeClipboardRead(const PlannedToolInvocation& invocation,
                                                           QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        QClipboard* clipboard = activeClipboard();
        if (!clipboard) {
            logs.append(
                QStringLiteral("clipboard-read: Clipboard is unavailable without a GUI session."));
            continue;
        }
        const QString text = clipboard->text();
        if (text.isEmpty()) {
            logs.append(QStringLiteral("clipboard-read: Clipboard is empty."));
        } else {
            QString preview = text;
            if (preview.size() > kWebFetchPreviewChars) {
                preview = preview.left(kWebFetchPreviewChars) +
                          QStringLiteral("\n... (content truncated at %1 characters)")
                              .arg(QString::number(kWebFetchPreviewChars));
            }
            logs.append(QStringLiteral("clipboard-read: (%1 characters)\n%2")
                            .arg(QString::number(text.size()), preview));
        }

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult
RealToolExecutor::executeClipboardWrite(const PlannedToolInvocation& invocation,
                                        QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString text = getArgument(invocation, QStringLiteral("text"));
        if (text.isEmpty()) {
            logs.append(QStringLiteral("clipboard-write: No text argument provided."));
            continue;
        }
        QClipboard* clipboard = activeClipboard();
        if (!clipboard) {
            logs.append(
                QStringLiteral("clipboard-write: Clipboard is unavailable without a GUI session."));
            continue;
        }
        clipboard->setText(text);
        logs.append(QStringLiteral("clipboard-write: Copied %1 character(s) to the clipboard.")
                        .arg(QString::number(text.size())));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeSystemInfo(const PlannedToolInvocation& invocation,
                                                        QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        logs.append(QStringLiteral("system-info:\n%1").arg(systemInfoReport()));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeProcessList(const PlannedToolInvocation& invocation,
                                                         QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        logs.append(processListReport());

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeCurrentTime(const PlannedToolInvocation& invocation,
                                                         QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QDateTime now = QDateTime::currentDateTime();
        logs.append(QStringLiteral("current-time: %1\nUTC: %2\nEpoch seconds: %3")
                        .arg(now.toString(QStringLiteral("dddd, yyyy-MM-dd HH:mm:ss t")),
                             now.toUTC().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss 'UTC'")),
                             QString::number(now.toSecsSinceEpoch())));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeSetAlarm(const PlannedToolInvocation& invocation,
                                                      QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        if (!alarmStore_) {
            logs.append(QStringLiteral("set-alarm: No alarm store is configured."));
            continue;
        }
        const QString rawTime = getArgument(invocation, QStringLiteral("time"));
        const QString label = getArgument(invocation, QStringLiteral("label")).trimmed();
        const auto triggerAt = parseAlarmTime(rawTime);
        if (!triggerAt.isValid()) {
            logs.append(
                QStringLiteral("set-alarm: Could not parse time '%1'. Use HH:mm, HH:mm:ss, or an "
                               "ISO datetime (yyyy-MM-ddTHH:mm).")
                    .arg(rawTime));
            continue;
        }
        const auto entry =
            alarmStore_->schedule(triggerAt, label.isEmpty() ? QStringLiteral("Alarm") : label);
        logs.append(QStringLiteral("set-alarm: Alarm scheduled at %1 (id: %2) - %3")
                        .arg(triggerAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")), entry.id,
                             entry.label));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeListAlarms(const PlannedToolInvocation& invocation,
                                                        QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        if (!alarmStore_) {
            logs.append(QStringLiteral("list-alarms: No alarm store is configured."));
            continue;
        }
        const auto alarms = alarmStore_->active();
        if (alarms.isEmpty()) {
            logs.append(QStringLiteral("list-alarms: No active alarms."));
        } else {
            QStringList lines;
            for (const auto& alarm : alarms) {
                lines.append(
                    QStringLiteral("%1 - %2 - %3")
                        .arg(alarm.triggerAt.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss")),
                             alarm.label, alarm.id));
            }
            logs.append(QStringLiteral("list-alarms: %1 active alarm(s):\n%2")
                            .arg(QString::number(alarms.size()), lines.join(QLatin1Char('\n'))));
        }

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeCancelAlarm(const PlannedToolInvocation& invocation,
                                                         QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        if (!alarmStore_) {
            logs.append(QStringLiteral("cancel-alarm: No alarm store is configured."));
            continue;
        }
        const QString id = getArgument(invocation, QStringLiteral("id")).trimmed();
        if (id.isEmpty()) {
            logs.append(QStringLiteral("cancel-alarm: No id argument provided."));
            continue;
        }
        logs.append(alarmStore_->remove(id)
                        ? QStringLiteral("cancel-alarm: Alarm %1 cancelled.").arg(id)
                        : QStringLiteral("cancel-alarm: No active alarm with id %1.").arg(id));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeTodoWrite(const PlannedToolInvocation& invocation,
                                                       QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString todosRaw = getArgument(invocation, QStringLiteral("todos"));
        const auto document = QJsonDocument::fromJson(todosRaw.toUtf8());
        if (!document.isArray()) {
            logs.append(
                QStringLiteral("todo-write: The 'todos' argument must be a JSON array of "
                               "{content, status, priority} objects. Please rewrite the input."));
            continue;
        }

        QJsonArray cleaned;
        const auto array = document.array();
        for (const auto& value : array) {
            const auto object = value.toObject();
            const QString status = object.value(QStringLiteral("status")).toString();
            if (status != QStringLiteral("pending") && status != QStringLiteral("in_progress") &&
                status != QStringLiteral("completed") && status != QStringLiteral("cancelled")) {
                logs.append(QStringLiteral(
                                "todo-write: Invalid status '%1'. Allowed: pending, in_progress, "
                                "completed, cancelled. Please rewrite the input.")
                                .arg(status));
                continue;
            }
            QJsonObject fixed;
            fixed.insert(QStringLiteral("content"),
                         object.value(QStringLiteral("content")).toString());
            fixed.insert(QStringLiteral("status"), status);
            fixed.insert(
                QStringLiteral("priority"),
                object.value(QStringLiteral("priority")).toString(QStringLiteral("medium")));
            cleaned.append(fixed);
        }
        todos_ = cleaned;

        int pending = 0;
        for (const auto& value : todos_) {
            if (value.toObject().value(QStringLiteral("status")).toString() ==
                QStringLiteral("pending")) {
                ++pending;
            }
        }
        logs.append(
            QStringLiteral("todo-write: %1 todo(s) saved (%2 pending). Current list:\n%3")
                .arg(QString::number(todos_.size()), QString::number(pending),
                     QString::fromUtf8(QJsonDocument(todos_).toJson(QJsonDocument::Compact))));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeTodoRead(const PlannedToolInvocation& invocation,
                                                      QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        logs.append(todos_.isEmpty()
                        ? QStringLiteral("todo-read: No todos recorded for this session yet.")
                        : QStringLiteral("todo-read: Current todos:\n%1")
                              .arg(QString::fromUtf8(
                                  QJsonDocument(todos_).toJson(QJsonDocument::Compact))));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeMemorySearch(const PlannedToolInvocation& invocation,
                                                          QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString query = getArgument(invocation, QStringLiteral("query")).trimmed();
        if (query.isEmpty()) {
            logs.append(QStringLiteral("memory-search: No query argument provided."));
            continue;
        }
        if (!memoryStore_ || !memoryStore_->isAvailable()) {
            logs.append(
                QStringLiteral("memory-search: No memory entries are available for this session."));
            continue;
        }
        const int limit = qBound(1, getIntArgument(invocation, QStringLiteral("limit"), 10), 50);
        const auto entries = memoryStore_->searchRelevant(query, limit);
        QStringList matches;
        for (const auto& entry : entries) {
            QString value = entry.second.simplified();
            if (value.size() > 400)
                value = value.left(400) + QStringLiteral("...");
            matches.append(QStringLiteral("%1: %2").arg(entry.first, value));
        }

        if (matches.isEmpty()) {
            logs.append(QStringLiteral("memory-search: No memory entries match '%1'.").arg(query));
        } else {
            logs.append(QStringLiteral("memory-search: showing up to %1 match(es) for '%2':\n%3")
                            .arg(QString::number(limit), query, matches.join(QLatin1Char('\n'))));
        }

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeHistorySearch(const PlannedToolInvocation& invocation,
                                                           QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString query = getArgument(invocation, QStringLiteral("query")).trimmed();
        if (query.isEmpty()) {
            logs.append(QStringLiteral("history-search: No query argument provided."));
            continue;
        }
        if (!chatHistoryStore_ || !chatHistoryStore_->isAvailable()) {
            logs.append(
                QStringLiteral("history-search: No chat history is available for this session."));
            continue;
        }
        const int limit = qBound(1, getIntArgument(invocation, QStringLiteral("limit"), 10), 50);
        const auto messages = chatHistoryStore_->searchMessages(query, limit);
        QStringList matches;
        for (const auto& message : messages) {
            QString preview = QStringLiteral("[%1] %2")
                                  .arg(chatRoleName(message.role), message.content.simplified());
            if (preview.size() > 400)
                preview = preview.left(400) + QStringLiteral("...");
            matches.append(preview);
        }

        if (matches.isEmpty()) {
            logs.append(
                QStringLiteral("history-search: No history entries match '%1'.").arg(query));
        } else {
            logs.append(QStringLiteral("history-search: showing up to %1 match(es) for '%2':\n%3")
                            .arg(QString::number(limit), query, matches.join(QLatin1Char('\n'))));
        }

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeAskQuestion(const PlannedToolInvocation& invocation,
                                                         QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString question = getArgument(invocation, QStringLiteral("question")).trimmed();
        if (question.isEmpty()) {
            logs.append(QStringLiteral("ask-question: No question argument provided."));
            continue;
        }
        const QString optionsRaw = getArgument(invocation, QStringLiteral("options"));
        QStringList options;
        for (const auto& option : optionsRaw.split(QLatin1Char('\n'))) {
            const QString trimmed = option.trimmed();
            if (!trimmed.isEmpty()) {
                options.append(trimmed);
            }
        }

        QString formatted = question;
        if (!options.isEmpty()) {
            QStringList numbered;
            for (int i = 0; i < options.size(); ++i) {
                numbered.append(
                    QStringLiteral("%1. %2").arg(QString::number(i + 1), options.at(i)));
            }
            formatted += QStringLiteral("\n") + numbered.join(QLatin1Char('\n'));
        }
        logs.append(
            QStringLiteral("ask-question: Question registered for the user:\n%1\n"
                           "End this run now with the question above as your final answer "
                           "(asked in the user's language). Wait for the user's reply before "
                           "taking further steps.")
                .arg(formatted));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeWebFetch(const PlannedToolInvocation& invocation,
                                                      QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString url = getArgument(invocation, QStringLiteral("url")).trimmed();
        if (url.isEmpty()) {
            logs.append(QStringLiteral("web-fetch: No url argument provided."));
            continue;
        }
        const QString format =
            getArgument(invocation, QStringLiteral("format")).trimmed().toLower();
        const auto fetchFormat =
            format == QStringLiteral("text")
                ? WebFetchFormat::Text
                : (format == QStringLiteral("html") ? WebFetchFormat::Html
                                                    : WebFetchFormat::Markdown);

        const auto response = webFetchTool_.fetch(url, fetchFormat);
        if (!response.success) {
            logs.append(QStringLiteral("web-fetch: Request failed: %1")
                            .arg(response.errorString.isEmpty()
                                     ? QStringLiteral("HTTP %1").arg(response.statusCode)
                                     : response.errorString));
            continue;
        }
        QString content = response.content;
        if (content.size() > kWebFetchPreviewChars) {
            content = content.left(kWebFetchPreviewChars) +
                      QStringLiteral("\n\n... [content truncated at %1 characters] ...")
                          .arg(QString::number(kWebFetchPreviewChars));
        }
        logs.append(QStringLiteral("web-fetch: %1\n\n%2").arg(url, content));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult
RealToolExecutor::executeVoiceTranscribe(const PlannedToolInvocation& invocation,
                                         QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString path = getArgument(invocation, QStringLiteral("path"));
        if (path.isEmpty()) {
            logs.append(QStringLiteral("voice-transcribe: No audio path provided."));
            continue;
        }
        QProcess process;
#if defined(Q_OS_WIN)
        const QString whisperBinary = QStringLiteral("whisper.exe");
#else
        const QString whisperBinary = QStringLiteral("whisper");
#endif
        process.start(whisperBinary, {path});
        if (!process.waitForFinished(15000)) {
            logs.append(QStringLiteral("voice-transcribe: Whisper timed out for '%1'").arg(path));
            continue;
        }
        const QString transcript = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
        logs.append(QStringLiteral("voice-transcribe: OK\n%1").arg(transcript));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeVoiceSpeak(const PlannedToolInvocation& invocation,
                                                        QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString text = getArgument(invocation, QStringLiteral("text"));
        if (text.isEmpty()) {
            logs.append(QStringLiteral("voice-speak: No text argument provided."));
            continue;
        }
        QProcess process;
#if defined(Q_OS_WIN)
        const QString piperBinary = QStringLiteral("piper.exe");
#else
        const QString piperBinary = QStringLiteral("piper");
#endif
        const QString ttsOutput =
            QDir(QDir::tempPath()).filePath(QStringLiteral("sentinel_tts.wav"));
        process.start(piperBinary,
                      {QStringLiteral("--model"), QStringLiteral("en_US-lessac-medium.onnx"),
                       QStringLiteral("--output_file"), ttsOutput});
        process.write(text.toUtf8());
        process.closeWriteChannel();
        if (!process.waitForFinished(10000)) {
            logs.append(QStringLiteral("voice-speak: Piper TTS timed out."));
            continue;
        }
        logs.append(QStringLiteral("voice-speak: TTS synthesis OK → %1").arg(ttsOutput));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeWebSearch(const PlannedToolInvocation& invocation,
                                                       QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const auto query = getArgument(invocation, QStringLiteral("query"));
        if (query.trimmed().isEmpty()) {
            logs.append(QStringLiteral("web-search: No query argument provided."));
            continue;
        }

        const auto response = webSearchTool_.search(query.trimmed());
        if (!response.success) {
            logs.append(QStringLiteral("web-search: Request failed: %1").arg(response.errorString));
            continue;
        }
        for (int i = 0; i < response.results.size(); ++i) {
            const auto& result = response.results.at(i);
            logs.append(QStringLiteral("%1. %2\n%3\n%4")
                            .arg(i + 1)
                            .arg(result.title)
                            .arg(result.url)
                            .arg(result.snippet));
        }
        if (response.results.isEmpty()) {
            logs.append(QStringLiteral("web-search: No results found for '%1'.").arg(query));
        }

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult RealToolExecutor::executeOpenWorkspace(const PlannedToolInvocation& invocation,
                                                           QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        const QString path = getArgument(invocation, QStringLiteral("path")).trimmed();
        if (path.isEmpty()) {
            return {
                ToolExecutionStatus::Blocked,
                QStringLiteral("open-workspace: path argument is required."),
            };
        }
        const auto authorized = authorizedFilePath(fileSystemService_, invocation,
                                                   QStringLiteral("path"), AccessMode::Read,
                                                   currentWorkingDirectory);
        if (!authorized.ok())
            return fileToolFailure(QStringLiteral("open-workspace"), authorized);
        const QString workspacePath = authorized.value->canonicalPath;
        if (workspacePath.isEmpty() || !QDir(workspacePath).exists()) {
            return {
                ToolExecutionStatus::Blocked,
                QStringLiteral("open-workspace: requested workspace does not exist."),
            };
        }
        if (isForbiddenWorkspacePath(workspacePath)) {
            return {
                ToolExecutionStatus::Blocked,
                QStringLiteral(
                    "open-workspace: requested workspace is a system or sensitive path."),
            };
        }
        currentWorkingDirectory = workspacePath;
        logs.append(QStringLiteral("open-workspace: Workspace context set → '%1'")
                        .arg(currentWorkingDirectory));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult
RealToolExecutor::executeSummarizeCurrentConversation(const PlannedToolInvocation& invocation,
                                                      QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        logs.append(QStringLiteral("summarize-current-conversation: Summary compiled."));

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult
RealToolExecutor::executeProviderTestCall(const PlannedToolInvocation& invocation,
                                          QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        return {
            ToolExecutionStatus::Blocked,
            QStringLiteral(
                "provider-test-call is unavailable: no provider test executor is configured."),
        };

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

ToolExecutionResult
RealToolExecutor::executeExportConversation(const PlannedToolInvocation& invocation,
                                            QString& currentWorkingDirectory) const {
    QStringList logs;
    do {
        return {
            ToolExecutionStatus::Blocked,
            QStringLiteral(
                "export-conversation is unavailable: use the conversation export service."),
        };

    } while (false);
    return {ToolExecutionStatus::Succeeded, logs.join(QStringLiteral("\n\n"))};
}

} // namespace sentinel::core
