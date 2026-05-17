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
    title: "Sentinel Desktop Alpha"
    color: SentinelTheme.backgroundBase
    property var viewModel: shellViewModel
    readonly property bool compactLayout: root.width < 1080
    readonly property bool wideLayout: root.width >= SentinelTheme.breakpointWide

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

    background: Atmosphere {
        modeName: root.viewModel.currentModeName
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: SentinelTheme.pageMargin(root.width)
        anchors.rightMargin: SentinelTheme.pageMargin(root.width)
        anchors.topMargin: root.compactLayout ? SentinelTheme.spaceMd : SentinelTheme.spaceXl
        anchors.bottomMargin: root.compactLayout ? SentinelTheme.spaceMd : SentinelTheme.spaceXl
        spacing: root.compactLayout ? SentinelTheme.spaceSm : SentinelTheme.spaceLg

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
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0
                    clip: true
                    currentIndex: root.currentPageIndex()

                    DashboardPage {
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex() === 0 ? 1.0 : 0.0

                        Behavior on opacity {
                            NumberAnimation {
                                duration: SentinelTheme.durationNormal
                                easing.type: SentinelTheme.easingStandard
                            }
                        }
                    }
                    MemoryPage {
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex() === 1 ? 1.0 : 0.0

                        Behavior on opacity {
                            NumberAnimation {
                                duration: SentinelTheme.durationNormal
                                easing.type: SentinelTheme.easingStandard
                            }
                        }
                    }
                    AgentsPage {
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex() === 2 ? 1.0 : 0.0

                        Behavior on opacity {
                            NumberAnimation {
                                duration: SentinelTheme.durationNormal
                                easing.type: SentinelTheme.easingStandard
                            }
                        }
                    }
                    SettingsPage {
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex() === 3 ? 1.0 : 0.0

                        Behavior on opacity {
                            NumberAnimation {
                                duration: SentinelTheme.durationNormal
                                easing.type: SentinelTheme.easingStandard
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
        z: 30
    }

    Button {
        id: settingsFab
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.rightMargin: SentinelTheme.space2Xl
        anchors.bottomMargin: SentinelTheme.space2Xl
        width: 52
        height: 52
        focusPolicy: Qt.StrongFocus
        hoverEnabled: true
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
            color: settingsFab.down ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.26)
                                   : settingsFab.hovered
                                     ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
                                     : SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.88)
            border.color: settingsFab.activeFocus ? SentinelTheme.focusBorder
                                                  : SentinelTheme.withAlpha(SentinelTheme.accent, 0.35)
            border.width: 1
        }
    }
}
