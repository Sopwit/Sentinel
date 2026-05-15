pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: sidebar
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    color: "transparent"
    border.color: "transparent"
    radius: 0
    showBrackets: false

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.045)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: SentinelTheme.spaceXs
        anchors.rightMargin: SentinelTheme.spaceXs
        anchors.topMargin: SentinelTheme.spaceSm
        anchors.bottomMargin: SentinelTheme.spaceSm
        spacing: SentinelTheme.spaceLg

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 34
            Layout.preferredHeight: 34
            radius: 17
            color: SentinelTheme.withAlpha(sidebar.modeAccent, 0.10)
            border.color: SentinelTheme.withAlpha(sidebar.modeAccent, 0.22)

            Label {
                anchors.centerIn: parent
                text: "S"
                color: sidebar.modeAccent
                font.pixelSize: SentinelTheme.fontControl
                font.weight: Font.Light
            }
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: SentinelTheme.withAlpha(sidebar.modeAccent, 0.075)
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Rectangle {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 7
                Layout.preferredHeight: 7
                radius: 4
                color: sidebar.modeAccent
                opacity: 0.88
            }

            Label {
                Layout.fillWidth: true
                text: "LOCAL"
                color: SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.74)
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.6
                horizontalAlignment: Text.AlignHCenter
                elide: Text.ElideRight
            }
        }
    }
}
