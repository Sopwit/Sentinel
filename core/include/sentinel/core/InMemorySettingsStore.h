#pragma once

#include "sentinel/core/ISettingsStore.h"

#include <QMap>

namespace sentinel::core {

class InMemorySettingsStore final : public ISettingsStore {
public:
    QString value(const QString& key, const QString& defaultValue = QString()) const override;
    void setValue(QString key, QString value) override;

private:
    QMap<QString, QString> values_;
};

} // namespace sentinel::core
