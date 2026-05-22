import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: telemetry
    property string label: ""
    property string value: ""
    property string detail: ""
    property color accent: SentinelTheme.accent
    property bool alignRight: false

    implicitWidth: 178
    implicitHeight: detail.length > 0 ? 78 : 58
    radius: SentinelTheme.radiusLg
    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.035)
    border.color: SentinelTheme.withAlpha(accent, 0.09)
    showBrackets: false
    bracketSize: 7
    bracketColor: SentinelTheme.withAlpha(accent, 0.22)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceMd
        spacing: SentinelTheme.spaceXs

        Label {
            Layout.fillWidth: true
            text: telemetry.label
            color: SentinelTheme.textMuted
            horizontalAlignment: telemetry.alignRight ? Text.AlignRight : Text.AlignLeft
            font.pixelSize: SentinelTheme.fontTiny
            font.letterSpacing: 2.4
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            text: telemetry.value
            color: telemetry.accent
            horizontalAlignment: telemetry.alignRight ? Text.AlignRight : Text.AlignLeft
            font.pixelSize: SentinelTheme.fontSmall
            font.letterSpacing: 1.4
            elide: Text.ElideRight
        }

        Label {
            Layout.fillWidth: true
            visible: telemetry.detail.length > 0
            text: telemetry.detail
            color: SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.78)
            horizontalAlignment: telemetry.alignRight ? Text.AlignRight : Text.AlignLeft
            font.pixelSize: SentinelTheme.fontTiny
            elide: Text.ElideRight
        }
    }
}
