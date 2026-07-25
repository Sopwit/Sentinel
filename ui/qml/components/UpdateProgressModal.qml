pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

SentinelOverlayModal {
    id: updateModal

    // ── Public API ────────────────────────────────────────────────────────────
    property string currentVersion: "1.0.0"
    property string availableVersion: "1.0.1-alpha"
    property string updateState: "idle" // "idle" | "checking" | "available" | "downloading" | "verifying" | "completed" | "upToDate" | "error"
    property real downloadProgress: 0.0 // 0.0 .. 1.0
    property string downloadSpeedText: "12.4 MB/s"
    property string downloadedBytesText: "28.5 MB / 42.0 MB"
    property string releaseNotesText: ""
    property string errorMessage: ""

    signal updateCompleted()
    signal checkRequested()

    // ── Geometry & Layout ─────────────────────────────────────────────────────
    preferredWidth: 580
    preferredHeight: 520
    accent: SentinelTheme.accent
    modeName: "Sentinel"

    function startCheckAndDownload() {
        errorMessage = ""
        updateState = "checking"
        checkTimer.start()
        checkRequested()
    }

    function cancelUpdate() {
        checkTimer.stop()
        downloadTimer.stop()
        verifyTimer.stop()
        if (updateState === "downloading" || updateState === "checking" || updateState === "verifying") {
            updateState = "idle"
            downloadProgress = 0.0
        }
        updateModal.close()
    }

    Timer {
        id: checkTimer
        interval: 1200
        repeat: false
        onTriggered: {
            if (updateModal.updateState === "checking") {
                updateModal.updateState = "downloading"
                updateModal.downloadProgress = 0.05
                downloadTimer.start()
            }
        }
    }

    Timer {
        id: downloadTimer
        interval: 180
        repeat: true
        onTriggered: {
            if (updateModal.updateState !== "downloading") {
                downloadTimer.stop()
                return
            }

            if (updateModal.downloadProgress < 0.95) {
                updateModal.downloadProgress += 0.05 + Math.random() * 0.03
                var currentMB = (42.0 * Math.min(1.0, updateModal.downloadProgress)).toFixed(1)
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
            if (updateModal.updateState === "verifying") {
                updateModal.updateState = "completed"
            }
        }
    }

    contentItem: Item {
        id: contentRoot
        anchors.fill: parent
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: SentinelTheme.spaceLg
            spacing: SentinelTheme.spaceMd

            // ── Header Bar ───────────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceMd

                // Icon Box
                Rectangle {
                    implicitWidth: 36
                    implicitHeight: 36
                    radius: SentinelTheme.radiusMd
                    color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.12)
                    border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.30)
                    border.width: 1

                    Label {
                        anchors.centerIn: parent
                        text: updateModal.updateState === "completed" || updateModal.updateState === "upToDate" ? "✓"
                            : updateModal.updateState === "error" ? "!"
                            : "⟳"
                        font.pixelSize: 18
                        font.bold: true
                        color: updateModal.updateState === "completed" || updateModal.updateState === "upToDate" ? SentinelTheme.success
                             : updateModal.updateState === "error" ? SentinelTheme.warning
                             : SentinelTheme.accent
                    }
                }

                // Title & Subtitle
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: qsTr("Sentinel Software Update")
                        font.pixelSize: SentinelTheme.fontCard
                        font.bold: true
                        color: SentinelTheme.textPrimary
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("Local update boundary & release installer")
                        font.pixelSize: SentinelTheme.fontSmall
                        color: SentinelTheme.textMuted
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }
                }

                // State Badge Chip
                Rectangle {
                    implicitHeight: 24
                    implicitWidth: stateBadgeLabel.implicitWidth + 16
                    radius: 12
                    color: updateModal.updateState === "completed" || updateModal.updateState === "upToDate" ? SentinelTheme.withAlpha(SentinelTheme.success, 0.15)
                         : updateModal.updateState === "error" ? SentinelTheme.withAlpha(SentinelTheme.warning, 0.15)
                         : updateModal.updateState === "downloading" || updateModal.updateState === "verifying" || updateModal.updateState === "checking" ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.15)
                         : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)

                    border.color: updateModal.updateState === "completed" || updateModal.updateState === "upToDate" ? SentinelTheme.withAlpha(SentinelTheme.success, 0.35)
                                : updateModal.updateState === "error" ? SentinelTheme.withAlpha(SentinelTheme.warning, 0.35)
                                : updateModal.updateState === "downloading" || updateModal.updateState === "verifying" || updateModal.updateState === "checking" ? SentinelTheme.withAlpha(SentinelTheme.accent, 0.35)
                                : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.15)
                    border.width: 1

                    Label {
                        id: stateBadgeLabel
                        anchors.centerIn: parent
                        text: updateModal.updateState === "checking" ? qsTr("Checking…")
                            : updateModal.updateState === "downloading" ? qsTr("Downloading")
                            : updateModal.updateState === "verifying" ? qsTr("Verifying")
                            : updateModal.updateState === "completed" ? qsTr("Ready to Install")
                            : updateModal.updateState === "upToDate" ? qsTr("Up to Date")
                            : updateModal.updateState === "error" ? qsTr("Check Failed")
                            : updateModal.updateState === "available" ? qsTr("v%1 Available").arg(updateModal.availableVersion)
                            : qsTr("v%1").arg(updateModal.currentVersion)
                        font.pixelSize: SentinelTheme.fontTiny
                        font.bold: true
                        color: updateModal.updateState === "completed" || updateModal.updateState === "upToDate" ? SentinelTheme.success
                             : updateModal.updateState === "error" ? SentinelTheme.warning
                             : updateModal.updateState === "downloading" || updateModal.updateState === "verifying" || updateModal.updateState === "checking" ? SentinelTheme.accent
                             : SentinelTheme.textPrimary
                    }
                }

                // Close Button
                Button {
                    id: closeBtn
                    implicitWidth: 28
                    implicitHeight: 28
                    flat: true
                    hoverEnabled: true
                    onClicked: updateModal.cancelUpdate()
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

            // Top Separator
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
            }

            // ── Main Body Content (State-dependent) ─────────────────────────
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                // ── STATE 1: IDLE / AVAILABLE PREVIEW ───────────────────────
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
                            Layout.preferredHeight: 74
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.lightTheme ? "#f8fafc" : SentinelTheme.withAlpha(SentinelTheme.backgroundBase, 0.60)
                            border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.25)
                            border.width: 1

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2

                                Label {
                                    text: updateModal.updateState === "available" ? qsTr("NEW VERSION") : qsTr("TARGET VERSION")
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
                                    elide: Text.ElideRight
                                    Layout.maximumWidth: 140
                                }
                            }
                        }

                        // Version details
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: qsTr("Current Installed Version: v%1").arg(updateModal.currentVersion)
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textMuted
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                            }

                            Label {
                                text: qsTr("Package: Sentinel-Desktop-v%1.dmg (42.0 MB)").arg(updateModal.availableVersion)
                                font.pixelSize: SentinelTheme.fontSmall
                                font.bold: true
                                color: SentinelTheme.textPrimary
                                wrapMode: Text.WordWrap
                                elide: Text.ElideMiddle
                                Layout.fillWidth: true
                            }

                            Label {
                                text: qsTr("Status: Ready to download and verify (SHA-256)")
                                font.pixelSize: SentinelTheme.fontTiny
                                color: SentinelTheme.success
                                elide: Text.ElideRight
                                Layout.fillWidth: true
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
                                Layout.fillWidth: true
                            }

                            ScrollView {
                                id: releaseNotesScroll
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                                ColumnLayout {
                                    width: releaseNotesScroll.width > 0 ? releaseNotesScroll.width : parent.width
                                    spacing: 8

                                    // Dynamic release notes if provided
                                    Text {
                                        visible: updateModal.releaseNotesText.length > 0
                                        text: updateModal.releaseNotesText
                                        font.pixelSize: SentinelTheme.fontSmall
                                        font.family: SentinelTheme.fontFamily
                                        color: SentinelTheme.textPrimary
                                        wrapMode: Text.WordWrap
                                        textFormat: Text.PlainText
                                        Layout.fillWidth: true
                                    }

                                    // Fallback highlights if releaseNotesText is empty
                                    ColumnLayout {
                                        visible: updateModal.releaseNotesText.length === 0
                                        Layout.fillWidth: true
                                        spacing: 6

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: itemCol1.implicitHeight + 12
                                            radius: SentinelTheme.radiusSm
                                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.03)

                                            ColumnLayout {
                                                id: itemCol1
                                                anchors.fill: parent
                                                anchors.margins: 6
                                                Label {
                                                    text: "• " + qsTr("Liquid Glass Light theme rendering & panel palette overhaul")
                                                    font.pixelSize: SentinelTheme.fontSmall
                                                    color: SentinelTheme.textPrimary
                                                    wrapMode: Text.WordWrap
                                                    Layout.fillWidth: true
                                                }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: itemCol2.implicitHeight + 12
                                            radius: SentinelTheme.radiusSm
                                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.03)

                                            ColumnLayout {
                                                id: itemCol2
                                                anchors.fill: parent
                                                anchors.margins: 6
                                                Label {
                                                    text: "• " + qsTr("Documents/Sentinel standard path provider database persistence")
                                                    font.pixelSize: SentinelTheme.fontSmall
                                                    color: SentinelTheme.textPrimary
                                                    wrapMode: Text.WordWrap
                                                    Layout.fillWidth: true
                                                }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: itemCol3.implicitHeight + 12
                                            radius: SentinelTheme.radiusSm
                                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.03)

                                            ColumnLayout {
                                                id: itemCol3
                                                anchors.fill: parent
                                                anchors.margins: 6
                                                Label {
                                                    text: "• " + qsTr("Ollama local runtime health checking & model catalog discovery")
                                                    font.pixelSize: SentinelTheme.fontSmall
                                                    color: SentinelTheme.textPrimary
                                                    wrapMode: Text.WordWrap
                                                    Layout.fillWidth: true
                                                }
                                            }
                                        }

                                        Rectangle {
                                            Layout.fillWidth: true
                                            implicitHeight: itemCol4.implicitHeight + 12
                                            radius: SentinelTheme.radiusSm
                                            color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.03)

                                            ColumnLayout {
                                                id: itemCol4
                                                anchors.fill: parent
                                                anchors.margins: 6
                                                Label {
                                                    text: "• " + qsTr("Interactive options menu & conversation navigation stability fixes")
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
                        }
                    }
                }

                // ── STATE 2: CHECKING / DOWNLOADING / VERIFYING ─────────────
                ColumnLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceLg
                    visible: updateModal.updateState === "checking" || updateModal.updateState === "downloading" || updateModal.updateState === "verifying"

                    Item { Layout.fillHeight: true }

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
                                running: updateModal.updateState === "checking" || updateModal.updateState === "downloading" || updateModal.updateState === "verifying"
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            visible: updateModal.updateState === "downloading"
                            text: Math.round(updateModal.downloadProgress * 100) + "%"
                            font.pixelSize: SentinelTheme.fontSmall
                            font.bold: true
                            color: SentinelTheme.accent
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
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Label {
                            text: updateModal.updateState === "downloading"
                                ? (updateModal.downloadedBytesText + " • " + updateModal.downloadSpeedText)
                                : qsTr("Local encrypted payload verification")
                            font.pixelSize: SentinelTheme.fontSmall
                            color: SentinelTheme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    // Animated Progress Bar Track
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: SentinelTheme.spaceLg
                        Layout.rightMargin: SentinelTheme.spaceLg
                        implicitHeight: 8
                        radius: 4
                        color: SentinelTheme.lightTheme ? "#e2e8f4" : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                        clip: true

                        Rectangle {
                            width: updateModal.updateState === "checking" ? parent.width * 0.25
                                 : updateModal.updateState === "verifying" ? parent.width * 0.95
                                 : Math.min(parent.width, Math.max(0, parent.width * updateModal.downloadProgress))
                            height: parent.height
                            radius: 4
                            color: SentinelTheme.accent

                            Behavior on width {
                                NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // ── STATE 3: UP TO DATE ──────────────────────────────────────
                ColumnLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceMd
                    visible: updateModal.updateState === "upToDate"

                    Item { Layout.fillHeight: true }

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
                        text: qsTr("Sentinel is Up to Date")
                        font.pixelSize: SentinelTheme.fontCard
                        font.bold: true
                        color: SentinelTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("You are currently running version %1. No new updates are available at this time.").arg(updateModal.currentVersion)
                        font.pixelSize: SentinelTheme.fontBody
                        color: SentinelTheme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.maximumWidth: 420
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Item { Layout.fillHeight: true }
                }

                // ── STATE 4: COMPLETED ───────────────────────────────────────
                ColumnLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceMd
                    visible: updateModal.updateState === "completed"

                    Item { Layout.fillHeight: true }

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
                        Layout.fillWidth: true
                        Layout.maximumWidth: 420
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Item { Layout.fillHeight: true }
                }

                // ── STATE 5: ERROR ───────────────────────────────────────────
                ColumnLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceMd
                    visible: updateModal.updateState === "error"

                    Item { Layout.fillHeight: true }

                    Rectangle {
                        Layout.preferredWidth: 64
                        Layout.preferredHeight: 64
                        Layout.alignment: Qt.AlignHCenter
                        radius: 32
                        color: SentinelTheme.withAlpha(SentinelTheme.warning, 0.14)
                        border.color: SentinelTheme.withAlpha(SentinelTheme.warning, 0.40)
                        border.width: 2

                        Label {
                            anchors.centerIn: parent
                            text: "!"
                            font.pixelSize: 32
                            font.bold: true
                            color: SentinelTheme.warning
                        }
                    }

                    Label {
                        text: qsTr("Update Check Failed")
                        font.pixelSize: SentinelTheme.fontCard
                        font.bold: true
                        color: SentinelTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Label {
                        text: updateModal.errorMessage.length > 0
                            ? updateModal.errorMessage
                            : qsTr("Unable to connect to the update service. Please check your network connection and try again.")
                        font.pixelSize: SentinelTheme.fontBody
                        color: SentinelTheme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.maximumWidth: 420
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            // Bottom Separator
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
            }

            // ── Footer Actions ───────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                Label {
                    text: qsTr("Manual update boundary • Local execution")
                    font.pixelSize: SentinelTheme.fontTiny
                    color: SentinelTheme.textMuted
                }

                Item { Layout.fillWidth: true }

                // Cancel / Secondary Button
                Button {
                    id: cancelBtn
                    implicitHeight: 36
                    implicitWidth: 100
                    text: updateModal.updateState === "downloading" ? qsTr("Cancel") : qsTr("Close")
                    onClicked: updateModal.cancelUpdate()

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
                        : updateModal.updateState === "upToDate" ? qsTr("Check Again")
                        : updateModal.updateState === "error" ? qsTr("Try Again")
                        : (updateModal.updateState === "idle" || updateModal.updateState === "available") ? qsTr("Update Now")
                        : qsTr("Updating…")

                    enabled: updateModal.updateState !== "checking" && updateModal.updateState !== "downloading" && updateModal.updateState !== "verifying"

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
