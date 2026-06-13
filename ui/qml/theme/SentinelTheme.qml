pragma Singleton

import QtQuick

QtObject {
    property string activeTheme: "Sentinel Dark"
    readonly property bool midnightTheme: activeTheme === "Midnight"
    readonly property bool auroraTheme: activeTheme === "Aurora"
    readonly property bool graphiteTheme: activeTheme === "Graphite"
    readonly property bool systemAdaptiveTheme: activeTheme === "System Adaptive"

    readonly property color backgroundBase: graphiteTheme ? "#151719"
                                          : auroraTheme ? "#121b1d"
                                          : midnightTheme ? "#0a1020"
                                          : systemAdaptiveTheme ? "#11181c"
                                          : "#10181f"
    readonly property color backgroundRaised: graphiteTheme ? "#202326"
                                            : auroraTheme ? "#1b2a2d"
                                            : midnightTheme ? "#111a31"
                                            : systemAdaptiveTheme ? "#1b2327"
                                            : "#18242d"
    readonly property color backgroundDeep: graphiteTheme ? "#08090a"
                                          : auroraTheme ? "#071112"
                                          : midnightTheme ? "#040713"
                                          : systemAdaptiveTheme ? "#070b0d"
                                          : "#06090e"
    readonly property color panel: "#27374473"
    readonly property color panelMuted: "#17222b5e"
    readonly property color panelStrong: "#33495882"
    readonly property color panelGlass: "#ffffff0c"
    readonly property color panelVeil: "#ffffff08"
    readonly property color panelGhost: "#ffffff05"
    readonly property color surface: graphiteTheme ? "#25292d"
                                   : auroraTheme ? "#213538"
                                   : midnightTheme ? "#15213d"
                                   : systemAdaptiveTheme ? "#20292d"
                                   : "#1d2d38"
    readonly property color surfaceSoft: "#ffffff0c"
    readonly property color surfaceMuted: "#ffffff10"
    readonly property color surfaceHover: "#2a404c"
    readonly property color metricSurface: "#22374378"
    readonly property color userMessageSurface: "#203b468c"
    readonly property color errorSurface: "#33191a"

    readonly property color textPrimary: graphiteTheme ? "#f2f4f4"
                                       : auroraTheme ? "#effbf7"
                                       : midnightTheme ? "#f0f5ff"
                                       : "#eef8ff"
    readonly property color textMuted: graphiteTheme ? "#a7adaf"
                                     : auroraTheme ? "#9fb8b4"
                                     : midnightTheme ? "#98a9c8"
                                     : "#94abb8"
    readonly property color textPlaceholder: graphiteTheme ? "#777f82"
                                           : auroraTheme ? "#78908c"
                                           : midnightTheme ? "#6e7f9e"
                                           : "#6d8490"
    readonly property color textOnAccent: "#07131a"

    readonly property color accent: graphiteTheme ? "#d0d7dc"
                                  : auroraTheme ? "#7de0b9"
                                  : midnightTheme ? "#8fb4ff"
                                  : "#79dcff"
    readonly property color accentHover: graphiteTheme ? "#edf1f3"
                                       : auroraTheme ? "#a2efcf"
                                       : midnightTheme ? "#b4ccff"
                                       : "#a6e9ff"
    readonly property color accentSecondary: graphiteTheme ? "#9db1bd"
                                           : auroraTheme ? "#8bb9ff"
                                           : midnightTheme ? "#a78bfa"
                                           : "#83aaf5"
    readonly property color accentTertiary: graphiteTheme ? "#b7c4bd"
                                          : auroraTheme ? "#f0c77a"
                                          : midnightTheme ? "#69d5cc"
                                          : "#72e4c7"
    readonly property color accentBorder: "#c8ecff26"
    readonly property color accentBorderSubtle: "#ffffff0d"
    readonly property color accentBorderSoft: "#9bdfff1a"
    readonly property color focusBorder: "#9bdfff66"
    readonly property color success: "#9ff0d0"
    readonly property color successBorder: "#9ff0d033"
    readonly property color errorBorder: "#d66b6b66"
    readonly property color separator: "#ffffff0f"
    readonly property color glowSoft: "#9bdfff2b"
    readonly property color glowStrong: "#9bdfff59"
    readonly property color glassSoft: "#ffffff09"
    readonly property color glassStrong: "#ffffff12"
    readonly property color orbitalLine: "#bfefff16"
    readonly property color bracketLine: "#bfefff2e"
    readonly property color warning: "#e7b76a"
    readonly property color warningText: "#18120a"
    readonly property color ambientCyan: "#65dfff"
    readonly property color ambientTeal: "#7fffd4"
    readonly property color ambientViolet: "#8bb8ff"

    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 14
    readonly property int spaceLg: 18
    readonly property int spaceXl: 22
    readonly property int space2Xl: 28
    readonly property int space3Xl: 36
    readonly property int space4Xl: 48
    readonly property int space5Xl: 64
    readonly property int controlHeight: 38
    readonly property int cardPadding: 14
    readonly property int panelPadding: 22

    readonly property int radiusSm: 8
    readonly property int radiusMd: 10
    readonly property int radiusLg: 14
    readonly property int radiusXl: 18
    readonly property int radiusPanel: 22
    readonly property int radiusPill: 999

    readonly property int fontTiny: 11
    readonly property int fontSmall: 12
    readonly property int fontBody: 13
    readonly property int fontControl: 14
    readonly property int fontCard: 18
    readonly property int fontTitle: 22
    readonly property int fontBrand: 24
    readonly property int fontHeader: 27
    readonly property int fontDisplay: 36
    readonly property int fontHero: 44

    readonly property int breakpointCompact: 760
    readonly property int breakpointWide: 1120
    readonly property int rightPanelWidth: 380
    readonly property int dockHeight: 72

    readonly property real glassOpacity: 0.42
    readonly property real panelBorderOpacity: 0.10
    readonly property real glowOpacity: 0.46
    readonly property real glowWhisperOpacity: 0.18
    readonly property real glowCinematicOpacity: 0.62
    readonly property real surfaceGlassOpacity: 0.34
    readonly property real mutedOpacity: 0.72
    readonly property real disabledOpacity: 0.48
    readonly property int durationFast: 90
    readonly property int durationNormal: 140
    readonly property int durationSlow: 220
    readonly property int durationAmbient: 3200
    readonly property int durationOrbit: 22000
    readonly property int easingStandard: Easing.InOutQuad
    readonly property int easingEmphasized: Easing.OutCubic
    readonly property color calmAccent: accent
    readonly property color calmAccentHover: accentHover
    readonly property color calmAccentBorder: "#9bdfff30"
    readonly property color calmFocusGlow: "#9bdfff52"
    readonly property color cardShadow: "#0000004a"

    function pageMargin(width) {
        return width < breakpointCompact ? spaceMd : space2Xl
    }

    function contentSpacing(width) {
        return width < breakpointCompact ? spaceSm : spaceXl
    }

    function modeAccent(modeName) {
        if (modeName === "Focus Mode")
            return "#8eb8ff"
        if (modeName === "Mission Mode")
            return "#67d8c2"
        if (modeName === "System Mode")
            return "#7fe7ff"
        if (modeName === "Minimal Mode")
            return "#9aa8af"
        if (modeName === "Tactical Mode")
            return "#e28a76"
        return accent
    }

    function modeSecondaryAccent(modeName) {
        if (modeName === "Mission Mode")
            return "#67d8c2"
        if (modeName === "Tactical Mode")
            return "#d7a06e"
        if (modeName === "Minimal Mode")
            return "#6f7d84"
        if (modeName === "Focus Mode")
            return "#6f9de8"
        return accentTertiary
    }

    function modePanelColor(modeName) {
        if (modeName === "Focus Mode")
            return "#1a2b42aa"
        if (modeName === "Mission Mode")
            return "#20363aaa"
        if (modeName === "System Mode")
            return "#1b3440aa"
        if (modeName === "Minimal Mode")
            return "#1d2429aa"
        if (modeName === "Tactical Mode")
            return "#352a28aa"
        return "#25364366"
    }

    function modeStatusText(modeName) {
        if (modeName === "Focus Mode")
            return "Quiet workspace, reduced visual pressure."
        if (modeName === "Mission Mode")
            return "Planning posture, operational context foregrounded."
        if (modeName === "System Mode")
            return "Diagnostics posture, system contracts visible."
        if (modeName === "Minimal Mode")
            return "Reduced chrome, essential state only."
        if (modeName === "Tactical Mode")
            return "High-contrast posture for active monitoring."
        return "Companion posture, calm ambient collaboration."
    }

    function modeGlowScale(modeName) {
        if (modeName === "Minimal Mode")
            return 0.28
        if (modeName === "Tactical Mode" || modeName === "Mission Mode")
            return 1.12
        if (modeName === "Focus Mode")
            return 0.68
        return 1.0
    }

    function withAlpha(sourceColor, alpha) {
        return Qt.rgba(sourceColor.r, sourceColor.g, sourceColor.b, alpha)
    }
}
