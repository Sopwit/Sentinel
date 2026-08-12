// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/app/IContextEngine.h"

namespace sentinel::core {

BasicContextEngine::BasicContextEngine(IMemoryStore* memoryStore)
    : memoryStore_(memoryStore) {}

QString BasicContextEngine::buildContextForPrompt(const QString& prompt) const {
    const auto trimmed = prompt.trimmed();

    QStringList lines;
    if (memoryStore_ != nullptr) {
        for (const auto& entry : memoryStore_->entries()) {
            const auto key = entry.first.trimmed();
            const auto value = entry.second.trimmed();
            if (key.isEmpty() || value.isEmpty()) {
                continue;
            }
            lines.append(QStringLiteral("%1 = %2").arg(key, value));
        }
    }

    if (lines.isEmpty()) {
        return trimmed;
    }

    return QStringLiteral("--- Memory Context ---\n%1\n---\n\nUser prompt:\n%2")
        .arg(lines.join(QStringLiteral("\n")), trimmed);
}

} // namespace sentinel::core
