// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/runtime/AlarmStore.h"
#include "sentinel/core/runtime/tools/WebFetchTool.h"
#include "sentinel/core/runtime/tools/WebSearchTool.h"
#include "sentinel/core/interfaces/IMemoryStore.h"

#include <QJsonArray>
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
    WebSearchResponse searchWeb(const QString& query) const;
    ToolExecutionResult execute(const ToolExecutionRequest& request) const override;

private:
    mutable WebSearchTool webSearchTool_;
    mutable WebFetchTool webFetchTool_;
    std::shared_ptr<AlarmStore> alarmStore_;
    MemoryEntries memorySnapshot_;
    mutable QJsonArray todos_;
};

} // namespace sentinel::core
