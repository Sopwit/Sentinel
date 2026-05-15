import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    property string title: ""
    property string subtitle: ""

    spacing: SentinelTheme.spaceXs

    Label {
        Layout.fillWidth: true
        text: parent.title
        color: SentinelTheme.textPrimary
        font.pixelSize: SentinelTheme.fontTitle
        font.bold: true
        wrapMode: Text.WordWrap
    }

    Label {
        Layout.fillWidth: true
        text: parent.subtitle
        color: SentinelTheme.textMuted
        font.pixelSize: SentinelTheme.fontBody
        wrapMode: Text.WordWrap
    }
}
