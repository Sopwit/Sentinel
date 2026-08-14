// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/config/JsoncParser.h"
#include <QJsonDocument>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

namespace sentinel::core {

QJsonObject JsoncParser::parse(const QString& jsonc, QString& error) {
    QString stripped = stripComments(jsonc);
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(stripped.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        error = parseError.errorString();
        return {};
    }
    return doc.object();
}

QString JsoncParser::stripComments(const QString& jsonc) {
    QString result = jsonc;
    QRegularExpression lineComment("//[^\n]*");
    result.remove(lineComment);

    QRegularExpression blockComment("/\\*[\\s\\S]*?\\*/");
    result.remove(blockComment);

    QRegularExpression trailingComma(",\\s*([}\\]])");
    result.replace(trailingComma, "\\1");

    return result;
}

bool JsoncParser::isValid(const QString& jsonc, QString& error) {
    parse(jsonc, error);
    return error.isEmpty();
}

QJsonObject JsoncParser::parseFile(const QString& filePath, QString& error) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("Cannot open file: %1").arg(filePath);
        return {};
    }
    return parse(QTextStream(&file).readAll(), error);
}

} // namespace sentinel::core
