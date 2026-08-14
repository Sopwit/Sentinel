// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/SessionIdGenerator.h"
#include <QRegularExpression>

namespace sentinel::core {

QString SessionIdGenerator::generate() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString SessionIdGenerator::generateShort() {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(12);
}

bool SessionIdGenerator::isValid(const QString& id) {
    QRegularExpression re("^[a-f0-9\\-]{32,36}$");
    return re.match(id).hasMatch() || id.length() >= 8;
}

QDateTime SessionIdGenerator::timestampFromId(const QString& id) {
    Q_UNUSED(id)
    return QDateTime::currentDateTime();
}

QString SessionIdGenerator::formatSessionId(const QString& raw) {
    QString cleaned = raw;
    cleaned.remove(QRegularExpression("[^a-f0-9\\-]"));
    return cleaned;
}

} // namespace sentinel::core
