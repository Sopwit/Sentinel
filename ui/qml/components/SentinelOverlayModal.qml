import QtQuick
import QtQuick.Controls.Basic

Popup {
    id: modal
    property color accent: SentinelTheme.calmAccent
    property int preferredWidth: 620
    property int preferredHeight: 420
    property string modeName: "Sentinel"

    parent: Overlay.overlay
    width: Math.min(preferredWidth, Math.max(320, parent ? parent.width - SentinelTheme.space4Xl : preferredWidth))
    height: Math.min(preferredHeight, Math.max(280, parent ? parent.height - SentinelTheme.space4Xl : preferredHeight))
    x: parent ? Math.round((parent.width - width) / 2) : 0
    y: parent ? Math.round((parent.height - height) * 0.28) : 0
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    padding: 0

    Overlay.modal: Rectangle {
        color: SentinelTheme.withAlpha(SentinelTheme.backgroundDeep, 0.58)

        Behavior on opacity {
            NumberAnimation {
                duration: MotionTokens.duration(MotionTokens.menu, modal.modeName)
                easing.type: MotionTokens.standard
            }
        }
    }

    enter: Transition {
        NumberAnimation {
            property: "opacity"
            from: 0.0
            to: 1.0
            duration: MotionTokens.duration(MotionTokens.menu, modal.modeName)
            easing.type: MotionTokens.enter
        }
        NumberAnimation {
            property: "scale"
            from: MotionTokens.reduced(modal.modeName) ? 1.0 : 0.985
            to: 1.0
            duration: MotionTokens.duration(MotionTokens.menu, modal.modeName)
            easing.type: MotionTokens.enter
        }
    }

    exit: Transition {
        NumberAnimation {
            property: "opacity"
            to: 0.0
            duration: MotionTokens.duration(MotionTokens.fast, modal.modeName)
            easing.type: MotionTokens.exit
        }
    }

    background: Rectangle {
        id: bgRec
        radius: SentinelTheme.radiusXl
        color: SentinelTheme.lightTheme
             ? SentinelTheme.withAlpha("#ffffff", 0.95)
             : SentinelTheme.withAlpha(SentinelTheme.backgroundRaised, 0.96)
        border.color: SentinelTheme.lightTheme
                    ? SentinelTheme.withAlpha("#ffffff", 0.88)
                    : SentinelTheme.withAlpha(modal.accent, 0.25)
        border.width: 1

        // Subtle gradient glow from the accent color
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            opacity: 0.04
            gradient: Gradient {
                GradientStop { position: 0.0; color: modal.accent }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        // Top highlight sheen for premium liquid glass feel
        Rectangle {
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.topMargin: 1
            anchors.leftMargin: SentinelTheme.radiusXl
            anchors.rightMargin: SentinelTheme.radiusXl
            height: 1
            color: SentinelTheme.lightTheme
                 ? SentinelTheme.withAlpha("#ffffff", 0.95)
                 : SentinelTheme.withAlpha("#ffffff", 0.35)
            radius: 1
        }
    }
}
