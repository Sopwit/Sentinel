// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/app/BackupService.h"

#include "sentinel/core/app/WorkspaceService.h"
#include <QJsonObject>
#include <QStringList>
#include <QVariant>

namespace sentinel::core {
class AppSettings;
class ModelService;
class ExtensionService;
class VoiceSessionService;
class PermissionService;
class IAgentRunStore;
class IChatHistoryStore;
class ModelOperationService;

enum class SettingScope { Global, WorkspaceOverride, Session };
enum class SettingSection { General, Appearance, Models, Agent, Tools, Extensions,
                            Workspaces, Speech, Privacy, Network, Storage,
                            Notifications, Advanced };

struct SettingSnapshot {
    QString id;
    SettingSection section;
    QVariant value;
    QVariant defaultValue;
    QString source = QStringLiteral("global");
    QStringList allowedValues;
    QStringList keywords;
    SettingScope scope = SettingScope::Global;
    bool enabled = true;
    bool restartRequired = false;
    bool sensitive = false;
    QString unavailableReason;
};

struct SettingActionResult {
    bool accepted = false;
    QString code;
    QString field;
    QJsonObject parameters;
};

class SettingsService final {
public:
    SettingsService(AppSettings& settings, ModelService* models = nullptr,
                    ExtensionService* extensions = nullptr,
                    VoiceSessionService* audio = nullptr,
                    const PermissionService* permissions = nullptr,
                    IConversationStore* conversations = nullptr,
                    IMemoryStore* memory = nullptr,
                    IAgentRunStore* agentRuns = nullptr,
                    IChatHistoryStore* chatHistory = nullptr,
                    ModelOperationService* modelOperations = nullptr);
    QList<SettingSnapshot> snapshots() const;
    QStringList sectionIds() const;
    SettingActionResult set(const QString& id, const QVariant& value);
    SettingActionResult reset(const QString& id);
    QList<SettingActionResult> resetSection(SettingSection section);
    SettingActionResult clearWorkspaceOverride(const QString& workspaceId,
                                               const QString& key);
    QJsonObject providerState(const QString& providerId) const;
    QJsonObject extensionState(const QString& extensionId) const;
    SettingActionResult performExtensionAction(const QString& extensionId, int action);
    WorkspaceProfileSnapshot workspaceProfile(const QString& workspaceId) const;
    QJsonObject speechState() const;
    QJsonObject securityState() const;
    QJsonObject privacyState() const;
    QJsonObject networkState() const;
    QJsonObject recoveryState() const;
    QJsonObject backupAvailability() const;
    BackupResult exportBackupJson(const QStringList& domains) const;
    BackupResult importBackupJson(const QByteArray& data, const QStringList& domains,
                                  ImportMode mode);
    SettingActionResult clearChatHistory();
    SettingActionResult clearMemory();
    SettingActionResult clearAgentHistory();
    SettingActionResult clearStaleTemporaryArtifacts();
    QJsonObject clearCacheAndTemporaryData();
    QJsonObject runRetentionMaintenance();
    SettingActionResult resolveInterruptedModelOperation(const QString& operationId);
    SettingActionResult clearCredential(const QString& logicalId);
    SettingActionResult setProviderCredential(const QString& providerId, const QString& value);
    SettingActionResult setPluginCredential(const QString& extensionId,
                                            const QString& credentialId, const QString& value);
    SettingActionResult clearPluginCredential(const QString& extensionId,
                                              const QString& credentialId);

private:
    AppSettings& settings_;
    ModelService* models_;
    ExtensionService* extensions_;
    VoiceSessionService* audio_;
    const PermissionService* permissions_;
    IConversationStore* conversations_;
    IMemoryStore* memory_;
    IAgentRunStore* agentRuns_;
    IChatHistoryStore* chatHistory_;
    ModelOperationService* modelOperations_;
    WorkspaceService workspaces_;
};
} // namespace sentinel::core
