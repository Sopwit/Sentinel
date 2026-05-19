#include "sentinel/core/MemoryRecall.h"

namespace sentinel::core {

QString memoryRecallStatusName(MemoryRecallStatus status) {
    switch (status) {
    case MemoryRecallStatus::NotSearched:
        return QStringLiteral("Not Searched");
    case MemoryRecallStatus::Disabled:
        return QStringLiteral("Disabled");
    case MemoryRecallStatus::StoreUnavailable:
        return QStringLiteral("Store Unavailable");
    case MemoryRecallStatus::EmptyQuery:
        return QStringLiteral("Empty Query");
    case MemoryRecallStatus::Completed:
        return QStringLiteral("Completed");
    }

    return QStringLiteral("Not Searched");
}

namespace {

QString previewValue(const QString& value) {
    const auto simplified = value.simplified();
    return simplified.isEmpty() ? QStringLiteral("Empty value") : simplified.left(160);
}

} // namespace

MemoryRecallSummary memoryRecallSummaryForEntries(const MemoryRecallQuery& query,
                                                  const MemoryEntries& entries,
                                                  bool memoryStoreAvailable,
                                                  const MemoryRecallPolicy& policy) {
    MemoryRecallSummary summary;
    summary.query.text = query.text.trimmed();
    summary.query.includeKeys = query.includeKeys;
    summary.query.includeValues = query.includeValues;
    summary.memoryEntryCount = entries.size();
    summary.checks.append(QStringLiteral("Source: local key-value memory store"));
    summary.checks.append(QStringLiteral("Matching: literal key/value contains"));
    summary.checks.append(QStringLiteral("Semantic recall: disabled"));
    summary.checks.append(QStringLiteral("Prompt injection: disabled"));

    if (!policy.enabled) {
        summary.status = MemoryRecallStatus::Disabled;
        summary.summary = policy.summary;
        summary.checks.append(QStringLiteral("Recall: disabled by policy"));
        return summary;
    }

    if (!memoryStoreAvailable) {
        summary.status = MemoryRecallStatus::StoreUnavailable;
        summary.summary = QStringLiteral("Local memory recall unavailable: memory store is not "
                                         "available.");
        summary.checks.append(QStringLiteral("Recall: target store unavailable"));
        return summary;
    }

    if (summary.query.text.isEmpty()) {
        summary.status = MemoryRecallStatus::EmptyQuery;
        summary.summary = QStringLiteral("No local memory recall query entered.");
        summary.checks.append(QStringLiteral("Recall: empty query"));
        return summary;
    }

    const auto caseSensitivity = Qt::CaseInsensitive;
    const int maxResults = policy.maxResults <= 0 ? entries.size() : policy.maxResults;
    for (const auto& entry : entries) {
        const bool keyMatch =
            query.includeKeys && entry.first.indexOf(summary.query.text, 0, caseSensitivity) >= 0;
        const bool valueMatch = query.includeValues &&
                                entry.second.indexOf(summary.query.text, 0, caseSensitivity) >= 0;
        if (!keyMatch && !valueMatch) {
            continue;
        }

        MemoryRecallResult result;
        result.key = entry.first;
        result.valuePreview = previewValue(entry.second);
        result.matchedField = keyMatch && valueMatch
                                  ? QStringLiteral("key and value")
                                  : (keyMatch ? QStringLiteral("key") : QStringLiteral("value"));
        result.summary = QStringLiteral("%1: %2 / matched %3")
                             .arg(result.key, result.valuePreview, result.matchedField);
        summary.results.append(result);
        if (summary.results.size() >= maxResults) {
            break;
        }
    }

    summary.status = MemoryRecallStatus::Completed;
    summary.resultCount = summary.results.size();
    summary.summary = summary.resultCount == 0
                          ? QStringLiteral("No committed local memory entries matched \"%1\".")
                                .arg(summary.query.text)
                          : QStringLiteral("Found %1 committed local memory %2 for \"%3\".")
                                .arg(summary.resultCount)
                                .arg(summary.resultCount == 1 ? QStringLiteral("entry")
                                                              : QStringLiteral("entries"))
                                .arg(summary.query.text);
    summary.checks.append(QStringLiteral("Recall: read-only completed"));
    return summary;
}

QStringList memoryRecallResultSummaries(const MemoryRecallSummary& summary) {
    QStringList results;
    results.reserve(summary.results.size());
    for (const auto& result : summary.results) {
        results.append(result.summary);
    }
    return results;
}

} // namespace sentinel::core
