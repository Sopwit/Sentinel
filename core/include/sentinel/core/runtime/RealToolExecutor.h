// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IToolExecutor.h"
#include "sentinel/core/runtime/tools/WebSearchTool.h"

namespace sentinel::core {

class RealToolExecutor final : public IToolExecutor {
public:
    void configureWebSearch(const QString& provider, const QString& apiKey, int maxResults);
    WebSearchResponse searchWeb(const QString& query) const;
    ToolExecutionResult execute(const ToolExecutionRequest& request) const override;

private:
    mutable WebSearchTool webSearchTool_;
};

} // namespace sentinel::core
