// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DaemonService.h"

#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/platform/DpapiEncryptedSettingsStore.h"

#include <QDebug>
#include <QStandardPaths>

namespace sentinel::daemon {

DaemonService::DaemonService(QObject* parent) : QObject(parent) {
    connect(&m_healthTimer, &QTimer::timeout, this, &DaemonService::performHealthCheck);
}

DaemonService::~DaemonService() {
    m_healthTimer.stop();
    if (m_controller) {
        m_controller->stopChatGeneration();
        if (m_controller->agentRuntime()) {
            m_controller->agentRuntime()->shutdown();
        }
    }
    m_ipcServer.setController(nullptr);
    m_controller.reset();
    m_settings.reset();
    m_ipcServer.stopServer();
}

bool DaemonService::initialize(const QString& socketPath) {
    qInfo().noquote() << "Initializing Sentinel Headless Daemon Service...";

    // Claim the user endpoint before opening stores or recovering interrupted runs.
    // A losing second daemon must not mutate the active daemon's persistence.
    if (!m_ipcServer.startServer(socketPath)) {
        return false;
    }

    // Stage A keeps the direct Desktop runtime on its own existing profile.
    // Sharing its databases here could interrupt a still-active desktop turn.
    if (!m_pathProvider.isPortable()) {
        m_pathProvider.setProfileDirectory(
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    }

    m_settings = std::make_unique<sentinel::core::AppSettings>(
        std::make_unique<sentinel::core::DpapiEncryptedSettingsStore>(
            std::make_unique<sentinel::core::JsonSettingsStore>(
                m_pathProvider.settingsFilePath())));

    sentinel::core::ApplicationControllerBuilder builder;
    m_controller = builder.withStandardDefaults(m_pathProvider, *m_settings).build();
    m_controller->attachControlledTaskSettings(*m_settings);
    m_controller->setToolPermissionPolicyState(m_settings->defaultPermissionPolicyState());
    m_controller->setLmStudioEndpoint(m_settings->lmStudioEndpoint());
    m_controller->setLlamaCppEndpoint(m_settings->llamaCppEndpoint());
    m_controller->setSelectedRuntimeProvider(m_settings->selectedRuntimeProvider());
    m_controller->setSelectedLocalModel(
        m_settings->selectedModelForProvider(m_settings->selectedRuntimeProvider()));
    m_controller->configureMcpServers(m_settings->mcpServersJson());
    m_ipcServer.setController(m_controller.get());

    m_controller->refreshOllamaStatus();

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
