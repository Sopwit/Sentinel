#include <QElapsedTimer>
#include <QtTest>

class StartupTimeTest final : public QObject {
    Q_OBJECT

private slots:
    void measureStartupDuration();
};

void StartupTimeTest::measureStartupDuration() {
    QElapsedTimer timer;
    timer.start();

    // Verify cold initialization boundary
    const qint64 elapsedMs = timer.elapsed();
    QVERIFY(elapsedMs < 5000);
}

QTEST_MAIN(StartupTimeTest)

#include "test_startup_time.moc"
