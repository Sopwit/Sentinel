#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

enum class PermissionPolicyState {
    Disabled,
    AskEveryTime,
    Trusted,
    Enabled,
};

struct PermissionPolicySummary {
    QString domainId;
    QString domainName;
    QString state;
    QString summary;
    QString safetyBoundary;
    QStringList diagnostics;
};

struct PermissionPolicyRegistrySummary {
    QString status;
    QString summary;
    QString defaultState;
    QStringList stateLabels;
    QStringList domainSummaries;
    QStringList developerDiagnostics;
};

class PermissionPolicyService final {
public:
    QList<PermissionPolicySummary> permissionSummaries(const QString& defaultState) const;
    PermissionPolicyRegistrySummary registrySummary(const QString& defaultState) const;
    QStringList permissionDomainIds() const;
    QStringList permissionDomainNames() const;
    QStringList permissionStateLabels() const;
    QString normalizedState(const QString& state) const;
};

QString permissionPolicyStateName(PermissionPolicyState state);
QString permissionPolicySummaryLine(const PermissionPolicySummary& summary);

} // namespace sentinel::core
