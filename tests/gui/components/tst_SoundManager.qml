// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtTest
import "../../../ui/qml/components/base" as Base

TestCase {
    id: testCase
    name: "SoundManagerTest"

    property Component managerComponent: Component {
        Base.SoundManager {}
    }

    function createManager() {
        return managerComponent.createObject()
    }

    // QSoundEffect only decodes WAV/AIFF sources. Pointing it at freedesktop *.oga cues
    // produced "Error decoding source" warnings at startup, so the manager must stay on
    // the bundled WAV cues on every platform.
    function test_sources_are_bundled_wav_cues() {
        var manager = createManager()
        compare(manager.notificationSource, "qrc:/sounds/notification.wav")
        compare(manager.errorSource, "qrc:/sounds/error.wav")
        compare(manager.successSource, "qrc:/sounds/success.wav")

        verify(!manager.notificationSource.startsWith("file:"),
               "system sound files must not be handed to QSoundEffect")
        verify(!manager.errorSource.endsWith(".oga"))
        verify(!manager.successSource.endsWith(".oga"))
        manager.destroy()
    }

    function test_disabled_manager_is_unavailable_and_stays_silent() {
        var manager = createManager()
        manager.enabled = false
        tryCompare(manager, "soundEffectsAvailable", false)

        manager.playNotification()
        manager.playError()
        manager.playSuccess()

        compare(manager.notificationSound, null, "disabled manager must not create effects")
        compare(manager.errorSound, null)
        compare(manager.successSound, null)
        manager.destroy()
    }

    function test_play_creates_effects_for_bundled_wav_cues() {
        var manager = createManager()
        manager.playNotification()
        manager.playError()
        manager.playSuccess()

        verify(manager.notificationSound, "play must create the effect")
        verify(manager.errorSound)
        verify(manager.successSound)

        compare(manager.notificationSound.source.toString(),
                "qrc:/sounds/notification.wav")
        compare(manager.errorSound.source.toString(), "qrc:/sounds/error.wav")
        compare(manager.successSound.source.toString(), "qrc:/sounds/success.wav")
        manager.destroy()
    }

    function test_reports_unavailable_when_a_cue_fails_to_load() {
        var manager = createManager()
        manager.playNotification()

        manager.notificationSound.source = "qrc:/sounds/not-a-real-cue.wav"
        tryCompare(manager, "soundEffectsAvailable", false, 3000)
        manager.destroy()
    }
}
