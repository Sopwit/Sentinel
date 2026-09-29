// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QString>

// Native ABI v3. Plugins and host must use the same Qt 6 binary ABI. The host owns
// IPluginContext and invocations; the plugin owns its QObject and registered tools.
// Neither side deletes objects allocated by the other. No Sentinel core header is public.
namespace sentinel::plugin_sdk {
inline constexpr int NativeAbiVersion = 3;

enum class PluginState { Unloaded, Initialized, Active };

struct PluginToolDescriptor {
    QString id;
    QString name;
    QString description;
    QString category;
    QString risk = QStringLiteral("medium");
    QJsonObject inputSchema;
    // String-valued domain, access, resourceKind, resourceArgument, staticResource.
    // The host validates and translates them; no internal enum layout crosses ABI.
    QJsonArray authorizationRequirements;
};

struct PluginInvocation {
    QString id;
    QString toolId;
    QJsonObject arguments;
};

struct PluginResult {
    bool ok = false;
    QString summary;
};

class IPluginTool {
public:
    virtual ~IPluginTool() = default;
    // Synchronous v3 call. Cancellation terminates the isolated host process.
    virtual PluginResult execute(const PluginInvocation& invocation) = 0;
};

class IPluginContext {
public:
    virtual ~IPluginContext() = default;
    virtual QString coreVersion() const = 0;
    virtual QString pluginDataDir() const = 0;
    virtual bool hasPermission(const QString& permission) const = 0;
    virtual void logMessage(const QString& level, const QString& message) = 0;
    virtual QJsonObject pluginConfig() const = 0;
    // Available only during execute() on that invocation's thread. Caller owns `value`.
    virtual bool credential(const QString& credentialId, QString* value) const = 0;
    virtual bool registerTool(const PluginToolDescriptor& descriptor, IPluginTool* tool) = 0;
};

class ISentinelPlugin {
public:
    virtual ~ISentinelPlugin() = default;
    virtual QString pluginId() const = 0;
    virtual QString displayName() const = 0;
    virtual QString vendor() const = 0;
    virtual QString version() const = 0;
    virtual QString requiredCoreVersion() const = 0;
    virtual bool initialize(IPluginContext* context) = 0;
    virtual bool start() = 0;
    virtual void stop() = 0;
    virtual void shutdown() = 0;
    virtual PluginState state() const = 0;
    virtual QJsonObject defaultConfig() const = 0;
    virtual void configure(const QJsonObject& config) = 0;
};
} // namespace sentinel::plugin_sdk

#define ISentinelPlugin_iid "dev.sentinel.ISentinelPlugin/3.0"
Q_DECLARE_INTERFACE(sentinel::plugin_sdk::ISentinelPlugin, ISentinelPlugin_iid)
