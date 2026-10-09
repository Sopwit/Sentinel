pragma ComponentBehavior: Bound

import QtQuick
import Sentinel.Desktop

Item {
    id: root
    property string text: ""
    property color color: "#8190a8"
    property font font
    property int horizontalAlignment: Text.AlignHCenter
    property int verticalAlignment: Text.AlignVCenter
    property int elide: Text.ElideNone
    readonly property int iconSize: Math.max(14, font.pixelSize > 0 ? font.pixelSize : 16)

    implicitWidth: iconSize
    implicitHeight: iconSize

    SentinelIcon {
        anchors.centerIn: parent
        name: root.text
        source: root.text ? "qrc:/icons/tabler/" + root.text + ".svg" : ""
        iconSize: root.iconSize
        tint: root.color
    }
}
