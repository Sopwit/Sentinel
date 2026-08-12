// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QString>
#include <QtGlobal>
#include <utility>

namespace sentinel::core {

using MemoryEntry = std::pair<QString, QString>;
using MemoryEntries = QList<MemoryEntry>;

class IMemoryStore {
public:
    Q_DISABLE_COPY(IMemoryStore)
    IMemoryStore() = default;
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
