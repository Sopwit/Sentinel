pragma Singleton

import QtQuick

QtObject {
    property bool reducedMotion: false

    readonly property int instant: 0
    readonly property int fast: reducedMotion ? 0 : 90
    readonly property int normal: reducedMotion ? 0 : 150
    readonly property int slow: reducedMotion ? 0 : 230
    readonly property int menu: reducedMotion ? 0 : 120
    readonly property int page: reducedMotion ? 0 : 170
    readonly property int message: reducedMotion ? 0 : 150

    readonly property int standard: Easing.InOutQuad
    readonly property int enter: Easing.OutCubic
    readonly property int exit: Easing.InCubic
    readonly property int press: Easing.OutQuad

    function reduced(modeName) {
        return reducedMotion
    }

    function duration(baseDuration, modeName) {
        if (reducedMotion)
            return 0
        return reduced(modeName) ? Math.max(0, Math.round(baseDuration * 0.55)) : baseDuration
    }

    function telemetryScale(modeName) {
        return reduced(modeName) ? 0.72 : 1.0
    }
}
