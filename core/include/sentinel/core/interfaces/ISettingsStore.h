// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QtGlobal>

namespace sentinel::core {

class ISettingsStore {
public:
    Q_DISABLE_COPY(ISettingsStore)
    ISettingsStore() = default;
    virtual ~ISettingsStore() = default;

    virtual QString value(const QString& key, const QString& defaultValue = QString()) const = 0;
    virtual void setValue(QString key, QString value) = 0;
};

} // namespace sentinel::core

namespace sentinel::core::interfaces {
    using ISettingsStore = ::sentinel::core::ISettingsStore;
}
