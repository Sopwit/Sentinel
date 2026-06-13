import QtQuick
import QtQuick.Controls.Basic
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
    property bool shellReady: false
    property string lastPrimaryPage: "Dashboard"
    readonly property bool compactLayout: root.width < 1080
    readonly property bool wideLayout: root.width >= SentinelTheme.breakpointWide
    readonly property int shellEntranceOffset: root.shellReady || MotionTokens.reduced(root.viewModel.currentModeName) ? 0 : 8
    readonly property int pageMotionOffset: MotionTokens.reduced(root.viewModel.currentModeName) ? 0 : 10
    readonly property int dockZoneHeight: (compactLayout ? 58 : 62)
                                          + (compactLayout ? SentinelTheme.spaceMd
                                                           : SentinelTheme.spaceXl)
                                          + SentinelTheme.spaceLg
    Component.onCompleted: Qt.callLater(function() {
        SentinelTheme.activeTheme = root.viewModel.themeName
        root.shellReady = true
        if (!root.viewModel.onboardingComplete)
            onboardingModal.open()
        else if (root.viewModel.recoveryDraftText.length > 0)
            recoveryModal.open()
    })

    Connections {
        target: root.viewModel
        function onThemeNameChanged() {
            SentinelTheme.activeTheme = root.viewModel.themeName
        }
    }

    function currentPageIndex() {
        if (root.viewModel.currentPage === "Dashboard")
            return 0
        if (root.viewModel.currentPage === "Memory")
            return 1
        if (root.viewModel.currentPage === "Agents")
            return 2
        return 0
    }

    function navigateToPage(pageName) {
        if (pageName === "Settings") {
            root.openSettings()
            return
        }
        root.viewModel.currentPage = pageName
    }

    function openSettings() {
        settingsModal.open()
    }

    function focusChatComposer() {
        root.viewModel.currentPage = "Dashboard"
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
        anchors.bottomMargin: root.dockZoneHeight - root.shellEntranceOffset
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

                HeaderBar {
                    viewModel: root.viewModel
                    compact: root.compactLayout
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.compactLayout ? 100 : 112
                }

                StackLayout {
                    id: pageStack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0
                    clip: true
                    currentIndex: root.currentPageIndex()

                    DashboardPage {
                        id: dashboardPage
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex() === 0 ? 1.0 : 0.0
                        transform: Translate {
                            y: root.currentPageIndex() === 0 ? 0 : root.pageMotionOffset

                            Behavior on y {
                                NumberAnimation {
                                    duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                                    easing.type: MotionTokens.enter
                                }
                            }
                        }

                        Behavior on opacity {
                            NumberAnimation {
                                duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                                easing.type: MotionTokens.standard
                            }
                        }
                    }
                    MemoryPage {
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex() === 1 ? 1.0 : 0.0
                        transform: Translate {
                            y: root.currentPageIndex() === 1 ? 0 : root.pageMotionOffset

                            Behavior on y {
                                NumberAnimation {
                                    duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                                    easing.type: MotionTokens.enter
                                }
                            }
                        }

                        Behavior on opacity {
                            NumberAnimation {
                                duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                                easing.type: MotionTokens.standard
                            }
                        }
                    }
                    AgentsPage {
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex() === 2 ? 1.0 : 0.0
                        transform: Translate {
                            y: root.currentPageIndex() === 2 ? 0 : root.pageMotionOffset

                            Behavior on y {
                                NumberAnimation {
                                    duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                                    easing.type: MotionTokens.enter
                                }
                            }
                        }

                        Behavior on opacity {
                            NumberAnimation {
                                duration: MotionTokens.duration(MotionTokens.page, root.viewModel.currentModeName)
                                easing.type: MotionTokens.standard
                            }
                        }
                    }
                }

                StatusBar {
                    viewModel: root.viewModel
                    Layout.fillWidth: true
                    Layout.preferredHeight: 34
                }
            }
        }
    }

    SentinelDock {
        id: dock
        viewModel: root.viewModel
        compact: root.compactLayout
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: root.compactLayout ? SentinelTheme.spaceMd : SentinelTheme.spaceXl
        width: Math.min(root.width - SentinelTheme.space4Xl,
                        root.compactLayout ? 360 : 440)
        height: compact ? 58 : 62
        opacity: root.shellReady ? 1.0 : 0.0
        z: 30

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
        anchors.bottom: parent.bottom
        anchors.rightMargin: SentinelTheme.space2Xl
        anchors.bottomMargin: SentinelTheme.space2Xl
        width: 52
        height: 52
        opacity: root.shellReady ? 1.0 : 0.0
        focusPolicy: Qt.StrongFocus
        hoverEnabled: true
        scale: settingsFab.down ? InteractionTokens.pressScale
                                : settingsFab.hovered || settingsFab.activeFocus
                                  ? InteractionTokens.focusScale
                                  : 1.0
        text: "\u2699"
        font.pixelSize: 22
        onClicked: root.openSettings()

        contentItem: Text {
            text: settingsFab.text
            color: SentinelTheme.textPrimary
            font.pixelSize: settingsFab.font.pixelSize
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        background: Rectangle {
            radius: width / 2
            color: InteractionTokens.surfaceColor(settingsFab.hovered, settingsFab.down,
                                                   settingsModal.opened,
                                                   SentinelTheme.calmAccent)
            border.color: InteractionTokens.borderColor(settingsFab.activeFocus, settingsFab.hovered,
                                                         settingsModal.opened,
                                                         SentinelTheme.calmAccent)
            border.width: 1

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

    CommandPalette {
        id: commandPalette
        viewModel: root.viewModel
        onOpenSettingsRequested: root.openSettings()
        onFocusChatRequested: root.focusChatComposer()
    }

    SentinelOverlayModal {
        id: onboardingModal
        accent: SentinelTheme.modeAccent(root.viewModel.currentModeName)
        modeName: root.viewModel.currentModeName
        preferredWidth: Math.min(720, root.width - SentinelTheme.space4Xl)
        preferredHeight: Math.min(560, root.height - SentinelTheme.space4Xl)
        closePolicy: Popup.NoAutoClose

        property int step: 0

        contentItem: ColumnLayout {
            spacing: SentinelTheme.spaceLg

            Label {
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceLg
                Layout.bottomMargin: 0
                text: onboardingModal.step === 0 ? qsTr("Welcome to Sentinel")
                     : onboardingModal.step === 1 ? qsTr("Device Overview")
                     : qsTr("Starter Setup")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontTitle
                font.bold: true
            }

            StackLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: SentinelTheme.spaceLg
                Layout.rightMargin: SentinelTheme.spaceLg
                currentIndex: onboardingModal.step

                Flow {
                    spacing: SentinelTheme.spaceSm
                    Repeater {
                        model: ["Coding", "Study", "Writing", "General Assistant"]
                        SentinelButton {
                            required property string modelData
                            text: modelData
                            onClicked: root.viewModel.onboardingUseCase = modelData
                        }
                    }
                }

                ColumnLayout {
                    spacing: SentinelTheme.spaceSm
                    InfoRow { label: qsTr("Platform"); value: Qt.platform.os; Layout.fillWidth: true }
                    InfoRow { label: qsTr("Runtime"); value: root.viewModel.activeRuntimeProviderLabel + " / " + root.viewModel.activeRuntimeReadinessState; Layout.fillWidth: true }
                    InfoRow { label: qsTr("Privacy"); value: qsTr("No telemetry, no hidden uploads, no background scans."); Layout.fillWidth: true }
                }

                ColumnLayout {
                    spacing: SentinelTheme.spaceSm
                    InfoRow { label: qsTr("Use Case"); value: root.viewModel.onboardingUseCase; Layout.fillWidth: true }
                    InfoRow { label: qsTr("Theme"); value: root.viewModel.themeName; Layout.fillWidth: true }
                    InfoRow { label: qsTr("Models"); value: qsTr("No downloads are started."); Layout.fillWidth: true }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceLg
                SentinelButton {
                    text: qsTr("Back")
                    enabled: onboardingModal.step > 0
                    onClicked: onboardingModal.step--
                }
                Item { Layout.fillWidth: true }
                SentinelButton {
                    text: onboardingModal.step < 2 ? qsTr("Next") : qsTr("Start")
                    onClicked: {
                        if (onboardingModal.step < 2) {
                            onboardingModal.step++
                        } else {
                            root.viewModel.onboardingComplete = true
                            onboardingModal.close()
                        }
                    }
                }
            }
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

    SentinelOverlayModal {
        id: settingsModal
        accent: SentinelTheme.modeAccent(root.viewModel.currentModeName)
        modeName: root.viewModel.currentModeName
        preferredWidth: Math.min(1040, root.width - SentinelTheme.space4Xl)
        preferredHeight: Math.min(760, root.height - SentinelTheme.space4Xl)

        contentItem: SettingsPage {
            viewModel: root.viewModel
            width: settingsModal.width
            height: settingsModal.height
        }
    }

    Connections {
        target: root.viewModel
        function onCurrentPageChanged() {
            if (root.viewModel.currentPage === "Settings") {
                root.openSettings()
                root.viewModel.currentPage = root.lastPrimaryPage
                return
            }
            root.lastPrimaryPage = root.viewModel.currentPage
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
        onActivated: root.navigateToPage("Memory")
    }

    Shortcut {
        sequences: ["Ctrl+3", "Meta+3"]
        onActivated: root.navigateToPage("Agents")
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
        sequence: "Esc"
        onActivated: {
            if (commandPalette.opened)
                commandPalette.close()
            else if (settingsModal.opened)
                settingsModal.close()
        }
    }
}
