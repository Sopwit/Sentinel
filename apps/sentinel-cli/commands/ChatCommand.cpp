// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChatCommand.h"

#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/platform/DpapiEncryptedSettingsStore.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/platform/StandardPathProvider.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QTextStream>
#include <QTimer>
#include <iostream>

namespace sentinel::cli {

int executeChatCommand(const QStringList& args) {
    if (args.isEmpty()) {
        std::cerr << "Usage: sentinel-cli chat <prompt>" << std::endl;
        return 1;
    }

    const QString prompt = args.join(QLatin1Char(' '));

    sentinel::core::StandardPathProvider pathProvider;
    sentinel::core::AppSettings settings(
        std::make_unique<sentinel::core::DpapiEncryptedSettingsStore>(
            std::make_unique<sentinel::core::JsonSettingsStore>(pathProvider.settingsFilePath())));

    sentinel::core::ApplicationControllerBuilder builder;
    auto controller = builder.withStandardDefaults(pathProvider, settings).build();

    QEventLoop loop;
    QObject::connect(controller.get(), &sentinel::core::ApplicationController::agentResponseChanged,
                     &loop, [&controller, &loop]() {
                         const QString response = controller->lastAgentResponse();
                         std::cout << "\nSentinel: " << response.toStdString() << std::endl;
                         loop.quit();
                     });

    std::cout << "User: " << prompt.toStdString() << std::endl;
    controller->sendMessage(prompt);

    // Timeout after 5 seconds if local echo or provider hasn't responded
    QTimer::singleShot(5000, &loop, [&loop]() {
        std::cout << "[Timeout waiting for response]" << std::endl;
        loop.quit();
    });

    loop.exec();
    return 0;
}

} // namespace sentinel::cli
