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

namespace sentinel::core::interfaces {
    using IMemoryStore = ::sentinel::core::IMemoryStore;
    using MemoryEntry = ::sentinel::core::MemoryEntry;
    using MemoryEntries = ::sentinel::core::MemoryEntries;
}
