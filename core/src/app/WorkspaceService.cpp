// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/app/WorkspaceService.h"
#include "sentinel/core/model/ModelService.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include "sentinel/core/extension/ExtensionService.h"
#include "sentinel/core/voice/UnifiedAudioService.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QUuid>
#include <QDateTime>
#include <QFileInfo>
#include <algorithm>
#include <optional>

namespace sentinel::core {

namespace {

QString stableWorkspaceId(const QString& name, const QString& templateName,
                          const QString& catalogJson) {
    Q_UNUSED(name);
    Q_UNUSED(templateName);
    Q_UNUSED(catalogJson);
    return QStringLiteral("workspace-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
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
        QStringLiteral(
            "Workspace-scoped metadata. No folder access, scanning, or background work."),
        templateName,
        archived,
        QStringLiteral("Selected model and role assignments are isolated to this workspace."),
        QStringLiteral(
            "Routing roles are workspace preferences; no automatic multi-model routing."),
        QStringLiteral("Context settings are workspace-scoped and opt-in."),
        QStringLiteral("Notification settings are workspace-scoped and respect the global policy."),
        QStringLiteral("Local RAG disabled by default; manual indexing only when enabled."),
        QStringLiteral(
            "Export defaults are workspace-scoped for chats, summaries, and retrieval reports."),
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
        {QStringLiteral("rootPath"), workspace.rootPath},
        {QStringLiteral("description"), workspace.description},
        {QStringLiteral("createdAt"), workspace.createdAt},
        {QStringLiteral("updatedAt"), workspace.updatedAt},
    };
}

WorkspaceMetadata fromJson(const QJsonObject& object) {
    auto workspace = makeWorkspace(
        object.value(QStringLiteral("id")).toString().trimmed(),
        object.value(QStringLiteral("name")).toString().trimmed(),
        object.value(QStringLiteral("kind")).toString(QStringLiteral("Custom")),
        object.value(QStringLiteral("templateName")).toString(QStringLiteral("Personal")),
        object.value(QStringLiteral("archived")).toBool(false));
    workspace.rootPath = object.value(QStringLiteral("rootPath")).toString();
    if (!workspace.rootPath.isEmpty())
        workspace.rootSummary = QStringLiteral("Context root: %1 (no access granted)")
                                    .arg(workspace.rootPath);
    workspace.description = object.value(QStringLiteral("description")).toString();
    workspace.createdAt = object.value(QStringLiteral("createdAt")).toString();
    workspace.updatedAt = object.value(QStringLiteral("updatedAt")).toString();
    return workspace;
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

QJsonObject profileDocument(const QString& json) {
    if (json.trimmed().isEmpty())
        return {{QStringLiteral("version"), 1}, {QStringLiteral("workspaces"), QJsonObject{}},
                {QStringLiteral("presets"), QJsonObject{}}};
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    if (document.isObject() && document.object().value(QStringLiteral("version")).toInt() == 1)
        return document.object();
    return {};
}

QString encodeProfileDocument(const QJsonObject& document) {
    return QString::fromUtf8(QJsonDocument(document).toJson(QJsonDocument::Compact));
}

QList<WorkspacePreset> builtInPresets() {
    return {{QStringLiteral("general"), QStringLiteral("General"), true, {}},
            {QStringLiteral("coding"), QStringLiteral("Coding"), true, {}},
            {QStringLiteral("research"), QStringLiteral("Research"), true, {}},
            {QStringLiteral("local-only"), QStringLiteral("Local Only"), true,
             {{QStringLiteral("privacy"), QStringLiteral("local-only")}}}};
}

bool presetExists(const QList<WorkspacePreset>& presets, const QString& id) {
    for (const auto& preset : presets)
        if (preset.id == id) return true;
    return false;
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

QList<WorkspacePreset> WorkspaceService::presets(const QString& profilesJson) const {
    auto result = builtInPresets();
    const auto custom = profileDocument(profilesJson).value(QStringLiteral("presets")).toObject();
    for (auto it = custom.begin(); it != custom.end(); ++it) {
        if (!it.value().isObject() || presetExists(result, it.key())) continue;
        const auto object = it.value().toObject();
        const auto name = object.value(QStringLiteral("name")).toString().trimmed();
        if (!name.isEmpty())
            result.append({it.key(), name, false,
                           object.value(QStringLiteral("preferences")).toObject()});
    }
    return result;
}

QString WorkspaceService::createPreset(const QString& profilesJson, const QString& name,
                                       const QJsonObject& preferences) const {
    if (name.trimmed().isEmpty()) return {};
    auto root = profileDocument(profilesJson);
    if (root.isEmpty()) return {};
    auto custom = root.value(QStringLiteral("presets")).toObject();
    const auto id = QStringLiteral("preset-") + QUuid::createUuid().toString(QUuid::WithoutBraces);
    custom.insert(id, QJsonObject{{QStringLiteral("name"), name.trimmed()},
                                  {QStringLiteral("preferences"), preferences}});
    root.insert(QStringLiteral("presets"), custom);
    return encodeProfileDocument(root);
}

QString WorkspaceService::renamePreset(const QString& profilesJson, const QString& presetId,
                                       const QString& name) const {
    if (name.trimmed().isEmpty()) return {};
    auto root = profileDocument(profilesJson);
    if (root.isEmpty()) return {};
    auto custom = root.value(QStringLiteral("presets")).toObject();
    if (!custom.value(presetId).isObject()) return {};
    auto entry = custom.value(presetId).toObject();
    entry.insert(QStringLiteral("name"), name.trimmed());
    custom.insert(presetId, entry);
    root.insert(QStringLiteral("presets"), custom);
    return encodeProfileDocument(root);
}

QString WorkspaceService::updatePreset(const QString& profilesJson, const QString& presetId,
                                       const QJsonObject& preferences) const {
    auto root = profileDocument(profilesJson);
    if (root.isEmpty()) return {};
    auto custom = root.value(QStringLiteral("presets")).toObject();
    if (!custom.value(presetId).isObject()) return {};
    auto entry = custom.value(presetId).toObject();
    entry.insert(QStringLiteral("preferences"), preferences);
    custom.insert(presetId, entry);
    root.insert(QStringLiteral("presets"), custom);
    return encodeProfileDocument(root);
}

QString WorkspaceService::duplicatePreset(const QString& profilesJson,
                                          const QString& presetId) const {
    for (const auto& preset : presets(profilesJson))
        if (preset.id == presetId)
            return createPreset(profilesJson, preset.name + QStringLiteral(" Copy"),
                                preset.preferences);
    return {};
}

QString WorkspaceService::deletePreset(const QString& profilesJson,
                                       const QString& presetId) const {
    auto root = profileDocument(profilesJson);
    if (root.isEmpty()) return {};
    auto custom = root.value(QStringLiteral("presets")).toObject();
    if (!custom.contains(presetId)) return {};
    custom.remove(presetId);
    root.insert(QStringLiteral("presets"), custom);
    return encodeProfileDocument(root);
}

QString WorkspaceService::updateProfile(const QString& profilesJson,
                                        const QString& workspaceId, const QString& presetId,
                                        const QJsonObject& overrides) const {
    if (workspaceId.trimmed().isEmpty()) return {};
    auto root = profileDocument(profilesJson);
    if (root.isEmpty()) return {};
    auto workspaces = root.value(QStringLiteral("workspaces")).toObject();
    auto entry = workspaces.value(workspaceId).toObject();
    entry.insert(QStringLiteral("presetId"), presetId);
    entry.insert(QStringLiteral("overrides"), overrides);
    entry.insert(QStringLiteral("updatedAt"),
                 QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    workspaces.insert(workspaceId, entry);
    root.insert(QStringLiteral("workspaces"), workspaces);
    return encodeProfileDocument(root);
}

WorkspaceProfileSnapshot WorkspaceService::resolveProfile(
    const QString& profilesJson, const QString& workspaceId,
    const QJsonObject& globalDefaults, const QJsonObject& sessionOverrides,
    const ModelService* models, const IToolRegistry* tools,
    const ExtensionService* extensions, const SpeechProviderInfo* stt,
    const SpeechProviderInfo* tts) const {
    WorkspaceProfileSnapshot snapshot;
    snapshot.workspaceId = workspaceId;
    const auto root = profileDocument(profilesJson);
    if (root.isEmpty()) {
        snapshot.statuses.insert(QStringLiteral("profile"), QStringLiteral("unavailable"));
        snapshot.reasons.insert(QStringLiteral("profile"), QStringLiteral("CorruptState"));
        return snapshot;
    }
    const auto entry = root.value(QStringLiteral("workspaces")).toObject()
                           .value(workspaceId).toObject();
    snapshot.presetId = entry.value(QStringLiteral("presetId")).toString();
    auto merge = [&snapshot](const QJsonObject& values, const QString& source) {
        for (auto it = values.begin(); it != values.end(); ++it) {
            if (it.value().isObject() && snapshot.configured.value(it.key()).isObject()) {
                auto current = snapshot.configured.value(it.key()).toObject();
                auto origins = snapshot.sources.value(it.key()).toObject();
                const auto nested = it.value().toObject();
                for (auto field = nested.begin(); field != nested.end(); ++field) {
                    current.insert(field.key(), field.value());
                    origins.insert(field.key(), source);
                }
                snapshot.configured.insert(it.key(), current);
                snapshot.sources.insert(it.key(), origins);
            } else {
                snapshot.configured.insert(it.key(), it.value());
                if (it.value().isObject()) {
                    QJsonObject origins;
                    const auto nested = it.value().toObject();
                    for (auto field = nested.begin(); field != nested.end(); ++field)
                        origins.insert(field.key(), source);
                    snapshot.sources.insert(it.key(), origins);
                } else snapshot.sources.insert(it.key(), source);
            }
        }
    };
    merge(globalDefaults, QStringLiteral("global"));
    bool foundPreset = snapshot.presetId.isEmpty();
    for (const auto& preset : presets(profilesJson)) {
        if (preset.id != snapshot.presetId) continue;
        merge(preset.preferences, QStringLiteral("preset"));
        foundPreset = true;
        break;
    }
    if (!foundPreset) snapshot.unavailableReferences.append(snapshot.presetId);
    merge(entry.value(QStringLiteral("overrides")).toObject(), QStringLiteral("workspace"));
    merge(sessionOverrides, QStringLiteral("session"));
    snapshot.effective = snapshot.configured;
    const auto providerId = snapshot.configured.value(QStringLiteral("providerId")).toString();
    const auto modelId = snapshot.configured.value(QStringLiteral("modelId")).toString();
    if (models && !providerId.isEmpty()) {
        const bool known = models->isKnownProvider(providerId);
        const auto health = known ? models->providerHealth(providerId) : ProviderHealth::Unavailable;
        snapshot.statuses.insert(QStringLiteral("providerId"),
                                 !known || health == ProviderHealth::Unavailable
                                     ? QStringLiteral("unavailable")
                                     : health == ProviderHealth::Available
                                         ? QStringLiteral("available")
                                         : QStringLiteral("unverified"));
        if (!known) snapshot.unavailableReferences.append(providerId);
        if (!known) snapshot.reasons.insert(QStringLiteral("providerId"),
                                            QStringLiteral("Provider is not registered"));
        else if (health == ProviderHealth::Unavailable) {
            snapshot.unavailableReferences.append(providerId);
            snapshot.reasons.insert(QStringLiteral("providerId"),
                                    QStringLiteral("Provider runtime is unavailable"));
        }
        if (known && !modelId.isEmpty()) {
            const auto status = models->providerStatus(providerId);
            const bool found = status.modelIds.contains(modelId);
            const bool catalogUnknown = status.modelIds.isEmpty() &&
                status.catalog != ProviderCatalogState::Empty;
            snapshot.statuses.insert(QStringLiteral("modelId"),
                                     found ? QStringLiteral("available")
                                     : catalogUnknown ? QStringLiteral("unverified")
                                                      : QStringLiteral("unavailable"));
            if (!found && !catalogUnknown) snapshot.unavailableReferences.append(modelId);
            if (!found && !catalogUnknown) snapshot.reasons.insert(QStringLiteral("modelId"),
                                                QStringLiteral("Model is absent from provider inventory"));
        }
    }
    if (snapshot.configured.value(QStringLiteral("privacy")).toString() ==
        QLatin1String("local-only")) {
        snapshot.statuses.insert(QStringLiteral("privacy"), QStringLiteral("required"));
        if (models && !providerId.isEmpty() &&
            models->currentModelMetadata(providerId, modelId).providerKind == ProviderKind::Cloud) {
            snapshot.statuses.insert(QStringLiteral("providerId"), QStringLiteral("blocked"));
            snapshot.reasons.insert(QStringLiteral("providerId"),
                                    QStringLiteral("Local Only blocks cloud providers"));
            snapshot.unavailableReferences.append(providerId);
        }
    }
    const auto configuredTools = snapshot.configured.value(QStringLiteral("tools")).toObject();
    QJsonObject toolStatus;
    for (auto it = configuredTools.begin(); it != configuredTools.end(); ++it) {
        const auto descriptor = tools ? tools->findToolById(it.key())
                                      : std::optional<ToolDescriptor>{};
        const bool present = descriptor.has_value();
        toolStatus.insert(it.key(), !tools ? QStringLiteral("unverified")
                                  : present && descriptor->enabled ? QStringLiteral("registered")
                                  : present ? QStringLiteral("inactive")
                                            : QStringLiteral("unavailable"));
        if (tools && !present) snapshot.unavailableReferences.append(it.key());
        if (tools && !present) snapshot.reasons.insert(it.key(), QStringLiteral("Tool is not registered"));
        else if (present && !descriptor->enabled)
            snapshot.reasons.insert(it.key(), QStringLiteral("Tool is globally disabled"));
    }
    snapshot.statuses.insert(QStringLiteral("tools"), toolStatus);
    QJsonObject extensionStatus;
    const auto configuredExtensions = snapshot.configured.value(QStringLiteral("extensions")).toObject();
    const auto inventory = extensions ? extensions->extensions() : QList<ExtensionSnapshot>{};
    for (auto it = configuredExtensions.begin(); it != configuredExtensions.end(); ++it) {
        auto found = std::find_if(inventory.cbegin(), inventory.cend(),
                                  [&it](const ExtensionSnapshot& entry) { return entry.id == it.key(); });
        const auto status = !extensions ? QStringLiteral("unverified")
                          : found == inventory.cend() ? QStringLiteral("unavailable")
                          : found->available ? QStringLiteral("available")
                                             : QStringLiteral("inactive");
        extensionStatus.insert(it.key(), status);
        if (status == QLatin1String("unavailable")) snapshot.unavailableReferences.append(it.key());
        if (status == QLatin1String("unavailable"))
            snapshot.reasons.insert(it.key(), QStringLiteral("Extension is not discovered"));
        else if (status == QLatin1String("inactive"))
            snapshot.reasons.insert(it.key(), QStringLiteral("Extension is disabled or not ready"));
    }
    snapshot.statuses.insert(QStringLiteral("extensions"), extensionStatus);
    auto speechStatus = [&snapshot](const QString& key, const QString& configured,
                                     const QString& actual, const SpeechProviderInfo* info) {
        if (configured.isEmpty()) return;
        const auto status = !info ? QStringLiteral("unverified")
            : configured != actual ? QStringLiteral("unavailable")
            : info->runtimeAvailable && info->modelAvailable ? QStringLiteral("available")
                                                              : QStringLiteral("unavailable");
        snapshot.statuses.insert(key, status);
        if (status == QLatin1String("unavailable")) {
            snapshot.unavailableReferences.append(configured);
            snapshot.reasons.insert(key, QStringLiteral("Speech runtime or asset is unavailable"));
        }
    };
    speechStatus(QStringLiteral("sttProviderId"),
                 snapshot.configured.value(QStringLiteral("sttProviderId")).toString(),
                 stt ? stt->id : QString{}, stt);
    speechStatus(QStringLiteral("sttModelId"),
                 snapshot.configured.value(QStringLiteral("sttModelId")).toString(),
                 stt ? stt->modelId : QString{}, stt);
    speechStatus(QStringLiteral("ttsProviderId"),
                 snapshot.configured.value(QStringLiteral("ttsProviderId")).toString(),
                 tts ? tts->id : QString{}, tts);
    if (snapshot.configured.value(QStringLiteral("privacy")).toString() ==
        QLatin1String("local-only")) {
        for (const auto& item : {qMakePair(QStringLiteral("sttProviderId"), stt),
                                 qMakePair(QStringLiteral("ttsProviderId"), tts)}) {
            const auto configured = snapshot.configured.value(item.first).toString();
            if (!configured.isEmpty() && item.second && !item.second->local) {
                snapshot.statuses.insert(item.first, QStringLiteral("blocked"));
                snapshot.reasons.insert(item.first,
                                        QStringLiteral("Local Only blocks cloud speech providers"));
                snapshot.unavailableReferences.append(configured);
            }
        }
    }
    const auto voice = snapshot.configured.value(QStringLiteral("ttsVoiceId")).toString();
    if (!voice.isEmpty()) {
        const auto status = !tts ? QStringLiteral("unverified")
            : tts->voices.contains(voice) && tts->runtimeAvailable
                ? QStringLiteral("available") : QStringLiteral("unavailable");
        snapshot.statuses.insert(QStringLiteral("ttsVoiceId"), status);
        if (status == QLatin1String("unavailable")) {
            snapshot.unavailableReferences.append(voice);
            snapshot.reasons.insert(QStringLiteral("ttsVoiceId"),
                                    QStringLiteral("Voice is absent from the active TTS runtime"));
        }
    }
    return snapshot;
}

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
            QStringLiteral(
                "Data boundary: settings, chat history, memory, and RAG storage remain separate"),
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
        return {false,
                {},
                catalogJson,
                QStringLiteral("Refused"),
                QStringLiteral("Workspace name is required.")};
    }

    auto custom = customWorkspaces(catalogJson);
    const auto id = stableWorkspaceId(normalizedName, normalizedTemplate, catalogJson);
    auto workspace = makeWorkspace(id, normalizedName, QStringLiteral("Custom workspace"),
                                   normalizedTemplate);
    workspace.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    workspace.updatedAt = workspace.createdAt;
    custom.append(workspace);
    return {true, id, encodeCustomWorkspaces(custom), QStringLiteral("Created"),
            QStringLiteral("Created workspace %1 from %2 template.")
                .arg(normalizedName, normalizedTemplate)};
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
            workspace.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
            return {true, id, encodeCustomWorkspaces(custom), QStringLiteral("Renamed"),
                    QStringLiteral("Renamed workspace to %1.").arg(normalizedName)};
        }
    }
    return {false, id, catalogJson, QStringLiteral("Refused"),
            QStringLiteral("Workspace was not found.")};
}

WorkspaceMutationResult WorkspaceService::setWorkspaceRoot(const QString& catalogJson,
                                                            const QString& workspaceId,
                                                            const QString& rootPath) const {
    if (isBuiltInId(workspaceId))
        return {false, workspaceId, catalogJson, QStringLiteral("Refused"),
                QStringLiteral("Built-in workspace roots cannot be changed.")};
    QString canonical;
    if (!rootPath.trimmed().isEmpty()) {
        const QFileInfo info(rootPath.trimmed());
        if (!info.isDir() || !info.isReadable())
            return {false, workspaceId, catalogJson, QStringLiteral("Refused"),
                    QStringLiteral("Workspace root must be an existing readable directory.")};
        canonical = info.canonicalFilePath();
        if (canonical.isEmpty())
            return {false, workspaceId, catalogJson, QStringLiteral("Refused"),
                    QStringLiteral("Workspace root could not be canonicalized.")};
    }
    auto custom = customWorkspaces(catalogJson);
    for (auto& workspace : custom) {
        if (workspace.id != workspaceId) continue;
        workspace.rootPath = canonical;
        workspace.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
        return {true, workspaceId, encodeCustomWorkspaces(custom), QStringLiteral("Updated"),
                QStringLiteral("Workspace root identity updated; permissions are unchanged.")};
    }
    return {false, workspaceId, catalogJson, QStringLiteral("Refused"),
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
            workspace.updatedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
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
    auto copy = makeWorkspace(id, duplicateName, QStringLiteral("Custom workspace"),
                              source.templateName, source.archived);
    copy.rootPath = source.rootPath;
    copy.description = source.description;
    copy.createdAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    copy.updatedAt = copy.createdAt;
    custom.append(copy);
    return {true, id, encodeCustomWorkspaces(custom), QStringLiteral("Duplicated"),
            QStringLiteral("Duplicated workspace %1.").arg(source.name)};
}

QString workspaceSummary(const WorkspaceMetadata& workspace) {
    return QStringLiteral("%1 / %2 / %3 / %4")
        .arg(workspace.name, workspace.templateName, workspace.accessState, workspace.ragSummary);
}

} // namespace sentinel::core
