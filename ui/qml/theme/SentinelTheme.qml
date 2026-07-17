pragma Singleton

import QtQuick

QtObject {
    property string activeTheme: "Liquid Glass Dark"
    property bool reducedMotion: false
    property bool highContrast: false
    property string uiDensity: "Comfortable"
    readonly property real densityScale: uiDensity === "Compact" ? 0.90
                                       : uiDensity === "Large" ? 1.12
                                       : 1.0
    readonly property bool midnightTheme: activeTheme === "Midnight Blue"
    readonly property bool auroraTheme: activeTheme === "Aurora Teal"
    readonly property bool graphiteTheme: activeTheme === "Graphite Grey"
    readonly property bool systemAdaptiveTheme: activeTheme === "System Sync"
    readonly property bool liquidGlassDarkTheme: activeTheme === "Liquid Glass Dark"
    readonly property bool liquidGlassLightTheme: activeTheme === "Liquid Glass Light"
    readonly property bool liquidGlassTheme: liquidGlassDarkTheme || liquidGlassLightTheme
    readonly property bool lightTheme: liquidGlassLightTheme

    readonly property color backgroundBase: liquidGlassLightTheme ? "#f4f6f9"
                                          : liquidGlassDarkTheme ? "#0e1117"
                                          : graphiteTheme ? "#151719"
                                          : auroraTheme ? "#121b1d"
                                          : midnightTheme ? "#0a1020"
                                          : systemAdaptiveTheme ? "#11181c"
                                          : "#10181f"
    readonly property color backgroundRaised: liquidGlassLightTheme ? "#ffffff"
                                            : liquidGlassDarkTheme ? "#161b27"
                                            : graphiteTheme ? "#202326"
                                            : auroraTheme ? "#1b2a2d"
                                            : midnightTheme ? "#111a31"
                                            : systemAdaptiveTheme ? "#1b2327"
                                            : "#18242d"
    readonly property color backgroundDeep: liquidGlassLightTheme ? "#e8ecf2"
                                          : liquidGlassDarkTheme ? "#070a10"
                                          : graphiteTheme ? "#08090a"
                                          : auroraTheme ? "#071112"
                                          : midnightTheme ? "#040713"
                                          : systemAdaptiveTheme ? "#070b0d"
                                          : "#06090e"
    readonly property color panel: liquidGlassLightTheme ? "#ffffff6e"
                                 : liquidGlassDarkTheme ? "#1e273a6e"
                                 : "#27374473"
    readonly property color panelMuted: liquidGlassLightTheme ? "#ffffff4a"
                                      : liquidGlassDarkTheme ? "#12192a4a"
                                      : "#17222b5e"
    readonly property color panelStrong: liquidGlassLightTheme ? "#ffffffa0"
                                       : liquidGlassDarkTheme ? "#2a364e90"
                                       : "#33495882"
    readonly property color panelGlass: liquidGlassLightTheme ? "#ffffff28"
                                      : "#ffffff0c"
    readonly property color panelVeil: liquidGlassLightTheme ? "#ffffff18"
                                     : "#ffffff08"
    readonly property color panelGhost: liquidGlassLightTheme ? "#ffffff10"
                                      : "#ffffff05"
    readonly property color surface: liquidGlassLightTheme ? "#f0f4fa"
                                   : liquidGlassDarkTheme ? "#1c2436"
                                   : graphiteTheme ? "#25292d"
                                   : auroraTheme ? "#213538"
                                   : midnightTheme ? "#15213d"
                                   : systemAdaptiveTheme ? "#20292d"
                                   : "#1d2d38"
    readonly property color surfaceSoft: liquidGlassLightTheme ? "#00000008" : "#ffffff0c"
    readonly property color surfaceMuted: liquidGlassLightTheme ? "#00000012" : "#ffffff10"
    readonly property color surfaceHover: liquidGlassLightTheme ? "#e2e8f4" : "#2a404c"
    readonly property color metricSurface: liquidGlassLightTheme ? "#d0daea78" : "#22374378"
    readonly property color userMessageSurface: liquidGlassLightTheme ? "#c8daf08c" : "#203b468c"
    readonly property color errorSurface: liquidGlassLightTheme ? "#fde8e8" : "#33191a"

    readonly property color textPrimary: highContrast
                                       ? (liquidGlassLightTheme ? "#000000" : "#ffffff")
                                       : liquidGlassLightTheme ? "#0f1724"
                                       : liquidGlassDarkTheme ? "#e8f0ff"
                                       : graphiteTheme ? "#f2f4f4"
                                       : auroraTheme ? "#effbf7"
                                       : midnightTheme ? "#f0f5ff"
                                       : "#eef8ff"
    readonly property color textMuted: highContrast
                                     ? (liquidGlassLightTheme ? "#2d3748" : "#f0f5ff")
                                     : liquidGlassLightTheme ? "#4a5568"
                                     : liquidGlassDarkTheme ? "#8899bb"
                                     : graphiteTheme ? "#a7adaf"
                                     : auroraTheme ? "#9fb8b4"
                                     : midnightTheme ? "#98a9c8"
                                     : "#94abb8"
    readonly property color textPlaceholder: highContrast
                                           ? (liquidGlassLightTheme ? "#4a5568" : "#d4e4ec")
                                           : liquidGlassLightTheme ? "#8898b0"
                                           : liquidGlassDarkTheme ? "#5a6a84"
                                           : graphiteTheme ? "#777f82"
                                           : auroraTheme ? "#78908c"
                                           : midnightTheme ? "#6e7f9e"
                                           : "#6d8490"
    readonly property color textOnAccent: liquidGlassLightTheme ? "#ffffff" : "#07131a"

    readonly property color accent: liquidGlassLightTheme ? "#4f8ef7"
                                  : liquidGlassDarkTheme ? "#7eb8ff"
                                  : graphiteTheme ? "#d0d7dc"
                                  : auroraTheme ? "#7de0b9"
                                  : midnightTheme ? "#8fb4ff"
                                  : "#79dcff"
    readonly property color accentHover: liquidGlassLightTheme ? "#2563eb"
                                       : liquidGlassDarkTheme ? "#a6d0ff"
                                       : graphiteTheme ? "#edf1f3"
                                       : auroraTheme ? "#a2efcf"
                                       : midnightTheme ? "#b4ccff"
                                       : "#a6e9ff"
    readonly property color accentSecondary: liquidGlassLightTheme ? "#7c3aed"
                                           : liquidGlassDarkTheme ? "#a78bfa"
                                           : graphiteTheme ? "#9db1bd"
                                           : auroraTheme ? "#8bb9ff"
                                           : midnightTheme ? "#a78bfa"
                                           : "#83aaf5"
    readonly property color accentTertiary: liquidGlassLightTheme ? "#0ea5e9"
                                          : liquidGlassDarkTheme ? "#67e8f9"
                                          : graphiteTheme ? "#b7c4bd"
                                          : auroraTheme ? "#f0c77a"
                                          : midnightTheme ? "#69d5cc"
                                          : "#72e4c7"
    readonly property color accentBorder: liquidGlassLightTheme ? "#4f8ef726" : "#c8ecff26"
    readonly property color accentBorderSubtle: liquidGlassLightTheme ? "#0000000d" : "#ffffff0d"
    readonly property color accentBorderSoft: liquidGlassLightTheme ? "#4f8ef71a" : "#9bdfff1a"
    readonly property color focusBorder: highContrast
                                       ? (liquidGlassLightTheme ? "#000000" : "#ffffff")
                                       : liquidGlassLightTheme ? "#4f8ef799"
                                       : "#9bdfff66"
    readonly property color success: liquidGlassLightTheme ? "#10b981" : "#9ff0d0"
    readonly property color successBorder: liquidGlassLightTheme ? "#10b98133" : "#9ff0d033"
    readonly property color errorBorder: liquidGlassLightTheme ? "#ef444466" : "#d66b6b66"
    readonly property color separator: highContrast
                                     ? (liquidGlassLightTheme ? "#33000000" : "#33ffffff")
                                     : liquidGlassLightTheme ? "#0000000f" : "#ffffff0f"
    readonly property color glowSoft: liquidGlassLightTheme ? "#4f8ef72b" : "#9bdfff2b"
    readonly property color glowStrong: liquidGlassLightTheme ? "#4f8ef759" : "#9bdfff59"
    readonly property color glassSoft: liquidGlassLightTheme ? "#ffffff48" : "#ffffff09"
    readonly property color glassStrong: liquidGlassLightTheme ? "#ffffffa0" : "#ffffff12"
    readonly property color orbitalLine: liquidGlassLightTheme ? "#4f8ef716" : "#bfefff16"
    readonly property color bracketLine: liquidGlassLightTheme ? "#4f8ef72e" : "#bfefff2e"
    readonly property color warning: "#e7b76a"
    readonly property color warningText: liquidGlassLightTheme ? "#7c4a00" : "#18120a"
    readonly property color ambientCyan: liquidGlassLightTheme ? "#0ea5e9" : "#65dfff"
    readonly property color ambientTeal: liquidGlassLightTheme ? "#14b8a6" : "#7fffd4"
    readonly property color ambientViolet: liquidGlassLightTheme ? "#7c3aed" : "#8bb8ff"

    readonly property int spaceXs: scaleSize(4)
    readonly property int spaceSm: scaleSize(8)
    readonly property int spaceMd: scaleSize(14)
    readonly property int spaceLg: scaleSize(18)
    readonly property int spaceXl: scaleSize(22)
    readonly property int space2Xl: scaleSize(28)
    readonly property int space3Xl: scaleSize(36)
    readonly property int space4Xl: scaleSize(48)
    readonly property int space5Xl: scaleSize(64)
    readonly property int controlHeight: scaleSize(38)
    readonly property int cardPadding: scaleSize(14)
    readonly property int panelPadding: scaleSize(22)

    readonly property int radiusSm: scaleSize(8)
    readonly property int radiusMd: scaleSize(10)
    readonly property int radiusLg: scaleSize(14)
    readonly property int radiusXl: scaleSize(18)
    readonly property int radiusPanel: scaleSize(22)
    readonly property int radiusPill: 999

    readonly property int fontTiny: scaleSize(11)
    readonly property int fontSmall: scaleSize(12)
    readonly property int fontBody: scaleSize(13)
    readonly property int fontControl: scaleSize(14)
    readonly property int fontCard: scaleSize(18)
    readonly property int fontTitle: scaleSize(22)
    readonly property int fontBrand: scaleSize(24)
    readonly property int fontHeader: scaleSize(27)
    readonly property int fontDisplay: scaleSize(36)
    readonly property int fontHero: scaleSize(44)

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
    readonly property color calmAccentBorder: liquidGlassLightTheme ? "#4f8ef730" : "#9bdfff30"
    readonly property color calmFocusGlow: liquidGlassLightTheme ? "#4f8ef752" : "#9bdfff52"
    readonly property color cardShadow: liquidGlassLightTheme ? "#00000018" : "#0000004a"
    // Liquid Glass specific tokens
    readonly property color glassBackdrop: liquidGlassLightTheme ? "#ffffff70" : "#ffffff0a"
    readonly property color glassBackdropStrong: liquidGlassLightTheme ? "#ffffffb0" : "#ffffff16"
    readonly property color glassBorder: liquidGlassLightTheme ? "#ffffff90" : "#ffffff1a"
    readonly property color glassInnerGlow: liquidGlassLightTheme ? "#ffffffc0" : "#ffffff08"
    readonly property color glassFrost: liquidGlassLightTheme ? "#f0f4ff90" : "#0a1428a0"

    function pageMargin(width) {
        return width < breakpointCompact ? spaceMd : space2Xl
    }

    function contentSpacing(width) {
        return width < breakpointCompact ? spaceSm : spaceXl
    }

    function modeAccent(_modeName) {
        return accent
    }

    function modeSecondaryAccent(_modeName) {
        return accentTertiary
    }

    function modePanelColor(_modeName) {
        return "#25364366"
    }

    function modeStatusText(_modeName) {
        return "Calm ambient collaboration."
    }

    function modeGlowScale(_modeName) {
        return 1.0
    }

    function withAlpha(sourceColor, alpha) {
        return Qt.rgba(sourceColor.r, sourceColor.g, sourceColor.b, alpha)
    }

    function scaleSize(value) {
        return Math.max(1, Math.round(value * densityScale))
    }
}
