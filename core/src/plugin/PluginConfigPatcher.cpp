// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/plugin/PluginConfigPatcher.h"

namespace sentinel::core {

QJsonObject PluginConfigPatcher::patch(const QJsonObject& original, const QJsonObject& patchData) const {
    QJsonObject result = original;
    for (auto it = patchData.begin(); it != patchData.end(); ++it) {
        if (it.value().isObject() && result[it.key()].isObject()) {
            result[it.key()] = patch(result[it.key()].toObject(), it.value().toObject());
        } else {
            result[it.key()] = it.value();
        }
    }
    return result;
}

QJsonObject PluginConfigPatcher::merge(const QJsonObject& base, const QJsonObject& override) const {
    return patch(base, override);
}

QJsonObject PluginConfigPatcher::applyDefaults(const QJsonObject& config, const QJsonObject& defaults) const {
    QJsonObject result = defaults;
    for (auto it = config.begin(); it != config.end(); ++it) {
        result[it.key()] = it.value();
    }
    return result;
}

bool PluginConfigPatcher::validate(const QJsonObject& config, const QJsonObject& schema) const {
    Q_UNUSED(config)
    Q_UNUSED(schema)
    return true;
}

} // namespace sentinel::core
