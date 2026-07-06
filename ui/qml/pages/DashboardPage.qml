import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop

ScrollView {
    id: dashboardPage
    required property var viewModel
    readonly property bool wideLayout: false
    readonly property bool compact: width < 860
    readonly property int panelPadding: SentinelTheme.spaceLg
    readonly property string runtimeStatusText: dashboardPage.viewModel.ollamaHealthStatus
                                                + " / "
                                                + dashboardPage.viewModel.localInferenceRuntimeState
    readonly property bool runtimeReady: dashboardPage.viewModel.ollamaHealthStatus === "Available"
                                  || dashboardPage.viewModel.ollamaHealthStatus === "Ready"
                                  || dashboardPage.viewModel.selectedLocalModelStatus === "Available"
                                  || dashboardPage.viewModel.selectedLocalModelStatus === "Fallback"
    readonly property bool streamingActive: dashboardPage.viewModel.localInferenceStreamingText.length > 0
                                            || dashboardPage.viewModel.localInferenceRuntimeState === "Streaming"
    readonly property string uiSelfCheck: "home-uses-available-height scroll-bottom-padding"
    clip: true
    ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

    function focusComposer() {
        homeChatSurface.focusComposer()
    }

    function restoreDraft(text) {
        homeChatSurface.restoreDraft(text)
    }

    contentWidth: availableWidth

    Item {
        width: dashboardPage.availableWidth
        height: dashboardPage.availableHeight
        implicitHeight: height

        GridLayout {
            id: dashboardGrid
            width: parent.width
            height: parent.height
            columns: dashboardPage.wideLayout ? 12 : 1
            columnSpacing: SentinelTheme.spaceLg
            rowSpacing: SentinelTheme.spaceLg

            ColumnLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.columnSpan: 1
                spacing: SentinelTheme.spaceLg

                HomeChatSurface {
                    id: homeChatSurface
                    viewModel: dashboardPage.viewModel
                    compact: dashboardPage.compact
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    Layout.minimumHeight: 0
                }
            }
        }
    }
}
