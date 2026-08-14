#include "sentinel/core/security/PathGuard.h"
#include "sentinel/core/security/PolicyEvaluator.h"
#include "sentinel/core/util/Base64Url.h"

#include <QtTest>

using namespace sentinel::core;

class SecurityUtilitiesTest final : public QObject {
    Q_OBJECT
private slots:
    void policyIsLastMatchWins();
    void pathGuardBlocksEscape();
    void base64UrlRoundTrips();
};

void SecurityUtilitiesTest::policyIsLastMatchWins() {
    const QList<PermissionRule> rules{{"shell", "*", PermissionEffect::Ask},
                                      {"shell", "safe *", PermissionEffect::Allow},
                                      {"shell", "safe rm", PermissionEffect::Deny}};
    QCOMPARE(PolicyEvaluator::evaluate(rules, QStringLiteral("shell"), QStringLiteral("safe rm")),
             PermissionEffect::Deny);
    QCOMPARE(PolicyEvaluator::evaluate(rules, QStringLiteral("shell"), QStringLiteral("safe ls")),
             PermissionEffect::Allow);
}

void SecurityUtilitiesTest::pathGuardBlocksEscape() {
    QVERIFY(PathGuard::contains(QStringLiteral("/tmp/project"), QStringLiteral("/tmp/project/src")));
    QVERIFY(!PathGuard::contains(QStringLiteral("/tmp/project"), QStringLiteral("/tmp/project-other")));
}

void SecurityUtilitiesTest::base64UrlRoundTrips() {
    const QByteArray value("Sentinel? safe");
    const QByteArray encoded = Base64Url::encode(value);
    QVERIFY(!encoded.contains('+'));
    QVERIFY(!encoded.contains('/'));
    QCOMPARE(Base64Url::decode(encoded), value);
}

QTEST_MAIN(SecurityUtilitiesTest)
#include "test_security_utilities.moc"
