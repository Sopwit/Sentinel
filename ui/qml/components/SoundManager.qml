import QtMultimedia
import QtQml
import QtQuick

QtObject {
    id: root

    property bool enabled: true

    readonly property SoundEffect notificationSound: SoundEffect {
        source: "qrc:/sounds/notification.wav"
        volume: 0.4
    }

    readonly property SoundEffect errorSound: SoundEffect {
        source: "qrc:/sounds/error.wav"
        volume: 0.5
    }

    readonly property SoundEffect successSound: SoundEffect {
        source: "qrc:/sounds/success.wav"
        volume: 0.3
    }

    function playNotification() {
        if (root.enabled) notificationSound.play()
    }

    function playError() {
        if (root.enabled) errorSound.play()
    }

    function playSuccess() {
        if (root.enabled) successSound.play()
    }
}
