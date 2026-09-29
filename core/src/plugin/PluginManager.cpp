// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginManager.h"
#include "sentinel/core/security/CredentialStore.h"
#include "sentinel/core/plugin/PluginDependencyResolver.h"
#include "sentinel/core/plugin/PluginHostProtocol.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QStandardPaths>
#include <QSet>
#include <QTimer>
#include <QThread>
#include <QPointer>
#include <algorithm>
#include <atomic>

namespace sentinel::core::plugin {

namespace {
int sdkDomain(const QString& value) {
    return QStringList{QStringLiteral("filesystem"), QStringLiteral("process"),
        QStringLiteral("network"), QStringLiteral("clipboard"), QStringLiteral("application"),
        QStringLiteral("system"), QStringLiteral("memory"), QStringLiteral("conversation"),
        QStringLiteral("audio"), QStringLiteral("browser"), QStringLiteral("agent"),
        QStringLiteral("external-service")}.indexOf(value);
}
int sdkAccess(const QString& value) {
    return QStringList{QStringLiteral("read"), QStringLiteral("write"),
        QStringLiteral("delete"), QStringLiteral("execute"),
        QStringLiteral("control"), QStringLiteral("invoke")}.indexOf(value);
}
int sdkResourceKind(const QString& value) {
    return QStringList{QStringLiteral("none"), QStringLiteral("argument"),
        QStringLiteral("argument-digest"), QStringLiteral("filesystem-path"),
        QStringLiteral("host"), QStringLiteral("provider")}.indexOf(value);
}
QStringList hostPermissionsFor(const ToolDescriptor& descriptor) {
    QStringList permissions{Permissions::ToolExecution};
    for (const auto& requirement : descriptor.authorizationRequirements) {
        if (requirement.domain == SecurityDomain::ExternalService &&
            requirement.staticResource.startsWith(QStringLiteral("credential:")) &&
            !permissions.contains(Permissions::CredentialUse))
            permissions.append(Permissions::CredentialUse);
        QString permission;
        switch (requirement.domain) {
        case SecurityDomain::FileSystem:
            permission = requirement.access == AccessMode::Read ? Permissions::FileSystemRead
                                                                 : Permissions::FileSystemWrite;
            break;
        case SecurityDomain::Network:
        case SecurityDomain::Browser:
        case SecurityDomain::ExternalService:
            permission = Permissions::NetworkExternal;
            break;
        case SecurityDomain::Memory:
        case SecurityDomain::Conversation:
            permission = Permissions::DatabaseAccess;
            break;
        default:
            break;
        }
        if (!permission.isEmpty() && !permissions.contains(permission))
            permissions.append(permission);
    }
    return permissions;
}

QString escapedId(const QString& value) {
    QString result;
    for (const auto byte : value.toUtf8()) {
        const auto c = static_cast<unsigned char>(byte);
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            result += QLatin1Char(c);
        else
            result += QStringLiteral("_%1_").arg(c, 2, 16, QLatin1Char('0'));
    }
    return result;
}

class RemotePluginToolHandler final : public IToolHandler {
public:
    RemotePluginToolHandler(std::weak_ptr<PluginHostSession> host,
                            std::shared_ptr<PluginSandbox> sandbox, QString pluginId,
                            QString localId, QStringList permissions)
        : host_(std::move(host)), sandbox_(std::move(sandbox)), pluginId_(std::move(pluginId)),
          localId_(std::move(localId)), permissions_(std::move(permissions)) {}
    IToolExecutor::Cancel execute(const ToolExecutionRequest& request, const QString&,
                                  const QString&, IToolExecutor::Output,
                                  IToolExecutor::Completion completion) override {
        const auto host = host_.lock();
        if (!host ||
            !std::all_of(permissions_.cbegin(), permissions_.cend(),
                         [this](const QString& value) { return sandbox_->checkPermission(pluginId_, value); })) {
            ToolExecutionResult result{ToolExecutionStatus::Blocked, QStringLiteral("Plugin unavailable or permission denied")};
            result.failureCategory = host ? ToolFailureCategory::SecurityDenied : ToolFailureCategory::RuntimeUnavailable;
            completion(result);
            return {};
        }
        if (request.plan.invocations.isEmpty()) {
            completion({ToolExecutionStatus::InvalidArguments, QStringLiteral("Missing plugin invocation")});
            return {};
        }
        QJsonObject arguments;
        for (const auto& argument : request.plan.invocations.first().arguments)
            arguments.insert(argument.id, argument.jsonValue.isUndefined()
                ? QJsonValue(argument.value) : argument.jsonValue);
        auto requestId = std::make_shared<QString>();
        auto cancelled = std::make_shared<std::atomic_bool>(false);
        QPointer<QObject> callbackContext = request.callbackContext;
        QMetaObject::invokeMethod(host.get(), [host, localId = localId_, arguments,
            requestId, cancelled, callbackContext, completion = std::move(completion)]() mutable {
          *requestId = host->invoke(localId, arguments,
            [callbackContext, completion = std::move(completion)](QJsonObject response) mutable {
                auto deliver = [completion = std::move(completion), response]() mutable {
                ToolExecutionResult result;
                result.status = response.value(QStringLiteral("ok")).toBool()
                    ? ToolExecutionStatus::Succeeded : ToolExecutionStatus::Failed;
                result.summary = response.value(QStringLiteral("summary")).toString().left(65536);
                if (result.status != ToolExecutionStatus::Succeeded) {
                    const auto category = response.value(QStringLiteral("category")).toString();
                    result.summary = category;
                    result.failureCategory = category == QStringLiteral("PluginTimeout")
                        ? ToolFailureCategory::Timeout : category == QStringLiteral("PluginCancelled")
                        ? ToolFailureCategory::Cancelled : category == QStringLiteral("PluginProtocolError")
                        ? ToolFailureCategory::ProtocolError :
                        (category == QStringLiteral("PluginCredentialDenied") ||
                         category == QStringLiteral("PluginCredentialNotDeclared") ||
                         category == QStringLiteral("PluginHostCapabilityDenied") ||
                         category == QStringLiteral("PluginHostCapabilityNotDeclared"))
                        ? ToolFailureCategory::SecurityDenied : ToolFailureCategory::RuntimeUnavailable;
                }
                completion(std::move(result));
                };
                if (callbackContext) QMetaObject::invokeMethod(callbackContext.data(), std::move(deliver), Qt::QueuedConnection);
                else deliver();
            });
          if (cancelled->load() && !requestId->isEmpty()) host->cancel(*requestId);
        }, Qt::QueuedConnection);
        return [host, requestId, cancelled] {
            cancelled->store(true);
            QMetaObject::invokeMethod(host.get(), [host, requestId] {
                if (!requestId->isEmpty()) host->cancel(*requestId);
            }, Qt::QueuedConnection);
        };
    }
private:
    std::weak_ptr<PluginHostSession> host_;
    std::shared_ptr<PluginSandbox> sandbox_;
    QString pluginId_;
    QString localId_;
    QStringList permissions_;
};
} // namespace

PluginManager::PluginManager(QString coreVersion, QString pluginStorageDir, QObject* parent)
    : QObject(parent), m_coreVersion(std::move(coreVersion)),
      m_pluginStorageDir(std::move(pluginStorageDir)) {
    if (m_pluginStorageDir.isEmpty()) {
        m_pluginStorageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                             QStringLiteral("/plugins");
    }
}

PluginManager::~PluginManager() {
    unloadAll();
}

void PluginManager::setPluginStorageDir(const QString& dir) {
    m_pluginStorageDir = dir;
}

QString PluginManager::pluginStorageDir() const {
    return m_pluginStorageDir;
}

PluginSandbox& PluginManager::sandbox() {
    return *m_sandbox;
}

const PluginSandbox& PluginManager::sandbox() const {
    return *m_sandbox;
}

void PluginManager::setToolRegistry(IToolRegistry* registry) {
    m_toolRegistry = registry;
}

QList<PluginCredentialState> PluginManager::credentialStates(const QString& pluginId) const {
    QList<PluginCredentialState> states;
    const auto it = m_plugins.constFind(pluginId);
    if (it == m_plugins.cend()) return states;
    auto store = defaultCredentialStore();
    for (const auto& declaration : it->manifest.credentials) {
        const auto result = store.containsCredential(
            {QStringLiteral("plugin.") + pluginId, declaration.id});
        states.append({pluginId, declaration, result.succeeded,
                       store.summary().status == CredentialStoreStatus::Ready});
    }
    return states;
}

bool PluginManager::setCredential(const QString& pluginId, const QString& credentialId,
                                  const QString& value) {
    const auto it = m_plugins.constFind(pluginId);
    if (it == m_plugins.cend() || value.isEmpty() || value.size() > 8192) return false;
    for (const auto& declaration : it->manifest.credentials) {
        if (declaration.id != credentialId) continue;
        auto store = defaultCredentialStore();
        const CredentialKey key{QStringLiteral("plugin.") + pluginId, credentialId};
        if (!store.storeCredential(key, value).succeeded) return false;
        const auto readback = store.readCredential(key);
        return readback.result.succeeded && readback.secret == value;
    }
    return false;
}

bool PluginManager::clearCredential(const QString& pluginId, const QString& credentialId) {
    const auto it = m_plugins.constFind(pluginId);
    if (it == m_plugins.cend()) return false;
    for (const auto& declaration : it->manifest.credentials) {
        if (declaration.id == credentialId)
            return defaultCredentialStore().deleteCredential(
                {QStringLiteral("plugin.") + pluginId, credentialId}).succeeded;
    }
    return false;
}

int PluginManager::discoverPlugins(const QString& searchDir) {
    QString targetDir = searchDir.isEmpty() ? m_pluginStorageDir : searchDir;
    QDir dir(targetDir);
    const QString absoluteRoot = QFileInfo(targetDir).absoluteFilePath();
    if (!dir.exists()) {
        for (auto it = m_plugins.begin(); it != m_plugins.end();) {
            if (it->pluginFilePath == absoluteRoot ||
                it->pluginFilePath.startsWith(absoluteRoot + QDir::separator())) {
                const auto id = it.key();
                unloadPlugin(id);
                m_sandbox->clearPlugin(id);
                it = m_plugins.erase(it);
                emit pluginRemoved(id);
            } else ++it;
        }
        m_orderedIds = m_plugins.keys();
        return 0;
    }

    int discoveredCount = 0;
    QSet<QString> seenIds;
    const QString root = absoluteRoot + QDir::separator();

    // 1. Search for directory-based plugins containing plugin.json
    QDirIterator it(targetDir, QStringList() << QStringLiteral("plugin.json"), QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString manifestPath = it.filePath();
        QString error;
        PluginManifest manifest = PluginManifest::parseFile(manifestPath, &error);

        if (manifest.isValid()) {
            seenIds.insert(manifest.id);
            if (m_plugins.contains(manifest.id) &&
                m_plugins.value(manifest.id).state != PluginState::Unloaded)
                continue;
            PluginDescriptor desc;
            desc.manifest = manifest;
            desc.pluginFilePath = QFileInfo(manifestPath).absolutePath(); // directory
            if (!manifest.isCompatibleWithCore(m_coreVersion) ||
                manifest.apiVersion != QStringLiteral("3.0")) {
                desc.state = PluginState::Error;
                desc.failureCategory = QStringLiteral("PluginIncompatible");
                desc.errorString = desc.failureCategory;
            }

            m_sandbox->registerPluginPermissions(manifest.id, manifest.permissions);
            m_plugins[manifest.id] = desc;
            emit pluginDiscovered(manifest.id);
            discoveredCount++;
        }
    }

    // Native plugins require an explicit directory manifest. Binary metadata is never
    // inspected through a loader in the main process.

    for (auto pluginIt = m_plugins.begin(); pluginIt != m_plugins.end();) {
        if ((pluginIt->pluginFilePath == absoluteRoot || pluginIt->pluginFilePath.startsWith(root)) &&
            !seenIds.contains(pluginIt.key())) {
            const auto id = pluginIt.key();
            unloadPlugin(id);
            m_sandbox->clearPlugin(id);
            pluginIt = m_plugins.erase(pluginIt);
            emit pluginRemoved(id);
        } else ++pluginIt;
    }

    // Re-resolve load order
    QList<PluginManifest> manifests;
    for (const auto& desc : m_plugins) {
        manifests.append(desc.manifest);
    }
    ResolutionResult res = PluginDependencyResolver::resolve(manifests);
    if (res.success) {
        m_orderedIds = res.loadOrder;
    } else {
        qWarning()
            << QStringLiteral("Plugin dependency resolution warning: %1").arg(res.errorMessage);
        m_orderedIds = m_plugins.keys();
    }

    return discoveredCount;
}

bool PluginManager::loadPlugin(const QString& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) return false;
    auto& desc = it.value();
    if (desc.state != PluginState::Unloaded && desc.state != PluginState::Disabled)
        return desc.state != PluginState::Error;
    desc.failureCategory.clear();
    desc.errorString.clear();
    if (!desc.manifest.isValid() || !desc.manifest.isCompatibleWithCore(m_coreVersion) ||
        desc.manifest.apiVersion != QStringLiteral("3.0")) {
        desc.failureCategory = QStringLiteral("PluginIncompatible");
        desc.errorString = desc.failureCategory;
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    const QStringList supportedPermissions{Permissions::NetworkLoopback, Permissions::NetworkExternal,
        Permissions::ModelConfigRead, Permissions::ModelConfigWrite, Permissions::FileSystemRead,
        Permissions::FileSystemWrite, Permissions::ToolExecution, Permissions::DatabaseAccess,
        Permissions::CredentialUse};
    for (const auto& permission : desc.manifest.permissions.toList()) {
        if (!supportedPermissions.contains(permission)) {
            desc.failureCategory = QStringLiteral("PluginIncompatible");
            desc.errorString = desc.failureCategory;
            updateState(desc, PluginState::Error);
            emit pluginError(pluginId, desc.errorString);
            return false;
        }
    }
    QString libPath = desc.pluginFilePath;
    if (QFileInfo(libPath).isDir()) {
        QDir dir(libPath);
        libPath = dir.filePath(desc.manifest.entryPoint);
        if (!QLibrary::isLibrary(libPath)) {
#if defined(Q_OS_WIN)
            libPath += QStringLiteral(".dll");
#elif defined(Q_OS_MACOS)
            libPath = dir.filePath(QStringLiteral("lib") + desc.manifest.entryPoint + QStringLiteral(".dylib"));
#else
            libPath = dir.filePath(QStringLiteral("lib") + desc.manifest.entryPoint + QStringLiteral(".so"));
#endif
        }
    }
    if (!QFileInfo(libPath).isFile()) {
        desc.failureCategory = QStringLiteral("PluginLoadFailure");
        desc.errorString = desc.failureCategory;
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    m_sandbox->setActive(pluginId, true);
    auto host = std::make_shared<PluginHostSession>();
    host->setHostRequestHandler([this, pluginId, generation = std::weak_ptr<PluginHostSession>(host)](
                                                 const QString&, const QString& toolId,
                                                 const QJsonObject& request) -> QJsonObject {
        const auto denied = [](const QString& category) {
            return QJsonObject{{QStringLiteral("ok"), false}, {QStringLiteral("category"), category}};
        };
        if (request.value(QStringLiteral("kind")) == QStringLiteral("capability"))
            return denied(QStringLiteral("PluginHostCapabilityNotDeclared"));
        const auto found = m_plugins.constFind(pluginId);
        const auto currentHost = generation.lock();
        if (found == m_plugins.cend() || !currentHost || found->host != currentHost ||
            request.value(QStringLiteral("kind")) != QStringLiteral("credential") ||
            request.value(QStringLiteral("purpose")) != QStringLiteral("tool-execution") ||
            request.value(QStringLiteral("pluginId")).toString() != pluginId)
            return denied(QStringLiteral("PluginCredentialDenied"));
        const auto credentialId = request.value(QStringLiteral("credentialId")).toString();
        if (credentialId.isEmpty() || credentialId.size() > 128)
            return denied(QStringLiteral("PluginCredentialNotDeclared"));
        bool declared = false;
        for (const auto& credential : found->manifest.credentials)
            if (credential.id == credentialId) { declared = true; break; }
        if (!declared) return denied(QStringLiteral("PluginCredentialNotDeclared"));
        // The active tool must name this exact credential as an external-service resource.
        // Manifest declaration and a general plugin permission are insufficient alone.
        bool scoped = false;
        for (const auto& item : found->remoteTools) {
            const auto tool = item.toObject();
            if (tool.value(QStringLiteral("id")).toString() != toolId) continue;
            for (const auto& value : tool.value(QStringLiteral("requirements")).toArray()) {
                const auto requirement = value.toObject();
                if (requirement.value(QStringLiteral("domain")).toString() == QStringLiteral("external-service") &&
                    requirement.value(QStringLiteral("staticResource")).toString() ==
                        QStringLiteral("credential:%1").arg(credentialId)) scoped = true;
            }
        }
        if (!scoped || !m_sandbox->checkPermission(pluginId, Permissions::CredentialUse))
            return denied(QStringLiteral("PluginCredentialDenied"));
        auto store = defaultCredentialStore();
        if (store.summary().status != CredentialStoreStatus::Ready)
            return denied(QStringLiteral("PluginCredentialUnavailable"));
        auto secret = store.readCredential({QStringLiteral("plugin.") + pluginId, credentialId});
        if (!secret.result.succeeded || !secret.secret || secret.secret->isEmpty())
            return denied(QStringLiteral("PluginCredentialNotConfigured"));
        QJsonObject response{{QStringLiteral("ok"), true},
                             {QStringLiteral("secret"), *secret.secret}};
        secret.secret->fill(QChar(0));
        secret.secret.reset();
        return response;
    });
    connect(host.get(), &PluginHostSession::failed, this, [this, pluginId](const QString& category) {
        auto found = m_plugins.find(pluginId);
        if (found == m_plugins.end() || !found->host) return;
        m_sandbox->setActive(pluginId, false);
        if (m_toolRegistry) m_toolRegistry->unregisterProvider(ToolSource::Plugin,
            QStringLiteral("plugin:%1").arg(pluginId));
        found->failureCategory = category;
        found->errorString = category;
        updateState(found.value(), PluginState::Error);
        emit pluginError(pluginId, category);
    });
    desc.host = host;
    if (!host->start(libPath, m_pluginStorageDir + QLatin1Char('/') + pluginId)) {
        desc.failureCategory = host->failureCategory();
        desc.errorString = desc.failureCategory;
        desc.host.reset();
        m_sandbox->setActive(pluginId, false);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    const auto result = host->call(QStringLiteral("load"),
        {{QStringLiteral("path"), QFileInfo(libPath).absoluteFilePath()},
         {QStringLiteral("pluginId"), pluginId},
         {QStringLiteral("abi"), NativePluginAbiVersion},
         {QStringLiteral("permissions"), m_sandbox->getPermissions(pluginId).toJsonArray()}});
    if (!result.value(QStringLiteral("ok")).toBool() ||
        !result.value(QStringLiteral("tools")).isArray()) {
        desc.failureCategory = result.value(QStringLiteral("category")).toString(QStringLiteral("PluginLoadFailure"));
        desc.errorString = desc.failureCategory;
        host->shutdown();
        desc.host.reset();
        m_sandbox->setActive(pluginId, false);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    desc.remoteTools = result.value(QStringLiteral("tools")).toArray();
    updateState(desc, PluginState::Loaded);
    emit pluginLoaded(pluginId);
    return true;
}

bool PluginManager::initializePlugin(const QString& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) return false;
    if (it->state == PluginState::Unloaded && !loadPlugin(pluginId)) return false;
    if (it->state == PluginState::Initialized || it->state == PluginState::Active) return true;
    if (it->state != PluginState::Loaded || !it->host ||
        (!it->remoteTools.isEmpty() && !m_toolRegistry)) return false;
    updateState(it.value(), PluginState::Initialized);
    return true;
}

bool PluginManager::registerRemoteTools(const QString& pluginId) {
    auto& desc = m_plugins[pluginId];
    if (!desc.host) return false;
    if (desc.remoteTools.isEmpty()) return true;
    if (!m_toolRegistry ||
        !m_sandbox->checkPermission(pluginId, Permissions::ToolExecution)) {
        desc.failureCategory = QStringLiteral("PluginPermissionDenied");
        desc.errorString = desc.failureCategory;
        unloadPlugin(pluginId);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    QList<IToolRegistry::Registration> registrations;
    QSet<QString> names;
    bool valid = true;
    for (const auto& value : desc.remoteTools) {
        if (!value.isObject()) { valid = false; break; }
        const auto item = value.toObject();
        const auto localId = item.value(QStringLiteral("id")).toString().trimmed();
        const auto schema = item.value(QStringLiteral("schema")).toObject();
        if (localId.isEmpty() || names.contains(localId) ||
            schema.value(QStringLiteral("type")) != QStringLiteral("object") ||
            schema.value(QStringLiteral("additionalProperties")) != false) { valid = false; break; }
        const auto properties = schema.value(QStringLiteral("properties")).toObject();
        for (const auto& required : schema.value(QStringLiteral("required")).toArray())
            if (!required.isString() || !properties.contains(required.toString())) valid = false;
        for (auto property = properties.begin(); property != properties.end(); ++property)
            if (!property.value().isObject() ||
                !property.value().toObject().contains(QStringLiteral("type"))) valid = false;
        if (!valid) break;
        names.insert(localId);
        ToolDescriptor tool;
        tool.id = QStringLiteral("plugin.%1.%2").arg(escapedId(pluginId), escapedId(localId));
        tool.name = item.value(QStringLiteral("name")).toString(localId);
        tool.description = item.value(QStringLiteral("description")).toString();
        tool.category = item.value(QStringLiteral("category")).toString();
        tool.inputSchema = schema;
        tool.source = ToolSource::Plugin;
        tool.providerId = QStringLiteral("plugin:%1").arg(pluginId);
        tool.version = desc.manifest.version;
        tool.executionMode = ToolExecutionMode::Local;
        tool.requiredPermissionDomain = QStringLiteral("tool-execution");
        const auto riskName = item.value(QStringLiteral("risk")).toString();
        const auto risk = riskName == QStringLiteral("low") ? int(ToolRiskLevel::Low) :
            riskName == QStringLiteral("medium") ? int(ToolRiskLevel::Medium) :
            riskName == QStringLiteral("high") ? int(ToolRiskLevel::High) : -1;
        if (risk < 0 || risk > int(ToolRiskLevel::High)) { valid = false; break; }
        tool.riskLevel = ToolRiskLevel(risk);
        for (const auto& requirement : item.value(QStringLiteral("requirements")).toArray()) {
            if (!requirement.isObject()) { valid = false; break; }
            const auto object = requirement.toObject();
            const auto domain = sdkDomain(object.value(QStringLiteral("domain")).toString());
            const auto access = sdkAccess(object.value(QStringLiteral("access")).toString());
            const auto kind = sdkResourceKind(object.value(QStringLiteral("resourceKind")).toString());
            if (domain < 0 || domain > int(SecurityDomain::ExternalService) ||
                access < 0 || access > int(AccessMode::Invoke) ||
                kind < 0 || kind > int(AuthorizationResourceKind::Provider)) { valid = false; break; }
            tool.authorizationRequirements.append({SecurityDomain(domain), AccessMode(access),
                AuthorizationResourceKind(kind), object.value(QStringLiteral("resourceArgument")).toString(),
                object.value(QStringLiteral("staticResource")).toString()});
        }
        if (!valid) break;
        if (tool.authorizationRequirements.isEmpty())
            tool.authorizationRequirements = {{SecurityDomain::Application, AccessMode::Invoke,
                AuthorizationResourceKind::Provider, {}, tool.providerId}};
        const auto permissions = hostPermissionsFor(tool);
        for (const auto& permission : permissions)
            if (!m_sandbox->checkPermission(pluginId, permission)) {
                desc.failureCategory = QStringLiteral("PluginPermissionDenied");
                valid = false;
            }
        if (!valid) break;
        registrations.append({std::move(tool), std::make_shared<RemotePluginToolHandler>(
            desc.host, m_sandbox, pluginId, localId, permissions)});
    }
    if (!valid || !m_toolRegistry->replaceProvider(ToolSource::Plugin,
            QStringLiteral("plugin:%1").arg(pluginId), std::move(registrations))) {
        if (desc.failureCategory.isEmpty()) desc.failureCategory = QStringLiteral("PluginLoadFailure");
        desc.errorString = desc.failureCategory;
        unloadPlugin(pluginId);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    return true;
}

bool PluginManager::startPlugin(const QString& pluginId) {
    if (!initializePlugin(pluginId)) return false;
    auto& desc = m_plugins[pluginId];
    if (desc.state == PluginState::Active) return true;
    const auto result = desc.host->call(QStringLiteral("start"));
    if (!result.value(QStringLiteral("ok")).toBool()) {
        desc.failureCategory = result.value(QStringLiteral("category")).toString(QStringLiteral("PluginLoadFailure"));
        desc.errorString = desc.failureCategory;
        unloadPlugin(pluginId);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }
    if (!registerRemoteTools(pluginId)) return false;
    updateState(desc, PluginState::Active);
    return true;
}

bool PluginManager::stopPlugin(const QString& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) return false;
    if (it->state != PluginState::Active) return true;
    if (!it->host || !it->host->call(QStringLiteral("stop")).value(QStringLiteral("ok")).toBool()) {
        it->failureCategory = QStringLiteral("PluginHostUnavailable");
        if (m_toolRegistry) m_toolRegistry->unregisterProvider(ToolSource::Plugin,
            QStringLiteral("plugin:%1").arg(pluginId));
        updateState(it.value(), PluginState::Error);
        return false;
    }
    if (m_toolRegistry) m_toolRegistry->unregisterProvider(ToolSource::Plugin,
        QStringLiteral("plugin:%1").arg(pluginId));
    updateState(it.value(), PluginState::Initialized);
    return true;
}

bool PluginManager::unloadPlugin(const QString& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) return false;
    m_sandbox->setActive(pluginId, false);
    if (m_toolRegistry) m_toolRegistry->unregisterProvider(ToolSource::Plugin,
        QStringLiteral("plugin:%1").arg(pluginId));
    if (it->host) {
        it->host->shutdown();
        it->host.reset();
    }
    it->remoteTools = {};
    updateState(it.value(), PluginState::Unloaded);
    emit pluginUnloaded(pluginId);
    return true;
}

bool PluginManager::setEnabled(const QString& pluginId, bool enabled) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end())
        return false;
    if (!enabled) {
        if (it->state != PluginState::Disabled && !unloadPlugin(pluginId))
            return false;
        updateState(it.value(), PluginState::Disabled);
        return true;
    }
    if (it->state == PluginState::Disabled)
        updateState(it.value(), PluginState::Unloaded);
    return startPlugin(pluginId);
}

bool PluginManager::reloadPlugin(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) {
        qWarning()
            << QStringLiteral("PluginManager::reloadPlugin: Plugin '%1' not found").arg(pluginId);
        return false;
    }

    auto& desc = m_plugins[pluginId];
    if (desc.state == PluginState::Disabled)
        return false;
    PluginState previousState = desc.state;
    QSet<QString> declaredIds;
    for (const auto& credential : desc.manifest.credentials)
        declaredIds.insert(credential.id + QLatin1Char(':') + credential.kind +
                           (credential.required ? QLatin1String(":required") :
                                                  QLatin1String(":optional")));
    m_reloadCredentialIds.insert(pluginId, declaredIds);

    qDebug() << QStringLiteral(
                    "PluginManager::reloadPlugin: Reloading plugin '%1' (previous state: %2)")
                    .arg(pluginId)
                    .arg(static_cast<int>(previousState));

    // Unload the plugin completely
    if (!unloadPlugin(pluginId)) {
        m_reloadCredentialIds.remove(pluginId);
        emit pluginReloadFailed(pluginId, QStringLiteral("Failed to unload plugin"));
        return false;
    }
    QTimer::singleShot(
        20, this, [this, pluginId, previousState] { finishReload(pluginId, previousState, 0); });
    return true;
}

void PluginManager::finishReload(const QString& pluginId, PluginState previousState, int attempts) {
    if (isModuleResident(pluginId)) {
        if (attempts >= 250) {
            emit pluginReloadFailed(pluginId, QStringLiteral("Active plugin calls did not finish"));
            return;
        }
        QTimer::singleShot(20, this, [this, pluginId, previousState, attempts] {
            finishReload(pluginId, previousState, attempts + 1);
        });
        return;
    }
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) {
        emit pluginReloadFailed(pluginId, QStringLiteral("Plugin disappeared during reload"));
        return;
    }
    auto& desc = it.value();

    // Re-discover the plugin (in case manifest changed)
    QString pluginDir = QFileInfo(desc.pluginFilePath).absoluteDir().absolutePath();
    discoverPlugins(pluginDir);

    const auto previousCredentials = m_reloadCredentialIds.take(pluginId);
    const auto refreshed = m_plugins.constFind(pluginId);
    if (refreshed == m_plugins.cend()) {
        emit pluginReloadFailed(pluginId, QStringLiteral("Plugin disappeared during discovery"));
        return;
    }
    for (const auto& credential : refreshed->manifest.credentials) {
        if (!previousCredentials.contains(credential.id + QLatin1Char(':') + credential.kind +
            (credential.required ? QLatin1String(":required") : QLatin1String(":optional")))) {
            emit pluginReloadFailed(pluginId,
                QStringLiteral("New credential declarations require a fresh plugin discovery"));
            return;
        }
    }

    // Reload and restore previous state
    if (!loadPlugin(pluginId)) {
        emit pluginReloadFailed(pluginId, QStringLiteral("Failed to load plugin after unload"));
        return;
    }

    // Restore to the previous state
    if (previousState == PluginState::Initialized || previousState == PluginState::Active ||
        previousState == PluginState::Error) {
        if (!initializePlugin(pluginId)) {
            emit pluginReloadFailed(pluginId,
                                    QStringLiteral("Failed to initialize plugin after reload"));
            return;
        }
    }

    if (previousState == PluginState::Active || previousState == PluginState::Error) {
        if (!startPlugin(pluginId)) {
            emit pluginReloadFailed(pluginId,
                                    QStringLiteral("Failed to start plugin after reload"));
            return;
        }
    }

    qDebug() << QStringLiteral("PluginManager::reloadPlugin: Successfully reloaded plugin '%1'")
                    .arg(pluginId);
    emit pluginReloaded(pluginId);
}

void PluginManager::enableHotReload(bool enabled) {
    if (enabled == m_hotReloadEnabled) {
        return;
    }

    m_hotReloadEnabled = enabled;

    if (enabled) {
        if (!m_hotReloader) {
            m_hotReloader = std::make_unique<PluginHotReloader>(this, this);
            connect(m_hotReloader.get(), &PluginHotReloader::reloadRequested, this,
                    &PluginManager::onHotReloadRequested);
        }

        HotReloadConfig config;
        config.enabled = true;
        config.watchedDirs << m_pluginStorageDir;
        m_hotReloader->setConfig(config);
        m_hotReloader->startWatching();

        qDebug() << "PluginManager: Hot-reload enabled";
    } else {
        if (m_hotReloader) {
            m_hotReloader->stopWatching();
        }
        qDebug() << "PluginManager: Hot-reload disabled";
    }
}

bool PluginManager::isHotReloadEnabled() const {
    return m_hotReloadEnabled;
}

void PluginManager::setHotReloadConfig(const HotReloadConfig& config) {
    if (m_hotReloader) {
        m_hotReloader->setConfig(config);
    }
}

HotReloadConfig PluginManager::hotReloadConfig() const {
    if (m_hotReloader) {
        return m_hotReloader->config();
    }
    return {};
}

bool PluginManager::initializeAll() {
    bool allSuccess = true;
    for (const QString& id : m_orderedIds) {
        if (!initializePlugin(id)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool PluginManager::startAll() {
    bool allSuccess = true;
    for (const QString& id : m_orderedIds) {
        if (!startPlugin(id)) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

bool PluginManager::stopAll() {
    bool allSuccess = true;
    for (int i = m_orderedIds.size() - 1; i >= 0; --i) {
        if (!stopPlugin(m_orderedIds[i])) {
            allSuccess = false;
        }
    }
    return allSuccess;
}

void PluginManager::unloadAll() {
    for (int i = m_orderedIds.size() - 1; i >= 0; --i) {
        unloadPlugin(m_orderedIds[i]);
    }
}

QList<QString> PluginManager::registeredPluginIds() const {
    return m_orderedIds;
}

bool PluginManager::isLoaded(const QString& pluginId) const {
    if (!m_plugins.contains(pluginId)) {
        return false;
    }
    PluginState s = m_plugins.value(pluginId).state;
    return s == PluginState::Loaded || s == PluginState::Initialized || s == PluginState::Active;
}

PluginState PluginManager::pluginState(const QString& pluginId) const {
    if (!m_plugins.contains(pluginId)) {
        return PluginState::Unloaded;
    }
    return m_plugins.value(pluginId).state;
}

const PluginDescriptor* PluginManager::descriptor(const QString& pluginId) const {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) {
        return nullptr;
    }
    return &it.value();
}

ISentinelPlugin* PluginManager::pluginInstance(const QString&) const {
    return nullptr;
}

bool PluginManager::isModuleResident(const QString&) const {
    return false;
}

void PluginManager::onHotReloadRequested(const QString& pluginId) {
    qDebug() << QStringLiteral("PluginManager: Hot-reload requested for plugin '%1'").arg(pluginId);
    reloadPlugin(pluginId);
}

void PluginManager::updateState(PluginDescriptor& desc, PluginState newState) {
    if (desc.state != newState) {
        desc.state = newState;
        emit pluginStateChanged(desc.manifest.id, newState);
    }
}

} // namespace sentinel::core::plugin
