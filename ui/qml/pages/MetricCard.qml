import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    id: metricCard

    property string label: ""
    property string value: ""

    Layout.fillWidth: true
    Layout.preferredHeight: 92
    radius: SentinelTheme.radiusXl
    color: SentinelTheme.metricSurface
    border.color: SentinelTheme.accentBorderSoft

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceMd
        spacing: SentinelTheme.spaceXs

        Label {
            text: metricCard.label
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
        }

        Label {
            text: metricCard.value
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontCard
            font.bold: true
        }
    }
}
