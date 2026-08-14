// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QRegularExpression>

namespace sentinel::core {

class WildcardMatcher {
public:
    static bool match(const QString& pattern, const QString& text);
    static bool isPattern(const QString& text);
    static QString regexFromWildcard(const QString& wildcard);
    static bool containsWildcard(const QString& pattern);
    static QStringList matchingFiles(const QString& pattern, const QStringList& files);
};

} // namespace sentinel::core
