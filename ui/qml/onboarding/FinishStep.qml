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

                Rectangle {
                    Layout.preferredWidth: 44
                    Layout.preferredHeight: 44
                    radius: 22
                    color: SentinelTheme.withAlpha(root.brandAccent, 0.12)
                    border.color: SentinelTheme.withAlpha(root.brandAccent, 0.35)
                    border.width: 1

                    Label {
                        anchors.centerIn: parent
                        text: qsTr("S")
                        color: root.brandAccent
                        font.pixelSize: 21
                        font.bold: true
                    }
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
            label: qsTr("Use Case")
            value: root.viewModel.onboardingUseCase ? root.viewModel.onboardingUseCase : qsTr("General Assistant")
            Layout.fillWidth: true
        }

        InfoRow {
            compact: false
            label: qsTr("Provider")
            value: root.viewModel.onboardingAiProvider ? root.viewModel.onboardingAiProvider : root.viewModel.activeRuntimeProviderLabel
            Layout.fillWidth: true
        }

        InfoRow {
            compact: false
            label: qsTr("Model")
            value: root.viewModel.activeLocalModelName ? root.viewModel.activeLocalModelName : root.viewModel.selectedLocalModel
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
