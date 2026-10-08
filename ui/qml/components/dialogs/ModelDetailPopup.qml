// SPDX-License-Identifier: GPL-3.0-or-later
pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Sentinel.Desktop
SentinelOverlayModal {
    id: root
    required property var modelInfo
    property bool installed: false
    property bool activePull: false
    property real pullProgress: 0
    property string pullStatus: ""
    property string pullError: ""
    signal downloadRequested(string modelId)
    signal cancelRequested()
    signal ggufSearchRequested(string query)
    preferredWidth: 760
    preferredHeight: 680
    // The download destination is independent of the active chat provider.
    property int downloadTarget: 0
    readonly property bool chatModel: !!(modelInfo && !modelInfo.cloudOnly && ["LLM", "Think", "Vision", "Embedding"].indexOf(modelInfo.category) >= 0)
    readonly property bool targetAvailable: downloadTarget === 0 ? canPull
        : downloadTarget === 1 ? (gguf ? modelInfo.category === "LLM" && (modelInfo.installed || modelInfo.downloadable) : chatModel)
        : chatModel
    readonly property string targetExplanation: modelInfo && modelInfo.cloudOnly
        ? qsTr("This catalog entry is cloud-only. No local download is available; use the model source for cloud availability.")
        : downloadTarget === 0
        ? (canPull ? qsTr("Download the selected variant into Ollama, regardless of your chat provider.") : qsTr("No verified Ollama variant is available for this model."))
        : downloadTarget === 1
          ? (gguf ? qsTr("Download or select this GGUF model for llama.cpp.") : chatModel ? qsTr("Find compatible GGUF files, then choose a quantization to download for llama.cpp.") : qsTr("This model requires a different runtime; no compatible llama.cpp download is available."))
          : (chatModel ? (managerRepository.length > 0 ? qsTr("Open this model in LM Studio's download screen. LM Studio manages the download.") : qsTr("Open the model catalog page to review available LM Studio downloads.")) : qsTr("This model requires a different runtime; it is not supported by LM Studio chat inference."))
    readonly property string targetError: downloadTarget === 0
        ? (pullError || ollamaModelDetailFetcher.errorText || ollamaPuller.errorText)
        : downloadTarget === 1 ? ggufLibraryFetcher.errorText : ""
    readonly property string targetAction: downloadTarget === 0 ? qsTr("Download with Ollama")
        : downloadTarget === 1 ? (gguf ? (modelInfo.installed ? qsTr("Use with llama.cpp") : qsTr("Download for llama.cpp")) : qsTr("Choose GGUF for llama.cpp"))
        : (managerRepository.length > 0 ? qsTr("Open LM Studio downloads ↗") : qsTr("Open LM Studio catalog ↗"))
    property var selectedTagObj: null
    readonly property string effectiveOllamaId: selectedTagObj ? selectedTagObj.fullTag : (modelInfo && modelInfo.ollamaId ? modelInfo.ollamaId : "")
    readonly property bool gguf: !!(modelInfo && modelInfo.gguf)
    readonly property bool canPull: !!(modelInfo && modelInfo.ollamaId && modelInfo.provider !== "LM Studio")
    readonly property string managerRepository: modelInfo && modelInfo.repositoryId ? modelInfo.repositoryId
        : modelInfo && modelInfo.externalUrl && modelInfo.externalUrl.indexOf("https://huggingface.co/") === 0
          ? modelInfo.externalUrl.substring(23).split('/').slice(0, 2).join('/') : ""
    readonly property string managerUrl: managerRepository.length > 0
        ? "lmstudio://open_from_hf?model=" + encodeURIComponent(managerRepository) : modelInfo && modelInfo.externalUrl && modelInfo.externalUrl.indexOf("https://lmstudio.ai/models/") === 0 ? modelInfo.externalUrl : "lmstudio://"
    function info(key) { return modelInfo && modelInfo[key] ? String(modelInfo[key]) : qsTr("Not reported") }
    onModelInfoChanged: {
        selectedTagObj = null
        if (canPull) ollamaModelDetailFetcher.fetchDetails(modelInfo.ollamaId.split(':')[0])
        else ollamaModelDetailFetcher.cancel()
    }
    onOpened: {
        downloadTarget = gguf ? 1 : canPull ? 0 : 2
        if (gguf && modelInfo.repositoryId) ggufLibraryFetcher.fetchDetails(modelInfo.repositoryId)
    }
    Connections {
        target: ggufLibraryFetcher
        function onChanged() {
            if (!root.opened || !root.gguf) return
            var entries = ggufLibraryFetcher.models
            for (var i = 0; i < entries.length; i++) {
                if (entries[i].id === root.modelInfo.id && JSON.stringify(entries[i]) !== JSON.stringify(root.modelInfo)) {
                    root.modelInfo = entries[i]
                    break
                }
            }
        }
    }
    onClosed: ollamaModelDetailFetcher.cancel()
    contentItem: ColumnLayout {
        anchors.fill: parent
        anchors.margins: SentinelTheme.spaceLg
        spacing: SentinelTheme.spaceMd
        RowLayout {
            Layout.fillWidth: true
            Label { Layout.fillWidth: true; text: root.info("name"); font.pixelSize: 22; font.bold: true; color: SentinelTheme.textPrimary; wrapMode: Text.WordWrap }
            SentinelButton { text: "×"; Accessible.name: qsTr("Close model details"); onClicked: root.close() }
        }
        Label { Layout.fillWidth: true; text: qsTr("Publisher: %1 • Type: %2").arg(root.info("provider")).arg(root.info("category")); color: SentinelTheme.textMuted; wrapMode: Text.WordWrap }
        ScrollView {
            id: detailsScroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth
            ColumnLayout {
                width: detailsScroll.availableWidth
                spacing: SentinelTheme.spaceMd
                Label { Layout.fillWidth: true; text: root.info("description"); color: SentinelTheme.textPrimary; wrapMode: Text.WordWrap; textFormat: Text.PlainText }
                Label { Layout.fillWidth: true; text: qsTr("Best for: %1").arg(root.info("bestFor")); color: SentinelTheme.textMuted; wrapMode: Text.WordWrap }
                GridLayout {
                    Layout.fillWidth: true
                    columns: root.width < 520 ? 1 : 2
                    columnSpacing: SentinelTheme.spaceLg
                    rowSpacing: SentinelTheme.spaceSm
                    Repeater {
                        model: [
                            {label: qsTr("Context window"), value: root.info("context")},
                            {label: qsTr("Input"), value: root.info("input")},
                            {label: qsTr("Format"), value: root.info("format")},
                            {label: qsTr("Quantization"), value: root.selectedTagObj ? root.selectedTagObj.tag : root.info("quantization")},
                            {label: qsTr("Download size"), value: root.selectedTagObj ? root.selectedTagObj.size : root.info("size")},
                            {label: qsTr("License"), value: root.info("license")},
                            {label: qsTr("Architecture"), value: root.info("architecture")},
                            {label: qsTr("Repository"), value: root.info("repositoryId")},
                            {label: qsTr("Artifact"), value: root.info("filename")},
                            {label: qsTr("Local file"), value: root.info("localFile")},
                            {label: qsTr("Tags / capabilities"), value: root.modelInfo && root.modelInfo.tags && root.modelInfo.tags.length > 0 ? root.modelInfo.tags.join(", ") : qsTr("Not reported")}
                        ]
                        delegate: ColumnLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            Label { text: parent.modelData.label; color: SentinelTheme.textMuted; font.pixelSize: SentinelTheme.fontTiny }
                            Label { Layout.fillWidth: true; text: parent.modelData.value; color: SentinelTheme.textPrimary; wrapMode: Text.WrapAnywhere; textFormat: Text.PlainText }
                        }
                    }
                }
                Label { Layout.fillWidth: true; visible: !!(root.modelInfo && root.modelInfo.runtimeNote); text: root.info("runtimeNote"); color: SentinelTheme.textMuted; wrapMode: Text.WordWrap }
                BusyIndicator { running: root.downloadTarget === 0 && ollamaModelDetailFetcher.fetching; visible: running; Layout.alignment: Qt.AlignHCenter }
                SentinelComboBox {
                    Layout.fillWidth: true
                    visible: root.downloadTarget === 0 && root.canPull && ollamaModelDetailFetcher.tags.length > 0
                    model: ollamaModelDetailFetcher.tags
                    textRole: "fullTag"
                    Accessible.name: qsTr("Model variant and quantization")
                    onModelChanged: {
                        if (!model || model.length === 0) { root.selectedTagObj = null; return }
                        var requested = root.modelInfo && root.modelInfo.ollamaId ? root.modelInfo.ollamaId : ""
                        if (requested.indexOf(':') < 0) requested += ":latest"
                        var chosen = 0
                        for (var i = 0; i < model.length; i++) if (model[i].fullTag === requested) { chosen = i; break }
                        currentIndex = chosen
                        root.selectedTagObj = model[chosen]
                    }
                    onActivated: root.selectedTagObj = model[currentIndex]
                }
                Label {
                    Layout.fillWidth: true
                    text: ollamaModelDetailFetcher.readme
                    visible: root.downloadTarget === 0 && root.canPull && text.length > 0
                    textFormat: Text.PlainText
                    wrapMode: Text.WordWrap
                    color: SentinelTheme.textPrimary
                }
                Label {
                    Layout.fillWidth: true
                    visible: root.targetError.length > 0
                    text: root.targetError
                    wrapMode: Text.WordWrap
                    color: SentinelTheme.warning
                }
            }
        }
        Label { Layout.fillWidth: true; visible: root.activePull || ggufLibraryFetcher.pulling; text: root.activePull ? root.pullStatus : ggufLibraryFetcher.statusText; color: SentinelTheme.textMuted; wrapMode: Text.WordWrap }
        ProgressBar { Layout.fillWidth: true; visible: root.activePull || ggufLibraryFetcher.pulling; value: root.activePull ? root.pullProgress : ggufLibraryFetcher.progress }
        Label { text: qsTr("Download destination"); color: SentinelTheme.textPrimary; font.bold: true }
        SentinelComboBox {
            id: destinationSelector
            Layout.fillWidth: true
            model: ["Ollama", "llama.cpp", "LM Studio"]
            currentIndex: root.downloadTarget
            Accessible.name: qsTr("Download destination")
            onActivated: root.downloadTarget = currentIndex
        }
        Label {
            Layout.fillWidth: true
            text: root.targetExplanation
            color: root.targetAvailable ? SentinelTheme.textMuted : SentinelTheme.warning
            wrapMode: Text.WordWrap
        }
        Flow {
            Layout.fillWidth: true
            spacing: SentinelTheme.spaceSm
            SentinelButton {
                text: root.targetAction
                enabled: root.targetAvailable && !ollamaPuller.pulling && !ggufLibraryFetcher.pulling
                onClicked: {
                    if (root.downloadTarget === 0) root.downloadRequested(root.effectiveOllamaId)
                    else if (root.downloadTarget === 2) Qt.openUrlExternally(root.managerUrl)
                    else if (!root.gguf) {
                        root.ggufSearchRequested(root.managerRepository ? root.managerRepository.split('/').pop() : (root.modelInfo.ollamaId || root.modelInfo.name).split(':')[0])
                        root.close()
                    } else if (root.modelInfo.installed) {
                        ggufLibraryFetcher.select(root.modelInfo.id)
                        root.close()
                    } else ggufLibraryFetcher.download(root.modelInfo.id)
                }
            }
            SentinelButton { visible: root.modelInfo && !!root.modelInfo.externalUrl; text: qsTr("Model source ↗"); onClicked: Qt.openUrlExternally(root.modelInfo.externalUrl) }
            SentinelButton { visible: root.activePull || ggufLibraryFetcher.pulling; text: qsTr("Cancel download"); onClicked: { if (ggufLibraryFetcher.pulling) ggufLibraryFetcher.cancel(); else root.cancelRequested() } }
        }
    }
}
