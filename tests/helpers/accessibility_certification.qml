// SPDX-License-Identifier: GPL-3.0-or-later
//
// Disposable accessibility fixture for P6-03. It mounts the production base
// controls and the production SentinelOverlayModal in an offscreen engine and
// records activation counts so the harness can prove that keyboard activation,
// dialog default focus and the no-accidental-approval contract hold. No daemon,
// provider or user data is involved.
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ApplicationWindow {
    id: win
    objectName: "accessibilityRoot"
    width: 900
    height: 700
    visible: true
    color: SentinelTheme.backgroundBase

    property int buttonClicks: 0
    property int disabledClicks: 0
    property int denials: 0
    property int approvals: 0
    property int dismissals: 0

    Column {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 10

        SentinelButton {
            objectName: "a11yButton"
            text: "Save changes"
            tooltipText: "Persist the current settings"
            onClicked: win.buttonClicks++
        }

        SentinelButton {
            objectName: "a11yIconButton"
            iconName: "check"
            Accessible.name: "Confirm action"
            onClicked: win.buttonClicks++
        }

        SentinelButton {
            objectName: "a11yDisabledButton"
            text: "Unavailable"
            enabled: false
            onClicked: win.disabledClicks++
        }

        SentinelTextField {
            objectName: "a11yTextField"
            placeholderText: "Search models"
            accessibleDescription: "Filters the locally known model catalog"
        }

        SentinelComboBox {
            objectName: "a11yComboBox"
            Accessible.name: "Provider"
            model: ["Alpha", "Beta", "Gamma"]
        }

        SentinelSwitch {
            objectName: "a11ySwitch"
            Accessible.name: "Enable notifications"
            Accessible.description: "Delivers desktop notifications for agent events"
        }

        SentinelSlider {
            objectName: "a11ySlider"
            Accessible.name: "Volume"
            from: 0
            to: 100
            stepSize: 10
            value: 40
        }

        SentinelSpinBox {
            objectName: "a11ySpinBox"
            Accessible.name: "Retries"
            from: 0
            to: 10
            value: 2
        }
    }

    // Approval-style overlay: mirrors the daemon approval contract in Main.qml so the
    // dialog role, default focus and no-accidental-approval behaviour are testable.
    SentinelOverlayModal {
        id: approvalModal
        objectName: "a11yApprovalModal"
        preferredWidth: 480
        preferredHeight: 260
        closePolicy: Popup.NoAutoClose
        initialFocusItem: approvalDeny

        contentItem: ColumnLayout {
            objectName: "a11yApprovalContent"
            Accessible.role: Accessible.Dialog
            Accessible.name: "Tool approval required"
            Accessible.description: "Review the requested tool before deciding."
            spacing: SentinelTheme.spaceSm

            Label {
                objectName: "a11yApprovalTitle"
                text: "Tool approval required"
                color: SentinelTheme.textPrimary
                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }

            Label {
                objectName: "a11yApprovalDetail"
                text: "Risk: 2"
                color: SentinelTheme.textMuted
                Accessible.role: Accessible.StaticText
                Accessible.name: text
            }

            Item { Layout.fillHeight: true }

            RowLayout {
                SentinelButton {
                    objectName: "a11yApprovalCancel"
                    text: "Cancel run"
                    onClicked: win.dismissals++
                }
                Item { Layout.fillWidth: true }
                SentinelButton {
                    id: approvalDeny
                    objectName: "a11yApprovalDeny"
                    text: "Deny"
                    onClicked: win.denials++
                }
                SentinelButton {
                    objectName: "a11yApprovalAllow"
                    text: "Allow once"
                    onClicked: win.approvals++
                }
            }
        }
    }

    // Recovery-style overlay: mirrors Main.qml's draft recovery dialog.
    SentinelOverlayModal {
        id: recoveryModal
        objectName: "a11yRecoveryModal"
        preferredWidth: 460
        preferredHeight: 220
        initialFocusItem: recoveryRestore

        contentItem: ColumnLayout {
            objectName: "a11yRecoveryContent"
            Accessible.role: Accessible.Dialog
            Accessible.name: "Restore previous session"
            Accessible.description: "Restore or discard the locally saved draft."
            spacing: SentinelTheme.spaceSm

            Item { Layout.fillHeight: true }

            RowLayout {
                SentinelButton {
                    objectName: "a11yRecoveryDiscard"
                    text: "Discard"
                }
                Item { Layout.fillWidth: true }
                SentinelButton {
                    id: recoveryRestore
                    objectName: "a11yRecoveryRestore"
                    text: "Restore"
                }
            }
        }
    }
}
