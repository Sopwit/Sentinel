#include "sentinel/core/WorkspaceService.h"

namespace sentinel::core {

namespace {

WorkspaceMetadata localPlaceholderWorkspace() {
    return {
        QStringLiteral("local-placeholder"),
        QStringLiteral("Local Workspace"),
        QStringLiteral("Local metadata placeholder"),
        QStringLiteral("Access not enabled"),
        QStringLiteral("Disabled"),
        QStringLiteral("No folder selected. No filesystem paths are read."),
        QStringLiteral("Disabled. Future modes are Ask Every Time, Trusted, and Enabled; none "
                       "grant access in this build."),
    };
}

} // namespace

QList<WorkspaceMetadata> WorkspaceService::availableWorkspaces() const {
    return {localPlaceholderWorkspace()};
}

WorkspaceMetadata WorkspaceService::selectedWorkspace(const QString& selectedWorkspaceId) const {
    const auto normalized = normalizedWorkspaceId(selectedWorkspaceId);
    for (const auto& workspace : availableWorkspaces()) {
        if (workspace.id == normalized) {
            return workspace;
        }
    }
    return localPlaceholderWorkspace();
}

WorkspaceReadinessSummary WorkspaceService::readiness(const QString& selectedWorkspaceId) const {
    const auto workspace = selectedWorkspace(selectedWorkspaceId);
    return {
        QStringLiteral("Not enabled"),
        QStringLiteral("Workspace access is not enabled yet. Sentinel stores only the selected "
                       "workspace metadata placeholder."),
        {
            QStringLiteral("Selected workspace: %1").arg(workspace.name),
            QStringLiteral("Permission posture: %1").arg(workspace.permissionPosture),
            QStringLiteral("Filesystem scanning: disabled"),
            QStringLiteral("File reading: disabled"),
            QStringLiteral("Indexing and embeddings: disabled"),
            QStringLiteral("Tool and plugin execution: disabled"),
            QStringLiteral("Autonomous agents and background workers: disabled"),
        },
        {
            QStringLiteral("Workspace root: metadata placeholder only"),
            QStringLiteral("Permission model: Disabled, Ask Every Time, Trusted, Enabled"),
            QStringLiteral("Current permission posture: %1").arg(workspace.permissionPosture),
            QStringLiteral("Runtime boundary: no subprocesses, no tools, no recursive scanning"),
            QStringLiteral("Data boundary: settings only; memory and chat history are separate"),
        },
    };
}

QStringList WorkspaceService::permissionPostures() const {
    return {
        QStringLiteral("Disabled"),
        QStringLiteral("Ask Every Time"),
        QStringLiteral("Trusted"),
        QStringLiteral("Enabled"),
    };
}

QStringList WorkspaceService::actionPlaceholders() const {
    return {
        QStringLiteral("Choose Workspace: unavailable until native picker wiring is scoped"),
        QStringLiteral("Clear Workspace: unavailable; only placeholder metadata is selected"),
        QStringLiteral("Scan Workspace: disabled; no filesystem scanning exists"),
        QStringLiteral("Index Workspace: disabled; no indexing or embeddings exist"),
    };
}

QStringList WorkspaceService::workspaceSummaries() const {
    QStringList summaries;
    for (const auto& workspace : availableWorkspaces()) {
        summaries.append(workspaceSummary(workspace));
    }
    return summaries;
}

QString WorkspaceService::normalizedWorkspaceId(const QString& workspaceId) const {
    const auto trimmed = workspaceId.trimmed();
    for (const auto& workspace : availableWorkspaces()) {
        if (workspace.id == trimmed) {
            return workspace.id;
        }
    }
    return localPlaceholderWorkspace().id;
}

QString workspaceSummary(const WorkspaceMetadata& workspace) {
    return QStringLiteral("%1 / %2 / %3")
        .arg(workspace.name, workspace.accessState, workspace.permissionPosture);
}

} // namespace sentinel::core
