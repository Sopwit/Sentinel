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

    property string activePage: "Dashboard"
    readonly property color panelColor: "#0d1d20e8"
    readonly property color panelStroke: "#35f2c044"
    readonly property color textPrimary: "#d9fff4"
    readonly property color textMuted: "#82aaa1"
    readonly property color accent: "#35f2c0"

    function modeIndex() {
        return shellViewModel.availableModes.indexOf(shellViewModel.currentModeName)
    }

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

        Rectangle {
            anchors.fill: parent
            opacity: 0.08
            color: "transparent"
            border.color: root.accent
            border.width: 1
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
                    currentIndex: root.activePage === "Dashboard" ? 0 : root.activePage === "Memory" ? 1 : 2

                    DashboardPage {}
                    MemoryPage {}
                    SettingsPage {}
                }
            }
        }

        StatusFooter {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
        }
    }

    component ShellPanel: Rectangle {
        radius: 22
        color: root.panelColor
        border.color: root.panelStroke
        border.width: 1
    }

    component SectionTitle: ColumnLayout {
        property string title: ""
        property string subtitle: ""

        spacing: 4

        Label {
            text: parent.title
            color: root.textPrimary
            font.pixelSize: 22
            font.bold: true
        }

        Label {
            Layout.fillWidth: true
            text: parent.subtitle
            color: root.textMuted
            font.pixelSize: 13
            wrapMode: Text.WordWrap
        }
    }

    component Sidebar: ShellPanel {
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 18

            ColumnLayout {
                spacing: 3

                Label {
                    text: "SENTINEL"
                    color: root.textPrimary
                    font.pixelSize: 24
                    font.bold: true
                    font.letterSpacing: 4
                }

                Label {
                    text: shellViewModel.configurationProfile
                    color: root.accent
                    font.pixelSize: 12
                    font.letterSpacing: 1.2
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: root.panelStroke
            }

            Repeater {
                model: ["Dashboard", "Memory", "Settings"]

                Button {
                    Layout.fillWidth: true
                    text: modelData
                    flat: true
                    highlighted: root.activePage === modelData
                    onClicked: root.activePage = modelData

                    contentItem: Text {
                        text: parent.text
                        color: parent.highlighted ? "#06110f" : root.textPrimary
                        font.pixelSize: 14
                        font.bold: parent.highlighted
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: 14
                        color: parent.highlighted ? root.accent : "#10232688"
                        border.color: parent.highlighted ? root.accent : "#35f2c022"
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 8

                Label {
                    text: "Provider"
                    color: root.textMuted
                    font.pixelSize: 11
                    font.letterSpacing: 1.1
                }

                Label {
                    text: shellViewModel.providerName
                    color: root.textPrimary
                    font.pixelSize: 14
                }

                Label {
                    text: "Theme: " + shellViewModel.themeName
                    color: root.textMuted
                    font.pixelSize: 12
                }
            }
        }
    }

    component HeaderBar: ShellPanel {
        RowLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 18

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 4

                Label {
                    text: shellViewModel.currentModeName
                    color: root.textPrimary
                    font.pixelSize: 27
                    font.bold: true
                }

                Label {
                    text: "Desktop shell bridge active. Local-first foundation, no network provider configured."
                    color: root.textMuted
                    font.pixelSize: 13
                }
            }

            ComboBox {
                Layout.preferredWidth: 230
                model: shellViewModel.availableModes
                currentIndex: root.modeIndex()
                onActivated: shellViewModel.setModeByName(currentText)
            }
        }
    }

    component DashboardPage: RowLayout {
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
                            color: root.accent
                            font.pixelSize: 13
                            font.letterSpacing: 1.2
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "Mode: " + shellViewModel.currentModeName
                                  + ". The app exposes only local fake-provider chat, runtime memory, settings placeholders, and lightweight plugin/integration contracts."
                            color: root.textPrimary
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
            Layout.preferredWidth: 410
            Layout.fillHeight: true
        }
    }

    component MetricCard: Rectangle {
        property string label: ""
        property string value: ""

        Layout.fillWidth: true
        Layout.preferredHeight: 92
        radius: 18
        color: "#12292dcc"
        border.color: "#35f2c02d"

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 5

            Label {
                text: parent.parent.label
                color: root.textMuted
                font.pixelSize: 12
            }

            Label {
                text: parent.parent.value
                color: root.textPrimary
                font.pixelSize: 18
                font.bold: true
            }
        }
    }

    component ChatPanel: ShellPanel {
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            SectionTitle {
                title: "AI Bridge"
                subtitle: "FakeProvider only. No API calls or network access."
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 10
                model: shellViewModel.chatMessages

                delegate: Rectangle {
                    width: ListView.view.width
                    radius: 14
                    color: modelData.indexOf("You:") === 0 ? "#173331" : "#102326"
                    border.color: modelData.indexOf("You:") === 0 ? "#35f2c044" : "#7be8c733"
                    implicitHeight: messageText.implicitHeight + 22

                    Text {
                        id: messageText
                        anchors.fill: parent
                        anchors.margins: 11
                        text: modelData
                        color: root.textPrimary
                        wrapMode: Text.WordWrap
                        font.pixelSize: 13
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                TextField {
                    id: chatInput
                    Layout.fillWidth: true
                    placeholderText: "Send a local test prompt"
                    color: root.textPrimary
                    placeholderTextColor: "#648c83"
                    onAccepted: sendButton.clicked()

                    background: Rectangle {
                        radius: 14
                        color: "#081719"
                        border.color: root.panelStroke
                    }
                }

                Button {
                    id: sendButton
                    text: "Send"
                    enabled: chatInput.text.trim().length > 0
                    onClicked: {
                        shellViewModel.sendMessage(chatInput.text)
                        chatInput.clear()
                    }
                }
            }
        }
    }

    component MemoryPage: ShellPanel {
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 14

            SectionTitle {
                title: "Runtime Memory"
                subtitle: "Development-only key-value store. SQLite storage is intentionally deferred."
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                TextField {
                    id: memoryKey
                    Layout.preferredWidth: 220
                    placeholderText: "key"
                    color: root.textPrimary
                    placeholderTextColor: "#648c83"
                }

                TextField {
                    id: memoryValue
                    Layout.fillWidth: true
                    placeholderText: "value"
                    color: root.textPrimary
                    placeholderTextColor: "#648c83"
                }

                Button {
                    text: "Store"
                    enabled: memoryKey.text.trim().length > 0
                    onClicked: {
                        shellViewModel.remember(memoryKey.text, memoryValue.text)
                        memoryKey.clear()
                        memoryValue.clear()
                    }
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 8
                model: shellViewModel.memoryEntries

                delegate: Rectangle {
                    width: ListView.view.width
                    radius: 12
                    color: "#102326aa"
                    border.color: "#35f2c022"
                    implicitHeight: memoryText.implicitHeight + 18

                    Text {
                        id: memoryText
                        anchors.fill: parent
                        anchors.margins: 9
                        text: modelData
                        color: root.textPrimary
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }

    component SettingsPage: ShellPanel {
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 14

            SectionTitle {
                title: "Settings Foundation"
                subtitle: "Runtime settings abstraction only. Persistent settings can be added behind ISettingsStore later."
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: 12
                rowSpacing: 12

                Label {
                    text: "Theme"
                    color: root.textMuted
                }

                TextField {
                    Layout.fillWidth: true
                    text: shellViewModel.themeName
                    color: root.textPrimary
                    onEditingFinished: shellViewModel.setThemeName(text)
                }

                Label {
                    text: "Config Profile"
                    color: root.textMuted
                }

                TextField {
                    Layout.fillWidth: true
                    text: shellViewModel.configurationProfile
                    color: root.textPrimary
                    onEditingFinished: shellViewModel.setConfigurationProfile(text)
                }
            }

            Label {
                Layout.fillWidth: true
                text: "Future settings should remain local-first and pass through AppSettings rather than being implemented in QML."
                color: root.textMuted
                wrapMode: Text.WordWrap
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    component StatusFooter: ShellPanel {
        radius: 16

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            spacing: 14

            Label {
                text: "Status: Local Alpha"
                color: root.textPrimary
                font.pixelSize: 12
            }

            Label {
                text: "Mode: " + shellViewModel.currentModeName
                color: root.textMuted
                font.pixelSize: 12
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: "Provider: " + shellViewModel.providerName
                color: root.textMuted
                font.pixelSize: 12
            }
        }
    }
}
