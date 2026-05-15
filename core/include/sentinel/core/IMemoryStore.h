#pragma once

#include <QList>
#include <QString>
#include <utility>

namespace sentinel::core {

using MemoryEntry = std::pair<QString, QString>;
using MemoryEntries = QList<MemoryEntry>;

class IMemoryStore {
public:
    virtual ~IMemoryStore() = default;

    // Backends store exact key/value pairs. Application-level validation belongs
    // in controllers and services so future SQLite storage can stay generic.
    virtual void put(QString key, QString value) = 0;
    virtual QString get(const QString& key) const = 0;
    virtual MemoryEntries entries() const = 0;
    virtual void clear() = 0;
    virtual bool isAvailable() const {
        return true;
    }
    virtual QString lastError() const {
        return {};
    }
};

} // namespace sentinel::core
