import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: control

    color: SentinelTheme.textPrimary
    placeholderTextColor: SentinelTheme.textPlaceholder
    selectByMouse: true
    hoverEnabled: true
    implicitHeight: SentinelTheme.controlHeight
    selectionColor: SentinelTheme.withAlpha(SentinelTheme.calmAccent, 0.34)
    selectedTextColor: SentinelTheme.textPrimary

    background: Rectangle {
        radius: SentinelTheme.radiusMd
        color: control.enabled
               ? SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
               : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
        border.color: InteractionTokens.borderColor(control.activeFocus, control.hovered, false, SentinelTheme.calmAccent)
        border.width: 1
        scale: control.activeFocus ? InteractionTokens.focusScale : 1.0

        Behavior on border.color {
            ColorAnimation {
                duration: MotionTokens.fast
                easing.type: MotionTokens.standard
            }
        }

        Behavior on color {
            ColorAnimation {
                duration: MotionTokens.fast
                easing.type: MotionTokens.standard
            }
        }

        Behavior on scale {
            NumberAnimation {
                duration: MotionTokens.fast
                easing.type: MotionTokens.enter
            }
        }
    }
}
