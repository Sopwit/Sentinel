// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/plugin/PluginManifest.h"
#include <QList>
#include <QMap>
#include <QString>

namespace sentinel::core::plugin {

struct ResolutionResult {
    bool success{false};
    QList<QString> loadOrder;
    QString errorMessage;
};

class PluginDependencyResolver {
public:
    PluginDependencyResolver() = default;

    static ResolutionResult resolve(const QList<PluginManifest>& manifests);
};

} // namespace sentinel::core::plugin
