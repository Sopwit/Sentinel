// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtMultimedia
import QtQml
import QtQuick

QtObject {
    id: root

    property bool enabled: true
    readonly property bool soundEffectsAvailable: root.enabled

    function systemSoundPath(name) {
        if (Qt.platform.os === "osx" || Qt.platform.os === "macos") {
            var macSounds = {
                "notification": "file:///System/Library/Sounds/Pop.aiff",
                "error": "file:///System/Library/Sounds/Basso.aiff",
                "success": "file:///System/Library/Sounds/Glass.aiff"
            }
            return macSounds[name] || ""
        }
        if (Qt.platform.os === "linux" || Qt.platform.os === "unix") {
            var linuxSounds = {
                "notification": "file:///usr/share/sounds/freedesktop/stereo/message-new-instant.oga",
                "error": "file:///usr/share/sounds/freedesktop/stereo/dialog-error.oga",
                "success": "file:///usr/share/sounds/freedesktop/stereo/complete.oga"
            }
            return linuxSounds[name] || ""
        }
        if (Qt.platform.os === "windows") {
            var winSounds = {
                "notification": "file:///C:/Windows/Media/Windows Notify System Generic.wav",
                "error": "file:///C:/Windows/Media/Windows Critical Stop.wav",
                "success": "file:///C:/Windows/Media/Windows Ding.wav"
            }
            return winSounds[name] || ""
        }
        return ""
    }

    readonly property string notificationPrimary: root.systemSoundPath("notification")
    readonly property string errorPrimary: root.systemSoundPath("error")
    readonly property string successPrimary: root.systemSoundPath("success")
    readonly property string notificationFallback: "qrc:/sounds/notification.wav"
    readonly property string errorFallback: "qrc:/sounds/error.wav"
    readonly property string successFallback: "qrc:/sounds/success.wav"

    property SoundEffect notificationSound: SoundEffect {
        source: root.notificationPrimary
        volume: 0.4
    }

    property SoundEffect errorSound: SoundEffect {
        source: root.errorPrimary
        volume: 0.5
    }

    property SoundEffect successSound: SoundEffect {
        source: root.successPrimary
        volume: 0.3
    }

    function playNotification() {
        if (!root.enabled) return
        if (notificationSound.status !== SoundEffect.Ready && notificationSound.source.toString() !== notificationFallback)
            notificationSound.source = notificationFallback
        notificationSound.play()
    }

    function playError() {
        if (!root.enabled) return
        if (errorSound.status !== SoundEffect.Ready && errorSound.source.toString() !== errorFallback)
            errorSound.source = errorFallback
        errorSound.play()
    }

    function playSuccess() {
        if (!root.enabled) return
        if (successSound.status !== SoundEffect.Ready && successSound.source.toString() !== successFallback)
            successSound.source = successFallback
        successSound.play()
    }
}
