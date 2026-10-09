// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import Sentinel.Desktop

Item {
    id: dashboardPage
    signal voiceSettingsRequested()
    required property var viewModel
    function focusComposer() { homeChatSurface.focusComposer() }
    function restoreDraft(text) { homeChatSurface.restoreDraft(text) }
    HomeChatSurface {
        id: homeChatSurface
        anchors.fill: parent
        viewModel: dashboardPage.viewModel
        onVoiceSettingsRequested: dashboardPage.voiceSettingsRequested()
    }
}
