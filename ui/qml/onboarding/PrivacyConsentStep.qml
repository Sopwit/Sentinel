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

    readonly property var assurances: [
        qsTr("All memory, chat history, and Local RAG metadata stays on your device."),
        qsTr("No telemetry, hidden uploads, silent updates, or hidden cloud activation."),
        qsTr("Task execution advances only through visible user actions."),
        qsTr("Model downloads go directly to your configured local runtimes."),
        qsTr("Workspace metadata does not grant folder scans or filesystem authority.")
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("Private by design")
            subtitle: qsTr("Sentinel is built so your information stays entirely on your local machine.")
            Layout.fillWidth: true
        }

        Column {
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceMd
            spacing: SentinelTheme.spaceMd

            Repeater {
                model: root.assurances

                delegate: RowLayout {
                    required property string modelData
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceMd

                    Rectangle {
                        Layout.preferredWidth: 24
                        Layout.preferredHeight: 24
                        radius: 12
                        color: SentinelTheme.withAlpha(root.brandAccent, 0.12)

                        Label {
                            anchors.centerIn: parent
                            text: "\u2713"
                            color: root.brandAccent
                            font.pixelSize: SentinelTheme.fontSmall
                            font.bold: true
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData
                        color: SentinelTheme.textPrimary
                        font.pixelSize: SentinelTheme.fontBody
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 96
            radius: SentinelTheme.radiusLg
            color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
            border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
            border.width: 1

            OnboardingToggle {
                anchors.fill: parent
                anchors.margins: SentinelTheme.spaceLg
                accent: root.brandAccent
                label: qsTr("I understand these privacy commitments")
                caption: qsTr("You can review these guarantees anytime in Settings.")
                checked: true
                onToggled: (on) => { /* consent is informational during setup */ }
            }
        }
    }
}
