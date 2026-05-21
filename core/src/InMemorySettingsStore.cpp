#include "sentinel/core/InMemorySettingsStore.h"

namespace sentinel::core {

QString InMemorySettingsStore::value(const QString& key, const QString& defaultValue) const {
    return values_.value(key, defaultValue);
}

void InMemorySettingsStore::setValue(QString key, QString value) {
    values_.insert(key, value);
}

} // namespace sentinel::core
