import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root
    required property var viewModel
    property color brandAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

    implicitHeight: layout.implicitHeight

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: SentinelTheme.spaceMd

        Label {
            text: qsTr("You're ready!")
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontDisplay
            font.bold: true
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Sentinel setup is complete. Click finish to open your workspace.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
