#pragma once

#include "sentinel/core/ToolExecution.h"

namespace sentinel::core {

class IToolExecutor {
public:
    virtual ~IToolExecutor() = default;

    virtual ToolExecutionResult execute(const ToolExecutionRequest& request) const = 0;
};

} // namespace sentinel::core
