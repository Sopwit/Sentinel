// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QtGlobal>

namespace sentinel::core {

class IClock {
public:
    Q_DISABLE_COPY(IClock)
    IClock() = default;
    virtual ~IClock() = default;

    virtual QDateTime nowUtc() const = 0;
};

class SystemClock final : public IClock {
public:
    QDateTime nowUtc() const override;
};

} // namespace sentinel::core
