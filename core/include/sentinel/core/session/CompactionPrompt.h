// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

namespace sentinel::core {

class CompactionPrompt {
public:
    // Build compaction prompt from messages
    static QString buildPrompt(const QJsonArray& messages, const QString& previousSummary = {});

    // Parse compaction response
    static QJsonObject parseResponse(const QString& response);

    // Extract structured summary components
    static QString extractObjective(const QJsonObject& parsedResponse);
    static QString extractImportantDetails(const QJsonObject& parsedResponse);
    static QString extractWorkState(const QJsonObject& parsedResponse);
    static QString extractNextMove(const QJsonObject& parsedResponse);
    static QString extractRelevantFiles(const QJsonObject& parsedResponse);

    // Format summary for storage
    static QString formatSummary(const QJsonObject& parsedResponse);

    // Template constants
    static constexpr const char* PromptTemplate = R"(
You are a conversation summarizer. Summarize the following conversation into a structured format.

## Previous Summary
%1

## Conversation to Summarize
%2

## Required Output Format
Provide your summary in this exact structure:

### Objective
[What the user is trying to accomplish]

### Important Details
[Key facts, decisions, and context that must be preserved]

### Work State
- Completed: [What has been done]
- Active: [What is currently being worked on]
- Blocked: [Any blockers or issues]

### Next Move
[What should happen next based on the conversation]

### Relevant Files
[List any files mentioned or worked on]

Be concise but comprehensive. Preserve all critical information needed to continue the work.
)";

    static constexpr const char* SystemMessage = "You are a precise conversation summarizer. Output only the structured summary in the specified format.";
};

} // namespace sentinel::core
