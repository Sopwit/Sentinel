// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/config/ConfigVariableSubstitutor.h"
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>

namespace sentinel::core {

ConfigVariableSubstitutor::ConfigVariableSubstitutor(const VariableSubstitutionConfig& config)
    : m_config(config) {}

QString ConfigVariableSubstitutor::substitute(const QString& input) const {
    QString result = input;

    QRegularExpression envRegex("\\{env:([A-Za-z_][A-Za-z0-9_]*)\\}");
    QRegularExpressionMatchIterator envIt = envRegex.globalMatch(result);
    while (envIt.hasNext()) {
        QRegularExpressionMatch match = envIt.next();
        QString fullMatch = match.captured(0);
        QString varName = match.captured(1);
        QString value = substituteEnv(varName);
        result.replace(fullMatch, value);
    }

    QRegularExpression fileRegex("\\{file:([^}]+)\\}");
    QRegularExpressionMatchIterator fileIt = fileRegex.globalMatch(result);
    while (fileIt.hasNext()) {
        QRegularExpressionMatch match = fileIt.next();
        QString fullMatch = match.captured(0);
        QString filePath = match.captured(1);
        QString value = substituteFile(filePath);
        result.replace(fullMatch, value);
    }

    return result;
}

QString ConfigVariableSubstitutor::substituteEnv(const QString& varName) const {
    if (!m_config.allowEnv)
        return QStringLiteral("{env:%1}").arg(varName);
    QByteArray value = qgetenv(varName.toUtf8().constData());
    return QString::fromUtf8(value);
}

QString ConfigVariableSubstitutor::substituteFile(const QString& filePath) const {
    if (!m_config.allowFile)
        return QStringLiteral("{file:%1}").arg(filePath);

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QStringLiteral("{file:%1}").arg(filePath);
    }

    QString content = QTextStream(&file).readAll();
    QJsonDocument doc = QJsonDocument::fromJson(content.toUtf8());
    if (!doc.isNull()) {
        return doc.toJson(QJsonDocument::Compact);
    }
    return content;
}

bool ConfigVariableSubstitutor::validate(const QString& input, QString& error) const {
    QRegularExpression envRegex("\\{env:([A-Za-z_][A-Za-z0-9_]*)\\}");
    QRegularExpression fileRegex("\\{file:([^}]+)\\}");

    QRegularExpressionMatchIterator it = envRegex.globalMatch(input);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        if (!m_config.allowEnv) {
            error = QStringLiteral("Environment variable substitution not allowed: %1")
                        .arg(match.captured(0));
            return false;
        }
    }

    it = fileRegex.globalMatch(input);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        if (!m_config.allowFile) {
            error = QStringLiteral("File substitution not allowed: %1").arg(match.captured(0));
            return false;
        }
    }

    return true;
}

} // namespace sentinel::core
