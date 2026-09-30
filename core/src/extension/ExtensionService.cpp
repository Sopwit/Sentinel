// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/extension/ExtensionService.h"
#include "sentinel/core/plugin/PluginHostProtocol.h"
#include <algorithm>
#include <QRegularExpression>
#include <QUrl>
#include <QTimer>

namespace {
QString safeDetail(QString value) {
    value.replace(QRegularExpression(QStringLiteral("https?://\\S+")),
                  QStringLiteral("[redacted URL]"));
    value.replace(QRegularExpression(QStringLiteral("[\\x00-\\x1f\\x7f]")), QLatin1String(" "));
    return value.left(300);
}
}

namespace sentinel::core {

void ExtensionService::setWorkspacePreferences(const QString& workspaceId,
                                               const QJsonObject& preferences) {
    if (activeWorkspaceId_ == workspaceId && workspacePreferences_ == preferences) return;
    activeWorkspaceId_ = workspaceId;
    workspacePreferences_ = preferences;
    if (skills_) {
        skills_->setActiveWorkspaceId(workspaceId);
        skills_->setWorkspacePreferences(preferences);
    }
    emit extensionsChanged();
}

ExtensionService::ExtensionService(McpService* mcp, plugin::PluginManager* plugins,
                                   SkillService* skills, const IToolRegistry* registry,
                                   QObject* parent)
    : QObject(parent), plugins_(plugins), skills_(skills), registry_(registry) {
    setMcpService(mcp);
    if (plugins_) {
        connect(plugins_, &plugin::PluginManager::pluginDiscovered, this, &ExtensionService::extensionsChanged);
        connect(plugins_, &plugin::PluginManager::pluginStateChanged, this, &ExtensionService::extensionsChanged);
        connect(plugins_, &plugin::PluginManager::pluginError, this, &ExtensionService::extensionsChanged);
        connect(plugins_, &plugin::PluginManager::pluginReloaded, this, &ExtensionService::extensionsChanged);
        connect(plugins_, &plugin::PluginManager::pluginRemoved, this, &ExtensionService::extensionsChanged);
        connect(plugins_, &plugin::PluginManager::pluginStateChanged, this,
                [this] { QTimer::singleShot(0, this, [this] { refreshSkillRequirements(); }); });
        connect(plugins_, &plugin::PluginManager::pluginRemoved, this,
                [this] { QTimer::singleShot(0, this, [this] { refreshSkillRequirements(); }); });
    }
    if (skills_) {
        connect(skills_, &SkillService::skillAdded, this, &ExtensionService::extensionsChanged);
        connect(skills_, &SkillService::skillRemoved, this, &ExtensionService::extensionsChanged);
        connect(skills_, &SkillService::skillUpdated, this, &ExtensionService::extensionsChanged);
    }
    refreshSkillRequirements();
}

QJsonArray ExtensionService::pluginCredentialStates(const QString& pluginId) const {
    QJsonArray result;
    if (!plugins_) return result;
    for (const auto& state : plugins_->credentialStates(pluginId))
        result.append(QJsonObject{{QStringLiteral("id"), state.declaration.id},
            {QStringLiteral("ownerId"), state.pluginId},
            {QStringLiteral("labelId"), state.declaration.labelId},
            {QStringLiteral("kind"), state.declaration.kind},
            {QStringLiteral("required"), state.declaration.required},
            {QStringLiteral("configured"), state.configured},
            {QStringLiteral("secureStoreAvailable"), state.storeAvailable}});
    return result;
}

bool ExtensionService::setPluginCredential(const QString& pluginId,
                                            const QString& credentialId, const QString& value) {
    if (!plugins_ || !plugins_->setCredential(pluginId, credentialId, value)) return false;
    emit extensionsChanged();
    return true;
}

bool ExtensionService::clearPluginCredential(const QString& pluginId,
                                              const QString& credentialId) {
    if (!plugins_ || !plugins_->clearCredential(pluginId, credentialId)) return false;
    emit extensionsChanged();
    return true;
}

void ExtensionService::refreshSkillRequirements() {
    if (!skills_ || !registry_)
        return;
    QStringList ids;
    QStringList capabilities;
    for (const auto& tool : registry_->enabledTools()) {
        ids.append(tool.id);
        if (tool.source == ToolSource::MCP) capabilities.append(QStringLiteral("mcp"));
        else if (tool.source == ToolSource::Plugin) capabilities.append(QStringLiteral("plugin"));
    }
    QStringList providers;
    if (model_) {
        for (const auto& id : model_->knownProviderIds())
            if (model_->providerHealth(id) == ProviderHealth::Available)
                providers.append(id);
        const auto selected = model_->selectedModel();
        const auto caps = model_->capabilities(selected.providerId, selected.modelId);
        if (caps.streaming == CapabilitySupport::Supported) capabilities.append(QStringLiteral("streaming"));
        if (caps.structuredOutput == CapabilitySupport::Supported) capabilities.append(QStringLiteral("structuredOutput"));
        if (caps.nativeToolCalling == CapabilitySupport::Supported) capabilities.append(QStringLiteral("nativeToolCalling"));
        if (caps.visionInput == CapabilitySupport::Supported) capabilities.append(QStringLiteral("visionInput"));
        if (caps.audioInput == CapabilitySupport::Supported) capabilities.append(QStringLiteral("audioInput"));
        if (caps.audioOutput == CapabilitySupport::Supported) capabilities.append(QStringLiteral("audioOutput"));
    }
    skills_->setAvailableRequirements(ids, providers, capabilities);
}

void ExtensionService::setModelService(ModelService* model) {
    if (model_) disconnect(model_, nullptr, this, nullptr);
    model_ = model;
    if (model_) {
        connect(model_, &ModelService::selectedModelChanged, this, &ExtensionService::refreshSkillRequirements);
        connect(model_, &ModelService::providerRegistryChanged, this, &ExtensionService::refreshSkillRequirements);
        connect(model_, &ModelService::providerHealthChanged, this, &ExtensionService::refreshSkillRequirements);
        connect(model_, &ModelService::modelCapabilitiesChanged, this, &ExtensionService::refreshSkillRequirements);
    }
    refreshSkillRequirements();
}

void ExtensionService::setMcpService(McpService* mcp) {
    if (mcp_)
        disconnect(mcp_, nullptr, this, nullptr);
    mcp_ = mcp;
    if (mcp_) {
        connect(mcp_, &McpService::serverAdded, this, &ExtensionService::extensionsChanged);
        connect(mcp_, &McpService::serverRemoved, this, &ExtensionService::extensionsChanged);
        connect(mcp_, &McpService::serverConfigChanged, this, &ExtensionService::extensionsChanged);
        connect(mcp_, &McpService::serverConnected, this, &ExtensionService::extensionsChanged);
        connect(mcp_, &McpService::serverDisconnected, this, &ExtensionService::extensionsChanged);
        connect(mcp_, &McpService::serverError, this, &ExtensionService::extensionsChanged);
        connect(mcp_, &McpService::toolsUpdated, this, &ExtensionService::extensionsChanged);
        connect(mcp_, &McpService::resourcesUpdated, this, &ExtensionService::extensionsChanged);
        connect(mcp_, &McpService::interactionRequested, this, &ExtensionService::pendingInteractionsChanged);
        connect(mcp_, &McpService::interactionResolved, this, &ExtensionService::pendingInteractionsChanged);
        connect(mcp_, &McpService::toolsUpdated, this,
                [this] { QTimer::singleShot(0, this, [this] { refreshSkillRequirements(); }); });
        connect(mcp_, &McpService::serverDisconnected, this,
                [this] { QTimer::singleShot(0, this, [this] { refreshSkillRequirements(); }); });
        connect(mcp_, &McpService::serverError, this,
                [this] { QTimer::singleShot(0, this, [this] { refreshSkillRequirements(); }); });
    }
    emit extensionsChanged();
    emit pendingInteractionsChanged();
}

QList<McpInteractionRequest> ExtensionService::pendingInteractions() const {
    return mcp_ ? mcp_->pendingInteractions() : QList<McpInteractionRequest>{};
}

bool ExtensionService::respondToInteraction(const QString& id, quint64 generation,
                                            McpInteractionDecision decision, const QJsonValue& value) {
    return mcp_ && mcp_->respondToInteraction(id, generation, decision, value);
}

int ExtensionService::toolCount(ToolSource source, const QString& providerId) const {
    if (!registry_)
        return 0;
    int count = 0;
    for (const auto& tool : registry_->enabledTools())
        if (tool.source == source && tool.providerId == providerId)
            ++count;
    return count;
}

QList<ExtensionSnapshot> ExtensionService::extensions() const {
    QList<ExtensionSnapshot> result;
    if (mcp_) for (const auto& config : mcp_->servers()) {
        ExtensionSnapshot item;
        item.id = QStringLiteral("mcp:%1").arg(config.name);
        item.type = ExtensionType::MCP;
        item.displayName = config.name;
        if (config.type == QLatin1String("remote")) {
            QUrl source(config.url);
            source.setUserInfo(QString());
            source.setQuery(QString());
            source.setFragment(QString());
            item.source = source.toString();
        } else item.source = config.command;
        item.enabled = config.enabled;
        item.enabledPreference = config.enabled;
        item.transport = config.type;
        if (config.type == QLatin1String("remote"))
            item.requestedPermissions.append(QStringLiteral("network:%1")
                .arg(QUrl(config.url).host()));
        item.registeredToolCount = toolCount(ToolSource::MCP, item.id);
        item.resourceCount = mcp_->resources(config.name).size();
        for (const auto& tool : mcp_->lastKnownTools(config.name))
            item.lastKnownTools.append(tool.name);
        for (const auto& resource : mcp_->lastKnownResources(config.name))
            item.lastKnownResources.append(resource.uri);
        item.failureCategory = mcpFailureCategoryName(mcp_->failureCategory(config.name));
        item.failureDetail = safeDetail(mcp_->lastError(config.name));
        const auto state = mcp_->connectionState(config.name);
        item.connectionState = state == McpConnectionState::Connected ? QStringLiteral("Ready")
            : state == McpConnectionState::Connecting ? QStringLiteral("Connecting")
            : state == McpConnectionState::Error ? QStringLiteral("Failed")
            : !config.enabled ? QStringLiteral("Disabled") : QStringLiteral("Disconnected");
        if (config.type == QLatin1String("remote"))
            item.sessionState = state == McpConnectionState::Connected
                ? (mcp_->hasRemoteSession(config.name) ? QStringLiteral("Active")
                                                    : QStringLiteral("Stateless"))
                : QStringLiteral("Inactive");
        item.health = !config.enabled ? ExtensionHealth::Disabled
                    : state == McpConnectionState::Connecting ? ExtensionHealth::Connecting
                    : state == McpConnectionState::Connected ? ExtensionHealth::Ready
                    : state == McpConnectionState::Error &&
                        (!item.lastKnownTools.isEmpty() || !item.lastKnownResources.isEmpty())
                        ? ExtensionHealth::Degraded
                    : state == McpConnectionState::Error ? ExtensionHealth::Failed
                    : ExtensionHealth::Disconnected;
        item.maturity = !config.enabled ? ExtensionMaturity::Disabled
                      : state == McpConnectionState::Error ? ExtensionMaturity::Failed
                      : ExtensionMaturity::Experimental;
        item.metadataStale = state != McpConnectionState::Connected &&
            (!item.lastKnownTools.isEmpty() || !item.lastKnownResources.isEmpty());
        item.available = config.enabled && state == McpConnectionState::Connected;
        item.availableActions = {ExtensionAction::Remove};
        if (config.enabled) {
            item.availableActions.append(ExtensionAction::Disable);
            item.availableActions.append(state == McpConnectionState::Connected
                                         ? ExtensionAction::Disconnect : ExtensionAction::Connect);
            if (state == McpConnectionState::Connected)
                item.availableActions.append(ExtensionAction::Refresh);
            if (state == McpConnectionState::Error || state == McpConnectionState::Connected)
                item.availableActions.append(ExtensionAction::Reconnect);
        } else item.availableActions.append(ExtensionAction::Enable);
        result.append(item);
    }
    if (plugins_) for (const auto& id : plugins_->registeredPluginIds()) {
        const auto* descriptor = plugins_->descriptor(id);
        if (!descriptor) continue;
        ExtensionSnapshot item;
        item.id = QStringLiteral("plugin:%1").arg(id);
        item.type = ExtensionType::Plugin;
        item.displayName = descriptor->manifest.name;
        item.source = descriptor->pluginFilePath;
        item.version = descriptor->manifest.version;
        item.transport = QStringLiteral("isolated-process");
        item.executionClass = QStringLiteral("isolated-user");
        item.isolationMode = QStringLiteral("per-plugin-process");
        item.nativeAbiVersion = plugin::NativePluginAbiVersion;
        item.hostProtocolVersion = plugin::PluginHostProtocolVersion;
        item.declaredCredentialCount = descriptor->manifest.credentials.size();
        for (const auto& credential : descriptor->manifest.credentials)
            if (!credential.allowedHosts.isEmpty() &&
                descriptor->manifest.hostCapabilities.contains(QStringLiteral("NetworkRequest")))
                item.brokeredCredentialSupport = true;
        item.rawScopedCredentialFallback = false;
        item.credentialMode = item.brokeredCredentialSupport ? QStringLiteral("Brokered")
                                                             : QStringLiteral("None");
        item.requestedHostCapabilities = descriptor->manifest.hostCapabilities;
        item.brokeredHostCapabilities = descriptor->manifest.hostCapabilities;
        item.connectionState = descriptor->host && descriptor->host->isRunning()
            ? QStringLiteral("Running") : QStringLiteral("Stopped");
        item.sessionState = item.connectionState;
        item.sandboxStatus = descriptor->host && descriptor->host->isRunning()
            ? QStringLiteral("Enforced") : QStringLiteral("Unavailable");
        item.publisher = descriptor->manifest.vendor;
        item.requestedPermissions = descriptor->manifest.permissions.toList();
        const auto granted = plugins_->sandbox().getPermissions(id);
        for (const auto& permission : item.requestedPermissions)
            if (!granted.has(permission))
                item.requirements.append(QStringLiteral("hostPermission:%1").arg(permission));
        item.registeredToolCount = toolCount(ToolSource::Plugin, item.id);
        item.enabled = descriptor->state != plugin::PluginState::Disabled;
        item.enabledPreference = item.enabled;
        item.health = descriptor->state == plugin::PluginState::Disabled ? ExtensionHealth::Disabled
                    : descriptor->state == plugin::PluginState::Error ? ExtensionHealth::Failed
                    : !descriptor->lastCapabilityFailure.isEmpty() ? ExtensionHealth::Degraded
                    : !item.requirements.isEmpty() ? ExtensionHealth::Degraded
                    : descriptor->state == plugin::PluginState::Active ? ExtensionHealth::Ready
                    : ExtensionHealth::Disconnected;
        item.maturity = item.health == ExtensionHealth::Disabled ? ExtensionMaturity::Disabled
                      : item.health == ExtensionHealth::Failed ? ExtensionMaturity::Failed
                      : ExtensionMaturity::Experimental;
        item.available = item.health == ExtensionHealth::Ready;
        item.failureCategory = !descriptor->failureCategory.isEmpty()
                                   ? descriptor->failureCategory
                                   : !descriptor->lastCapabilityFailure.isEmpty()
                                   ? descriptor->lastCapabilityFailure
                                   : !item.requirements.isEmpty()
                                   ? QStringLiteral("PermissionPending")
                                   : descriptor->state == plugin::PluginState::Error
                                       ? QStringLiteral("PluginFailure") : QString();
        item.failureDetail = safeDetail(descriptor->errorString);
        item.availableActions = item.enabled
            ? QList<ExtensionAction>{ExtensionAction::Disable, ExtensionAction::Reload}
            : QList<ExtensionAction>{ExtensionAction::Enable};
        result.append(item);
    }
    if (skills_) for (const auto& skill : skills_->skills()) {
        ExtensionSnapshot item;
        item.id = QStringLiteral("skill:%1").arg(skill.name);
        item.type = ExtensionType::Skill;
        item.displayName = skill.name;
        if (skill.sourceType == SkillSourceType::Url) {
            QUrl source(skill.sourceLocation);
            source.setUserInfo(QString());
            source.setQuery(QString());
            source.setFragment(QString());
            item.source = source.toString();
        } else item.source = skill.sourceLocation;
        item.version = skill.version;
        item.publisher = skill.author;
        item.workspaceId = skill.workspaceId;
        item.scope = skill.scope == SkillScope::Workspace ? ExtensionScope::Workspace
                   : skill.scope == SkillScope::GlobalWithWorkspaceOverride ? ExtensionScope::GlobalWithWorkspaceOverride
                   : ExtensionScope::Global;
        item.enabled = skill.enabledPreference;
        item.enabledPreference = skill.enabledPreference;
        item.health = skill.state == SkillState::Disabled ? ExtensionHealth::Disabled
                    : skill.state == SkillState::Failed ? ExtensionHealth::Failed
                    : skill.state == SkillState::Incompatible ? ExtensionHealth::Incompatible
                    : skill.state == SkillState::MissingRequirements ? ExtensionHealth::Degraded
                    : ExtensionHealth::Ready;
        item.maturity = item.health == ExtensionHealth::Disabled ? ExtensionMaturity::Disabled
                      : item.health == ExtensionHealth::Failed ? ExtensionMaturity::Failed
                      : item.health == ExtensionHealth::Incompatible ? ExtensionMaturity::Incompatible
                      : ExtensionMaturity::Experimental;
        item.available = skill.state == SkillState::Enabled;
        for (const auto& tool : skill.requiredTools)
            item.requirements.append(QStringLiteral("tool:%1").arg(tool));
        for (const auto& provider : skill.requiredProviders)
            item.requirements.append(QStringLiteral("provider:%1").arg(provider));
        for (const auto& capability : skill.requiredCapabilities)
            item.requirements.append(QStringLiteral("capability:%1").arg(capability));
        item.failureCategory = skill.state == SkillState::MissingRequirements
                                   ? QStringLiteral("MissingRequirements") : QString();
        item.failureDetail = safeDetail(skill.state == SkillState::MissingRequirements
                                  ? skill.missingRequirements.join(QStringLiteral(", "))
                                  : skill.lastError);
        item.availableActions = item.enabled
            ? QList<ExtensionAction>{ExtensionAction::Disable, ExtensionAction::Remove}
            : QList<ExtensionAction>{ExtensionAction::Enable, ExtensionAction::Remove};
        if (skill.sourceType == SkillSourceType::Directory ||
            (skill.sourceType == SkillSourceType::Url && !skill.indexSource.isEmpty()))
            item.availableActions.append(ExtensionAction::Refresh);
        result.append(item);
    }
    for (auto& item : result) {
        item.globallyEnabled = item.enabledPreference;
        item.workspaceId = activeWorkspaceId_;
        const auto preference = workspacePreferences_.value(item.id);
        item.workspacePreference = preference.isBool()
            ? (preference.toBool() ? QStringLiteral("enabled") : QStringLiteral("disabled"))
            : QStringLiteral("inherit");
        item.effectiveEnabled = item.globallyEnabled &&
            item.workspacePreference != QLatin1String("disabled");
        item.available = item.available && item.effectiveEnabled;
    }
    return result;
}

QList<ExtensionSnapshot> ExtensionService::filter(std::optional<ExtensionType> type,
                                                  std::optional<ExtensionHealth> health) const {
    QList<ExtensionSnapshot> result;
    for (const auto& item : extensions())
        if ((!type || item.type == *type) && (!health || item.health == *health))
            result.append(item);
    return result;
}

bool ExtensionService::perform(const QString& id, ExtensionAction action) {
    const auto snapshot = extensions();
    auto it = std::find_if(snapshot.cbegin(), snapshot.cend(), [&](const auto& item) { return item.id == id; });
    if (it == snapshot.cend() || !it->availableActions.contains(action))
        return false;
    const QString nativeId = id.mid(id.indexOf(QLatin1Char(':')) + 1);
    if (it->type == ExtensionType::MCP && mcp_) {
        if (action == ExtensionAction::Connect) return mcp_->connectToServer(nativeId);
        if (action == ExtensionAction::Disconnect) return mcp_->disconnectFromServer(nativeId);
        if (action == ExtensionAction::Refresh) return mcp_->refreshTools(nativeId);
        if (action == ExtensionAction::Reconnect) {
            return mcp_->disconnectFromServer(nativeId) &&
                mcp_->connectToServer(nativeId);
        }
        if (action == ExtensionAction::Remove) return mcp_->removeServer(nativeId);
        if (action == ExtensionAction::Enable || action == ExtensionAction::Disable)
            return mcp_->setEnabled(nativeId, action == ExtensionAction::Enable);
    }
    if (it->type == ExtensionType::Plugin && plugins_) {
        if (action == ExtensionAction::Enable || action == ExtensionAction::Disable)
            return plugins_->setEnabled(nativeId, action == ExtensionAction::Enable);
        if (action == ExtensionAction::Reload) return plugins_->reloadPlugin(nativeId);
    }
    if (it->type == ExtensionType::Skill && skills_) {
        if (action == ExtensionAction::Enable || action == ExtensionAction::Disable)
            return skills_->setEnabled(nativeId, action == ExtensionAction::Enable);
        if (action == ExtensionAction::Remove) return skills_->removeSkill(nativeId);
        if (action == ExtensionAction::Refresh) return skills_->refreshSkill(nativeId);
    }
    return false;
}

} // namespace sentinel::core
