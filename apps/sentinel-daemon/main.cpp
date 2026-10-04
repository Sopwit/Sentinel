// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "service/DaemonService.h"

#include "sentinel/core/app/AppMetadata.h"

#include <QCommandLineParser>
#include <QTimer>
#include <csignal>

namespace {
volatile std::sig_atomic_t shutdownRequested = 0;
void requestShutdown(int) {
    shutdownRequested = 1;
}
} // namespace
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(sentinel::core::AppMetadata::displayName() +
                                         QStringLiteral(" Daemon"));
    QCoreApplication::setOrganizationName(sentinel::core::AppMetadata::organizationName());
    QCoreApplication::setApplicationVersion(sentinel::core::AppMetadata::version());

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption(
        {"portable", "Store state beside the daemon executable (disposable profiles)."});
    parser.addOption({"socket", "Owner-only local socket path.", "path"});
    parser.addOption({"profile-name", "Isolated QStandardPaths identity for testing.", "name"});
    parser.process(app);
    if (parser.isSet("profile-name")) {
        QCoreApplication::setApplicationName(parser.value("profile-name"));
    }
    sentinel::daemon::DaemonService service;
    if (!service.initialize(parser.value("socket"))) {
        qCritical().noquote() << "Failed to start Sentinel Daemon Service.";
        return 1;
    }

    std::signal(SIGINT, requestShutdown);
    std::signal(SIGTERM, requestShutdown);
    QTimer signalTimer;
    QObject::connect(&signalTimer, &QTimer::timeout, &app, [&app] {
        if (shutdownRequested) {
            app.quit();
        }
    });
    signalTimer.start(50);
    qInfo().noquote() << "Sentinel Daemon running in background.";
    return QCoreApplication::exec();
}
