// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

namespace sentinel::core {

class ISystemIntegrationService {
public:
    virtual ~ISystemIntegrationService() = default;

    virtual bool isAvailable() const {
        return false;
    }
};

} // namespace sentinel::core
