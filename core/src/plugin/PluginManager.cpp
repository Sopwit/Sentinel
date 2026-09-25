// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginManager.h"
#include "sentinel/core/plugin/PluginDependencyResolver.h"
#include "sentinel/core/runtime/IToolRegistry.h"
#include <QCoreApplication>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLibrary>
#include <QStandardPaths>
#include <QTimer>
#include <algorithm>
#include <atomic>

namespace sentinel::core::plugin {

struct PluginModuleState {
    std::shared_ptr<QPluginLoader> loader;
    ISentinelPlugin* instance = nullptr;
    std::atomic_bool unloading{false};
    ~PluginModuleState() {
        // Cleanup runs in a later event turn. A plugin can release its final
        // completion callback while its own dynamic-library frame is active.
        auto retiredLoader = std::move(loader);
        auto* retiredInstance = instance;
        auto cleanup = [retiredLoader = std::move(retiredLoader), retiredInstance] {
            if (retiredInstance) {
                try {
                    if (retiredInstance->state() == PluginState::Active)
                        retiredInstance->stop();
                    retiredInstance->shutdown();
                } catch (...) {
                    qWarning() << "Plugin shutdown threw an exception";
                }
            }
            if (retiredLoader)
                retiredLoader->unload();
        };
        if (auto* app = QCoreApplication::instance())
            QTimer::singleShot(0, app, std::move(cleanup));
        else
            cleanup();
    }
};

namespace {
QStringList hostPermissionsFor(const ToolDescriptor& descriptor) {
    QStringList permissions{Permissions::ToolExecution};
    for (const auto& requirement : descriptor.authorizationRequirements) {
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

class PluginToolHandler final : public IToolHandler,
                                public std::enable_shared_from_this<PluginToolHandler> {
public:
    PluginToolHandler(std::shared_ptr<PluginModuleState> module,
                      std::shared_ptr<IToolHandler> inner, std::shared_ptr<PluginSandbox> sandbox,
                      QString pluginId, QStringList requiredPermissions)
        : module_(std::move(module)), inner_(std::move(inner)), sandbox_(std::move(sandbox)),
          pluginId_(std::move(pluginId)), requiredPermissions_(std::move(requiredPermissions)) {}

    IToolExecutor::Cancel execute(const ToolExecutionRequest& request, const QString& sessionId,
                                  const QString& toolCallId, IToolExecutor::Output output,
                                  IToolExecutor::Completion completion) override {
        const bool permissionsGranted = std::all_of(
            requiredPermissions_.cbegin(), requiredPermissions_.cend(), [this](const QString& item) {
                return sandbox_->checkPermission(pluginId_, item);
            });
        if (module_->unloading || !permissionsGranted) {
            completion({ToolExecutionStatus::Blocked,
                        QStringLiteral("Plugin tool unavailable or permission denied.")});
            return {};
        }
        auto self = shared_from_this();
        auto finished = std::make_shared<std::atomic_bool>(false);
        try {
            auto cancel = inner_->execute(
                request, sessionId, toolCallId, std::move(output),
                [self, finished, completion = std::move(completion)](ToolExecutionResult result) {
                    if (!finished->exchange(true))
                        completion(std::move(result));
                });
            return [self, cancel = std::move(cancel)] {
                if (cancel)
                    cancel();
            };
        } catch (const std::exception& error) {
            if (!finished->exchange(true))
                completion({ToolExecutionStatus::Blocked,
                            QStringLiteral("Plugin tool failed: %1").arg(error.what())});
        } catch (...) {
            if (!finished->exchange(true))
                completion({ToolExecutionStatus::Blocked,
                            QStringLiteral("Plugin tool failed with an unknown exception.")});
        }
        return {};
    }

private:
    // The handler is destroyed before the module; its destructor remains in loaded code.
    std::shared_ptr<PluginModuleState> module_;
    std::shared_ptr<IToolHandler> inner_;
    std::shared_ptr<PluginSandbox> sandbox_;
    QString pluginId_;
    QStringList requiredPermissions_;
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

void PluginManager::setMemoryStore(IMemoryStore* store) {
    m_memoryStore = store;
}

void PluginManager::setProviderCatalog(IProviderCatalog* catalog) {
    m_providerCatalog = catalog;
}

int PluginManager::discoverPlugins(const QString& searchDir) {
    QString targetDir = searchDir.isEmpty() ? m_pluginStorageDir : searchDir;
    QDir dir(targetDir);
    if (!dir.exists()) {
        return 0;
    }

    int discoveredCount = 0;

    // 1. Search for directory-based plugins containing plugin.json
    QDirIterator it(targetDir, QStringList() << QStringLiteral("plugin.json"), QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString manifestPath = it.filePath();
        QString error;
        PluginManifest manifest = PluginManifest::parseFile(manifestPath, &error);

        if (manifest.isValid()) {
            if (!manifest.isCompatibleWithCore(m_coreVersion)) {
                qWarning() << QStringLiteral("Plugin '%1' is incompatible with core version '%2'")
                                  .arg(manifest.id, m_coreVersion);
                continue;
            }

            PluginDescriptor desc;
            desc.manifest = manifest;
            desc.pluginFilePath = QFileInfo(manifestPath).absolutePath(); // directory
            desc.state = PluginState::Unloaded;

            m_sandbox->registerPluginPermissions(manifest.id, manifest.permissions);
            m_plugins[manifest.id] = desc;
            discoveredCount++;
        }
    }

    // 2. Search for standalone dynamic library files (.so, .dylib, .dll)
    QDirIterator libIt(targetDir, QDir::Files, QDirIterator::Subdirectories);
    while (libIt.hasNext()) {
        libIt.next();
        QString filePath = libIt.filePath();
        if (!QLibrary::isLibrary(filePath)) {
            continue;
        }

        QPluginLoader loader(filePath);
        QJsonObject meta = loader.metaData().value(QStringLiteral("MetaData")).toObject();
        if (meta.isEmpty()) {
            // Also check root loader metadata
            meta = loader.metaData();
        }

        if (meta.contains(QStringLiteral("id"))) {
            QString error;
            PluginManifest manifest = PluginManifest::parseJson(meta, &error);
            if (manifest.isValid()) {
                if (!m_plugins.contains(manifest.id)) {
                    PluginDescriptor desc;
                    desc.manifest = manifest;
                    desc.pluginFilePath = filePath;
                    desc.state = PluginState::Unloaded;

                    m_sandbox->registerPluginPermissions(manifest.id, manifest.permissions);
                    m_plugins[manifest.id] = desc;
                    discoveredCount++;
                } else if (m_plugins[manifest.id].pluginFilePath.isEmpty() ||
                           !m_plugins[manifest.id].pluginFilePath.endsWith(
                               QLibrary::isLibrary(filePath) ? filePath : QString())) {
                    // Update entry point path to library file if found
                    m_plugins[manifest.id].pluginFilePath = filePath;
                }
            }
        }
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
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    auto& desc = m_plugins[pluginId];
    if (desc.state != PluginState::Unloaded && desc.state != PluginState::Disabled) {
        return true; // Already loaded or active
    }

    // Determine target binary path
    QString libPath = desc.pluginFilePath;
    if (QFileInfo(libPath).isDir()) {
        QDir pluginDir(libPath);
        QString entryName = desc.manifest.entryPoint;
        QString fileCandidate = pluginDir.filePath(entryName);

        if (!QLibrary::isLibrary(fileCandidate)) {
            // Try matching system library extensions
#if defined(Q_OS_WIN)
            fileCandidate = pluginDir.filePath(entryName + QStringLiteral(".dll"));
#elif defined(Q_OS_MACOS)
            fileCandidate =
                pluginDir.filePath(QStringLiteral("lib") + entryName + QStringLiteral(".dylib"));
            if (!QFile::exists(fileCandidate)) {
                fileCandidate = pluginDir.filePath(entryName + QStringLiteral(".dylib"));
            }
#else
            fileCandidate =
                pluginDir.filePath(QStringLiteral("lib") + entryName + QStringLiteral(".so"));
            if (!QFile::exists(fileCandidate)) {
                fileCandidate = pluginDir.filePath(entryName + QStringLiteral(".so"));
            }
#endif
        }
        libPath = fileCandidate;
    }

    if (!QFile::exists(libPath)) {
        desc.errorString = QStringLiteral("Plugin binary file not found: %1").arg(libPath);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }

    auto loader = std::make_shared<QPluginLoader>(libPath);
    if (!loader->load()) {
        desc.errorString = loader->errorString();
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }

    QObject* pluginObj = loader->instance();
    if (!pluginObj) {
        desc.errorString =
            QStringLiteral("Failed to instantiate plugin object from %1").arg(libPath);
        loader->unload();
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }

    auto* sentinelPlugin = qobject_cast<ISentinelPlugin*>(pluginObj);
    if (!sentinelPlugin) {
        desc.errorString =
            QStringLiteral("Plugin object does not implement ISentinelPlugin interface");
        loader->unload();
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }

    desc.loader = loader;
    desc.instance = sentinelPlugin;
    updateState(desc, PluginState::Loaded);
    emit pluginLoaded(pluginId);
    return true;
}

bool PluginManager::initializePlugin(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    auto& desc = m_plugins[pluginId];
    if (desc.state == PluginState::Unloaded) {
        if (!loadPlugin(pluginId)) {
            return false;
        }
    }

    if (desc.state != PluginState::Loaded) {
        return desc.state == PluginState::Initialized || desc.state == PluginState::Active;
    }

    QString dataDir = m_pluginStorageDir + QStringLiteral("/") + pluginId;
    QDir().mkpath(dataDir);

    auto context = std::make_shared<PluginContext>(pluginId, m_coreVersion, dataDir,
                                                   desc.manifest.permissions);

    auto module = std::make_shared<PluginModuleState>();
    module->loader = desc.loader;
    module->instance = desc.instance;
    desc.module = module;
    m_modules[pluginId] = module;
    const auto version = desc.manifest.version;
    std::weak_ptr<PluginModuleState> weakModule = module;
    context->setToolRegistrar(
        [weakModule, registry = m_toolRegistry, sandbox = m_sandbox, pluginId,
         version](ToolDescriptor descriptor, std::shared_ptr<IToolHandler> handler) {
            const auto module = weakModule.lock();
            if (!module || module->unloading || !registry || !handler ||
                !sandbox->checkPermission(pluginId, Permissions::ToolExecution))
                return false;
            const auto localName = descriptor.id.trimmed();
            if (localName.isEmpty())
                return false;
            descriptor.id =
                QStringLiteral("plugin.%1.%2").arg(escapedId(pluginId), escapedId(localName));
            if (descriptor.name.isEmpty())
                descriptor.name = localName;
            descriptor.source = ToolSource::Plugin;
            descriptor.providerId = QStringLiteral("plugin:%1").arg(pluginId);
            descriptor.version = version;
            descriptor.executionMode = ToolExecutionMode::Local;
            descriptor.requiredPermissionDomain = QStringLiteral("tool-execution");
            if (descriptor.authorizationRequirements.isEmpty())
                descriptor.authorizationRequirements = {
                    {SecurityDomain::ExternalService, AccessMode::Invoke,
                     AuthorizationResourceKind::Provider, {}, descriptor.providerId}};
            if (descriptor.evidenceProduced.isEmpty())
                descriptor.evidenceProduced = {{ObservationDomain::ExternalService,
                                                EvidenceFreshness::TurnScoped,
                                                EvidenceScope::Provider, {}}};
            const auto requiredPermissions = hostPermissionsFor(descriptor);
            for (const auto& permission : requiredPermissions) {
                if (!sandbox->checkPermission(pluginId, permission))
                    return false;
            }
            auto wrapped =
                std::make_shared<PluginToolHandler>(module, std::move(handler), sandbox, pluginId,
                                                    requiredPermissions);
            return registry->registerTool({std::move(descriptor), std::move(wrapped)});
        });

    // Inject core services into plugin context
    injectCoreServices(context.get());

    desc.context = context;
    if (!desc.instance->initialize(context)) {
        module->unloading = true;
        if (m_toolRegistry)
            m_toolRegistry->unregisterProvider(ToolSource::Plugin,
                                               QStringLiteral("plugin:%1").arg(pluginId));
        desc.instance = nullptr;
        desc.context.reset();
        desc.loader.reset();
        desc.module.reset();
        desc.errorString = QStringLiteral("Plugin initialize() returned false");
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }

    updateState(desc, PluginState::Initialized);
    return true;
}

bool PluginManager::startPlugin(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    auto& desc = m_plugins[pluginId];
    if (desc.state == PluginState::Loaded || desc.state == PluginState::Unloaded) {
        if (!initializePlugin(pluginId)) {
            return false;
        }
    }

    if (desc.state != PluginState::Initialized) {
        return desc.state == PluginState::Active;
    }

    if (!desc.instance->start()) {
        desc.errorString = QStringLiteral("Plugin start() returned false");
        unloadPlugin(pluginId);
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }

    updateState(desc, PluginState::Active);
    return true;
}

bool PluginManager::stopPlugin(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    auto& desc = m_plugins[pluginId];
    if (desc.state != PluginState::Active) {
        return true;
    }

    if (desc.instance) {
        desc.instance->stop();
    }

    updateState(desc, PluginState::Initialized);
    return true;
}

bool PluginManager::unloadPlugin(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) {
        return false;
    }

    auto& desc = m_plugins[pluginId];
    if (desc.state == PluginState::Unloaded) {
        return true;
    }

    if (desc.module)
        desc.module->unloading = true;
    if (m_toolRegistry)
        m_toolRegistry->unregisterProvider(ToolSource::Plugin,
                                           QStringLiteral("plugin:%1").arg(pluginId));

    if (desc.module) {
        // Active gateway snapshots keep their wrapped handler and module alive. The
        // module destructor performs stop/shutdown/unload after the last snapshot ends.
        desc.instance = nullptr;
        desc.context.reset();
        desc.loader.reset();
        desc.module.reset();
        updateState(desc, PluginState::Unloaded);
        emit pluginUnloaded(pluginId);
        return true;
    }

    if (desc.state == PluginState::Active) {
        stopPlugin(pluginId);
    }

    if (desc.instance) {
        desc.instance->shutdown();
        desc.instance = nullptr;
    }

    desc.context.reset();
    if (desc.loader) {
        desc.loader->unload();
        desc.loader.reset();
    }

    updateState(desc, PluginState::Unloaded);
    emit pluginUnloaded(pluginId);
    return true;
}

bool PluginManager::reloadPlugin(const QString& pluginId) {
    if (!m_plugins.contains(pluginId)) {
        qWarning()
            << QStringLiteral("PluginManager::reloadPlugin: Plugin '%1' not found").arg(pluginId);
        return false;
    }

    auto& desc = m_plugins[pluginId];
    PluginState previousState = desc.state;

    qDebug() << QStringLiteral(
                    "PluginManager::reloadPlugin: Reloading plugin '%1' (previous state: %2)")
                    .arg(pluginId)
                    .arg(static_cast<int>(previousState));

    // Unload the plugin completely
    if (!unloadPlugin(pluginId)) {
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
    auto& desc = m_plugins[pluginId];

    // Re-discover the plugin (in case manifest changed)
    QString pluginDir = QFileInfo(desc.pluginFilePath).absoluteDir().absolutePath();
    discoverPlugins(pluginDir);

    // Reload and restore previous state
    if (!loadPlugin(pluginId)) {
        emit pluginReloadFailed(pluginId, QStringLiteral("Failed to load plugin after unload"));
        return;
    }

    // Restore to the previous state
    if (previousState >= PluginState::Initialized) {
        if (!initializePlugin(pluginId)) {
            emit pluginReloadFailed(pluginId,
                                    QStringLiteral("Failed to initialize plugin after reload"));
            return;
        }
    }

    if (previousState >= PluginState::Active) {
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

ISentinelPlugin* PluginManager::pluginInstance(const QString& pluginId) const {
    if (!m_plugins.contains(pluginId)) {
        return nullptr;
    }
    return m_plugins.value(pluginId).instance;
}

bool PluginManager::isModuleResident(const QString& pluginId) const {
    const auto it = m_modules.constFind(pluginId);
    return it != m_modules.cend() && !it.value().expired();
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

void PluginManager::injectCoreServices(PluginContext* context) {
    if (!context) {
        return;
    }

    context->setToolRegistry(m_toolRegistry);
    context->setMemoryStore(m_memoryStore);
    context->setProviderCatalog(m_providerCatalog);
}

} // namespace sentinel::core::plugin
