// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/interfaces/IMemoryStore.h"

#include <QString>
#include <QtGlobal>

namespace sentinel::core {

class IContextEngine {
public:
    Q_DISABLE_COPY(IContextEngine)
    IContextEngine() = default;
    virtual ~IContextEngine() = default;

    virtual QString buildContextForPrompt(const QString& prompt) const = 0;
};

class BasicContextEngine final : public IContextEngine {
public:
    explicit BasicContextEngine(IMemoryStore* memoryStore = nullptr);

    QString buildContextForPrompt(const QString& prompt) const override;

private:
    IMemoryStore* memoryStore_;
};

} // namespace sentinel::core
