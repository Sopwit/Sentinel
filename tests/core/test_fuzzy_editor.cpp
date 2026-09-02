// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/editor/FuzzyEditor.h"

#include <QtTest>

using sentinel::core::FuzzyEditor;
using sentinel::core::FuzzyEditRequest;
using sentinel::core::FuzzyEditResult;
using sentinel::core::FuzzyMatchResult;
using sentinel::core::MatchStrategy;

class FuzzyEditorTest final : public QObject {
    Q_OBJECT

private slots:
    void testLevenshteinDistanceEmpty();
    void testLevenshteinDistanceIdentical();
    void testLevenshteinDistanceSingleOp();
    void testLevenshteinDistanceKnownWords();
    void testLevenshteinDistanceLongStrings();
    void testExactMatch();
    void testLineTrimmedMatch();
    void testWhitespaceNormalizedMatch();
    void testIndentationFlexibleMatch();
    void testEscapeNormalizedMatch();
    void testTrimmedBoundaryMatch();
};

void FuzzyEditorTest::testLevenshteinDistanceEmpty() {
    const FuzzyEditor editor;
    QCOMPARE(editor.levenshteinDistance("", ""), 0);
    QCOMPARE(editor.levenshteinDistance("abc", ""), 3);
    QCOMPARE(editor.levenshteinDistance("", "hello"), 5);
}

void FuzzyEditorTest::testLevenshteinDistanceIdentical() {
    const FuzzyEditor editor;
    QCOMPARE(editor.levenshteinDistance("hello", "hello"), 0);
    QCOMPARE(editor.levenshteinDistance("a", "a"), 0);
}

void FuzzyEditorTest::testLevenshteinDistanceSingleOp() {
    const FuzzyEditor editor;
    // Substitution
    QCOMPARE(editor.levenshteinDistance("cat", "bat"), 1);
    // Insertion
    QCOMPARE(editor.levenshteinDistance("cat", "cats"), 1);
    // Deletion
    QCOMPARE(editor.levenshteinDistance("cats", "cat"), 1);
}

void FuzzyEditorTest::testLevenshteinDistanceKnownWords() {
    const FuzzyEditor editor;
    QCOMPARE(editor.levenshteinDistance("kitten", "sitting"), 3);
    QCOMPARE(editor.levenshteinDistance("flaw", "lawn"), 2);
    QCOMPARE(editor.levenshteinDistance("saturday", "sunday"), 3);
}

void FuzzyEditorTest::testLevenshteinDistanceLongStrings() {
    const FuzzyEditor editor;
    QString s1 = QStringLiteral("the quick brown fox jumps over the lazy dog");
    QString s2 = QStringLiteral("that quick brown dog jumps over the lazy fox");
    int dist = editor.levenshteinDistance(s1, s2);
    QVERIFY(dist > 0);
    QCOMPARE(editor.levenshteinDistance(s1, s1), 0);
}

void FuzzyEditorTest::testExactMatch() {
    const FuzzyEditor editor;
    QString content = "line1\nline2\nline3\nline4";
    auto match = editor.findBestMatch(content, "line2\nline3");
    QVERIFY(match.found);
    QCOMPARE(match.strategy, MatchStrategy::Exact);
    QCOMPARE(match.confidence, 100);
    QCOMPARE(match.lineStart, 1);
    QCOMPARE(match.lineEnd, 2);
}

void FuzzyEditorTest::testLineTrimmedMatch() {
    const FuzzyEditor editor;
    QString content = "   line1   \n   line2   \n   line3   ";
    auto match = editor.findBestMatch(content, "line2");
    QVERIFY(match.found);
    QVERIFY(match.confidence >= 70);
}

void FuzzyEditorTest::testWhitespaceNormalizedMatch() {
    const FuzzyEditor editor;
    QString content = "foo   bar    baz";
    auto match = editor.findBestMatch(content, "foo bar baz");
    QVERIFY(match.found);
    QCOMPARE(match.strategy, MatchStrategy::WhitespaceNormalized);
}

void FuzzyEditorTest::testIndentationFlexibleMatch() {
    const FuzzyEditor editor;
    QString content = "\t\tvoid hello() {\n\t\t    return;\n\t\t}";
    QString needle = "    void hello() {\n        return;\n    }";
    auto match = editor.findBestMatch(content, needle);
    QVERIFY(match.found);
    QVERIFY(match.confidence >= 70);
}

void FuzzyEditorTest::testEscapeNormalizedMatch() {
    const FuzzyEditor editor;
    QString content = "line with \"quotes\" and newline\n";
    QString needle = "line with \\\"quotes\\\" and newline\\n";
    auto match = editor.findBestMatch(content, needle);
    QVERIFY(match.found);
    QCOMPARE(match.strategy, MatchStrategy::EscapeNormalized);
}

void FuzzyEditorTest::testTrimmedBoundaryMatch() {
    const FuzzyEditor editor;
    QString content = "header\n  first\n  second  \nfooter";
    QString needle = "first\nsecond";
    auto match = editor.findBestMatch(content, needle);
    QVERIFY(match.found);
    QVERIFY(match.confidence >= 70);
}

QTEST_MAIN(FuzzyEditorTest)
#include "test_fuzzy_editor.moc"
