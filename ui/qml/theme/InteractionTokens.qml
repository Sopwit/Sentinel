pragma Singleton

import QtQuick

QtObject {
    readonly property real hoverOpacity: 0.075
    readonly property real pressOpacity: 0.145
    readonly property real activeOpacity: 0.135
    readonly property real selectedOpacity: 0.115
    readonly property real focusOpacity: 0.34
    readonly property real calmBorderOpacity: 0.075
    readonly property real hoverBorderOpacity: 0.18
    readonly property real activeBorderOpacity: 0.24
    readonly property real focusBorderOpacity: 0.46
    readonly property real disabledOpacity: 0.48
    readonly property real cardHoverLift: 1.006
    readonly property real dockHoverLift: 1.018
    readonly property real pressScale: 0.985
    readonly property real focusScale: 1.004

    function surfaceColor(hovered, pressed, active, accent) {
        if (pressed)
            return SentinelTheme.withAlpha(accent, pressOpacity)
        if (active)
            return SentinelTheme.withAlpha(accent, activeOpacity)
        if (hovered)
            return SentinelTheme.withAlpha(accent, hoverOpacity)
        return SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.040)
    }

    function borderColor(focused, hovered, active, accent) {
        if (focused)
            return SentinelTheme.withAlpha(accent, focusBorderOpacity)
        if (active)
            return SentinelTheme.withAlpha(accent, activeBorderOpacity)
        if (hovered)
            return SentinelTheme.withAlpha(accent, hoverBorderOpacity)
        return SentinelTheme.withAlpha(SentinelTheme.textPrimary, calmBorderOpacity)
    }

    function cardColor(hovered) {
        return hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.052)
                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.034)
    }

    function cardBorderColor(hovered, accent) {
        return hovered ? SentinelTheme.withAlpha(accent, 0.14)
                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
    }
}
