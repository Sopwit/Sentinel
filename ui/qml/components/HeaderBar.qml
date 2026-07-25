import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ShellPanel {
    id: headerBar
    property var viewModel: null
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(headerBar.viewModel ? headerBar.viewModel.currentModeName : "Sentinel")
    property date now: new Date()
    readonly property string dashboardSubtitleText: qsTr("Chat through configured local providers.")
    readonly property string subtitleText: (headerBar.viewModel && headerBar.viewModel.currentPage === "Settings")
                                           ? qsTr("Floating local preferences and readiness controls.")
                                           : headerBar.dashboardSubtitleText

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
                text: headerBar.greetingFor(headerBar.now)
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

            SentinelButton {
                implicitWidth: 32
                implicitHeight: 32
                flat: true
                text: "\uD83D\uDCAC"
                font.pixelSize: 16
                tooltipText: qsTr("Toggle System Tray Companion Chat")
                onClicked: {
                    if (headerBar.viewModel) {
                        headerBar.viewModel.toggleCompanionChat()
                    }
                }
            }

            Rectangle {
                id: dndIndicator
                visible: headerBar.viewModel && headerBar.viewModel.dndEnabled
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28
                radius: 14
                color: SentinelTheme.withAlpha("#e74c3c", 0.12)

                SentinelButton {
                    anchors.centerIn: parent
                    implicitWidth: 24
                    implicitHeight: 24
                    flat: true
                    text: "\uD83D\uDD07"
                    font.pixelSize: 12
                    tooltipText: "Do Not Disturb is on. Click to disable."
                    Accessible.name: "Disable do not disturb"
                    onClicked: {
                        if (headerBar.viewModel) headerBar.viewModel.dndEnabled = false
                    }
                }
            }

            Item {
                id: notifBellArea
                Layout.preferredWidth: 36
                Layout.preferredHeight: 36

                SentinelButton {
                    id: notifBellButton
                    anchors.centerIn: parent
                    implicitWidth: 32
                    implicitHeight: 32
                    flat: true
                    text: {
                        if (headerBar.viewModel && headerBar.viewModel.dndEnabled) return "\uD83D\uDD07"
                        return "\uD83D\uDD14"
                    }
                    font.pixelSize: 16
                    tooltipText: headerBar.viewModel && headerBar.viewModel.dndEnabled ? "Do Not Disturb is on" : "Open notifications"
                    Accessible.name: headerBar.viewModel && headerBar.viewModel.dndEnabled ? "Do not disturb is on, click to toggle" : "Open notifications"
                    onClicked: {
                        if (headerBar.viewModel) {
                            if (headerBar.viewModel.dndEnabled) {
                                headerBar.viewModel.dndEnabled = false
                            } else {
                                headerBar.viewModel.toggleNotificationCenter()
                            }
                        }
                    }
                }

                Rectangle {
                    id: notifBadge
                    anchors.top: parent.top
                    anchors.topMargin: 2
                    anchors.right: parent.right
                    anchors.rightMargin: 2
                    width: 16
                    height: 16
                    radius: 8
                    color: "#e74c3c"
                    visible: count > 0

                    property int count: headerBar.viewModel ? headerBar.viewModel.unreadNotificationCount : 0

                    Text {
                        anchors.centerIn: parent
                        text: parent.count > 99 ? "99+" : parent.count.toString()
                        font.pixelSize: 9
                        font.bold: true
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

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
                    text: headerBar.viewModel ? headerBar.viewModel.ollamaHealthStatus : ""
                    color: SentinelTheme.textPlaceholder
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: SentinelTheme.fontSmall
                    elide: Text.ElideRight
                }
            }

        }
    }

    Component.onCompleted: headerBar.now = new Date()
}
