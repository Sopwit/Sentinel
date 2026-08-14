// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/CompactionPrompt.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace sentinel::core {

QString CompactionPrompt::buildPrompt(const QJsonArray& messages, const QString& previousSummary) {
    QString messagesStr;
    for (const auto& msg : messages) {
        QJsonObject msgObj = msg.toObject();
        QString role = msgObj["role"].toString();
        QString content = msgObj["content"].toString();
        messagesStr += QStringLiteral("[%1]: %2\n\n").arg(role, content);
    }

    QString summary = previousSummary.isEmpty() ? "No previous summary." : previousSummary;

    return QString(PromptTemplate).arg(summary, messagesStr);
}

QJsonObject CompactionPrompt::parseResponse(const QString& response) {
    QJsonObject parsed;

    // Simple parsing of the structured response
    QStringList lines = response.split('\n');
    QString currentSection;
    QStringList currentContent;

    for (const QString& line : lines) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith("### ")) {
            // Save previous section
            if (!currentSection.isEmpty()) {
                parsed[currentSection] = currentContent.join('\n').trimmed();
            }

            // Start new section
            currentSection = trimmed.mid(4).trimmed();
            currentContent.clear();
        } else if (!trimmed.isEmpty()) {
            currentContent.append(trimmed);
        }
    }

    // Save last section
    if (!currentSection.isEmpty()) {
        parsed[currentSection] = currentContent.join('\n').trimmed();
    }

    return parsed;
}

QString CompactionPrompt::extractObjective(const QJsonObject& parsedResponse) {
    return parsedResponse["Objective"].toString();
}

QString CompactionPrompt::extractImportantDetails(const QJsonObject& parsedResponse) {
    return parsedResponse["Important Details"].toString();
}

QString CompactionPrompt::extractWorkState(const QJsonObject& parsedResponse) {
    return parsedResponse["Work State"].toString();
}

QString CompactionPrompt::extractNextMove(const QJsonObject& parsedResponse) {
    return parsedResponse["Next Move"].toString();
}

QString CompactionPrompt::extractRelevantFiles(const QJsonObject& parsedResponse) {
    return parsedResponse["Relevant Files"].toString();
}

QString CompactionPrompt::formatSummary(const QJsonObject& parsedResponse) {
    QStringList parts;

    QString objective = extractObjective(parsedResponse);
    if (!objective.isEmpty()) {
        parts << QStringLiteral("Objective: %1").arg(objective);
    }

    QString details = extractImportantDetails(parsedResponse);
    if (!details.isEmpty()) {
        parts << QStringLiteral("Important Details: %1").arg(details);
    }

    QString workState = extractWorkState(parsedResponse);
    if (!workState.isEmpty()) {
        parts << QStringLiteral("Work State: %1").arg(workState);
    }

    QString nextMove = extractNextMove(parsedResponse);
    if (!nextMove.isEmpty()) {
        parts << QStringLiteral("Next Move: %1").arg(nextMove);
    }

    QString files = extractRelevantFiles(parsedResponse);
    if (!files.isEmpty()) {
        parts << QStringLiteral("Relevant Files: %1").arg(files);
    }

    return parts.join('\n');
}

} // namespace sentinel::core
