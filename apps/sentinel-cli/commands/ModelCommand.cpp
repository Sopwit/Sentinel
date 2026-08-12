// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ModelCommand.h"

#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/platform/DpapiEncryptedSettingsStore.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/platform/StandardPathProvider.h"
#include "sentinel/core/runtime/OllamaRuntime.h"

#include <QEventLoop>
#include <QTimer>

#include <iomanip>
#include <iostream>

namespace sentinel::cli {

int executeModelCommand(const QStringList& args) {
    const QString sub = args.value(0, QStringLiteral("list"));

    sentinel::core::StandardPathProvider pathProvider;
    sentinel::core::AppSettings settings(
        std::make_unique<sentinel::core::DpapiEncryptedSettingsStore>(
            std::make_unique<sentinel::core::JsonSettingsStore>(pathProvider.settingsFilePath())));

    sentinel::core::ApplicationControllerBuilder builder;
    auto controller = builder.withStandardDefaults(pathProvider, settings).build();

    if (sub == QStringLiteral("list")) {
        std::cout << "Ollama Endpoint: " << settings.ollamaEndpoint().toStdString() << std::endl;

        QEventLoop loop;
        auto statusRefreshed = false;
        QObject::connect(controller.get(),
                         &sentinel::core::ApplicationController::ollamaStatusChanged, &loop,
                         [&loop, &statusRefreshed]() {
                             statusRefreshed = true;
                             loop.quit();
                         });

        controller->refreshOllamaStatus();
        QTimer::singleShot(5000, &loop, [&loop]() { loop.quit(); });
        loop.exec();

        std::cout << "Ollama Health:    " << controller->ollamaHealthStatus().toStdString() << " ("
                  << controller->ollamaConnectionStatus().toStdString() << ")" << std::endl;

        const auto models = controller->currentOllamaModels();
        if (models.isEmpty()) {
            std::cout << "Installed models: none" << std::endl;
        } else {
            std::cout << "Installed models:" << std::endl;
            for (const auto& model : models) {
                const double sizeMb = static_cast<double>(model.sizeBytes) / (1024.0 * 1024.0);
                std::cout << "  - " << model.name.toStdString();
                if (model.sizeBytes > 0) {
                    std::cout << " (" << std::fixed << std::setprecision(1) << sizeMb << " MB)";
                }
                std::cout << std::endl;
            }
        }
        return 0;
    }

    if (sub == QStringLiteral("pull")) {
        const QString modelId = args.value(1);
        if (modelId.isEmpty()) {
            std::cerr << "Usage: sentinel-cli model pull <model_name>" << std::endl;
            return 1;
        }

        OllamaModelPuller puller;
        puller.setEndpoint(settings.ollamaEndpoint());

        QEventLoop loop;
        auto finished = false;
        QObject::connect(&puller, &OllamaModelPuller::pullFinished, &loop,
                         [&loop, &finished](const QString& model, bool success) {
                             finished = true;
                             std::cout << std::endl
                                       << (success ? "Pull completed: "
                                                   : "Pull failed: ")
                                       << model.toStdString() << std::endl;
                             loop.quit();
                         });

        QTimer progressTimer;
        QObject::connect(&progressTimer, &QTimer::timeout, &loop, [&puller, &finished]() {
            if (finished) {
                return;
            }
            std::cout << "\r  Progress: " << static_cast<int>(puller.progress() * 100)
                      << "% - " << puller.statusText().toStdString() << "          ";
            std::cout.flush();
        });
        progressTimer.start(250);

        std::cout << "Pulling model: " << modelId.toStdString() << " ..." << std::endl;
        puller.pull(modelId);

        loop.exec();
        return 0;
    }

    std::cerr << "Unknown model subcommand: " << sub.toStdString() << std::endl;
    std::cerr << "Available: list, pull" << std::endl;
    return 1;
}

} // namespace sentinel::cli
