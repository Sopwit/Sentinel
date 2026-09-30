// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QString>
#include <QStringList>

// Native ABI v5. Plugins and host must use the same Qt 6 binary ABI. The host owns
// IPluginContext and invocations; the plugin owns its QObject and registered tools.
// Neither side deletes objects allocated by the other. No Sentinel core header is public.
namespace sentinel::plugin_sdk {
inline constexpr int NativeAbiVersion = 5;

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
    // A subset of the owning manifest's host_capabilities.
    QJsonArray hostCapabilities;
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

struct FilesystemReadRequest { QString argument; QString path; };
struct FilesystemReadResult {
    bool ok = false;
    QString category;
    QByteArray content;
    bool truncated = false;
    bool binary = false;
};
struct FilesystemWriteRequest { QString argument; QString path; QByteArray content; };
struct FilesystemWriteResult {
    bool ok = false;
    QString category;
    qint64 bytesWritten = 0;
    bool created = false;
    QString mutationPath;
};
enum class NetworkMethod { Get, Post };
struct NetworkRequest {
    NetworkMethod method = NetworkMethod::Get;
    QString urlArgument;
    QString url;
    QMap<QString, QString> headers;
    QByteArray body;
    int timeoutMs = 10000;
    QString credentialId;
};
struct NetworkResult {
    bool ok = false;
    QString category;
    int status = 0;
    QByteArray body;
};
struct ProcessExecuteRequest {
    QString programArgument;
    QString program;
    QString argumentsArgument;
    QStringList arguments;
    int timeoutMs = 10000;
};
struct ProcessExecuteResult {
    bool ok = false;
    QString category;
    int exitCode = -1;
    QByteArray stdoutData;
    QByteArray stderrData;
};

class IPluginTool {
public:
    virtual ~IPluginTool() = default;
    // Synchronous call. Cancellation terminates the isolated host process.
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
    // Typed operations only. The host and Sentinel independently validate each request.
    virtual FilesystemReadResult filesystemRead(const FilesystemReadRequest& request) const = 0;
    virtual FilesystemWriteResult filesystemWrite(const FilesystemWriteRequest& request) const = 0;
    virtual NetworkResult networkRequest(const NetworkRequest& request) const = 0;
    virtual ProcessExecuteResult processExecute(const ProcessExecuteRequest& request) const = 0;
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

#define ISentinelPlugin_iid "dev.sentinel.ISentinelPlugin/5.0"
Q_DECLARE_INTERFACE(sentinel::plugin_sdk::ISentinelPlugin, ISentinelPlugin_iid)
