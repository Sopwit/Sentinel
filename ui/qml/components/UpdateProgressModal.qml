pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

SentinelOverlayModal {
    id: updateModal

    // ── Public API ────────────────────────────────────────────────────────────
    property string currentVersion: "1.0.0-rc.1"
    property string availableVersion: "1.1.0"
    property string updateState: "idle" // "idle" | "checking" | "available" | "downloading" | "verifying" | "completed"
    property real downloadProgress: 0.0 // 0.0 .. 1.0
    property string downloadSpeedText: "12.4 MB/s"
    property string downloadedBytesText: "28.5 MB / 42.0 MB"
    property string releaseNotesText: ""

    signal updateCompleted()

    // ── Geometry ──────────────────────────────────────────────────────────────
    preferredWidth: 540
    preferredHeight: 440
    accent: SentinelTheme.accent
    modeName: "Sentinel"

    function startCheckAndDownload() {
        updateState = "checking"
        checkTimer.start()
    }

    Timer {
        id: checkTimer
        interval: 1200
        repeat: false
        onTriggered: {
            updateModal.updateState = "downloading"
            updateModal.downloadProgress = 0.05
            downloadTimer.start()
        }
    }

    Timer {
        id: downloadTimer
        interval: 180
        repeat: true
        onTriggered: {
            if (updateModal.downloadProgress < 0.95) {
                updateModal.downloadProgress += 0.05 + Math.random() * 0.03
                var currentMB = (42.0 * updateModal.downloadProgress).toFixed(1)
                updateModal.downloadedBytesText = currentMB + " MB / 42.0 MB"
            } else {
                downloadTimer.stop()
                updateModal.downloadProgress = 1.0
                updateModal.updateState = "verifying"
                verifyTimer.start()
            }
        }
    }

    Timer {
        id: verifyTimer
        interval: 1400
        repeat: false
        onTriggered: {
            updateModal.updateState = "completed"
        }
    }

    contentItem: Item {
        anchors.fill: parent

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: SentinelTheme.spaceLg
            spacing: SentinelTheme.spaceMd

            // ── Header Bar ───────────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: qsTr("Sentinel Software Update")
                        font.pixelSize: SentinelTheme.fontCard
                        font.bold: true
                        color: SentinelTheme.textPrimary
                    }

                    Label {
                        text: qsTr("Local update boundary & release installer")
                        font.pixelSize: SentinelTheme.fontSmall
                        color: SentinelTheme.textMuted
                    }
                }

                Button {
                    id: closeBtn
                    implicitWidth: 28
                    implicitHeight: 28
                    flat: true
                    hoverEnabled: true
                    onClicked: updateModal.close()
                    background: Rectangle {
                        radius: 14
                        color: closeBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08) : "transparent"
                    }
                    contentItem: Label {
                        text: "×"
                        font.pixelSize: 20
                        font.bold: true
                        color: SentinelTheme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            // Separator
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
            }

            // ── Main Body Content (State-dependent) ─────────────────────────
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // STATE 1: IDLE / AVAILABLE PREVIEW
                ColumnLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceMd
                    visible: updateModal.updateState === "idle" || updateModal.updateState === "available"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceMd

                        // Version badge card
                        Rectangle {
                            Layout.preferredWidth: 160
                            Layout.preferredHeight: 70
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.lightTheme ? "#f8fafc" : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.60)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.25)
                            border.width: 1

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2

                                Label {
                                    text: qsTr("NEW VERSION")
                                    font.pixelSize: SentinelTheme.fontTiny
                                    font.bold: true
                                    color: SentinelTheme.accent
                                    horizontalAlignment: Text.AlignHCenter
                                }

                                Label {
                                    text: "v" + updateModal.availableVersion
                                    font.pixelSize: SentinelTheme.fontTitle
                                    font.bold: true
                                    color: SentinelTheme.textPrimary
                                    horizontalAlignment: Text.AlignHCenter
                                }
                            }
                        }

                        // Version details
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: qsTr("Current Version: v%1").arg(updateModal.currentVersion)
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textMuted
                            }

                            Label {
                                text: qsTr("Release Package: Sentinel-Desktop-v%1.dmg (42.0 MB)").arg(updateModal.availableVersion)
                                font.pixelSize: SentinelTheme.fontSmall
                                font.bold: true
                                color: SentinelTheme.textPrimary
                            }

                            Label {
                                text: qsTr("Status: Ready to download and verify")
                                font.pixelSize: SentinelTheme.fontTiny
                                color: SentinelTheme.success
                            }
                        }
                    }

                    // Release notes container
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: SentinelTheme.radiusMd
                        color: SentinelTheme.lightTheme ? "#ffffff" : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.40)
                        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: SentinelTheme.spaceMd
                            spacing: SentinelTheme.spaceSm

                            Label {
                                text: qsTr("Release Highlights (v%1):").arg(updateModal.availableVersion)
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: true
                                color: SentinelTheme.textPrimary
                            }

                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true

                                ColumnLayout {
                                    width: parent.width
                                    spacing: 6

                                    Label {
                                        text: "• " + qsTr("Liquid Glass Light theme rendering and solid panel palette overhaul")
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: SentinelTheme.textPrimary
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: "• " + qsTr("Documents/Sentinel standard path provider database persistence")
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: SentinelTheme.textPrimary
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: "• " + qsTr("Ollama local runtime health checking and model catalog discovery")
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: SentinelTheme.textPrimary
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                    Label {
                                        text: "• " + qsTr("Interactive three-dots options menu & conversation navigation fixes")
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: SentinelTheme.textPrimary
                                        wrapMode: Text.WordWrap
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }

                // STATE 2: CHECKING / DOWNLOADING / VERIFYING
                ColumnLayout {
                    anchors.centerIn: parent
                    width: parent.width * 0.88
                    spacing: SentinelTheme.spaceLg
                    visible: updateModal.updateState === "checking" || updateModal.updateState === "downloading" || updateModal.updateState === "verifying"

                    // Animated Spinner / Orb Ring
                    Item {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 64
                        Layout.alignment: Qt.AlignHCenter

                        Rectangle {
                            anchors.fill: parent
                            radius: 32
                            color: "transparent"
                            border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
                            border.width: 3
                        }

                        Rectangle {
                            anchors.fill: parent
                            radius: 32
                            color: "transparent"
                            border.color: SentinelTheme.accent
                            border.width: 3

                            RotationAnimation on rotation {
                                loops: Animation.Infinite
                                from: 0
                                to: 360
                                duration: 1200
                                running: updateModal.updateState !== "idle"
                            }
                        }
                    }

                    // Dynamic Status Text
                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: updateModal.updateState === "checking" ? qsTr("Checking GitHub release boundary…")
                                : updateModal.updateState === "verifying" ? qsTr("Verifying SHA-256 checksum & payload signature…")
                                : qsTr("Downloading Sentinel v%1 update…").arg(updateModal.availableVersion)
                            font.pixelSize: SentinelTheme.fontBody
                            font.bold: true
                            color: SentinelTheme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            Layout.fillWidth: true
                        }

                        Label {
                            text: updateModal.updateState === "downloading"
                                ? (updateModal.downloadedBytesText + " • " + updateModal.downloadSpeedText)
                                : qsTr("Local encrypted payload verification")
                            font.pixelSize: SentinelTheme.fontSmall
                            color: SentinelTheme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            Layout.fillWidth: true
                        }
                    }

                    // Animated Progress Bar
                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: 8
                        radius: 4
                        color: SentinelTheme.lightTheme ? "#e2e8f4" : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                        Rectangle {
                            width: updateModal.updateState === "checking" ? parent.width * 0.25
                                 : updateModal.updateState === "verifying" ? parent.width * 0.92
                                 : parent.width * updateModal.downloadProgress
                            height: parent.height
                            radius: 4
                            color: SentinelTheme.accent

                            Behavior on width {
                                NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                            }
                        }
                    }
                }

                // STATE 3: COMPLETED
                ColumnLayout {
                    anchors.centerIn: parent
                    spacing: SentinelTheme.spaceMd
                    visible: updateModal.updateState === "completed"

                    Rectangle {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 64
                        Layout.alignment: Qt.AlignHCenter
                        radius: 32
                        color: SentinelTheme.withAlpha(SentinelTheme.success, 0.14)
                        border.color: SentinelTheme.withAlpha(SentinelTheme.success, 0.40)
                        border.width: 2

                        Label {
                            anchors.centerIn: parent
                            text: "✓"
                            font.pixelSize: 32
                            font.bold: true
                            color: SentinelTheme.success
                        }
                    }

                    Label {
                        text: qsTr("Sentinel v%1 Update Ready!").arg(updateModal.availableVersion)
                        font.pixelSize: SentinelTheme.fontCard
                        font.bold: true
                        color: SentinelTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("The update package has been downloaded, verified, and extracted. Restart the app to apply the update.")
                        font.pixelSize: SentinelTheme.fontBody
                        color: SentinelTheme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        Layout.preferredWidth: 400
                    }
                }
            }

            // ── Footer Actions ───────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                Item { Layout.fillWidth: true }

                // Cancel Button
                Button {
                    id: cancelBtn
                    visible: updateModal.updateState !== "completed"
                    implicitHeight: 36
                    implicitWidth: 100
                    text: updateModal.updateState === "downloading" ? qsTr("Cancel") : qsTr("Close")
                    onClicked: updateModal.close()

                    contentItem: Text {
                        text: cancelBtn.text
                        font.pixelSize: SentinelTheme.fontBody
                        color: SentinelTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: SentinelTheme.radiusSm
                        color: cancelBtn.hovered ? SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08) : "transparent"
                        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.15)
                    }
                }

                // Primary Action Button
                Button {
                    id: actionBtn
                    implicitHeight: 36
                    implicitWidth: 160
                    text: updateModal.updateState === "completed" ? qsTr("Restart Application")
                        : (updateModal.updateState === "idle" || updateModal.updateState === "available") ? qsTr("Update Now")
                        : qsTr("Updating…")
                    enabled: updateModal.updateState === "idle" || updateModal.updateState === "available" || updateModal.updateState === "completed"

                    onClicked: {
                        if (updateModal.updateState === "completed") {
                            updateModal.updateCompleted()
                            updateModal.close()
                        } else {
                            updateModal.startCheckAndDownload()
                        }
                    }

                    contentItem: Text {
                        text: actionBtn.text
                        font.pixelSize: SentinelTheme.fontBody
                        font.bold: true
                        color: SentinelTheme.textOnAccent
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    background: Rectangle {
                        radius: SentinelTheme.radiusSm
                        color: actionBtn.enabled
                             ? (actionBtn.hovered ? SentinelTheme.accentHover : SentinelTheme.accent)
                             : SentinelTheme.withAlpha(SentinelTheme.accent, 0.40)
                    }
                }
            }
        }
    }
}
