// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: root
    required property var viewModel
    property color brandAccent: SentinelTheme.modeAccent(viewModel.currentModeName)

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

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("Language & Appearance")
            subtitle: qsTr("Choose your preferred display language and theme preset.")
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Language")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelComboBox {
                id: langCombo
                accent: root.brandAccent
                Layout.fillWidth: true
                implicitHeight: 38
                model: root.viewModel.availableLanguages
                currentIndex: root.viewModel.availableLanguages.indexOf(root.viewModel.appLanguage)
                textRole: ""
                displayText: root.viewModel.languageDisplayName(currentValue)
                delegateTextResolver: (code) => root.viewModel.languageDisplayName(code)
                onActivated: root.viewModel.appLanguage = currentValue
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("Theme")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            SentinelComboBox {
                id: themeCombo
                accent: root.brandAccent
                Layout.fillWidth: true
                implicitHeight: 38
                model: root.themeChoices
                currentIndex: root.themeChoices.indexOf(root.viewModel.themeName)
                displayText: root.localizedThemeName(currentIndex >= 0 ? model[currentIndex] : root.viewModel.themeName)
                delegateTextResolver: (label) => root.localizedThemeName(label)
                onActivated: (index) => root.viewModel.themeName = model[index]
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceSm
            text: qsTr("Theme takes effect immediately and can be changed later in Settings.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
        }

        SectionTitle {
            title: qsTr("Accessibility")
            subtitle: qsTr("Motion, contrast, and density preferences apply across the interface.")
            Layout.fillWidth: true
        }

        OnboardingToggle {
            Layout.fillWidth: true
            accent: root.brandAccent
            label: qsTr("Reduced Motion")
            caption: qsTr("Disables all animations and transitions throughout the UI.")
            checked: root.viewModel.reducedMotionEnabled
            onToggled: (on) => root.viewModel.reducedMotionEnabled = on
        }

        OnboardingToggle {
            Layout.fillWidth: true
            accent: root.brandAccent
            label: qsTr("High Contrast")
            caption: qsTr("Increases text and border contrast for better readability.")
            checked: root.viewModel.highContrastEnabled
            onToggled: (on) => root.viewModel.highContrastEnabled = on
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                Layout.preferredWidth: 132
                Layout.alignment: Qt.AlignVCenter
                text: qsTr("UI Density")
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 38
                radius: SentinelTheme.radiusMd
                color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.48)
                border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 2
                    spacing: 2

                    Repeater {
                        model: root.densityChoices

                        delegate: Button {
                            id: densityBtn
                            required property string modelData
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
                                       ? SentinelTheme.withAlpha(root.brandAccent, 0.16)
                                       : densityBtn.hovered
                                         ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.04)
                                         : "transparent"
                                border.color: (root.viewModel.uiDensity === densityBtn.modelData)
                                              ? SentinelTheme.withAlpha(root.brandAccent, 0.36)
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
