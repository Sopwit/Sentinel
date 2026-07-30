// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import Sentinel.Desktop

Item {
    id: onboarding
    required property var viewModel

    property int step: 0
    property bool active: false
    readonly property int totalSteps: 9
    readonly property color brandAccent: SentinelTheme.modeAccent(viewModel.currentModeName)
    readonly property bool reducedMotion: viewModel.reducedMotionEnabled

    signal finished()

    opacity: active ? 1.0 : 0.0
    visible: opacity > 0.01
    enabled: active
    z: 300

    readonly property var stepMeta: [
        { key: "welcome",      title: qsTr("Welcome"),      caption: qsTr("A calm assistant built around you.") },
        { key: "privacy",      title: qsTr("Privacy"),      caption: qsTr("Your data stays on your device.") },
        { key: "appearance",   title: qsTr("Appearance"),   caption: qsTr("Make Sentinel feel like home.") },
        { key: "provider",     title: qsTr("AI Provider"),  caption: qsTr("Connect to models, your way.") },
        { key: "model",        title: qsTr("AI Model"),     caption: qsTr("Choose and download a model.") },
        { key: "settings",     title: qsTr("Preferences"),  caption: qsTr("Fine-tune your local assistant.") },
        { key: "voice",        title: qsTr("Voice Setup"),  caption: qsTr("Configure voice engines and models.") },
        { key: "capabilities", title: qsTr("Capabilities"), caption: qsTr("Everything Sentinel can do.") },
        { key: "finish",       title: qsTr("Finish"),       caption: qsTr("You're ready to begin.") }
    ]

    Rectangle {
        anchors.fill: parent
        color: SentinelTheme.backgroundBase

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: SentinelTheme.spaceXl
            spacing: SentinelTheme.spaceLg

            // Header Progress
            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceMd

                Label {
                    text: qsTr("Step %1 of %2").arg(onboarding.step + 1).arg(onboarding.totalSteps)
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                }

                ProgressBar {
                    Layout.fillWidth: true
                    from: 0
                    to: onboarding.totalSteps - 1
                    value: onboarding.step
                }
            }

            // Active Step Content Container
            StackLayout {
                id: stepStack
                Layout.fillWidth: true
                Layout.fillHeight: true
                currentIndex: onboarding.step

                WelcomeStep {
                    viewModel: onboarding.viewModel
                }

                PrivacyConsentStep {
                    viewModel: onboarding.viewModel
                }

                AppearanceStep {
                    viewModel: onboarding.viewModel
                }

                ProviderSetupStep {
                    viewModel: onboarding.viewModel
                }

                ModelSetupStep {
                    viewModel: onboarding.viewModel
                }

                PreferencesStep {
                    viewModel: onboarding.viewModel
                }

                VoiceSetupStep {
                    viewModel: onboarding.viewModel
                }

                CapabilitiesStep {
                    viewModel: onboarding.viewModel
                }

                FinishStep {
                    viewModel: onboarding.viewModel
                }
            }

            // Footer Navigation Buttons
            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceMd

                SentinelButton {
                    text: qsTr("Back")
                    enabled: onboarding.step > 0
                    onClicked: onboarding.step--
                }

                Item { Layout.fillWidth: true }

                SentinelButton {
                    text: onboarding.step === onboarding.totalSteps - 1 ? qsTr("Finish") : qsTr("Next")
                    accent: onboarding.brandAccent
                    onClicked: {
                        if (onboarding.step < onboarding.totalSteps - 1) {
                            onboarding.step++
                        } else {
                            onboarding.viewModel.onboardingComplete = true
                            onboarding.active = false
                            onboarding.finished()
                        }
                    }
                }
            }
        }
    }
}
