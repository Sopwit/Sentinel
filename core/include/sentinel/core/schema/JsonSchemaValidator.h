#pragma once

#include <QJsonObject>
#include <QStringList>

namespace sentinel::core {

class JsonSchemaValidator final {
public:
    static QStringList validate(const QJsonObject& value, const QJsonObject& schema);
    static bool isValid(const QJsonObject& value, const QJsonObject& schema);
};

} // namespace sentinel::core
