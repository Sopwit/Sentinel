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
    readonly property var themeChoices: ["Liquid Glass Light", "Liquid Glass Dark", "Sentinel Classic", "Midnight Blue", "Aurora Teal", "Graphite Grey", "Solarized Light", "Nord Frost", "Dracula", "Tokyo Night"]
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
    implicitHeight: visible ? generalSection.implicitHeight + appearanceSection.implicitHeight + accessibilitySection.implicitHeight : 0

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    Column {
        width: parent.width
        spacing: 0

        Item {
            id: generalSection
            width: parent.width
            height: implicitHeight
            implicitHeight: root.sectionHeight(generalContent)

            ColumnLayout {
                id: generalContent
                x: root.panelPadding
                y: root.panelPadding
                width: parent.width - root.panelPadding * 2
                spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: qsTr("General")
                    subtitle: qsTr("Desktop shell preferences.")
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceMd

                    Label {
                        Layout.preferredWidth: root.compact ? 88 : 132
                        Layout.alignment: Qt.AlignVCenter
                        text: qsTr("Language")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    SentinelComboBox {
                        id: languageCombo
                        accent: root.modeAccent
                        Layout.fillWidth: true
                        implicitHeight: 36
                        model: root.viewModel.availableLanguages
                        currentIndex: root.viewModel.availableLanguages.indexOf(root.viewModel.appLanguage)
                        textRole: ""
                        displayText: root.viewModel.languageDisplayName(currentValue)
                        delegateTextResolver: (code) => root.viewModel.languageDisplayName(code)
                        onActivated: root.viewModel.appLanguage = currentValue
                    }
                }
            }
        }

        Item {
            id: appearanceSection
            width: parent.width
            height: implicitHeight
            implicitHeight: root.sectionHeight(appearanceContent)

            ColumnLayout {
                id: appearanceContent
                x: root.panelPadding
                y: root.panelPadding
                width: parent.width - root.panelPadding * 2
                spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: qsTr("Appearance")
                    subtitle: qsTr("Theme foundation for a calm native desktop UI.")
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceMd

                    Label {
                        Layout.preferredWidth: root.compact ? 88 : 132
                        Layout.alignment: Qt.AlignVCenter
                        text: qsTr("Theme")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    SentinelComboBox {
                        id: themeCombo
                        accent: root.modeAccent
                        Layout.fillWidth: true
                        implicitHeight: 36
                        model: root.themeChoices
                        currentIndex: root.themeChoices.indexOf(root.viewModel.themeName)
                        displayText: root.localizedThemeName(currentIndex >= 0 ? model[currentIndex] : root.viewModel.themeName)
                        delegateTextResolver: (label) => root.localizedThemeName(label)
                        onActivated: (index) => root.viewModel.themeName = model[index]
                    }
                }

                Label {
                    text: qsTr("Visual Theme Presets")
                    color: SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontBody
                    font.bold: true
                    Layout.fillWidth: true
                    Layout.topMargin: SentinelTheme.spaceMd
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceMd
                    Layout.bottomMargin: SentinelTheme.spaceMd

                    Repeater {
                        model: root.themeChoices

                        delegate: Button {
                            id: themeCard
                            required property string modelData
                            readonly property bool isSelected: root.viewModel.themeName === modelData

                            implicitWidth: root.compact ? 130 : 160
                            implicitHeight: 90
                            hoverEnabled: true

                            onClicked: root.viewModel.themeName = modelData

                            background: Rectangle {
                                id: themeCardBg
                                radius: SentinelTheme.radiusLg
                                color: themeCard.isSelected
                                       ? SentinelTheme.withAlpha(root.modeAccent, 0.12)
                                       : themeCard.hovered
                                         ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.03)
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
                                    height: 40
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
                                    font.bold: themeCard.isSelected
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

        Item {
            id: accessibilitySection
            width: parent.width
            height: implicitHeight
            implicitHeight: root.sectionHeight(accessibilityContent)

            ColumnLayout {
                id: accessibilityContent
                x: root.panelPadding
                y: root.panelPadding
                width: parent.width - root.panelPadding * 2
                spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: qsTr("Accessibility")
                    subtitle: qsTr("Comfort, motion, contrast, and density preferences. Changes take effect immediately.")
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm
                    Layout.topMargin: SentinelTheme.spaceXs
                    Layout.bottomMargin: SentinelTheme.spaceXs

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Reduced Motion")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontBody
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Disables all animations and transitions throughout the UI.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                            wrapMode: Text.WordWrap
                        }
                    }

                    Switch {
                        id: reducedMotionSwitch
                        checked: root.viewModel.reducedMotionEnabled
                        hoverEnabled: true
                        onToggled: root.viewModel.reducedMotionEnabled = checked

                        indicator: Rectangle {
                            implicitWidth: 46
                            implicitHeight: 24
                            x: reducedMotionSwitch.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: height / 2
                            color: reducedMotionSwitch.checked
                                   ? SentinelTheme.withAlpha(root.modeAccent, 0.18)
                                   : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                            border.color: reducedMotionSwitch.activeFocus
                                          ? SentinelTheme.withAlpha(root.modeAccent, 0.46)
                                          : reducedMotionSwitch.hovered
                                            ? SentinelTheme.withAlpha(root.modeAccent, 0.24)
                                          : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                            Rectangle {
                                x: reducedMotionSwitch.checked ? parent.width - width - 3 : 3
                                y: parent.height / 2 - height / 2
                                width: 18; height: 18
                                radius: height / 2
                                color: reducedMotionSwitch.checked
                                       ? root.modeAccent
                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.40)
                            }
                        }

                        background: Item {}
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceSm
                    Layout.topMargin: SentinelTheme.spaceXs
                    Layout.bottomMargin: SentinelTheme.spaceXs

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 2

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("High Contrast")
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontBody
                            font.bold: true
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("Increases text and border contrast for better readability.")
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                            wrapMode: Text.WordWrap
                        }
                    }

                    Switch {
                        id: highContrastSwitch
                        checked: root.viewModel.highContrastEnabled
                        hoverEnabled: true
                        onToggled: root.viewModel.highContrastEnabled = checked

                        indicator: Rectangle {
                            implicitWidth: 46
                            implicitHeight: 24
                            x: highContrastSwitch.leftPadding
                            y: parent.height / 2 - height / 2
                            radius: height / 2
                            color: highContrastSwitch.checked
                                   ? SentinelTheme.withAlpha(root.modeAccent, 0.18)
                                   : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.060)
                            border.color: highContrastSwitch.activeFocus
                                          ? SentinelTheme.withAlpha(root.modeAccent, 0.46)
                                          : highContrastSwitch.hovered
                                            ? SentinelTheme.withAlpha(root.modeAccent, 0.24)
                                          : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                            Rectangle {
                                x: highContrastSwitch.checked ? parent.width - width - 3 : 3
                                y: parent.height / 2 - height / 2
                                width: 18; height: 18
                                radius: height / 2
                                color: highContrastSwitch.checked
                                       ? root.modeAccent
                                       : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.40)
                            }
                        }

                        background: Item {}
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceMd

                    Label {
                        Layout.preferredWidth: root.compact ? 88 : 132
                        Layout.alignment: Qt.AlignVCenter
                        text: qsTr("UI Density")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 36
                        radius: SentinelTheme.radiusMd
                        color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
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
                                        font.bold: (root.viewModel.uiDensity === densityBtn.modelData)
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    background: Rectangle {
                                        radius: SentinelTheme.radiusSm
                                        color: (root.viewModel.uiDensity === densityBtn.modelData)
                                               ? SentinelTheme.withAlpha(root.modeAccent, 0.16)
                                               : densityBtn.hovered
                                                 ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                                 : "transparent"
                                        border.color: (root.viewModel.uiDensity === densityBtn.modelData)
                                                      ? SentinelTheme.withAlpha(root.modeAccent, 0.36)
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
}
