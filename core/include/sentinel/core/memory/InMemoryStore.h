#pragma once

#include "sentinel/core/interfaces/IMemoryStore.h"

#include <QMap>

namespace sentinel::core {

class InMemoryStore final : public IMemoryStore {
public:
    void put(QString key, QString value) override;
    QString get(const QString& key) const override;
    MemoryEntries entries() const override;
    void clear() override;

private:
    QMap<QString, QString> entries_;
};

} // namespace sentinel::core
