// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/surrogate/SurrogateSanitizer.h"

namespace sentinel::core {

QString SurrogateSanitizer::sanitize(const QString& input) {
    QString result;
    result.reserve(input.size());
    for (int i = 0; i < input.size(); ++i) {
        QChar c = input[i];
        if (c.isHighSurrogate() && i + 1 < input.size() && input[i + 1].isLowSurrogate()) {
            result.append(c);
            result.append(input[i + 1]);
            ++i;
        } else if (c.isHighSurrogate() || c.isLowSurrogate()) {
            continue;
        } else {
            result.append(c);
        }
    }
    return result;
}

QByteArray SurrogateSanitizer::sanitize(const QByteArray& input) {
    return sanitize(QString::fromUtf8(input)).toUtf8();
}

bool SurrogateSanitizer::containsSurrogates(const QString& input) {
    for (const QChar& c : input) {
        if (c.isHighSurrogate() || c.isLowSurrogate()) return true;
    }
    return false;
}

bool SurrogateSanitizer::isValidUtf16(const QString& input) {
    for (int i = 0; i < input.size(); ++i) {
        QChar c = input[i];
        if (c.isHighSurrogate()) {
            if (i + 1 >= input.size() || !input[i + 1].isLowSurrogate()) return false;
            ++i;
        } else if (c.isLowSurrogate()) {
            return false;
        }
    }
    return true;
}

QString SurrogateSanitizer::removeInvalidCodepoints(const QString& input) {
    return sanitize(input);
}

} // namespace sentinel::core
