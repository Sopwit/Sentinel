import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Rectangle {
    property string label: ""
    property string value: ""

    Layout.fillWidth: true
    Layout.preferredHeight: 92
    radius: 18
    color: "#12292dcc"
    border.color: "#35f2c02d"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 5

        Label {
            text: parent.parent.label
            color: "#82aaa1"
            font.pixelSize: 12
        }

        Label {
            text: parent.parent.value
            color: "#d9fff4"
            font.pixelSize: 18
            font.bold: true
        }
    }
}
