import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: presence
    required property var viewModel
    property bool compact: width < 620
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.032)
    border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.10)
    bracketColor: SentinelTheme.withAlpha(SentinelTheme.accent, 0.24)
    bracketSize: 12
    implicitHeight: compact ? 430 : 560

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: presence.compact ? SentinelTheme.spaceLg : SentinelTheme.space2Xl
        spacing: SentinelTheme.spaceSm

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Label {
                text: "CORE / SENTINEL"
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 2.6
            }

            Item {
                Layout.fillWidth: true
            }

            Label {
                text: "COHERENCE 0.974"
                color: SentinelTheme.accent
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 2.4
            }
        }

        Item {
            id: scene
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: presence.compact ? 300 : 420
            readonly property real safeWidth: Math.max(1, width)
            readonly property real safeHeight: Math.max(1, height)
            readonly property real safeSize: Math.max(1, Math.min(safeWidth, safeHeight))

            Rectangle {
                anchors.centerIn: parent
                width: Math.min(scene.safeWidth * 0.88, presence.compact ? 390 : 620)
                height: width
                radius: width / 2
                color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.038)
                border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.035)
            }

            SentinelOrb {
                viewModel: presence.viewModel
                compact: presence.compact
                width: Math.min(scene.safeWidth * 0.74, presence.compact ? 340 : 520)
                height: width
                anchors.centerIn: parent
            }

            Label {
                anchors.left: parent.left
                anchors.top: parent.top
                text: "NODE / 04A\nSYNCED"
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
                lineHeight: 1.35
            }

            Label {
                anchors.right: parent.right
                anchors.top: parent.top
                text: "LAT / 12ms\nTOOLS / " + (presence.viewModel.availableToolCount === undefined ? 0 : presence.viewModel.availableToolCount)
                color: SentinelTheme.textMuted
                horizontalAlignment: Text.AlignRight
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
                lineHeight: 1.35
            }

            Label {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                text: "PSI / 7.318"
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
            }

            Label {
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                text: "EXEC / DISABLED"
                color: SentinelTheme.textMuted
                horizontalAlignment: Text.AlignRight
                font.pixelSize: SentinelTheme.fontTiny
                font.letterSpacing: 1.8
            }
        }

        Label {
            Layout.fillWidth: true
            text: "\"I am listening across local channels. Nothing demands your attention.\""
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.78)
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font.pixelSize: SentinelTheme.fontBody
            font.weight: Font.Light
        }
    }
}
