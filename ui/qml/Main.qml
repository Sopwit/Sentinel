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
    readonly property bool compactLayout: root.width < 1080
    readonly property bool wideLayout: root.width >= SentinelTheme.breakpointWide
    readonly property int shellEntranceOffset: root.shellReady || MotionTokens.reduced(root.viewModel.currentModeName) ? 0 : 8
    readonly property int pageMotionOffset: MotionTokens.reduced(root.viewModel.currentModeName) ? 0 : 10
    readonly property int dockZoneHeight: (compactLayout ? 58 : 62)
                                          + (compactLayout ? SentinelTheme.spaceMd
                                                           : SentinelTheme.spaceXl)
                                          + SentinelTheme.spaceLg
    Component.onCompleted: Qt.callLater(function() {
        root.shellReady = true
    })

    function currentPageIndex() {
        if (root.viewModel.currentPage === "Dashboard")
            return 0
        if (root.viewModel.currentPage === "Memory")
            return 1
        if (root.viewModel.currentPage === "Agents")
            return 2
        if (root.viewModel.currentPage === "Settings")
            return 3
        return 0
    }

    function navigateToPage(pageName) {
        root.viewModel.currentPage = pageName
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
                    SettingsPage {
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex() === 3 ? 1.0 : 0.0
                        transform: Translate {
                            y: root.currentPageIndex() === 3 ? 0 : root.pageMotionOffset

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

            ChatPanel {
                viewModel: root.viewModel
                visible: !root.compactLayout && root.viewModel.currentPage === "Dashboard"
                Layout.preferredWidth: Math.min(SentinelTheme.rightPanelWidth, root.width * 0.32)
                Layout.fillHeight: true
                Layout.minimumHeight: 0
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
        onClicked: root.viewModel.currentPage = "Settings"

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
                                                   root.viewModel.currentPage === "Settings",
                                                   SentinelTheme.calmAccent)
            border.color: InteractionTokens.borderColor(settingsFab.activeFocus, settingsFab.hovered,
                                                         root.viewModel.currentPage === "Settings",
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
        onActivated: root.navigateToPage("Settings")
    }

    Shortcut {
        sequences: ["Ctrl+L", "Meta+L"]
        onActivated: root.focusChatComposer()
    }

    Shortcut {
        sequences: ["Ctrl+,", "Meta+,"]
        onActivated: root.navigateToPage("Settings")
    }

    Shortcut {
        sequence: "Esc"
        onActivated: {
            if (commandPalette.opened)
                commandPalette.close()
        }
    }
}
