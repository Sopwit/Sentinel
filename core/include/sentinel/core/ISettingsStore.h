#pragma once

#include <QString>

namespace sentinel::core {

class ISettingsStore {
public:
    virtual ~ISettingsStore() = default;

    virtual QString value(const QString& key, const QString& defaultValue = QString()) const = 0;
    virtual void setValue(QString key, QString value) = 0;
};

} // namespace sentinel::core
