import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ShellPanel {
    id: headerBar
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    property date now: new Date()
    readonly property string dashboardSubtitleText: qsTr("Chat through configured local providers.")
    readonly property string subtitleText: headerBar.viewModel.currentPage === "Settings"
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

        }
    }

    Component.onCompleted: headerBar.now = new Date()
}
