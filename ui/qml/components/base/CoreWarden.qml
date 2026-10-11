// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import QtQuick.Window
import Sentinel.Desktop
import Sentinel.Warden 1.0

ColumnLayout {
    id: root
    required property var viewModel
    property bool presentationEnabled: true
    property bool surfaceActive: true
    property bool neutral: false
    property bool showStatus: true
    property string statusOverride: ""
    function readinessSummary() {
        return player.readinessSummary(viewModel.daemonConnected, viewModel.activeRuntimeReadinessState)
    }
    property string previewState: ""
    property int imageSize: 144
    readonly property bool preview: previewState.length > 0
    readonly property string selectedState: neutral ? "idle" : player.productionState(viewModel.daemonConnected,
        viewModel.agentAwaitingApproval, viewModel.chatSendLifecycleState)
    visible: presentationEnabled && (preview || coreWardenPreferences.mode !== "off")
    spacing: SentinelTheme.spaceXs
    CoreWardenPlayer {
        id: player
        objectName: "coreWardenPlayer"
        visible: root.visible && root.surfaceActive && (!Window.window || (Window.window.visible && Window.window.visibility !== Window.Minimized))
        Layout.alignment: Qt.AlignHCenter
        Layout.preferredWidth: root.imageSize
        Layout.preferredHeight: root.imageSize
        animationState: root.preview ? root.previewState : root.selectedState
        mode: root.preview ? "animated" : coreWardenPreferences.mode
        reducedMotion: root.viewModel.reducedMotionEnabled
        animate: root.preview || (root.viewModel.daemonConnected && root.selectedState !== "idle")
        Accessible.role: Accessible.Graphic
        Accessible.name: qsTr("Core Warden") + " — " + statusText.text
    }
    Label {
        id: statusText
        visible: root.showStatus
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        text: root.statusOverride.length > 0 ? root.statusOverride : root.preview ? qsTr("Synthetic preview: %1").arg(root.previewState)
              : !root.viewModel.daemonConnected ? qsTr("Disconnected")
              : root.selectedState === "idle" ? qsTr("Inactive") : player.statusLabel
        color: SentinelTheme.textMuted
        font.pixelSize: SentinelTheme.fontSmall
        Accessible.name: text
    }
    Label {
        visible: player.assetError.length > 0
        text: qsTr("Artwork unavailable")
        color: SentinelTheme.textMuted
        Layout.fillWidth: true
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }
}
