// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QUuid>
#include <QDateTime>

namespace sentinel::core {

class SessionIdGenerator {
public:
    static QString generate();
    static QString generateShort();
    static bool isValid(const QString& id);
    static QDateTime timestampFromId(const QString& id);
    static QString formatSessionId(const QString& raw);
};

} // namespace sentinel::core
