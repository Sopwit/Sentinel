// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/ToolExecution.h"

namespace sentinel::core {

class IToolExecutor {
public:
    virtual ~IToolExecutor() = default;

    virtual ToolExecutionResult execute(const ToolExecutionRequest& request) const = 0;
};

} // namespace sentinel::core
