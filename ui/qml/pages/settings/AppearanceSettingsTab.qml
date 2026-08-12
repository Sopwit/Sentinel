// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root
    required property var viewModel
    property bool compact: false
    property color modeAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property int panelPadding: SentinelTheme.spaceLg
    readonly property var themeChoices: ["Liquid Glass Light", "Liquid Glass Dark", "Sentinel Classic", "Midnight Blue", "Aurora Teal", "Graphite Grey", "Nord Frost", "Dracula"]
    readonly property var densityChoices: ["Compact", "Comfortable", "Large"]

    function localizedThemeName(key) {
        switch (key) {
        case "Liquid Glass Light": return qsTr("Liquid Glass Light")
        case "Liquid Glass Dark": return qsTr("Liquid Glass Dark")
        case "Sentinel Classic": return qsTr("Sentinel Classic")
        case "Midnight Blue": return qsTr("Midnight Blue")
        case "Aurora Teal": return qsTr("Aurora Teal")
        case "Graphite Grey": return qsTr("Graphite Grey")
        case "Solarized Light": return qsTr("Solarized Light")
        case "Nord Frost": return qsTr("Nord Frost")
        case "Dracula": return qsTr("Dracula")
        case "Tokyo Night": return qsTr("Tokyo Night")
        default: return key ? key : ""
        }
    }

    function localizedDensityName(key) {
        switch (key) {
        case "Compact": return qsTr("Compact")
        case "Comfortable": return qsTr("Comfortable")
        case "Large": return qsTr("Large")
        default: return key ? key : ""
        }
    }

    height: implicitHeight
    implicitHeight: visible ? mainLayout.implicitHeight + panelPadding * 2 : 0

    ColumnLayout {
        id: mainLayout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: root.panelPadding
        spacing: SentinelTheme.spaceMd

        SectionTitle {
            title: qsTr("General")
            subtitle: qsTr("Desktop shell and localization preferences.")
            Layout.fillWidth: true
        }

        SettingCard {
            SettingControlRow {
                title: qsTr("Language")
                subtitle: qsTr("Application interface display language.")
                accent: root.modeAccent
                compact: root.compact

                SentinelComboBox {
                    id: languageCombo
                    accent: root.modeAccent
                    anchors.fill: parent
                    implicitHeight: 36
                    model: root.viewModel.availableLanguages
                    currentIndex: root.viewModel.availableLanguages.indexOf(root.viewModel.appLanguage)
                    textRole: ""
                    displayText: root.viewModel.languageDisplayName(currentIndex >= 0 ? model[currentIndex] : root.viewModel.appLanguage)
                    delegateTextResolver: (code) => root.viewModel.languageDisplayName(code)
                    onActivated: (index) => root.viewModel.appLanguage = model[index]
                }
            }
        }

        SectionTitle {
            title: qsTr("Appearance")
            subtitle: qsTr("Theme foundation and visual presets for desktop UI.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingControlRow {
                title: qsTr("Active Theme")
                subtitle: qsTr("Primary color theme palette.")
                accent: root.modeAccent
                compact: root.compact

                SentinelComboBox {
                    id: themeCombo
                    accent: root.modeAccent
                    anchors.fill: parent
                    implicitHeight: 36
                    model: root.themeChoices
                    currentIndex: root.themeChoices.indexOf(root.viewModel.themeName)
                    displayText: root.localizedThemeName(currentIndex >= 0 ? model[currentIndex] : root.viewModel.themeName)
                    delegateTextResolver: (label) => root.localizedThemeName(label)
                    onActivated: (index) => root.viewModel.themeName = model[index]
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: SentinelTheme.spaceMd
                Layout.rightMargin: SentinelTheme.spaceMd
                Layout.topMargin: SentinelTheme.spaceSm
                Layout.bottomMargin: SentinelTheme.spaceSm
                spacing: SentinelTheme.spaceSm

                Label {
                    text: qsTr("Visual Theme Presets")
                    color: SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontBody
                    font.weight: Font.Medium
                    Layout.fillWidth: true
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.compact ? 2 : 4
                    columnSpacing: SentinelTheme.spaceMd
                    rowSpacing: SentinelTheme.spaceMd

                    Repeater {
                        model: root.themeChoices

                        delegate: Button {
                            id: themeCard
                            required property string modelData
                            readonly property bool isSelected: root.viewModel.themeName === modelData

                            Layout.fillWidth: true
                            implicitHeight: 88
                            hoverEnabled: true

                            onClicked: root.viewModel.themeName = modelData

                            background: Rectangle {
                                id: themeCardBg
                                radius: SentinelTheme.radiusLg
                                color: themeCard.isSelected
                                       ? SentinelTheme.withAlpha(root.modeAccent, 0.14)
                                       : themeCard.hovered
                                         ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                         : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
                                border.color: themeCard.isSelected
                                               ? root.modeAccent
                                               : themeCard.hovered
                                                 ? SentinelTheme.withAlpha(root.modeAccent, 0.30)
                                                 : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                                border.width: themeCard.isSelected ? 2 : 1

                                Behavior on border.color { ColorAnimation { duration: MotionTokens.fast } }
                                Behavior on color { ColorAnimation { duration: MotionTokens.fast } }

                                Rectangle {
                                    id: palettePreview
                                    anchors.top: parent.top
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.margins: SentinelTheme.spaceSm
                                    height: 38
                                    radius: SentinelTheme.radiusMd
                                    clip: true

                                    color: {
                                        if (modelData === "Liquid Glass Light") return "#f4f6f9"
                                        if (modelData === "Liquid Glass Dark") return "#0d1117"
                                        if (modelData === "Sentinel Classic") return "#1b1f24"
                                        if (modelData === "Midnight Blue") return "#0a0f1e"
                                        if (modelData === "Aurora Teal") return "#0f1a1c"
                                        if (modelData === "Graphite Grey") return "#121416"
                                        return "#141721"
                                    }

                                    Rectangle {
                                        width: 12
                                        height: 12
                                        radius: 6
                                        anchors.right: parent.right
                                        anchors.bottom: parent.bottom
                                        anchors.margins: 6
                                        color: {
                                            if (modelData === "Liquid Glass Light") return "#4f8ef7"
                                            if (modelData === "Liquid Glass Dark") return "#7eb8ff"
                                            if (modelData === "Sentinel Classic") return "#2f81f7"
                                            if (modelData === "Midnight Blue") return "#8fb4ff"
                                            if (modelData === "Aurora Teal") return "#7de0b9"
                                            if (modelData === "Graphite Grey") return "#d0d7dc"
                                            return root.modeAccent
                                        }
                                    }

                                    RowLayout {
                                        anchors.left: parent.left
                                        anchors.top: parent.top
                                        anchors.margins: 6
                                        spacing: 4
                                        Rectangle { width: 16; height: 4; radius: 2; color: themeCard.isSelected ? "#ffffff" : SentinelTheme.textMuted }
                                        Rectangle { width: 8; height: 4; radius: 2; color: themeCard.isSelected ? "#ffffff" : SentinelTheme.textPlaceholder }
                                    }
                                }

                                Label {
                                    anchors.bottom: parent.bottom
                                    anchors.left: parent.left
                                    anchors.right: parent.right
                                    anchors.margins: SentinelTheme.spaceSm
                                    text: root.localizedThemeName(modelData)
                                    color: themeCard.isSelected ? SentinelTheme.textPrimary : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    font.weight: themeCard.isSelected ? Font.DemiBold : Font.Normal
                                    horizontalAlignment: Text.AlignHCenter
                                    elide: Text.ElideRight
                                    maximumLineCount: 1
                                }
                            }
                        }
                    }
                }
            }
        }

        SectionTitle {
            title: qsTr("Accessibility")
            subtitle: qsTr("Comfort, motion, contrast, and density preferences.")
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
        }

        SettingCard {
            SettingToggleRow {
                title: qsTr("Reduced Motion")
                subtitle: qsTr("Disables all animations and transitions throughout the UI.")
                checked: root.viewModel.reducedMotionEnabled
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.reducedMotionEnabled = checked
            }

            SettingToggleRow {
                title: qsTr("High Contrast")
                subtitle: qsTr("Increases text and border contrast for better readability.")
                checked: root.viewModel.highContrastEnabled
                accent: root.modeAccent
                compact: root.compact
                showDivider: true
                onToggled: (checked) => root.viewModel.highContrastEnabled = checked
            }

            SettingControlRow {
                title: qsTr("UI Density")
                subtitle: qsTr("Scale layout density for compact or spacious controls.")
                accent: root.modeAccent
                compact: root.compact
                controlWidth: root.compact ? 180 : 260

                Rectangle {
                    anchors.fill: parent
                    implicitHeight: 34
                    radius: SentinelTheme.radiusMd
                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.60)
                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 2
                        spacing: 2

                        Repeater {
                            model: root.densityChoices

                            Button {
                                id: densityBtn
                                required property string modelData
                                required property int index
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                hoverEnabled: true
                                focusPolicy: Qt.NoFocus

                                contentItem: Text {
                                    text: root.localizedDensityName(densityBtn.modelData)
                                    color: (root.viewModel.uiDensity === densityBtn.modelData)
                                           ? SentinelTheme.textPrimary
                                           : SentinelTheme.textMuted
                                    font.pixelSize: SentinelTheme.fontSmall
                                    font.weight: (root.viewModel.uiDensity === densityBtn.modelData) ? Font.DemiBold : Font.Normal
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }

                                background: Rectangle {
                                    radius: SentinelTheme.radiusSm
                                    color: (root.viewModel.uiDensity === densityBtn.modelData)
                                           ? SentinelTheme.withAlpha(root.modeAccent, 0.20)
                                           : densityBtn.hovered
                                             ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                             : "transparent"
                                    border.color: (root.viewModel.uiDensity === densityBtn.modelData)
                                                  ? SentinelTheme.withAlpha(root.modeAccent, 0.40)
                                                  : "transparent"
                                }

                                onClicked: root.viewModel.uiDensity = densityBtn.modelData
                            }
                        }
                    }
                }
            }
        }
    }
}
