#include "sentinel/core/config/RuntimeFeatureFlags.h"
#include <QtTest>

using sentinel::core::RuntimeFeatureFlags;

class RuntimeFeatureFlagsTest final : public QObject {
    Q_OBJECT
private slots:
    void defaultsAndOverrides();
};

void RuntimeFeatureFlagsTest::defaultsAndOverrides() {
    RuntimeFeatureFlags flags;
    QVERIFY(!flags.enabled(QStringLiteral("SENTINEL_TEST_UNKNOWN")));
    QVERIFY(flags.enabled(QStringLiteral("SENTINEL_TEST_UNKNOWN"), true));
    flags.setOverride(QStringLiteral("feature"), true);
    QVERIFY(flags.enabled(QStringLiteral("feature")));
    flags.clearOverrides();
    QVERIFY(!flags.enabled(QStringLiteral("feature")));
}

QTEST_MAIN(RuntimeFeatureFlagsTest)
#include "test_runtime_feature_flags.moc"
