// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef SENTINEL_DAEMON_DAEMONSERVICE_H
#define SENTINEL_DAEMON_DAEMONSERVICE_H

#include "DaemonIpcServer.h"

#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/app/ApplicationController.h"
#include "sentinel/core/platform/StandardPathProvider.h"

#include <QObject>
#include <QTimer>
#include <memory>

namespace sentinel::daemon {

class DaemonService final : public QObject {
    Q_OBJECT
public:
    explicit DaemonService(QObject* parent = nullptr);
    ~DaemonService() override;

    bool initialize();

private slots:
    void performHealthCheck();

private:
    sentinel::core::StandardPathProvider m_pathProvider;
    std::unique_ptr<sentinel::core::AppSettings> m_settings;
    std::unique_ptr<sentinel::core::ApplicationController> m_controller;
    DaemonIpcServer m_ipcServer;
    QTimer m_healthTimer;
};

} // namespace sentinel::daemon

#endif // SENTINEL_DAEMON_DAEMONSERVICE_H
