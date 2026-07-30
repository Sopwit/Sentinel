// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "service/DaemonService.h"

#include "sentinel/core/app/AppMetadata.h"

#include <QCoreApplication>
#include <QDebug>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(sentinel::core::AppMetadata::displayName() + QStringLiteral(" Daemon"));
    QCoreApplication::setOrganizationName(sentinel::core::AppMetadata::organizationName());
    QCoreApplication::setApplicationVersion(sentinel::core::AppMetadata::version());

    sentinel::daemon::DaemonService service;
    if (!service.initialize()) {
        qCritical().noquote() << "Failed to start Sentinel Daemon Service.";
        return 1;
    }

    qInfo().noquote() << "Sentinel Daemon running in background.";
    return QCoreApplication::exec();
}
