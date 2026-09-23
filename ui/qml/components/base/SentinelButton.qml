// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import Sentinel.Desktop

Button {
    id: control

    property string tooltipText: ""
    property string iconName: ""
    property bool premium: false
    property color accent: SentinelTheme.calmAccent

    ToolTip.visible: control.hovered && control.tooltipText.length > 0
    ToolTip.text: control.tooltipText

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    opacity: enabled ? 1.0 : InteractionTokens.disabledOpacity
    implicitHeight: SentinelTheme.controlHeight
    scale: down ? InteractionTokens.pressScale
                : hovered || activeFocus ? InteractionTokens.focusScale : 1.0
    font.pixelSize: SentinelTheme.fontControl
    font.bold: false

    contentItem: Item {
        implicitWidth: contentRow.implicitWidth
        implicitHeight: Math.max(18, contentText.implicitHeight)

        Row {
            id: contentRow
            anchors.centerIn: parent
            spacing: control.iconName && control.text ? 6 : 0

            Image {
                visible: !!control.iconName
                width: visible ? 18 : 0
                height: 18
                source: control.iconName ? "qrc:/icons/tabler/" + control.iconName + ".svg" : ""
                sourceSize.width: 18
                sourceSize.height: 18
                layer.enabled: true
                layer.effect: MultiEffect {
                    colorization: 1.0
                    colorizationColor: control.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                }
            }

            Text {
                id: contentText
                visible: !!control.text
                text: control.text
                color: control.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                font: control.font
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }
    }

    background: Rectangle {
        id: bg
        radius: SentinelTheme.radiusMd
        color: InteractionTokens.surfaceColor(control.hovered, control.down, false, control.accent)
        border.color: InteractionTokens.borderColor(control.activeFocus, control.hovered, false, control.accent)
        border.width: 1

        layer.enabled: control.enabled && (control.premium || control.highlighted)
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12)
            shadowVerticalOffset: 1
            shadowBlur: 0.08
            shadowOpacity: 1.0
        }

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

    // Focus glow ring (visible on keyboard focus)
    Rectangle {
        anchors.fill: parent
        anchors.margins: -3
        radius: SentinelTheme.radiusMd + 3
        color: "transparent"
        border.color: SentinelTheme.calmFocusGlow
        border.width: 2
        opacity: control.activeFocus ? 1.0 : 0.0
        z: -1

        Behavior on opacity {
            NumberAnimation { duration: MotionTokens.fast; easing.type: MotionTokens.standard }
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
