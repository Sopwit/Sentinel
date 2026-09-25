// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/interfaces/IMemoryStore.h"

#include <QMap>

namespace sentinel::core {

class InMemoryStore final : public IMemoryStore {
public:
    void put(QString key, QString value) override;
    QString get(const QString& key) const override;
    MemoryEntries entries() const override;
    MemoryEntries searchRelevant(const QString& query, int limit) const override;
    QList<MemoryRecord> searchRelevantRecords(const QString& query, int limit) const override;
    void clear() override;

private:
    QMap<QString, QString> entries_;
    QMap<QString, qint64> ids_;
    qint64 nextId_ = 1;
};

} // namespace sentinel::core
