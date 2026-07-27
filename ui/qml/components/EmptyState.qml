import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root

    property string icon: "\u{1F50D}"
    property string title: qsTr("Nothing here yet")
    property string description: ""
    property string actionLabel: ""
    property string modeName: ""
    property bool compact: false

    signal actionTriggered()

    implicitWidth: parent ? parent.width : 0
    implicitHeight: layout.implicitHeight + SentinelTheme.space4Xl * 2

    RowLayout {
        id: layout
        anchors.centerIn: parent
        width: Math.min(parent.width - SentinelTheme.space4Xl, 380)
        spacing: SentinelTheme.spaceLg
        opacity: 0.7

        ColumnLayout {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            spacing: root.compact ? SentinelTheme.spaceSm : SentinelTheme.spaceMd

            Text {
                Layout.alignment: Qt.AlignHCenter
                text: root.icon
                font.pixelSize: root.compact ? 36 : 48
            }

            Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
                text: root.title
                color: SentinelTheme.textPrimary
                font.pixelSize: root.compact ? SentinelTheme.fontCard : SentinelTheme.fontTitle
                font.weight: Font.Medium
                wrapMode: Text.WordWrap
            }

            Label {
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignHCenter
                horizontalAlignment: Text.AlignHCenter
                visible: root.description.length > 0
                text: root.description
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontBody
                wrapMode: Text.WordWrap
                lineHeight: 1.4
            }

            SentinelButton {
                Layout.alignment: Qt.AlignHCenter
                Layout.topMargin: SentinelTheme.spaceMd
                visible: root.actionLabel.length > 0
                text: root.actionLabel
                implicitHeight: 36
                onClicked: root.actionTriggered()
            }
        }
    }
}
