#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

struct WorkspaceMetadata {
    QString id;
    QString name;
    QString kind;
    QString accessState;
    QString permissionPosture;
    QString rootSummary;
    QString permissionSummary;
    QString templateName;
    bool archived = false;
    QString modelSummary;
    QString routingSummary;
    QString contextSummary;
    QString notificationSummary;
    QString ragSummary;
    QString exportSummary;
};

struct WorkspaceReadinessSummary {
    QString status;
    QString summary;
    QStringList checks;
    QStringList boundaryDiagnostics;
};

struct WorkspaceMutationResult {
    bool success = false;
    QString selectedWorkspaceId;
    QString catalogJson;
    QString status = QStringLiteral("Refused");
    QString summary;
};

class WorkspaceService final {
public:
    QList<WorkspaceMetadata> availableWorkspaces(const QString& catalogJson = {}) const;
    WorkspaceMetadata selectedWorkspace(const QString& selectedWorkspaceId,
                                        const QString& catalogJson = {}) const;
    WorkspaceReadinessSummary readiness(const QString& selectedWorkspaceId,
                                        const QString& catalogJson = {}) const;
    QStringList permissionPostures() const;
    QStringList actionPlaceholders() const;
    QStringList workspaceSummaries(const QString& catalogJson = {}) const;
    QString normalizedWorkspaceId(const QString& workspaceId,
                                  const QString& catalogJson = {}) const;
    QStringList builtInTemplateNames() const;
    QString defaultCatalogJson() const;
    WorkspaceMutationResult createWorkspace(const QString& catalogJson, const QString& name,
                                            const QString& templateName) const;
    WorkspaceMutationResult renameWorkspace(const QString& catalogJson, const QString& workspaceId,
                                            const QString& name) const;
    WorkspaceMutationResult archiveWorkspace(const QString& catalogJson,
                                             const QString& workspaceId) const;
    WorkspaceMutationResult deleteWorkspace(const QString& catalogJson, const QString& workspaceId,
                                            const QString& currentWorkspaceId) const;
    WorkspaceMutationResult duplicateWorkspace(const QString& catalogJson,
                                               const QString& workspaceId) const;
};

QString workspaceSummary(const WorkspaceMetadata& workspace);

} // namespace sentinel::core
