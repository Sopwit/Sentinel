#pragma once

#include "sentinel/core/IToolExecutor.h"

namespace sentinel::core {

class RealToolExecutor final : public IToolExecutor {
public:
    ToolExecutionResult execute(const ToolExecutionRequest& request) const override;
};

} // namespace sentinel::core
