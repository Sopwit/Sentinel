// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/compaction/StructuredCompactionSummary.h"
#include <QStringList>
#include <QRegularExpression>

namespace sentinel::core {

StructuredSummary StructuredCompactionSummary::buildSummary(const QString& conversationText, const StructuredSummary& previousSummary) const {
    StructuredSummary summary;

    summary.objective = extractObjective(conversationText);
    summary.importantDetails = extractDetails(conversationText);
    summary.relevantFiles = extractFiles(conversationText);
    summary.rawMarkdown = formatMarkdown(summary);

    if (!previousSummary.objective.isEmpty()) {
        summary.completedWork = previousSummary.completedWork;
        summary.activeWork = previousSummary.activeWork;
    }

    return summary;
}

QString StructuredCompactionSummary::formatMarkdown(const StructuredSummary& summary) const {
    QString md;
    md += "## Objective\n" + summary.objective + "\n\n";
    md += "## Important Details\n" + summary.importantDetails + "\n\n";
    md += "## Work State\n";
    md += "### Completed\n" + summary.completedWork + "\n\n";
    md += "### Active\n" + summary.activeWork + "\n\n";
    md += "### Blocked\n" + summary.blockedWork + "\n\n";
    md += "## Next Move\n" + summary.nextMove + "\n\n";
    md += "## Relevant Files\n" + summary.relevantFiles + "\n";
    return md;
}

StructuredSummary StructuredCompactionSummary::parseSummary(const QString& markdown) const {
    StructuredSummary summary;
    summary.rawMarkdown = markdown;

    QRegularExpression objectiveRx("## Objective\\n(.+?)(?=\\n##|$)", QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch m = objectiveRx.match(markdown);
    if (m.hasMatch()) summary.objective = m.captured(1).trimmed();

    QRegularExpression detailsRx("## Important Details\\n(.+?)(?=\\n##|$)", QRegularExpression::DotMatchesEverythingOption);
    m = detailsRx.match(markdown);
    if (m.hasMatch()) summary.importantDetails = m.captured(1).trimmed();

    QRegularExpression filesRx("## Relevant Files\\n(.+?)(?=\\n##|$)", QRegularExpression::DotMatchesEverythingOption);
    m = filesRx.match(markdown);
    if (m.hasMatch()) summary.relevantFiles = m.captured(1).trimmed();

    return summary;
}

QString StructuredCompactionSummary::incrementalUpdate(const StructuredSummary& previous, const QString& newConversation) const {
    StructuredSummary updated = previous;
    updated.importantDetails += "\n" + extractDetails(newConversation);
    updated.relevantFiles += "\n" + extractFiles(newConversation);
    return formatMarkdown(updated);
}

int StructuredCompactionSummary::estimateTokens(const QString& text) const {
    return text.length() / 4;
}

int StructuredCompactionSummary::preserveRecentTokens() const {
    return 4000;
}

QString StructuredCompactionSummary::extractObjective(const QString& text) const {
    QStringList lines = text.split('\n');
    for (const auto& line : lines) {
        QString trimmed = line.trimmed();
        if (trimmed.startsWith("objective:", Qt::CaseInsensitive) ||
            trimmed.startsWith("goal:", Qt::CaseInsensitive)) {
            return trimmed.section(':', 1).trimmed();
        }
    }
    if (!lines.isEmpty()) return lines.first().trimmed();
    return "No objective identified";
}

QString StructuredCompactionSummary::extractDetails(const QString& text) const {
    QString details;
    QRegularExpression errorRx("(error|exception|failed|failure)", QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator it = errorRx.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        int start = qMax(0, match.capturedStart() - 50);
        int end = qMin(text.length(), match.capturedEnd() + 50);
        details += text.mid(start, end - start).trimmed() + "\n";
    }
    return details.trimmed().isEmpty() ? "No notable details" : details;
}

QString StructuredCompactionSummary::extractFiles(const QString& text) const {
    QRegularExpression fileRx("[\\/\\w\\.\\-]+\\.(cpp|h|hpp|c|py|js|ts|json|yaml|yml|md|txt|cmake|toml)");
    QSet<QString> files;
    QRegularExpressionMatchIterator it = fileRx.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        files.insert(match.captured(0));
    }
    return files.isEmpty() ? "No files referenced" : QStringList(files.values()).join("\n");
}

QString StructuredCompactionSummary::extractCommands(const QString& text) const {
    QRegularExpression cmdRx("(cmake|make|npm|git|cargo|rustup|pip|brew|sudo)\\s+[^\\n]+");
    QSet<QString> cmds;
    QRegularExpressionMatchIterator it = cmdRx.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        cmds.insert(match.captured(0).trimmed());
    }
    return cmds.isEmpty() ? "" : QStringList(cmds.values()).join("\n");
}

QString StructuredCompactionSummary::extractErrors(const QString& text) const {
    QRegularExpression errorRx("(error|fatal|panic|failed)[^\\n]*", QRegularExpression::CaseInsensitiveOption);
    QStringList errors;
    QRegularExpressionMatchIterator it = errorRx.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        errors.append(match.captured(0).trimmed());
    }
    return errors.isEmpty() ? "" : errors.join("\n");
}

QString StructuredCompactionSummary::extractSymbols(const QString& text) const {
    QRegularExpression symbolRx("(class|struct|function|def|fn|func)\\s+(\\w+)");
    QSet<QString> symbols;
    QRegularExpressionMatchIterator it = symbolRx.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        symbols.insert(match.captured(2));
    }
    return symbols.isEmpty() ? "" : QStringList(symbols.values()).join(", ");
}

} // namespace sentinel::core
