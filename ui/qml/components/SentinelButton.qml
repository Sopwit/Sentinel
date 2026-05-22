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
        color: control.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted
        font.pixelSize: SentinelTheme.fontControl
        font.bold: false
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
        color: control.down
               ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.18)
               : control.hovered || control.activeFocus
                 ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.11)
                 : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.052)
        border.color: control.activeFocus
                      ? SentinelTheme.focusBorder
                      : control.hovered
                        ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
                        : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.080)
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
