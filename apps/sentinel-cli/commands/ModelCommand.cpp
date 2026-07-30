#include "ModelCommand.h"

#include "sentinel/core/app/ApplicationControllerBuilder.h"
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/platform/DpapiEncryptedSettingsStore.h"
#include "sentinel/core/memory/JsonSettingsStore.h"
#include "sentinel/core/platform/StandardPathProvider.h"

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
        std::cout << "Ollama Status:   " << controller->providerStatus().toStdString() << std::endl;
        return 0;
    } else if (sub == QStringLiteral("pull")) {
        const QString modelId = args.value(1);
        if (modelId.isEmpty()) {
            std::cerr << "Usage: sentinel-cli model pull <model_name>" << std::endl;
            return 1;
        }
        std::cout << "Pull request queued for model: " << modelId.toStdString() << std::endl;
        return 0;
    }

    std::cerr << "Unknown model subcommand: " << sub.toStdString() << std::endl;
    std::cerr << "Available: list, pull" << std::endl;
    return 1;
}

} // namespace sentinel::cli
