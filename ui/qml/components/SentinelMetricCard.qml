import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: card
    property string label: ""
    property string value: ""
    property string unit: ""
    property string trend: ""

    implicitHeight: 124
    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)
    border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.085)
    bracketColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.18)
    bracketSize: 8

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceXs

        RowLayout {
            Layout.fillWidth: true

            Rectangle {
                Layout.preferredWidth: 5
                Layout.preferredHeight: 5
                radius: 3
                color: SentinelTheme.accent
                opacity: 0.75
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: card.trend
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
            }
        }

        Item {
            Layout.fillHeight: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceXs

            Label {
                text: card.value
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontHeader
                font.weight: Font.Light
            }

            Label {
                text: card.unit
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                Layout.alignment: Qt.AlignBottom
                Layout.bottomMargin: 4
            }
        }

        Label {
            Layout.fillWidth: true
            text: card.label.toUpperCase()
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontTiny
            font.letterSpacing: 2.0
            elide: Text.ElideRight
        }
    }
}
