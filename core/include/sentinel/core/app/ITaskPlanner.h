// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/app/TaskPlanning.h"
#include <QtGlobal>

namespace sentinel::core {

class ITaskPlanner {
public:
    Q_DISABLE_COPY(ITaskPlanner)
    ITaskPlanner() = default;
    virtual ~ITaskPlanner() = default;

    virtual TaskPlan plan(const TaskPlanningRequest& request) const = 0;
};

} // namespace sentinel::core
