// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Effects
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import Sentinel.Desktop

Window {
    id: panel
    objectName: "sentinelQuickPanel"
    required property var viewModel
    required property var controller
    readonly property var snapshot: controller.state
    title: qsTr("Sentinel Quick Panel")
    width: 380
    height: Math.min(650, Screen.desktopAvailableHeight - 24)
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: SentinelTheme.backgroundBase
    function focusPrompt() { prompt.forceActiveFocus() }
    function toggleVisibility() { nativeDesktop.togglePanel() }
    onActiveChanged: if (!active && visible) hide()
    onVisibleChanged: viewModel.companionChatVisible = visible
    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: SentinelTheme.accentBorderSoft
    }
    ScrollView {
        anchors.fill: parent
        anchors.margins: 16
        contentWidth: availableWidth
        ColumnLayout {
            width: parent.width
            spacing: 10
            RowLayout {
                Image { source: "qrc:/branding/sentinel-lockup.svg"; Layout.preferredWidth: 130; Layout.preferredHeight: 30; fillMode: Image.PreserveAspectFit; layer.enabled: true; layer.effect: MultiEffect { brightness: 1; colorization: 1; colorizationColor: SentinelTheme.textPrimary } }
                Label { text: panel.snapshot.connection; color: SentinelTheme.textPrimary; Layout.fillWidth: true; horizontalAlignment: Text.AlignRight }
            }
            TextField {
                id: prompt
                Layout.fillWidth: true
                placeholderText: qsTr("Ask Sentinel…")
                enabled: panel.snapshot.ready && !panel.snapshot.busy
                selectByMouse: true
                Accessible.name: qsTr("Quick prompt")
                onAccepted: if (panel.controller.ask(text)) text = ""
                color: SentinelTheme.textPrimary
                background: Rectangle { color: SentinelTheme.backgroundRaised; radius: 6; border.width: 2; border.color: prompt.activeFocus ? SentinelTheme.accent : SentinelTheme.accentBorderSoft }
            }
            RowLayout {
                Button { text: qsTr("Ask"); enabled: prompt.enabled && prompt.text.trim().length > 0; onClicked: if (panel.controller.ask(prompt.text)) prompt.text = "" }
                Button { text: qsTr("Quick Agent"); enabled: prompt.enabled && prompt.text.trim().length > 0; onClicked: if (panel.controller.agent(prompt.text)) prompt.text = "" }
                Button { text: panel.snapshot.voiceState === "listening" ? qsTr("Stop PTT") : qsTr("Voice/PTT"); enabled: panel.snapshot.voiceAvailable && !panel.snapshot.busy; Accessible.name: text; onClicked: panel.controller.voiceAction(panel.snapshot.voiceState === "listening" ? "stop" : "start") }
                Button { text: qsTr("Cancel voice"); visible: ["listening", "transcribing", "requestingpermission", "startingcapture"].indexOf(panel.snapshot.voiceState) >= 0; enabled: panel.snapshot.ready; onClicked: panel.controller.voiceAction("cancel") }
            }
            ProgressBar { Layout.fillWidth: true; visible: panel.snapshot.voiceState === "listening"; from: 0; to: 1; value: panel.snapshot.voiceLevel || 0; Accessible.name: qsTr("Microphone input level") }
            Label { Layout.fillWidth: true; visible: (panel.snapshot.voiceTranscript || "").length > 0; text: panel.snapshot.voiceTranscript || ""; color: SentinelTheme.textPrimary; wrapMode: Text.Wrap }
            Button { text: qsTr("Use transcript"); visible: (panel.snapshot.voiceTranscript || "").length > 0; enabled: prompt.enabled; onClicked: prompt.text = panel.snapshot.voiceTranscript }
            Label { Layout.fillWidth: true; text: panel.snapshot.voice; color: SentinelTheme.textMuted; wrapMode: Text.Wrap; font.pixelSize: 11 }
            Label { Layout.fillWidth: true; text: qsTr("Run: %1").arg(panel.snapshot.state || "Idle"); color: SentinelTheme.textPrimary }
            Label { Layout.fillWidth: true; text: qsTr("Workspace: %1").arg(panel.snapshot.workspace || "Unavailable"); color: SentinelTheme.textMuted; elide: Text.ElideRight }
            Label { Layout.fillWidth: true; text: qsTr("Model: %1").arg(panel.snapshot.model); color: SentinelTheme.textMuted; elide: Text.ElideRight }
            Label { Layout.fillWidth: true; text: qsTr("Network: %1").arg(panel.snapshot.network || "Unavailable"); color: SentinelTheme.textMuted }
            Label { Layout.fillWidth: true; visible: panel.snapshot.kind === "agent" && panel.snapshot.busy && text.length > 0; text: panel.snapshot.activity || ""; color: SentinelTheme.textMuted; wrapMode: Text.Wrap; maximumLineCount: 2; elide: Text.ElideRight }
            Label { Layout.fillWidth: true; text: qsTr("Pending approvals: %1").arg(panel.snapshot.approvalCount); color: SentinelTheme.textPrimary }
            ColumnLayout {
                Layout.fillWidth: true
                visible: panel.snapshot.approvalCount > 0
                Label { Layout.fillWidth: true; text: (panel.snapshot.approval.tool || qsTr("Requested action")) + " · " + ([qsTr("Low risk"), qsTr("Medium risk"), qsTr("High risk")][panel.snapshot.approval.risk] || qsTr("Unknown risk")) + "\n" + (panel.snapshot.approval.resources || []).map(function(item) { return item.resource || "" }).join(", ") + "\n" + (panel.snapshot.approval.detail || qsTr("Review in Sentinel")); color: SentinelTheme.textPrimary; wrapMode: Text.Wrap; maximumLineCount: 4; elide: Text.ElideRight }
                RowLayout {
                    Button { text: qsTr("Allow Once"); enabled: panel.snapshot.ready && !panel.snapshot.approvalPending; onClicked: panel.controller.approve(true) }
                    Button { text: qsTr("Deny"); enabled: panel.snapshot.ready && !panel.snapshot.approvalPending; onClicked: panel.controller.approve(false) }
                    Button { text: qsTr("Open"); onClicked: { panel.controller.openApproval(); panel.hide() } }
                }
            }
            Label { Layout.fillWidth: true; visible: text.length > 0; text: panel.controller.error; color: SentinelTheme.error; wrapMode: Text.Wrap }
            TextArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 110
                readOnly: true
                visible: panel.snapshot.preview.length > 0
                text: panel.snapshot.preview
                wrapMode: TextEdit.Wrap
                color: SentinelTheme.textPrimary
                Accessible.name: qsTr("Latest response preview")
                background: Rectangle { color: SentinelTheme.backgroundRaised; radius: 6 }
            }
            Button { text: qsTr("Cancel"); visible: panel.snapshot.busy; enabled: panel.snapshot.ready; onClicked: panel.controller.cancel() }
            Button { Layout.fillWidth: true; text: qsTr("Continue last conversation"); enabled: panel.snapshot.ready && panel.snapshot.sessionId.length > 0; onClicked: { panel.controller.continueConversation(); panel.hide() } }
            RowLayout {
                Button { text: qsTr("Open Sentinel"); onClicked: { panel.viewModel.requestWindowActive("Dashboard"); panel.hide() } }
                Button { text: qsTr("Settings"); onClicked: { panel.viewModel.requestWindowActive("Settings"); panel.hide() } }
                Button { text: qsTr("Quit"); onClicked: Qt.quit() }
            }
        }
    }
}
