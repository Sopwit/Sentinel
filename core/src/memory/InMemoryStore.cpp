// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/memory/InMemoryStore.h"

namespace sentinel::core {

void InMemoryStore::put(QString key, QString value) {
    if (!ids_.contains(key))
        ids_.insert(key, nextId_++);
    entries_.insert(key, value);
}

QString InMemoryStore::get(const QString& key) const {
    return entries_.value(key);
}

MemoryEntries InMemoryStore::entries() const {
    MemoryEntries result;
    result.reserve(entries_.size());

    for (auto it = entries_.cbegin(); it != entries_.cend(); ++it) {
        result.append({it.key(), it.value()});
    }

    return result;
}

MemoryEntries InMemoryStore::searchRelevant(const QString& query, int limit) const {
    MemoryEntries result;
    for (const auto& record : searchRelevantRecords(query, limit))
        result.append({record.key, record.value});
    return result;
}

QList<MemoryRecord> InMemoryStore::searchRelevantRecords(const QString& query, int limit) const {
    QList<MemoryRecord> result;
    if (query.trimmed().isEmpty() || limit <= 0)
        return result;
    for (auto it = entries_.cbegin(); it != entries_.cend() && result.size() < limit; ++it)
        if (it.key().contains(query, Qt::CaseInsensitive) ||
            it.value().contains(query, Qt::CaseInsensitive))
            result.append({ids_.value(it.key()), it.key(), it.value()});
    return result;
}

void InMemoryStore::clear() {
    entries_.clear();
    ids_.clear();
}

} // namespace sentinel::core
