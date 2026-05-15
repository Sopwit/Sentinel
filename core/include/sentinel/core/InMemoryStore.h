#pragma once

#include "sentinel/core/IMemoryStore.h"

#include <QMap>

namespace sentinel::core {

class InMemoryStore final : public IMemoryStore {
public:
    void put(QString key, QString value) override;
    QString get(const QString& key) const override;
    MemoryEntries entries() const override;

private:
    QMap<QString, QString> entries_;
};

} // namespace sentinel::core
