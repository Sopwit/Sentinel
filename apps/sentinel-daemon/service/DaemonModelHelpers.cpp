// SPDX-License-Identifier: GPL-3.0-or-later
#include "DaemonModelHelpers.h"
#include "sentinel/core/model/ModelCategory.h"
#include "sentinel/core/model/ModelOperationService.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include <QDesktopServices>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QUrlQuery>
static void initModelCatalog() {
    Q_INIT_RESOURCE(model_catalog);
}
namespace sentinel::daemon {
DaemonModelHelpers::DaemonModelHelpers(core::ApplicationController* controller, QObject* parent)
    : QObject(parent), puller(this), controller_(controller), library(this), detail(this),
      lmStudio(this) {
    initModelCatalog();
    QFile file(":/sentinel/model-catalog.json");
    if (file.open(QIODevice::ReadOnly))
        catalog_ = QJsonDocument::fromJson(file.readAll()).array();
}

QJsonObject DaemonModelHelpers::state(const QString& component) const {
    if (component == "ggufLibraryFetcher" && controller_) {
        QJsonArray models;
        for (const auto& entry : controller_->modelLibrary()->entries()) {
            if (entry.format != "GGUF" || models.size() >= 100)
                continue;
            const auto category =
                core::modelCategory(entry.repositoryId + " " + entry.displayName, entry.tags);
            models.append(QJsonObject{
                {"id", entry.id},
                {"name", entry.displayName},
                {"provider",
                 entry.publisher.isEmpty() ? entry.source.displayName : entry.publisher},
                {"category", category},
                {"format", entry.format},
                {"quantization", entry.quantization},
                {"license", entry.license},
                {"architecture", entry.architecture},
                {"repositoryId", entry.repositoryId},
                {"filename", entry.artifactFilename},
                {"size",
                 entry.sizeBytes
                     ? QString::number(double(*entry.sizeBytes) / 1073741824.0, 'f', 2) + " GB"
                     : "—"},
                {"description", entry.catalogDetail.isEmpty()
                                    ? "GGUF artifact for local llama.cpp inference."
                                    : entry.catalogDetail},
                {"badge", entry.quantization.isEmpty() ? "GGUF" : entry.quantization},
                {"badgeColor", "#10b981"},
                {"tags", QJsonArray::fromStringList(entry.tags)},
                {"downloadable", core::ModelLibraryService::availableActions(entry).contains(
                                     core::ModelLibraryAction::DownloadAndRegister) &&
                                     QString(category) == "LLM"},
                {"gguf", true},
                {"installed", !entry.localFile.isEmpty()},
                {"ollamaId", ""},
                {"externalUrl", entry.source.url},
                {"localFile", entry.localFile},
                {"nativeModelId", entry.nativeModelId}});
        }
        const auto record = controller_->modelOperations()->operation(activeOperation_);
        const auto pending = record.state == core::ModelOperationState::Queued ||
                             record.state == core::ModelOperationState::Running;
        return {{"models", models},
                {"catalog", catalog_},
                {"catalogStatus",
                 QStringLiteral("Bundled catalog checked 2026-10-08. Hugging Face: %1 Last "
                                "fetched: %2. Showing at most 100 GGUF artifacts.")
                     .arg(controller_->modelOperations()->huggingFaceSource()->catalogDetail(),
                          controller_->modelOperations()->huggingFaceSource()->fetchedAt().isValid()
                              ? controller_->modelOperations()
                                    ->huggingFaceSource()
                                    ->fetchedAt()
                                    .toString(Qt::ISODate)
                              : QStringLiteral("not yet fetched"))},
                {"fetching", controller_->modelOperations()->huggingFaceSource()->fetching()},
                {"pulling", !activeOperation_.isEmpty() && pending},
                {"activeModel", record.libraryId},
                {"progress", record.progress.value_or(0)},
                {"statusText", activeOperation_.isEmpty()
                                   ? controller_->property("localLlamaRuntimeStatus").toString()
                                   : record.statusText},
                {"errorText", record.state == core::ModelOperationState::Failed ? record.statusText
                                                                                : QString{}}};
    }
    if (component == "ollamaPuller")
        return {{"pulling", puller.pulling()},
                {"activeModel", puller.activeModel()},
                {"progress", puller.progress()},
                {"statusText", puller.statusText()},
                {"errorText", puller.errorText()}};
    if (component == "ollamaLibraryFetcher")
        return {{"fetching", library.fetching()},
                {"models", QJsonArray::fromVariantList(library.models())},
                {"errorText", library.errorText()}};
    if (component == "ollamaModelDetailFetcher")
        return {{"fetching", detail.fetching()},
                {"readme", detail.readme()},
                {"tags", QJsonArray::fromVariantList(detail.tags())},
                {"installCmd", detail.installCmd()},
                {"errorText", detail.errorText()}};
    if (component == "lmStudioLibraryFetcher")
        return {{"fetching", lmStudio.fetching()},
                {"models", QJsonArray::fromVariantList(lmStudio.models())},
                {"errorText", lmStudio.errorText()}};
    return {};
}
bool DaemonModelHelpers::action(const QString& component, const QString& action,
                                const QString& value, const QString& endpoint) {
    if (component == "ggufLibraryFetcher" && controller_) {
        auto* operations = controller_->modelOperations();
        if (action == "fetch" || action == "refresh")
            operations->huggingFaceSource()->search(value.isEmpty() ? "GGUF" : value,
                                                    action == "refresh");
        else if (action == "fetchDetails")
            operations->huggingFaceSource()->fetchRepository(value);
        else if (action == "import")
            activeOperation_ = operations->importGguf(
                QUrl(value).isLocalFile() ? QUrl(value).toLocalFile() : value,
                controller_->selectedRuntimeProvider() == "llama-cpp-server"
                    ? controller_->selectedLocalModel()
                    : QString{});
        else if (action == "cancel")
            return operations->cancel(activeOperation_);
        else if (action == "download" || action == "select") {
            for (const auto& entry : controller_->modelLibrary()->entries()) {
                if (entry.id != value || entry.format != "GGUF")
                    continue;
                if (action == "download")
                    activeOperation_ =
                        operations->start(core::ModelLibraryAction::DownloadAndRegister, entry);
                else if (!entry.localFile.isEmpty()) {
                    controller_->setSelectedRuntimeProvider("llama-cpp-server");
                    controller_->setSelectedLocalModel(
                        entry.nativeModelId.isEmpty() ? entry.displayName : entry.nativeModelId);
                } else
                    return false;
                return true;
            }
            return false;
        } else
            return false;
        return true;
    }
    const auto url = component == "ollamaPuller" ? QUrl(endpoint)
                                                 : QUrl(component == "lmStudioLibraryFetcher"
                                                            ? "https://lmstudio.ai/models"
                                                            : "https://ollama.com/library");
    if (action != "cancel" &&
        core::NetworkPolicyService::instance().check(url) != core::NetworkDecision::Allowed)
        return false;
    if (component == "ollamaPuller") {
        puller.setEndpoint(endpoint);
        if (action == "pull" && !value.trimmed().isEmpty())
            puller.pull(value);
        else if (action == "removeModel" && !value.trimmed().isEmpty())
            puller.removeModel(value);
        else if (action == "cancel")
            puller.cancel();
        else
            return false;
    } else if (component == "ollamaLibraryFetcher") {
        if (action == "fetch")
            library.fetch(value);
        else if (action == "cancel")
            library.cancel();
        else
            return false;
    } else if (component == "ollamaModelDetailFetcher") {
        if (action == "fetchDetails" && !value.trimmed().isEmpty())
            detail.fetchDetails(value);
        else if (action == "cancel")
            detail.cancel();
        else
            return false;
    } else if (component == "lmStudioLibraryFetcher") {
        if (action == "fetch")
            lmStudio.fetch();
        else if (action == "cancel")
            lmStudio.cancel();
        else
            return false;
    } else
        return false;
    return true;
}
} // namespace sentinel::daemon
