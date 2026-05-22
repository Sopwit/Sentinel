pragma Singleton

import QtQuick

QtObject {
    readonly property int instant: 0
    readonly property int fast: 90
    readonly property int normal: 150
    readonly property int slow: 230
    readonly property int menu: 120
    readonly property int page: 170
    readonly property int message: 150

    readonly property int standard: Easing.InOutQuad
    readonly property int enter: Easing.OutCubic
    readonly property int exit: Easing.InCubic
    readonly property int press: Easing.OutQuad

    function reduced(modeName) {
        return modeName === "Focus Mode" || modeName === "Minimal Mode"
    }

    function duration(baseDuration, modeName) {
        return reduced(modeName) ? Math.max(0, Math.round(baseDuration * 0.55)) : baseDuration
    }

    function telemetryScale(modeName) {
        return modeName === "Mission Mode" || modeName === "System Mode" || modeName === "Tactical Mode"
               ? 1.08
               : reduced(modeName) ? 0.72 : 1.0
    }
}
