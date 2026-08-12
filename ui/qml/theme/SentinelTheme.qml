// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
    readonly property bool solarizedLightTheme: activeTheme === "Solarized Light"
    readonly property bool nordFrostTheme: activeTheme === "Nord Frost"
    readonly property bool draculaTheme: activeTheme === "Dracula"
    readonly property bool tokyoNightTheme: activeTheme === "Tokyo Night"
    readonly property bool liquidGlassDarkTheme: activeTheme === "Liquid Glass Dark"
    readonly property bool sentinelClassicTheme: activeTheme === "Sentinel Classic"
    readonly property bool liquidGlassLightTheme: !liquidGlassDarkTheme && !sentinelClassicTheme && !midnightTheme && !auroraTheme && !graphiteTheme && !solarizedLightTheme && !nordFrostTheme && !draculaTheme && !tokyoNightTheme
    readonly property bool liquidGlassTheme: liquidGlassDarkTheme || liquidGlassLightTheme
    readonly property bool lightTheme: liquidGlassLightTheme || solarizedLightTheme

    readonly property color backgroundBase: solarizedLightTheme ? "#fdf6e3"
                                           : liquidGlassLightTheme ? "#f4f6f9"
                                           : draculaTheme ? "#1e1f2e"
                                           : tokyoNightTheme ? "#0f1419"
                                           : nordFrostTheme ? "#2e3440"
                                           : sentinelClassicTheme ? "#1b1f24"
                                           : liquidGlassDarkTheme ? "#0d1117"
                                           : graphiteTheme ? "#121416"
                                           : auroraTheme ? "#0f1a1c"
                                           : midnightTheme ? "#080d1a"
                                           : "#f4f6f9"
    readonly property color backgroundRaised: solarizedLightTheme ? "#eee8d5"
                                              : liquidGlassLightTheme ? "#ffffff"
                                              : draculaTheme ? "#282a3a"
                                              : tokyoNightTheme ? "#1a1f2b"
                                              : nordFrostTheme ? "#3b4252"
                                              : sentinelClassicTheme ? "#22262c"
                                               : liquidGlassDarkTheme ? "#151a26"
                                              : graphiteTheme ? "#1c1f21"
                                              : auroraTheme ? "#1a282b"
                                              : midnightTheme ? "#10182e"
                                              : "#ffffff"
    readonly property color backgroundDeep: solarizedLightTheme ? "#e4dbcf"
                                            : liquidGlassLightTheme ? "#e8ecf2"
                                            : draculaTheme ? "#161728"
                                            : tokyoNightTheme ? "#0a0e15"
                                            : nordFrostTheme ? "#232832"
                                            : sentinelClassicTheme ? "#14171c"
                                            : liquidGlassDarkTheme ? "#070a10"
                                            : graphiteTheme ? "#08090a"
                                            : auroraTheme ? "#071112"
                                            : midnightTheme ? "#030610"
                                            : "#e8ecf2"
    readonly property color panel: draculaTheme ? "#6e282a3a"
                                 : tokyoNightTheme ? "#6e1a1f2b"
                                 : nordFrostTheme ? "#6e3b4252"
                                 : sentinelClassicTheme ? "#262a31"
                                 : liquidGlassDarkTheme ? "#6e1e273a"
                                 : graphiteTheme ? "#6e1e252e"
                                 : auroraTheme ? "#6e163032"
                                 : midnightTheme ? "#6e121f38"
                                 : "#ffffff"
    readonly property color panelMuted: draculaTheme ? "#4a282a3a"
                                        : tokyoNightTheme ? "#4a1a1f2b"
                                        : nordFrostTheme ? "#4a3b4252"
                                        : sentinelClassicTheme ? "#20242b"
                                        : liquidGlassDarkTheme ? "#4a12192a"
                                        : graphiteTheme ? "#4a131a22"
                                        : "#f8fafc"
    readonly property color panelStrong: draculaTheme ? "#90282a3a"
                                         : tokyoNightTheme ? "#901a1f2b"
                                         : nordFrostTheme ? "#903b4252"
                                         : sentinelClassicTheme ? "#2b3039"
                                         : liquidGlassDarkTheme ? "#902a364e"
                                         : graphiteTheme ? "#82222b38"
                                         : "#ffffff"
    readonly property color panelGlass: draculaTheme ? "#0cffffff"
                                       : tokyoNightTheme ? "#0cffffff"
                                       : nordFrostTheme ? "#0cffffff"
                                       : sentinelClassicTheme ? "#2a2f37"
                                       : liquidGlassDarkTheme ? "#0cffffff"
                                       : "#ffffff"
    readonly property color panelVeil: draculaTheme ? "#08ffffff"
                                      : tokyoNightTheme ? "#08ffffff"
                                      : nordFrostTheme ? "#08ffffff"
                                      : sentinelClassicTheme ? "#1f232a"
                                      : liquidGlassDarkTheme ? "#08ffffff"
                                      : "#f8fafc"
    readonly property color panelGhost: draculaTheme ? "#05ffffff"
                                       : tokyoNightTheme ? "#05ffffff"
                                       : nordFrostTheme ? "#05ffffff"
                                       : sentinelClassicTheme ? "#181c22"
                                       : liquidGlassDarkTheme ? "#05ffffff"
                                       : "#ffffff"
    readonly property color surface: draculaTheme ? "#282a3a"
                                     : tokyoNightTheme ? "#1a1f2b"
                                     : nordFrostTheme ? "#3b4252"
                                     : sentinelClassicTheme ? "#24282f"
                                     : liquidGlassDarkTheme ? "#1a2233"
                                     : graphiteTheme ? "#202427"
                                     : auroraTheme ? "#1e3033"
                                     : midnightTheme ? "#121d38"
                                     : "#f0f4fa"
    readonly property color surfaceSoft: liquidGlassLightTheme ? "#08000000" : sentinelClassicTheme ? "#1b1f25" : "#0cffffff"
    readonly property color surfaceMuted: liquidGlassLightTheme ? "#12000000" : sentinelClassicTheme ? "#1f232a" : "#10ffffff"
    readonly property color surfaceHover: liquidGlassLightTheme ? "#e2e8f4" : sentinelClassicTheme ? "#2d333c" : "#2a404c"
    readonly property color metricSurface: liquidGlassLightTheme ? "#78d0daea" : sentinelClassicTheme ? "#7d242a33" : "#78223743"
    readonly property color userMessageSurface: liquidGlassLightTheme ? "#8cc8daf0" : sentinelClassicTheme ? "#7d202a3a" : "#8c203b46"
    readonly property color errorSurface: liquidGlassLightTheme ? "#fde8e8" : sentinelClassicTheme ? "#35282b" : "#33191a"

    readonly property color textPrimary: highContrast
                                       ? (lightTheme ? "#000000" : "#ffffff")
                                       : solarizedLightTheme ? "#073642"
                                       : liquidGlassLightTheme ? "#0f1724"
                                       : draculaTheme ? "#f8f8f2"
                                       : tokyoNightTheme ? "#c0caf5"
                                       : nordFrostTheme ? "#e5e9f0"
                                       : sentinelClassicTheme ? "#e2e6ec"
                                       : liquidGlassDarkTheme ? "#e8f0ff"
                                       : graphiteTheme ? "#eef0f0"
                                       : auroraTheme ? "#effbf7"
                                       : midnightTheme ? "#eef3ff"
                                       : "#0f1724"
    readonly property color textMuted: highContrast
                                      ? (lightTheme ? "#2d3748" : "#f0f5ff")
                                    : solarizedLightTheme ? "#586e75"
                                    : liquidGlassLightTheme ? "#4a5568"
                                      : draculaTheme ? "#6272a4"
                                      : tokyoNightTheme ? "#565f89"
                                      : nordFrostTheme ? "#81a1c1"
                                      : sentinelClassicTheme ? "#9aa3b1"
                                      : liquidGlassDarkTheme ? "#8899bb"
                                      : graphiteTheme ? "#9ea6a8"
                                      : auroraTheme ? "#9fb8b4"
                                      : midnightTheme ? "#98a9c8"
                                      : "#4a5568"
    readonly property color textPlaceholder: highContrast
                                            ? (lightTheme ? "#4a5568" : "#d4e4ec")
                                            : solarizedLightTheme ? "#657b83"
                                            : liquidGlassLightTheme ? "#6d8490"
                                            : draculaTheme ? "#6272a4"
                                            : tokyoNightTheme ? "#565f89"
                                            : nordFrostTheme ? "#81a1c1"
                                            : sentinelClassicTheme ? "#707a88"
                                            : liquidGlassDarkTheme ? "#5a6a84"
                                            : graphiteTheme ? "#6d7578"
                                            : auroraTheme ? "#78908c"
                                            : midnightTheme ? "#6e7f9e"
                                            : "#6d8490"
    readonly property color textOnAccent: lightTheme ? "#ffffff" : "#07131a"

    readonly property color accent: solarizedLightTheme ? "#268bd2"
                                  : liquidGlassLightTheme ? "#4f8ef7"
                                  : draculaTheme ? "#bd93f9"
                                  : tokyoNightTheme ? "#7aa2f7"
                                  : nordFrostTheme ? "#88c0d0"
                                  : sentinelClassicTheme ? "#2f81f7"
                                  : liquidGlassDarkTheme ? "#7eb8ff"
                                  : graphiteTheme ? "#d0d7dc"
                                  : auroraTheme ? "#7de0b9"
                                  : midnightTheme ? "#8fb4ff"
                                  : "#79dcff"
    readonly property color accentHover: solarizedLightTheme ? "#3399e8"
                                        : liquidGlassLightTheme ? "#3b7ee8"
                                        : draculaTheme ? "#caa1fa"
                                        : tokyoNightTheme ? "#89b4fa"
                                        : nordFrostTheme ? "#8fd6e0"
                                        : sentinelClassicTheme ? "#4d94ff"
                                        : liquidGlassDarkTheme ? "#99c8ff"
                                        : graphiteTheme ? "#e1e7eb"
                                        : auroraTheme ? "#99ebd0"
                                        : midnightTheme ? "#a8c6ff"
                                        : "#9fe4ff"
    readonly property color accentSecondary: solarizedLightTheme ? "#6c71c4"
                                              : liquidGlassLightTheme ? "#7c3aed"
                                              : draculaTheme ? "#ff79c6"
                                              : tokyoNightTheme ? "#bb9af7"
                                              : nordFrostTheme ? "#81a1c1"
                                              : sentinelClassicTheme ? "#a371f7"
                                              : liquidGlassDarkTheme ? "#a78bfa"
                                              : graphiteTheme ? "#9db1bd"
                                              : auroraTheme ? "#8bb9ff"
                                              : midnightTheme ? "#a78bfa"
                                              : "#83aaf5"
    readonly property color accentTertiary: solarizedLightTheme ? "#2aa198"
                                            : liquidGlassLightTheme ? "#a855f7"
                                            : draculaTheme ? "#50fa7b"
                                            : tokyoNightTheme ? "#9ece6a"
                                            : nordFrostTheme ? "#a3be8c"
                                            : sentinelClassicTheme ? "#39c5cf"
                                            : liquidGlassDarkTheme ? "#c084fc"
                                            : graphiteTheme ? "#e2e8f0"
                                            : auroraTheme ? "#81e6d9"
                                            : midnightTheme ? "#b4c6ff"
                                            : "#9be4ff"
    readonly property color accentBorder: lightTheme ? "#264f8ef7" : "#26c8ecff"
    readonly property color accentBorderSubtle: lightTheme ? "#0d000000" : "#0dffffff"
    readonly property color accentBorderSoft: lightTheme ? "#1a4f8ef7" : "#1a9bdfff"
    readonly property color focusBorder: highContrast
                                       ? (lightTheme ? "#000000" : "#ffffff")
                                       : lightTheme ? "#994f8ef7"
                                       : "#669bdfff"
    readonly property color success: lightTheme ? "#10b981" : "#9ff0d0"
    readonly property color successBorder: lightTheme ? "#3310b981" : "#339ff0d0"
    readonly property color errorBorder: liquidGlassLightTheme ? "#66ef4444" : "#66d66b6b"
    readonly property color separator: highContrast
                                     ? (liquidGlassLightTheme ? "#33000000" : "#33ffffff")
                                     : liquidGlassLightTheme ? "#0f000000" : "#0fffffff"
    readonly property color glowSoft: liquidGlassLightTheme ? "#2b4f8ef7" : sentinelClassicTheme ? "#2b2f81f7" : "#2b9bdfff"
    readonly property color glowStrong: liquidGlassLightTheme ? "#594f8ef7" : sentinelClassicTheme ? "#4d2f81f7" : "#599bdfff"
    readonly property color glassSoft: liquidGlassLightTheme ? "#48ffffff" : sentinelClassicTheme ? "#06ffffff" : "#09ffffff"
    readonly property color glassStrong: liquidGlassLightTheme ? "#a0ffffff" : sentinelClassicTheme ? "#12ffffff" : "#12ffffff"
    readonly property color orbitalLine: liquidGlassLightTheme ? "#164f8ef7" : sentinelClassicTheme ? "#2e2f81f7" : "#16bfefff"
    readonly property color bracketLine: liquidGlassLightTheme ? "#2e4f8ef7" : sentinelClassicTheme ? "#2e2f81f7" : "#2ebfefff"
    readonly property color warning: "#e7b76a"
    readonly property color warningText: liquidGlassLightTheme ? "#7c4a00" : sentinelClassicTheme ? "#191410" : "#18120a"
    readonly property color ambientCyan: liquidGlassLightTheme ? "#0ea5e9" : sentinelClassicTheme ? "#38bdf8" : "#65dfff"
    readonly property color ambientTeal: liquidGlassLightTheme ? "#14b8a6" : sentinelClassicTheme ? "#2dd4bf" : "#7fffd4"
    readonly property color ambientViolet: liquidGlassLightTheme ? "#7c3aed" : sentinelClassicTheme ? "#a78bfa" : "#8bb8ff"

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

    property string fontFamily: Qt.platform.os === "osx" || Qt.platform.os === "macos" ? ".AppleSystemUIFont" : (Qt.platform.os === "windows" ? "Segoe UI" : "sans-serif")
    readonly property string iconFontFamily: Qt.platform.os === "osx" || Qt.platform.os === "macos" ? "Apple Color Emoji" : (Qt.platform.os === "windows" ? "Segoe UI Emoji" : "sans-serif")

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

    // Elevation / shadow system
    readonly property int shadowElevationNone: 0
    readonly property int shadowElevationCard: 4
    readonly property int shadowElevationPanel: 8
    readonly property int shadowElevationModal: 16
    readonly property int shadowElevationDock: 12

    readonly property real shadowOpacityCard: liquidGlassLightTheme ? 0.10 : sentinelClassicTheme ? 0.16 : 0.35
    readonly property real shadowOpacityPanel: liquidGlassLightTheme ? 0.12 : sentinelClassicTheme ? 0.22 : 0.45
    readonly property real shadowOpacityModal: liquidGlassLightTheme ? 0.18 : sentinelClassicTheme ? 0.30 : 0.55
    readonly property real shadowOpacityDock: liquidGlassLightTheme ? 0.12 : sentinelClassicTheme ? 0.20 : 0.40

    readonly property real shadowBlurCard: 8
    readonly property real shadowBlurPanel: 16
    readonly property real shadowBlurModal: 32
    readonly property real shadowBlurDock: 24

    // Blur radius tokens for live glass
    readonly property int blurRadiusGlass: 12
    readonly property int blurRadiusStrong: 24
    readonly property int blurRadiusSubtle: 6
    readonly property int durationAmbient: 3200
    readonly property int durationOrbit: 22000
    readonly property int easingStandard: Easing.InOutQuad
    readonly property int easingEmphasized: Easing.OutCubic
    readonly property color calmAccent: accent
    readonly property color calmAccentHover: accentHover
    readonly property color calmAccentBorder: liquidGlassLightTheme ? "#304f8ef7" : sentinelClassicTheme ? "#4d2f81f7" : "#307eb8ff"
    readonly property color calmFocusGlow: liquidGlassLightTheme ? "#424f8ef7" : sentinelClassicTheme ? "#4d2f81f7" : "#427eb8ff"
    readonly property color cardShadow: liquidGlassLightTheme ? "#14000000" : sentinelClassicTheme ? "#33000000" : "#40000000"
    // Liquid Glass specific tokens
    readonly property color glassBackdrop: liquidGlassLightTheme ? "#70ffffff" : sentinelClassicTheme ? "#cc22262c" : "#0affffff"
    readonly property color glassBackdropStrong: liquidGlassLightTheme ? "#b0ffffff" : sentinelClassicTheme ? "#dd2b3039" : "#16ffffff"
    readonly property color glassBorder: liquidGlassLightTheme ? "#90ffffff" : sentinelClassicTheme ? "#262f81f7" : "#1affffff"
    readonly property color glassInnerGlow: liquidGlassLightTheme ? "#c0ffffff" : sentinelClassicTheme ? "#04ffffff" : "#08ffffff"
    readonly property color glassFrost: liquidGlassLightTheme ? "#90f0f4ff" : sentinelClassicTheme ? "#f0242830" : "#a00a1428"

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
        return liquidGlassLightTheme ? "#f0f4fa8c" : sentinelClassicTheme ? "#f0242830" : "#25364366"
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
