#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>

namespace sentinel::core {

class RuntimeFeatureFlags final {
public:
    bool enabled(const QString& name, bool defaultValue = false) const;
    QString value(const QString& name, const QString& defaultValue = {}) const;
    bool experimental() const;
    void setOverride(const QString& name, bool enabled);
    void clearOverrides();

private:
    QHash<QString, bool> overrides_;
};

} // namespace sentinel::core
