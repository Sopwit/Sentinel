// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace sentinel::core {

class INotificationService {
public:
    virtual ~INotificationService() = default;

    virtual bool isAvailable() const {
        return false;
    }
};

} // namespace sentinel::core
