// SPDX-License-Identifier: GPL-3.0-or-later
#include "DaemonModelHelpers.h"
#include "sentinel/core/model/ModelCategory.h"
#include "sentinel/core/model/ModelOperationService.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include <QDesktopServices>
#include <QFile>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
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
    refreshTimer_.setInterval(15 * 60 * 1000);
    connect(&refreshTimer_, &QTimer::timeout, this, [this] { refreshCatalog(true); });
    refreshTimer_.start();
}

void DaemonModelHelpers::refreshCatalog(bool force) {
    if (!controller_)
        return;
    auto* source = controller_->modelOperations()->huggingFaceSource();
    if (!source->fetching()) {
        catalogPage_ = 0;
        source->searchCatalog(searchText_, searchTask_, searchSort_, force, ggufOnly_);
    }
    if (!library.fetching() && core::NetworkPolicyService::instance().check(QUrl(
                                   "https://ollama.com/library")) == core::NetworkDecision::Allowed)
        library.fetch(ollamaSort_);
    if (!lmStudio.fetching() &&
        core::NetworkPolicyService::instance().check(QUrl("https://lmstudio.ai/models")) ==
            core::NetworkDecision::Allowed)
        lmStudio.fetch();
}

QJsonObject DaemonModelHelpers::state(const QString& component) const {
    if (component == "ggufLibraryFetcher" && controller_) {
        QJsonArray models;
        QHash<QString, core::HuggingFaceRepository> repositories;
        for (const auto& repository :
             controller_->modelOperations()->huggingFaceSource()->repositories())
            repositories.insert(repository.id, repository);
        int eligible = 0;
        QSet<QString> represented;
        for (const auto& entry : controller_->modelLibrary()->entries()) {
            const bool gguf = entry.format == "GGUF";
            if (ggufOnly_ && !gguf)
                continue;
            if (!gguf && (entry.repositoryId.isEmpty() || represented.contains(entry.repositoryId)))
                continue;
            if (!gguf)
                represented.insert(entry.repositoryId);
            if (eligible++ < catalogPage_ * 40 || models.size() >= 40)
                continue;
            const auto category =
                core::modelCategory(entry.repositoryId + " " + entry.displayName, entry.tags);
            const auto repository = repositories.value(entry.repositoryId);
            const bool chatCompatible = category == "LLM" &&
                (repository.pipelineTask.isEmpty() || repository.pipelineTask == "text-generation");
            models.append(QJsonObject{
                {"id", gguf ? entry.id : "hf-repository:" + entry.repositoryId},
                {"name", gguf || repository.displayName.isEmpty() ? entry.displayName
                                                                  : repository.displayName},
                {"provider",
                 entry.publisher.isEmpty() ? entry.source.displayName : entry.publisher},
                {"category", category},
                {"catalogSource", "huggingface"},
                {"format", gguf ? entry.format : "Repository"},
                {"quantization", entry.quantization},
                {"license", entry.license},
                {"architecture", entry.architecture},
                {"lastUpdated", repository.lastUpdated},
                {"pipelineTask", repository.pipelineTask},
                {"downloads",
                 repository.downloads ? QJsonValue(*repository.downloads) : QJsonValue{}},
                {"likes", repository.likes ? QJsonValue(*repository.likes) : QJsonValue{}},
                {"gated", repository.gated},
                {"revision", entry.revision},
                {"repositoryId", entry.repositoryId},
                {"filename", gguf ? entry.artifactFilename : QString{}},
                {"size",
                 gguf && entry.sizeBytes
                     ? QString::number(double(*entry.sizeBytes) / 1073741824.0, 'f', 2) + " GB"
                     : "—"},
                {"description", repository.pipelineTask.isEmpty()
                                    ? "Model repository; review the source for runtime and "
                                      "installation requirements."
                                    : QString("Source task: %1. Review the publisher's model card "
                                              "for requirements and examples.")
                                          .arg(repository.pipelineTask)},
                {"runtimeNote",
                 repository.gated
                     ? "The publisher requires access approval on Hugging Face before downloading."
                     : QString{}},
                {"badge", gguf ? (entry.quantization.isEmpty() ? entry.format : entry.quantization)
                               : "Hugging Face"},
                {"badgeColor", "#10b981"},
                {"tags", QJsonArray::fromStringList(entry.tags)},
                {"downloadable", core::ModelLibraryService::availableActions(entry).contains(
                                     core::ModelLibraryAction::DownloadAndRegister) &&
                                     gguf && chatCompatible},
                {"gguf", gguf},
                {"chatCompatible", chatCompatible},
                {"installed", !entry.localFile.isEmpty()},
                {"ollamaId", ""},
                {"externalUrl",
                 gguf ? entry.source.url : "https://huggingface.co/" + entry.repositoryId},
                {"localFile", entry.localFile},
                {"nativeModelId", entry.nativeModelId}});
        }
        const auto record = controller_->modelOperations()->operation(activeOperation_);
        const auto pending = record.state == core::ModelOperationState::Queued ||
                             record.state == core::ModelOperationState::Running;
        const auto* source = controller_->modelOperations()->huggingFaceSource();
        const bool sourceFailed = !source->fetching() &&
            source->catalogState() != core::HuggingFaceCatalogState::Current &&
            source->catalogState() != core::HuggingFaceCatalogState::Empty;
        return {{"models", models},
                {"catalog", catalog_},
                {"hasMore", eligible > (catalogPage_ + 1) * 40 ||
                                controller_->modelOperations()->huggingFaceSource()->hasMore()},
                {"hasPrevious", catalogPage_ > 0},
                {"catalogStatus",
                 QStringLiteral("Bundled catalog checked 2026-10-08. Hugging Face: %1 Last "
                                "fetched: %2. Automatic refresh every 15 minutes. Discovery page "
                                "%3 (40 entries per page).")
                     .arg(controller_->modelOperations()->huggingFaceSource()->catalogDetail(),
                          controller_->modelOperations()->huggingFaceSource()->fetchedAt().isValid()
                              ? controller_->modelOperations()
                                    ->huggingFaceSource()
                                    ->fetchedAt()
                                    .toString(Qt::ISODate)
                              : QStringLiteral("not yet fetched"))
                     .arg(catalogPage_ + 1)},
                {"fetching", controller_->modelOperations()->huggingFaceSource()->fetching()},
                {"pulling", !activeOperation_.isEmpty() && pending},
                {"activeModel", record.libraryId},
                {"progress", record.progress.value_or(0)},
                {"statusText", activeOperation_.isEmpty()
                                   ? controller_->property("localLlamaRuntimeStatus").toString()
                                   : record.statusText},
                {"errorText", record.state == core::ModelOperationState::Failed ? record.statusText
                                                                                : sourceFailed ? source->catalogDetail() : QString{}}};
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
                {"catalogStatus",
                 QString("Ollama last fetched: %1")
                     .arg(library.fetchedAt().isEmpty() ? "not yet fetched" : library.fetchedAt())},
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
                {"catalogStatus", QString("LM Studio last fetched: %1")
                                      .arg(lmStudio.fetchedAt().isEmpty() ? "not yet fetched"
                                                                          : lmStudio.fetchedAt())},
                {"errorText", lmStudio.errorText()}};
    return {};
}
bool DaemonModelHelpers::action(const QString& component, const QString& action,
                                const QString& value, const QString& endpoint) {
    if (component == "ggufLibraryFetcher" && controller_) {
        auto* operations = controller_->modelOperations();
        if (action == "fetch" || action == "refresh") {
            const auto query = QJsonDocument::fromJson(value.toUtf8()).object();
            searchText_ = query.isEmpty() ? (value.isEmpty() ? "GGUF" : value)
                                          : query.value("text").toString();
            ggufOnly_ = query.value("ggufOnly").toBool();
            const auto category = query.value("category").toString();
            static const QHash<QString, QString> tasks{
                {"LLM", "text-generation"},       {"Think", "text-generation"},
                {"Vision", "image-text-to-text"}, {"Image", "text-to-image"},
                {"Video", "text-to-video"},       {"STT", "automatic-speech-recognition"},
                {"TTS", "text-to-speech"},        {"Embedding", "feature-extraction"}};
            searchTask_ = tasks.value(category);
            const auto requestedTask = query.value("task").toString();
            if ((category == "Video" && requestedTask == "image-to-video") ||
                (category == "Image" && requestedTask == "image-to-image"))
                searchTask_ = requestedTask;
            searchSort_ =
                query.value("sort").toString() == "lastModified" ? "lastModified" : "downloads";
            catalogPage_ = 0;
            operations->huggingFaceSource()->searchCatalog(searchText_, searchTask_, searchSort_,
                                                           action == "refresh", ggufOnly_);
        } else if (action == "nextPage") {
            if (operations->huggingFaceSource()->fetching())
                return false;
            int count = 0;
            QSet<QString> represented;
            for (const auto& entry : controller_->modelLibrary()->entries()) {
                if (entry.format == "GGUF")
                    ++count;
                else if (!ggufOnly_ && !entry.repositoryId.isEmpty() &&
                         !represented.contains(entry.repositoryId)) {
                    represented.insert(entry.repositoryId);
                    ++count;
                }
            }
            if (count > (catalogPage_ + 1) * 40)
                ++catalogPage_;
            else if (operations->huggingFaceSource()->hasMore()) {
                catalogPage_ = 0;
                operations->huggingFaceSource()->fetchMore();
            } else
                return false;
        } else if (action == "previousPage") {
            if (catalogPage_ == 0)
                return false;
            --catalogPage_;
        } else if (action == "fetchDetails")
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
            library.fetch(ollamaSort_ = value);
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
