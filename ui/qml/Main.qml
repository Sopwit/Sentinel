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
    color: "#071113"

    background: Rectangle {
        gradient: Gradient {
            GradientStop {
                position: 0.0
                color: "#0b2428"
            }
            GradientStop {
                position: 0.55
                color: "#071113"
            }
            GradientStop {
                position: 1.0
                color: "#03100f"
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 14

            Sidebar {
                Layout.preferredWidth: 244
                Layout.fillHeight: true
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 14

                HeaderBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 96
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: shellViewModel.currentPage === "Dashboard" ? 0
                                  : shellViewModel.currentPage === "Memory" ? 1 : 2

                    DashboardPage {}
                    MemoryPage {}
                    SettingsPage {}
                }
            }
        }

        StatusBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
        }
    }
}
