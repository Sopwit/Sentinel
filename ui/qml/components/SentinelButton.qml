import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1.0 : 0.48
    implicitHeight: SentinelTheme.controlHeight

    contentItem: Text {
        text: control.text
        color: control.down || control.hovered || control.activeFocus ? SentinelTheme.textOnAccent : SentinelTheme.textPrimary
        font.pixelSize: SentinelTheme.fontControl
        font.bold: control.hovered || control.activeFocus
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight

        Behavior on color {
            ColorAnimation {
                duration: SentinelTheme.durationFast
                easing.type: SentinelTheme.easingStandard
            }
        }
    }

    background: Rectangle {
        radius: SentinelTheme.radiusMd
        color: control.down || control.hovered || control.activeFocus ? SentinelTheme.accentHover : SentinelTheme.surfaceSoft
        border.color: control.activeFocus ? SentinelTheme.focusBorder : control.hovered ? SentinelTheme.accentBorder : SentinelTheme.accentBorderSubtle
        border.width: 1

        Behavior on color {
            ColorAnimation {
                duration: SentinelTheme.durationFast
                easing.type: SentinelTheme.easingStandard
            }
        }

        Behavior on border.color {
            ColorAnimation {
                duration: SentinelTheme.durationFast
                easing.type: SentinelTheme.easingStandard
            }
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: SentinelTheme.durationFast
            easing.type: SentinelTheme.easingStandard
        }
    }
}
