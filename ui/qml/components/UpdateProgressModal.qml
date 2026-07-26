pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Shapes
import Sentinel.Desktop

SentinelOverlayModal {
    id: updateModal

    // ── Public API ────────────────────────────────────────────────────────────
    property string currentVersion: ""
    property string availableVersion: ""
    property string updateState: "idle" // "idle" | "checking" | "available" | "downloading" | "verifying" | "completed" | "upToDate" | "error"
    property real downloadProgress: 0.0 // 0.0 .. 1.0
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

    // ── Geometry & Layout ─────────────────────────────────────────────────────
    preferredWidth: 580
    preferredHeight: 520
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
        id: contentRoot
        anchors.fill: parent
        clip: true

        layer.enabled: true
        layer.effect: MultiEffect {
            shadowEnabled: true
            shadowColor: "#000000"
            shadowOpacity: 0.08
            shadowBlur: 0.08
            shadowHorizontalOffset: 1
            shadowVerticalOffset: 1
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: SentinelTheme.spaceLg
            spacing: SentinelTheme.spaceMd

            // ── Header Bar ───────────────────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                spacing: SentinelTheme.spaceMd

                Rectangle {
                    implicitWidth: 36
                    implicitHeight: 36
                    radius: SentinelTheme.radiusMd
                    color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.12)
                    border.color: SentinelTheme.withAlpha(SentinelTheme.accent, 0.30)
                    border.width: 1

                    Label {
                        anchors.centerIn: parent
                        text: updateModal.updateState === "completed" || updateModal.updateState === "upToDate" ? "\u2713"
                            : updateModal.updateState === "error" ? "!"
                            : "\u27F3"
                        font.pixelSize: 18
                        font.bold: true
                        color: updateModal.updateState === "completed" || updateModal.updateState === "upToDate" ? SentinelTheme.success
                             : updateModal.updateState === "error" ? SentinelTheme.warning
                             : SentinelTheme.accent
                    }
                }

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
                        text: updateModal.updateState === "checking" ? qsTr("Checking\u2026")
                            : updateModal.updateState === "downloading" ? qsTr("Downloading")
                            : updateModal.updateState === "verifying" ? qsTr("Verifying")
                            : updateModal.updateState === "completed" ? qsTr("Ready to Install")
                            : updateModal.updateState === "upToDate" ? qsTr("Up to Date")
                            : updateModal.updateState === "error" ? qsTr("Check Failed")
                            : updateModal.updateState === "available" ? qsTr("v%1 Available").arg(updateModal.availableVersion)
                            : updateModal.currentVersion.length > 0 ? qsTr("v%1").arg(updateModal.currentVersion)
                            : qsTr("Update")
                        font.pixelSize: SentinelTheme.fontTiny
                        font.bold: true
                        color: updateModal.updateState === "completed" || updateModal.updateState === "upToDate" ? SentinelTheme.success
                             : updateModal.updateState === "error" ? SentinelTheme.warning
                             : updateModal.updateState === "downloading" || updateModal.updateState === "verifying" || updateModal.updateState === "checking" ? SentinelTheme.accent
                             : SentinelTheme.textPrimary
                    }
                }

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

            // ── Main Body Content ───────────────────────────────────────────
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true

                // ── STATE: IDLE / AVAILABLE ──────────────────────────────────
                ColumnLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceMd
                    visible: updateModal.updateState === "idle" || updateModal.updateState === "available"

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: SentinelTheme.spaceMd

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

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: updateModal.currentVersion.length > 0
                                    ? qsTr("Current Installed Version: v%1").arg(updateModal.currentVersion)
                                    : ""
                                font.pixelSize: SentinelTheme.fontSmall
                                color: SentinelTheme.textMuted
                                elide: Text.ElideRight
                                Layout.fillWidth: true
                                visible: text.length > 0
                            }

                            Label {
                                text: updateModal.packageName.length > 0
                                    ? qsTr("Package: %1 (%2)").arg(updateModal.packageName).arg(updateModal.packageSizeText)
                                    : qsTr("Package: GitHub Release")
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
                                text: updateModal.availableVersion.length > 0
                                    ? qsTr("Release Highlights (v%1):").arg(updateModal.availableVersion)
                                    : qsTr("Release Highlights:")
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

                                    Text {
                                        visible: updateModal.releaseNotesText.length > 0
                                        text: updateModal.releaseNotesText
                                        font.pixelSize: SentinelTheme.fontSmall
                                        font.family: SentinelTheme.fontFamily
                                        color: SentinelTheme.textPrimary
                                        wrapMode: Text.WordWrap
                                        textFormat: TextEdit.Markdown
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        visible: updateModal.releaseNotesText.length === 0
                                        text: qsTr("No release notes available.")
                                        font.pixelSize: SentinelTheme.fontSmall
                                        color: SentinelTheme.textMuted
                                        Layout.fillWidth: true
                                    }
                                }
                            }
                        }
                    }
                }

                // ── STATE: CHECKING / DOWNLOADING / VERIFYING ───────────────
                ColumnLayout {
                    anchors.fill: parent
                    spacing: SentinelTheme.spaceLg
                    visible: updateModal.updateState === "checking" || updateModal.updateState === "downloading" || updateModal.updateState === "verifying"

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

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: updateModal.updateState === "checking" ? qsTr("Checking GitHub release boundary\u2026")
                                : updateModal.updateState === "verifying" ? qsTr("Verifying SHA-256 checksum & payload signature\u2026")
                                : qsTr("Downloading Sentinel v%1 update\u2026").arg(updateModal.availableVersion)
                            font.pixelSize: SentinelTheme.fontBody
                            font.bold: true
                            color: SentinelTheme.textPrimary
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }

                        Label {
                            text: updateModal.updateState === "downloading"
                                ? (updateModal.downloadedBytesText.length > 0
                                    ? updateModal.downloadedBytesText
                                    : "")
                                  + (updateModal.downloadSpeedText.length > 0 && updateModal.downloadedBytesText.length > 0
                                    ? " \u2022 " + updateModal.downloadSpeedText
                                    : updateModal.downloadSpeedText.length > 0 ? updateModal.downloadSpeedText : "")
                                : qsTr("Local encrypted payload verification")
                            font.pixelSize: SentinelTheme.fontSmall
                            color: SentinelTheme.textMuted
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.leftMargin: SentinelTheme.spaceLg
                        Layout.rightMargin: SentinelTheme.spaceLg
                        implicitHeight: 8
                        radius: 4
                        color: SentinelTheme.lightTheme ? "#e2e8f4" : SentinelTheme.withAlpha(SentinelTheme.textPrimary, 0.10)
                        clip: true

                        layer.enabled: true
                        layer.effect: MultiEffect {
                            shadowEnabled: true
                            shadowColor: "#000000"
                            shadowOpacity: 0.08
                            shadowBlur: 0.06
                            shadowHorizontalOffset: 1
                            shadowVerticalOffset: 1
                        }

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
                        color: SentinelTheme.withAlpha(SentinelTheme.success, 0.14)
                        border.color: SentinelTheme.withAlpha(SentinelTheme.success, 0.40)
                        border.width: 2

                        Label {
                            anchors.centerIn: parent
                            text: "\u2713"
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
                        text: updateModal.currentVersion.length > 0
                            ? qsTr("You are currently running version %1. No new updates are available at this time.").arg(updateModal.currentVersion)
                            : qsTr("No new updates are available at this time.")
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
                        color: SentinelTheme.withAlpha(SentinelTheme.success, 0.14)
                        border.color: SentinelTheme.withAlpha(SentinelTheme.success, 0.40)
                        border.width: 2

                        Label {
                            anchors.centerIn: parent
                            text: "\u2713"
                            font.pixelSize: 32
                            font.bold: true
                            color: SentinelTheme.success
                        }
                    }

                    Label {
                        text: qsTr("Sentinel v%1 Update Downloaded!").arg(updateModal.availableVersion)
                        font.pixelSize: SentinelTheme.fontCard
                        font.bold: true
                        color: SentinelTheme.textPrimary
                        horizontalAlignment: Text.AlignHCenter
                        Layout.fillWidth: true
                    }

                    Label {
                        text: qsTr("The update package has been downloaded to your Downloads folder. Open the file to install the update.")
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
                    text: qsTr("Manual update boundary \u2022 Local execution")
                    font.pixelSize: SentinelTheme.fontTiny
                    color: SentinelTheme.textMuted
                }

                Item { Layout.fillWidth: true }

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

                        layer.enabled: cancelBtn.hovered
                        layer.effect: MultiEffect {
                            shadowEnabled: true
                            shadowColor: "#000000"
                            shadowOpacity: 0.10
                            shadowBlur: 0.08
                            shadowHorizontalOffset: 1
                            shadowVerticalOffset: 1
                        }
                    }
                }

                Button {
                    id: actionBtn
                    implicitHeight: 36
                    implicitWidth: 160
                    text: updateModal.updateState === "completed" ? qsTr("Open Downloads Folder")
                        : updateModal.updateState === "upToDate" ? qsTr("Check Again")
                        : updateModal.updateState === "error" ? qsTr("Try Again")
                        : (updateModal.updateState === "idle" || updateModal.updateState === "available") ? qsTr("Update Now")
                        : qsTr("Updating\u2026")

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

                        layer.enabled: actionBtn.hovered && actionBtn.enabled
                        layer.effect: MultiEffect {
                            shadowEnabled: true
                            shadowColor: "#000000"
                            shadowOpacity: 0.12
                            shadowBlur: 0.08
                            shadowHorizontalOffset: 1
                            shadowVerticalOffset: 1
                        }
                    }
                }
            }
        }
    }
}
