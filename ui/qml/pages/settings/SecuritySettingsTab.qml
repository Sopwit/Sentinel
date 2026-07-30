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

    height: implicitHeight
    implicitHeight: visible ? permissionsSection.implicitHeight : 0

    function sectionHeight(content) {
        return content.implicitHeight + panelPadding * 2
    }

    Column {
        width: parent.width
        spacing: 0

        Item {
            id: permissionsSection
            width: parent.width
            height: implicitHeight
            implicitHeight: root.sectionHeight(permissionsContent)

            ColumnLayout {
                id: permissionsContent
                x: root.panelPadding
                y: root.panelPadding
                width: parent.width - root.panelPadding * 2
                spacing: SentinelTheme.spaceSm

                SectionTitle {
                    title: qsTr("Security & Agent Boundaries")
                    subtitle: qsTr("Configure system-level permission policies and sandbox boundaries for local tool execution.")
                    Layout.fillWidth: true
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: SentinelTheme.spaceMd

                    Label {
                        Layout.preferredWidth: root.compact ? 88 : 132
                        Layout.alignment: Qt.AlignVCenter
                        text: qsTr("Default Policy")
                        color: SentinelTheme.textMuted
                        font.pixelSize: SentinelTheme.fontSmall
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    ComboBox {
                        id: permissionStateCombo
                        Layout.fillWidth: true
                        hoverEnabled: true
                        model: root.viewModel.permissionPolicyStateLabels
                        currentIndex: root.viewModel.permissionPolicyStateLabels.indexOf(root.viewModel.defaultPermissionPolicyState)
                        displayText: currentIndex >= 0 ? currentText : root.viewModel.defaultPermissionPolicyState
                        onActivated: (index) => {
                            if (index >= 0 && index < root.viewModel.permissionPolicyStateLabels.length)
                                root.viewModel.defaultPermissionPolicyState = root.viewModel.permissionPolicyStateLabels[index]
                        }
                    }
                }

                InfoRow {
                    compact: root.compact
                    label: qsTr("Tool Execution Gateway")
                    value: root.viewModel.toolGatewayStatus || qsTr("Inactive")
                    Layout.fillWidth: true
                }
            }
        }
    }
}
