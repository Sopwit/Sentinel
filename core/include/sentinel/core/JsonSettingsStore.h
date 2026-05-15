#pragma once

#include "sentinel/core/ISettingsStore.h"

#include <QJsonObject>
#include <QString>

namespace sentinel::core {

class JsonSettingsStore final : public ISettingsStore {
public:
    explicit JsonSettingsStore(QString filePath);

    QString value(const QString& key, const QString& defaultValue = QString()) const override;
    void setValue(QString key, QString value) override;

    QString filePath() const;

private:
    void load();
    void save() const;

    QString filePath_;
    QJsonObject values_;
};

} // namespace sentinel::core
