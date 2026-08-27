// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

class PluginConfigPatcher {
public:
    QJsonObject patch(const QJsonObject& original, const QJsonObject& patchData) const;
    QJsonObject merge(const QJsonObject& base, const QJsonObject& override) const;
    QJsonObject applyDefaults(const QJsonObject& config, const QJsonObject& defaults) const;
    bool validate(const QJsonObject& config, const QJsonObject& schema) const;
};

} // namespace sentinel::core
