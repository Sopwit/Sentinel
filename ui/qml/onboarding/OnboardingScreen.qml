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

    // ── Ambient background ──────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: SentinelTheme.backgroundBase
    }

    Rectangle {
        anchors.fill: parent
        gradient: Gradient {
            GradientStop { position: 0.0; color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.050) }
            GradientStop { position: 0.5; color: "transparent" }
            GradientStop { position: 1.0; color: SentinelTheme.withAlpha(SentinelTheme.accentSecondary, 0.030) }
        }
    }

    Rectangle {
        id: ambientGlowA
        width: parent.width * 0.42
        height: parent.height * 0.42
        radius: width / 2
        color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.035)

        SequentialAnimation on x {
            running: !onboarding.reducedMotion
            loops: Animation.Infinite
            NumberAnimation { from: -parent.width * 0.12; to: parent.width * 0.55; duration: 18000; easing.type: Easing.InOutSine }
            NumberAnimation { from: parent.width * 0.55; to: -parent.width * 0.12; duration: 18000; easing.type: Easing.InOutSine }
        }
        SequentialAnimation on y {
            running: !onboarding.reducedMotion
            loops: Animation.Infinite
            NumberAnimation { from: -parent.height * 0.12; to: parent.height * 0.45; duration: 24000; easing.type: Easing.InOutSine }
            NumberAnimation { from: parent.height * 0.45; to: -parent.height * 0.12; duration: 24000; easing.type: Easing.InOutSine }
        }
        SequentialAnimation on opacity {
            running: !onboarding.reducedMotion
            loops: Animation.Infinite
            NumberAnimation { from: 0.0; to: 0.7; duration: 7000; easing.type: Easing.InOutSine }
            PauseAnimation { duration: 5000 }
            NumberAnimation { from: 0.7; to: 0.0; duration: 7000; easing.type: Easing.InOutSine }
            PauseAnimation { duration: 5000 }
        }
    }

    Rectangle {
        id: ambientGlowB
        width: parent.width * 0.30
        height: parent.height * 0.30
        radius: width / 2
        color: SentinelTheme.withAlpha(SentinelTheme.accentSecondary, 0.030)

        SequentialAnimation on x {
            running: !onboarding.reducedMotion
            loops: Animation.Infinite
            NumberAnimation { from: parent.width * 0.6; to: parent.width * 0.05; duration: 21000; easing.type: Easing.InOutSine }
            NumberAnimation { from: parent.width * 0.05; to: parent.width * 0.6; duration: 21000; easing.type: Easing.InOutSine }
        }
        SequentialAnimation on y {
            running: !onboarding.reducedMotion
            loops: Animation.Infinite
            NumberAnimation { from: parent.height * 0.5; to: parent.height * 0.0; duration: 26000; easing.type: Easing.InOutSine }
            NumberAnimation { from: parent.height * 0.0; to: parent.height * 0.5; duration: 26000; easing.type: Easing.InOutSine }
        }
        SequentialAnimation on opacity {
            running: !onboarding.reducedMotion
            loops: Animation.Infinite
            NumberAnimation { from: 0.0; to: 0.6; duration: 8000; easing.type: Easing.InOutSine }
            PauseAnimation { duration: 6000 }
            NumberAnimation { from: 0.6; to: 0.0; duration: 8000; easing.type: Easing.InOutSine }
            PauseAnimation { duration: 6000 }
        }
    }

    // ── Main layout ─────────────────────────────────────────────────────────
    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: SentinelTheme.space2Xl
        anchors.bottomMargin: SentinelTheme.space2Xl
        anchors.leftMargin: SentinelTheme.space4Xl
        anchors.rightMargin: SentinelTheme.space4Xl
        spacing: SentinelTheme.spaceLg

        // Brand header
        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceMd

            Rectangle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                radius: 22
                color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.12)
                border.color: SentinelTheme.withAlpha(onboarding.brandAccent, 0.35)
                border.width: 1

                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: SentinelTheme.withAlpha(onboarding.brandAccent, 0.30)
                    shadowBlur: 0.35
                    shadowVerticalOffset: 1
                    shadowOpacity: 1.0
                }

                Label {
                    anchors.centerIn: parent
                    text: qsTr("S")
                    color: onboarding.brandAccent
                    font.pixelSize: 21
                    font.bold: true
                }
            }

            ColumnLayout {
                spacing: 0
                Label {
                    text: qsTr("Sentinel")
                    color: SentinelTheme.textPrimary
                    font.pixelSize: SentinelTheme.fontCard
                    font.bold: true
                }
                Label {
                    text: qsTr("Welcome & Setup")
                    color: SentinelTheme.textMuted
                    font.pixelSize: SentinelTheme.fontSmall
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: qsTr("Step %1 of %2").arg(onboarding.step + 1).arg(onboarding.totalSteps)
                color: SentinelTheme.textMuted
                font.pixelSize: SentinelTheme.fontSmall
            }
        }

        // Step indicator dots
        RowLayout {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm

            Repeater {
                model: onboarding.totalSteps

                Rectangle {
                    required property int index
                    readonly property bool isCurrent: index === onboarding.step
                    readonly property bool isDone: index < onboarding.step

                    Layout.preferredWidth: isCurrent ? 34 : 10
                    Layout.preferredHeight: 10
                    radius: 5
                    color: isCurrent ? onboarding.brandAccent
                         : isDone ? SentinelTheme.withAlpha(onboarding.brandAccent, 0.45)
                         : SentinelTheme.withAlpha(SentinelTheme.textMuted, 0.25)

                    Behavior on Layout.preferredWidth {
                        NumberAnimation { duration: MotionTokens.normal }
                    }
                    Behavior on color {
                        ColorAnimation { duration: MotionTokens.fast }
                    }
                }
            }
        }

        // Content card
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 0

            Rectangle {
                id: contentCard
                anchors.fill: parent
                radius: SentinelTheme.radiusPanel
                color: SentinelTheme.lightTheme
                       ? SentinelTheme.backgroundRaised
                       : SentinelTheme.withAlpha(SentinelTheme.panel, 0.92)
                border.color: SentinelTheme.separator
                border.width: 1
                clip: true

                layer.enabled: true
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.14)
                    shadowBlur: 0.45
                    shadowVerticalOffset: 4
                    shadowOpacity: 1.0
                }

                StackLayout {
                    id: stepStack
                    anchors.fill: parent
                    anchors.margins: SentinelTheme.space2Xl
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
            }
        }

        // Footer navigation
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
                premium: true
                implicitWidth: 140
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
