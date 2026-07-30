#pragma once

#include "sentinel/core/app/TaskPlanning.h"

namespace sentinel::core {

class ITaskPlanner {
public:
    virtual ~ITaskPlanner() = default;

    virtual TaskPlan plan(const TaskPlanningRequest& request) const = 0;
};

} // namespace sentinel::core
