// Lucide Icons — ISC License: https://lucide.dev/license
import QtQuick
import QtQuick.Effects
import Sentinel.Desktop

Image {
    id: root
    required property string name
    property color tint: SentinelTheme.textPrimary
    property int iconSize: 18
    source: "qrc:/icons/lucide/" + name + ".svg"
    sourceSize.width: iconSize
    sourceSize.height: iconSize
    width: iconSize
    height: iconSize
    fillMode: Image.PreserveAspectFit
    layer.enabled: true
    layer.effect: MultiEffect { colorization: 1.0; colorizationColor: root.tint }
}
