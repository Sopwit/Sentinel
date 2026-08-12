// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/app/ITaskPlanner.h"

namespace sentinel::core {

class StaticTaskPlanner final : public ITaskPlanner {
public:
    TaskPlan plan(const TaskPlanningRequest& request) const override;
};

} // namespace sentinel::core
