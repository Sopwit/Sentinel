// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import Sentinel.Desktop

TextField {
    id: control

    property string accessibleDescription: ""

    color: SentinelTheme.textPrimary
    placeholderTextColor: SentinelTheme.textPlaceholder
    selectByMouse: true
    hoverEnabled: true

    // Accessibility contract: an editable text field is exposed as EditableText.
    // The placeholder is the default accessible name until the usage site supplies
    // a real label via Accessible.name (a placeholder is not a substitute for a label).
    Accessible.role: Accessible.EditableText
    Accessible.name: control.placeholderText
    Accessible.description: control.accessibleDescription
    Accessible.readOnly: control.readOnly

    // Font fallback and DPI can make the line taller than the nominal control.
    implicitHeight: Math.max(36, Math.ceil(lineMetrics.height) + topPadding + bottomPadding)
    FontMetrics {
        id: lineMetrics
        font: control.font
    }
    leftPadding: SentinelTheme.spaceMd
    rightPadding: SentinelTheme.spaceMd
    font.pixelSize: SentinelTheme.fontBody
    selectionColor: SentinelTheme.withAlpha(SentinelTheme.calmAccent, 0.34)
    selectedTextColor: SentinelTheme.textPrimary

    background: Rectangle {
        radius: SentinelTheme.radiusMd
        color: control.enabled
               ? SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.72)
               : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
        border.color: control.activeFocus
                      ? SentinelTheme.withAlpha(SentinelTheme.calmAccent, 0.46)
                      : control.hovered
                        ? SentinelTheme.withAlpha(SentinelTheme.calmAccent, 0.24)
                      : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
        border.width: 1

        layer.enabled: control.activeFocus
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
            shadowVerticalOffset: 1
            shadowBlur: 0.08
            shadowOpacity: 1.0
        }

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
    }
}
