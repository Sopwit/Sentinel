// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace sentinel::core {

struct StructuredOutputSchema {
    QString type;
    QJsonObject properties;
    QJsonArray required;
};

class StructuredOutputTool {
public:
    QString generateJsonSchema(const StructuredOutputSchema& schema) const;
    QJsonObject validateOutput(const QString& json, const StructuredOutputSchema& schema) const;
    QString wrapToolResponse(const QJsonObject& data, const QString& mimeType = "application/json") const;
    bool isValidJson(const QString& text) const;
};

} // namespace sentinel::core
