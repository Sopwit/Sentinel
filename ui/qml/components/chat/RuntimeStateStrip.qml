// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

Flow {
    id: stateStrip
    required property var viewModel
    property color accent: SentinelTheme.accent

    readonly property bool streaming: viewModel.conversationRuntimeStreaming
                                      || viewModel.localInferenceBusy
                                      || viewModel.chatSendLifecycleState === "streaming"
    readonly property string errorText: viewModel.globalErrorVisible
                                       ? viewModel.globalErrorMessage
                                       : viewModel.conversationRuntimeLastErrorSummary === "No error or refusal yet."
                                         ? "" : viewModel.conversationRuntimeLastErrorSummary

    width: parent ? parent.width : implicitWidth
    spacing: SentinelTheme.spaceXs

    StatusChip {
        label: qsTr("Provider")
        value: stateStrip.viewModel.activeRuntimeProviderLabel
        accent: stateStrip.viewModel.activeRuntimeReadinessState === "Ready"
                ? SentinelTheme.success : stateStrip.accent
        muted: stateStrip.viewModel.activeRuntimeProviderLabel.length === 0
    }

    StatusChip {
        label: qsTr("Readiness")
        value: stateStrip.viewModel.activeRuntimeReadinessState
        accent: stateStrip.viewModel.activeRuntimeReadinessState === "Ready"
                ? SentinelTheme.success : SentinelTheme.warning
        muted: false
    }

    StatusChip {
        label: qsTr("Health")
        value: stateStrip.viewModel.localRuntimeHealth
        accent: stateStrip.viewModel.localRuntimeHealth === "Healthy"
                ? SentinelTheme.success : SentinelTheme.textMuted
        muted: stateStrip.viewModel.localRuntimeHealth.length === 0
    }

    StatusChip {
        label: qsTr("Stream")
        value: stateStrip.streaming ? qsTr("Streaming")
                                    : stateStrip.viewModel.localInferenceStreamStatus
        accent: stateStrip.streaming ? SentinelTheme.accent : SentinelTheme.textMuted
        active: stateStrip.streaming
        muted: !stateStrip.streaming
    }

    StatusChip {
        label: qsTr("Tools")
        value: stateStrip.viewModel.latestToolExecutionStatus
        accent: SentinelTheme.accentTertiary
        muted: stateStrip.viewModel.latestToolExecutionStatus.length === 0
    }

    Label {
        width: stateStrip.width
        visible: stateStrip.errorText.length > 0
        text: stateStrip.errorText
        color: SentinelTheme.error
        font.pixelSize: SentinelTheme.fontSmall
        elide: Text.ElideRight
        maximumLineCount: 1
    }
}
