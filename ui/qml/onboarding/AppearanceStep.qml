// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
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

    implicitHeight: layout.implicitHeight

    ColumnLayout {
        id: layout
        anchors.fill: parent
        spacing: SentinelTheme.spaceMd

        Label {
            text: qsTr("Language & Appearance")
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontDisplay
            font.bold: true
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Choose your preferred display language and theme preset.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontBody
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Language")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 100
            }

            ComboBox {
                id: langCombo
                Layout.fillWidth: true
                model: root.viewModel.availableLanguages
                currentIndex: {
                    var idx = root.viewModel.availableLanguages.indexOf(root.viewModel.appLanguage)
                    return idx >= 0 ? idx : 0
                }
                displayText: root.viewModel.languageDisplayName(model[currentIndex] || root.viewModel.appLanguage)
                onActivated: (index) => {
                    var langs = root.viewModel.availableLanguages
                    if (index >= 0 && index < langs.length) {
                        root.viewModel.appLanguage = langs[index]
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Label {
                text: qsTr("Theme")
                color: SentinelTheme.textPrimary
                font.pixelSize: SentinelTheme.fontBody
                Layout.preferredWidth: 100
            }

            ComboBox {
                id: themeCombo
                Layout.fillWidth: true
                model: [qsTr("Dark (Default)"), qsTr("Light"), qsTr("System")]
                currentIndex: 0
            }
        }
    }
}
