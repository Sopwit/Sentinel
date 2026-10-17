// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import Sentinel.Desktop

Popup {
    id: modal
    property color accent: SentinelTheme.calmAccent
    property int preferredWidth: 620
    property int preferredHeight: 420
    property string modeName: "Sentinel"

    // ── Keyboard / focus contract ─────────────────────────────────────────────
    // initialFocusItem pins the item that receives focus when the overlay opens.
    // Safety-critical dialogs must point it at a non-destructive action (e.g. Deny)
    // so that a stray Enter cannot confirm the dangerous action. When it is unset the
    // first tabbable direct child of the content is used, preserving earlier behaviour.
    property Item initialFocusItem: null
    property Item previousFocusItem: null

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

    // Move focus into the overlay once it is shown. This is scheduled from
    // `aboutToShow` and deferred to the next event-loop turn so it runs after the
    // popup has taken focus itself. It deliberately does not use `opened`, which is
    // only emitted once the enter transition has finished and therefore never fires
    // when transitions are disabled (reduced motion) or not animated (headless).
    function applyInitialFocus() {
        if (!modal.visible)
            return
        var target = modal.initialFocusItem
        if (!target && contentItem) {
            var children = contentItem.children
            for (var i = 0; i < children.length; ++i) {
                var child = children[i]
                if (child.visible && child.enabled && child.activeFocusOnTab) {
                    target = child
                    break
                }
            }
        }
        if (target && target.visible !== false && target.enabled !== false)
            target.forceActiveFocus(Qt.TabFocusReason)
    }

    // Restore the previously focused item as soon as the overlay starts hiding, so
    // closing a dialog never strands focus. `aboutToHide` is used rather than
    // `closed` so focus is returned even when the exit transition does not run.
    function restorePreviousFocus() {
        var target = modal.previousFocusItem
        modal.previousFocusItem = null
        if (target && target.visible && target.enabled)
            target.forceActiveFocus(Qt.TabFocusReason)
    }

    onAboutToShow: {
        // Captured before the overlay takes focus so it can be handed back on close.
        modal.previousFocusItem = contentItem ? contentItem.Window.activeFocusItem : null
        Qt.callLater(modal.applyInitialFocus)
    }

    onAboutToHide: modal.restorePreviousFocus()

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
