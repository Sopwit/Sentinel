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
        spacing: SentinelTheme.spaceLg

        Label {
            text: qsTr("Welcome to Sentinel")
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontDisplay
            font.bold: true
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("A calm, private AI assistant built directly into your desktop environment.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
