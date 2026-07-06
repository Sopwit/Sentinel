import QtQuick
import QtQuick.Controls.Basic

Button {
    id: control

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1.0 : InteractionTokens.disabledOpacity
    implicitHeight: SentinelTheme.controlHeight
    scale: down ? InteractionTokens.pressScale
                : hovered || activeFocus ? InteractionTokens.focusScale : 1.0
    font.pixelSize: SentinelTheme.fontControl
    font.bold: false

    contentItem: Text {
        text: control.text
        color: control.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted
        font: control.font
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight

        Behavior on color {
            ColorAnimation {
                duration: MotionTokens.fast
                easing.type: MotionTokens.standard
            }
        }
    }

    background: Rectangle {
        radius: SentinelTheme.radiusMd
        color: InteractionTokens.surfaceColor(control.hovered, control.down, false, SentinelTheme.calmAccent)
        border.color: InteractionTokens.borderColor(control.activeFocus, control.hovered, false, SentinelTheme.calmAccent)
        border.width: 1

        Behavior on color {
            ColorAnimation {
                duration: MotionTokens.fast
                easing.type: MotionTokens.standard
            }
        }

        Behavior on border.color {
            ColorAnimation {
                duration: MotionTokens.fast
                easing.type: MotionTokens.standard
            }
        }
    }

    Behavior on opacity {
        NumberAnimation {
            duration: MotionTokens.fast
            easing.type: MotionTokens.standard
        }
    }

    Behavior on scale {
        NumberAnimation {
            duration: MotionTokens.fast
            easing.type: MotionTokens.press
        }
    }
}
