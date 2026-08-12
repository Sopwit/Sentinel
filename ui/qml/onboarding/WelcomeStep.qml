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

    readonly property var features: [
        { icon: "\u2699", title: qsTr("Local-first"), caption: qsTr("Inference runs on your machine with Ollama, LM Studio, or llama.cpp.") },
        { icon: "\uD83D\uDD10", title: qsTr("Private by design"), caption: qsTr("Memory, chat history, and knowledge stay on your device.") },
        { icon: "\u2714", title: qsTr("No telemetry"), caption: qsTr("No hidden uploads, silent updates, or cloud activation.") },
        { icon: "\uD83C\uDF10", title: qsTr("Cross-platform"), caption: qsTr("A portable Qt experience across Linux, Windows, and macOS.") }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
        }

        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: 64
            Layout.preferredHeight: 64
            radius: 32
            color: SentinelTheme.withAlpha(root.brandAccent, 0.12)
            border.color: SentinelTheme.withAlpha(root.brandAccent, 0.35)
            border.width: 1

            Label {
                anchors.centerIn: parent
                text: qsTr("S")
                color: root.brandAccent
                font.pixelSize: 30
                font.bold: true
            }
        }

        Label {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("Welcome to Sentinel")
            color: SentinelTheme.textPrimary
            font.pixelSize: SentinelTheme.fontDisplay
            font.bold: true
        }

        Label {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: 560
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("A calm, private AI assistant built directly into your desktop environment.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontBody
            wrapMode: Text.WordWrap
        }

        Flow {
            Layout.fillWidth: true
            Layout.topMargin: SentinelTheme.spaceLg
            spacing: SentinelTheme.spaceMd
            Layout.alignment: Qt.AlignHCenter

            Repeater {
                model: root.features

                delegate: Rectangle {
                    required property var modelData
                    readonly property int cardWidth: 240

                    width: root.width < 640 ? 200 : cardWidth
                    implicitHeight: 132
                    radius: SentinelTheme.radiusLg
                    color: SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
                    border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: SentinelTheme.spaceLg
                        spacing: SentinelTheme.spaceSm

                        Label {
                            text: modelData.icon
                            color: root.brandAccent
                            font.pixelSize: 22
                        }

                        Label {
                            text: modelData.title
                            color: SentinelTheme.textPrimary
                            font.pixelSize: SentinelTheme.fontBody
                            font.bold: true
                            elide: Text.ElideRight
                            maximumLineCount: 1
                        }

                        Label {
                            Layout.fillWidth: true
                            text: modelData.caption
                            color: SentinelTheme.textMuted
                            font.pixelSize: SentinelTheme.fontSmall
                            wrapMode: Text.WordWrap
                            maximumLineCount: 3
                            elide: Text.ElideRight
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            Layout.maximumWidth: 560
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("A few quick steps will tailor Sentinel to how you work.")
            color: SentinelTheme.textMuted
            font.pixelSize: SentinelTheme.fontSmall
            wrapMode: Text.WordWrap
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0
        }
    }
}
