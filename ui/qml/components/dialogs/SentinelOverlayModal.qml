// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import Sentinel.Desktop

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
    clip: true

    onOpened: {
        Qt.callLater(function() {
            if (contentItem && contentItem.children.length > 0) {
                for (var i = 0; i < contentItem.children.length; i++) {
                    var child = contentItem.children[i]
                    if (child.visible && child.enabled && child.activeFocusOnTab) {
                        child.forceActiveFocus()
                        return
                    }
                }
            }
        })
    }

    Overlay.modal: Rectangle {
        color: SentinelTheme.lightTheme
             ? SentinelTheme.withAlpha("#0f1724", 0.20)
             : SentinelTheme.withAlpha("#000000", 0.60)

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
            from: MotionTokens.reduced(modal.modeName) ? 1.0 : 0.975
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
        NumberAnimation {
            property: "scale"
            to: 0.975
            duration: MotionTokens.duration(MotionTokens.fast, modal.modeName)
            easing.type: MotionTokens.exit
        }
    }

    background: Rectangle {
        id: bgRec
        radius: SentinelTheme.radiusXl
        color: SentinelTheme.backgroundBase
        border.color: SentinelTheme.withAlpha(modal.accent, 0.25)
        border.width: 1
    }
}
