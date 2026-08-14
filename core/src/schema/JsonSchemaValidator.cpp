#include "sentinel/core/schema/JsonSchemaValidator.h"

#include <QJsonArray>

namespace sentinel::core {

QStringList JsonSchemaValidator::validate(const QJsonObject& value, const QJsonObject& schema) {
    QStringList errors;
    const QJsonArray required = schema.value(QStringLiteral("required")).toArray();
    for (const QJsonValue& requiredValue : required) {
        const QString key = requiredValue.toString();
        if (!key.isEmpty() && !value.contains(key)) {
            errors.append(QStringLiteral("Missing required property: %1").arg(key));
        }
    }

    const QJsonObject properties = schema.value(QStringLiteral("properties")).toObject();
    for (auto it = properties.constBegin(); it != properties.constEnd(); ++it) {
        if (!value.contains(it.key())) {
            continue;
        }
        const QString expected = it.value().toObject().value(QStringLiteral("type")).toString();
        const QJsonValue actual = value.value(it.key());
        bool matches = expected.isEmpty();
        if (expected == QStringLiteral("string")) matches = actual.isString();
        if (expected == QStringLiteral("number")) matches = actual.isDouble();
        if (expected == QStringLiteral("boolean")) matches = actual.isBool();
        if (expected == QStringLiteral("object")) matches = actual.isObject();
        if (expected == QStringLiteral("array")) matches = actual.isArray();
        if (!matches) {
            errors.append(QStringLiteral("Invalid type for property: %1").arg(it.key()));
        }
    }
    return errors;
}

bool JsonSchemaValidator::isValid(const QJsonObject& value, const QJsonObject& schema) {
    return validate(value, schema).isEmpty();
}

} // namespace sentinel::core
