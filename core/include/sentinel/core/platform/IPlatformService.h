// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace sentinel::core {

class IPlatformService {
public:
    virtual ~IPlatformService() = default;

    virtual QString platformName() const = 0;
};

} // namespace sentinel::core
