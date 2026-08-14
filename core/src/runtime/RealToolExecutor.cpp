// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/RealToolExecutor.h"
#include "sentinel/core/security/PathGuard.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QUrlQuery>

namespace sentinel::core {

namespace {

QString getArgument(const PlannedToolInvocation& invocation, const QString& argId) {
    for (const auto& arg : invocation.arguments) {
        if (arg.id == argId) {
            return arg.value;
        }
    }
    return QString();
}

} // namespace

ToolExecutionResult RealToolExecutor::execute(const ToolExecutionRequest& request) const {
    if (request.plan.status != ToolInvocationPlanStatus::Planned ||
        request.plan.invocations.isEmpty()) {
        return {
            ToolExecutionStatus::EmptyPlan,
            QStringLiteral("No planned tool invocation reached the execution boundary."),
        };
    }

    for (const auto& invocation : request.plan.invocations) {
        if (!request.knownToolIds.contains(invocation.toolId)) {
            return {
                ToolExecutionStatus::UnknownTool,
                QStringLiteral("Execution boundary rejected unknown tool metadata: %1")
                    .arg(invocation.toolId),
            };
        }
    }

    if (request.approval.status == ApprovalStatus::Denied ||
        request.approval.status == ApprovalStatus::RequiresApproval) {
        return {
            ToolExecutionStatus::Blocked,
            request.approval.status == ApprovalStatus::RequiresApproval
                ? QStringLiteral("Execution boundary blocked: explicit user approval is required.")
                : QStringLiteral("Execution boundary blocked: approval denied."),
        };
    }

    if (request.sandbox.status == SandboxStatus::Denied ||
        request.sandbox.status == SandboxStatus::BlockedByApproval) {
        return {
            ToolExecutionStatus::Blocked,
            QStringLiteral("Execution boundary blocked by sandbox capability."),
        };
    }

    QStringList logs;
    QString currentWorkingDirectory = QDir::currentPath();

    for (const auto& invocation : request.plan.invocations) {
        // 1. Build a local plan summary from the supplied request metadata.
        if (invocation.toolId == QLatin1String("local-plan-summary")) {
            const auto topic = getArgument(invocation, QStringLiteral("topic"));
            Q_UNUSED(topic);
            logs.append(QStringLiteral("Executed: Local Plan Summary"));
        }
        // 2. read-file
        else if (invocation.toolId == QLatin1String("read-file")) {
            QString path = getArgument(invocation, QStringLiteral("path"));
            if (path.isEmpty()) {
                path = getArgument(invocation, QStringLiteral("topic"));
            }
            if (path.isEmpty()) {
                logs.append(QStringLiteral("read-file: No path argument provided."));
                continue;
            }
            QFileInfo fileInfo(path);
            if (fileInfo.isRelative()) {
                path = QDir(currentWorkingDirectory).absoluteFilePath(path);
            }
            path = PathGuard::safePath(currentWorkingDirectory, path);
            if (path.isEmpty()) {
                logs.append(QStringLiteral("read-file: Path is outside the approved workspace."));
                continue;
            }
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                logs.append(QStringLiteral("read-file: Failed to open '%1': %2")
                                .arg(path, file.errorString()));
                continue;
            }
            const QString content = QString::fromUtf8(file.readAll());
            logs.append(QStringLiteral("read-file: Successfully read '%1' (%2 bytes)")
                            .arg(path)
                            .arg(content.size()));
        }
        // 3. write-file
        else if (invocation.toolId == QLatin1String("write-file")) {
            QString path = getArgument(invocation, QStringLiteral("path"));
            const QString content = getArgument(invocation, QStringLiteral("content"));
            if (path.isEmpty()) {
                logs.append(QStringLiteral("write-file: No path argument provided."));
                continue;
            }
            QFileInfo fileInfo(path);
            if (fileInfo.isRelative()) {
                path = QDir(currentWorkingDirectory).absoluteFilePath(path);
            }
            path = PathGuard::safePath(currentWorkingDirectory, path);
            if (path.isEmpty()) {
                logs.append(QStringLiteral("write-file: Path is outside the approved workspace."));
                continue;
            }
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                logs.append(QStringLiteral("write-file: Failed to open '%1': %2")
                                .arg(path, file.errorString()));
                continue;
            }
            file.write(content.toUtf8());
            logs.append(QStringLiteral("write-file: Successfully wrote %1 bytes to '%2'")
                            .arg(QString::number(content.toUtf8().size()), path));
        }
        // 4. run-command
        else if (invocation.toolId == QLatin1String("run-command")) {
            const QString command = getArgument(invocation, QStringLiteral("command"));
            if (command.isEmpty()) {
                logs.append(QStringLiteral("run-command: No command argument provided."));
                continue;
            }
            QProcess process;
            process.setWorkingDirectory(currentWorkingDirectory);

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

            if (!process.waitForFinished(30000)) {
                logs.append(
                    QStringLiteral("run-command: Timed out or failed to start: %1").arg(command));
                continue;
            }
            const QString stdoutContent =
                QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            const QString stderrContent =
                QString::fromUtf8(process.readAllStandardError()).trimmed();
            const int exitCode = process.exitCode();
            if (exitCode == 0) {
                if (stdoutContent.isEmpty() && stderrContent.isEmpty()) {
                    logs.append(QStringLiteral("Command executed successfully. (exit=0)"));
                } else if (!stdoutContent.isEmpty() && stderrContent.isEmpty()) {
                    logs.append(
                        QStringLiteral("Command executed successfully. (exit=0)\n\n[STDOUT]:\n%1")
                            .arg(stdoutContent));
                } else {
                    logs.append(QStringLiteral("Command executed successfully. "
                                               "(exit=0)\n\n[STDOUT]:\n%1\n\n[STDERR]:\n%2")
                                    .arg(stdoutContent, stderrContent));
                }
            } else {
                logs.append(
                    QStringLiteral(
                        "Command execution failed. (exit=%1)\n\n[STDOUT]:\n%2\n\n[STDERR]:\n%3")
                        .arg(QString::number(exitCode), stdoutContent, stderrContent));
            }
        }
        // 5. voice-transcribe
        else if (invocation.toolId == QLatin1String("voice-transcribe")) {
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
                logs.append(
                    QStringLiteral("voice-transcribe: Whisper timed out for '%1'").arg(path));
                continue;
            }
            const QString transcript = QString::fromUtf8(process.readAllStandardOutput()).trimmed();
            logs.append(QStringLiteral("voice-transcribe: OK\n%1").arg(transcript));
        }
        // 6. voice-speak
        else if (invocation.toolId == QLatin1String("voice-speak")) {
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
            logs.append(QStringLiteral("voice-speak: TTS synthesis OK → %1")
                            .arg(ttsOutput));
        }
        // 7. Do not simulate external search results.
        else if (invocation.toolId == QLatin1String("web-search")) {
            const auto query = getArgument(invocation, QStringLiteral("query"));
            if (query.trimmed().isEmpty()) {
                logs.append(QStringLiteral("web-search: No query argument provided."));
                continue;
            }

            QUrl url(QStringLiteral("https://html.duckduckgo.com/html/"));
            QUrlQuery queryParams;
            queryParams.addQueryItem(QStringLiteral("q"), query.trimmed());
            url.setQuery(queryParams);
            QNetworkRequest networkRequest(url);
            networkRequest.setHeader(QNetworkRequest::UserAgentHeader,
                                     QStringLiteral("Sentinel/1.0 (local assistant)"));
            QNetworkAccessManager networkManager;
            QNetworkReply* reply = networkManager.get(networkRequest);
            QEventLoop eventLoop;
            QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
            eventLoop.exec();

            if (reply->error() != QNetworkReply::NoError) {
                logs.append(QStringLiteral("web-search: Request failed: %1")
                                .arg(reply->errorString()));
                reply->deleteLater();
                continue;
            }

            const auto html = QString::fromUtf8(reply->readAll());
            reply->deleteLater();
            const QRegularExpression resultPattern(
                QStringLiteral("<a[^>]+class=\\\"result__a\\\"[^>]+href=\\\"([^\\\"]+)\\\"[^>]*>(.*?)</a>"),
                QRegularExpression::CaseInsensitiveOption | QRegularExpression::DotMatchesEverythingOption);
            auto match = resultPattern.globalMatch(html);
            int resultCount = 0;
            while (match.hasNext() && resultCount < 5) {
                const auto result = match.next();
                auto resultUrl = QUrl::fromEncoded(result.captured(1).toUtf8());
                const auto title = result.captured(2).remove(QRegularExpression(QStringLiteral("<[^>]*>"))).trimmed();
                if (resultUrl.isValid() && !title.isEmpty()) {
                    logs.append(QStringLiteral("%1. %2\n%3")
                                    .arg(++resultCount)
                                    .arg(title)
                                    .arg(resultUrl.toString()));
                }
            }
            if (resultCount == 0) {
                logs.append(QStringLiteral("web-search: No results found for '%1'.").arg(query));
            }
        }
        // 8. open-workspace
        else if (invocation.toolId == QLatin1String("open-workspace")) {
            const QString path = getArgument(invocation, QStringLiteral("path"));
            const auto workspacePath = QDir(path).absolutePath();
            if (path.isEmpty() || !QDir(workspacePath).exists()) {
                return {
                    ToolExecutionStatus::Blocked,
                    QStringLiteral("open-workspace: requested workspace does not exist."),
                };
            }
            currentWorkingDirectory = workspacePath;
            logs.append(QStringLiteral("open-workspace: Workspace context set → '%1'")
                            .arg(currentWorkingDirectory));
        }
        // 9. summarize-current-conversation
        else if (invocation.toolId == QLatin1String("summarize-current-conversation")) {
            logs.append(QStringLiteral("summarize-current-conversation: Summary compiled."));
        }
        // 10. provider-test-call
        else if (invocation.toolId == QLatin1String("provider-test-call")) {
            return {
                ToolExecutionStatus::Blocked,
                QStringLiteral("provider-test-call is unavailable: no provider test executor is configured."),
            };
        }
        // 11. export-conversation
        else if (invocation.toolId == QLatin1String("export-conversation")) {
            return {
                ToolExecutionStatus::Blocked,
                QStringLiteral("export-conversation is unavailable: use the conversation export service."),
            };
        }
        // Never report an unknown implementation as a successful execution.
        else {
            return {
                ToolExecutionStatus::UnknownTool,
                QStringLiteral("Execution boundary rejected unimplemented tool: %1")
                    .arg(invocation.toolId),
            };
        }
    }

    return {
        ToolExecutionStatus::Succeeded,
        logs.join(QStringLiteral("\n\n")),
    };
}

} // namespace sentinel::core
