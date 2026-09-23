// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/ProcessExecutor.h"
#include "sentinel/core/runtime/ToolExecution.h"

#include <functional>

namespace sentinel::core {

class IToolExecutor {
public:
    using Completion = std::function<void(ToolExecutionResult)>;
    using Output = std::function<void(const QString&, ProcessStream, const QByteArray&)>;
    using Cancel = std::function<void()>;
    virtual ~IToolExecutor() = default;

    virtual ToolExecutionResult execute(const ToolExecutionRequest& request) const = 0;
    // Called on an event-loop thread. Immediate tools invoke completion inline;
    // process-backed tools invoke it from their QProcess terminal signal.
    virtual Cancel executeAsync(const ToolExecutionRequest& request, const QString& sessionId,
                                const QString& toolCallId, Output output,
                                Completion completion) const {
        Q_UNUSED(sessionId)
        Q_UNUSED(toolCallId)
        Q_UNUSED(output)
        completion(execute(request));
        return {};
    }
};

} // namespace sentinel::core
