// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/runtime/ToolArgumentValidator.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <cmath>

namespace sentinel::core {
namespace {
QString jsonText(const QJsonValue& value) {
    if (value.isString())
        return value.toString();
    return QString::fromUtf8(QJsonDocument(QJsonArray{value}).toJson(QJsonDocument::Compact))
        .mid(1)
        .chopped(1);
}
void error(QList<ToolValidationError>& errors, const QString& path, const QString& keyword,
           const QString& message) {
    errors.append({path, keyword, message});
}
bool isInteger(const QJsonValue& value) {
    return value.isDouble() && std::isfinite(value.toDouble()) &&
           std::floor(value.toDouble()) == value.toDouble();
}
bool validType(const QString& type) {
    return type == QLatin1String("object") || type == QLatin1String("array") ||
           type == QLatin1String("string") || type == QLatin1String("integer") ||
           type == QLatin1String("number") || type == QLatin1String("boolean") ||
           type == QLatin1String("null");
}
bool matchesType(const QJsonValue& value, const QString& type) {
    if (type == QLatin1String("object"))
        return value.isObject();
    if (type == QLatin1String("array"))
        return value.isArray();
    if (type == QLatin1String("string"))
        return value.isString();
    if (type == QLatin1String("integer"))
        return isInteger(value);
    if (type == QLatin1String("number"))
        return value.isDouble();
    if (type == QLatin1String("boolean"))
        return value.isBool();
    return value.isNull();
}
QStringList types(const QJsonObject& schema) {
    const auto type = schema.value(QStringLiteral("type"));
    if (type.isString())
        return {type.toString()};
    QStringList result;
    for (const auto& item : type.toArray())
        result.append(item.toString());
    return result;
}
QJsonValue validateValue(const QJsonObject& schema, const QJsonValue& value,
                         const QString& path, QList<ToolValidationError>& errors, int depth);
void checkSchema(const QJsonObject& schema, const QString& path, QList<ToolValidationError>& errors,
                 int depth) {
    if (depth > 16) {
        error(errors, path, QStringLiteral("depth"),
              QStringLiteral("Schema nesting exceeds 16 levels."));
        return;
    }
    for (const auto& keyword :
         {"$ref", "oneOf", "anyOf", "allOf", "not", "if", "then", "else", "patternProperties"})
        if (schema.contains(QLatin1String(keyword)))
            error(errors, path, QString::fromLatin1(keyword),
                  QStringLiteral("Unsupported structural schema keyword."));
    const auto type = schema.value(QStringLiteral("type"));
    if (type.isArray() && type.toArray().isEmpty())
        error(errors, path, QStringLiteral("type"), QStringLiteral("Type list must not be empty."));
    if (!type.isUndefined()) {
        if (!type.isString() && !type.isArray())
            error(errors, path, QStringLiteral("type"),
                  QStringLiteral("Type must be a string or array of strings."));
        for (const auto& name : types(schema))
            if (!validType(name))
                error(errors, path, QStringLiteral("type"), QStringLiteral("Unknown type."));
    }
    const auto properties = schema.value(QStringLiteral("properties"));
    if (!properties.isUndefined() && !properties.isObject())
        error(errors, path, QStringLiteral("properties"),
              QStringLiteral("Properties must be an object."));
    const auto propertyObject = properties.toObject();
    for (auto it = propertyObject.begin(); it != propertyObject.end(); ++it) {
        if (!it.value().isObject())
            error(errors, path + QLatin1Char('.') + it.key(), QStringLiteral("properties"),
                  QStringLiteral("Property schema must be an object."));
        else
            checkSchema(it.value().toObject(), path + QLatin1Char('.') + it.key(), errors,
                        depth + 1);
    }
    const auto required = schema.value(QStringLiteral("required"));
    if (!required.isUndefined()) {
        if (!required.isArray())
            error(errors, path, QStringLiteral("required"),
                  QStringLiteral("Required must be an array."));
        for (const auto& item : required.toArray())
            if (!item.isString())
                error(errors, path, QStringLiteral("required"),
                      QStringLiteral("Required entries must be strings."));
    }
    const auto additional = schema.value(QStringLiteral("additionalProperties"));
    if (!additional.isUndefined() && !additional.isBool() && !additional.isObject())
        error(errors, path, QStringLiteral("additionalProperties"),
              QStringLiteral("Expected boolean or schema object."));
    if (additional.isObject())
        checkSchema(additional.toObject(), path + QStringLiteral(".*"), errors, depth + 1);
    const auto items = schema.value(QStringLiteral("items"));
    if (!items.isUndefined() && !items.isObject())
        error(errors, path, QStringLiteral("items"),
              QStringLiteral("Items must be a schema object."));
    if (items.isObject())
        checkSchema(items.toObject(), path + QStringLiteral("[]"), errors, depth + 1);
    for (const auto& keyword : {"minItems", "maxItems", "minLength", "maxLength"}) {
        const auto value = schema.value(QLatin1String(keyword));
        if (!value.isUndefined() && (!isInteger(value) || value.toDouble() < 0))
            error(errors, path, QString::fromLatin1(keyword),
                  QStringLiteral("Expected nonnegative integer."));
    }
    for (const auto& keyword : {"minimum", "maximum"}) {
        const auto value = schema.value(QLatin1String(keyword));
        if (!value.isUndefined() && !value.isDouble())
            error(errors, path, QString::fromLatin1(keyword), QStringLiteral("Expected number."));
    }
    const auto pattern = schema.value(QStringLiteral("pattern"));
    if (!pattern.isUndefined() &&
        (!pattern.isString() || !QRegularExpression(pattern.toString()).isValid()))
        error(errors, path, QStringLiteral("pattern"),
              QStringLiteral("Invalid regular expression."));
    const auto defaultValue = schema.value(QStringLiteral("default"));
    if (!defaultValue.isUndefined()) {
        const auto allowedTypes = types(schema);
        if (!allowedTypes.isEmpty()) {
            bool accepted = false;
            for (const auto& name : allowedTypes)
                accepted |= matchesType(defaultValue, name);
            if (!accepted)
                error(errors, path, QStringLiteral("default"),
                      QStringLiteral("Default has the wrong JSON type."));
        }
    }
    if (!defaultValue.isUndefined()) {
        QList<ToolValidationError> defaultErrors;
        validateValue(schema, defaultValue, path, defaultErrors, depth + 1);
        for (const auto& issue : defaultErrors)
            error(errors, issue.path, QStringLiteral("default"),
                  QStringLiteral("Default violates %1: %2").arg(issue.keyword, issue.message));
    }
    const auto enumeration = schema.value(QStringLiteral("enum"));
    if (!enumeration.isUndefined() && (!enumeration.isArray() || enumeration.toArray().isEmpty()))
        error(errors, path, QStringLiteral("enum"),
              QStringLiteral("Enum must be a nonempty array."));
}
QJsonValue validateValue(const QJsonObject& schema, const QJsonValue& value, const QString& path,
                         QList<ToolValidationError>& errors, int depth) {
    if (depth > 16) {
        error(errors, path, QStringLiteral("depth"),
              QStringLiteral("Argument nesting exceeds 16 levels."));
        return value;
    }
    const auto allowed = types(schema);
    if (!allowed.isEmpty()) {
        bool matches = false;
        for (const auto& type : allowed)
            matches |= matchesType(value, type);
        if (!matches) {
            error(errors, path, QStringLiteral("type"),
                  QStringLiteral("Value has the wrong JSON type; expected %1.")
                      .arg(allowed.join(QStringLiteral(" or "))));
            return value;
        }
    }
    if (schema.contains(QStringLiteral("const")) && value != schema.value(QStringLiteral("const")))
        error(errors, path, QStringLiteral("const"),
              QStringLiteral("Value must equal the declared constant."));
    if (schema.contains(QStringLiteral("enum")) &&
        !schema.value(QStringLiteral("enum")).toArray().contains(value))
        error(errors, path, QStringLiteral("enum"),
              QStringLiteral("Value is not one of the allowed values."));
    if (value.isString()) {
        const auto text = value.toString();
        const int codePoints = text.toUcs4().size();
        if (schema.contains(QStringLiteral("minLength")) &&
            codePoints < schema.value(QStringLiteral("minLength")).toInt())
            error(errors, path, QStringLiteral("minLength"),
                  QStringLiteral("String is too short."));
        if (schema.contains(QStringLiteral("maxLength")) &&
            codePoints > schema.value(QStringLiteral("maxLength")).toInt())
            error(errors, path, QStringLiteral("maxLength"), QStringLiteral("String is too long."));
        if (schema.contains(QStringLiteral("pattern")) &&
            !QRegularExpression(schema.value(QStringLiteral("pattern")).toString())
                 .match(text)
                 .hasMatch())
            error(errors, path, QStringLiteral("pattern"),
                  QStringLiteral("String does not match the required pattern."));
    }
    if (value.isDouble()) {
        if (schema.contains(QStringLiteral("minimum")) &&
            value.toDouble() < schema.value(QStringLiteral("minimum")).toDouble())
            error(errors, path, QStringLiteral("minimum"),
                  QStringLiteral("Number is below the minimum."));
        if (schema.contains(QStringLiteral("maximum")) &&
            value.toDouble() > schema.value(QStringLiteral("maximum")).toDouble())
            error(errors, path, QStringLiteral("maximum"),
                  QStringLiteral("Number exceeds the maximum."));
    }
    if (value.isArray()) {
        const auto array = value.toArray();
        if (schema.contains(QStringLiteral("minItems")) &&
            array.size() < schema.value(QStringLiteral("minItems")).toInt())
            error(errors, path, QStringLiteral("minItems"), QStringLiteral("Array is too short."));
        if (schema.contains(QStringLiteral("maxItems")) &&
            array.size() > schema.value(QStringLiteral("maxItems")).toInt())
            error(errors, path, QStringLiteral("maxItems"), QStringLiteral("Array is too long."));
        if (schema.value(QStringLiteral("items")).isObject()) {
            QJsonArray normalized;
            for (int i = 0; i < array.size(); ++i)
                normalized.append(validateValue(schema.value(QStringLiteral("items")).toObject(),
                                                array.at(i), path + QStringLiteral("[%1]").arg(i),
                                                errors, depth + 1));
            return normalized;
        }
    }
    if (value.isObject()) {
        auto object = value.toObject();
        const auto properties = schema.value(QStringLiteral("properties")).toObject();
        for (auto it = properties.begin(); it != properties.end(); ++it) {
            if (!object.contains(it.key()) &&
                it.value().toObject().contains(QStringLiteral("default")))
                object.insert(it.key(), it.value().toObject().value(QStringLiteral("default")));
        }
        for (const auto& required : schema.value(QStringLiteral("required")).toArray())
            if (!object.contains(required.toString()))
                error(errors, path + QLatin1Char('.') + required.toString(),
                      QStringLiteral("required"), QStringLiteral("Required property is missing."));
        QJsonObject normalized;
        for (auto it = object.begin(); it != object.end(); ++it) {
            const QString childPath = path + QLatin1Char('.') + it.key();
            if (properties.contains(it.key()))
                normalized.insert(it.key(),
                                  validateValue(properties.value(it.key()).toObject(), it.value(),
                                                childPath, errors, depth + 1));
            else if (schema.value(QStringLiteral("additionalProperties")).isBool() &&
                     !schema.value(QStringLiteral("additionalProperties")).toBool())
                error(errors, childPath, QStringLiteral("additionalProperties"),
                      QStringLiteral("Unknown property."));
            else if (schema.value(QStringLiteral("additionalProperties")).isObject())
                normalized.insert(
                    it.key(),
                    validateValue(schema.value(QStringLiteral("additionalProperties")).toObject(),
                                  it.value(), childPath, errors, depth + 1));
            else
                normalized.insert(it.key(), it.value());
        }
        return normalized;
    }
    return value;
}
} // namespace

QList<ToolValidationError> ToolArgumentValidator::validateSchema(const QJsonObject& schema) {
    QList<ToolValidationError> errors;
    if (!schema.isEmpty()) {
        if (!types(schema).contains(QStringLiteral("object")))
            error(errors, QStringLiteral("$"), QStringLiteral("type"),
                  QStringLiteral("Tool input schema must declare an object root."));
        checkSchema(schema, QStringLiteral("$"), errors, 0);
    }
    return errors;
}
ToolValidationResult
ToolArgumentValidator::validate(const ToolDescriptor& descriptor,
                                const QList<ToolInvocationArgument>& arguments) {
    ToolValidationResult result;
    const auto schemaErrors = validateSchema(descriptor.inputSchema);
    if (!schemaErrors.isEmpty()) {
        result.errors = schemaErrors;
        return result;
    }
    QJsonObject raw;
    for (const auto& argument : arguments) {
        if (raw.contains(argument.id)) {
            error(result.errors, QStringLiteral("$.") + argument.id, QStringLiteral("duplicate"),
                  QStringLiteral("Duplicate argument."));
            continue;
        }
        raw.insert(argument.id, argument.jsonValue.isUndefined() ? QJsonValue(argument.value)
                                                                 : argument.jsonValue);
    }
    // Missing MCP schemas are restricted to no-argument calls. A remote server
    // cannot define an enforceable argument contract by omitting inputSchema.
    const QJsonObject schema = descriptor.inputSchema.isEmpty()
                                   ? QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
                                                 {QStringLiteral("additionalProperties"),
                                                  descriptor.source != ToolSource::MCP}}
                                   : descriptor.inputSchema;
    result.normalizedArguments =
        validateValue(schema, raw, QStringLiteral("$"), result.errors, 0).toObject();
    result.valid = result.errors.isEmpty();
    return result;
}
QList<ToolInvocationArgument>
ToolArgumentValidator::toInvocationArguments(const QJsonObject& normalized) {
    QList<ToolInvocationArgument> result;
    for (auto it = normalized.begin(); it != normalized.end(); ++it)
        result.append({it.key(), jsonText(it.value()), it.value()});
    return result;
}
QString ToolArgumentValidator::compactContract(const ToolDescriptor& descriptor) {
    const auto schema = descriptor.inputSchema;
    if (schema.isEmpty())
        return descriptor.source == ToolSource::MCP
                   ? QStringLiteral("args: {} (remote schema missing)")
                   : QStringLiteral("args: object (contract unavailable)");
    const auto properties = schema.value(QStringLiteral("properties")).toObject();
    const auto required = schema.value(QStringLiteral("required")).toArray();
    QStringList fields;
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        const auto field = it.value().toObject();
        QString part = it.key() + QStringLiteral(": ") + types(field).join(QLatin1Char('|'));
        if (required.contains(it.key()))
            part += QStringLiteral(" required");
        if (field.contains(QStringLiteral("default")))
            part += QLatin1Char('=') + jsonText(field.value(QStringLiteral("default")));
        fields.append(part);
    }
    return fields.isEmpty() ? QStringLiteral("args: {}")
                            : QStringLiteral("args: {%1}").arg(fields.join(QStringLiteral(", ")));
}
} // namespace sentinel::core
