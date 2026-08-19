// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/runtime/AlarmStore.h"
#include "sentinel/core/runtime/tools/WebFetchTool.h"
#include "sentinel/core/runtime/tools/WebSearchTool.h"
#include "sentinel/core/interfaces/IMemoryStore.h"
#include "sentinel/core/mcp/IMcpService.h"

#include <QJsonArray>
#include <functional>
#include <memory>

namespace sentinel::core {

class RealToolExecutor final : public IToolExecutor {
public:
    RealToolExecutor();
    explicit RealToolExecutor(std::shared_ptr<AlarmStore> alarmStore);

    void configureWebSearch(const QString& provider, const QString& apiKey, int maxResults);
    void setAlarmStore(std::shared_ptr<AlarmStore> alarmStore);
    // Snapshots long-term memory entries for the memory-search tool. Captured on the
    // controller thread before an agent run starts so tool threads never touch the
    // SQLite-backed store directly.
    void setMemorySnapshot(MemoryEntries entries);
    // Snapshots chat history lines ("[role] content") for the history-search tool.
    void setHistorySnapshot(QStringList entries);
    // Replaces the MCP server set used by the mcp-list/mcp-call tools and connects
    // to every enabled server.
    void configureMcpServers(const QList<McpServerConfig>& configs);
    // Test seam: inject a custom MCP service implementation.
    void setMcpService(std::shared_ptr<IMcpService> service);
    // Injects the subagent runner used by the spawn-agent tool. The runner
    // executes a bounded, read-only agent loop for the given task and returns
    // its final answer (or an error description).
    void setSubagentRunner(std::function<QString(const QString& task)> runner);
    WebSearchResponse searchWeb(const QString& query) const;
    ToolExecutionResult execute(const ToolExecutionRequest& request) const override;

private:
    mutable WebSearchTool webSearchTool_;
    mutable WebFetchTool webFetchTool_;
    std::shared_ptr<AlarmStore> alarmStore_;
    MemoryEntries memorySnapshot_;
    QStringList historySnapshot_;
    std::shared_ptr<IMcpService> mcpService_;
    std::function<QString(const QString& task)> subagentRunner_;
    mutable bool subagentActive_ = false;
    mutable QJsonArray todos_;
};

} // namespace sentinel::core
