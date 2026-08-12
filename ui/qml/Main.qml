// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

ApplicationWindow {
    id: root
    width: 1320
    height: 860
    minimumWidth: 780
    minimumHeight: 640
    visible: true
    title: qsTr("Sentinel Desktop Alpha")
    color: SentinelTheme.backgroundBase
    property var viewModel: shellViewModel
    property bool shellReady: true
    property string lastPrimaryPage: "Dashboard"
    property string currentShellPage: "Dashboard"
    readonly property bool compactLayout: root.width < 1080
    readonly property bool wideLayout: root.width >= SentinelTheme.breakpointWide
    readonly property int shellEntranceOffset: root.shellReady || MotionTokens.reduced(root.viewModel.currentModeName) ? 0 : 8
    readonly property int pageMotionOffset: MotionTokens.reduced(root.viewModel.currentModeName) ? 0 : 10
    readonly property bool nativeTitlebar: true

    Component.onCompleted: Qt.callLater(function() {
        SentinelTheme.activeTheme = root.viewModel.themeName
        SentinelTheme.reducedMotion = root.viewModel.reducedMotionEnabled
        SentinelTheme.highContrast = root.viewModel.highContrastEnabled
        SentinelTheme.uiDensity = root.viewModel.uiDensity
        MotionTokens.reducedMotion = root.viewModel.reducedMotionEnabled
        root.shellReady = true
        splashScreen.close()
        root.viewModel.registerMainWindow(root.winId)
        if (!root.viewModel.onboardingComplete)
            onboardingScreen.active = true
        else if (root.viewModel.recoveryDraftText.length > 0)
            recoveryModal.open()
    })

    Binding {
        target: SentinelTheme
        property: "activeTheme"
        value: root.viewModel.themeName
    }

    Connections {
        target: root.viewModel
        function onThemeNameChanged() {
            SentinelTheme.activeTheme = root.viewModel.themeName
        }
        function onNativeExperienceChanged() {
            SentinelTheme.reducedMotion = root.viewModel.reducedMotionEnabled
            SentinelTheme.highContrast = root.viewModel.highContrastEnabled
            SentinelTheme.uiDensity = root.viewModel.uiDensity
            MotionTokens.reducedMotion = root.viewModel.reducedMotionEnabled
        }
        function onRequestWindowActive(pageName) {
            root.raise()
            root.requestActivate()
            if (pageName.length > 0) {
                root.navigateToPage(pageName)
            }
        }
        function onRequestCompanionChat() {
            trayCompanionWindow.toggleVisibility()
        }
        function onOnboardingCompleteChanged() {
            if (!root.viewModel.onboardingComplete && !onboardingScreen.active) {
                onboardingScreen.step = 0
                onboardingScreen.active = true
            }
        }
    }



    function navigateToPage(pageName) {
        if (pageName === "Settings") {
            root.openSettings()
            return
        }
        if (pageName === "Dashboard" || pageName === "Models") {
            root.currentShellPage = pageName
            return
        }
        root.viewModel.currentPage = pageName
    }

    function openSettings() {
        settingsModal.open()
    }

    function openUpdateModal() {
        if (updateModal.updateState !== "available"
            && updateModal.updateState !== "downloading"
            && updateModal.updateState !== "completed") {
            updateModal.open()
            updateModal.startCheckAndDownload()
        } else {
            updateModal.open()
        }
    }

    function focusChatComposer() {
        root.currentShellPage = "Dashboard"
        Qt.callLater(function() {
            dashboardPage.focusComposer()
        })
    }

    background: Atmosphere {
        modeName: root.viewModel.currentModeName
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: SentinelTheme.pageMargin(root.width)
        anchors.rightMargin: SentinelTheme.pageMargin(root.width)
        anchors.topMargin: (root.compactLayout ? SentinelTheme.spaceMd : SentinelTheme.spaceXl)
                           + root.shellEntranceOffset
        anchors.bottomMargin: (root.compactLayout ? SentinelTheme.spaceMd : SentinelTheme.spaceXl)
                               + 72 + SentinelTheme.spaceMd
                               - root.shellEntranceOffset
        spacing: root.compactLayout ? SentinelTheme.spaceSm : SentinelTheme.spaceLg
        opacity: root.shellReady ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation {
                duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                easing.type: MotionTokens.enter
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
            spacing: root.compactLayout ? SentinelTheme.spaceMd : SentinelTheme.spaceXl

        ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: 0
                spacing: SentinelTheme.spaceMd


                ErrorBanner {
                    id: globalErrorBanner
                    Layout.fillWidth: true
                    Layout.maximumHeight: root.viewModel.globalErrorVisible ? 48 : 0
                    show: root.viewModel.globalErrorVisible
                    message: root.viewModel.globalErrorMessage
                    severity: root.viewModel.globalErrorSeverity
                    modeName: root.viewModel.currentModeName
                    onDismissed: root.viewModel.dismissGlobalError()
                }

                // Page stack: Dashboard / Models with slide + crossfade transitions
                Item {
                    id: pageStack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0

                    property real dashSlide: root.currentShellPage === "Models" ? -1 : 0
                    property real modelsSlide: root.currentShellPage === "Dashboard" ? 1 : 0

                    DashboardPage {
                        id: dashboardPage
                        viewModel: root.viewModel
                        anchors.fill: parent
                        visible: root.currentShellPage === "Dashboard"
                        opacity: root.currentShellPage === "Dashboard" ? 1.0 : 0.0

                        transform: Translate {
                            id: dashTranslate
                            x: pageStack.dashSlide * root.pageMotionOffset * 2
                        }

                        Behavior on opacity {
                            NumberAnimation { duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName); easing.type: Easing.OutCubic }
                        }
                    }

                    ModelsPage {
                        id: modelsPage
                        anchors.fill: parent
                        visible: opacity > 0
                        opacity: root.currentShellPage === "Models" ? 1.0 : 0.0

                        transform: Translate {
                            id: modelsTranslate
                            x: pageStack.modelsSlide * root.pageMotionOffset * 2
                        }

                        Behavior on opacity {
                            NumberAnimation { duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName); easing.type: Easing.OutCubic }
                        }
                    }

                    Behavior on dashSlide {
                        NumberAnimation {
                            duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                            easing.type: Easing.OutCubic
                        }
                    }

                    Behavior on modelsSlide {
                        NumberAnimation {
                            duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                            easing.type: Easing.OutCubic
                        }
                    }
                }
            }
        }
    }

    // ── Bottom Dock ──────────────────────────────────────────────────────────
    BottomDock {
        id: bottomDock
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: SentinelTheme.spaceLg
        currentPage: root.currentShellPage
        opacity: root.shellReady ? 1.0 : 0.0
        z: 100
        onPageRequested: function(pageName) {
            root.navigateToPage(pageName)
        }

        Behavior on opacity {
            NumberAnimation {
                duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                easing.type: MotionTokens.enter
            }
        }
    }



    Button {
        id: settingsFab
        anchors.right: parent.right
        anchors.rightMargin: SentinelTheme.pageMargin(root.width)
        anchors.verticalCenter: bottomDock.verticalCenter
        width: 52
        height: 52
        opacity: root.shellReady ? 1.0 : 0.0
        focusPolicy: Qt.StrongFocus
        hoverEnabled: true
        scale: settingsFab.down ? InteractionTokens.pressScale
                                : settingsFab.hovered || settingsFab.activeFocus
                                  ? InteractionTokens.focusScale
                                  : 1.0
        onClicked: root.openSettings()

        contentItem: Item {
            implicitWidth: 22
            implicitHeight: 22

            Canvas {
                anchors.centerIn: parent
                width: 20
                height: 20
                antialiasing: true

                onPaint: {
                    var ctx = getContext("2d")
                    ctx.reset()
                    var c = SentinelTheme.textPrimary
                    ctx.strokeStyle = Qt.rgba(c.r, c.g, c.b, 0.9)
                    ctx.lineWidth = 1.6
                    ctx.lineJoin = "round"
                    ctx.lineCap = "round"

                    var cx = width / 2
                    var cy = height / 2

                    // Outer gear ring
                    for (var i = 0; i < 8; i++) {
                        var a = i * Math.PI / 4
                        var innerR = 7
                        var outerR = 10
                        ctx.beginPath()
                        ctx.moveTo(cx + Math.cos(a - 0.25) * innerR, cy + Math.sin(a - 0.25) * innerR)
                        ctx.lineTo(cx + Math.cos(a) * outerR, cy + Math.sin(a) * outerR)
                        ctx.lineTo(cx + Math.cos(a + 0.25) * innerR, cy + Math.sin(a + 0.25) * innerR)
                        ctx.stroke()
                    }

                    // Inner circle
                    ctx.beginPath()
                    ctx.arc(cx, cy, 4.5, 0, Math.PI * 2)
                    ctx.stroke()
                }
            }
        }

        background: Rectangle {
            id: fabBg
            radius: width / 2
            color: InteractionTokens.surfaceColor(settingsFab.hovered, settingsFab.down,
                                                   settingsModal.opened,
                                                   SentinelTheme.calmAccent)
            border.color: InteractionTokens.borderColor(settingsFab.activeFocus, settingsFab.hovered,
                                                         settingsModal.opened,
                                                         SentinelTheme.calmAccent)
            border.width: 1

            layer.enabled: true
            layer.effect: MultiEffect {
                shadowEnabled: true
                shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.18)
                shadowVerticalOffset: 2
                shadowBlur: 0.15
                shadowOpacity: 1.0
            }

            Behavior on color {
                ColorAnimation {
                    duration: MotionTokens.fast
                    easing.type: MotionTokens.standard
                }
            }

            Behavior on border.color {
                ColorAnimation {
                    duration: MotionTokens.fast
                    easing.type: MotionTokens.standard
                }
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: MotionTokens.duration(MotionTokens.fast, root.viewModel.currentModeName)
                easing.type: MotionTokens.press
            }
        }

        Behavior on opacity {
            NumberAnimation {
                duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                easing.type: MotionTokens.enter
            }
        }
    }

    SoundManager {
        id: soundManager
        enabled: !root.viewModel.dndEnabled && root.viewModel.soundEffectsEnabled
    }

    SplashScreen {
        id: splashScreen
        modeName: root.viewModel.currentModeName
    }

    CommandPalette {
        id: commandPalette
        viewModel: root.viewModel
        onOpenSettingsRequested: root.openSettings()
        onOpenUpdateRequested: root.openUpdateModal()
        onFocusChatRequested: root.focusChatComposer()
    }

    TrayCompanionWindow {
        id: trayCompanionWindow
        viewModel: root.viewModel
    }

    OnboardingScreen {
        id: onboardingScreen
        viewModel: root.viewModel
        anchors.fill: parent
        onFinished: {
            root.viewModel.onboardingComplete = true
            onboardingScreen.active = false
        }
    }

    UpdateProgressModal {
        id: updateModal
        viewModel: root.viewModel
        currentVersion: Qt.application.version
        modeName: root.viewModel.currentModeName
        accent: SentinelTheme.modeAccent(root.viewModel.currentModeName)
        onCheckRequested: {
            root.viewModel.checkForUpdates()
        }
        onDownloadRequested: {
            var downloadUrl = root.viewModel.lastAssetUrl
            if (downloadUrl && downloadUrl.length > 0) {
                updateModal.startDownload()
                root.viewModel.startDownload(downloadUrl)
            }
        }
        onUpdateCompleted: {
            root.viewModel.relaunchApplication()
        }
    }

    SentinelOverlayModal {
        id: recoveryModal
        accent: SentinelTheme.warning
        modeName: root.viewModel.currentModeName
        preferredWidth: Math.min(520, root.width - SentinelTheme.space4Xl)
        preferredHeight: 260

        contentItem: ColumnLayout {
            spacing: SentinelTheme.spaceMd
            Label {
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceLg
                text: qsTr("Restore previous session?")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontCard
                font.bold: true
            }
            Label {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceLg
                Layout.rightMargin: SentinelTheme.spaceLg
                text: qsTr("Sentinel saved your last draft locally. Restore it into the composer or discard it.")
                color: SentinelTheme.textMuted
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceLg
                SentinelButton {
                    text: qsTr("Discard")
                    onClicked: {
                        root.viewModel.recoveryDraftText = ""
                        recoveryModal.close()
                    }
                }
                Item { Layout.fillWidth: true }
                SentinelButton {
                    text: qsTr("Restore")
                    onClicked: {
                        dashboardPage.restoreDraft(root.viewModel.recoveryDraftText)
                        root.viewModel.recoveryDraftText = ""
                        recoveryModal.close()
                    }
                }
            }
        }
    }

    // ── Notification Center Panel ─────────────────────────────────────────────
    Popup {
        id: notifCenterPopup
        x: Math.max(16, root.width - 520 - SentinelTheme.pageMargin(root.width))
        y: 72
        width: 500
        height: Math.min(600, root.height - 120)
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        visible: root.viewModel ? root.viewModel.notificationCenterVisible : false

        onClosed: {
            if (root.viewModel) root.viewModel.notificationCenterVisible = false
        }

        background: Rectangle {
            color: "transparent"
        }

        contentItem: NotificationCenterPanel {
            viewModel: root.viewModel
            width: parent.width
            height: parent.height
            onCloseRequested: {
                notifCenterPopup.close()
            }
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0; to: 1; duration: MotionTokens.normal; easing.type: Easing.OutCubic }
            NumberAnimation { property: "y"; from: 56; to: 72; duration: MotionTokens.normal; easing.type: Easing.OutCubic }
        }

        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1; to: 0; duration: MotionTokens.fast; easing.type: Easing.InCubic }
            NumberAnimation { property: "y"; from: 72; to: 56; duration: MotionTokens.fast; easing.type: Easing.InCubic }
        }
    }

    Connections {
        target: root.viewModel
        function onNotificationCenterVisibleChanged() {
            if (root.viewModel.notificationCenterVisible) {
                notifCenterPopup.open()
            } else {
                notifCenterPopup.close()
            }
        }
    }

    SentinelOverlayModal {
        id: settingsModal
        accent: SentinelTheme.modeAccent(root.viewModel.currentModeName)
        modeName: root.viewModel.currentModeName
        preferredWidth: Math.min(1040, root.width - SentinelTheme.space4Xl)
        preferredHeight: Math.min(760, root.height - SentinelTheme.space4Xl)

        contentItem: SettingsPage {
            viewModel: root.viewModel
            soundManager: soundManager
            width: settingsModal.width
            height: settingsModal.height
            onOpenUpdateRequested: {
                settingsModal.close()
                root.openUpdateModal()
            }
        }
    }

    Connections {
        target: root.viewModel
        function onRequestOpenUpdateModal() {
            root.openUpdateModal()
        }
        function onCurrentPageChanged() {
            if (root.viewModel.currentPage === "Settings") {
                root.openSettings()
                root.viewModel.currentPage = root.lastPrimaryPage
                return
            }
            root.lastPrimaryPage = root.viewModel.currentPage
        }
        function onUpdateCheckCompleted(available, version, releaseNotes, downloadUrl, assetSize) {
            if (available) {
                updateModal.availableVersion = version
                if (releaseNotes && releaseNotes.length > 0) {
                    updateModal.releaseNotesText = releaseNotes
                }
                if (assetSize > 0) {
                    var sizeMB = (assetSize / (1024 * 1024)).toFixed(1)
                    updateModal.packageSizeText = sizeMB + " MB"
                }
                updateModal.updateState = "available"
            } else {
                var wf = root.viewModel.updateWorkflowState
                if (wf && wf.startsWith("Update check failed")) {
                    updateModal.updateState = "error"
                    updateModal.errorMessage = wf
                } else {
                    updateModal.updateState = "upToDate"
                }
            }
        }
        function onUpdateDownloadProgressChanged(bytesReceived, bytesTotal, speedBytesPerSec) {
            updateModal.updateState = "downloading"
            updateModal.downloadProgress = bytesTotal > 0 ? bytesReceived / bytesTotal : 0.0

            var receivedMB = (bytesReceived / (1024 * 1024)).toFixed(1)
            var totalMB = (bytesTotal / (1024 * 1024)).toFixed(1)
            updateModal.downloadedBytesText = receivedMB + " MB / " + totalMB + " MB"

            if (speedBytesPerSec > 0) {
                var speedText
                if (speedBytesPerSec > 1024 * 1024) {
                    speedText = (speedBytesPerSec / (1024 * 1024)).toFixed(1) + " MB/s"
                } else if (speedBytesPerSec > 1024) {
                    speedText = (speedBytesPerSec / 1024).toFixed(1) + " KB/s"
                } else {
                    speedText = speedBytesPerSec.toFixed(0) + " B/s"
                }
                updateModal.downloadSpeedText = speedText
            }
        }
        function onUpdateDownloadFinished(success, filePath) {
            if (success) {
                updateModal.updateState = "completed"
                updateModal.downloadProgress = 1.0
            } else {
                updateModal.updateState = "error"
                updateModal.errorMessage = qsTr("Download failed. Please check your network connection and try again.")
            }
        }
    }

    Shortcut {
        sequences: ["Ctrl+K", "Meta+K"]
        onActivated: commandPalette.openPalette()
    }

    Shortcut {
        sequences: ["Ctrl+1", "Meta+1"]
        onActivated: root.navigateToPage("Dashboard")
    }

    Shortcut {
        sequences: ["Ctrl+2", "Meta+2"]
        onActivated: root.navigateToPage("Models")
    }

    Shortcut {
        sequences: ["Ctrl+4", "Meta+4"]
        onActivated: root.openSettings()
    }

    Shortcut {
        sequences: ["Ctrl+L", "Meta+L"]
        onActivated: root.focusChatComposer()
    }

    Shortcut {
        sequences: ["Ctrl+,", "Meta+,"]
        onActivated: root.openSettings()
    }

    Shortcut {
        sequences: ["Ctrl+Shift+C", "Meta+Shift+C"]
        onActivated: trayCompanionWindow.toggleVisibility()
    }

    Shortcut {
        sequence: "Esc"
        onActivated: {
            if (commandPalette.opened)
                commandPalette.close()
            else if (settingsModal.opened)
                settingsModal.close()
        }
    }
}
