// SPDX-License-Identifier: GPL-3.0-or-later
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <sentinel/core/memory/JsonSettingsStore.h>
#include <sentinel/desktop/CoreWardenItem.h>
using namespace sentinel::desktop;
class TestCoreWarden : public QObject {
    Q_OBJECT
    const QString root = QStringLiteral(WARDEN_TEST_ROOT);
private Q_SLOTS:
    void allStates() {
        for (const auto& state :
             QStringList{"idle", "listening", "thinking", "processing", "tool_running", "approval",
                         "speaking", "success", "warning", "error"}) {
            const auto frames = CoreWardenItem::load(root, state, true);
            QVERIFY2(frames.error.isEmpty(), qPrintable(state));
            QVERIFY(frames.images.size() > 1);
            QCOMPARE(frames.fps, 8);
            QCOMPARE(frames.loop, state != "success" && state != "error");
            for (const auto& image : frames.images) {
                QCOMPARE(image.size(), QSize(192, 192));
                QVERIFY(image.hasAlphaChannel());
            }
            QCOMPARE(CoreWardenItem::load(root, state, false).images.size(), 1);
        }
    }
    void readinessCopy() {
        QVERIFY(CoreWardenItem::readinessSummary(false, "ready").contains("Connect the daemon"));
        QVERIFY(CoreWardenItem::readinessSummary(true, "unknown").contains("not confirmed"));
        QVERIFY(CoreWardenItem::readinessSummary(true, "Ready").contains("verify inference"));
        QVERIFY(CoreWardenItem::readinessSummary(true, "busy").contains("request is running"));
    }
    void honestMapping() {
        QCOMPARE(CoreWardenItem::productionState(false, true, "streaming"), "idle");
        QCOMPARE(CoreWardenItem::productionState(true, false, "idle"), "idle");
        QCOMPARE(CoreWardenItem::productionState(true, true, "running"), "approval");
        QCOMPARE(CoreWardenItem::productionState(true, false, "sending"), "thinking");
        QCOMPARE(CoreWardenItem::productionState(true, false, "streaming"), "thinking");
        QCOMPARE(CoreWardenItem::productionState(true, false, "failed"), "error");
        for (const auto& state :
             QStringList{"queued", "tool_completed", "completed", "running", "unknown"})
            QCOMPARE(CoreWardenItem::productionState(true, false, state), "idle");
    }
    void fallbackAndTraversal() {
        QVERIFY(!CoreWardenItem::load("/nonexistent", "idle", true).error.isEmpty());
        QVERIFY(!CoreWardenItem::load(root, "unknown", true).error.isEmpty());
        QTemporaryDir temp;
        QDir().mkpath(temp.path() + "/animations");
        QVERIFY(QFile::copy(root + "/state-contract.json", temp.path() + "/state-contract.json"));
        QFile source(root + "/animations/manifest.json");
        QVERIFY(source.open(QIODevice::ReadOnly));
        auto manifest = QJsonDocument::fromJson(source.readAll()).object();
        auto states = manifest["states"].toObject();
        auto idle = states["idle"].toObject();
        auto frames = idle["frames"].toObject();
        auto paths = frames["192"].toArray();
        paths[0] = "../../outside.png";
        frames["192"] = paths;
        idle["frames"] = frames;
        states["idle"] = idle;
        manifest["states"] = states;
        QFile target(temp.path() + "/animations/manifest.json");
        QVERIFY(target.open(QIODevice::WriteOnly));
        target.write(QJsonDocument(manifest).toJson());
        target.close();
        QVERIFY(!CoreWardenItem::load(temp.path(), "idle", true).error.isEmpty());
    }
    void corruptedFramesAndManifest() {
        QTemporaryDir temp;
        QDir().mkpath(temp.path() + "/animations/idle/192");
        QVERIFY(QFile::copy(root + "/state-contract.json", temp.path() + "/state-contract.json"));
        QVERIFY(QFile::copy(root + "/animations/manifest.json",
                            temp.path() + "/animations/manifest.json"));
        const auto frame = temp.path() + "/animations/idle/192/frame_000.png";
        QVERIFY(QFile::copy(root + "/animations/idle/192/frame_000.png", frame));
        QCOMPARE(CoreWardenItem::load(temp.path(), "idle", false).images.size(), 1);
        QFile badFrame(frame);
        QVERIFY(badFrame.open(QIODevice::WriteOnly | QIODevice::Truncate));
        badFrame.write("not a PNG");
        badFrame.close();
        const auto corrupt = CoreWardenItem::load(temp.path(), "idle", false);
        QVERIFY(!corrupt.error.isEmpty());
        QVERIFY(corrupt.images.isEmpty());
        QFile badManifest(temp.path() + "/animations/manifest.json");
        QVERIFY(badManifest.open(QIODevice::WriteOnly | QIODevice::Truncate));
        badManifest.write("{invalid");
        badManifest.close();
        QVERIFY(!CoreWardenItem::load(temp.path(), "idle", false).error.isEmpty());
        QVERIFY(QFile::remove(temp.path() + "/animations/manifest.json"));
        QVERIFY(QFile::copy(root + "/animations/manifest.json",
                            temp.path() + "/animations/manifest.json"));
        QVERIFY(QFile::remove(frame));
        QVERIFY(QFile::link(root + "/animations/idle/192/frame_000.png", frame));
        QVERIFY(!CoreWardenItem::load(temp.path(), "idle", false).error.isEmpty());
    }
    void lifecycle() {
        QQuickWindow window;
        CoreWardenItem item(window.contentItem());
        window.show();
        item.setAssetRoot(root);
        item.setAnimationState("thinking");
        item.setAnimate(true);
        QTRY_VERIFY_WITH_TIMEOUT(item.playing(), 3000);
        const int first = item.frameIndex();
        QTRY_VERIFY(item.frameIndex() != first);
        item.setReducedMotion(true);
        QTRY_COMPARE(item.cachedFrames(), 1);
        QVERIFY(!item.playing());
        item.setReducedMotion(false);
        item.setMode("static");
        QTRY_COMPARE(item.cachedFrames(), 1);
        QVERIFY(!item.playing());
        item.setMode("animated");
        QTRY_VERIFY(item.playing());
        item.setVisible(false);
        QTRY_COMPARE(item.cachedFrames(), 0);
        QVERIFY(!item.playing());
        item.setVisible(true);
        QTRY_VERIFY(item.playing());
        item.setMode("off");
        QTRY_COMPARE(item.cachedFrames(), 0);
        QVERIFY(!item.playing());
        item.setMode("animated");
        item.setAnimationState("success");
        QTRY_COMPARE(item.cachedFrames(), 13);
        QTRY_COMPARE_WITH_TIMEOUT(item.frameIndex(), 12, 3000);
        QTRY_VERIFY(!item.playing());
        item.setAnimationState("approval");
        QTRY_COMPARE(item.cachedFrames(), 24);
        QVERIFY(item.playing());
        item.setAnimationState("error");
        item.setAnimationState("idle");
        item.setMode("static");
        QTRY_COMPARE(item.cachedFrames(), 1);
        QCOMPARE(item.animationState(), "idle");
    }
    void timingAndNoHiddenWakeups() {
        QQuickWindow window;
        CoreWardenItem item(window.contentItem());
        window.show();
        item.setAssetRoot(root);
        item.setAnimationState("approval");
        item.setAnimate(true);
        QTRY_VERIFY(item.playing());
        QSignalSpy ticks(&item, &CoreWardenItem::frameChanged);
        QElapsedTimer elapsed;
        elapsed.start();
        QTest::qWait(1050);
        QVERIFY(ticks.count() >= 7 && ticks.count() <= 9);
        QCOMPARE(item.cachedFrames(), 24);
        qInfo() << "Playback events" << ticks.count() << "elapsed ms" << elapsed.elapsed()
                << "active RGBA cache bytes" << 24 * 192 * 192 * 4;
        item.setVisible(false);
        ticks.clear();
        QTest::qWait(300);
        QCOMPARE(ticks.count(), 0);
        QCOMPARE(item.cachedFrames(), 0);
    }
    void preferencePersistence() {
        QTemporaryDir temp;
        sentinel::core::JsonSettingsStore store(temp.path() + "/preferences.json");
        CoreWardenPreferences preferences(store);
        QCOMPARE(preferences.mode(), "auto");
        preferences.setMode("off");
        preferences.setMode("invalid");
        QCOMPARE(preferences.mode(), "off");
        CoreWardenPreferences restored(store);
        QCOMPARE(restored.mode(), "off");
    }
};
QTEST_MAIN(TestCoreWarden)
#include "test_core_warden.moc"
