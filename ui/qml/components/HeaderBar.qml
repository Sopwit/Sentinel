import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: headerBar
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    property date now: new Date()
    readonly property int modeButtonWidth: compact ? 124 : 136
    readonly property string dashboardSubtitleText: qsTr("Chat through configured local providers.")
    readonly property string modeSubtitleText: pageSubtitle(headerBar.viewModel.currentPage)
    readonly property string subtitleText: headerBar.viewModel.currentPage === "Dashboard"
                                           ? headerBar.dashboardSubtitleText
                                           : headerBar.modeSubtitleText

    function pageSubtitle(pageName) {
        if (pageName === "Memory")
            return qsTr("Memory, recall, context, summaries, and continuity.")
        if (pageName === "Agents")
            return qsTr("Metadata-only agent planning and capability status.")
        if (pageName === "Settings")
            return qsTr("Floating local preferences and readiness controls.")
        return qsTr("Local Ollama chatbot. No cloud provider active.")
    }

    function greetingFor(dateValue) {
        const hour = dateValue.getHours()
        if (hour < 12)
            return qsTr("Good morning, Operator.")
        if (hour < 18)
            return qsTr("Good afternoon, Operator.")
        return qsTr("Good evening, Operator.")
    }

    color: "transparent"
    border.color: "transparent"
    showBrackets: false

    GridLayout {
        anchors.fill: parent
        anchors.leftMargin: headerBar.compact ? SentinelTheme.spaceMd : 0
        anchors.rightMargin: headerBar.compact ? SentinelTheme.spaceMd : SentinelTheme.spaceLg
        anchors.topMargin: SentinelTheme.spaceSm
        anchors.bottomMargin: SentinelTheme.spaceSm
        columns: headerBar.compact ? 1 : 2
        columnSpacing: SentinelTheme.spaceLg
        rowSpacing: SentinelTheme.spaceSm

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceXs

            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                Rectangle {
                    Layout.preferredWidth: 6
                    Layout.preferredHeight: 6
                    radius: 3
                    color: headerBar.modeAccent
                    opacity: 0.9
                }

                Label {
                text: qsTr("SENTINEL")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            Label {
                text: headerBar.viewModel.currentPage === "Dashboard" ? headerBar.greetingFor(headerBar.now)
                      : headerBar.viewModel.currentPage === "Memory" ? qsTr("Brain")
                      : headerBar.viewModel.currentPage
                color: SentinelTheme.textPrimary
                font.pixelSize: headerBar.compact ? SentinelTheme.fontTitle : SentinelTheme.fontTitle + 2
                font.weight: Font.Light
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Label {
                Layout.fillWidth: true
                text: headerBar.subtitleText
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                maximumLineCount: 1
                elide: Text.ElideRight
            }
        }

        RowLayout {
            Layout.alignment: headerBar.compact ? Qt.AlignLeft : Qt.AlignRight | Qt.AlignVCenter
            spacing: SentinelTheme.spaceSm

            Rectangle {
                visible: !headerBar.compact
                Layout.preferredWidth: 104
                Layout.preferredHeight: 28
                radius: 14
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.075)

                Label {
                    anchors.fill: parent
                    anchors.leftMargin: SentinelTheme.spaceSm
                    anchors.rightMargin: SentinelTheme.spaceSm
                    text: headerBar.viewModel.ollamaHealthStatus
                    color: SentinelTheme.textPlaceholder
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: SentinelTheme.fontSmall
                    elide: Text.ElideRight
                }
            }

            Button {
                id: modeButton
                Layout.preferredWidth: headerBar.modeButtonWidth
                Layout.preferredHeight: 28
                hoverEnabled: true
                focusPolicy: Qt.StrongFocus
                text: headerBar.viewModel.currentModeName
                onClicked: modePopup.open()

                contentItem: RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: SentinelTheme.spaceMd
                    anchors.rightMargin: SentinelTheme.spaceSm
                    spacing: SentinelTheme.spaceSm

                    Label {
                        Layout.fillWidth: true
                        text: modeButton.text
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontSmall
                        elide: Text.ElideRight
                    }

                    Label {
                        text: "\u25be"
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontTiny
                    }
                }

                background: Rectangle {
                    radius: 16
                    color: InteractionTokens.surfaceColor(modeButton.hovered, modeButton.down,
                                                           modePopup.opened, headerBar.modeAccent)
                    border.color: InteractionTokens.borderColor(modeButton.activeFocus, modeButton.hovered,
                                                                 modePopup.opened, headerBar.modeAccent)
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

                Popup {
                    id: modePopup
                    y: modeButton.height + SentinelTheme.spaceXs
                    width: headerBar.modeButtonWidth
                    modal: false
                    focus: true
                    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
                    padding: SentinelTheme.spaceXs
                    enter: Transition {
                        NumberAnimation {
                            property: "opacity"
                            from: 0.0
                            to: 1.0
                            duration: MotionTokens.menu
                            easing.type: MotionTokens.enter
                        }
                    }
                    exit: Transition {
                        NumberAnimation {
                            property: "opacity"
                            to: 0.0
                            duration: MotionTokens.fast
                            easing.type: MotionTokens.exit
                        }
                    }

                    background: Rectangle {
                        radius: SentinelTheme.radiusLg
                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.98)
                        border.color: SentinelTheme.withAlpha(headerBar.modeAccent, 0.18)
                    }

                    contentItem: ColumnLayout {
                        spacing: SentinelTheme.spaceXs

                        Repeater {
                            model: headerBar.viewModel.availableModes

                            Button {
                                id: modeOption
                                required property string modelData
                                Layout.fillWidth: true
                                Layout.preferredHeight: SentinelTheme.controlHeight
                                flat: true
                                hoverEnabled: true
                                text: modelData
                                onClicked: {
                                    headerBar.viewModel.setModeByName(modelData)
                                    modePopup.close()
                                }

                                contentItem: Label {
                                    text: modeOption.text
                                    color: modeOption.modelData === headerBar.viewModel.currentModeName
                                           ? SentinelTheme.textPrimary
                                           : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    verticalAlignment: Text.AlignVCenter
                                    elide: Text.ElideRight
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusMd
                                    color: modeOption.hovered
                                           ? SentinelTheme.withAlpha(headerBar.modeAccent, 0.10)
                                           : "transparent"
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Component.onCompleted: headerBar.now = new Date()
}
