// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/editor/FuzzyEditor.h"
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <algorithm>

namespace sentinel::core {

FuzzyEditResult FuzzyEditor::edit(const FuzzyEditRequest& request) const {
    FuzzyEditResult result;

    QString content = readFile(request.filePath);
    if (content.isEmpty()) {
        result.error = QStringLiteral("Could not read file: %1").arg(request.filePath);
        return result;
    }

    if (content.contains(request.oldString)) {
        QString newContent = content;
        int count = 0;
        if (request.replaceAll) {
            count = newContent.count(request.oldString);
            newContent.replace(request.oldString, request.newString);
        } else {
            count = 1;
            newContent.replace(request.oldString, request.newString);
        }

        if (writeFile(request.filePath, newContent)) {
            result.success = true;
            result.linesChanged = count;
            result.usedStrategy = MatchStrategy::Exact;
            result.confidence = 100;
        } else {
            result.error = QStringLiteral("Could not write file: %1").arg(request.filePath);
        }
        return result;
    }

    FuzzyMatchResult match = findBestMatch(content, request.oldString);
    if (!match.found) {
        result.error = "Could not find matching text in file";
        return result;
    }

    if (match.confidence < 70) {
        result.error = QStringLiteral("Match confidence too low (%1%). Best match: \"%2\"")
                           .arg(match.confidence)
                           .arg(match.suggestion.left(100));
        return result;
    }

    QString newContent = content;
    QStringList lines = newContent.split('\n');
    for (int i = match.lineStart; i <= match.lineEnd && i < lines.size(); ++i) {
        if (i == match.lineStart) {
            lines[i] = request.newString;
        } else {
            lines.removeAt(i);
            i--;
        }
    }
    newContent = lines.join('\n');

    if (writeFile(request.filePath, newContent)) {
        result.success = true;
        result.linesChanged = match.lineEnd - match.lineStart + 1;
        result.usedStrategy = match.strategy;
        result.confidence = match.confidence;
    } else {
        result.error = QStringLiteral("Could not write file: %1").arg(request.filePath);
    }

    return result;
}

FuzzyMatchResult FuzzyEditor::findBestMatch(const QString& content, const QString& needle) const {
    FuzzyMatchResult best;

    best = exactMatch(content, needle);
    if (best.found && best.confidence >= 95)
        return best;

    FuzzyMatchResult trimmed = lineTrimmedMatch(content, needle);
    if (trimmed.found && trimmed.confidence > best.confidence)
        best = trimmed;

    FuzzyMatchResult whitespace = whitespaceNormalizedMatch(content, needle);
    if (whitespace.found && whitespace.confidence > best.confidence)
        best = whitespace;

    FuzzyMatchResult indent = indentationFlexibleMatch(content, needle);
    if (indent.found && indent.confidence > best.confidence)
        best = indent;

    FuzzyMatchResult escaped = escapeNormalizedMatch(content, needle);
    if (escaped.found && escaped.confidence > best.confidence)
        best = escaped;

    FuzzyMatchResult boundary = trimmedBoundaryMatch(content, needle);
    if (boundary.found && boundary.confidence > best.confidence)
        best = boundary;

    return best;
}

FuzzyMatchResult FuzzyEditor::exactMatch(const QString& content, const QString& needle) const {
    FuzzyMatchResult result;
    int idx = content.indexOf(needle);
    if (idx >= 0) {
        result.found = true;
        result.confidence = 100;
        result.strategy = MatchStrategy::Exact;
        result.matchedText = needle;

        QString before = content.left(idx);
        result.lineStart = before.count('\n');
        QString after = content.mid(idx + needle.length());
        result.lineEnd = result.lineStart + needle.count('\n');
    }
    return result;
}

FuzzyMatchResult FuzzyEditor::lineTrimmedMatch(const QString& content,
                                               const QString& needle) const {
    FuzzyMatchResult result;
    QStringList contentLines = content.split('\n');
    QStringList needleLines = needle.trimmed().split('\n');

    for (int i = 0; i <= contentLines.size() - needleLines.size(); ++i) {
        bool match = true;
        for (int j = 0; j < needleLines.size(); ++j) {
            if (contentLines[i + j].trimmed() != needleLines[j].trimmed()) {
                match = false;
                break;
            }
        }
        if (match) {
            result.found = true;
            result.lineStart = i;
            result.lineEnd = i + needleLines.size() - 1;
            result.confidence = 90;
            result.strategy = MatchStrategy::LineTrimmed;
            return result;
        }
    }
    return result;
}

FuzzyMatchResult FuzzyEditor::whitespaceNormalizedMatch(const QString& content,
                                                        const QString& needle) const {
    FuzzyMatchResult result;
    QString normalizedContent = normalizeWhitespace(content);
    QString normalizedNeedle = normalizeWhitespace(needle);

    int idx = normalizedContent.indexOf(normalizedNeedle);
    if (idx >= 0) {
        result.found = true;
        result.confidence = 85;
        result.strategy = MatchStrategy::WhitespaceNormalized;
    }
    return result;
}

FuzzyMatchResult FuzzyEditor::indentationFlexibleMatch(const QString& content,
                                                       const QString& needle) const {
    FuzzyMatchResult result;
    QStringList contentLines = content.split('\n');
    QStringList needleLines = needle.trimmed().split('\n');

    for (int i = 0; i <= contentLines.size() - needleLines.size(); ++i) {
        bool match = true;
        for (int j = 0; j < needleLines.size(); ++j) {
            QString contentTrimmed = contentLines[i + j].trimmed();
            QString needleTrimmed = needleLines[j].trimmed();
            if (contentTrimmed != needleTrimmed) {
                match = false;
                break;
            }
        }
        if (match) {
            result.found = true;
            result.lineStart = i;
            result.lineEnd = i + needleLines.size() - 1;
            result.confidence = 80;
            result.strategy = MatchStrategy::IndentationFlexible;
            return result;
        }
    }
    return result;
}

FuzzyMatchResult FuzzyEditor::escapeNormalizedMatch(const QString& content,
                                                    const QString& needle) const {
    FuzzyMatchResult result;
    QString normalizedContent = normalizeEscapes(content);
    QString normalizedNeedle = normalizeEscapes(needle);

    int idx = normalizedContent.indexOf(normalizedNeedle);
    if (idx >= 0) {
        result.found = true;
        result.confidence = 75;
        result.strategy = MatchStrategy::EscapeNormalized;
    }
    return result;
}

FuzzyMatchResult FuzzyEditor::trimmedBoundaryMatch(const QString& content,
                                                   const QString& needle) const {
    FuzzyMatchResult result;
    QStringList contentLines = content.split('\n');
    QStringList needleLines = needle.trimmed().split('\n');

    for (int i = 0; i <= contentLines.size() - needleLines.size(); ++i) {
        bool match = true;
        for (int j = 0; j < needleLines.size(); ++j) {
            QString cLine = contentLines[i + j].trimmed();
            QString nLine = needleLines[j].trimmed();
            if (cLine != nLine) {
                match = false;
                break;
            }
        }
        if (match) {
            result.found = true;
            result.lineStart = i;
            result.lineEnd = i + needleLines.size() - 1;
            result.confidence = 70;
            result.strategy = MatchStrategy::TrimmedBoundary;
            return result;
        }
    }
    return result;
}

FuzzyMatchResult FuzzyEditor::contextAwareMatch(const QString& content, const QString& needle,
                                                int contextLines) const {
    Q_UNUSED(contextLines)
    return exactMatch(content, needle);
}

FuzzyMatchResult FuzzyEditor::multiOccurrenceMatch(const QString& content,
                                                   const QString& needle) const {
    FuzzyMatchResult result;
    int count = 0;
    int idx = 0;
    while ((idx = content.indexOf(needle, idx)) >= 0) {
        count++;
        idx += needle.length();
    }
    if (count > 1) {
        result.found = true;
        result.confidence = 60;
        result.strategy = MatchStrategy::MultiOccurrence;
    }
    return result;
}

int FuzzyEditor::levenshteinDistance(const QString& s1, const QString& s2) const {
    qsizetype m = s1.length();
    qsizetype n = s2.length();

    if (m == 0) {
        return static_cast<int>(n);
    }
    if (n == 0) {
        return static_cast<int>(m);
    }

    // Use shorter string for n to minimize memory allocation
    const QString* str1 = &s1;
    const QString* str2 = &s2;
    if (m < n) {
        std::swap(m, n);
        std::swap(str1, str2);
    }

    QVector<int> dp(n + 1);
    for (qsizetype j = 0; j <= n; ++j) {
        dp[j] = static_cast<int>(j);
    }

    for (qsizetype i = 1; i <= m; ++i) {
        int prev = dp[0];
        dp[0] = static_cast<int>(i);
        for (qsizetype j = 1; j <= n; ++j) {
            const int temp = dp[j];
            const int cost = ((*str1)[i - 1] == (*str2)[j - 1]) ? 0 : 1;
            int minVal = dp[j] + 1;
            if (dp[j - 1] + 1 < minVal) {
                minVal = dp[j - 1] + 1;
            }
            if (prev + cost < minVal) {
                minVal = prev + cost;
            }
            dp[j] = minVal;
            prev = temp;
        }
    }
    return dp[n];
}

QString FuzzyEditor::normalizeWhitespace(const QString& text) const {
    QString result = text;
    result.replace(QRegularExpression("\\s+"), " ");
    return result.trimmed();
}

QString FuzzyEditor::normalizeEscapes(const QString& text) const {
    QString result = text;
    result.replace("\\n", "\n");
    result.replace("\\t", "\t");
    result.replace("\\\"", "\"");
    result.replace("\\'", "'");
    return result;
}

QString FuzzyEditor::readFile(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QTextStream(&file).readAll();
}

bool FuzzyEditor::writeFile(const QString& filePath, const QString& content) const {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream(&file) << content;
    return true;
}

QStringList FuzzyEditor::strategyNames() const {
    return {"Exact",
            "LineTrimmed",
            "WhitespaceNormalized",
            "IndentationFlexible",
            "EscapeNormalized",
            "TrimmedBoundary",
            "ContextAware",
            "MultiOccurrence"};
}

} // namespace sentinel::core
