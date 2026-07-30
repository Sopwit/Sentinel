// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QtGlobal>

namespace sentinel::core {

class IIntegration {
public:
    Q_DISABLE_COPY(IIntegration)
    IIntegration() = default;
    virtual ~IIntegration() = default;

    virtual QString id() const = 0;
    virtual QString displayName() const = 0;
    virtual bool isAvailable() const = 0;
};

} // namespace sentinel::core
