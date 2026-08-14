// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

struct StructuredSummary {
    QString objective;
    QString importantDetails;
    QString completedWork;
    QString activeWork;
    QString blockedWork;
    QString nextMove;
    QString relevantFiles;
    QString rawMarkdown;
};

class StructuredCompactionSummary {
public:
    StructuredSummary buildSummary(const QString& conversationText, const StructuredSummary& previousSummary = {}) const;
    QString formatMarkdown(const StructuredSummary& summary) const;
    StructuredSummary parseSummary(const QString& markdown) const;
    QString incrementalUpdate(const StructuredSummary& previous, const QString& newConversation) const;
    int estimateTokens(const QString& text) const;
    int preserveRecentTokens() const;

private:
    QString extractObjective(const QString& text) const;
    QString extractDetails(const QString& text) const;
    QString extractFiles(const QString& text) const;
    QString extractCommands(const QString& text) const;
    QString extractErrors(const QString& text) const;
    QString extractSymbols(const QString& text) const;
};

} // namespace sentinel::core
