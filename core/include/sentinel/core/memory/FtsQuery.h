// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace sentinel::core {

inline QString ftsMatchQuery(const QString& text) {
    static const QRegularExpression token(QStringLiteral("[\\p{L}\\p{N}_]+"));
    QStringList terms;
    auto matches = token.globalMatch(text);
    while (matches.hasNext() && terms.size() < 6) {
        const auto term = matches.next().captured().left(64);
        if (!term.isEmpty() && !terms.contains(term, Qt::CaseInsensitive))
            terms.append(QStringLiteral("\"%1\"").arg(term));
    }
    return terms.join(QStringLiteral(" OR "));
}

} // namespace sentinel::core
