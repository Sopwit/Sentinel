// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/desktop/DesktopErrorProjection.h"
#include "sentinel/desktop/DesktopShellViewModel.h"
#include "sentinel/desktop/WindowStatePersistence.h"

#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/app/ModeManager.h"
#include "sentinel/core/memory/InMemorySettingsStore.h"
#include "sentinel/desktop/DaemonClient.h"
#include "sentinel/desktop/DesktopRuntimeClient.h"

#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QWindow>
#include <QtTest>

#include <memory>

using sentinel::desktop::DesktopErrorCategory;
using sentinel::desktop::DesktopShellViewModel;
using sentinel::desktop::WindowGeometry;
using sentinel::desktop::WindowStateController;
using sentinel::desktop::WindowStateRules;

namespace {

WindowGeometry geometry(int x, int y, int width, int height, bool hasPosition, bool maximized) {
    WindowGeometry value;
    value.x = x;
    value.y = y;
    value.width = width;
    value.height = height;
    value.hasPosition = hasPosition;
    value.maximized = maximized;
    return value;
}

const QList<QRect> kSingleScreen{QRect(0, 0, 1920, 1080)};

void writeWindowSettings(const QString& path, const WindowGeometry& value) {
    QSettings settings(path, QSettings::IniFormat);
    settings.beginGroup(QStringLiteral("mainWindow"));
    settings.setValue(QStringLiteral("schema"), 1);
    settings.setValue(QStringLiteral("width"), value.width);
    settings.setValue(QStringLiteral("height"), value.height);
    settings.setValue(QStringLiteral("x"), value.x);
    settings.setValue(QStringLiteral("y"), value.y);
    settings.setValue(QStringLiteral("hasPosition"), value.hasPosition);
    settings.setValue(QStringLiteral("maximized"), value.maximized);
    settings.endGroup();
    settings.sync();
}

} // namespace

class DesktopShellStateTest final : public QObject {
    Q_OBJECT

private slots:
    void defaultGeometryWhenNothingSaved();
    void enforcesMinimumSize();
    void rejectsFullyOffscreenPosition();
    void clampsPartiallyOffscreenPosition();
    void clampsOversizedWindowToReducedResolution();
    void allowsSecondaryScreenPosition();
    void dropsPositionWhenNoScreens();
    void preservesMaximizedFlag();
    void reachabilityDetectsStrandedFrames();

    void persistenceWritesCapturedGeometry();
    void restoresSavedGeometryOnAttach();
    void recoversFromInvalidSavedGeometry();

    void daemonStateMapsToBannerCategory();

    void daemonUnavailableBannerAppearsAndDismisses();
    void daemonUnavailableBannerWaitsForGracePeriod();
    void repeatedEquivalentErrorsAreDeduplicated();
};

void DesktopShellStateTest::defaultGeometryWhenNothingSaved() {
    const auto resolved = WindowStateRules::resolve({}, kSingleScreen, kSingleScreen.first());
    QCOMPARE(resolved.width, WindowStateRules::kDefaultWidth);
    QCOMPARE(resolved.height, WindowStateRules::kDefaultHeight);
    QVERIFY(!resolved.hasPosition);
    QVERIFY(!resolved.maximized);
}

void DesktopShellStateTest::enforcesMinimumSize() {
    const auto resolved = WindowStateRules::resolve(geometry(10, 10, 100, 100, true, false),
                                                    kSingleScreen, kSingleScreen.first());
    QCOMPARE(resolved.width, WindowStateRules::kMinimumWidth);
    QCOMPARE(resolved.height, WindowStateRules::kMinimumHeight);
}

void DesktopShellStateTest::rejectsFullyOffscreenPosition() {
    const auto resolved = WindowStateRules::resolve(geometry(60000, 60000, 1000, 700, true, false),
                                                    kSingleScreen, kSingleScreen.first());
    QVERIFY(!resolved.hasPosition);
    QCOMPARE(resolved.width, 1000);
    QCOMPARE(resolved.height, 700);
}

void DesktopShellStateTest::clampsPartiallyOffscreenPosition() {
    const auto resolved = WindowStateRules::resolve(geometry(1800, 1000, 1000, 700, true, false),
                                                    kSingleScreen, kSingleScreen.first());
    QVERIFY(resolved.hasPosition);
    QVERIFY(resolved.x >= 0);
    QVERIFY(resolved.y >= 0);
    QVERIFY(resolved.x + resolved.width <= kSingleScreen.first().width());
    QVERIFY(resolved.y + resolved.height <= kSingleScreen.first().height());
}

void DesktopShellStateTest::clampsOversizedWindowToReducedResolution() {
    const QList<QRect> smallScreen{QRect(0, 0, 1280, 800)};
    const auto resolved = WindowStateRules::resolve(geometry(4000, 4000, 3000, 2000, true, false),
                                                    smallScreen, smallScreen.first());
    QVERIFY(!resolved.hasPosition);
    QCOMPARE(resolved.width, 1280);
    QCOMPARE(resolved.height, 800);
}

void DesktopShellStateTest::allowsSecondaryScreenPosition() {
    const QList<QRect> screens{QRect(0, 0, 1920, 1080), QRect(1920, 0, 1920, 1080)};
    const auto resolved = WindowStateRules::resolve(geometry(2000, 200, 1000, 700, true, false),
                                                    screens, screens.first());
    QVERIFY(resolved.hasPosition);
    QCOMPARE(resolved.x, 2000);
    QCOMPARE(resolved.y, 200);
}

void DesktopShellStateTest::dropsPositionWhenNoScreens() {
    const auto resolved =
        WindowStateRules::resolve(geometry(100, 100, 1000, 700, true, false), {}, QRect());
    QVERIFY(!resolved.hasPosition);
    QCOMPARE(resolved.width, 1000);
    QCOMPARE(resolved.height, 700);
}

void DesktopShellStateTest::preservesMaximizedFlag() {
    const auto resolved = WindowStateRules::resolve(geometry(100, 100, 1000, 700, true, true),
                                                    kSingleScreen, kSingleScreen.first());
    QVERIFY(resolved.maximized);
}

void DesktopShellStateTest::reachabilityDetectsStrandedFrames() {
    // Fully on the only screen.
    QVERIFY(WindowStateRules::isReachable(100, 100, 800, 600, kSingleScreen));
    // Partially overlapping is still reachable.
    QVERIFY(WindowStateRules::isReachable(1800, 900, 800, 600, kSingleScreen));
    // A frame from a display that is no longer attached is stranded.
    QVERIFY(!WindowStateRules::isReachable(3000, 100, 800, 600, kSingleScreen));
    // A frame on a valid secondary screen is reachable.
    const QList<QRect> dual{QRect(0, 0, 1920, 1080), QRect(1920, 0, 1920, 1080)};
    QVERIFY(WindowStateRules::isReachable(2600, 100, 800, 600, dual));
    // Invalid screen rectangles are ignored.
    QVERIFY(!WindowStateRules::isReachable(50, 50, 100, 100, {QRect()}));
}

void DesktopShellStateTest::persistenceWritesCapturedGeometry() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("window.ini"));

    {
        QSettings settings(path, QSettings::IniFormat);
        QWindow window;
        WindowStateController controller(settings);
        controller.attach(&window);
        window.resize(900, 680);
        window.setPosition(20, 30);
        controller.saveNow();
    }

    QSettings stored(path, QSettings::IniFormat);
    stored.beginGroup(QStringLiteral("mainWindow"));
    QCOMPARE(stored.value(QStringLiteral("schema")).toInt(), 1);
    QCOMPARE(stored.value(QStringLiteral("width")).toInt(), 900);
    QCOMPARE(stored.value(QStringLiteral("height")).toInt(), 680);
    QCOMPARE(stored.value(QStringLiteral("hasPosition")).toBool(), true);
    QCOMPARE(stored.value(QStringLiteral("maximized")).toBool(), false);
    stored.endGroup();
}

void DesktopShellStateTest::restoresSavedGeometryOnAttach() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    QVERIFY(QGuiApplication::primaryScreen() != nullptr);
    const QRect available = QGuiApplication::primaryScreen()->availableGeometry();

    const WindowGeometry saved =
        geometry(available.left(), available.top(), 1000, 700, true, false);
    const QString path = directory.filePath(QStringLiteral("window.ini"));
    writeWindowSettings(path, saved);

    QSettings settings(path, QSettings::IniFormat);
    QWindow window;
    WindowStateController controller(settings);
    controller.attach(&window);

    QVERIFY(controller.restoredFromDisk());
    QVERIFY(controller.lastRestored().hasPosition);
    QCOMPARE(window.width(), controller.lastRestored().width);
    QCOMPARE(window.height(), controller.lastRestored().height);
    QVERIFY(controller.positionApplied());
    QCOMPARE(window.x(), controller.lastRestored().x);
    QCOMPARE(window.y(), controller.lastRestored().y);
}

void DesktopShellStateTest::recoversFromInvalidSavedGeometry() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath(QStringLiteral("window.ini"));
    writeWindowSettings(path, geometry(100000, 100000, 30, 30, true, false));

    QSettings settings(path, QSettings::IniFormat);
    QWindow window;
    WindowStateController controller(settings);
    controller.attach(&window);

    // Invalid size is raised to the minimum and the unusable position is dropped.
    QVERIFY(window.width() >= WindowStateRules::kMinimumWidth);
    QVERIFY(window.height() >= WindowStateRules::kMinimumHeight);
    QVERIFY(!controller.positionApplied());
    QVERIFY(!controller.lastRestored().hasPosition);
}

void DesktopShellStateTest::daemonStateMapsToBannerCategory() {
    using State = sentinel::desktop::DaemonClient::ConnectionState;
    QCOMPARE(sentinel::desktop::desktopErrorCategoryForDaemonState(State::Unavailable).value(),
             DesktopErrorCategory::DaemonUnavailable);
    QCOMPARE(sentinel::desktop::desktopErrorCategoryForDaemonState(State::Reconnecting).value(),
             DesktopErrorCategory::DaemonDisconnected);
    QCOMPARE(sentinel::desktop::desktopErrorCategoryForDaemonState(State::Connected).value(),
             DesktopErrorCategory::None);
    QVERIFY(!sentinel::desktop::desktopErrorCategoryForDaemonState(State::Connecting).has_value());
    QVERIFY(
        !sentinel::desktop::desktopErrorCategoryForDaemonState(State::Disconnected).has_value());
    QVERIFY(
        !sentinel::desktop::desktopErrorCategoryForDaemonState(State::ShuttingDown).has_value());
    QVERIFY(
        !sentinel::desktop::desktopErrorCategoryForDaemonState(State::VersionMismatch).has_value());
}

void DesktopShellStateTest::daemonUnavailableBannerAppearsAndDismisses() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    sentinel::core::AppSettings appSettings(
        std::make_unique<sentinel::core::InMemorySettingsStore>(),
        sentinel::core::inMemoryTestCredentialStore());
    sentinel::core::ModeManager modes;
    sentinel::desktop::DaemonClient transport(directory.filePath(QStringLiteral("absent.sock")),
                                              500, 1000);
    sentinel::desktop::DesktopRuntimeClient runtime(transport);
    DesktopShellViewModel shell(runtime, modes, appSettings);
    shell.setDaemonAvailabilityGracePeriodMs(0);

    QTRY_VERIFY(shell.globalErrorVisible());
    QCOMPARE(shell.globalErrorSeverity(), QStringLiteral("Warning"));
    QCOMPARE(shell.globalErrorActionLabel(), QStringLiteral("Retry"));

    shell.dismissGlobalError();
    QVERIFY(!shell.globalErrorVisible());
}

void DesktopShellStateTest::daemonUnavailableBannerWaitsForGracePeriod() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    sentinel::core::AppSettings appSettings(
        std::make_unique<sentinel::core::InMemorySettingsStore>(),
        sentinel::core::inMemoryTestCredentialStore());
    sentinel::core::ModeManager modes;
    sentinel::desktop::DaemonClient transport(directory.filePath(QStringLiteral("absent.sock")),
                                              500, 400);
    sentinel::desktop::DesktopRuntimeClient runtime(transport);
    DesktopShellViewModel shell(runtime, modes, appSettings);
    shell.setDaemonAvailabilityGracePeriodMs(1200);

    // A daemon that is merely still starting must not raise an alarm immediately.
    QTest::qWait(200);
    QVERIFY(!shell.globalErrorVisible());

    // A daemon that stays absent past the grace window is surfaced, even across retries.
    QTRY_VERIFY_WITH_TIMEOUT(shell.globalErrorVisible(), 4000);
    QCOMPARE(shell.globalErrorActionLabel(), QStringLiteral("Retry"));
}

void DesktopShellStateTest::repeatedEquivalentErrorsAreDeduplicated() {
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    sentinel::core::AppSettings appSettings(
        std::make_unique<sentinel::core::InMemorySettingsStore>(),
        sentinel::core::inMemoryTestCredentialStore());
    sentinel::core::ModeManager modes;
    sentinel::desktop::DaemonClient transport(directory.filePath(QStringLiteral("absent.sock")),
                                              500, 1000);
    sentinel::desktop::DesktopRuntimeClient runtime(transport);
    DesktopShellViewModel shell(runtime, modes, appSettings);

    QSignalSpy spy(&shell, &DesktopShellViewModel::globalErrorChanged);
    shell.setGlobalError(QStringLiteral("Same failure"), QStringLiteral("Error"),
                         QStringLiteral("Retry"));
    shell.setGlobalError(QStringLiteral("Same failure"), QStringLiteral("Error"),
                         QStringLiteral("Retry"));
    QCOMPARE(spy.count(), 1);
    QVERIFY(shell.globalErrorVisible());

    shell.setGlobalError(QStringLiteral("Different failure"), QStringLiteral("Error"),
                         QStringLiteral("Retry"));
    QCOMPARE(spy.count(), 2);
}

QTEST_MAIN(DesktopShellStateTest)

#include "test_desktop_shell_state.moc"
