// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/output/StructuredOutputTool.h"
#include <QJsonArray>
#include <QJsonDocument>

namespace sentinel::core {

QString StructuredOutputTool::generateJsonSchema(const StructuredOutputSchema& schema) const {
    QJsonObject obj;
    obj["type"] = schema.type;
    obj["properties"] = schema.properties;
    obj["required"] = schema.required;
    return QJsonDocument(obj).toJson(QJsonDocument::Indented);
}

QJsonObject StructuredOutputTool::validateOutput(const QString& json,
                                                 const StructuredOutputSchema& schema) const {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        QJsonObject err;
        err["error"] = error.errorString();
        return err;
    }
    return doc.object();
}

QString StructuredOutputTool::wrapToolResponse(const QJsonObject& data,
                                               const QString& mimeType) const {
    Q_UNUSED(mimeType)
    return QJsonDocument(data).toJson(QJsonDocument::Compact);
}

bool StructuredOutputTool::isValidJson(const QString& text) const {
    QJsonParseError error;
    QJsonDocument::fromJson(text.toUtf8(), &error);
    return error.error == QJsonParseError::NoError;
}

} // namespace sentinel::core
