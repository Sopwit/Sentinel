#pragma once

#include "sentinel/core/ITaskPlanner.h"

namespace sentinel::core {

class StaticTaskPlanner final : public ITaskPlanner {
public:
    TaskPlan plan(const TaskPlanningRequest& request) const override;
};

} // namespace sentinel::core
