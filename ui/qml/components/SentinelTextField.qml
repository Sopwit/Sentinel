import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: control

    color: SentinelTheme.textPrimary
    placeholderTextColor: SentinelTheme.textPlaceholder
    selectByMouse: true
    implicitHeight: SentinelTheme.controlHeight

    background: Rectangle {
        radius: SentinelTheme.radiusMd
        color: SentinelTheme.backgroundBase
        border.color: control.activeFocus ? SentinelTheme.focusBorder : SentinelTheme.accentBorder
        border.width: 1

        Behavior on border.color {
            ColorAnimation {
                duration: SentinelTheme.durationFast
                easing.type: SentinelTheme.easingStandard
            }
        }
    }
}
