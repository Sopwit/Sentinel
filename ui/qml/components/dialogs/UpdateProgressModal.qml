// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

SentinelOverlayModal {
    id: updateModal

    property string currentVersion: ""
    property string availableVersion: ""
    property string updateState: "idle"
    property real downloadProgress: 0.0
    property string downloadSpeedText: ""
    property string downloadedBytesText: ""
    property string releaseNotesText: ""
    property string errorMessage: ""
    property string packageName: ""
    property string packageSizeText: ""
    property var viewModel: null

    signal updateCompleted()
    signal checkRequested()
    signal downloadRequested()

    preferredWidth: 540
    preferredHeight: 480
    accent: SentinelTheme.accent
    modeName: "Sentinel"

    function startCheckAndDownload() {
        errorMessage = ""
        updateState = "checking"
        checkRequested()
    }

    function startDownload() {
        if (updateModal.updateState !== "downloading") {
            downloadProgress = 0.0
            downloadSpeedText = ""
            downloadedBytesText = ""
        }
    }

    onVisibleChanged: {
        if (visible && viewModel) {
            var wf = viewModel.updateWorkflowState
            if (wf && wf.startsWith("Available:")) {
                updateState = "available"
            } else if (wf && wf.startsWith("Up to date:")) {
                updateState = "upToDate"
            } else if (wf && wf.startsWith("Update check failed:")) {
                updateState = "error"
                errorMessage = wf
            }
        }
    }

    Connections {
        target: updateModal.viewModel
        ignoreUnknownSignals: true
        function onUpdateCheckCompleted(available, version, releaseNotes, downloadUrl, assetSize) {
            if (available) {
                updateModal.availableVersion = version
                if (releaseNotes && releaseNotes.length > 0) {
                    updateModal.releaseNotesText = releaseNotes
                }
                if (assetSize > 0) {
                    var sizeMB = (assetSize / (1024 * 1024)).toFixed(1)
                    updateModal.packageSizeText = sizeMB + " MB"
                }
                updateModal.updateState = "available"
            } else {
                updateModal.availableVersion = version || Qt.application.version
                var wf = updateModal.viewModel ? updateModal.viewModel.updateWorkflowState : ""
                if (wf && wf.startsWith("Update check failed")) {
                    updateModal.updateState = "error"
                    updateModal.errorMessage = wf
                } else {
                    updateModal.updateState = "upToDate"
                }
            }
        }
        function onUpdateDownloadProgressChanged(bytesReceived, bytesTotal, speedBytesPerSec) {
            updateModal.updateState = "downloading"
            updateModal.downloadProgress = bytesTotal > 0 ? bytesReceived / bytesTotal : 0.0
            var receivedMB = (bytesReceived / (1024 * 1024)).toFixed(1)
            var totalMB = (bytesTotal / (1024 * 1024)).toFixed(1)
            updateModal.downloadedBytesText = receivedMB + " MB / " + totalMB + " MB"
            if (speedBytesPerSec > 0) {
                var speedText = (speedBytesPerSec / (1024 * 1024)).toFixed(1) + " MB/s"
                updateModal.downloadSpeedText = speedText
            }
        }
        function onUpdateDownloadFinished(success, filePath) {
            if (success) {
                updateModal.updateState = "completed"
                updateModal.downloadProgress = 1.0
            } else {
                updateModal.updateState = "error"
                updateModal.errorMessage = qsTr("Download failed. Please check your network connection and try again.")
            }
        }
    }

    function cancelUpdate() {
        if (updateState === "downloading") {
            if (typeof viewModel !== "undefined" && viewModel)
                viewModel.cancelDownload()
        }
        if (updateState === "downloading" || updateState === "checking" || updateState === "verifying") {
            updateState = "idle"
            downloadProgress = 0.0
        }
        updateModal.close()
    }

    contentItem: Item {
        anchors.fill: parent
        clip: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: SentinelTheme.spaceLg
            spacing: SentinelTheme.spaceMd

            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceMd

                Label {
                    text: qsTr("Software Update")
                    font.pixelSize: SentinelTheme.fontCard
                    font.bold: true
                    color: SentinelTheme.textPrimary
                }

                Item { Layout.fillWidth: true }

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
                        text: "\u00D7"
                        font.pixelSize: 20
                        font.bold: true
                        color: SentinelTheme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                // ── STATE: IDLE / AVAILABLE ──────────────────────────────────
                ColumnLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceMd
                    visible: updateModal.updateState === "idle" || updateModal.updateState === "available"

                    Label {
                        text: updateModal.updateState === "available"
                            ? qsTr("A new version is available for download.")
                            : qsTr("Check for updates to see if a newer version is available.")
                        font.pixelSize: SentinelTheme.fontBody
                        color: SentinelTheme.textMuted
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.maximumWidth: 460
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceMd

                        Rectangle {
                            Layout.preferredWidth: 140
                            Layout.preferredHeight: 68
                            radius: SentinelTheme.radiusMd
                            color: SentinelTheme.backgroundBase
                            border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.25)
                            border.width: 1

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2

                                Label {
                                    text: qsTr("VERSION")
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
                                    Layout.maximumWidth: 120
                                    elide: Text.ElideRight
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: updateModal.currentVersion.length > 0
                                    ? qsTr("Current: v%1").arg(updateModal.currentVersion)
                                    : ""
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textMuted
                                visible: text.length > 0
                            }

                            Label {
                                text: updateModal.packageSizeText.length > 0
                                    ? qsTr("Download size: %1").arg(updateModal.packageSizeText)
                                    : qsTr("Download size unknown")
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textPrimary
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        radius: SentinelTheme.radiusMd
                        color: SentinelTheme.backgroundBase
                        border.color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: SentinelTheme.spaceMd
                            spacing: SentinelTheme.spaceSm

                            Label {
                                text: qsTr("Release Notes")
                                font.pixelSize: SentinelTheme.fontBody
                                font.bold: true
                                color: SentinelTheme.textPrimary
                            }

                            ScrollView {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                                ColumnLayout {
                                    width: parent.width
                                    spacing: 8

                                    Text {
                                        visible: updateModal.releaseNotesText.length > 0
                                        text: updateModal.releaseNotesText
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: SentinelTheme.textPrimary
                                        wrapMode: Text.WordWrap
                                        textFormat: Text.MarkdownText
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        visible: updateModal.releaseNotesText.length === 0
                                        text: qsTr("No release notes.")
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: SentinelTheme.textMuted
                                    }
                                }
                            }
                        }
                    }
                }

                // ── STATE: CHECKING ───────────────────────────────────────────
                ColumnLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceLg
                    visible: updateModal.updateState === "checking"

                    Item { Layout.fillHeight: true }

                    Item {
                        Layout.preferredWidth: 48
                        Layout.preferredHeight: 48
                        Layout.alignment: Qt.AlignHCenter

                        Rectangle {
                            anchors.fill: parent
                            radius: 24
                            color: "transparent"
                            border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.20)
                            border.width: 3
                        }

                        Rectangle {
                            anchors.fill: parent
                            radius: 24
                            color: "transparent"
                            border.color: SentinelTheme.accent
                            border.width: 3

                            RotationAnimation on rotation {
                                loops: Animation.Infinite
                                from: 0
                                to: 360
                                duration: 1200
                                running: visible
                            }
                        }
                    }

                    Label {
                        text: qsTr("Checking for updates\u2026")
                        font.pixelSize: SentinelTheme.fontBody
                        font.bold: true
                        color: SentinelTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Item { Layout.fillHeight: true }
                }

                // ── STATE: DOWNLOADING / VERIFYING ────────────────────────────
                ColumnLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceLg
                    visible: updateModal.updateState === "downloading" || updateModal.updateState === "verifying"

                    Item { Layout.fillHeight: true }

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
                                running: visible
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            text: Math.round(updateModal.downloadProgress * 100) + "%"
                            font.pixelSize: SentinelTheme.fontSmall
                            font.bold: true
                            color: SentinelTheme.accent
                            visible: updateModal.updateState === "downloading"
                        }
                    }

                    Label {
                        text: qsTr("Downloading update\u2026")
                        font.pixelSize: SentinelTheme.fontBody
                        font.bold: true
                        color: SentinelTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Label {
                        text: updateModal.downloadedBytesText
                        font.pixelSize: SentinelTheme.fontSmall
                        color: SentinelTheme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: SentinelTheme.spaceLg
                        Layout.rightMargin: SentinelTheme.spaceLg
                        implicitHeight: 6
                        radius: 3
                        color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)

                        Rectangle {
                            width: Math.min(parent.width, Math.max(0, parent.width * updateModal.downloadProgress))
                            height: parent.height
                            radius: 3
                            color: SentinelTheme.accent

                            Behavior on width {
                                NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }

                // ── STATE: UP TO DATE ────────────────────────────────────────
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
                        color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.12)

                        Label {
                            anchors.centerIn: parent
                            text: "✓"
                            font.pixelSize: 28
                            font.bold: true
                            color: SentinelTheme.accent
                        }
                    }

                    Label {
                        text: qsTr("You're up to date.")
                        font.pixelSize: SentinelTheme.fontCard
                        font.bold: true
                        color: SentinelTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Label {
                        text: updateModal.currentVersion.length > 0
                            ? qsTr("Sentinel v%1 is currently the latest version available.").arg(updateModal.currentVersion)
                            : qsTr("No new updates available.")
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

                // ── STATE: COMPLETED ─────────────────────────────────────────
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
                        color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.12)

                        Label {
                            anchors.centerIn: parent
                            text: "✓"
                            font.pixelSize: 28
                            font.bold: true
                            color: SentinelTheme.accent
                        }
                    }

                    Label {
                        text: qsTr("Download complete!")
                        font.pixelSize: SentinelTheme.fontCard
                        font.bold: true
                        color: SentinelTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("The update package has been downloaded successfully.")
                        font.pixelSize: SentinelTheme.fontBody
                        color: SentinelTheme.textMuted
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode: Text.WordWrap
                        Layout.fillWidth: true
                        Layout.maximumWidth: 400
                        Layout.alignment: Qt.AlignHCenter
                    }

                    Item { Layout.fillHeight: true }
                }

                // ── STATE: ERROR ─────────────────────────────────────────────
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
                        color: SentinelTheme.withAlpha(SentinelTheme.warning, 0.15)

                        Label {
                            anchors.centerIn: parent
                            text: "!"
                            font.pixelSize: 28
                            font.bold: true
                            color: SentinelTheme.warning
                        }
                    }

                    Label {
                        text: qsTr("Update check failed.")
                        font.pixelSize: SentinelTheme.fontCard
                        font.bold: true
                        color: SentinelTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Label {
                        text: updateModal.errorMessage.length > 0
                            ? updateModal.errorMessage
                            : qsTr("Could not connect. Check your internet connection and try again.")
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

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 1
                color: SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.08)
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceSm

                Item { Layout.fillWidth: true }

                Button {
                    id: cancelBtn
                    implicitHeight: 36
                    implicitWidth: 96
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

                Button {
                    id: actionBtn
                    implicitHeight: 36
                    implicitWidth: 150
                    text: updateModal.updateState === "completed" ? qsTr("Open Downloads Folder")
                        : updateModal.updateState === "upToDate" ? qsTr("Check Again")
                        : updateModal.updateState === "error" ? qsTr("Try Again")
                        : (updateModal.updateState === "idle" || updateModal.updateState === "available") ? qsTr("Update")
                        : qsTr("Please wait\u2026")

                    enabled: updateModal.updateState !== "checking" && updateModal.updateState !== "downloading" && updateModal.updateState !== "verifying"

                    onClicked: {
                        if (updateModal.updateState === "completed") {
                            updateModal.updateCompleted()
                            updateModal.close()
                        } else if (updateModal.updateState === "available") {
                            updateModal.downloadRequested()
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
