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
        color: control.enabled
               ? SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
               : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
        border.color: control.activeFocus
                      ? SentinelTheme.focusBorder
                      : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.085)
        border.width: 1

        Behavior on border.color {
            ColorAnimation {
                duration: SentinelTheme.durationFast
                easing.type: SentinelTheme.easingStandard
            }
        }
    }
}
