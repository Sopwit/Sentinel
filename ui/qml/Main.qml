import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1200
    height: 780
    minimumWidth: 760
    minimumHeight: 660
    visible: true
    title: "Sentinel Desktop Alpha"
    color: SentinelTheme.backgroundBase
    property var viewModel: shellViewModel
    readonly property bool compactLayout: root.width < 900
    readonly property bool wideLayout: root.width >= SentinelTheme.breakpointWide
    readonly property int currentPageIndex: root.viewModel.currentPage === "Dashboard" ? 0
                                            : root.viewModel.currentPage === "Memory" ? 1 : 2

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
            spacing: root.compactLayout ? SentinelTheme.spaceMd : SentinelTheme.space2Xl

            Sidebar {
                viewModel: root.viewModel
                compact: root.compactLayout
                Layout.preferredWidth: root.compactLayout ? SentinelTheme.sidebarCompactWidth : SentinelTheme.sidebarNormalWidth
                Layout.fillHeight: true
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: root.compactLayout ? SentinelTheme.spaceSm : SentinelTheme.spaceLg

                HeaderBar {
                    viewModel: root.viewModel
                    compact: root.compactLayout
                    Layout.fillWidth: true
                    Layout.preferredHeight: root.compactLayout ? 108 : 116
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    currentIndex: root.currentPageIndex

                    DashboardPage {
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex === 0 ? 1.0 : 0.0

                        Behavior on opacity {
                            NumberAnimation {
                                duration: SentinelTheme.durationNormal
                                easing.type: SentinelTheme.easingStandard
                            }
                        }
                    }
                    MemoryPage {
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex === 1 ? 1.0 : 0.0

                        Behavior on opacity {
                            NumberAnimation {
                                duration: SentinelTheme.durationNormal
                                easing.type: SentinelTheme.easingStandard
                            }
                        }
                    }
                    SettingsPage {
                        viewModel: root.viewModel
                        opacity: root.currentPageIndex === 2 ? 1.0 : 0.0

                        Behavior on opacity {
                            NumberAnimation {
                                duration: SentinelTheme.durationNormal
                                easing.type: SentinelTheme.easingStandard
                            }
                        }
                    }
                }
            }
        }

        SentinelDock {
            viewModel: root.viewModel
            compact: root.compactLayout
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(root.width - SentinelTheme.space4Xl, root.compactLayout ? 320 : 390)
            Layout.preferredHeight: implicitHeight
        }
    }
}
