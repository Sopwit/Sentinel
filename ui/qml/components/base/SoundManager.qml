// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtMultimedia
import QtQml
import QtQuick

QtObject {
    id: root

    property bool enabled: true

    // QSoundEffect only decodes WAV/AIFF sources (QSoundEffect::supportedMimeTypes()),
    // so platform sound themes (freedesktop *.oga) cannot be played through it. Bundled
    // WAV cues are used on every platform to avoid decode failures and the resulting
    // "Error decoding source" warnings.
    readonly property string notificationSource: "qrc:/sounds/notification.wav"
    readonly property string errorSource: "qrc:/sounds/error.wav"
    readonly property string successSource: "qrc:/sounds/success.wav"

    // Effects are created on first use. Creating them eagerly races the platform audio
    // device enumeration during startup: QSoundEffect resolves the default audio output
    // while loading and reports an error when none is available yet.
    property var notificationSound: null
    property var errorSound: null
    property var successSound: null

    readonly property bool soundEffectsAvailable: root.enabled
        && (!notificationSound || notificationSound.status !== SoundEffect.Error)
        && (!errorSound || errorSound.status !== SoundEffect.Error)
        && (!successSound || successSound.status !== SoundEffect.Error)

    readonly property Component effectComponent: Component {
        SoundEffect {}
    }

    function createEffect(source, volume) {
        return effectComponent.createObject(root, {"source": source, "volume": volume})
    }

    function playNotification() {
        if (!root.enabled) return
        if (!notificationSound)
            notificationSound = createEffect(notificationSource, 0.4)
        if (notificationSound.status !== SoundEffect.Error)
            notificationSound.play()
    }

    function playError() {
        if (!root.enabled) return
        if (!errorSound)
            errorSound = createEffect(errorSource, 0.5)
        if (errorSound.status !== SoundEffect.Error)
            errorSound.play()
    }

    function playSuccess() {
        if (!root.enabled) return
        if (!successSound)
            successSound = createEffect(successSource, 0.3)
        if (successSound.status !== SoundEffect.Error)
            successSound.play()
    }
}
