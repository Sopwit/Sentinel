#include "sentinel/core/app/LocationServiceMap.h"
#include <QtTest>

using sentinel::core::LocationServiceMap;

class LocationServiceMapTest final : public QObject {
    Q_OBJECT
private slots:
    void cachesPerLocation();
};

void LocationServiceMapTest::cachesPerLocation() {
    int creations = 0;
    LocationServiceMap<int> services([&creations](const QString&) {
        ++creations;
        return std::make_shared<int>(creations);
    });
    const auto first = services.get(QStringLiteral("."));
    const auto second = services.get(QStringLiteral("."));
    QVERIFY(first == second);
    QCOMPARE(creations, 1);
    QCOMPARE(services.size(), 1);
    services.invalidate(QStringLiteral("."));
    QCOMPARE(services.size(), 0);
}

QTEST_MAIN(LocationServiceMapTest)
#include "test_location_service_map.moc"
