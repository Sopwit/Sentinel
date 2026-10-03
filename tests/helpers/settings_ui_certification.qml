// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ApplicationWindow {
    id: window
    width: 1100
    height: 780
    visible: true
    title: "Sentinel — disposable certification profile"
    color: SentinelTheme.backgroundBase
    property int surface: 0
    ColumnLayout {
        anchors.fill: parent
        RowLayout {
            Button { text: "Settings"; onClicked: surface = 0 }
            Button { text: "Onboarding"; onClicked: { shellViewModel.reopenOnboarding(); surface = 1; onboarding.active = true } }
            Button { text: "Chat"; onClicked: surface = 2 }
            Button { text: "780 px"; onClicked: { window.width = 780; window.height = 640 } }
        }
        SettingsPage { viewModel: shellViewModel; visible: surface === 0; Layout.fillWidth: true; Layout.fillHeight: true }
        HomeChatSurface { viewModel: shellViewModel; visible: surface === 2; Layout.fillWidth: true; Layout.fillHeight: true }
    }
    OnboardingScreen {
        id: onboarding
        anchors.fill: parent
        viewModel: shellViewModel
        active: !shellViewModel.onboardingComplete || surface === 1
        onFinished: surface = 0
    }
}
