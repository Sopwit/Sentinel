// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QJsonObject>
#include <QString>
#include <QStringList>

namespace sentinel::core {
class ModelService;
class IToolRegistry;
class ExtensionService;
struct SpeechProviderInfo;

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
    QString rootPath;
    QString description;
    QString createdAt;
    QString updatedAt;
};

struct WorkspacePreset {
    QString id;
    QString name;
    bool builtIn = false;
    QJsonObject preferences;
};

struct WorkspaceProfileSnapshot {
    QString workspaceId;
    QString presetId;
    QJsonObject configured;
    QJsonObject effective;
    QJsonObject sources;
    QJsonObject statuses;
    QJsonObject reasons;
    QStringList unavailableReferences;
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
    QList<WorkspacePreset> presets(const QString& profilesJson = {}) const;
    QString createPreset(const QString& profilesJson, const QString& name,
                         const QJsonObject& preferences) const;
    QString renamePreset(const QString& profilesJson, const QString& presetId,
                         const QString& name) const;
    QString updatePreset(const QString& profilesJson, const QString& presetId,
                         const QJsonObject& preferences) const;
    QString duplicatePreset(const QString& profilesJson, const QString& presetId) const;
    QString deletePreset(const QString& profilesJson, const QString& presetId) const;
    QString updateProfile(const QString& profilesJson, const QString& workspaceId,
                          const QString& presetId, const QJsonObject& overrides) const;
    WorkspaceProfileSnapshot resolveProfile(const QString& profilesJson,
        const QString& workspaceId, const QJsonObject& globalDefaults = {},
        const QJsonObject& sessionOverrides = {}, const ModelService* models = nullptr,
        const IToolRegistry* tools = nullptr,
        const ExtensionService* extensions = nullptr,
        const SpeechProviderInfo* stt = nullptr, const SpeechProviderInfo* tts = nullptr) const;
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
    WorkspaceMutationResult setWorkspaceRoot(const QString& catalogJson,
                                             const QString& workspaceId,
                                             const QString& rootPath) const;
    WorkspaceMutationResult archiveWorkspace(const QString& catalogJson,
                                             const QString& workspaceId) const;
    WorkspaceMutationResult deleteWorkspace(const QString& catalogJson, const QString& workspaceId,
                                            const QString& currentWorkspaceId) const;
    WorkspaceMutationResult duplicateWorkspace(const QString& catalogJson,
                                               const QString& workspaceId) const;
};

QString workspaceSummary(const WorkspaceMetadata& workspace);

} // namespace sentinel::core
