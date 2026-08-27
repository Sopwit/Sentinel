// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

class JsoncParser {
public:
    static QJsonObject parse(const QString& jsonc, QString& error);
    static QString stripComments(const QString& jsonc);
    static bool isValid(const QString& jsonc, QString& error);
    static QJsonObject parseFile(const QString& filePath, QString& error);
};

} // namespace sentinel::core
