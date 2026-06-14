#include "sentinel/core/WorkspaceService.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

namespace sentinel::core {

namespace {

QString stableWorkspaceId(const QString& name, const QString& templateName,
                          const QString& catalogJson) {
    const auto seed = name.trimmed() + QStringLiteral("|") + templateName.trimmed() +
                      QStringLiteral("|") + catalogJson;
    const auto digest =
        QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha1).toHex().left(10);
    return QStringLiteral("workspace-%1").arg(QString::fromLatin1(digest));
}

WorkspaceMetadata makeWorkspace(const QString& id, const QString& name, const QString& kind,
                                const QString& templateName, bool archived = false) {
    return {
        id,
        name,
        kind,
        archived ? QStringLiteral("Archived") : QStringLiteral("Active"),
        QStringLiteral("Workspace Only"),
        QStringLiteral("No folder selected. Files are attached explicitly by the user only."),
        QStringLiteral("Workspace-scoped metadata. No folder access, scanning, or background work."),
        templateName,
        archived,
        QStringLiteral("Selected model and role assignments are isolated to this workspace."),
        QStringLiteral("Routing roles are workspace preferences; no automatic multi-model routing."),
        QStringLiteral("Context settings are workspace-scoped and opt-in."),
        QStringLiteral("Notification settings are workspace-scoped and respect the global policy."),
        QStringLiteral("Local RAG disabled by default; manual indexing only when enabled."),
        QStringLiteral("Export defaults are workspace-scoped for chats, summaries, and retrieval reports."),
    };
}

QList<WorkspaceMetadata> builtIns() {
    return {
        makeWorkspace(QStringLiteral("personal"), QStringLiteral("Personal"),
                      QStringLiteral("Built-in template"), QStringLiteral("Personal")),
        makeWorkspace(QStringLiteral("coding"), QStringLiteral("Coding"),
                      QStringLiteral("Built-in template"), QStringLiteral("Coding")),
        makeWorkspace(QStringLiteral("research"), QStringLiteral("Research"),
                      QStringLiteral("Built-in template"), QStringLiteral("Research")),
        makeWorkspace(QStringLiteral("writing"), QStringLiteral("Writing"),
                      QStringLiteral("Built-in template"), QStringLiteral("Writing")),
        makeWorkspace(QStringLiteral("student"), QStringLiteral("Student"),
                      QStringLiteral("Built-in template"), QStringLiteral("Student")),
    };
}

QJsonObject toJson(const WorkspaceMetadata& workspace) {
    return {
        {QStringLiteral("id"), workspace.id},
        {QStringLiteral("name"), workspace.name},
        {QStringLiteral("kind"), workspace.kind},
        {QStringLiteral("templateName"), workspace.templateName},
        {QStringLiteral("archived"), workspace.archived},
    };
}

WorkspaceMetadata fromJson(const QJsonObject& object) {
    return makeWorkspace(object.value(QStringLiteral("id")).toString().trimmed(),
                         object.value(QStringLiteral("name")).toString().trimmed(),
                         object.value(QStringLiteral("kind")).toString(QStringLiteral("Custom")),
                         object.value(QStringLiteral("templateName")).toString(QStringLiteral("Personal")),
                         object.value(QStringLiteral("archived")).toBool(false));
}

QList<WorkspaceMetadata> customWorkspaces(const QString& catalogJson) {
    QList<WorkspaceMetadata> workspaces;
    const auto document = QJsonDocument::fromJson(catalogJson.toUtf8());
    if (!document.isObject()) {
        return workspaces;
    }

    const auto array = document.object().value(QStringLiteral("customWorkspaces")).toArray();
    QSet<QString> seenIds;
    for (const auto& value : array) {
        if (!value.isObject()) {
            continue;
        }

        auto workspace = fromJson(value.toObject());
        if (workspace.id.isEmpty() || workspace.name.isEmpty() || seenIds.contains(workspace.id)) {
            continue;
        }
        seenIds.insert(workspace.id);
        workspaces.append(workspace);
    }
    return workspaces;
}

QString encodeCustomWorkspaces(const QList<WorkspaceMetadata>& workspaces) {
    QJsonArray array;
    for (const auto& workspace : workspaces) {
        if (!workspace.kind.startsWith(QStringLiteral("Built-in"))) {
            array.append(toJson(workspace));
        }
    }
    QJsonObject root;
    root.insert(QStringLiteral("version"), 1);
    root.insert(QStringLiteral("customWorkspaces"), array);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

bool isBuiltInId(const QString& workspaceId) {
    for (const auto& workspace : builtIns()) {
        if (workspace.id == workspaceId) {
            return true;
        }
    }
    return false;
}

} // namespace

QList<WorkspaceMetadata> WorkspaceService::availableWorkspaces(const QString& catalogJson) const {
    auto workspaces = builtIns();
    workspaces.append(customWorkspaces(catalogJson));
    return workspaces;
}

WorkspaceMetadata WorkspaceService::selectedWorkspace(const QString& selectedWorkspaceId,
                                                      const QString& catalogJson) const {
    const auto normalized = normalizedWorkspaceId(selectedWorkspaceId, catalogJson);
    for (const auto& workspace : availableWorkspaces(catalogJson)) {
        if (workspace.id == normalized) {
            return workspace;
        }
    }
    return builtIns().first();
}

WorkspaceReadinessSummary WorkspaceService::readiness(const QString& selectedWorkspaceId,
                                                      const QString& catalogJson) const {
    const auto workspace = selectedWorkspace(selectedWorkspaceId, catalogJson);
    return {
        workspace.archived ? QStringLiteral("Archived") : QStringLiteral("Ready"),
        QStringLiteral("%1 workspace is active metadata scope. Chat context, Brain summaries, "
                       "settings, attachments, and Local RAG metadata are isolated by workspace.")
            .arg(workspace.name),
        {
            QStringLiteral("Selected workspace: %1").arg(workspace.name),
            QStringLiteral("Template: %1").arg(workspace.templateName),
            QStringLiteral("Chat context: workspace-separated"),
            QStringLiteral("Brain state: workspace-separated summaries"),
            QStringLiteral("Settings: workspace-scoped preferences"),
            QStringLiteral("Filesystem scanning: disabled"),
            QStringLiteral("Background indexing: disabled"),
            QStringLiteral("Cloud retrieval: disabled"),
        },
        {
            QStringLiteral("Document scope: Workspace Only"),
            QStringLiteral("Attachment lifecycle: explicit user-selected files only"),
            QStringLiteral("Local RAG: disabled by default; manual indexing only"),
            QStringLiteral("Runtime boundary: no autonomous agents, no recursive scanning"),
            QStringLiteral("Data boundary: settings, chat history, memory, and RAG storage remain separate"),
        },
    };
}

QStringList WorkspaceService::permissionPostures() const {
    return {
        QStringLiteral("Workspace Only"),
        QStringLiteral("Manual Attachments Only"),
        QStringLiteral("Manual Indexing Only"),
        QStringLiteral("Cloud Retrieval Disabled"),
    };
}

QStringList WorkspaceService::actionPlaceholders() const {
    return {
        QStringLiteral("Create Workspace: available"),
        QStringLiteral("Rename Workspace: available for user workspaces"),
        QStringLiteral("Archive Workspace: available for user workspaces"),
        QStringLiteral("Delete Workspace: available for user workspaces"),
        QStringLiteral("Duplicate Workspace: available"),
        QStringLiteral("Folder Import: disabled"),
        QStringLiteral("Recursive Scan: disabled"),
        QStringLiteral("Background Processing: disabled"),
    };
}

QStringList WorkspaceService::workspaceSummaries(const QString& catalogJson) const {
    QStringList summaries;
    for (const auto& workspace : availableWorkspaces(catalogJson)) {
        summaries.append(workspaceSummary(workspace));
    }
    return summaries;
}

QString WorkspaceService::normalizedWorkspaceId(const QString& workspaceId,
                                                const QString& catalogJson) const {
    const auto trimmed = workspaceId.trimmed();
    for (const auto& workspace : availableWorkspaces(catalogJson)) {
        if (workspace.id == trimmed) {
            return workspace.id;
        }
    }
    return QStringLiteral("personal");
}

QStringList WorkspaceService::builtInTemplateNames() const {
    return {QStringLiteral("Personal"), QStringLiteral("Coding"), QStringLiteral("Research"),
            QStringLiteral("Writing"), QStringLiteral("Student")};
}

QString WorkspaceService::defaultCatalogJson() const {
    return encodeCustomWorkspaces({});
}

WorkspaceMutationResult WorkspaceService::createWorkspace(const QString& catalogJson,
                                                          const QString& name,
                                                          const QString& templateName) const {
    const auto normalizedName = name.trimmed();
    const auto normalizedTemplate = builtInTemplateNames().contains(templateName.trimmed())
                                        ? templateName.trimmed()
                                        : QStringLiteral("Personal");
    if (normalizedName.isEmpty()) {
        return {false, {}, catalogJson, QStringLiteral("Refused"),
                QStringLiteral("Workspace name is required.")};
    }

    auto custom = customWorkspaces(catalogJson);
    const auto id = stableWorkspaceId(normalizedName, normalizedTemplate, catalogJson);
    custom.append(makeWorkspace(id, normalizedName, QStringLiteral("Custom workspace"),
                                normalizedTemplate));
    return {true, id, encodeCustomWorkspaces(custom), QStringLiteral("Created"),
            QStringLiteral("Created workspace %1 from %2 template.").arg(normalizedName,
                                                                         normalizedTemplate)};
}

WorkspaceMutationResult WorkspaceService::renameWorkspace(const QString& catalogJson,
                                                          const QString& workspaceId,
                                                          const QString& name) const {
    const auto id = workspaceId.trimmed();
    const auto normalizedName = name.trimmed();
    if (isBuiltInId(id)) {
        return {false, id, catalogJson, QStringLiteral("Refused"),
                QStringLiteral("Built-in workspaces cannot be renamed.")};
    }
    if (normalizedName.isEmpty()) {
        return {false, id, catalogJson, QStringLiteral("Refused"),
                QStringLiteral("Workspace name is required.")};
    }

    auto custom = customWorkspaces(catalogJson);
    for (auto& workspace : custom) {
        if (workspace.id == id) {
            workspace.name = normalizedName;
            return {true, id, encodeCustomWorkspaces(custom), QStringLiteral("Renamed"),
                    QStringLiteral("Renamed workspace to %1.").arg(normalizedName)};
        }
    }
    return {false, id, catalogJson, QStringLiteral("Refused"),
            QStringLiteral("Workspace was not found.")};
}

WorkspaceMutationResult WorkspaceService::archiveWorkspace(const QString& catalogJson,
                                                           const QString& workspaceId) const {
    const auto id = workspaceId.trimmed();
    if (isBuiltInId(id)) {
        return {false, id, catalogJson, QStringLiteral("Refused"),
                QStringLiteral("Built-in workspaces cannot be archived.")};
    }

    auto custom = customWorkspaces(catalogJson);
    for (auto& workspace : custom) {
        if (workspace.id == id) {
            workspace.archived = true;
            return {true, id, encodeCustomWorkspaces(custom), QStringLiteral("Archived"),
                    QStringLiteral("Archived workspace %1.").arg(workspace.name)};
        }
    }
    return {false, id, catalogJson, QStringLiteral("Refused"),
            QStringLiteral("Workspace was not found.")};
}

WorkspaceMutationResult WorkspaceService::deleteWorkspace(const QString& catalogJson,
                                                          const QString& workspaceId,
                                                          const QString& currentWorkspaceId) const {
    const auto id = workspaceId.trimmed();
    if (isBuiltInId(id)) {
        return {false, id, catalogJson, QStringLiteral("Refused"),
                QStringLiteral("Built-in workspaces cannot be deleted.")};
    }

    auto custom = customWorkspaces(catalogJson);
    for (qsizetype index = 0; index < custom.size(); ++index) {
        if (custom.at(index).id == id) {
            const auto name = custom.at(index).name;
            custom.removeAt(index);
            const auto selected =
                currentWorkspaceId == id ? QStringLiteral("personal") : currentWorkspaceId;
            return {true, selected, encodeCustomWorkspaces(custom), QStringLiteral("Deleted"),
                    QStringLiteral("Deleted workspace %1.").arg(name)};
        }
    }
    return {false, id, catalogJson, QStringLiteral("Refused"),
            QStringLiteral("Workspace was not found.")};
}

WorkspaceMutationResult WorkspaceService::duplicateWorkspace(const QString& catalogJson,
                                                             const QString& workspaceId) const {
    const auto source = selectedWorkspace(workspaceId, catalogJson);
    auto custom = customWorkspaces(catalogJson);
    const auto duplicateName = QStringLiteral("%1 Copy").arg(source.name);
    const auto id = stableWorkspaceId(duplicateName, source.templateName, catalogJson);
    custom.append(makeWorkspace(id, duplicateName, QStringLiteral("Custom workspace"),
                                source.templateName, source.archived));
    return {true, id, encodeCustomWorkspaces(custom), QStringLiteral("Duplicated"),
            QStringLiteral("Duplicated workspace %1.").arg(source.name)};
}

QString workspaceSummary(const WorkspaceMetadata& workspace) {
    return QStringLiteral("%1 / %2 / %3 / %4")
        .arg(workspace.name, workspace.templateName, workspace.accessState, workspace.ragSummary);
}

} // namespace sentinel::core
