import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

RowLayout {
    id: dashboardPage
    required property var viewModel

    spacing: 14

    ShellPanel {
        Layout.fillWidth: true
        Layout.fillHeight: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16

            SectionTitle {
                title: "Operations Dashboard"
                subtitle: "Minimal state overview for the Sentinel Desktop Alpha shell."
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: 12
                rowSpacing: 12

                MetricCard {
                    label: "Core"
                    value: "Online"
                }
                MetricCard {
                    label: "Memory"
                    value: "Runtime"
                }
                MetricCard {
                    label: "Network"
                    value: "Disabled"
                }
            }

            ShellPanel {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#081719aa"

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 10

                    Label {
                        text: "Current Posture"
                        color: "#35f2c0"
                        font.pixelSize: 13
                        font.letterSpacing: 1.2
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "Mode: " + dashboardPage.viewModel.currentModeName
                              + ". The app exposes only local echo-provider chat, runtime memory, settings, and lightweight plugin/integration contracts."
                        color: "#d9fff4"
                        font.pixelSize: 16
                        lineHeight: 1.25
                        wrapMode: Text.WordWrap
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }
            }
        }
    }

    ChatPanel {
        viewModel: dashboardPage.viewModel
        Layout.preferredWidth: 410
        Layout.fillHeight: true
    }
}
