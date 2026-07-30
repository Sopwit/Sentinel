// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "bootstrap/ApplicationBootstrapper.h"

#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    sentinel::desktop::ApplicationBootstrapper bootstrapper(argc, argv);

    if (!bootstrapper.ensureSingleInstance()) {
        return 1;
    }

    bootstrapper.initializeLogging();
    bootstrapper.initializeGraphics();
    bootstrapper.initializePlatformIntegrations();

    if (!bootstrapper.setupQmlEngine(app)) {
        return -1;
    }

    return QApplication::exec();
}
