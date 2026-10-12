// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QStringList>
#include <QVariantMap>
namespace sentinel::desktop {
// Pure presentation of existing settings/registry projections; no lifecycle or execution authority.
struct ExtensionsHubPresentation {
    static QVariantMap inventory(bool connected, bool loaded, const QVariantMap& settings,
                                 const QStringList& toolIds, const QStringList& policy);
    static QVariantMap detail(bool connected, const QString& id, const QVariantMap& response,
                              bool answered = false);
};
} // namespace sentinel::desktop
