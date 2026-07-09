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
    property bool shellReady: true
    property string lastPrimaryPage: "Dashboard"
    property string currentShellPage: "Dashboard"
    readonly property bool compactLayout: root.width < 1080
    readonly property bool wideLayout: root.width >= SentinelTheme.breakpointWide
    readonly property int shellEntranceOffset: root.shellReady || MotionTokens.reduced(root.viewModel.currentModeName) ? 0 : 8
    readonly property int pageMotionOffset: MotionTokens.reduced(root.viewModel.currentModeName) ? 0 : 10
    Component.onCompleted: Qt.callLater(function() {
        SentinelTheme.activeTheme = root.viewModel.themeName
        SentinelTheme.reducedMotion = root.viewModel.reducedMotionEnabled
        SentinelTheme.highContrast = root.viewModel.highContrastEnabled
        SentinelTheme.uiDensity = root.viewModel.uiDensity
        MotionTokens.reducedMotion = root.viewModel.reducedMotionEnabled
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
        function onNativeExperienceChanged() {
            SentinelTheme.reducedMotion = root.viewModel.reducedMotionEnabled
            SentinelTheme.highContrast = root.viewModel.highContrastEnabled
            SentinelTheme.uiDensity = root.viewModel.uiDensity
            MotionTokens.reducedMotion = root.viewModel.reducedMotionEnabled
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


                // Page stack: Dashboard / Models
                StackLayout {
                    id: pageStack
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0
                    currentIndex: root.currentShellPage === "Models" ? 1 : 0

                    Behavior on currentIndex {
                        enabled: false
                    }

                    DashboardPage {
                        id: dashboardPage
                        viewModel: root.viewModel
                    }

                    ModelsPage {
                        id: modelsPage
                    }
                }
                // StatusBar is removed/hidden as requested
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
        preferredHeight: Math.min(620, root.height - SentinelTheme.space4Xl)
        closePolicy: Popup.NoAutoClose

        property int step: 0
        readonly property var titles: [
            qsTr("Welcome to Sentinel"),
            qsTr("Privacy Philosophy"),
            qsTr("Choose Theme"),
            qsTr("Configure AI"),
            qsTr("Workspace Introduction"),
            qsTr("Finish")
        ]

        contentItem: ColumnLayout {
            spacing: SentinelTheme.spaceLg

            Label {
                Layout.fillWidth: true
                Layout.margins: SentinelTheme.spaceLg
                Layout.bottomMargin: 0
                text: onboardingModal.titles[onboardingModal.step]
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

                ColumnLayout {
                    spacing: SentinelTheme.spaceSm
                    Label {
                        Layout.fillWidth: true
                        text: qsTr("Sentinel is a local-first desktop assistant for chat, Brain, workspaces, controlled tasks, and notifications.")
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                    }
                    Flow {
                        Layout.fillWidth: true
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
                }

                ColumnLayout {
                    spacing: SentinelTheme.spaceSm
                    InfoRow { label: qsTr("Local-first"); value: qsTr("Settings, memory, chat history, notifications, diagnostics, and recovery data stay local."); Layout.fillWidth: true; valueMaximumLineCount: 3 }
                    InfoRow { label: qsTr("No telemetry"); value: qsTr("Telemetry is disabled by default and no analytics uploads are added."); Layout.fillWidth: true; valueMaximumLineCount: 3 }
                    InfoRow { label: qsTr("User control"); value: qsTr("Updates, exports, tasks, providers, and sensitive operations require visible user action."); Layout.fillWidth: true; valueMaximumLineCount: 3 }
                }

                Flow {
                    spacing: SentinelTheme.spaceSm
                    Repeater {
                        model: ["Sentinel Dark", "Midnight", "Aurora", "Graphite",
                                "Liquid Glass Dark", "Liquid Glass Light", "System Adaptive"]
                        SentinelButton {
                            required property string modelData
                            text: modelData
                            onClicked: root.viewModel.themeName = modelData
                        }
                    }
                }

                Flow {
                    spacing: SentinelTheme.spaceSm
                    Repeater {
                        model: ["Ollama", "LM Studio", "llama.cpp server", "OpenAI-compatible local endpoint"]
                        SentinelButton {
                            required property string modelData
                            text: modelData
                            onClicked: root.viewModel.onboardingAiProvider = modelData
                        }
                    }
                }

                ColumnLayout {
                    spacing: SentinelTheme.spaceSm
                    InfoRow { label: qsTr("Brain"); value: qsTr("Local memory, recall, context, summaries, and insights."); Layout.fillWidth: true; valueMaximumLineCount: 2 }
                    InfoRow { label: qsTr("Workspaces"); value: qsTr("Personal, Coding, Research, Writing, and Student scopes without folder scans."); Layout.fillWidth: true; valueMaximumLineCount: 2 }
                    InfoRow { label: qsTr("Tasks"); value: qsTr("Controlled task plans require approval and visible step-by-step progress."); Layout.fillWidth: true; valueMaximumLineCount: 2 }
                    InfoRow { label: qsTr("Notifications"); value: qsTr("Local in-app center with categories, pin, archive, read state, filtering, and search."); Layout.fillWidth: true; valueMaximumLineCount: 2 }
                }

                ColumnLayout {
                    spacing: SentinelTheme.spaceSm
                    InfoRow { label: qsTr("Use Case"); value: root.viewModel.onboardingUseCase; Layout.fillWidth: true }
                    InfoRow { label: qsTr("Theme"); value: root.viewModel.themeName; Layout.fillWidth: true }
                    InfoRow { label: qsTr("AI"); value: root.viewModel.onboardingAiProvider + qsTr(" / no downloads are started."); Layout.fillWidth: true; valueMaximumLineCount: 2 }
                    InfoRow { label: qsTr("Privacy"); value: qsTr("No telemetry, no hidden uploads, no silent updates, no hidden indexing, no hidden cloud activation."); Layout.fillWidth: true; valueMaximumLineCount: 3 }
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
                    text: onboardingModal.step < 5 ? qsTr("Next") : qsTr("Start")
                    onClicked: {
                        if (onboardingModal.step < 5) {
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
        sequence: "Esc"
        onActivated: {
            if (commandPalette.opened)
                commandPalette.close()
            else if (settingsModal.opened)
                settingsModal.close()
        }
    }
}
