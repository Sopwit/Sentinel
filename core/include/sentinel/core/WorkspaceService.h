#pragma once

#include <QString>
#include <QStringList>
#include <QList>

namespace sentinel::core {

struct WorkspaceMetadata {
    QString id;
    QString name;
    QString kind;
    QString accessState;
    QString permissionPosture;
    QString rootSummary;
    QString permissionSummary;
};

struct WorkspaceReadinessSummary {
    QString status;
    QString summary;
    QStringList checks;
    QStringList boundaryDiagnostics;
};

class WorkspaceService final {
public:
    QList<WorkspaceMetadata> availableWorkspaces() const;
    WorkspaceMetadata selectedWorkspace(const QString& selectedWorkspaceId) const;
    WorkspaceReadinessSummary readiness(const QString& selectedWorkspaceId) const;
    QStringList permissionPostures() const;
    QStringList actionPlaceholders() const;
    QStringList workspaceSummaries() const;
    QString normalizedWorkspaceId(const QString& workspaceId) const;
};

QString workspaceSummary(const WorkspaceMetadata& workspace);

} // namespace sentinel::core
