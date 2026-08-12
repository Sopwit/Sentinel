// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/runtime/IToolExecutor.h"

namespace sentinel::core {

class NullToolExecutor final : public IToolExecutor {
public:
    ToolExecutionResult execute(const ToolExecutionRequest& request) const override;
};

} // namespace sentinel::core
