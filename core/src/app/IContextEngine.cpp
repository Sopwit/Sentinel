// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/app/IContextEngine.h"

namespace sentinel::core {

QString BasicContextEngine::buildContextForPrompt(const QString& prompt) const {
    return prompt.trimmed();
}

} // namespace sentinel::core
