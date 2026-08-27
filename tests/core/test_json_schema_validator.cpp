#include "sentinel/core/schema/JsonSchemaValidator.h"
#include <QtTest>

using sentinel::core::JsonSchemaValidator;

class JsonSchemaValidatorTest final : public QObject {
    Q_OBJECT
private slots:
    void validatesRequiredAndTypes();
};

void JsonSchemaValidatorTest::validatesRequiredAndTypes() {
    const QJsonObject schema{
        {"required", QJsonArray{"name"}},
        {"properties", QJsonObject{{"name", QJsonObject{{"type", "string"}}}}}};
    QVERIFY(JsonSchemaValidator::isValid(QJsonObject{{"name", "Sentinel"}}, schema));
    QVERIFY(!JsonSchemaValidator::isValid(QJsonObject{{"name", 42}}, schema));
    QVERIFY(!JsonSchemaValidator::isValid(QJsonObject{}, schema));
}

QTEST_MAIN(JsonSchemaValidatorTest)
#include "test_json_schema_validator.moc"
