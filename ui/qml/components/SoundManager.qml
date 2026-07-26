import QtMultimedia
import QtQml
import QtQuick

QtObject {
    id: root

    property bool enabled: true

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
        return ""
    }

    SoundEffect {
        id: notificationSound
        readonly property string primarySource: root.systemSoundPath("notification")
        readonly property string fallbackSource: "qrc:/sounds/notification.wav"
        source: primarySource
        volume: 0.4
    }

    SoundEffect {
        id: errorSound
        readonly property string primarySource: root.systemSoundPath("error")
        readonly property string fallbackSource: "qrc:/sounds/error.wav"
        source: primarySource
        volume: 0.5
    }

    SoundEffect {
        id: successSound
        readonly property string primarySource: root.systemSoundPath("success")
        readonly property string fallbackSource: "qrc:/sounds/success.wav"
        source: primarySource
        volume: 0.3
    }

    function playNotification() {
        if (!root.enabled) return
        if (notificationSound.status !== SoundEffect.Ready && notificationSound.source !== notificationSound.fallbackSource)
            notificationSound.source = notificationSound.fallbackSource
        notificationSound.play()
    }

    function playError() {
        if (!root.enabled) return
        if (errorSound.status !== SoundEffect.Ready && errorSound.source !== errorSound.fallbackSource)
            errorSound.source = errorSound.fallbackSource
        errorSound.play()
    }

    function playSuccess() {
        if (!root.enabled) return
        if (successSound.status !== SoundEffect.Ready && successSound.source !== successSound.fallbackSource)
            successSound.source = successSound.fallbackSource
        successSound.play()
    }
}
