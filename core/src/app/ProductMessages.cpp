// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/app/ProductMessages.h"
#include <QCoreApplication>

namespace sentinel::core {
QString ProductMessages::present(const ProductMessage& message) {
    const auto& id = message.id;
    if (id == QLatin1String("settings.unavailable-item"))
        return QCoreApplication::translate("ProductMessages", "%1 is unavailable.")
            .arg(message.parameters.value(QStringLiteral("item")).toString().left(80));
    if (id == QLatin1String("settings.invalid-type"))
        return QCoreApplication::translate("ProductMessages", "This setting has the wrong value type.");
    if (id == QLatin1String("settings.invalid-choice"))
        return QCoreApplication::translate("ProductMessages", "Choose a supported value.");
    if (id == QLatin1String("settings.empty-value"))
        return QCoreApplication::translate("ProductMessages", "A value is required.");
    if (id == QLatin1String("settings.invalid-endpoint"))
        return QCoreApplication::translate("ProductMessages", "Enter an HTTP or HTTPS endpoint without credentials in the URL.");
    if (id == QLatin1String("settings.invalid-range"))
        return QCoreApplication::translate("ProductMessages", "Enter a value within the supported range.");
    if (id == QLatin1String("settings.unavailable-device"))
        return QCoreApplication::translate("ProductMessages", "This audio device is unavailable.");
    if (id == QLatin1String("settings.unknown-workspace"))
        return QCoreApplication::translate("ProductMessages", "This workspace is unavailable.");
    if (id == QLatin1String("settings.unknown-provider"))
        return QCoreApplication::translate("ProductMessages", "This provider is unavailable.");
    if (id == QLatin1String("settings.invalid-model") ||
        id == QLatin1String("onboarding.model-unavailable"))
        return QCoreApplication::translate("ProductMessages", "Choose an available model or continue without one.");
    if (id == QLatin1String("settings.override-absent"))
        return QCoreApplication::translate("ProductMessages", "This workspace already inherits the setting.");
    if (id == QLatin1String("onboarding.cloud-blocked"))
        return QCoreApplication::translate("ProductMessages", "Local mode cannot select a cloud provider.");
    if (id == QLatin1String("onboarding.invalid-mode"))
        return QCoreApplication::translate("ProductMessages", "Choose local, cloud, or hybrid processing.");
    return QCoreApplication::translate("ProductMessages", "The requested change is unavailable.");
}
} // namespace sentinel::core
