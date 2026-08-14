#include "sentinel/core/codemode/CodeModeInterpreter.h"
#include <QtTest>

using namespace sentinel::core;

class CodeModeInterpreterTest final : public QObject {
    Q_OBJECT
private slots:
    void acceptsApprovedCalls();
    void rejectsUnsafeConstructs();
    void rejectsUnknownTools();
};

void CodeModeInterpreterTest::acceptsApprovedCalls() {
    CodeModeInterpreter interpreter;
    CodeModeScript script;
    script.script = QStringLiteral("read_file.get({path: 'a.txt'})");
    script.toolCatalog = interpreter.buildToolCatalog({QStringLiteral("read_file")});
    const auto result = interpreter.execute(script);
    QVERIFY(result.success);
    QCOMPARE(result.toolCalls.size(), 1);
}

void CodeModeInterpreterTest::rejectsUnsafeConstructs() {
    CodeModeInterpreter interpreter;
    QString error;
    QVERIFY(!interpreter.validateScript(QStringLiteral("system('rm -rf /')"), error));
    QVERIFY(!error.isEmpty());
}

void CodeModeInterpreterTest::rejectsUnknownTools() {
    CodeModeInterpreter interpreter;
    CodeModeScript script;
    script.script = QStringLiteral("unknown.run()");
    script.toolCatalog = interpreter.buildToolCatalog({QStringLiteral("read_file")});
    const auto result = interpreter.execute(script);
    QVERIFY(!result.success);
    QVERIFY(!result.error.isEmpty());
}

QTEST_MAIN(CodeModeInterpreterTest)
#include "test_codemode_interpreter.moc"
