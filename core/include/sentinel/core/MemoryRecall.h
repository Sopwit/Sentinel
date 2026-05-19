#pragma once

#include "sentinel/core/IMemoryStore.h"

#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class MemoryRecallStatus : std::uint8_t {
    NotSearched,
    Disabled,
    StoreUnavailable,
    EmptyQuery,
    Completed,
};

QString memoryRecallStatusName(MemoryRecallStatus status);

struct MemoryRecallPolicy {
    bool enabled = true;
    int maxResults = 20;
    QString status = QStringLiteral("Local Literal Recall");
    QString summary = QStringLiteral(
        "Recall reads committed key-value memory only using literal key/value matching.");
};

struct MemoryRecallQuery {
    QString text;
    bool includeKeys = true;
    bool includeValues = true;
};

struct MemoryRecallResult {
    QString key;
    QString valuePreview;
    QString matchedField;
    QString summary;
};

struct MemoryRecallSummary {
    MemoryRecallStatus status = MemoryRecallStatus::NotSearched;
    MemoryRecallQuery query;
    QList<MemoryRecallResult> results;
    int resultCount = 0;
    int memoryEntryCount = 0;
    QString summary = QStringLiteral("No local memory recall query has been run.");
    QStringList checks;
};

MemoryRecallSummary memoryRecallSummaryForEntries(const MemoryRecallQuery& query,
                                                  const MemoryEntries& entries,
                                                  bool memoryStoreAvailable,
                                                  const MemoryRecallPolicy& policy);
QStringList memoryRecallResultSummaries(const MemoryRecallSummary& summary);

} // namespace sentinel::core
