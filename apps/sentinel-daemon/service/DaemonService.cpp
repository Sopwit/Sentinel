#include "DaemonService.h"

#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/platform/DpapiEncryptedSettingsStore.h"
#include "sentinel/core/memory/JsonSettingsStore.h"

#include <QDebug>

namespace sentinel::daemon {

DaemonService::DaemonService(QObject* parent) : QObject(parent) {
    connect(&m_healthTimer, &QTimer::timeout, this, &DaemonService::performHealthCheck);
}

DaemonService::~DaemonService() = default;

bool DaemonService::initialize() {
    qInfo().noquote() << "Initializing Sentinel Headless Daemon Service...";

    m_settings = std::make_unique<sentinel::core::AppSettings>(
        std::make_unique<sentinel::core::DpapiEncryptedSettingsStore>(
            std::make_unique<sentinel::core::JsonSettingsStore>(m_pathProvider.settingsFilePath())));

    sentinel::core::ApplicationControllerBuilder builder;
    m_controller = builder.withStandardDefaults(m_pathProvider, *m_settings).build();

    if (!m_ipcServer.startServer()) {
        qWarning().noquote() << "Failed to start Daemon IPC server.";
        return false;
    }

    // Health check every 60 seconds
    m_healthTimer.start(60000);
    qInfo().noquote() << "Sentinel Daemon Service initialized successfully.";
    return true;
}

void DaemonService::performHealthCheck() {
    if (m_controller) {
        m_controller->refreshOllamaStatus();
    }
}

} // namespace sentinel::daemon
