// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IToolExecutor.h"

namespace sentinel::core {

// Gateway-mediated calls enforce ToolDescriptor.inputSchema before invoking a handler.
// Arguments then carry normalized JSON values and defaults; handlers still enforce
// semantic permissions, resource state, and operation-specific safety.
class IToolHandler {
public:
    virtual ~IToolHandler() = default;
    virtual IToolExecutor::Cancel execute(const ToolExecutionRequest& request,
                                          const QString& sessionId, const QString& toolCallId,
                                          IToolExecutor::Output output,
                                          IToolExecutor::Completion completion) = 0;
};

} // namespace sentinel::core
