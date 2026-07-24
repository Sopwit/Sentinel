pragma Singleton

import QtQuick

QtObject {
    property string activeTheme: "Liquid Glass Light"
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
    readonly property bool liquidGlassDarkTheme: activeTheme === "Liquid Glass Dark" || activeTheme === "Sentinel Classic"
    readonly property bool liquidGlassLightTheme: !liquidGlassDarkTheme && !midnightTheme && !auroraTheme && !graphiteTheme
    readonly property bool liquidGlassTheme: liquidGlassDarkTheme || liquidGlassLightTheme
    readonly property bool lightTheme: liquidGlassLightTheme

    readonly property color backgroundBase: liquidGlassDarkTheme ? "#0e1117"
                                          : graphiteTheme ? "#151719"
                                          : auroraTheme ? "#121b1d"
                                          : midnightTheme ? "#0a1020"
                                          : "#f4f6f9"
    readonly property color backgroundRaised: liquidGlassDarkTheme ? "#161b27"
                                            : graphiteTheme ? "#202326"
                                            : auroraTheme ? "#1b2a2d"
                                            : midnightTheme ? "#111a31"
                                            : "#ffffff"
    readonly property color backgroundDeep: liquidGlassDarkTheme ? "#070a10"
                                          : graphiteTheme ? "#08090a"
                                          : auroraTheme ? "#071112"
                                          : midnightTheme ? "#040713"
                                          : "#e8ecf2"
    readonly property color panel: liquidGlassDarkTheme ? "#6e1e273a"
                                 : graphiteTheme ? "#73273744"
                                 : "#ffffff"
    readonly property color panelMuted: liquidGlassDarkTheme ? "#4a12192a"
                                      : graphiteTheme ? "#5e17222b"
                                      : "#f8fafc"
    readonly property color panelStrong: liquidGlassDarkTheme ? "#902a364e"
                                       : graphiteTheme ? "#82334958"
                                       : "#ffffff"
    readonly property color panelGlass: liquidGlassDarkTheme ? "#0cffffff"
                                      : "#ffffff"
    readonly property color panelVeil: liquidGlassDarkTheme ? "#08ffffff"
                                     : "#f8fafc"
    readonly property color panelGhost: liquidGlassDarkTheme ? "#05ffffff"
                                      : "#ffffff"
    readonly property color surface: liquidGlassDarkTheme ? "#1c2436"
                                   : graphiteTheme ? "#25292d"
                                   : auroraTheme ? "#213538"
                                   : midnightTheme ? "#15213d"
                                   : "#f0f4fa"
    readonly property color surfaceSoft: liquidGlassDarkTheme ? "#0cffffff" : "#08000000"
    readonly property color surfaceMuted: liquidGlassDarkTheme ? "#10ffffff" : "#12000000"
    readonly property color surfaceHover: liquidGlassDarkTheme ? "#2a404c" : "#e2e8f4"
    readonly property color metricSurface: liquidGlassDarkTheme ? "#78223743" : "#78d0daea"
    readonly property color userMessageSurface: liquidGlassDarkTheme ? "#8c203b46" : "#8cc8daf0"
    readonly property color errorSurface: liquidGlassDarkTheme ? "#33191a" : "#fde8e8"

    readonly property color textPrimary: highContrast
                                       ? (liquidGlassLightTheme ? "#000000" : "#ffffff")
                                       : liquidGlassDarkTheme ? "#e8f0ff"
                                       : graphiteTheme ? "#f2f4f4"
                                       : auroraTheme ? "#effbf7"
                                       : midnightTheme ? "#f0f5ff"
                                       : "#0f1724"
    readonly property color textMuted: highContrast
                                     ? (liquidGlassLightTheme ? "#2d3748" : "#f0f5ff")
                                     : liquidGlassDarkTheme ? "#8899bb"
                                     : graphiteTheme ? "#a7adaf"
                                     : auroraTheme ? "#9fb8b4"
                                     : midnightTheme ? "#98a9c8"
                                     : "#4a5568"
    readonly property color textPlaceholder: highContrast
                                           ? (liquidGlassLightTheme ? "#4a5568" : "#d4e4ec")
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
                                  : systemAdaptiveTheme ? "#4f8ef7"
                                  : "#79dcff"
    readonly property color accentHover: liquidGlassLightTheme ? "#3b7ee8"
                                       : liquidGlassDarkTheme ? "#99c8ff"
                                       : graphiteTheme ? "#e1e7eb"
                                       : auroraTheme ? "#99ebd0"
                                       : midnightTheme ? "#a8c6ff"
                                       : "#9fe4ff"
    readonly property color accentSecondary: liquidGlassLightTheme ? "#7c3aed"
                                           : liquidGlassDarkTheme ? "#a78bfa"
                                           : graphiteTheme ? "#9db1bd"
                                           : auroraTheme ? "#8bb9ff"
                                           : midnightTheme ? "#a78bfa"
                                           : "#83aaf5"
    readonly property color accentTertiary: liquidGlassLightTheme ? "#a855f7"
                                             : liquidGlassDarkTheme ? "#c084fc"
                                             : graphiteTheme ? "#e2e8f0"
                                             : auroraTheme ? "#81e6d9"
                                             : midnightTheme ? "#b4c6ff"
                                             : "#9be4ff"
    readonly property color accentBorder: liquidGlassLightTheme ? "#264f8ef7" : "#26c8ecff"
    readonly property color accentBorderSubtle: liquidGlassLightTheme ? "#0d000000" : "#0dffffff"
    readonly property color accentBorderSoft: liquidGlassLightTheme ? "#1a4f8ef7" : "#1a9bdfff"
    readonly property color focusBorder: highContrast
                                       ? (liquidGlassLightTheme ? "#000000" : "#ffffff")
                                       : liquidGlassLightTheme ? "#994f8ef7"
                                       : "#669bdfff"
    readonly property color success: liquidGlassLightTheme ? "#10b981" : "#9ff0d0"
    readonly property color successBorder: liquidGlassLightTheme ? "#3310b981" : "#339ff0d0"
    readonly property color errorBorder: liquidGlassLightTheme ? "#66ef4444" : "#66d66b6b"
    readonly property color separator: highContrast
                                     ? (liquidGlassLightTheme ? "#33000000" : "#33ffffff")
                                     : liquidGlassLightTheme ? "#0f000000" : "#0fffffff"
    readonly property color glowSoft: liquidGlassLightTheme ? "#2b4f8ef7" : "#2b9bdfff"
    readonly property color glowStrong: liquidGlassLightTheme ? "#594f8ef7" : "#599bdfff"
    readonly property color glassSoft: liquidGlassLightTheme ? "#48ffffff" : "#09ffffff"
    readonly property color glassStrong: liquidGlassLightTheme ? "#a0ffffff" : "#12ffffff"
    readonly property color orbitalLine: liquidGlassLightTheme ? "#164f8ef7" : "#16bfefff"
    readonly property color bracketLine: liquidGlassLightTheme ? "#2e4f8ef7" : "#2ebfefff"
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
    readonly property color calmAccentBorder: liquidGlassLightTheme ? "#304f8ef7" : "#309bdfff"
    readonly property color calmFocusGlow: liquidGlassLightTheme ? "#524f8ef7" : "#529bdfff"
    readonly property color cardShadow: liquidGlassLightTheme ? "#18000000" : "#4a000000"
    // Liquid Glass specific tokens
    readonly property color glassBackdrop: liquidGlassLightTheme ? "#70ffffff" : "#0affffff"
    readonly property color glassBackdropStrong: liquidGlassLightTheme ? "#b0ffffff" : "#16ffffff"
    readonly property color glassBorder: liquidGlassLightTheme ? "#90ffffff" : "#1affffff"
    readonly property color glassInnerGlow: liquidGlassLightTheme ? "#c0ffffff" : "#08ffffff"
    readonly property color glassFrost: liquidGlassLightTheme ? "#90f0f4ff" : "#a00a1428"

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
        return liquidGlassLightTheme ? "#f0f4fa8c" : "#25364366"
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
