// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import Sentinel.Desktop

Item {
    id: root
    required property string name
    property color tint: SentinelTheme.textPrimary
    property int iconSize: 18
    property url source: "qrc:/icons/lucide/" + name + ".svg"
    implicitWidth: iconSize
    implicitHeight: iconSize
    width: iconSize
    height: iconSize
    // Public Controls icon tint uses the image alpha mask, including software rendering.
    ToolButton {
        anchors.fill: parent
        enabled: false
        focusPolicy: Qt.NoFocus
        Accessible.ignored: true
        padding: 0
        background: null
        display: AbstractButton.IconOnly
        icon.source: root.source
        icon.color: root.tint
        icon.width: root.iconSize
        icon.height: root.iconSize
        palette.disabled.buttonText: root.tint
    }
}
