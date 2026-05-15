#include "sentinel/core/InMemoryStore.h"

namespace sentinel::core {

void InMemoryStore::put(QString key, QString value) {
    entries_.insert(std::move(key), std::move(value));
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

} // namespace sentinel::core
