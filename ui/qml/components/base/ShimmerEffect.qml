// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Effects
import Sentinel.Desktop

Rectangle {
    id: shimmer

    property bool active: true
    property color baseColor: Qt.rgba(1, 1, 1, 0.04)
    property color highlightColor: Qt.rgba(1, 1, 1, 0.08)

    color: baseColor
    radius: SentinelTheme.radiusSm
    clip: true

    Rectangle {
        id: shimmerBar
        width: parent.width * 0.5
        height: parent.height
        color: shimmer.highlightColor
        opacity: 0.0

        SequentialAnimation on x {
            loops: Animation.Infinite
            running: shimmer.active
            NumberAnimation {
                from: -shimmerBar.width
                to: shimmer.parent.width
                duration: 1500
                easing.type: Easing.InOutSine
            }
            PauseAnimation { duration: 1200 }
        }

        SequentialAnimation on opacity {
            loops: Animation.Infinite
            running: shimmer.active
            NumberAnimation { from: 0.0; to: 0.8; duration: 750; easing.type: Easing.InOutSine }
            NumberAnimation { from: 0.8; to: 0.0; duration: 750; easing.type: Easing.InOutSine }
            PauseAnimation { duration: 1200 }
        }
    }
}
