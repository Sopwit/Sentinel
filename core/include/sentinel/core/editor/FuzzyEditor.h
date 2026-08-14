// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>

namespace sentinel::core {

enum class MatchStrategy : std::uint8_t {
    Exact,
    LineTrimmed,
    BlockAnchor,
    WhitespaceNormalized,
    IndentationFlexible,
    EscapeNormalized,
    TrimmedBoundary,
    ContextAware,
    MultiOccurrence
};

struct FuzzyMatchResult {
    bool found{false};
    int lineStart{-1};
    int lineEnd{-1};
    int confidence{0};
    MatchStrategy strategy{MatchStrategy::Exact};
    QString matchedText;
    QString suggestion;
};

struct FuzzyEditRequest {
    QString filePath;
    QString oldString;
    QString newString;
    bool replaceAll{false};
    int contextLines{3};
};

struct FuzzyEditResult {
    bool success{false};
    QString error;
    int linesChanged{0};
    MatchStrategy usedStrategy{MatchStrategy::Exact};
    int confidence{0};
    QStringList warnings;
};

class FuzzyEditor {
public:
    FuzzyEditResult edit(const FuzzyEditRequest& request) const;
    FuzzyMatchResult findBestMatch(const QString& content, const QString& needle) const;
    QString readFile(const QString& filePath) const;
    bool writeFile(const QString& filePath, const QString& content) const;

private:
    QStringList strategyNames() const;
    FuzzyMatchResult exactMatch(const QString& content, const QString& needle) const;
    FuzzyMatchResult lineTrimmedMatch(const QString& content, const QString& needle) const;
    FuzzyMatchResult whitespaceNormalizedMatch(const QString& content, const QString& needle) const;
    FuzzyMatchResult indentationFlexibleMatch(const QString& content, const QString& needle) const;
    FuzzyMatchResult escapeNormalizedMatch(const QString& content, const QString& needle) const;
    FuzzyMatchResult trimmedBoundaryMatch(const QString& content, const QString& needle) const;
    FuzzyMatchResult contextAwareMatch(const QString& content, const QString& needle, int contextLines) const;
    FuzzyMatchResult multiOccurrenceMatch(const QString& content, const QString& needle) const;
    int levenshteinDistance(const QString& s1, const QString& s2) const;
    QString normalizeWhitespace(const QString& text) const;
    QString normalizeEscapes(const QString& text) const;
};

} // namespace sentinel::core
