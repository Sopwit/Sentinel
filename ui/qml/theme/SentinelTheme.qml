pragma Singleton

import QtQuick

QtObject {
    readonly property color backgroundBase: "#071113"
    readonly property color backgroundRaised: "#0b2428"
    readonly property color backgroundDeep: "#03100f"
    readonly property color panel: "#0d1d20e8"
    readonly property color panelMuted: "#081719aa"
    readonly property color panelStrong: "#0b2022cc"
    readonly property color surface: "#102326"
    readonly property color surfaceSoft: "#10232688"
    readonly property color surfaceMuted: "#102326aa"
    readonly property color metricSurface: "#12292dcc"
    readonly property color userMessageSurface: "#173331"
    readonly property color errorSurface: "#33191a"

    readonly property color textPrimary: "#d9fff4"
    readonly property color textMuted: "#82aaa1"
    readonly property color textPlaceholder: "#648c83"
    readonly property color textOnAccent: "#06110f"

    readonly property color accent: "#35f2c0"
    readonly property color accentBorder: "#35f2c044"
    readonly property color accentBorderSubtle: "#35f2c022"
    readonly property color accentBorderSoft: "#35f2c02d"
    readonly property color successBorder: "#7be8c733"
    readonly property color errorBorder: "#d66b6b66"

    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 14
    readonly property int spaceLg: 18
    readonly property int spaceXl: 22

    readonly property int radiusSm: 12
    readonly property int radiusMd: 14
    readonly property int radiusLg: 16
    readonly property int radiusXl: 18
    readonly property int radiusPanel: 22

    readonly property int fontTiny: 11
    readonly property int fontSmall: 12
    readonly property int fontBody: 13
    readonly property int fontControl: 14
    readonly property int fontCard: 18
    readonly property int fontTitle: 22
    readonly property int fontBrand: 24
    readonly property int fontHeader: 27
}
