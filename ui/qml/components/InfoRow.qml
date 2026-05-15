import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

GridLayout {
    id: infoRow

    property string label: ""
    property string value: ""
    property bool compact: false

    Layout.fillWidth: true
    columns: infoRow.compact ? 1 : 2
    columnSpacing: SentinelTheme.spaceMd
    rowSpacing: SentinelTheme.spaceXs

    Label {
        text: infoRow.label
        color: SentinelTheme.textMuted
        font.pixelSize: SentinelTheme.fontBody
        Layout.preferredWidth: infoRow.compact ? -1 : 112
    }

    Label {
        Layout.fillWidth: true
        text: infoRow.value
        color: SentinelTheme.textPrimary
        font.pixelSize: SentinelTheme.fontBody
        wrapMode: Text.WordWrap
    }
}
