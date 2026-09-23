import QtQuick
import QtQuick.Effects

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

    Image {
        anchors.centerIn: parent
        width: root.iconSize
        height: root.iconSize
        source: root.text ? "qrc:/icons/tabler/" + root.text + ".svg" : ""
        sourceSize.width: root.iconSize
        sourceSize.height: root.iconSize
        fillMode: Image.PreserveAspectFit
        layer.enabled: true
        layer.effect: MultiEffect {
            colorization: 1.0
            colorizationColor: root.color
        }
    }
}
