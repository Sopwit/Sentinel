// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
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
    property bool agentMode: false
    readonly property bool voiceBusy: ["listening", "transcribing", "requestingpermission", "startingcapture"].indexOf(snapshot.voiceState) >= 0
    title: qsTr("Sentinel")
    width: 420
    height: Math.min(content.implicitHeight + 24, Screen.desktopAvailableHeight - 24, 540)
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    color: SentinelTheme.backgroundBase
    readonly property bool detailPopupVisible: statusDetails.visible
    function dismissDetails() { statusDetails.close() }
    function focusPrompt() { prompt.forceActiveFocus() }
    function toggleVisibility() { nativeDesktop.togglePanel() }
    function submit() {
        if (!snapshot.ready || snapshot.busy || !prompt.text.trim()) return
        if ((agentMode ? controller.agent(prompt.text) : controller.ask(prompt.text))) prompt.clear()
    }
    function errorSummary(code) {
        if (code.indexOf("request-timeout") >= 0) return qsTr("The daemon did not reply in time. Check connection details before retrying.")
        if (code.indexOf("provider-unavailable") >= 0) return qsTr("The selected provider is unavailable. Review Models & Providers.")
        if (code.indexOf("model-unavailable") >= 0 || code.indexOf("ModelNotFound") >= 0) return qsTr("The selected model is unavailable. Choose an available model.")
        return code
    }
    Shortcut { sequence: "Escape"; enabled: panel.visible && !statusDetails.visible; onActivated: panel.hide() }
    onVisibleChanged: viewModel.companionChatVisible = visible

    component IconAction: Button {
        id: action
        required property string glyph
        required property string hint
        property bool selected: false
        implicitWidth: 34
        implicitHeight: 34
        hoverEnabled: true
        focusPolicy: Qt.StrongFocus
        Accessible.name: hint
        ToolTip.visible: hovered || activeFocus
        ToolTip.text: hint
        contentItem: TablerGlyph { text: action.glyph; color: action.enabled ? SentinelTheme.textPrimary : SentinelTheme.textMuted; font.pixelSize: 18; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
        background: Rectangle { radius: 9; color: action.selected || action.hovered ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.12) : "transparent"; border.width: action.activeFocus ? 2 : 0; border.color: SentinelTheme.accent }
    }
    Popup {
        id: statusDetails
        objectName: "quickPanelStatusDetails"
        parent: panel.contentItem
        x: 12; y: 48
        width: panel.width - 24
        padding: 12
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        onClosed: prompt.forceActiveFocus()
        background: Rectangle { color: SentinelTheme.backgroundRaised; radius: 10; border.color: SentinelTheme.accentBorderSoft }
        contentItem: ColumnLayout {
            Label { text: qsTr("Connection") + ": " + panel.snapshot.connection; color: SentinelTheme.textPrimary; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: qsTr("Runtime") + ": " + (panel.snapshot.state || qsTr("Unknown")); color: SentinelTheme.textPrimary; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { text: panel.snapshot.model || qsTr("No model confirmed"); color: SentinelTheme.textMuted; wrapMode: Text.Wrap; Layout.fillWidth: true }
            Label { visible: panel.controller.error.length > 0; text: panel.controller.error; color: SentinelTheme.error; wrapMode: Text.WrapAnywhere; Layout.fillWidth: true; Accessible.name: qsTr("Technical error details") }
            SentinelButton { text: qsTr("Close"); onClicked: statusDetails.close() }
        }
    }
    Rectangle { anchors.fill: parent; color: "transparent"; border.color: SentinelTheme.accentBorderSoft; radius: 12 }
    ScrollView {
        id: scroll
        anchors.fill: parent
        anchors.margins: 12
        contentWidth: availableWidth
        clip: true
        ColumnLayout {
            id: content
            width: scroll.availableWidth
            spacing: 8
            RowLayout {
                Layout.fillWidth: true
                Label { text: "Sentinel"; color: SentinelTheme.textPrimary; font.pixelSize: 17; font.bold: true }
                IconAction {
                    objectName: "quickPanelStatus"
                    glyph: "circle"
                    hint: qsTr("Connection details") + ": " + panel.snapshot.connection
                    onClicked: statusDetails.open()
                }
                Label {
                    text: panel.snapshot.connection || qsTr("Disconnected")
                    color: !panel.snapshot.ready ? SentinelTheme.warning : SentinelTheme.textMuted
                    font.pixelSize: 11
                    Layout.maximumWidth: 110
                    elide: Text.ElideRight
                }
                Item { Layout.fillWidth: true }
                IconAction { glyph: "arrow-up-right"; hint: qsTr("Continue in Sentinel"); enabled: panel.snapshot.ready; onClicked: { panel.controller.continueConversation(); panel.hide() } }
                IconAction { glyph: "settings"; hint: qsTr("Settings"); onClicked: { panel.viewModel.requestWindowActive("Settings"); panel.hide() } }
                IconAction { glyph: "x"; hint: qsTr("Close panel"); onClicked: panel.hide() }
            }
            TextArea {
                id: prompt
                objectName: "quickPanelPrompt"
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(140, Math.max(70, contentHeight + 24))
                placeholderText: qsTr("Ask Sentinel…")
                enabled: panel.snapshot.ready && !panel.snapshot.busy
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                color: SentinelTheme.textPrimary
                Accessible.name: qsTr("Quick prompt")
                Keys.onReturnPressed: function(event) {
                    if (event.modifiers & Qt.ShiftModifier) { event.accepted = false; return }
                    event.accepted = true
                    if (text.trim().length > 0) panel.submit()
                }
                background: Rectangle { color: SentinelTheme.backgroundRaised; radius: 10; border.width: prompt.activeFocus ? 2 : 1; border.color: prompt.activeFocus ? SentinelTheme.accent : SentinelTheme.accentBorderSoft }
            }
            RowLayout {
                Layout.fillWidth: true
                IconAction { glyph: "message-circle"; hint: qsTr("Chat"); selected: !panel.agentMode; enabled: !panel.snapshot.busy; onClicked: panel.agentMode = false }
                IconAction { glyph: "sparkles"; hint: qsTr("Agent"); selected: panel.agentMode; enabled: !panel.snapshot.busy; onClicked: panel.agentMode = true }
                IconAction { glyph: panel.snapshot.voiceState === "listening" ? "x" : "microphone"; hint: panel.voiceBusy ? panel.snapshot.voice : qsTr("Voice input"); enabled: panel.snapshot.ready && !panel.snapshot.busy && (panel.snapshot.voiceAvailable || panel.snapshot.voiceState === "listening"); onClicked: panel.controller.voiceAction(panel.snapshot.voiceState === "listening" ? "stop" : "start") }
                IconAction { glyph: "x"; hint: qsTr("Cancel voice"); visible: panel.voiceBusy; onClicked: panel.controller.voiceAction("cancel") }
                Item { Layout.fillWidth: true }
                IconAction { objectName: "quickPanelSend"; glyph: panel.snapshot.busy ? "x" : "arrow-up"; hint: panel.snapshot.busy ? qsTr("Stop") : qsTr("Send"); enabled: panel.snapshot.ready && (panel.snapshot.busy || prompt.text.trim().length > 0); onClicked: panel.snapshot.busy ? panel.controller.cancel() : panel.submit() }
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 6
                SentinelComboBox {
                    objectName: "quickPanelWorkspace"
                    Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 120; Layout.preferredHeight: 32
                    font.pixelSize: 11
                    Accessible.name: qsTr("Workspace")
                    enabled: panel.snapshot.ready && !panel.snapshot.busy
                    model: panel.viewModel.workspaceNames
                    currentIndex: panel.viewModel.workspaceIds.indexOf(panel.viewModel.selectedWorkspaceId)
                    onActivated: function(index) { panel.viewModel.selectedWorkspaceId = panel.viewModel.workspaceIds[index] }
                }
                SentinelComboBox {
                    objectName: "quickPanelProvider"
                    Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 110; Layout.preferredHeight: 32
                    font.pixelSize: 11
                    Accessible.name: qsTr("Provider")
                    enabled: panel.snapshot.ready && !panel.snapshot.busy
                    model: panel.viewModel.selectableRuntimeProviderLabels
                    currentIndex: panel.viewModel.selectableRuntimeProviderIds.indexOf(panel.viewModel.selectedRuntimeProvider)
                    onActivated: function(index) { panel.viewModel.selectedRuntimeProvider = panel.viewModel.selectableRuntimeProviderIds[index] }
                }
                SentinelComboBox {
                    objectName: "quickPanelModel"
                    readonly property var names: panel.viewModel.selectedRuntimeProvider === "lm-studio" ? panel.viewModel.loadedLMStudioModelNames : panel.viewModel.ollamaModelNames
                    Layout.fillWidth: true; Layout.minimumWidth: 0; Layout.preferredWidth: 150; Layout.preferredHeight: 32
                    font.pixelSize: 11
                    Accessible.name: qsTr("Model")
                    enabled: panel.snapshot.ready && !panel.snapshot.busy && names.length > 0
                    model: names
                    currentIndex: names.indexOf(panel.viewModel.selectedLocalModel)
                    displayText: panel.viewModel.selectedLocalModel || qsTr("Choose model")
                    onActivated: function(index) { panel.viewModel.selectedLocalModel = names[index] }
                }
            }
            ProgressBar { Layout.fillWidth: true; visible: panel.snapshot.voiceState === "listening"; value: panel.snapshot.voiceLevel || 0; Accessible.name: qsTr("Microphone input level") }
            RowLayout {
                visible: (panel.snapshot.voiceTranscript || "").length > 0
                Label { Layout.fillWidth: true; text: panel.snapshot.voiceTranscript || ""; color: SentinelTheme.textMuted; maximumLineCount: 2; elide: Text.ElideRight; wrapMode: Text.Wrap }
                IconAction { glyph: "arrow-up"; hint: qsTr("Use transcript"); enabled: prompt.enabled; onClicked: prompt.text = panel.snapshot.voiceTranscript }
            }
            Label { Layout.fillWidth: true; visible: panel.snapshot.voiceState === "failed"; text: panel.snapshot.voice || ""; color: SentinelTheme.error; wrapMode: Text.Wrap }
            ColumnLayout {
                Layout.fillWidth: true
                visible: panel.snapshot.approvalCount > 0
                Label { Layout.fillWidth: true; text: (panel.snapshot.approval.tool || qsTr("Requested action")) + " · " + ([qsTr("Low risk"), qsTr("Medium risk"), qsTr("High risk")][panel.snapshot.approval.risk] || qsTr("Unknown risk")) + "\n" + (panel.snapshot.approval.resources || []).map(function(item) { return item.resource || "" }).join(", ") + "\n" + (panel.snapshot.approval.detail || qsTr("Review in Sentinel")); color: SentinelTheme.textPrimary; wrapMode: Text.Wrap; maximumLineCount: 4; elide: Text.ElideRight }
                RowLayout {
                    SentinelButton { text: qsTr("Allow Once"); enabled: panel.snapshot.ready && !panel.snapshot.approvalPending; onClicked: panel.controller.approve(true) }
                    SentinelButton { text: qsTr("Deny"); enabled: panel.snapshot.ready && !panel.snapshot.approvalPending; onClicked: panel.controller.approve(false) }
                    IconAction { glyph: "arrow-up-right"; hint: qsTr("Review in Sentinel"); onClicked: { panel.controller.openApproval(); panel.hide() } }
                }
            }
            Label { objectName: "quickPanelError"; Layout.fillWidth: true; visible: panel.controller.error.length > 0; text: panel.errorSummary(panel.controller.error); color: SentinelTheme.error; wrapMode: Text.Wrap }
            TextArea {
                objectName: "quickPanelPreview"
                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(140, Math.max(64, contentHeight + 20))
                selectByMouse: true
                readOnly: true
                visible: panel.snapshot.preview.length > 0
                text: panel.snapshot.preview
                wrapMode: TextEdit.Wrap
                color: SentinelTheme.textPrimary
                Accessible.name: qsTr("Latest response preview")
                background: Rectangle { color: SentinelTheme.backgroundRaised; radius: 10 }
            }
        }
    }
}
