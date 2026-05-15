import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 1180
    height: 760
    minimumWidth: 980
    minimumHeight: 640
    visible: true
    title: "Sentinel Desktop Alpha"
    color: "#071113"

    property string activePage: "Dashboard"

    background: Rectangle {
        color: "#071113"

        Rectangle {
            anchors.fill: parent
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#0b1f24" }
                GradientStop { position: 0.52; color: "#071113" }
                GradientStop { position: 1.0; color: "#04100f" }
            }
        }

        Canvas {
            anchors.fill: parent
            opacity: 0.18
            onPaint: {
                const ctx = getContext("2d")
                ctx.strokeStyle = "#35f2c033"
                ctx.lineWidth = 1
                for (let x = 0; x < width; x += 42) {
                    ctx.beginPath()
                    ctx.moveTo(x, 0)
                    ctx.lineTo(x, height)
                    ctx.stroke()
                }
                for (let y = 0; y < height; y += 42) {
                    ctx.beginPath()
                    ctx.moveTo(0, y)
                    ctx.lineTo(width, y)
                    ctx.stroke()
                }
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 18

        Rectangle {
            Layout.preferredWidth: 232
            Layout.fillHeight: true
            radius: 24
            color: "#0d1d20dd"
            border.color: "#35f2c044"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 18
                spacing: 18

                ColumnLayout {
                    spacing: 2
                    Label {
                        text: "SENTINEL"
                        color: "#d9fff4"
                        font.pixelSize: 24
                        font.bold: true
                        font.letterSpacing: 4
                    }
                    Label {
                        text: "Desktop Alpha"
                        color: "#7be8c7"
                        font.pixelSize: 12
                        font.letterSpacing: 1.4
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#35f2c055"
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
                            color: parent.highlighted ? "#06110f" : "#bcefe0"
                            font.pixelSize: 14
                            font.bold: parent.highlighted
                            horizontalAlignment: Text.AlignLeft
                            verticalAlignment: Text.AlignVCenter
                        }

                        background: Rectangle {
                            radius: 14
                            color: parent.highlighted ? "#35f2c0" : "transparent"
                            border.color: parent.highlighted ? "#35f2c0" : "#35f2c022"
                        }
                    }
                }

                Item { Layout.fillHeight: true }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Label {
                        text: "Provider"
                        color: "#648c83"
                        font.pixelSize: 11
                        font.letterSpacing: 1.1
                    }
                    Label {
                        text: appController.providerName
                        color: "#d9fff4"
                        font.pixelSize: 14
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 18

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 98
                radius: 24
                color: "#102326cc"
                border.color: "#35f2c044"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 16

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4
                        Label {
                            text: modeManager.currentModeName
                            color: "#d9fff4"
                            font.pixelSize: 26
                            font.bold: true
                        }
                        Label {
                            text: "Native desktop-first AI operating layer foundation"
                            color: "#82aaa1"
                            font.pixelSize: 13
                        }
                    }

                    ComboBox {
                        id: modeSelector
                        Layout.preferredWidth: 230
                        model: modeManager.availableModes
                        currentIndex: model.indexOf(modeManager.currentModeName)
                        onActivated: modeManager.setModeByName(currentText)
                    }
                }
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

    component Panel: Rectangle {
        radius: 24
        color: "#0d1d20dd"
        border.color: "#35f2c03d"
        border.width: 1
    }

    component DashboardPage: RowLayout {
        spacing: 18

        Panel {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 14

                Label {
                    text: "Dashboard"
                    color: "#d9fff4"
                    font.pixelSize: 22
                    font.bold: true
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: 3
                    columnSpacing: 12
                    rowSpacing: 12

                    Repeater {
                        model: [
                            ["Core", "Online"],
                            ["Memory", "Runtime only"],
                            ["Network", "Disabled"]
                        ]

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 92
                            radius: 18
                            color: "#12292dcc"
                            border.color: "#35f2c02d"

                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 14
                                spacing: 4
                                Label {
                                    text: modelData[0]
                                    color: "#82aaa1"
                                    font.pixelSize: 12
                                }
                                Label {
                                    text: modelData[1]
                                    color: "#d9fff4"
                                    font.pixelSize: 18
                                    font.bold: true
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 18
                    color: "#081719aa"
                    border.color: "#35f2c026"

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 10

                        Label {
                            text: "Mode Profile"
                            color: "#7be8c7"
                            font.pixelSize: 13
                            font.letterSpacing: 1.2
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "Current operating posture: " + modeManager.currentModeName + ". Advanced automation, voice, cloud sync, and semantic memory are intentionally absent in this alpha foundation."
                            color: "#bcefe0"
                            wrapMode: Text.WordWrap
                            font.pixelSize: 16
                            lineHeight: 1.25
                        }

                        Item { Layout.fillHeight: true }
                    }
                }
            }
        }

        ChatPanel {
            Layout.preferredWidth: 390
            Layout.fillHeight: true
        }
    }

    component ChatPanel: Panel {
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 18
            spacing: 12

            Label {
                text: "AI Bridge"
                color: "#d9fff4"
                font.pixelSize: 20
                font.bold: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 10
                model: appController.chatMessages

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
                        color: "#d9fff4"
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
                    placeholderText: "Send a test prompt"
                    color: "#d9fff4"
                    placeholderTextColor: "#648c83"
                    onAccepted: sendButton.clicked()
                    background: Rectangle {
                        radius: 14
                        color: "#081719"
                        border.color: "#35f2c044"
                    }
                }

                Button {
                    id: sendButton
                    text: "Send"
                    enabled: chatInput.text.trim().length > 0
                    onClicked: {
                        appController.sendMessage(chatInput.text)
                        chatInput.clear()
                    }
                }
            }
        }
    }

    component MemoryPage: Panel {
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 14

            Label {
                text: "Memory Placeholder"
                color: "#d9fff4"
                font.pixelSize: 22
                font.bold: true
            }

            Label {
                text: "Runtime key-value memory only. SQLite and semantic memory are future phases."
                color: "#82aaa1"
                font.pixelSize: 14
            }

            RowLayout {
                Layout.fillWidth: true
                TextField {
                    id: memoryKey
                    Layout.preferredWidth: 220
                    placeholderText: "key"
                }
                TextField {
                    id: memoryValue
                    Layout.fillWidth: true
                    placeholderText: "value"
                }
                Button {
                    text: "Store"
                    enabled: memoryKey.text.trim().length > 0
                    onClicked: {
                        appController.remember(memoryKey.text, memoryValue.text)
                        memoryKey.clear()
                        memoryValue.clear()
                    }
                }
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: appController.memoryEntries
                delegate: Label {
                    width: ListView.view.width
                    text: modelData
                    color: "#d9fff4"
                    font.pixelSize: 14
                    padding: 10
                }
            }
        }
    }

    component SettingsPage: Panel {
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 22
            spacing: 12

            Label {
                text: "Settings Placeholder"
                color: "#d9fff4"
                font.pixelSize: 22
                font.bold: true
            }

            Label {
                Layout.fillWidth: true
                text: "Provider configuration, local storage settings, privacy controls, and device profiles will be added after the desktop alpha foundation is stable."
                color: "#bcefe0"
                wrapMode: Text.WordWrap
                font.pixelSize: 15
            }

            Item { Layout.fillHeight: true }
        }
    }
}
