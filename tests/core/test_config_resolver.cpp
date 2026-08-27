#include "sentinel/core/config/ConfigResolver.h"
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

using sentinel::core::ConfigResolver;

class ConfigResolverTest final : public QObject {
    Q_OBJECT
private slots:
    void mergesLayersAndReportsSources();
};

void ConfigResolverTest::mergesLayersAndReportsSources() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QFile file(dir.filePath(QStringLiteral("sentinel.jsonc")));
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
    file.write("{\"provider\":{\"model\":\"local\"},\"enabled\":true}");
    file.close();

    const auto resolved = ConfigResolver::resolve(
        dir.path(), QJsonObject{{"provider", QJsonObject{{"endpoint", "local"}}}},
        QJsonObject{{"policy", QJsonObject{{"allow", true}}}});
    QCOMPARE(resolved.value.value("provider").toObject().value("endpoint").toString(),
             QStringLiteral("local"));
    QCOMPARE(resolved.value.value("provider").toObject().value("model").toString(),
             QStringLiteral("local"));
    QVERIFY(resolved.value.value("policy").toObject().value("allow").toBool());
    QVERIFY(resolved.errors.isEmpty());
    QVERIFY(!resolved.sources.isEmpty());
}

QTEST_MAIN(ConfigResolverTest)
#include "test_config_resolver.moc"
