// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/app/ModeManager.h"
#include "sentinel/core/memory/InMemorySettingsStore.h"
#include "sentinel/desktop/DesktopRuntimeClient.h"
#include "sentinel/desktop/DesktopShellViewModel.h"
#include "sentinel/desktop/NativeCompanionAdapter.h"
#include "sentinel/desktop/QuickPanelController.h"
#include <QApplication>
#include <QKeyEvent>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QWindow>
class NativeIntegrationTest final : public QObject {
    Q_OBJECT
private slots:
    void settingsActionsAndUnavailableRegistration() {
        QTemporaryDir directory;
        sentinel::core::AppSettings settings(
            std::make_unique<sentinel::core::InMemorySettingsStore>(),
            sentinel::core::inMemoryTestCredentialStore());
        sentinel::core::ModeManager modes;
        sentinel::desktop::DaemonClient transport(directory.filePath("absent.sock"), 500, 1000);
        sentinel::desktop::DesktopRuntimeClient runtime(transport);
        sentinel::desktop::DesktopShellViewModel shell(runtime, modes, settings);
        sentinel::desktop::QuickPanelController panel(runtime);
        QWindow window;
        sentinel::desktop::NativeCompanionAdapter native(shell, settings, &window);
        native.bindQuickPanel(&panel);
        QVERIFY(native.shortcutStatus().contains("headless"));
        QCOMPARE(native.shortcut(), QString("Disabled"));
        native.setShortcut("Ctrl+Alt+S");
        QCOMPARE(settings.quickPanelShortcut(), QString("Ctrl+Alt+S"));
        native.setStartAtLogin(true);
        QVERIFY(!settings.quickPanelStartAtLogin());
        QVERIFY(native.startupStatus().contains("headless"));
        QSignalSpy active(&shell, &sentinel::desktop::DesktopShellViewModel::requestWindowActive);
        native.openSettings();
        QVERIFY(window.isVisible());
        QCOMPARE(active.last().first().toString(), QString("Settings"));
        panel.continueConversation();
        QCOMPARE(active.last().first().toString(), QString("Dashboard"));
        window.hide();
        QWindow quickWindow;
        quickWindow.setObjectName("sentinelQuickPanel");
        settings.setCompanionEnabled(true);
        for (int repeat = 0; repeat < 3; ++repeat) {
            native.togglePanel();
            QVERIFY(quickWindow.isVisible());
            QKeyEvent override(QEvent::ShortcutOverride, Qt::Key_Escape, Qt::NoModifier);
            override.ignore();
            QCoreApplication::sendEvent(&quickWindow, &override);
            QVERIFY(override.isAccepted());
            QVERIFY(quickWindow.isVisible());
            QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
            QCoreApplication::sendEvent(&quickWindow, &escape);
            QVERIFY(!quickWindow.isVisible());
        }
    }
};
QTEST_MAIN(NativeIntegrationTest)
#include "test_native_integration.moc"
