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
            text: qsTr("Select AI Provider")
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontDisplay
            font.bold: true
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Connect to local runtimes like Ollama or LM Studio, or cloud provider APIs.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
