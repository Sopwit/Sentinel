// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Effects
import Sentinel.Desktop

Rectangle {
    id: rail
    property bool connected: false
    property string connectionStatus: ""
    required property string currentPage
    signal pageRequested(string pageName)
    width: 76
    color: SentinelTheme.surface
    Accessible.role: Accessible.Pane
    Accessible.name: qsTr("Main navigation")

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12
        Repeater {
            model: [
                { page: "Dashboard", label: qsTr("Home"), icon: "home" },
                { page: "Models", label: qsTr("Models"), icon: "brain" },
                { page: "Inspector", label: qsTr("Inspector"), icon: "eye" }
            ]
            delegate: Button {
                id: navigationButton
                objectName: modelData.page + "NavigationButton"
                required property var modelData
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                focusPolicy: Qt.StrongFocus
                hoverEnabled: true
                Accessible.name: modelData.label
                checkable: true
                checked: rail.currentPage === modelData.page
                onClicked: rail.pageRequested(modelData.page)
                ToolTip.visible: hovered || activeFocus
                ToolTip.text: modelData.label
                contentItem: Item {
                    Image {
                        anchors.centerIn: parent
                        source: "qrc:/icons/tabler/" + navigationButton.modelData.icon + ".svg"
                        width: 24; height: 24
                        sourceSize.width: 24; sourceSize.height: 24
                        layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
                        layer.effect: MultiEffect { colorization: 1.0; colorizationColor: navigationButton.checked ? SentinelTheme.accent : SentinelTheme.textMuted }
                    }
                }
                background: Rectangle {
                    radius: 14
                    color: navigationButton.checked || navigationButton.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.12) : "transparent"
                    border.width: navigationButton.activeFocus ? 2 : 0
                    border.color: SentinelTheme.accent
                }
            }
        }
        Item { Layout.fillHeight: true }
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 12; implicitHeight: 12; radius: 6
            color: /loading|connecting|starting|waiting/i.test(rail.connectionStatus) ? SentinelTheme.warning
                : rail.connected ? SentinelTheme.success : SentinelTheme.error
            Accessible.name: rail.connectionStatus
            ToolTip.visible: statusHover.hovered
            ToolTip.text: rail.connectionStatus
            HoverHandler { id: statusHover }
        }
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.12)
        }
        Button {
            id: settingsButton
            objectName: "settingsNavigationButton"
            Layout.fillWidth: true
            Layout.preferredHeight: 52
            focusPolicy: Qt.StrongFocus
            hoverEnabled: true
            Accessible.name: qsTr("Settings")
            onClicked: rail.pageRequested("Settings")
            ToolTip.visible: hovered || activeFocus
            ToolTip.text: qsTr("Settings")
            contentItem: Item {
                Image {
                    anchors.centerIn: parent
                    source: "qrc:/icons/tabler/settings.svg"
                    width: 24; height: 24
                    sourceSize.width: 24; sourceSize.height: 24
                    layer.enabled: GraphicsInfo.api !== GraphicsInfo.Software
                    layer.effect: MultiEffect { colorization: 1.0; colorizationColor: rail.currentPage === "Settings" ? SentinelTheme.accent : SentinelTheme.textMuted }
                }
            }
            background: Rectangle {
                radius: 14
                color: rail.currentPage === "Settings" || settingsButton.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.12) : "transparent"
                border.width: settingsButton.activeFocus ? 2 : 0
                border.color: SentinelTheme.accent
            }
        }
    }
}
