// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sentinel/core/mcp/McpService.h"
#include "sentinel/core/plugin/PluginManager.h"
#include "sentinel/core/skill/SkillService.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include "sentinel/core/model/ModelService.h"
#include <QObject>
#include <QPointer>
#include <QSet>
#include <QJsonObject>
#include <QJsonArray>
#include <optional>

namespace sentinel::core {

enum class ExtensionType { MCP, Plugin, Skill };
enum class ExtensionHealth { Disabled, Disconnected, Connecting, Ready, Degraded, Failed, Incompatible };
enum class ExtensionMaturity { ProductionReady, Experimental, Disabled, Failed, Incompatible };
enum class ExtensionScope { Global, Workspace, GlobalWithWorkspaceOverride };
enum class ExtensionAction { Enable, Disable, Connect, Disconnect, Reconnect, Refresh, Reload, Remove, OpenSource };

struct ExtensionSnapshot {
    QString id;
    ExtensionType type{ExtensionType::MCP};
    QString displayName;
    QString source;
    QString version;
    QString publisher;
    bool enabled{false};
    bool enabledPreference{false};
    bool globallyEnabled{false};
    bool effectiveEnabled{false};
    QString workspacePreference = QStringLiteral("inherit");
    bool available{false};
    QString transport;
    QString executionClass;
    QString connectionState;
    QString sessionState;
    ExtensionHealth health{ExtensionHealth::Disconnected};
    ExtensionMaturity maturity{ExtensionMaturity::Experimental};
    ExtensionScope scope{ExtensionScope::Global};
    QString workspaceId;
    QStringList requestedPermissions;
    QString isolationMode;
    int nativeAbiVersion{0};
    int hostProtocolVersion{0};
    int declaredCredentialCount{0};
    bool brokeredCredentialSupport{false};
    bool rawScopedCredentialFallback{false};
    QString credentialMode = QStringLiteral("None");
    QStringList requestedHostCapabilities;
    QStringList brokeredHostCapabilities;
    QString sandboxStatus;
    QStringList requirements;
    int registeredToolCount{0};
    int resourceCount{0};
    QStringList lastKnownTools;
    QStringList lastKnownResources;
    QString failureCategory;
    QString failureDetail;
    bool metadataStale{false};
    QList<ExtensionAction> availableActions;
};

// Product state only. Native services own lifecycle and ToolRegistry owns execution inventory.
class ExtensionService final : public QObject {
    Q_OBJECT
public:
    ExtensionService(McpService* mcp, plugin::PluginManager* plugins,
                     SkillService* skills, const IToolRegistry* registry,
                     QObject* parent = nullptr);
    void setMcpService(McpService* mcp);
    void setModelService(ModelService* model);
    void refreshSkillRequirements();
    void setWorkspacePreferences(const QString& workspaceId, const QJsonObject& preferences);
    QList<ExtensionSnapshot> extensions() const;
    QJsonArray pluginCredentialStates(const QString& pluginId) const;
    bool setPluginCredential(const QString& pluginId, const QString& credentialId,
                             const QString& value);
    bool clearPluginCredential(const QString& pluginId, const QString& credentialId);
    QList<ExtensionSnapshot> filter(std::optional<ExtensionType> type,
                                    std::optional<ExtensionHealth> health = {}) const;
    bool perform(const QString& id, ExtensionAction action);
    QList<McpInteractionRequest> pendingInteractions() const;
    bool respondToInteraction(const QString& id, quint64 generation,
                              McpInteractionDecision decision, const QJsonValue& value = {});

signals:
    void extensionsChanged();
    void pendingInteractionsChanged();

private:
    int toolCount(ToolSource source, const QString& providerId) const;
    QPointer<McpService> mcp_;
    QPointer<plugin::PluginManager> plugins_;
    QPointer<SkillService> skills_;
    QPointer<ModelService> model_;
    const IToolRegistry* registry_;
    QString activeWorkspaceId_;
    QJsonObject workspacePreferences_;
};

} // namespace sentinel::core
