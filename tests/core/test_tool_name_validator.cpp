#include "sentinel/core/runtime/ToolNameValidator.h"
#include <QtTest>

using sentinel::core::ToolNameValidator;

class ToolNameValidatorTest final : public QObject {
    Q_OBJECT
private slots:
    void validatesNames();
};

void ToolNameValidatorTest::validatesNames() {
    QVERIFY(ToolNameValidator::isValid(QStringLiteral("read_file")));
    QVERIFY(ToolNameValidator::isValid(QStringLiteral("tool-1")));
    QVERIFY(!ToolNameValidator::isValid(QStringLiteral("1tool")));
    QVERIFY(!ToolNameValidator::isValid(QStringLiteral("tool.name")));
    QVERIFY(!ToolNameValidator::isValid(QString(65, QLatin1Char('a'))));
}

QTEST_MAIN(ToolNameValidatorTest)
#include "test_tool_name_validator.moc"
