// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

namespace sentinel::core::plugin {
inline constexpr int PluginHostProtocolVersion = 2;
inline constexpr int NativePluginAbiVersion = 3;
inline constexpr qsizetype PluginHostMaxFrame = 1024 * 1024;

inline QByteArray pluginHostFrame(const QJsonObject& object) {
    return QJsonDocument(object).toJson(QJsonDocument::Compact) + '\n';
}

inline bool parsePluginHostFrame(const QByteArray& line, QJsonObject* object) {
    if (line.size() > PluginHostMaxFrame) return false;
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(line, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) return false;
    *object = document.object();
    return object->value(QStringLiteral("protocol")).toInt(-1) == PluginHostProtocolVersion;
}
} // namespace sentinel::core::plugin
