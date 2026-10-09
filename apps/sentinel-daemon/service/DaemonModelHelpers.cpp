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
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLocale>
#include <QFutureWatcher>
#include <QtConcurrentRun>
static void initModelCatalog() {
    Q_INIT_RESOURCE(model_catalog);
}
namespace sentinel::daemon {
DaemonModelHelpers::DaemonModelHelpers(core::ApplicationController* controller, QObject* parent)
    : QObject(parent), puller(this), controller_(controller), library(this), detail(this),
      lmStudio(this) {
    controller_->modelOperations()->huggingFaceSource()->setAutoFetch(false);
    initModelCatalog();
    QFile file(":/sentinel/model-catalog.json");
    if (file.open(QIODevice::ReadOnly))
        catalog_ = QJsonDocument::fromJson(file.readAll()).array();
    refreshTimer_.setInterval(15 * 60 * 1000);
    connect(&refreshTimer_, &QTimer::timeout, this, [this] { refreshCatalog(true); });
    refreshTimer_.start();
}

DaemonModelHelpers::~DaemonModelHelpers() {
    if (modelRowsCancelled_) modelRowsCancelled_->store(true);
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
        QJsonArray models = modelRows_;
        if (!modelRowsBuilding_ && !controller_->modelOperations()->huggingFaceSource()->fetching() && (!modelRowsAge_.isValid() || modelRowsAge_.elapsed() >= 10000))
            const_cast<DaemonModelHelpers*>(this)->rebuildModelRows();
        const auto record = controller_->modelOperations()->operation(activeOperation_);
        const auto pending = record.state == core::ModelOperationState::Queued ||
                             record.state == core::ModelOperationState::Running;
        const auto* source = controller_->modelOperations()->huggingFaceSource();
        const bool sourceFailed = !source->fetching() &&
            source->catalogState() != core::HuggingFaceCatalogState::Current &&
            source->catalogState() != core::HuggingFaceCatalogState::Empty;
        return {{"models", models},
                {"installedCount", installedCount_},
                {"runtimeInstalled", !runtimeBinary().isEmpty()},
                {"setupStatus", setupStatus_}, {"setupBusy", setupBusy_},
                {"catalog", catalog_},
                {"hasMore", catalogPage_ + 1 < catalogPages_ || source->hasMore()},
                {"hasPrevious", catalogPage_ > 0},
                {"catalogPage", catalogPage_}, {"catalogPages", catalogPages_},
                {"query", queryKey_},
                {"catalogStatus", source->catalogDetail()},
                {"fetching", modelRowsBuilding_ || controller_->modelOperations()->huggingFaceSource()->fetching()},
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
void DaemonModelHelpers::rebuildModelRows() {
    const auto* source = controller_->modelOperations()->huggingFaceSource();
    const auto snapshot = source->snapshot();
    const auto localEntries = controller_->modelLibrary()->entries(source);
    const auto loadedStudio = controller_->loadedLMStudioModelNames();
    const auto generation = modelRowsGeneration_;
    const bool ggufOnly = ggufOnly_;
    const int page = catalogPage_;
    modelRowsBuilding_ = true;
    modelRowsCancelled_ = std::make_shared<std::atomic_bool>(false);
    const auto cancelled = modelRowsCancelled_;
    auto* watcher = new QFutureWatcher<ModelRows>(this);
    connect(watcher, &QFutureWatcher<ModelRows>::finished, this, [this, watcher, generation] {
        modelRowsBuilding_ = false;
        if (generation == modelRowsGeneration_) {
            const auto result = watcher->result();
            modelRows_ = result.models;
            installedCount_ = result.installed;
            catalogPages_ = result.pages;
            catalogPage_ = qMin(catalogPage_, catalogPages_ - 1);
            modelRowsAge_.start();
        }
        watcher->deleteLater();
    });
    watcher->setFuture(QtConcurrent::run([snapshot, localEntries, loadedStudio, ggufOnly, cancelled, page] {
        int pages = 1;
        const auto window = core::HuggingFaceModelSource::pageSnapshot(snapshot, page, 40, &pages);
        QJsonArray models;
        QSet<QString> represented;
        QSet<QString> installed;
        auto append = [&](const core::ModelLibraryEntry& entry, const core::HuggingFaceRepository& repository) {
            const bool gguf = entry.format == "GGUF";
            if (ggufOnly && !gguf)
                return;
            const bool artifact = gguf || entry.format == "safetensors" || entry.artifactFilename.endsWith(".bin") || entry.artifactFilename.endsWith(".onnx");
            if (entry.source.id != "hugging-face" && !gguf) return;
            if (!artifact && (entry.repositoryId.isEmpty() || represented.contains(entry.repositoryId))) return;
            represented.insert(entry.repositoryId);
            const auto category = core::modelCategory(entry.repositoryId + " " + entry.displayName,
                                                      entry.tags, repository.pipelineTask);
            const bool chatCompatible = category == "LLM" &&
                (repository.pipelineTask.isEmpty() || repository.pipelineTask == "text-generation");
            models.append(QJsonObject{
                {"id", artifact ? entry.id : "hf-repository:" + entry.repositoryId},
                {"name", gguf || repository.displayName.isEmpty() ? entry.displayName
                                                                  : repository.displayName},
                {"provider",
                 entry.publisher.isEmpty() ? entry.source.displayName : entry.publisher},
                {"category", category},
                {"catalogSource", "huggingface"},
                {"format", artifact ? entry.format : "Repository"},
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
                {"filename", artifact ? entry.artifactFilename : QString{}},
                {"sizeBytes",
                 entry.sizeBytes ? QJsonValue(*entry.sizeBytes) : QJsonValue{}},
                {"size",
                 entry.sizeBytes
                     ? QLocale().formattedDataSize(*entry.sizeBytes, 2) + " (" + QString::number(*entry.sizeBytes) + " bytes)"
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
                {"artifactDownloadable", artifact && core::ModelLibraryService::availableActions(entry).contains(core::ModelLibraryAction::Download)},
                {"context", repository.metadata.value("config").toObject().value("max_position_embeddings").isDouble()
                    ? QString::number(repository.metadata.value("config").toObject().value("max_position_embeddings").toInteger())
                    : repository.metadata.value("gguf").toObject().value("context_length").isDouble()
                    ? QString::number(repository.metadata.value("gguf").toObject().value("context_length").toInteger()) : QString{}},
                {"input", repository.pipelineTask.startsWith("text-") ? "Text" : repository.pipelineTask == "audio-to-audio" || repository.pipelineTask == "automatic-speech-recognition" ? "Audio" : repository.pipelineTask.startsWith("image-") ? "Image" : ""},
                {"bestFor", repository.pipelineTask},
                {"modelCard", repository.metadata.value("modelCard").toString().left(16000)},
                {"chatCompatible", chatCompatible},
                {"installed", !entry.localFile.isEmpty()},
                {"ollamaId", ""},
                {"externalUrl",
                 gguf ? entry.source.url : "https://huggingface.co/" + entry.repositoryId},
                {"localFile", entry.localFile},
                {"nativeModelId", entry.nativeModelId}});
        };

        auto consume = [&](core::ModelLibraryEntry entry, const core::HuggingFaceRepository& repository) {
            if (entry.id.isEmpty()) entry.id = core::ModelLibraryService::entryId(
                entry.provider.id, entry.runtime.id, entry.source.id, entry.artifactId);
            append(entry, repository);
            if (entry.installed == core::ModelLibraryInstalledState::Installed || !entry.localFile.isEmpty())
                installed.insert(!entry.localFile.isEmpty() ? QFileInfo(entry.localFile).canonicalFilePath()
                    : entry.provider.id + ":" + entry.nativeModelId);
        };
        for (const auto& entry : localEntries) consume(entry, {});
        core::HuggingFaceModelSource::visitSnapshot(window,
            [&](const core::HuggingFaceRepository& repository, const QList<core::ModelLibraryEntry>& entries) {
                for (const auto& entry : entries) {
                    if (cancelled->load()) break;
                    consume(entry, repository);
                }
            }, [cancelled] { return cancelled->load(); });
        for (const auto& name : loadedStudio) installed.insert("lm-studio:" + name);
        return ModelRows{models, static_cast<int>(installed.size()), pages};
    }));
}

bool DaemonModelHelpers::action(const QString& component, const QString& action,
                                const QString& value, const QString& endpoint) {
    if (component == "ggufLibraryFetcher" && controller_) {
        modelRowsAge_.invalidate();
        ++modelRowsGeneration_;
        if (modelRowsCancelled_) modelRowsCancelled_->store(true);
        auto* operations = controller_->modelOperations();
        if (action == "setupRuntime") return setupRuntime();
        if (action == "cancelSetup") {
            setupCancelled_ = true; setupBusy_ = false;
            setupStatus_ = "Runtime installation cancelled.";
            if (!setupProcess_.isEmpty()) setupProcesses_.terminate(setupProcess_);
            return true;
        }
        if (action == "fetch" || action == "refresh") {
            modelRows_ = {};
            queryKey_ = value;
            const auto query = QJsonDocument::fromJson(value.toUtf8()).object();
            searchText_ = query.isEmpty() ? (value.isEmpty() ? "GGUF" : value)
                                          : query.value("text").toString();
            ggufOnly_ = query.value("ggufOnly").toBool();
            const auto category = query.value("category").toString();
            static const QHash<QString, QString> tasks{
                {"LLM", "text-generation"},       {"Think", "text-generation"},
                {"Vision", "image-text-to-text"}, {"Image", "text-to-image"},
                {"Video", "text-to-video"},       {"STT", "automatic-speech-recognition"},
                {"TTS", "text-to-speech"}, {"STS", "audio-to-audio"},        {"Embedding", "feature-extraction"}};
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
            if (catalogPage_ + 1 < catalogPages_) ++catalogPage_;
            else if (operations->huggingFaceSource()->hasMore()) {
                ++catalogPage_;
                operations->huggingFaceSource()->fetchMore();
            } else return false;
            modelRows_ = {};
        } else if (action == "previousPage") {
            if (catalogPage_ == 0)
                return false;
            --catalogPage_;
            modelRows_ = {};
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
        else if (action == "download" || action == "downloadFile" || action == "select") {
            for (const auto& entry : controller_->modelLibrary()->entries()) {
                if (entry.id != value)
                    continue;
                if (action == "download" || action == "downloadFile")
                    activeOperation_ =
                        operations->start(action == "download" && entry.format == "GGUF" && entry.shardGroup.isEmpty() ? core::ModelLibraryAction::DownloadAndRegister : core::ModelLibraryAction::Download, entry);
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

QString DaemonModelHelpers::runtimeBinary() const {
    auto binary = QStandardPaths::findExecutable("llama-server");
    if (!binary.isEmpty()) return binary;
    const auto root = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("runtimes/llama.cpp");
    return QStandardPaths::findExecutable("llama-server", {QCoreApplication::applicationDirPath(), "/opt/homebrew/bin", "/usr/local/bin", root + "/build/bin", root + "/build/bin/Release"});
}
bool DaemonModelHelpers::setupRuntime() {
    if (setupBusy_) return false;
    if (!runtimeBinary().isEmpty()) {
        setupStatus_ = "llama.cpp is installed. Choose or download a model.";
        controller_->setSelectedRuntimeProvider("llama-cpp-server");
        return true;
    }
    if (core::NetworkPolicyService::instance().check(QUrl("https://github.com/ggml-org/llama.cpp")) != core::NetworkDecision::Allowed) {
        setupStatus_ = "Runtime installation is blocked by network policy."; return false;
    }
    setupCancelled_ = false; setupBusy_ = true;
    runSetupStep(0);
    return true;
}
void DaemonModelHelpers::runSetupStep(int step) {
    if (setupCancelled_) return;
    const auto root = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath("runtimes/llama.cpp");
    core::ProcessRequest request;
    request.timeoutMs = 30 * 60 * 1000;
    request.unconfinedPermitted = true;
    request.environment = QProcessEnvironment::systemEnvironment();
    auto brew = QStandardPaths::findExecutable("brew", {"/opt/homebrew/bin", "/usr/local/bin", "/home/linuxbrew/.linuxbrew/bin"});
    const bool useBrew = !brew.isEmpty();
    if (useBrew) {
        request.program = brew; request.arguments = {"install", "llama.cpp"};
        setupStatus_ = "Installing llama.cpp with Homebrew…";
    } else {
        const auto git = QStandardPaths::findExecutable("git");
        const auto cmake = QStandardPaths::findExecutable("cmake");
        if (git.isEmpty() || cmake.isEmpty()) {
            setupBusy_ = false;
            setupStatus_ = "Install Git, CMake and a C++ compiler, then retry. On macOS Homebrew can install llama.cpp directly. Official setup: github.com/ggml-org/llama.cpp/blob/master/docs/install.md";
            return;
        }
        QDir().mkpath(QFileInfo(root).absolutePath());
        if (step == 0 && QFileInfo::exists(root + "/CMakeLists.txt")) step = 1;
        if (step == 0) { request.program = git; request.arguments = {"clone", "--depth", "1", "https://github.com/ggml-org/llama.cpp.git", root}; setupStatus_ = "Downloading official llama.cpp sources…"; }
        else if (step == 1) { request.program = cmake; request.arguments = {"-S", root, "-B", root + "/build", "-DCMAKE_BUILD_TYPE=Release", "-DLLAMA_CURL=OFF"}; setupStatus_ = "Configuring llama.cpp…"; }
        else { request.program = cmake; request.arguments = {"--build", root + "/build", "--config", "Release", "--target", "llama-server", "-j", "2"}; setupStatus_ = "Building llama.cpp…"; }
    }
    setupProcess_ = setupProcesses_.start(request, [this, step, useBrew](const core::ProcessRecord& record) {
        if (setupCancelled_ || (record.state != core::ProcessState::Exited && record.state != core::ProcessState::Failed && record.state != core::ProcessState::Cancelled)) return;
        if (record.state != core::ProcessState::Exited || record.exitCode != 0) {
            setupBusy_ = false;
            setupStatus_ = "Installation failed. Check Git/CMake/compiler availability and network access, then retry. " + record.error;
        } else if (!useBrew && step < 2) runSetupStep(step + 1);
        else {
            setupBusy_ = false;
            if (runtimeBinary().isEmpty()) setupStatus_ = "Installation finished but llama-server was not found. Check the installation output.";
            else { setupStatus_ = "llama.cpp installed. Choose or download a model."; controller_->setSelectedRuntimeProvider("llama-cpp-server"); controller_->refreshModelDiscovery(); }
        }
    }, [this](const QString&, core::ProcessStream, const QByteArray& output) {
        if (!setupCancelled_ && !output.isEmpty()) setupStatus_ = QString::fromUtf8(output).right(1200);
    });
}
} // namespace sentinel::daemon
