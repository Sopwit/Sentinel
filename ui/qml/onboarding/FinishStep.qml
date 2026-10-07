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

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("You're ready!")
            subtitle: qsTr("Sentinel setup is complete. Click finish to open your workspace.")
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 76
            radius: SentinelTheme.radiusLg
            color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

            RowLayout {
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceLg
                spacing: SentinelTheme.spaceMd

                Image {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 44
                    source: "qrc:/icons/dev.sentinel.Sentinel.png"
                    fillMode: Image.PreserveAspectFit
                }

                ColumnLayout {
                    spacing: 2
                    Label {
                        text: qsTr("Your configuration has been saved locally.")
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontBody
                        font.bold: true
                    }
                    Label {
                        text: qsTr("You can change anything anytime from Settings.")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                    }
                }
            }
        }

        Label {
            Layout.topMargin: SentinelTheme.spaceMd
            text: qsTr("Summary")
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontBody
            font.bold: true
        }

        InfoRow {
            compact: false
            label: qsTr("Processing")
            value: root.viewModel.onboardingProcessingMode === "local" ? qsTr("Local")
                   : root.viewModel.onboardingProcessingMode === "cloud" ? qsTr("Cloud")
                   : qsTr("Hybrid")
            Layout.fillWidth: true
        }

        InfoRow {
            compact: false
            label: qsTr("Provider")
            value: root.viewModel.onboardingSnapshot.providerId || qsTr("Set up later")
            Layout.fillWidth: true
        }

        InfoRow {
            compact: false
            label: qsTr("Model")
            value: root.viewModel.onboardingSnapshot.modelId || qsTr("Set up later")
            Layout.fillWidth: true
        }

        InfoRow {
            compact: false
            label: qsTr("Language")
            value: root.viewModel.languageDisplayName(root.viewModel.appLanguage)
            Layout.fillWidth: true
        }

        InfoRow {
            compact: false
            label: qsTr("Theme")
            value: root.viewModel.themeName ? root.viewModel.themeName : qsTr("Default")
            Layout.fillWidth: true
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
        }

        Label {
            Layout.fillWidth: true
            text: qsTr("Thanks for setting up Sentinel. Your assistant is ready when you are.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontBody
            wrapMode: Text.WordWrap
        }
    }
}
