import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    property string title: ""
    property string subtitle: ""

    spacing: 4

    Label {
        text: parent.title
        color: "#d9fff4"
        font.pixelSize: 22
        font.bold: true
    }

    Label {
        Layout.fillWidth: true
        text: parent.subtitle
        color: "#82aaa1"
        font.pixelSize: 13
        wrapMode: Text.WordWrap
    }
}
