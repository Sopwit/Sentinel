// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/interfaces/IChatProvider.h"

namespace sentinel::core {

class LocalEchoProvider final : public IChatProvider {
public:
    QString name() const override;
    ChatProviderStatus status() const override;
    ChatProviderReply sendMessage(const QString& message) override;
};

} // namespace sentinel::core
