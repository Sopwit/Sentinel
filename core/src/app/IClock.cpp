// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/app/IClock.h"

namespace sentinel::core {

QDateTime SystemClock::nowUtc() const {
    return QDateTime::currentDateTimeUtc();
}

} // namespace sentinel::core
