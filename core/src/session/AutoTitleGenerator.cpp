// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/AutoTitleGenerator.h"
#include <QRegularExpression>
#include <QStringList>

namespace sentinel::core {

QString AutoTitleGenerator::generateTitle(const QString& conversationContent) const {
    if (conversationContent.isEmpty())
        return "New Session";

    QString topic = extractTopic(conversationContent);
    QString action = summarizeKeyAction(conversationContent);

    QString title;
    if (!topic.isEmpty() && !action.isEmpty()) {
        title = QStringLiteral("%1: %2").arg(action, topic);
    } else if (!topic.isEmpty()) {
        title = topic;
    } else if (!action.isEmpty()) {
        title = action;
    } else {
        title = "New Session";
    }

    return cleanTitle(title);
}

QString AutoTitleGenerator::cleanTitle(const QString& title) const {
    QString cleaned = title;
    cleaned.remove(QRegularExpression("```[^`]*```"));
    cleaned.remove(QRegularExpression("<[^>]*>"));
    cleaned.remove(QRegularExpression("\\*\\*|\\*|__|_"));
    cleaned = cleaned.trimmed();
    if (cleaned.length() > m_maxLength) {
        cleaned = cleaned.left(m_maxLength - 3) + "...";
    }
    return cleaned.isEmpty() ? "New Session" : cleaned;
}

int AutoTitleGenerator::maxLength() const {
    return m_maxLength;
}

QString AutoTitleGenerator::extractTopic(const QString& text) const {
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    for (const auto& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.isEmpty() || trimmed.startsWith("```"))
            continue;
        if (trimmed.length() > 10 && trimmed.length() < 100) {
            return trimmed;
        }
    }
    return {};
}

QString AutoTitleGenerator::summarizeKeyAction(const QString& text) const {
    QRegularExpression actionRx(
        "(implement|fix|add|create|update|refactor|debug|optimize|build|test|deploy|configure|"
        "setup|write|generate|fix|resolve|debug|analyze|review|improve|enhance|modify|change|"
        "replace|remove|delete|fix|patch|hotfix|bugfix|feature|chore|docs|style|perf|test|ci|build|"
        "revert)",
        QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatch match = actionRx.match(text);
    if (match.hasMatch()) {
        return match.captured(1).toLower();
    }
    return {};
}

} // namespace sentinel::core
