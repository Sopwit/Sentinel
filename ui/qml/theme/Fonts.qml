import QtQml
import QtQuick

QtObject {
    id: root

    property bool loaded: false
    property string family: SentinelTheme.fontFamily

    readonly property var primary: FontLoader {
        id: primaryLoader
        source: "qrc:/fonts/Inter/Inter-Variable.ttf"
        onStatusChanged: {
            if (status === FontLoader.Ready) {
                root.family = name
                root.loaded = true
            }
        }
    }

    readonly property var monospace: FontLoader {
        id: monoLoader
        source: "qrc:/fonts/IBM_Plex_Mono/IBMPlexMono-Regular.ttf"
    }
}
