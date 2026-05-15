#pragma once

#include "sentinel/core/ToolDescriptor.h"

#include <QList>
#include <QString>

namespace sentinel::core {

enum class SandboxStatus {
    NotEvaluated,
    Allowed,
    Denied,
    BlockedByApproval,
};

inline QString sandboxStatusName(SandboxStatus status) {
    switch (status) {
    case SandboxStatus::NotEvaluated:
        return QStringLiteral("Not Evaluated");
    case SandboxStatus::Allowed:
        return QStringLiteral("Allowed");
    case SandboxStatus::Denied:
        return QStringLiteral("Denied");
    case SandboxStatus::BlockedByApproval:
        return QStringLiteral("Blocked By Approval");
    }

    return QStringLiteral("Not Evaluated");
}

struct CapabilityDescriptor {
    QString id;
    QString description;
};

struct SandboxCapabilityDecision {
    QString toolId;
    CapabilityDescriptor capability;
    bool allowed = false;
    QString reason;
};

struct SandboxEvaluationResult {
    SandboxStatus status = SandboxStatus::NotEvaluated;
    QString summary;
    QList<SandboxCapabilityDecision> capabilityDecisions;
};

} // namespace sentinel::core
