import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1200
    height: 780
    minimumWidth: 1000
    minimumHeight: 660
    visible: true
    title: "Sentinel Desktop Alpha"
    color: SentinelTheme.backgroundBase
    property var viewModel: shellViewModel

    background: Rectangle {
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: SentinelTheme.backgroundRaised
            }
            GradientStop {
                position: 0.55
                color: SentinelTheme.backgroundBase
            }
            GradientStop {
                position: 1.0
                color: SentinelTheme.backgroundDeep
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: SentinelTheme.spaceMd

            Sidebar {
                viewModel: root.viewModel
                Layout.preferredWidth: 244
                Layout.fillHeight: true
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: SentinelTheme.spaceMd

                HeaderBar {
                    viewModel: root.viewModel
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: root.viewModel.currentPage === "Dashboard" ? 0
                                  : root.viewModel.currentPage === "Memory" ? 1 : 2

                    DashboardPage {
                        viewModel: root.viewModel
                    }
                    MemoryPage {
                        viewModel: root.viewModel
                    }
                    SettingsPage {
                        viewModel: root.viewModel
                    }
                }
            }
        }

        StatusBar {
            viewModel: root.viewModel
            Layout.fillWidth: true
            Layout.preferredHeight: 46
        }
    }
}
