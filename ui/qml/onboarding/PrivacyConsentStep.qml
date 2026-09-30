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

    readonly property var assurances: [
        qsTr("Memory, chat history, and local retrieval data use local storage."),
        qsTr("Cloud model requests send prompt context to the selected provider when cloud use is configured."),
        qsTr("Tool execution remains subject to authorization and sandbox policy."),
        qsTr("Raw microphone recordings are not retained by default."),
        qsTr("A workspace root gives context; it does not grant file or process access.")
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: SentinelTheme.spaceLg

        SectionTitle {
            title: qsTr("Privacy summary")
            subtitle: qsTr("Processing and network use depend on the providers and features you enable.")
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

                        TablerGlyph {
                            anchors.centerIn: parent
                            text: "check"
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

    }
}
