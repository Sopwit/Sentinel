// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QByteArray>
#include <QString>

namespace sentinel::core {

class SurrogateSanitizer {
public:
    static QString sanitize(const QString& input);
    static QByteArray sanitize(const QByteArray& input);
    static bool containsSurrogates(const QString& input);
    static bool isValidUtf16(const QString& input);
    static QString removeInvalidCodepoints(const QString& input);
};

} // namespace sentinel::core
