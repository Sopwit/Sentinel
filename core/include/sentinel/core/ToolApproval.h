#pragma once

#include "sentinel/core/ToolDescriptor.h"

#include <QList>
#include <QString>

namespace sentinel::core {

enum class ApprovalStatus {
    NotRequested,
    NotRequired,
    RequiresApproval,
    Approved,
    Denied,
};

inline QString approvalStatusName(ApprovalStatus status) {
    switch (status) {
    case ApprovalStatus::NotRequested:
        return QStringLiteral("Not Requested");
    case ApprovalStatus::NotRequired:
        return QStringLiteral("Not Required");
    case ApprovalStatus::RequiresApproval:
        return QStringLiteral("Requires Approval");
    case ApprovalStatus::Approved:
        return QStringLiteral("Approved");
    case ApprovalStatus::Denied:
        return QStringLiteral("Denied");
    }

    return QStringLiteral("Not Requested");
}

struct PermissionDescriptor {
    QString id;
    QString description;
};

struct ToolApprovalRequest {
    QString toolId;
    QString summary;
    ToolRiskLevel riskLevel = ToolRiskLevel::Low;
    QList<PermissionDescriptor> permissions;
};

struct ApprovalDecision {
    ApprovalStatus status = ApprovalStatus::NotRequested;
    QString summary;
    QList<ToolApprovalRequest> requests;
};

} // namespace sentinel::core
