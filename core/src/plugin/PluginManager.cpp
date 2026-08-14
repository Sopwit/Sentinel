// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginManager.h"
#include "sentinel/core/plugin/PluginDependencyResolver.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QLibrary>
#include <QDebug>

namespace sentinel::core::plugin {

PluginManager::PluginManager(QString coreVersion, QString pluginStorageDir, QObject* parent)
    : QObject(parent)
    , m_coreVersion(std::move(coreVersion))
    , m_pluginStorageDir(std::move(pluginStorageDir))
{
    if (m_pluginStorageDir.isEmpty()) {
        m_pluginStorageDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/plugins");
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
    return m_sandbox;
}

const PluginSandbox& PluginManager::sandbox() const {
    return m_sandbox;
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
    QDirIterator it(targetDir, QStringList() << QStringLiteral("plugin.json"), QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString manifestPath = it.filePath();
        QString error;
        PluginManifest manifest = PluginManifest::parseFile(manifestPath, &error);

        if (manifest.isValid()) {
            if (!manifest.isCompatibleWithCore(m_coreVersion)) {
                qWarning() << QStringLiteral("Plugin '%1' is incompatible with core version '%2'").arg(manifest.id, m_coreVersion);
                continue;
            }

            PluginDescriptor desc;
            desc.manifest = manifest;
            desc.pluginFilePath = QFileInfo(manifestPath).absolutePath(); // directory
            desc.state = PluginState::Unloaded;

            m_sandbox.registerPluginPermissions(manifest.id, manifest.permissions);
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

                    m_sandbox.registerPluginPermissions(manifest.id, manifest.permissions);
                    m_plugins[manifest.id] = desc;
                    discoveredCount++;
                } else if (m_plugins[manifest.id].pluginFilePath.isEmpty() ||
                           !m_plugins[manifest.id].pluginFilePath.endsWith(QLibrary::isLibrary(filePath) ? filePath : QString())) {
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
        qWarning() << QStringLiteral("Plugin dependency resolution warning: %1").arg(res.errorMessage);
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
            fileCandidate = pluginDir.filePath(QStringLiteral("lib") + entryName + QStringLiteral(".dylib"));
            if (!QFile::exists(fileCandidate)) {
                fileCandidate = pluginDir.filePath(entryName + QStringLiteral(".dylib"));
            }
#else
            fileCandidate = pluginDir.filePath(QStringLiteral("lib") + entryName + QStringLiteral(".so"));
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
        desc.errorString = QStringLiteral("Failed to instantiate plugin object from %1").arg(libPath);
        loader->unload();
        updateState(desc, PluginState::Error);
        emit pluginError(pluginId, desc.errorString);
        return false;
    }

    auto* sentinelPlugin = qobject_cast<ISentinelPlugin*>(pluginObj);
    if (!sentinelPlugin) {
        desc.errorString = QStringLiteral("Plugin object does not implement ISentinelPlugin interface");
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

    auto context = std::make_shared<PluginContext>(
        pluginId,
        m_coreVersion,
        dataDir,
        desc.manifest.permissions
    );

    // Inject core services into plugin context
    injectCoreServices(context.get());

    desc.context = context;
    if (!desc.instance->initialize(context)) {
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
        qWarning() << QStringLiteral("PluginManager::reloadPlugin: Plugin '%1' not found").arg(pluginId);
        return false;
    }

    auto& desc = m_plugins[pluginId];
    PluginState previousState = desc.state;

    qDebug() << QStringLiteral("PluginManager::reloadPlugin: Reloading plugin '%1' (previous state: %2)")
                    .arg(pluginId)
                    .arg(static_cast<int>(previousState));

    // Unload the plugin completely
    if (!unloadPlugin(pluginId)) {
        emit pluginReloadFailed(pluginId, QStringLiteral("Failed to unload plugin"));
        return false;
    }

    // Re-discover the plugin (in case manifest changed)
    QString pluginDir = QFileInfo(desc.pluginFilePath).absoluteDir().absolutePath();
    discoverPlugins(pluginDir);

    // Reload and restore previous state
    if (!loadPlugin(pluginId)) {
        emit pluginReloadFailed(pluginId, QStringLiteral("Failed to load plugin after unload"));
        return false;
    }

    // Restore to the previous state
    if (previousState >= PluginState::Initialized) {
        if (!initializePlugin(pluginId)) {
            emit pluginReloadFailed(pluginId, QStringLiteral("Failed to initialize plugin after reload"));
            return false;
        }
    }

    if (previousState >= PluginState::Active) {
        if (!startPlugin(pluginId)) {
            emit pluginReloadFailed(pluginId, QStringLiteral("Failed to start plugin after reload"));
            return false;
        }
    }

    qDebug() << QStringLiteral("PluginManager::reloadPlugin: Successfully reloaded plugin '%1'").arg(pluginId);
    emit pluginReloaded(pluginId);
    return true;
}

void PluginManager::enableHotReload(bool enabled) {
    if (enabled == m_hotReloadEnabled) {
        return;
    }

    m_hotReloadEnabled = enabled;

    if (enabled) {
        if (!m_hotReloader) {
            m_hotReloader = std::make_unique<PluginHotReloader>(this, this);
            connect(m_hotReloader.get(), &PluginHotReloader::reloadRequested,
                    this, &PluginManager::onHotReloadRequested);
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
