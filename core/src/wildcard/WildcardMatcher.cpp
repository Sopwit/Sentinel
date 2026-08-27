// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/wildcard/WildcardMatcher.h"

namespace sentinel::core {

bool WildcardMatcher::match(const QString& pattern, const QString& text) {
    QRegularExpression re(regexFromWildcard(pattern));
    return re.match(text).hasMatch();
}

bool WildcardMatcher::isPattern(const QString& text) {
    return text.contains('*') || text.contains('?');
}

QString WildcardMatcher::regexFromWildcard(const QString& wildcard) {
    QString regex;
    for (int i = 0; i < wildcard.size(); ++i) {
        QChar c = wildcard[i];
        if (c == '*')
            regex += ".*";
        else if (c == '?')
            regex += ".";
        else if (c.isPunct()) {
            regex += "\\";
            regex += QString(c);
        } else
            regex += c;
    }
    return "^" + regex + "$";
}

bool WildcardMatcher::containsWildcard(const QString& pattern) {
    return pattern.contains('*') || pattern.contains('?');
}

QStringList WildcardMatcher::matchingFiles(const QString& pattern, const QStringList& files) {
    QStringList result;
    for (const auto& f : files) {
        if (match(pattern, f))
            result.append(f);
    }
    return result;
}

} // namespace sentinel::core
