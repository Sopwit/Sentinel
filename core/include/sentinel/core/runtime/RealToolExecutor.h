// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/interfaces/IMemoryStore.h"
#include "sentinel/core/mcp/IMcpService.h"
#include "sentinel/core/runtime/AlarmStore.h"
#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/runtime/IFileSystemService.h"
#include "sentinel/core/runtime/tools/WebFetchTool.h"
#include "sentinel/core/runtime/tools/WebSearchTool.h"

#include <QJsonArray>
#include <functional>
#include <memory>

namespace sentinel::core {

class ExternalDirectoryGate;
class IChatHistoryStore;

class RealToolExecutor final : public IToolExecutor {
public:
    RealToolExecutor();
    explicit RealToolExecutor(std::shared_ptr<AlarmStore> alarmStore);

    void setExternalDirectoryGate(const ExternalDirectoryGate* gate) {
        externalDirectoryGate_ = gate;
        fileSystemService_.setExternalDirectoryGate(gate);
    }
    void configureWebSearch(const QString& provider, const QString& apiKey, int maxResults);
    void setAlarmStore(std::shared_ptr<AlarmStore> alarmStore);
    void setSearchStores(const IMemoryStore* memory, const IChatHistoryStore* history) {
        memoryStore_ = memory;
        chatHistoryStore_ = history;
    }
    // Replaces the MCP server set used by the dynamic MCP provider and connects
    // to every enabled server.
    void configureMcpServers(const QList<McpServerConfig>& configs);
    // Test seam: inject a custom MCP service implementation.
    void setMcpService(std::shared_ptr<IMcpService> service);
    std::shared_ptr<IMcpService> mcpService() const {
        return mcpService_;
    }
    // Injects the subagent runner used by the spawn-agent tool. The runner
    // executes a bounded, read-only agent loop for the given task and returns
    // its final answer (or an error description).
    void setSubagentRunner(std::function<QString(const QString& task)> runner);
    WebSearchResponse searchWeb(const QString& query) const;
    ToolExecutionResult execute(const ToolExecutionRequest& request) const override;
    Cancel executeAsync(const ToolExecutionRequest& request, const QString& sessionId,
                        const QString& toolCallId, Output output,
                        Completion completion) const override;

public:
    using BuiltInMethod = ToolExecutionResult (RealToolExecutor::*)(const PlannedToolInvocation&,
                                                                    QString&) const;
    ToolExecutionResult executeLocalPlanSummary(const PlannedToolInvocation& invocation,
                                                QString& currentWorkingDirectory) const;
    ToolExecutionResult executeListDirectory(const PlannedToolInvocation& invocation,
                                             QString& currentWorkingDirectory) const;
    ToolExecutionResult executeReadFile(const PlannedToolInvocation& invocation,
                                        QString& currentWorkingDirectory) const;
    ToolExecutionResult executeWriteFile(const PlannedToolInvocation& invocation,
                                         QString& currentWorkingDirectory) const;
    ToolExecutionResult executeEditFile(const PlannedToolInvocation& invocation,
                                        QString& currentWorkingDirectory) const;
    ToolExecutionResult executeDeleteFile(const PlannedToolInvocation& invocation,
                                          QString& currentWorkingDirectory) const;
    ToolExecutionResult executeMoveFile(const PlannedToolInvocation& invocation,
                                        QString& currentWorkingDirectory) const;
    ToolExecutionResult executeApplyPatch(const PlannedToolInvocation& invocation,
                                          QString& currentWorkingDirectory) const;
    ToolExecutionResult executeListCodeDefinitions(const PlannedToolInvocation& invocation,
                                                   QString& currentWorkingDirectory) const;
    ToolExecutionResult executeGrep(const PlannedToolInvocation& invocation,
                                    QString& currentWorkingDirectory) const;
    ToolExecutionResult executeGlob(const PlannedToolInvocation& invocation,
                                    QString& currentWorkingDirectory) const;
    ToolExecutionResult executeRunCommand(const PlannedToolInvocation& invocation,
                                          QString& currentWorkingDirectory) const;
    ToolExecutionResult executeAppLaunch(const PlannedToolInvocation& invocation,
                                         QString& currentWorkingDirectory) const;
    ToolExecutionResult executeAppQuit(const PlannedToolInvocation& invocation,
                                       QString& currentWorkingDirectory) const;
    ToolExecutionResult executeOpenUrl(const PlannedToolInvocation& invocation,
                                       QString& currentWorkingDirectory) const;
    ToolExecutionResult executeSpawnAgent(const PlannedToolInvocation& invocation,
                                          QString& currentWorkingDirectory) const;
    ToolExecutionResult executeBrowserScreenshot(const PlannedToolInvocation& invocation,
                                                 QString& currentWorkingDirectory) const;
    ToolExecutionResult executeBrowserPdf(const PlannedToolInvocation& invocation,
                                          QString& currentWorkingDirectory) const;
    ToolExecutionResult executeSystemNotify(const PlannedToolInvocation& invocation,
                                            QString& currentWorkingDirectory) const;
    ToolExecutionResult executeClipboardRead(const PlannedToolInvocation& invocation,
                                             QString& currentWorkingDirectory) const;
    ToolExecutionResult executeClipboardWrite(const PlannedToolInvocation& invocation,
                                              QString& currentWorkingDirectory) const;
    ToolExecutionResult executeSystemInfo(const PlannedToolInvocation& invocation,
                                          QString& currentWorkingDirectory) const;
    ToolExecutionResult executeProcessList(const PlannedToolInvocation& invocation,
                                           QString& currentWorkingDirectory) const;
    ToolExecutionResult executeCurrentTime(const PlannedToolInvocation& invocation,
                                           QString& currentWorkingDirectory) const;
    ToolExecutionResult executeSetAlarm(const PlannedToolInvocation& invocation,
                                        QString& currentWorkingDirectory) const;
    ToolExecutionResult executeListAlarms(const PlannedToolInvocation& invocation,
                                          QString& currentWorkingDirectory) const;
    ToolExecutionResult executeCancelAlarm(const PlannedToolInvocation& invocation,
                                           QString& currentWorkingDirectory) const;
    ToolExecutionResult executeTodoWrite(const PlannedToolInvocation& invocation,
                                         QString& currentWorkingDirectory) const;
    ToolExecutionResult executeTodoRead(const PlannedToolInvocation& invocation,
                                        QString& currentWorkingDirectory) const;
    ToolExecutionResult executeMemorySearch(const PlannedToolInvocation& invocation,
                                            QString& currentWorkingDirectory) const;
    ToolExecutionResult executeHistorySearch(const PlannedToolInvocation& invocation,
                                             QString& currentWorkingDirectory) const;
    ToolExecutionResult executeAskQuestion(const PlannedToolInvocation& invocation,
                                           QString& currentWorkingDirectory) const;
    ToolExecutionResult executeWebFetch(const PlannedToolInvocation& invocation,
                                        QString& currentWorkingDirectory) const;
    ToolExecutionResult executeVoiceTranscribe(const PlannedToolInvocation& invocation,
                                               QString& currentWorkingDirectory) const;
    ToolExecutionResult executeVoiceSpeak(const PlannedToolInvocation& invocation,
                                          QString& currentWorkingDirectory) const;
    ToolExecutionResult executeWebSearch(const PlannedToolInvocation& invocation,
                                         QString& currentWorkingDirectory) const;
    ToolExecutionResult executeOpenWorkspace(const PlannedToolInvocation& invocation,
                                             QString& currentWorkingDirectory) const;
    ToolExecutionResult executeSummarizeCurrentConversation(const PlannedToolInvocation& invocation,
                                                            QString& currentWorkingDirectory) const;
    ToolExecutionResult executeProviderTestCall(const PlannedToolInvocation& invocation,
                                                QString& currentWorkingDirectory) const;
    ToolExecutionResult executeExportConversation(const PlannedToolInvocation& invocation,
                                                  QString& currentWorkingDirectory) const;

private:
    QString resolveToolPath(const QString& workingDirectory, const QString& rawPath,
                            bool write = false) const;
    const ExternalDirectoryGate* externalDirectoryGate_ = nullptr;
    QtFileSystemService fileSystemService_;
    mutable WebSearchTool webSearchTool_;
    mutable WebFetchTool webFetchTool_;
    std::shared_ptr<AlarmStore> alarmStore_;
    const IMemoryStore* memoryStore_ = nullptr;
    const IChatHistoryStore* chatHistoryStore_ = nullptr;
    std::shared_ptr<IMcpService> mcpService_;
    std::function<QString(const QString& task)> subagentRunner_;
    mutable bool subagentActive_ = false;
    mutable QJsonArray todos_;
};

} // namespace sentinel::core
