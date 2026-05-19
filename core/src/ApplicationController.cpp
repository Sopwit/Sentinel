#include "sentinel/core/ApplicationController.h"

#include "sentinel/core/InMemoryConversationStore.h"
#include "sentinel/core/InMemoryMemoryCandidateStore.h"
#include "sentinel/core/LocalRuntime.h"
#include "sentinel/core/NullToolExecutor.h"
#include "sentinel/core/StaticAgentRegistry.h"
#include "sentinel/core/StaticApprovalPolicy.h"
#include "sentinel/core/StaticMemoryCatalog.h"
#include "sentinel/core/StaticModelRouter.h"
#include "sentinel/core/StaticProviderCatalog.h"
#include "sentinel/core/StaticSandboxPolicy.h"
#include "sentinel/core/StaticTaskPlanner.h"

#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

#include <algorithm>
#include <cstdint>

namespace sentinel::core {

namespace {

AgentActivityStatus planActivityStatus(ToolInvocationPlanStatus status) {
    return status == ToolInvocationPlanStatus::Planned ? AgentActivityStatus::Completed
                                                       : AgentActivityStatus::Blocked;
}

AgentActivityStatus approvalActivityStatus(ApprovalStatus status) {
    return status == ApprovalStatus::Denied || status == ApprovalStatus::RequiresApproval
               ? AgentActivityStatus::Blocked
               : AgentActivityStatus::Completed;
}

AgentActivityStatus sandboxActivityStatus(SandboxStatus status) {
    return status == SandboxStatus::Allowed ? AgentActivityStatus::Completed
                                            : AgentActivityStatus::Blocked;
}

AgentActivityStatus executionActivityStatus(ToolExecutionStatus status) {
    return status == ToolExecutionStatus::PlaceholderSucceeded ? AgentActivityStatus::Completed
                                                               : AgentActivityStatus::Blocked;
}

OrchestrationHealthStatus healthStatusFor(const QString& routingStatus,
                                          const QString& taskPlanStatus) {
    if (routingStatus == QStringLiteral("Unavailable") ||
        taskPlanStatus == QStringLiteral("Blocked")) {
        return OrchestrationHealthStatus::Degraded;
    }

    if (routingStatus.isEmpty() || taskPlanStatus.isEmpty()) {
        return OrchestrationHealthStatus::Unknown;
    }

    return OrchestrationHealthStatus::Ready;
}

QString localInferenceChatFailureMessage(const LocalInferenceResponse& response) {
    switch (response.error) {
    case LocalInferenceError::BlankPrompt:
        return QStringLiteral("Local inference did not run: the prompt was blank.");
    case LocalInferenceError::MissingModel:
        return QStringLiteral("Local inference did not run: no local model is selected. Select a "
                              "discovered Ollama model in Settings or pass an explicit model.");
    case LocalInferenceError::EndpointBlocked:
        return QStringLiteral("Local inference blocked: Ollama must use local loopback HTTP.");
    case LocalInferenceError::ModelUnavailable:
        return QStringLiteral("Local inference did not run: the selected model is not available "
                              "in discovered Ollama metadata.");
    case LocalInferenceError::PermissionDenied:
        return QStringLiteral("Local inference blocked by runtime permission policy.");
    case LocalInferenceError::SafetyBlocked:
        return QStringLiteral("Local inference blocked by runtime safety policy.");
    case LocalInferenceError::ClientUnavailable:
        return QStringLiteral("Local inference unavailable: the local Ollama client is not ready.");
    case LocalInferenceError::OllamaNotRunning:
        return QStringLiteral("Local inference unavailable: Ollama is not running locally.");
    case LocalInferenceError::EndpointUnreachable:
        return QStringLiteral("Local inference unavailable: the local Ollama endpoint is "
                              "unreachable.");
    case LocalInferenceError::Timeout:
        return QStringLiteral("Local inference failed: the local Ollama request timed out.");
    case LocalInferenceError::InvalidResponse:
        return QStringLiteral("Local inference failed: Ollama returned an invalid response.");
    case LocalInferenceError::StreamInterrupted:
        return QStringLiteral("Local inference failed: the Ollama stream ended before a complete "
                              "assistant response was received.");
    case LocalInferenceError::BusyRequest:
        return QStringLiteral("Local inference is already running. Wait for the current request "
                              "to finish before sending another message.");
    case LocalInferenceError::RequestFailed:
        if (!response.summary.trimmed().isEmpty()) {
            return QStringLiteral("Local inference failed: %1").arg(response.summary.trimmed());
        }
        return QStringLiteral("Local inference failed before a response was produced.");
    case LocalInferenceError::None:
        break;
    }

    return safeLocalInferenceSummary(response);
}

QString conversationSearchPreview(const QString& content, int matchIndex, int queryLength) {
    const auto trimmed = content.simplified();
    if (matchIndex < 0 || queryLength <= 0) {
        return trimmed.left(96);
    }

    const auto start = std::max<qsizetype>(0, matchIndex - 32);
    const auto end = std::min<qsizetype>(trimmed.size(), matchIndex + queryLength + 48);
    auto preview = trimmed.mid(start, end - start);
    if (start > 0) {
        preview.prepend(QStringLiteral("..."));
    }
    if (end < trimmed.size()) {
        preview.append(QStringLiteral("..."));
    }
    return preview;
}

QString conversationSearchResultSummary(const ConversationSearchResult& result) {
    return QStringLiteral("%1 #%2: %3")
        .arg(result.role, QString::number(result.messageId), result.preview);
}

ConversationBrowserStatus conversationBrowserStatusFor(const ConversationHistorySummary& summary) {
    const auto nonSystemCount = summary.userMessageCount + summary.assistantMessageCount;
    return nonSystemCount > 0 ? ConversationBrowserStatus::Ready
                              : ConversationBrowserStatus::EmptyTranscript;
}

QString conversationSearchAvailabilitySummary(const ConversationSearchSummary& summary) {
    if (summary.query.text.trimmed().isEmpty()) {
        return QStringLiteral("Search ready for current transcript.");
    }
    return QStringLiteral("%1 / %2").arg(conversationSearchStatusName(summary.status),
                                         summary.summary);
}

QString conversationExportAvailabilitySummary(const ConversationExportReadiness& readiness,
                                              const ConversationExportResult& result) {
    const auto availability =
        readiness.available ? QStringLiteral("Available") : QStringLiteral("Unavailable");
    if (!result.status.trimmed().isEmpty() && result.status != QStringLiteral("Not Run")) {
        return QStringLiteral("%1 / Last export: %2 (%3)")
            .arg(availability, result.status, result.summary);
    }
    return QStringLiteral("%1 / %2").arg(availability, readiness.summary);
}

ConversationExportFormat conversationExportFormatFromName(const QString& format) {
    const auto normalized = format.trimmed().toLower();
    if (normalized == QStringLiteral("markdown") || normalized == QStringLiteral("md")) {
        return ConversationExportFormat::Markdown;
    }
    if (normalized == QStringLiteral("json")) {
        return ConversationExportFormat::Json;
    }
    return ConversationExportFormat::PlainText;
}

QString defaultConversationExportDirectory() {
    const auto appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const auto basePath = appDataPath.isEmpty() ? QDir::currentPath() : appDataPath;
    return basePath + QStringLiteral("/exports");
}

QString sanitizedExportFileStem(QString stem) {
    stem = stem.trimmed().toLower();

    QString sanitized;
    sanitized.reserve(stem.size());
    bool previousDash = false;
    for (const auto ch : stem) {
        const bool allowed = ch.isLetterOrNumber() || ch == QLatin1Char('_');
        if (allowed) {
            sanitized.append(ch);
            previousDash = false;
            continue;
        }

        if (!previousDash) {
            sanitized.append(QLatin1Char('-'));
            previousDash = true;
        }
    }

    while (sanitized.startsWith(QLatin1Char('-'))) {
        sanitized.remove(0, 1);
    }
    while (sanitized.endsWith(QLatin1Char('-'))) {
        sanitized.chop(1);
    }

    return sanitized.isEmpty() ? QStringLiteral("sentinel-transcript") : sanitized.left(96);
}

QString conversationExportExtension(ConversationExportFormat format) {
    switch (format) {
    case ConversationExportFormat::Markdown:
        return QStringLiteral("md");
    case ConversationExportFormat::Json:
        return QStringLiteral("json");
    case ConversationExportFormat::PlainText:
        break;
    }

    return QString();
}

QString uniqueExportFilePath(const QDir& directory, const QString& baseName,
                             const QString& extension) {
    auto candidate = directory.filePath(QStringLiteral("%1.%2").arg(baseName, extension));
    int suffix = 2;
    while (QFileInfo::exists(candidate)) {
        candidate = directory.filePath(
            QStringLiteral("%1-%2.%3").arg(baseName, QString::number(suffix++), extension));
    }
    return candidate;
}

QString markdownTranscript(const QList<ChatMessage>& messages, const QString& exportedAtUtc) {
    QString output;
    QTextStream stream(&output);
    stream << "# Sentinel Transcript\n\n";
    stream << "Exported: " << exportedAtUtc << "\n";
    stream << "Messages: " << messages.size() << "\n\n";
    for (const auto& message : messages) {
        stream << "## " << chatRoleName(message.role) << " #" << message.id << "\n\n";
        stream << "- Timestamp: " << message.timestamp.toUTC().toString(Qt::ISODate) << "\n";
        stream << "- Status: " << chatMessageStatusName(message.status) << "\n\n";
        stream << message.content.trimmed() << "\n\n";
    }
    return output;
}

QByteArray jsonTranscript(const QList<ChatMessage>& messages, const QString& exportedAtUtc) {
    QJsonArray messageArray;
    for (const auto& message : messages) {
        QJsonObject object;
        object.insert(QStringLiteral("id"), message.id);
        object.insert(QStringLiteral("role"), chatRoleName(message.role));
        object.insert(QStringLiteral("status"), chatMessageStatusName(message.status));
        object.insert(QStringLiteral("timestamp"), message.timestamp.toUTC().toString(Qt::ISODate));
        object.insert(QStringLiteral("content"), message.content);
        messageArray.append(object);
    }

    QJsonObject root;
    root.insert(QStringLiteral("format"), QStringLiteral("sentinel.transcript.v1"));
    root.insert(QStringLiteral("exportedAt"), exportedAtUtc);
    root.insert(QStringLiteral("messageCount"), messages.size());
    root.insert(QStringLiteral("messages"), messageArray);
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool hasConfiguredVoicePath(const QString& path) {
    return !path.trimmed().isEmpty();
}

enum class VoicePathReadiness : std::uint8_t {
    Missing,
    Blocked,
    Ready,
};

struct VoicePathValidation {
    QString label;
    QString path;
    bool configured = false;
    bool exists = false;
    bool file = false;
    bool readable = false;
    bool executable = false;
    bool requiresExecutable = false;
    bool requiresFile = false;
};

QString voicePathReadinessName(VoicePathReadiness readiness) {
    switch (readiness) {
    case VoicePathReadiness::Missing:
        return QStringLiteral("Missing");
    case VoicePathReadiness::Blocked:
        return QStringLiteral("Blocked");
    case VoicePathReadiness::Ready:
        return QStringLiteral("Ready");
    }

    return QStringLiteral("Blocked");
}

QString displayedVoicePath(const QString& path) {
    return hasConfiguredVoicePath(path) ? path.trimmed() : QStringLiteral("not configured");
}

QString pathExistenceText(const QFileInfo& info) {
    return info.exists() ? QStringLiteral("exists") : QStringLiteral("missing");
}

QString pathReadabilityText(const QFileInfo& info) {
    return info.exists() && info.isReadable() ? QStringLiteral("readable")
                                              : QStringLiteral("unreadable");
}

QString pathExecutableText(const QFileInfo& info) {
    return info.exists() && info.isExecutable() ? QStringLiteral("executable")
                                                : QStringLiteral("non-executable");
}

VoicePathValidation validateVoicePath(const QString& label, const QString& path,
                                      bool requiresExecutable, bool requiresFile) {
    const auto trimmed = path.trimmed();
    const QFileInfo info(trimmed);
    const auto configured = hasConfiguredVoicePath(trimmed);
    return VoicePathValidation{
        label,
        displayedVoicePath(trimmed),
        configured,
        configured && info.exists(),
        configured && info.exists() && info.isFile(),
        configured && info.exists() && info.isReadable(),
        configured && info.exists() && info.isExecutable(),
        requiresExecutable,
        requiresFile,
    };
}

bool isVoicePathReady(const VoicePathValidation& validation) {
    if (!validation.configured || !validation.exists || !validation.readable) {
        return false;
    }
    if (validation.requiresFile && !validation.file) {
        return false;
    }
    if (validation.requiresExecutable && !validation.executable) {
        return false;
    }
    return true;
}

QString voicePathBlockedReason(const VoicePathValidation& validation) {
    if (!validation.configured) {
        return QStringLiteral("path is not configured");
    }
    if (!validation.exists) {
        return QStringLiteral("path is missing");
    }
    if (validation.requiresFile && !validation.file) {
        return QStringLiteral("path is not a file");
    }
    if (!validation.readable) {
        return QStringLiteral("path is not readable");
    }
    if (validation.requiresExecutable && !validation.executable) {
        return QStringLiteral("path is not executable");
    }
    return QStringLiteral("path is ready");
}

QString voicePathValidationSummary(const VoicePathValidation& validation) {
    if (!validation.configured) {
        return QStringLiteral("%1: Missing - path is not configured.").arg(validation.label);
    }

    QStringList metadata{
        validation.exists ? QStringLiteral("exists") : QStringLiteral("missing"),
        validation.readable ? QStringLiteral("readable") : QStringLiteral("unreadable"),
    };
    if (validation.requiresFile) {
        metadata.append(validation.file ? QStringLiteral("file") : QStringLiteral("not a file"));
    }
    if (validation.requiresExecutable) {
        metadata.append(validation.executable ? QStringLiteral("executable")
                                              : QStringLiteral("non-executable"));
    }

    return QStringLiteral("%1: %2 - %3 at %4.")
        .arg(validation.label,
             isVoicePathReady(validation) ? QStringLiteral("Ready") : QStringLiteral("Blocked"),
             metadata.join(QStringLiteral(", ")), validation.path);
}

QString voicePathStatusBadge(const VoicePathValidation& validation) {
    if (!validation.configured) {
        return QStringLiteral("%1: Missing").arg(validation.label);
    }
    if (isVoicePathReady(validation)) {
        return QStringLiteral("%1: Ready").arg(validation.label);
    }
    return QStringLiteral("%1: Blocked (%2)")
        .arg(validation.label, voicePathBlockedReason(validation));
}

VoicePathReadiness combinedReadiness(const QList<VoicePathValidation>& validations) {
    bool anyConfigured = false;
    for (const auto& validation : validations) {
        anyConfigured = anyConfigured || validation.configured;
    }
    if (!anyConfigured) {
        return VoicePathReadiness::Missing;
    }

    for (const auto& validation : validations) {
        if (!isVoicePathReady(validation)) {
            return VoicePathReadiness::Blocked;
        }
    }
    return VoicePathReadiness::Ready;
}

QString combinedBlockedReasons(const QList<VoicePathValidation>& validations) {
    QStringList reasons;
    for (const auto& validation : validations) {
        if (!isVoicePathReady(validation)) {
            reasons.append(
                QStringLiteral("%1 %2").arg(validation.label, voicePathBlockedReason(validation)));
        }
    }
    return reasons.join(QStringLiteral("; "));
}

QString binaryHintSummary(const QString& label, const QString& configuredPath,
                          const QStringList& knownPaths) {
    const QFileInfo configuredInfo(configuredPath.trimmed());
    if (hasConfiguredVoicePath(configuredPath) && configuredInfo.exists() &&
        configuredInfo.isReadable() && configuredInfo.isExecutable()) {
        return QStringLiteral("%1 hint: configured path is executable; no suggestion needed.")
            .arg(label);
    }

    for (const auto& path : knownPaths) {
        const QFileInfo info(path);
        if (info.exists() && info.isReadable() && info.isExecutable()) {
            return QStringLiteral("%1 hint: %2 appears executable. Suggestion only; settings are "
                                  "not changed automatically.")
                .arg(label, path);
        }
    }

    return QStringLiteral("%1 hint: no executable found in known Homebrew/local locations.")
        .arg(label);
}

QString modelHintSummary(const QString& label, const QString& configuredPath,
                         const QString& missingSummary) {
    const QFileInfo info(configuredPath.trimmed());
    if (hasConfiguredVoicePath(configuredPath) && info.exists() && info.isReadable()) {
        return QStringLiteral("%1 hint: configured path is readable; no suggestion needed.")
            .arg(label);
    }

    return QStringLiteral("%1 hint: %2").arg(label, missingSummary);
}

VoiceBinaryDescriptor configuredVoiceBinaryDescriptor(const QString& id, const QString& name,
                                                      VoiceCapability capability,
                                                      const QString& path,
                                                      const QString& runtimeName) {
    const QFileInfo info(path.trimmed());
    const auto configured = hasConfiguredVoicePath(path);
    const auto exists = configured && info.exists();
    return VoiceBinaryDescriptor{
        id,
        name,
        capability,
        exists ? VoiceBinaryStatus::PresentMetadata : VoiceBinaryStatus::Missing,
        displayedVoicePath(path),
        exists && info.isReadable() && info.isExecutable(),
        configured
            ? QStringLiteral("%1 binary metadata only: path %2, %3, %4. Sentinel will not execute "
                             "%1 in this phase.")
                  .arg(runtimeName, pathExistenceText(info), pathReadabilityText(info),
                       pathExecutableText(info))
            : QStringLiteral("%1 binary path is not configured; Sentinel will not execute %1.")
                  .arg(runtimeName),
    };
}

VoiceModelDescriptor configuredVoiceModelDescriptor(const QString& id, const QString& name,
                                                    VoiceCapability capability, const QString& path,
                                                    const QString& runtimeName) {
    const QFileInfo info(path.trimmed());
    const auto configured = hasConfiguredVoicePath(path);
    const auto exists = configured && info.exists();
    return VoiceModelDescriptor{
        id,
        name,
        capability,
        exists ? VoiceModelStatus::PresentMetadata : VoiceModelStatus::Missing,
        displayedVoicePath(path),
        exists && info.isReadable(),
        configured
            ? QStringLiteral("%1 model metadata only: path %2, %3. Sentinel will not load or scan "
                             "%1 models in this phase.")
                  .arg(runtimeName, pathExistenceText(info), pathReadabilityText(info))
            : QStringLiteral("%1 model path is not configured or loaded.").arg(runtimeName),
    };
}

bool hasConfiguredVoicePaths(const QString& piperBinaryPath, const QString& piperModelPath,
                             const QString& whisperBinaryPath, const QString& whisperModelPath) {
    return hasConfiguredVoicePath(piperBinaryPath) || hasConfiguredVoicePath(piperModelPath) ||
           hasConfiguredVoicePath(whisperBinaryPath) || hasConfiguredVoicePath(whisperModelPath);
}

VoiceRuntimeSafetyReport controlledPiperFileOutputSafetyReport(bool executionAllowed) {
    VoiceRuntimeSafetyReport report;
    report.status = executionAllowed ? QStringLiteral("Allowed") : QStringLiteral("Blocked");
    report.summary =
        executionAllowed
            ? QStringLiteral("Voice runtime safety allows explicit local Piper file output only.")
            : QStringLiteral("Voice runtime safety blocks Piper file output by default.");
    report.executionAllowed = executionAllowed;
    report.processExecutionAllowed = executionAllowed;
    report.microphoneAllowed = false;
    report.playbackAllowed = false;
    report.filesystemWideScanAllowed = false;
    report.downloadsAllowed = false;
    report.cloudAllowed = false;
    report.checks = {
        QStringLiteral("Execution: %1")
            .arg(executionAllowed ? QStringLiteral("Allowed") : QStringLiteral("Blocked")),
        QStringLiteral("Process execution: %1")
            .arg(executionAllowed ? QStringLiteral("Allowed") : QStringLiteral("Blocked")),
        QStringLiteral("Microphone: Blocked"),
        QStringLiteral("Playback: Blocked"),
        QStringLiteral("Filesystem-wide scan: Blocked"),
        QStringLiteral("Downloads: Blocked"),
        QStringLiteral("Cloud: Blocked"),
    };
    return report;
}

PiperTtsConfig configuredPiperTtsConfig(const QString& piperBinaryPath,
                                        const QString& piperModelPath,
                                        bool executionAllowed = false) {
    auto config = defaultDisabledPiperTtsConfig();
    const auto binary = configuredVoiceBinaryDescriptor(
        QStringLiteral("piper-binary"), QStringLiteral("Piper Binary"),
        VoiceCapability::TextToSpeech, piperBinaryPath, QStringLiteral("Piper"));
    const auto model = configuredVoiceModelDescriptor(
        QStringLiteral("piper-voice-model"), QStringLiteral("Piper Voice Model"),
        VoiceCapability::TextToSpeech, piperModelPath, QStringLiteral("Piper"));

    config.enabled =
        hasConfiguredVoicePath(piperBinaryPath) || hasConfiguredVoicePath(piperModelPath);
    config.processExecutionAllowed = executionAllowed;
    config.fileOutputAllowed = executionAllowed;
    config.audioPlaybackAllowed = false;
    config.binary = binary;
    config.voiceModel = PiperVoiceModelDescriptor{
        QStringLiteral("piper-voice-model"),
        QStringLiteral("Piper Voice Model"),
        model.status,
        model.expectedPath,
        {},
        {},
        model.loadAllowed,
        model.summary,
    };
    config.safetyReport = controlledPiperFileOutputSafetyReport(executionAllowed);
    config.summary = config.enabled
                         ? executionAllowed
                               ? QStringLiteral("Piper path configuration is enabled for explicit "
                                                "controlled file output only.")
                               : QStringLiteral("Piper path configuration is stored as metadata "
                                                "only; Piper execution and playback remain "
                                                "blocked.")
                         : QStringLiteral("Piper TTS adapter is disabled and not configured; it "
                                          "exposes readiness metadata only.");
    return config;
}

} // namespace

ApplicationController::ApplicationController(
    std::unique_ptr<IChatProvider> provider, std::unique_ptr<IMemoryStore> memoryStore,
    std::unique_ptr<ChatSession> chatSession, std::unique_ptr<IChatHistoryStore> chatHistoryStore,
    std::unique_ptr<IAgentRuntime> agentRuntime, std::unique_ptr<IApprovalPolicy> approvalPolicy,
    std::unique_ptr<ISandboxPolicy> sandboxPolicy, std::unique_ptr<IToolExecutor> toolExecutor,
    std::unique_ptr<IModelRouter> modelRouter, std::unique_ptr<IProviderCatalog> providerCatalog,
    std::unique_ptr<ITaskPlanner> taskPlanner, std::unique_ptr<IAgentRegistry> agentRegistry,
    std::unique_ptr<IMemoryCatalog> memoryCatalog, std::unique_ptr<ILocalRuntime> localRuntime,
    std::unique_ptr<ILocalRuntimeSessionManager> localRuntimeSessions,
    std::unique_ptr<IRuntimeCapabilityRegistry> runtimeCapabilities,
    std::unique_ptr<IRuntimePermissionPolicy> runtimePermissionPolicy,
    std::unique_ptr<IRuntimeSafetyPolicy> runtimeSafetyPolicy,
    std::unique_ptr<IRuntimePipeline> runtimePipeline,
    std::unique_ptr<IExecutionLifecycle> executionLifecycle,
    std::unique_ptr<ExecutionCoordinator> executionCoordinator,
    std::unique_ptr<ILocalRuntimeAdapter> localRuntimeAdapter,
    std::unique_ptr<IProviderRuntimeBridge> providerRuntimeBridge,
    std::unique_ptr<StaticRuntimeIntegrationReadiness> runtimeIntegrationReadiness,
    std::unique_ptr<IOllamaRuntimeClient> ollamaRuntimeClient,
    std::unique_ptr<ILocalInferenceClient> localInferenceClient,
    std::unique_ptr<ILocalInferenceStreamClient> localInferenceStreamClient,
    std::unique_ptr<IModelManagementService> modelManagementService,
    std::unique_ptr<ITextToSpeechProvider> textToSpeechProvider,
    std::unique_ptr<ISpeechToTextProvider> speechToTextProvider,
    std::unique_ptr<IVoiceRuntimeCoordinator> voiceRuntimeCoordinator,
    std::unique_ptr<IVoiceRuntimeEnvironment> voiceRuntimeEnvironment,
    std::unique_ptr<PiperTextToSpeechProvider> piperTextToSpeechProvider,
    std::unique_ptr<ILocalInferenceWorker> localInferenceWorker,
    std::unique_ptr<IConversationStore> conversationStore, QObject* parent)
    : QObject(parent), provider_(std::move(provider)), agentRuntime_(std::move(agentRuntime)),
      approvalPolicy_(approvalPolicy ? std::move(approvalPolicy)
                                     : std::make_unique<StaticApprovalPolicy>()),
      sandboxPolicy_(sandboxPolicy ? std::move(sandboxPolicy)
                                   : std::make_unique<StaticSandboxPolicy>()),
      toolExecutor_(toolExecutor ? std::move(toolExecutor) : std::make_unique<NullToolExecutor>()),
      providerCatalog_(providerCatalog ? std::move(providerCatalog)
                                       : std::make_unique<StaticProviderCatalog>()),
      modelRouter_(modelRouter ? std::move(modelRouter)
                               : std::make_unique<StaticModelRouter>(*providerCatalog_)),
      taskPlanner_(taskPlanner ? std::move(taskPlanner) : std::make_unique<StaticTaskPlanner>()),
      agentRegistry_(agentRegistry ? std::move(agentRegistry)
                                   : std::make_unique<StaticAgentRegistry>()),
      memoryCatalog_(memoryCatalog ? std::move(memoryCatalog)
                                   : std::make_unique<StaticMemoryCatalog>()),
      localRuntime_(localRuntime ? std::move(localRuntime) : std::make_unique<NullLocalRuntime>()),
      localRuntimeSessions_(localRuntimeSessions
                                ? std::move(localRuntimeSessions)
                                : std::make_unique<NullLocalRuntimeSessionManager>()),
      runtimeCapabilities_(runtimeCapabilities
                               ? std::move(runtimeCapabilities)
                               : std::make_unique<StaticRuntimeCapabilityRegistry>()),
      runtimePermissionPolicy_(runtimePermissionPolicy
                                   ? std::move(runtimePermissionPolicy)
                                   : std::make_unique<StaticRuntimePermissionPolicy>()),
      runtimeSafetyPolicy_(runtimeSafetyPolicy ? std::move(runtimeSafetyPolicy)
                                               : std::make_unique<StaticRuntimeSafetyPolicy>()),
      runtimePipeline_(runtimePipeline ? std::move(runtimePipeline)
                                       : std::make_unique<StaticRuntimePipeline>()),
      executionLifecycle_(executionLifecycle ? std::move(executionLifecycle)
                                             : std::make_unique<StaticExecutionLifecycle>()),
      executionCoordinator_(executionCoordinator ? std::move(executionCoordinator)
                                                 : std::make_unique<ExecutionCoordinator>()),
      localRuntimeAdapter_(localRuntimeAdapter ? std::move(localRuntimeAdapter)
                                               : std::make_unique<StaticLocalRuntimeAdapter>()),
      providerRuntimeBridge_(providerRuntimeBridge
                                 ? std::move(providerRuntimeBridge)
                                 : std::make_unique<StaticProviderRuntimeBridge>()),
      runtimeIntegrationReadiness_(runtimeIntegrationReadiness
                                       ? std::move(runtimeIntegrationReadiness)
                                       : std::make_unique<StaticRuntimeIntegrationReadiness>()),
      ollamaRuntimeClient_(ollamaRuntimeClient ? std::move(ollamaRuntimeClient)
                                               : std::make_unique<NullOllamaRuntimeClient>()),
      modelManagementService_(modelManagementService
                                  ? std::move(modelManagementService)
                                  : std::make_unique<StaticModelManagementService>()),
      textToSpeechProvider_(textToSpeechProvider ? std::move(textToSpeechProvider)
                                                 : std::make_unique<NullTextToSpeechProvider>()),
      speechToTextProvider_(speechToTextProvider ? std::move(speechToTextProvider)
                                                 : std::make_unique<NullSpeechToTextProvider>()),
      voiceRuntimeCoordinator_(voiceRuntimeCoordinator
                                   ? std::move(voiceRuntimeCoordinator)
                                   : std::make_unique<StaticVoiceRuntimeCoordinator>()),
      voiceRuntimeEnvironment_(voiceRuntimeEnvironment
                                   ? std::move(voiceRuntimeEnvironment)
                                   : std::make_unique<NullVoiceRuntimeEnvironment>()),
      piperTextToSpeechProvider_(
          piperTextToSpeechProvider
              ? std::move(piperTextToSpeechProvider)
              : std::make_unique<PiperTextToSpeechProvider>(
                    defaultDisabledPiperTtsConfig(), std::make_unique<ProcessPiperTtsClient>())),
      memoryStore_(std::move(memoryStore)),
      memoryCandidateStore_(std::make_unique<InMemoryMemoryCandidateStore>()),
      chatSession_(chatSession ? std::move(chatSession)
                               : std::make_unique<ChatSession>(std::make_unique<SystemClock>())),
      chatHistoryStore_(std::move(chatHistoryStore)),
      conversationStore_(conversationStore ? std::move(conversationStore)
                                           : std::make_unique<InMemoryConversationStore>()),
      conversationExportDirectory_(defaultConversationExportDirectory()) {
    localInferenceClientIsRealOllama_ =
        dynamic_cast<OllamaLocalInferenceClient*>(localInferenceClient.get()) != nullptr ||
        !localInferenceClient;
    localInferenceStreamClientIsRealOllama_ = dynamic_cast<OllamaLocalInferenceStreamClient*>(
                                                  localInferenceStreamClient.get()) != nullptr ||
                                              !localInferenceStreamClient;
    localInferenceWorker_ =
        localInferenceWorker
            ? std::move(localInferenceWorker)
            : std::make_unique<LocalInferenceWorker>(
                  localInferenceClient ? std::move(localInferenceClient)
                                       : std::make_unique<OllamaLocalInferenceClient>(
                                             ollamaRuntimeClient_->config()),
                  localInferenceStreamClient ? std::move(localInferenceStreamClient)
                                             : std::make_unique<OllamaLocalInferenceStreamClient>(
                                                   ollamaRuntimeClient_->config()),
                  this, localInferenceClientIsRealOllama_, localInferenceStreamClientIsRealOllama_);

    const auto persistedMessages = chatHistoryStore_ && chatHistoryStore_->isAvailable()
                                       ? chatHistoryStore_->loadMessages()
                                       : QList<ChatMessage>{};
    if (!persistedMessages.isEmpty()) {
        chatSession_->loadMessages(persistedMessages);
        conversationHistorySummary_.lastRestoredStatus =
            QStringLiteral("Restored %1 persisted messages.").arg(persistedMessages.size());
        conversationHistorySummary_.lastSavedStatus =
            QStringLiteral("Persisted transcript loaded.");
    } else {
        conversationHistorySummary_.lastRestoredStatus =
            chatHistoryStore_ && chatHistoryStore_->isAvailable()
                ? QStringLiteral("No persisted transcript restored.")
                : QStringLiteral("Chat history store unavailable; runtime-only transcript.");
        const auto message = chatSession_->appendSystemMessage(
            QStringLiteral("Sentinel Core online."), ChatMessageStatus::Received);
        if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
            chatHistoryStore_->appendMessage(message);
            conversationHistorySummary_.lastSavedStatus =
                chatHistoryStore_->lastError().isEmpty()
                    ? QStringLiteral("Saved initial system message.")
                    : QStringLiteral("Initial system message save failed.");
        } else {
            conversationHistorySummary_.lastSavedStatus =
                QStringLiteral("Runtime-only transcript; persistence unavailable.");
        }
    }
    initializeActiveConversation();
    refreshConversationHistorySummary();
    resetConversationSearchSummary();
    refreshLatestTaskPlan();
    refreshConversationSession();
}

QString ApplicationController::providerName() const {
    return provider_ ? provider_->name() : QStringLiteral("No Provider");
}

QString ApplicationController::providerStatus() const {
    return provider_ ? chatProviderStatusName(provider_->status()) : QStringLiteral("Unavailable");
}

QString ApplicationController::agentStatus() const {
    return agentRuntime_ ? agentStatusName(agentRuntime_->status()) : QStringLiteral("Unavailable");
}

QString ApplicationController::lastAgentResponse() const {
    return lastAgentResponse_;
}

QString ApplicationController::latestToolPlanStatus() const {
    return toolInvocationPlanStatusName(latestAgentPipelineResult_.planningStatus());
}

QString ApplicationController::latestToolPlanSummary() const {
    return safeToolPlanSummary(latestAgentPipelineResult_.plan);
}

QString ApplicationController::latestApprovalStatus() const {
    return approvalStatusName(latestAgentPipelineResult_.approvalStatus());
}

QString ApplicationController::latestApprovalSummary() const {
    return safeApprovalSummary(latestAgentPipelineResult_.approval);
}

QString ApplicationController::latestSandboxStatus() const {
    return sandboxStatusName(latestAgentPipelineResult_.sandboxStatus());
}

QString ApplicationController::latestSandboxSummary() const {
    return safeSandboxSummary(latestAgentPipelineResult_.sandbox);
}

QString ApplicationController::latestToolExecutionStatus() const {
    return toolExecutionStatusName(latestAgentPipelineResult_.executionStatus());
}

QString ApplicationController::latestToolExecutionSummary() const {
    return safeToolExecutionSummary(latestAgentPipelineResult_.execution);
}

QString ApplicationController::latestAgentPipelineStatus() const {
    return agentPipelineStatusName(latestAgentPipelineResult_);
}

QString ApplicationController::latestAgentPipelineSummary() const {
    return safeAgentPipelineSummary(latestAgentPipelineResult_);
}

QString ApplicationController::runtimeSessionId() const {
    return runtimeSession_.context().sessionId.value;
}

QString ApplicationController::runtimeContextStatus() const {
    return runtimeSession_.context().statusName();
}

QString ApplicationController::runtimeContextSummary() const {
    return safeAgentRuntimeContextSummary(runtimeSession_.context());
}

QStringList ApplicationController::runtimeContextActiveToolIds() const {
    return runtimeSession_.context().activePlannedToolIds;
}

const ConversationSession& ApplicationController::currentConversationSession() const {
    return conversationSession_.session();
}

QString ApplicationController::conversationSessionId() const {
    return conversationSession_.session().id.value;
}

QString ApplicationController::conversationSessionStatus() const {
    return conversationSession_.session().statusName();
}

QString ApplicationController::interactionMode() const {
    return conversationSession_.session().interactionModeName();
}

QString ApplicationController::attentionState() const {
    return conversationSession_.session().attentionStateName();
}

QString ApplicationController::contextWindowSummary() const {
    return safeRuntimeContextWindowSummary(conversationSession_.session().contextWindow);
}

QString ApplicationController::conversationState() const {
    return conversationStateName(conversationStateGraph_.currentState());
}

QString ApplicationController::conversationTransitionStatus() const {
    return conversationTransitionStatusName(conversationStateGraph_.lastTransitionResult().status);
}

QString ApplicationController::conversationTransitionSummary() const {
    return safeConversationTransitionSummary(conversationStateGraph_.lastTransitionResult());
}

QString ApplicationController::conversationRuntimeSummary() const {
    return QStringLiteral("%1 | %2 | %3 | %4")
        .arg(conversationState(), conversationRuntimeActiveRoute_, conversationRuntimeActiveModel_,
             conversationRuntimeStreaming_ ? QStringLiteral("Streaming")
                                           : QStringLiteral("Not streaming"));
}

QStringList ApplicationController::conversationRuntimeSummaryLines() const {
    return {
        QStringLiteral("State: %1").arg(conversationState()),
        QStringLiteral("Request: %1").arg(conversationRuntimeRequestId_),
        QStringLiteral("Model: %1").arg(conversationRuntimeActiveModel_),
        QStringLiteral("Route: %1").arg(conversationRuntimeActiveRoute_),
        QStringLiteral("Streaming: %1")
            .arg(conversationRuntimeStreaming_ ? QStringLiteral("Active")
                                               : QStringLiteral("Inactive")),
        QStringLiteral("Success: %1").arg(conversationRuntimeLastSuccessSummary_),
        QStringLiteral("Error: %1").arg(conversationRuntimeLastErrorSummary_),
        QStringLiteral("Latency: %1").arg(conversationRuntimeLastLatencySummary_),
    };
}

QString ApplicationController::conversationRuntimeRequestId() const {
    return conversationRuntimeRequestId_;
}

QString ApplicationController::conversationRuntimeActiveModel() const {
    return conversationRuntimeActiveModel_;
}

QString ApplicationController::conversationRuntimeActiveRoute() const {
    return conversationRuntimeActiveRoute_;
}

bool ApplicationController::conversationRuntimeStreaming() const {
    return conversationRuntimeStreaming_;
}

QString ApplicationController::conversationRuntimeLastSuccessSummary() const {
    return conversationRuntimeLastSuccessSummary_;
}

QString ApplicationController::conversationRuntimeLastErrorSummary() const {
    return conversationRuntimeLastErrorSummary_;
}

QString ApplicationController::conversationRuntimeLastLatencySummary() const {
    return conversationRuntimeLastLatencySummary_;
}

int ApplicationController::agentActivityCount() const {
    return agentActivityLog_.count();
}

QString ApplicationController::latestAgentActivitySummary() const {
    return agentActivityLog_.latestSummary();
}

QString ApplicationController::currentRoutingMode() const {
    return modelRouter_ ? routingModeName(modelRouter_->routingMode())
                        : routingModeName(RoutingMode::LocalOnly);
}

void ApplicationController::setRoutingModeByName(const QString& routingModeName) {
    if (!modelRouter_) {
        return;
    }

    const auto nextMode = routingModeFromName(routingModeName);
    if (nextMode == modelRouter_->routingMode()) {
        return;
    }

    modelRouter_->setRoutingMode(nextMode);
    refreshLatestTaskPlan();
    refreshConversationSession();
    emit modelRoutingChanged();
    emit taskPlanChanged();
    emit conversationSessionChanged();
    emit orchestrationSnapshotChanged();
}

QString ApplicationController::modelRoutingStatus() const {
    if (!modelRouter_) {
        return modelRoutingStatusName(ModelRoutingStatus::Unavailable);
    }
    return modelRoutingStatusName(
        modelRouter_->route(TaskClassification{TaskType::Unknown}).status);
}

QString ApplicationController::selectedModelProviderSummary() const {
    if (!modelRouter_) {
        return QStringLiteral("No model router available.");
    }
    return safeModelRouteSummary(modelRouter_->route(TaskClassification{TaskType::Unknown}));
}

QString ApplicationController::latestTaskPlanStatus() const {
    return taskPlanStatusName(latestTaskPlan_.status);
}

QString ApplicationController::latestTaskPlanSummary() const {
    return safeTaskPlanSummary(latestTaskPlan_);
}

int ApplicationController::plannedTaskStepCount() const {
    return static_cast<int>(latestTaskPlan_.steps.size());
}

int ApplicationController::registeredAgentCount() const {
    return agentRegistry_ ? static_cast<int>(agentRegistry_->agents().size()) : 0;
}

QStringList ApplicationController::activeAgentSummaries() const {
    QStringList summaries;
    if (!agentRegistry_) {
        return summaries;
    }

    for (const auto& agent : agentRegistry_->agents()) {
        if (isAgentAvailable(agent.state)) {
            summaries.append(agentDescriptorSummary(agent));
        }
    }
    return summaries;
}

QString ApplicationController::currentAgentSummary() const {
    return latestTaskPlan_.preferredAgentSummary.isEmpty()
               ? QStringLiteral("No agent metadata selected.")
               : latestTaskPlan_.preferredAgentSummary;
}

QString ApplicationController::currentMemoryAffinitySummary() const {
    return latestTaskPlan_.preferredMemorySummary.isEmpty()
               ? QStringLiteral("No memory taxonomy metadata selected.")
               : latestTaskPlan_.preferredMemorySummary;
}

int ApplicationController::providerCatalogCount() const {
    return providerCatalog_ ? static_cast<int>(providerCatalog_->entries().size()) : 0;
}

QStringList ApplicationController::providerCatalogSummaries() const {
    QStringList summaries;
    if (!providerCatalog_) {
        return summaries;
    }

    for (const auto& entry : providerCatalog_->entries()) {
        summaries.append(providerCatalogEntrySummary(entry));
    }
    return summaries;
}

int ApplicationController::memoryCatalogCount() const {
    return memoryCatalog_ ? static_cast<int>(memoryCatalog_->shards().size()) : 0;
}

QStringList ApplicationController::memoryCatalogSummaries() const {
    QStringList summaries;
    if (!memoryCatalog_) {
        return summaries;
    }

    for (const auto& shard : memoryCatalog_->shards()) {
        summaries.append(memoryShardSummary(shard));
    }
    return summaries;
}

OrchestrationSnapshot ApplicationController::currentOrchestrationSnapshot() const {
    WorkspaceStateSummary workspace{
        currentRoutingMode(),           modelRoutingStatus(),    selectedModelProviderSummary(),
        latestTaskPlanStatus(),         latestTaskPlanSummary(), currentAgentSummary(),
        currentMemoryAffinitySummary(), runtimeContextStatus(),  runtimeContextSummary(),
        latestAgentActivitySummary(),   providerCatalogCount(),  registeredAgentCount(),
        memoryCatalogCount(),           agentActivityCount(),
    };

    OrchestrationSnapshot snapshot;
    snapshot.healthStatus = healthStatusFor(workspace.routingStatus, workspace.taskPlanStatus);
    snapshot.workspace = workspace;
    snapshot.summary = QStringLiteral("%1 orchestration snapshot: %2 route, %3 task plan, %4 "
                                      "provider entries, %5 agents, %6 memory categories.")
                           .arg(orchestrationHealthStatusName(snapshot.healthStatus),
                                workspace.routingMode, workspace.taskPlanStatus)
                           .arg(workspace.providerCatalogCount)
                           .arg(workspace.registeredAgentCount)
                           .arg(workspace.memoryCatalogCount);
    snapshot.executionEnabled = false;
    snapshot.signalList = {
        OrchestrationSignal{
            QStringLiteral("routing"), QStringLiteral("Routing"),
            QStringLiteral("%1 / %2").arg(workspace.routingMode, workspace.routingStatus)},
        OrchestrationSignal{QStringLiteral("provider-model"), QStringLiteral("Provider Model"),
                            workspace.selectedProviderModelSummary},
        OrchestrationSignal{
            QStringLiteral("task-plan"), QStringLiteral("Task Plan"),
            QStringLiteral("%1 / %2").arg(workspace.taskPlanStatus, workspace.taskPlanSummary)},
        OrchestrationSignal{QStringLiteral("agent"), QStringLiteral("Agent"),
                            workspace.preferredAgentSummary},
        OrchestrationSignal{QStringLiteral("memory"), QStringLiteral("Memory"),
                            workspace.memoryAffinitySummary},
        OrchestrationSignal{QStringLiteral("catalogs"), QStringLiteral("Catalogs"),
                            QStringLiteral("%1 providers / %2 agents / %3 memory")
                                .arg(workspace.providerCatalogCount)
                                .arg(workspace.registeredAgentCount)
                                .arg(workspace.memoryCatalogCount)},
        OrchestrationSignal{QStringLiteral("runtime"), QStringLiteral("Runtime"),
                            QStringLiteral("%1 / activity %2")
                                .arg(workspace.runtimeContextStatus)
                                .arg(workspace.activityCount)},
    };
    return snapshot;
}

QString ApplicationController::orchestrationSnapshotStatus() const {
    return orchestrationHealthStatusName(currentOrchestrationSnapshot().healthStatus);
}

QString ApplicationController::orchestrationSnapshotSummary() const {
    return safeOrchestrationSnapshotSummary(currentOrchestrationSnapshot());
}

QStringList ApplicationController::orchestrationSignals() const {
    return orchestrationSignalSummaries(currentOrchestrationSnapshot());
}

OrchestrationReadinessReport ApplicationController::currentOrchestrationReadinessReport() const {
    return StaticOrchestrationDiagnostics{}.generate(OrchestrationDiagnosticsInput{
        currentOrchestrationSnapshot(),
        providerCatalog_ ? providerCatalog_->entries() : QList<ProviderCatalogEntry>{},
        providerCatalog_ != nullptr,
        agentRegistry_ != nullptr,
        memoryCatalog_ != nullptr,
        taskPlanner_ != nullptr,
    });
}

QString ApplicationController::orchestrationReadinessStatus() const {
    return currentOrchestrationReadinessReport().status;
}

QString ApplicationController::orchestrationReadinessSummary() const {
    return safeOrchestrationReadinessSummary(currentOrchestrationReadinessReport());
}

QStringList ApplicationController::orchestrationDiagnostics() const {
    return orchestrationDiagnosticSummaries(currentOrchestrationReadinessReport());
}

QString ApplicationController::localRuntimeStatus() const {
    if (!localRuntime_) {
        return localRuntimeStatusName(LocalRuntimeStatus::Unavailable);
    }
    return localRuntimeStatusName(localRuntime_->descriptor().status);
}

QString ApplicationController::localRuntimeHealth() const {
    if (!localRuntime_) {
        return localRuntimeHealthName(LocalRuntimeHealth::Unavailable);
    }
    return localRuntimeHealthName(localRuntime_->descriptor().health);
}

QString ApplicationController::localRuntimeSummary() const {
    if (!localRuntime_) {
        return QStringLiteral("No local runtime boundary available.");
    }
    return safeLocalRuntimeSummary(localRuntime_->descriptor());
}

QStringList ApplicationController::localRuntimeCapabilities() const {
    if (!localRuntime_) {
        return {};
    }
    return localRuntimeCapabilitySummaries(localRuntime_->descriptor().capabilities);
}

QString ApplicationController::localRuntimeResponseStatus() const {
    if (!localRuntime_) {
        return QStringLiteral("Unavailable");
    }
    return localRuntime_->evaluate(LocalRuntimeRequest{}).status;
}

QString ApplicationController::localRuntimeResponseSummary() const {
    if (!localRuntime_) {
        return QStringLiteral("No local runtime boundary available.");
    }
    return safeLocalRuntimeResponseSummary(localRuntime_->evaluate(LocalRuntimeRequest{}));
}

int ApplicationController::localRuntimeSessionCount() const {
    return localRuntimeSessions_ ? static_cast<int>(localRuntimeSessions_->sessions().size()) : 0;
}

QString ApplicationController::localRuntimeSessionStatus() const {
    if (!localRuntimeSessions_) {
        return localRuntimeSessionStatusName(LocalRuntimeSessionStatus::NotStarted);
    }
    return localRuntimeSessionStatusName(localRuntimeSessions_->currentSession().status);
}

QString ApplicationController::localRuntimeSessionHealth() const {
    if (!localRuntimeSessions_) {
        return localRuntimeSessionHealthName(LocalRuntimeSessionHealth::Unavailable);
    }
    return localRuntimeSessionHealthName(localRuntimeSessions_->currentSession().health);
}

QString ApplicationController::localRuntimeSessionSummary() const {
    if (!localRuntimeSessions_) {
        return QStringLiteral("No local runtime session metadata.");
    }
    return safeLocalRuntimeSessionSummary(localRuntimeSessions_->currentSession());
}

QString ApplicationController::localRuntimeAllocationSummary() const {
    if (!localRuntimeSessions_) {
        return QStringLiteral("No local runtime allocation metadata.");
    }
    return safeLocalRuntimeAllocationSummary(localRuntimeSessions_->currentSession().allocation);
}

QString ApplicationController::localRuntimeReservationSummary() const {
    if (!localRuntimeSessions_) {
        return QStringLiteral("No local runtime reservation metadata.");
    }
    return safeLocalRuntimeReservationSummary(localRuntimeSessions_->currentSession().reservation);
}

QStringList ApplicationController::localRuntimeSessionSummaries() const {
    if (!localRuntimeSessions_) {
        return {};
    }
    return sentinel::core::localRuntimeSessionSummaries(localRuntimeSessions_->sessions());
}

int ApplicationController::runtimeCapabilityCount() const {
    return runtimeCapabilities_ ? static_cast<int>(runtimeCapabilities_->capabilities().size()) : 0;
}

QStringList ApplicationController::enabledRuntimeCapabilitySummaries() const {
    if (!runtimeCapabilities_) {
        return {};
    }
    return sentinel::core::enabledRuntimeCapabilitySummaries(runtimeCapabilities_->capabilities());
}

QStringList ApplicationController::disabledRuntimeCapabilitySummaries() const {
    if (!runtimeCapabilities_) {
        return {};
    }
    return sentinel::core::disabledRuntimeCapabilitySummaries(runtimeCapabilities_->capabilities());
}

QString ApplicationController::runtimeNegotiationProfileSummary() const {
    if (!runtimeCapabilities_) {
        return QStringLiteral("No runtime negotiation profile metadata.");
    }
    return safeRuntimeNegotiationProfileSummary(runtimeCapabilities_->negotiate().profile);
}

QString ApplicationController::runtimeNegotiationSummary() const {
    if (!runtimeCapabilities_) {
        return QStringLiteral("No runtime negotiation metadata.");
    }
    return safeRuntimeNegotiationSummary(runtimeCapabilities_->negotiate());
}

QString ApplicationController::localOnlyRuntimeEnforcementSummary() const {
    if (!runtimeCapabilities_) {
        return QStringLiteral("No local-only runtime enforcement metadata.");
    }
    return sentinel::core::localOnlyRuntimeEnforcementSummary(runtimeCapabilities_->negotiate());
}

QString ApplicationController::runtimePermissionDecision() const {
    return runtimePermissionDecisionStatusName(currentRuntimePermissionDecision().status);
}

QString ApplicationController::runtimePermissionSummary() const {
    return safeRuntimePermissionDecisionSummary(currentRuntimePermissionDecision());
}

QString ApplicationController::runtimeSafetyDecision() const {
    return runtimeSafetyDecisionName(currentRuntimeSafetyReport().decision);
}

QString ApplicationController::runtimeSafetySummary() const {
    return safeRuntimeSafetySummary(currentRuntimeSafetyReport());
}

QString ApplicationController::runtimePipelineStatus() const {
    return runtimePipelineStatusName(currentRuntimePipelineResult().status);
}

QString ApplicationController::runtimePipelineSummary() const {
    return safeRuntimePipelineSummary(currentRuntimePipelineResult());
}

QStringList ApplicationController::runtimePipelineTraceSummaries() const {
    return sentinel::core::runtimePipelineTraceSummaries(currentRuntimePipelineResult().traces);
}

QString ApplicationController::executionLifecycleState() const {
    return executionLifecycleStateName(currentExecutionLifecycleResult().state);
}

QString ApplicationController::executionLifecycleStatus() const {
    return executionLifecycleStatusName(currentExecutionLifecycleResult().status);
}

QString ApplicationController::executionLifecycleSummary() const {
    return safeExecutionLifecycleSummary(currentExecutionLifecycleResult());
}

QStringList ApplicationController::executionLifecycleTraceSummaries() const {
    return sentinel::core::executionLifecycleTraceSummaries(
        currentExecutionLifecycleResult().traces);
}

QString ApplicationController::executionSessionId() const {
    return executionCoordinator_ ? executionCoordinator_->session().id.value : QString{};
}

QString ApplicationController::executionSessionStatus() const {
    return executionCoordinator_
               ? executionSessionStatusName(executionCoordinator_->session().status)
               : QStringLiteral("Blocked");
}

QString ApplicationController::executionSessionOwnership() const {
    return executionCoordinator_
               ? executionOwnershipName(executionCoordinator_->session().ownership)
               : QStringLiteral("Application Controller");
}

QString ApplicationController::executionCoordinationMode() const {
    return executionCoordinator_
               ? executionCoordinationModeName(executionCoordinator_->session().coordinationMode)
               : QStringLiteral("Metadata Only");
}

QString ApplicationController::executionSessionSummary() const {
    return executionCoordinator_ ? safeExecutionSessionSummary(executionCoordinator_->session())
                                 : QStringLiteral("No execution session metadata.");
}

QString ApplicationController::executionCoordinationSnapshotSummary() const {
    return safeExecutionCoordinationSnapshotSummary(currentExecutionCoordinationSnapshot());
}

QString ApplicationController::localRuntimeAdapterStatus() const {
    if (!localRuntimeAdapter_) {
        return localRuntimeAdapterStatusName(LocalRuntimeAdapterStatus::Unavailable);
    }
    return localRuntimeAdapterStatusName(localRuntimeAdapter_->descriptor().status);
}

QString ApplicationController::localRuntimeAdapterHealth() const {
    if (!localRuntimeAdapter_) {
        return localRuntimeAdapterHealthName(LocalRuntimeAdapterHealth::NotExecutable);
    }
    return localRuntimeAdapterHealthName(localRuntimeAdapter_->descriptor().health);
}

QString ApplicationController::localRuntimeAdapterSummary() const {
    if (!localRuntimeAdapter_) {
        return QStringLiteral("No local runtime adapter metadata.");
    }
    return safeLocalRuntimeAdapterSummary(localRuntimeAdapter_->descriptor());
}

QStringList ApplicationController::localRuntimeAdapterCapabilitySummaries() const {
    if (!localRuntimeAdapter_) {
        return {};
    }
    return sentinel::core::localRuntimeAdapterCapabilitySummaries(
        localRuntimeAdapter_->descriptor().capabilities);
}

QString ApplicationController::providerRuntimeBridgeStatus() const {
    if (!providerRuntimeBridge_) {
        return providerRuntimeBridgeStatusName(ProviderRuntimeBridgeStatus::Unavailable);
    }
    return providerRuntimeBridgeStatusName(providerRuntimeBridge_->summary().status);
}

QString ApplicationController::providerRuntimeBridgeSummary() const {
    if (!providerRuntimeBridge_) {
        return QStringLiteral("No provider runtime bridge metadata.");
    }
    return safeProviderRuntimeBridgeSummary(providerRuntimeBridge_->summary());
}

QString ApplicationController::providerRuntimeBridgeResponseSummary() const {
    return safeProviderRuntimeBridgeResponseSummary(currentProviderRuntimeBridgeResponse());
}

QString ApplicationController::runtimeIntegrationReadinessStatus() const {
    return runtimeIntegrationReadinessName(currentRuntimeIntegrationReport().readiness);
}

QString ApplicationController::runtimeIntegrationReadinessSummary() const {
    return safeRuntimeIntegrationReportSummary(currentRuntimeIntegrationReport());
}

QStringList ApplicationController::runtimeIntegrationReadinessChecks() const {
    return sentinel::core::runtimeIntegrationCheckSummaries(
        currentRuntimeIntegrationReport().checks);
}

QString ApplicationController::ollamaEndpoint() const {
    return ollamaRuntimeClient_ ? ollamaRuntimeClient_->config().endpoint.toString()
                                : OllamaEndpoint::defaultEndpoint().toString();
}

QString ApplicationController::ollamaConnectionStatus() const {
    return ollamaConnectionStatusName(currentOllamaHealthCheck().connectionStatus);
}

QString ApplicationController::ollamaHealthStatus() const {
    return ollamaHealthStatusName(currentOllamaHealthCheck().healthStatus);
}

QString ApplicationController::ollamaHealthSummary() const {
    return safeOllamaHealthSummary(currentOllamaHealthCheck());
}

int ApplicationController::ollamaModelCount() const {
    return static_cast<int>(currentOllamaModels().size());
}

QStringList ApplicationController::ollamaModelNames() const {
    QStringList names;
    const auto models = currentOllamaModels();
    for (const auto& model : models) {
        names.append(model.name);
    }
    return names;
}

QStringList ApplicationController::ollamaModelSummaries() const {
    return sentinel::core::ollamaModelSummaries(currentOllamaModels());
}

QString ApplicationController::selectedLocalModel() const {
    return selectedLocalModel_;
}

void ApplicationController::setSelectedLocalModel(const QString& model) {
    const auto normalized = model.trimmed();
    if (normalized == selectedLocalModel_) {
        return;
    }

    selectedLocalModel_ = normalized;
    emit localModelSelectionChanged();
    emit localChatInferenceRoutingChanged();
}

QString ApplicationController::selectedLocalModelStatus() const {
    const auto models = currentOllamaModels();
    const auto selected = selectedLocalModel_.trimmed();
    if (selected.isEmpty()) {
        return models.isEmpty() ? QStringLiteral("Missing") : QStringLiteral("Fallback");
    }
    if (models.isEmpty()) {
        return QStringLiteral("Unverified");
    }
    return discoveredModelNamesContain(selected, models) ? QStringLiteral("Available")
                                                         : QStringLiteral("Invalid");
}

QString ApplicationController::selectedLocalModelSummary() const {
    const auto models = currentOllamaModels();
    const auto selected = selectedLocalModel_.trimmed();
    if (selected.isEmpty()) {
        if (!models.isEmpty()) {
            return QStringLiteral("No local model selected; fallback candidate is %1 from "
                                  "discovered Ollama metadata.")
                .arg(models.first().name);
        }
        return QStringLiteral("No local model selected and no discovered Ollama models are "
                              "available; local inference requires a selected or explicit model.");
    }

    if (!models.isEmpty() && !discoveredModelNamesContain(selected, models)) {
        return QStringLiteral("Selected local model %1 was not found in discovered local models.")
            .arg(selected);
    }

    if (models.isEmpty()) {
        return QStringLiteral("Selected local model %1 is configured; discovery metadata is "
                              "currently unavailable.")
            .arg(selected);
    }

    return QStringLiteral("Selected local model %1 is available in local discovery metadata.")
        .arg(selected);
}

QString ApplicationController::selectedLocalModelMetadataSummary() const {
    const auto models = currentOllamaModels();
    const auto selected = selectedLocalModel_.trimmed();
    const auto effective = effectiveLocalModel({});

    if (effective.isEmpty()) {
        return QStringLiteral("No local model metadata available.");
    }

    for (const auto& model : models) {
        if (model.name == effective) {
            const auto prefix =
                selected.isEmpty() ? QStringLiteral("Fallback") : QStringLiteral("Selected");
            return QStringLiteral("%1 model: %2").arg(prefix, ollamaModelSummary(model));
        }
    }

    if (!selected.isEmpty() && !models.isEmpty()) {
        return QStringLiteral("Invalid selection: %1 is not in discovered local model metadata.")
            .arg(selected);
    }

    return QStringLiteral("Selected model: %1 (Local Only, discovery metadata unavailable).")
        .arg(effective);
}

QString ApplicationController::activeLocalRuntimeBadge() const {
    const auto model = effectiveLocalModel({});
    return model.isEmpty() ? QStringLiteral("Ollama Local / No Model")
                           : QStringLiteral("Ollama Local / %1").arg(model);
}

QString ApplicationController::modelManagementStatus() const {
    return modelManagementService_ ? modelManagementStatusName(modelManagementService_->status())
                                   : modelManagementStatusName(ModelManagementStatus::Unavailable);
}

QString ApplicationController::modelManagementSummary() const {
    if (!modelManagementService_) {
        return QStringLiteral("No model management readiness metadata available.");
    }
    return modelManagementService_->statusSummary(ollamaModelCount(), effectiveLocalModel({}));
}

QString ApplicationController::modelManagementActionAvailability() const {
    if (!modelManagementService_) {
        return QStringLiteral("Model management actions are unavailable.");
    }

    const auto model =
        effectiveLocalModel({}).isEmpty() ? QStringLiteral("local model") : effectiveLocalModel({});
    const QStringList summaries{
        safeModelManagementResultSummary(modelManagementService_->evaluate(
            ModelManagementRequest{ModelManagementAction::Pull, model})),
        safeModelManagementResultSummary(modelManagementService_->evaluate(
            ModelManagementRequest{ModelManagementAction::Delete, model})),
        safeModelManagementResultSummary(modelManagementService_->evaluate(
            ModelManagementRequest{ModelManagementAction::Install, model})),
    };
    return summaries.join(QStringLiteral(" "));
}

QStringList ApplicationController::modelRecommendationSummaries() const {
    return modelManagementService_ ? sentinel::core::modelRecommendationSummaries(
                                         modelManagementService_->recommendations())
                                   : QStringList{};
}

QStringList ApplicationController::modelRequirementSummaries() const {
    return modelManagementService_ ? sentinel::core::modelRequirementSummaries(
                                         modelManagementService_->requirementSummaries())
                                   : QStringList{};
}

QString ApplicationController::voiceRuntimeMode() const {
    return voiceRuntimeModeName(VoiceRuntimeMode::Disabled);
}

bool ApplicationController::voiceEnabled() const {
    return false;
}

VoiceReadinessReport ApplicationController::currentVoiceReadinessReport() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        const PiperTextToSpeechProvider piperProvider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return buildVoiceReadinessReport(piperProvider.descriptor(),
                                         speechToTextProvider_ ? speechToTextProvider_->descriptor()
                                                               : VoiceProviderDescriptor{});
    }

    return buildVoiceReadinessReport(
        textToSpeechProvider_ ? textToSpeechProvider_->descriptor() : VoiceProviderDescriptor{},
        speechToTextProvider_ ? speechToTextProvider_->descriptor() : VoiceProviderDescriptor{});
}

QString ApplicationController::voiceReadinessStatus() const {
    return currentVoiceReadinessReport().status;
}

QString ApplicationController::voiceReadinessSummary() const {
    return currentVoiceReadinessReport().summary;
}

QStringList ApplicationController::voiceReadinessChecks() const {
    return currentVoiceReadinessReport().checks;
}

QStringList ApplicationController::voiceCapabilitySummaries() const {
    QStringList summaries;
    if (textToSpeechProvider_) {
        summaries.append(voiceProviderCapabilitySummaries(textToSpeechProvider_->descriptor()));
    }
    if (speechToTextProvider_) {
        summaries.append(voiceProviderCapabilitySummaries(speechToTextProvider_->descriptor()));
    }
    return summaries;
}

QString ApplicationController::textToSpeechStatus() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        const PiperTextToSpeechProvider piperProvider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return voiceProviderStatusName(piperProvider.descriptor().status);
    }

    return textToSpeechProvider_
               ? voiceProviderStatusName(textToSpeechProvider_->descriptor().status)
               : voiceProviderStatusName(VoiceProviderStatus::Unavailable);
}

QString ApplicationController::textToSpeechSummary() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        const PiperTextToSpeechProvider piperProvider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return piperProvider.statusSummary();
    }

    return textToSpeechProvider_ ? textToSpeechProvider_->statusSummary()
                                 : QStringLiteral("No text-to-speech provider metadata.");
}

QString ApplicationController::speechToTextStatus() const {
    return speechToTextProvider_
               ? voiceProviderStatusName(speechToTextProvider_->descriptor().status)
               : voiceProviderStatusName(VoiceProviderStatus::Unavailable);
}

QString ApplicationController::speechToTextSummary() const {
    return speechToTextProvider_ ? speechToTextProvider_->statusSummary()
                                 : QStringLiteral("No speech-to-text provider metadata.");
}

VoiceSession ApplicationController::currentVoiceSession() const {
    return voiceRuntimeCoordinator_ ? voiceRuntimeCoordinator_->currentSession() : VoiceSession{};
}

VoicePipelineResult ApplicationController::currentVoicePipelineResult() const {
    return voiceRuntimeCoordinator_
               ? voiceRuntimeCoordinator_->evaluate(VoiceSessionState::Completed)
               : VoicePipelineResult{};
}

VoiceRuntimeSummary ApplicationController::currentVoiceRuntimeSummary() const {
    return voiceRuntimeCoordinator_ ? voiceRuntimeCoordinator_->runtimeSummary()
                                    : VoiceRuntimeSummary{};
}

QString ApplicationController::voiceSessionId() const {
    return currentVoiceSession().id.value;
}

QString ApplicationController::voiceSessionStatus() const {
    return voiceSessionStateName(currentVoicePipelineResult().session.state);
}

QString ApplicationController::voiceSessionSummary() const {
    return currentVoicePipelineResult().session.summary;
}

QString ApplicationController::voicePipelineStatus() const {
    return voicePipelineStatusName(currentVoicePipelineResult().status);
}

QString ApplicationController::voicePipelineSummary() const {
    return safeVoicePipelineSummary(currentVoicePipelineResult());
}

QStringList ApplicationController::voicePipelineTraceSummaries() const {
    return sentinel::core::voicePipelineTraceSummaries(currentVoicePipelineResult().traces);
}

QString ApplicationController::voiceRuntimeStatus() const {
    return currentVoiceRuntimeSummary().status;
}

QString ApplicationController::voiceRuntimeSummary() const {
    return voiceRuntimeSummaryText(currentVoiceRuntimeSummary());
}

QStringList ApplicationController::voiceRuntimeCheckSummaries() const {
    return sentinel::core::voiceRuntimeCheckSummaries(currentVoiceRuntimeSummary());
}

bool ApplicationController::voiceRuntimeAvailable() const {
    return currentVoiceRuntimeSummary().runtimeAvailable;
}

bool ApplicationController::voiceTextToSpeechAvailable() const {
    return currentVoiceRuntimeSummary().textToSpeechAvailable;
}

bool ApplicationController::voiceSpeechToTextAvailable() const {
    return currentVoiceRuntimeSummary().speechToTextAvailable;
}

bool ApplicationController::voiceMicrophoneEnabled() const {
    return currentVoiceRuntimeSummary().microphoneEnabled;
}

bool ApplicationController::voicePlaybackEnabled() const {
    return currentVoiceRuntimeSummary().playbackEnabled;
}

bool ApplicationController::voiceLocalOnlyPolicy() const {
    return currentVoiceRuntimeSummary().localOnlyPolicy;
}

bool ApplicationController::voiceProcessExecutionEnabled() const {
    return currentVoiceRuntimeSummary().processExecutionEnabled;
}

QString ApplicationController::voiceRuntimeEnvironmentStatus() const {
    if (hasConfiguredVoicePaths(piperBinaryPath_, piperModelPath_, whisperBinaryPath_,
                                whisperModelPath_)) {
        return QStringLiteral("Configured Metadata");
    }

    return voiceRuntimeEnvironment_ ? voiceRuntimeEnvironment_->status()
                                    : QStringLiteral("Blocked");
}

QString ApplicationController::voiceRuntimeEnvironmentSummary() const {
    if (hasConfiguredVoicePaths(piperBinaryPath_, piperModelPath_, whisperBinaryPath_,
                                whisperModelPath_)) {
        return QStringLiteral("Voice configuration stores Piper and Whisper paths as metadata "
                              "only. Exact configured paths are checked for exists/readable and "
                              "binary executable state without running Piper, running Whisper, "
                              "opening audio devices, downloading models, or scanning the "
                              "filesystem.");
    }

    return voiceRuntimeEnvironment_
               ? voiceRuntimeEnvironment_->summary()
               : QStringLiteral("No voice runtime environment metadata available.");
}

QStringList ApplicationController::voiceBinarySummaries() const {
    if (hasConfiguredVoicePaths(piperBinaryPath_, piperModelPath_, whisperBinaryPath_,
                                whisperModelPath_)) {
        return voiceBinaryDescriptorSummaries({
            configuredVoiceBinaryDescriptor(
                QStringLiteral("piper-binary"), QStringLiteral("Piper Binary"),
                VoiceCapability::TextToSpeech, piperBinaryPath_, QStringLiteral("Piper")),
            configuredVoiceBinaryDescriptor(
                QStringLiteral("whisper-binary"), QStringLiteral("Whisper Binary"),
                VoiceCapability::SpeechToText, whisperBinaryPath_, QStringLiteral("Whisper")),
        });
    }

    return voiceRuntimeEnvironment_
               ? voiceBinaryDescriptorSummaries(voiceRuntimeEnvironment_->binaries())
               : QStringList{};
}

QStringList ApplicationController::voiceModelSummaries() const {
    if (hasConfiguredVoicePaths(piperBinaryPath_, piperModelPath_, whisperBinaryPath_,
                                whisperModelPath_)) {
        return voiceModelDescriptorSummaries({
            configuredVoiceModelDescriptor(
                QStringLiteral("piper-voice-model"), QStringLiteral("Piper Voice Model"),
                VoiceCapability::TextToSpeech, piperModelPath_, QStringLiteral("Piper")),
            configuredVoiceModelDescriptor(
                QStringLiteral("whisper-model"), QStringLiteral("Whisper Model"),
                VoiceCapability::SpeechToText, whisperModelPath_, QStringLiteral("Whisper")),
        });
    }

    return voiceRuntimeEnvironment_
               ? voiceModelDescriptorSummaries(voiceRuntimeEnvironment_->models())
               : QStringList{};
}

QStringList ApplicationController::voiceRuntimePermissionSummaries() const {
    return voiceRuntimeEnvironment_ ? sentinel::core::voiceRuntimePermissionSummaries(
                                          voiceRuntimeEnvironment_->permissions())
                                    : QStringList{};
}

QString ApplicationController::voiceRuntimeSafetyStatus() const {
    return voiceRuntimeEnvironment_ ? voiceRuntimeEnvironment_->safetyReport().status
                                    : QStringLiteral("Blocked");
}

QString ApplicationController::voiceRuntimeSafetySummary() const {
    return voiceRuntimeEnvironment_
               ? voiceRuntimeSafetySummaryText(voiceRuntimeEnvironment_->safetyReport())
               : QStringLiteral("Voice runtime safety metadata is unavailable.");
}

QStringList ApplicationController::voiceRuntimeSafetyChecks() const {
    return voiceRuntimeEnvironment_
               ? voiceRuntimeSafetyCheckSummaries(voiceRuntimeEnvironment_->safetyReport())
               : QStringList{};
}

bool ApplicationController::voiceRuntimeExecutionAllowed() const {
    return voiceRuntimeEnvironment_ && voiceRuntimeEnvironment_->safetyReport().executionAllowed;
}

QString ApplicationController::piperTtsStatus() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return piperTtsStatusName(provider.status());
    }

    return piperTextToSpeechProvider_ ? piperTtsStatusName(piperTextToSpeechProvider_->status())
                                      : piperTtsStatusName(PiperTtsStatus::Disabled);
}

QString ApplicationController::piperTtsSummary() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return provider.piperStatusSummary();
    }

    return piperTextToSpeechProvider_ ? piperTextToSpeechProvider_->piperStatusSummary()
                                      : QStringLiteral("No Piper TTS provider metadata available.");
}

QStringList ApplicationController::piperTtsReadinessChecks() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return provider.readinessChecks();
    }

    return piperTextToSpeechProvider_ ? piperTextToSpeechProvider_->readinessChecks()
                                      : QStringList{};
}

bool ApplicationController::piperTtsReady() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return provider.status() == PiperTtsStatus::Configured;
    }

    return piperTextToSpeechProvider_ &&
           piperTextToSpeechProvider_->status() == PiperTtsStatus::Configured;
}

QString ApplicationController::piperTtsFileOutputStatus() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return provider.fileOutputStatus();
    }

    return piperTextToSpeechProvider_ ? piperTextToSpeechProvider_->fileOutputStatus()
                                      : piperTtsStatusName(PiperTtsStatus::Disabled);
}

QString ApplicationController::piperTtsFileOutputSummary() const {
    if (hasConfiguredVoicePath(piperBinaryPath_) || hasConfiguredVoicePath(piperModelPath_)) {
        PiperTextToSpeechProvider provider{
            configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_),
            std::make_unique<NullPiperTtsClient>()};
        return provider.fileOutputSummary();
    }

    return piperTextToSpeechProvider_
               ? piperTextToSpeechProvider_->fileOutputSummary()
               : QStringLiteral("No Piper TTS file-output metadata available.");
}

QString ApplicationController::piperBinaryPath() const {
    return piperBinaryPath_;
}

void ApplicationController::setPiperBinaryPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == piperBinaryPath_) {
        return;
    }

    piperBinaryPath_ = normalized;
    emit voiceConfigurationChanged();
}

QString ApplicationController::piperModelPath() const {
    return piperModelPath_;
}

void ApplicationController::setPiperModelPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == piperModelPath_) {
        return;
    }

    piperModelPath_ = normalized;
    emit voiceConfigurationChanged();
}

QString ApplicationController::whisperBinaryPath() const {
    return whisperBinaryPath_;
}

void ApplicationController::setWhisperBinaryPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == whisperBinaryPath_) {
        return;
    }

    whisperBinaryPath_ = normalized;
    emit voiceConfigurationChanged();
}

QString ApplicationController::whisperModelPath() const {
    return whisperModelPath_;
}

void ApplicationController::setWhisperModelPath(const QString& path) {
    const auto normalized = path.trimmed();
    if (normalized == whisperModelPath_) {
        return;
    }

    whisperModelPath_ = normalized;
    emit voiceConfigurationChanged();
}

QStringList ApplicationController::voiceConfigurationSummaries() const {
    return voiceBinarySummaries() + voiceModelSummaries();
}

QString ApplicationController::voiceConfigurationReadinessSummary() const {
    return QStringLiteral("Piper file-output TTS: %1. Whisper STT preparation: %2. "
                          "Configuration is metadata-only; Piper and Whisper are not executed.")
        .arg(piperFileOutputReadinessStatus(), whisperPreparationReadinessStatus());
}

QStringList ApplicationController::voiceConfigurationStatusBadges() const {
    const auto piperBinary =
        validateVoicePath(QStringLiteral("Piper binary"), piperBinaryPath_, true, true);
    const auto piperModel =
        validateVoicePath(QStringLiteral("Piper model"), piperModelPath_, false, true);
    const auto whisperBinary =
        validateVoicePath(QStringLiteral("Whisper binary"), whisperBinaryPath_, true, true);
    const auto whisperModel =
        validateVoicePath(QStringLiteral("Whisper model"), whisperModelPath_, false, false);
    return {
        QStringLiteral("Piper file-output TTS: %1").arg(piperFileOutputReadinessStatus()),
        voicePathStatusBadge(piperBinary),
        voicePathStatusBadge(piperModel),
        QStringLiteral("Whisper STT preparation: %1").arg(whisperPreparationReadinessStatus()),
        voicePathStatusBadge(whisperBinary),
        voicePathStatusBadge(whisperModel),
    };
}

QStringList ApplicationController::voiceConfigurationHintSummaries() const {
    const QStringList piperBinaryKnownPaths{
        QStringLiteral("/opt/homebrew/bin/piper"),
        QStringLiteral("/usr/local/bin/piper"),
    };
    const QStringList whisperBinaryKnownPaths{
        QStringLiteral("/opt/homebrew/bin/whisper"),
        QStringLiteral("/usr/local/bin/whisper"),
    };

    return {
        binaryHintSummary(QStringLiteral("Piper binary"), piperBinaryPath_, piperBinaryKnownPaths),
        modelHintSummary(QStringLiteral("Piper .onnx model"), piperModelPath_,
                         QStringLiteral("set a local .onnx model path to validate it.")),
        binaryHintSummary(QStringLiteral("Whisper binary"), whisperBinaryPath_,
                          whisperBinaryKnownPaths),
        modelHintSummary(QStringLiteral("Whisper model"), whisperModelPath_,
                         QStringLiteral("set a model folder or model file path to validate it.")),
        QStringLiteral("Hints are read-only. Apply Paths stores only the entered paths; no "
                       "Piper, Whisper, microphone, playback, download, or scan is started."),
    };
}

QStringList ApplicationController::voiceConfigurationValidationSummaries() const {
    return {
        voicePathValidationSummary(
            validateVoicePath(QStringLiteral("Piper binary"), piperBinaryPath_, true, true)),
        voicePathValidationSummary(
            validateVoicePath(QStringLiteral("Piper .onnx model"), piperModelPath_, false, true)),
        voicePathValidationSummary(
            validateVoicePath(QStringLiteral("Whisper binary"), whisperBinaryPath_, true, true)),
        voicePathValidationSummary(validateVoicePath(QStringLiteral("Whisper model folder or file"),
                                                     whisperModelPath_, false, false)),
    };
}

QString ApplicationController::piperFileOutputReadinessStatus() const {
    const auto readiness = combinedReadiness({
        validateVoicePath(QStringLiteral("Piper binary"), piperBinaryPath_, true, true),
        validateVoicePath(QStringLiteral("Piper .onnx model"), piperModelPath_, false, true),
    });
    return voicePathReadinessName(readiness);
}

QString ApplicationController::piperFileOutputReadinessSummary() const {
    const QList<VoicePathValidation> validations{
        validateVoicePath(QStringLiteral("Piper binary"), piperBinaryPath_, true, true),
        validateVoicePath(QStringLiteral("Piper .onnx model"), piperModelPath_, false, true),
    };
    const auto readiness = combinedReadiness(validations);
    if (readiness == VoicePathReadiness::Ready) {
        return QStringLiteral("Ready for a later controlled file-output TTS phase: Piper binary "
                              "is executable and the .onnx model path is readable. Piper is not "
                              "executed in this phase.");
    }
    if (readiness == VoicePathReadiness::Missing) {
        return QStringLiteral("Missing Piper paths: set a Piper binary path and a Piper .onnx "
                              "model path. Piper is not executed.");
    }
    return QStringLiteral("Blocked Piper file-output TTS preparation: %1. Piper is not executed.")
        .arg(combinedBlockedReasons(validations));
}

bool ApplicationController::piperFileOutputExecutionEnabled() const {
    return piperFileOutputExecutionEnabled_;
}

void ApplicationController::setPiperFileOutputExecutionEnabled(bool enabled) {
    if (enabled == piperFileOutputExecutionEnabled_) {
        return;
    }

    piperFileOutputExecutionEnabled_ = enabled;
    if (!enabled && latestPiperTtsResult_.status == PiperTtsStatus::Running) {
        latestPiperTtsResult_ = PiperTtsResult{
            PiperTtsStatus::Disabled,
            false,
            {},
            {},
            0,
            -1,
            {},
            QStringLiteral("Piper execution disabled."),
            {QStringLiteral("Piper execution opt-in was disabled.")},
        };
    }
    emit voiceConfigurationChanged();
}

QString ApplicationController::piperFileOutputExecutionStatus() const {
    if (!piperFileOutputExecutionEnabled_) {
        return QStringLiteral("Disabled");
    }
    if (latestPiperTtsResult_.status == PiperTtsStatus::Disabled &&
        latestPiperTtsResult_.summary.trimmed().isEmpty()) {
        return piperFileOutputReadinessStatus() == QStringLiteral("Ready")
                   ? QStringLiteral("Ready")
                   : piperFileOutputReadinessStatus();
    }
    return piperTtsStatusName(latestPiperTtsResult_.status);
}

QString ApplicationController::piperFileOutputExecutionSummary() const {
    if (!piperFileOutputExecutionEnabled_) {
        return QStringLiteral("Piper execution disabled. Enable controlled file-output execution "
                              "and use Generate TTS File to write a local file.");
    }
    if (!latestPiperTtsResult_.summary.trimmed().isEmpty()) {
        return safePiperTtsResultSummary(latestPiperTtsResult_);
    }
    if (piperFileOutputReadinessStatus() != QStringLiteral("Ready")) {
        return piperFileOutputReadinessSummary();
    }
    return QStringLiteral("Piper execution is enabled for explicit controlled file output only. "
                          "No playback or microphone access is available.");
}

QString ApplicationController::piperFileOutputAudioPathSummary() const {
    if (latestPiperTtsResult_.audioPath.trimmed().isEmpty()) {
        return QStringLiteral("No generated Piper audio file.");
    }
    return latestPiperTtsResult_.outputPathSummary.trimmed().isEmpty()
               ? QStringLiteral("Controlled Piper TTS output path: %1")
                     .arg(latestPiperTtsResult_.audioPath)
               : latestPiperTtsResult_.outputPathSummary;
}

bool ApplicationController::generatePiperTtsFile(const QString& text) {
    if (!piperFileOutputExecutionEnabled_) {
        latestPiperTtsResult_ = PiperTtsResult{
            PiperTtsStatus::Disabled,
            false,
            {},
            {},
            0,
            -1,
            {},
            QStringLiteral("Piper execution disabled."),
            {QStringLiteral("Piper file-output generation refused because opt-in is disabled.")},
        };
        emit voiceConfigurationChanged();
        return false;
    }

    auto config = configuredPiperTtsConfig(piperBinaryPath_, piperModelPath_, true);
    piperTextToSpeechProvider_->setConfig(std::move(config));
    latestPiperTtsResult_ = PiperTtsResult{
        PiperTtsStatus::Running,
        false,
        {},
        {},
        piperTextToSpeechProvider_->config().timeoutMs,
        -1,
        {},
        QStringLiteral("Piper TTS file output is running."),
        {QStringLiteral("Explicit user action requested controlled Piper file output.")},
    };
    emit voiceConfigurationChanged();

    latestPiperTtsResult_ = piperTextToSpeechProvider_->synthesizePiper(PiperTtsRequest{
        text,
        {},
        {},
        true,
        true,
        false,
        piperTextToSpeechProvider_->config().timeoutMs,
    });
    emit voiceConfigurationChanged();
    return latestPiperTtsResult_.success;
}

QString ApplicationController::whisperPreparationReadinessStatus() const {
    const auto readiness = combinedReadiness({
        validateVoicePath(QStringLiteral("Whisper binary"), whisperBinaryPath_, true, true),
        validateVoicePath(QStringLiteral("Whisper model folder or file"), whisperModelPath_, false,
                          false),
    });
    return voicePathReadinessName(readiness);
}

QString ApplicationController::whisperPreparationReadinessSummary() const {
    const QList<VoicePathValidation> validations{
        validateVoicePath(QStringLiteral("Whisper binary"), whisperBinaryPath_, true, true),
        validateVoicePath(QStringLiteral("Whisper model folder or file"), whisperModelPath_, false,
                          false),
    };
    const auto readiness = combinedReadiness(validations);
    if (readiness == VoicePathReadiness::Ready) {
        return QStringLiteral("Ready for later STT preparation: Whisper binary is executable and "
                              "the model folder or file is readable. Whisper is not executed.");
    }
    if (readiness == VoicePathReadiness::Missing) {
        return QStringLiteral("Missing Whisper paths: set a Whisper binary path and a model "
                              "folder or model file path. Whisper is not executed.");
    }
    return QStringLiteral("Blocked Whisper STT preparation: %1. Whisper is not executed.")
        .arg(combinedBlockedReasons(validations));
}

bool ApplicationController::localChatInferenceEnabled() const {
    return localChatInferenceEnabled_;
}

void ApplicationController::setLocalChatInferenceEnabled(bool enabled) {
    if (enabled == localChatInferenceEnabled_) {
        return;
    }

    localChatInferenceEnabled_ = enabled;
    emit localChatInferenceRoutingChanged();
}

QString ApplicationController::localChatInferenceStatus() const {
    if (!localChatInferenceEnabled_) {
        return QStringLiteral("Disabled");
    }
    if (!localInferenceEndpointAllowed()) {
        return QStringLiteral("Blocked");
    }
    if (effectiveLocalModel({}).isEmpty()) {
        return QStringLiteral("Missing Model");
    }
    const auto models = currentOllamaModels();
    const auto model = effectiveLocalModel({});
    if (!models.isEmpty() && !discoveredModelNamesContain(model, models)) {
        return QStringLiteral("Invalid Model");
    }
    return QStringLiteral("Enabled");
}

QString ApplicationController::localChatInferenceSummary() const {
    if (!localChatInferenceEnabled_) {
        return QStringLiteral("Local chat inference is disabled; chat stays on the local safe "
                              "provider path and no Ollama prompt is sent.");
    }
    if (!localInferenceEndpointAllowed()) {
        return QStringLiteral("Local chat inference is blocked: Ollama endpoint must be local "
                              "loopback HTTP.");
    }
    if (effectiveLocalModel({}).isEmpty()) {
        return QStringLiteral("Local chat inference is enabled but no local model is selected or "
                              "available from discovery metadata.");
    }
    if (localChatInferenceStatus() == QStringLiteral("Invalid Model")) {
        return QStringLiteral("Local chat inference is enabled but the selected model is missing "
                              "from discovered Ollama metadata.");
    }
    return QStringLiteral("Local chat inference is enabled for guarded local-only Ollama routing.");
}

bool ApplicationController::localInferenceStreamingEnabled() const {
    return localInferenceStreamingEnabled_;
}

void ApplicationController::setLocalInferenceStreamingEnabled(bool enabled) {
    if (enabled == localInferenceStreamingEnabled_) {
        return;
    }

    localInferenceStreamingEnabled_ = enabled;
    if (!enabled) {
        latestLocalInferenceStreamResult_ = LocalInferenceStreamResult{};
    }
    emit localInferenceChanged();
}

bool ApplicationController::localInferenceBusy() const {
    return localInferenceBusy_;
}

QString ApplicationController::localInferenceRuntimeState() const {
    if (localInferenceBusy_) {
        return latestLocalInferenceStreamResult_.status == LocalInferenceStreamStatus::Streaming
                   ? QStringLiteral("Streaming")
                   : QStringLiteral("Inferencing");
    }
    if (latestLocalInferenceResponse_.status == LocalInferenceStatus::Error ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::Refused ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::Blocked ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::InvalidRequest ||
        latestLocalInferenceResponse_.status == LocalInferenceStatus::ModelUnavailable) {
        return QStringLiteral("Failed");
    }
    if (latestLocalInferenceResponse_.status == LocalInferenceStatus::NotRequested &&
        currentOllamaHealthCheck().healthStatus != OllamaHealthStatus::Healthy) {
        return QStringLiteral("Unavailable");
    }
    return QStringLiteral("Idle");
}

QString ApplicationController::localInferenceStatus() const {
    return localInferenceBusy_ ? localInferenceStatusName(LocalInferenceStatus::Busy)
                               : localInferenceStatusName(latestLocalInferenceResponse_.status);
}

QString ApplicationController::localInferenceSummary() const {
    if (latestLocalInferenceResponse_.status == LocalInferenceStatus::NotRequested &&
        localInferenceWorker_) {
        return localInferenceWorker_->statusSummary();
    }

    return safeLocalInferenceSummary(latestLocalInferenceResponse_);
}

QString ApplicationController::localInferenceLastResponseSummary() const {
    return safeLocalInferenceResponseSummary(latestLocalInferenceResponse_);
}

QString ApplicationController::localInferenceLatencySummary() const {
    return latestLocalInferenceResponse_.latencyMs >= 0
               ? QStringLiteral("Last local inference latency: %1 ms.")
                     .arg(latestLocalInferenceResponse_.latencyMs)
               : QStringLiteral("No local inference latency recorded.");
}

QStringList ApplicationController::localInferenceTraceSummaries() const {
    return sentinel::core::localInferenceTraceSummaries(latestLocalInferenceResponse_.traces);
}

bool ApplicationController::localInferenceStreamingAvailable() const {
    return localInferenceStreamingEnabled_ && localInferenceWorker_ &&
           localInferenceWorker_->streamingAvailable();
}

QString ApplicationController::localInferenceStreamStatus() const {
    return localInferenceStreamStatusName(latestLocalInferenceStreamResult_.status);
}

QString ApplicationController::localInferenceStreamSummary() const {
    if (!localInferenceStreamingEnabled_) {
        return QStringLiteral("Local inference streaming is disabled; responses finalize through "
                              "normal chat history.");
    }
    if (!localInferenceWorker_) {
        return QStringLiteral("Local inference streaming client is unavailable.");
    }
    if (localInferenceBusy_ &&
        latestLocalInferenceStreamResult_.status == LocalInferenceStreamStatus::Streaming) {
        return latestLocalInferenceStreamResult_.summary.isEmpty()
                   ? QStringLiteral("Local streaming response is active.")
                   : latestLocalInferenceStreamResult_.summary;
    }
    if (latestLocalInferenceStreamResult_.status == LocalInferenceStreamStatus::Disabled) {
        return localInferenceWorker_->streamStatusSummary();
    }
    return latestLocalInferenceStreamResult_.summary;
}

QString ApplicationController::localInferenceStreamingText() const {
    if (!localInferenceBusy_ ||
        latestLocalInferenceStreamResult_.status != LocalInferenceStreamStatus::Streaming) {
        return {};
    }
    return latestLocalInferenceStreamResult_.accumulatedText;
}

int ApplicationController::availableToolCount() const {
    return agentRuntime_ ? static_cast<int>(agentRuntime_->availableTools().size()) : 0;
}

QStringList ApplicationController::availableToolIds() const {
    QStringList ids;
    if (!agentRuntime_) {
        return ids;
    }

    for (const auto& tool : agentRuntime_->availableTools()) {
        ids.append(tool.id);
    }
    return ids;
}

QString ApplicationController::memoryStatus() const {
    return memoryStore_ && memoryStore_->isAvailable() ? QStringLiteral("Available")
                                                       : QStringLiteral("Unavailable");
}

QString ApplicationController::chatHistoryStatus() const {
    return chatHistoryStore_ && chatHistoryStore_->isAvailable() ? QStringLiteral("Available")
                                                                 : QStringLiteral("Runtime Only");
}

QList<ConversationRecord> ApplicationController::conversationRecords() const {
    return conversationStore_ ? conversationStore_->listConversations()
                              : QList<ConversationRecord>{};
}

ConversationRecord ApplicationController::activeConversationRecord() const {
    for (const auto& record : conversationRecords()) {
        if (record.id == activeConversationId_) {
            return record;
        }
    }
    return {};
}

ConversationMessageRecord
ApplicationController::conversationMessageRecordFromChatMessage(const ChatMessage& message) const {
    return ConversationMessageRecord{
        activeConversationId_, message.id,        message.role,
        message.content,       message.timestamp, message.status,
    };
}

ChatMessage ApplicationController::chatMessageFromConversationMessageRecord(
    const ConversationMessageRecord& message) const {
    return ChatMessage{
        message.messageId, message.role, message.content, message.timestampUtc, message.status,
    };
}

bool ApplicationController::ensureActiveConversation() {
    if (!conversationStore_ || conversationStore_->status() != ConversationStoreStatus::Ready) {
        activeConversationId_.clear();
        return false;
    }

    const auto records = conversationStore_->listConversations();
    for (const auto& record : records) {
        if (record.id == activeConversationId_ && !record.deleted) {
            return true;
        }
    }
    for (const auto& record : records) {
        if (!record.archived && !record.deleted) {
            activeConversationId_ = record.id;
            return true;
        }
    }

    const auto record =
        conversationStore_->createConversation(QStringLiteral("Current Transcript"));
    activeConversationId_ = record.id;
    return !activeConversationId_.isEmpty();
}

void ApplicationController::initializeActiveConversation() {
    if (!ensureActiveConversation()) {
        return;
    }

    const auto storedMessages = conversationStore_->loadMessages(activeConversationId_);
    if (!storedMessages.isEmpty()) {
        QList<ChatMessage> messages;
        messages.reserve(storedMessages.size());
        for (const auto& message : storedMessages) {
            messages.append(chatMessageFromConversationMessageRecord(message));
        }
        chatSession_->loadMessages(messages);
        conversationHistorySummary_.lastRestoredStatus =
            QStringLiteral("Restored %1 active conversation messages.").arg(storedMessages.size());
        conversationHistorySummary_.lastSavedStatus = QStringLiteral("Active conversation loaded.");
        return;
    }

    for (const auto& message : chatSession_->messages()) {
        persistActiveConversationMessage(message);
    }
}

void ApplicationController::loadActiveConversationTranscript() {
    if (!ensureActiveConversation()) {
        return;
    }

    const auto storedMessages = conversationStore_->loadMessages(activeConversationId_);
    QList<ChatMessage> messages;
    messages.reserve(storedMessages.size());
    for (const auto& message : storedMessages) {
        messages.append(chatMessageFromConversationMessageRecord(message));
    }
    chatSession_->loadMessages(messages);
    if (chatSession_->messages().isEmpty()) {
        const auto systemMessage = chatSession_->appendSystemMessage(
            QStringLiteral("Sentinel Core online."), ChatMessageStatus::Received);
        persistActiveConversationMessage(systemMessage);
    }
    conversationHistorySummary_.lastRestoredStatus =
        QStringLiteral("Loaded active conversation transcript.");
}

bool ApplicationController::persistActiveConversationMessage(const ChatMessage& message) {
    if (!ensureActiveConversation()) {
        return false;
    }
    return conversationStore_->appendMessage(conversationMessageRecordFromChatMessage(message));
}

QString ApplicationController::conversationStoreStatus() const {
    return conversationStore_ ? conversationStoreStatusName(conversationStore_->status())
                              : QStringLiteral("Unavailable");
}

int ApplicationController::conversationStoreConversationCount() const {
    return conversationStore_ ? conversationStore_->listConversations().size() : 0;
}

QString ApplicationController::activeConversationSummary() const {
    if (!conversationStore_) {
        return QStringLiteral("Current Transcript / %1 %2 / Conversation store unavailable")
            .arg(conversationHistorySummary_.messageCount)
            .arg(conversationHistorySummary_.messageCount == 1 ? QStringLiteral("message")
                                                               : QStringLiteral("messages"));
    }

    const auto records = conversationRecords();
    if (records.isEmpty()) {
        return QStringLiteral("Current Transcript / %1 %2 / Single Transcript active")
            .arg(conversationHistorySummary_.messageCount)
            .arg(conversationHistorySummary_.messageCount == 1 ? QStringLiteral("message")
                                                               : QStringLiteral("messages"));
    }

    const auto active = activeConversationRecord();
    return active.id.isEmpty() ? conversationRecordSummary(records.first())
                               : conversationRecordSummary(active);
}

QStringList ApplicationController::conversationStoreSummaries() const {
    QStringList summaries;
    if (!conversationStore_) {
        return summaries;
    }

    const auto records = conversationRecords();
    summaries.reserve(records.size());
    for (const auto& record : records) {
        summaries.append(conversationRecordSummary(record));
    }
    return summaries;
}

QString ApplicationController::activeConversationId() const {
    return activeConversationId_.isEmpty() ? QStringLiteral("single-transcript")
                                           : activeConversationId_;
}

bool ApplicationController::activeConversationArchived() const {
    return activeConversationRecord().archived;
}

QString ApplicationController::activeConversationStateSummary() const {
    const auto active = activeConversationRecord();
    if (active.id.isEmpty()) {
        return QStringLiteral("No active conversation metadata is available.");
    }
    if (active.archived) {
        return QStringLiteral("Archived conversation selected; sending is disabled until it is "
                              "unarchived.");
    }
    return QStringLiteral("Active conversation selected; sending is available.");
}

QStringList ApplicationController::conversationIds() const {
    QStringList ids;
    for (const auto& record : conversationRecords()) {
        ids.append(record.id);
    }
    return ids;
}

QStringList ApplicationController::conversationTitles() const {
    QStringList titles;
    for (const auto& record : conversationRecords()) {
        titles.append(record.title);
    }
    return titles;
}

QStringList ApplicationController::conversationActiveSummaries() const {
    QStringList summaries;
    for (const auto& record : conversationRecords()) {
        summaries.append(record.id == activeConversationId_ ? QStringLiteral("Current")
                                                            : QStringLiteral("Inactive"));
    }
    return summaries;
}

QStringList ApplicationController::conversationLastUpdatedSummaries() const {
    QStringList summaries;
    for (const auto& record : conversationRecords()) {
        summaries.append(record.updatedAtUtc.isValid()
                             ? QStringLiteral("Updated %1 UTC")
                                   .arg(record.updatedAtUtc.toUTC().toString(Qt::ISODateWithMs))
                             : QStringLiteral("Updated time unavailable"));
    }
    return summaries;
}

QStringList ApplicationController::conversationMessageCountSummaries() const {
    QStringList summaries;
    for (const auto& record : conversationRecords()) {
        summaries.append(QStringLiteral("%1 %2")
                             .arg(record.messageCount)
                             .arg(record.messageCount == 1 ? QStringLiteral("message")
                                                           : QStringLiteral("messages")));
    }
    return summaries;
}

QStringList ApplicationController::conversationArchivedSummaries() const {
    QStringList summaries;
    for (const auto& record : conversationRecords()) {
        summaries.append(record.archived ? QStringLiteral("Archived") : QStringLiteral("Active"));
    }
    return summaries;
}

int ApplicationController::activeConversationCount() const {
    int count = 0;
    for (const auto& record : conversationRecords()) {
        if (!record.archived && !record.deleted) {
            ++count;
        }
    }
    return count;
}

int ApplicationController::archivedConversationCount() const {
    int count = 0;
    for (const auto& record : conversationRecords()) {
        if (record.archived && !record.deleted) {
            ++count;
        }
    }
    return count;
}

int ApplicationController::userCreatedConversationCount() const {
    int count = 0;
    for (const auto& record : conversationRecords()) {
        const bool initialTranscript =
            record.title == QStringLiteral("Current Transcript") && record.messageCount <= 1;
        if (!initialTranscript && !record.deleted) {
            ++count;
        }
    }
    return count;
}

bool ApplicationController::conversationBrowserEmptyStateVisible() const {
    return userCreatedConversationCount() == 0;
}

QString ApplicationController::conversationBrowserEmptyStateSummary() const {
    if (conversationBrowserEmptyStateVisible()) {
        return QStringLiteral("No user-created conversations yet. Start with New or send a "
                              "message in the current transcript.");
    }
    return QStringLiteral("%1 user-created %2 available.")
        .arg(userCreatedConversationCount())
        .arg(userCreatedConversationCount() == 1 ? QStringLiteral("conversation")
                                                 : QStringLiteral("conversations"));
}

ConversationHistorySummary ApplicationController::conversationHistorySummary() const {
    return conversationHistorySummary_;
}

QString ApplicationController::conversationHistorySummaryText() const {
    return conversationHistorySummary_.summary;
}

QStringList ApplicationController::conversationHistorySummaryLines() const {
    return {
        QStringLiteral("Persistence: %1").arg(conversationPersistenceStatus()),
        QStringLiteral("Messages: %1 total, %2 user, %3 assistant, %4 system")
            .arg(conversationHistorySummary_.messageCount)
            .arg(conversationHistorySummary_.userMessageCount)
            .arg(conversationHistorySummary_.assistantMessageCount)
            .arg(conversationHistorySummary_.systemMessageCount),
        QStringLiteral("Saved: %1").arg(conversationHistorySummary_.lastSavedStatus),
        QStringLiteral("Restored: %1").arg(conversationHistorySummary_.lastRestoredStatus),
    };
}

int ApplicationController::conversationHistoryMessageCount() const {
    return conversationHistorySummary_.messageCount;
}

QString ApplicationController::conversationPersistenceStatus() const {
    return conversationPersistenceStatusName(conversationHistorySummary_.persistenceStatus);
}

QString ApplicationController::conversationLastSavedStatus() const {
    return conversationHistorySummary_.lastSavedStatus;
}

QString ApplicationController::conversationLastRestoredStatus() const {
    return conversationHistorySummary_.lastRestoredStatus;
}

ConversationId ApplicationController::currentConversationId() const {
    return ConversationId{activeConversationId()};
}

ConversationDescriptor ApplicationController::currentConversationDescriptor() const {
    ConversationDescriptor descriptor;
    descriptor.id = currentConversationId();
    const auto active = activeConversationRecord();
    descriptor.displayTitle.text =
        active.id.isEmpty() ? QStringLiteral("Current Transcript") : active.title;
    descriptor.displayTitle.summary =
        QStringLiteral("%1 / %2 %3")
            .arg(conversationPersistenceStatus())
            .arg(conversationHistorySummary_.messageCount)
            .arg(conversationHistorySummary_.messageCount == 1 ? QStringLiteral("message")
                                                               : QStringLiteral("messages"));
    descriptor.lifecycleStatus = active.archived ? ConversationLifecycleStatus::Archived
                                                 : ConversationLifecycleStatus::Active;
    descriptor.storageMode = active.id.isEmpty() ? ConversationStorageMode::SingleTranscript
                                                 : ConversationStorageMode::MultiConversation;
    descriptor.summary =
        QStringLiteral("%1 (%2)").arg(descriptor.displayTitle.summary,
                                      conversationLifecycleStatusName(descriptor.lifecycleStatus));
    return descriptor;
}

ConversationDisplayTitle ApplicationController::currentConversationDisplayTitle() const {
    return currentConversationDescriptor().displayTitle;
}

ConversationListEntry ApplicationController::currentConversationListEntry() const {
    ConversationListEntry entry;
    const auto descriptor = currentConversationDescriptor();
    entry.displayTitle = descriptor.displayTitle;
    entry.messageCount = conversationHistorySummary_.messageCount;
    entry.persistenceStatus = conversationHistorySummary_.persistenceStatus;
    entry.lastUpdatedSummary = conversationHistorySummary_.lastSavedStatus;
    entry.searchAvailabilitySummary =
        conversationSearchAvailabilitySummary(latestConversationSearchSummary_);
    entry.exportAvailabilitySummary = conversationExportAvailabilitySummary(
        conversationExportReadiness_, latestConversationExportResult_);
    entry.summary = QStringLiteral("%1 / %2 / %3")
                        .arg(entry.displayTitle.summary, entry.searchAvailabilitySummary,
                             entry.exportAvailabilitySummary);
    return entry;
}

ConversationListSummary ApplicationController::conversationListSummary() const {
    ConversationListSummary summary;
    summary.status = conversationBrowserStatusFor(conversationHistorySummary_);
    summary.entryCount = 1;
    summary.entries = {currentConversationListEntry()};
    summary.summary = QStringLiteral("%1 browser with %2 current transcript entry.")
                          .arg(conversationBrowserStatusName(summary.status),
                               QString::number(summary.entryCount));
    return summary;
}

QString ApplicationController::conversationBrowserStatus() const {
    return conversationBrowserStatusName(conversationListSummary().status);
}

QString ApplicationController::conversationBrowserSummaryText() const {
    return conversationListSummary().summary;
}

int ApplicationController::conversationListEntryCount() const {
    return conversationListSummary().entryCount;
}

QString ApplicationController::conversationListCurrentTitle() const {
    return currentConversationListEntry().displayTitle.text;
}

int ApplicationController::conversationListCurrentMessageCount() const {
    return currentConversationListEntry().messageCount;
}

QString ApplicationController::conversationListCurrentPersistenceStatus() const {
    return conversationPersistenceStatusName(currentConversationListEntry().persistenceStatus);
}

QString ApplicationController::conversationListCurrentLastUpdatedSummary() const {
    return currentConversationListEntry().lastUpdatedSummary;
}

QString ApplicationController::conversationListCurrentSearchAvailabilitySummary() const {
    return currentConversationListEntry().searchAvailabilitySummary;
}

QString ApplicationController::conversationListCurrentExportAvailabilitySummary() const {
    return currentConversationListEntry().exportAvailabilitySummary;
}

QString ApplicationController::conversationListCurrentSummary() const {
    return currentConversationListEntry().summary;
}

ConversationSchemaPlan ApplicationController::conversationSchemaPlan() const {
    return {};
}

QString ApplicationController::conversationCurrentStorageMode() const {
    return conversationStorageModeName(conversationSchemaPlan().currentStorageMode);
}

QString ApplicationController::conversationFutureStorageMode() const {
    return conversationStorageModeName(conversationSchemaPlan().futureStorageMode);
}

QString ApplicationController::conversationMigrationReadiness() const {
    return conversationMigrationReadinessName(conversationSchemaPlan().migrationReadiness);
}

QString ApplicationController::conversationMigrationStatusSummary() const {
    return conversationSchemaPlan().summary;
}

QString ApplicationController::conversationSchemaStatusSummary() const {
    return conversationSchemaPlan().schemaStatusSummary;
}

ConversationSearchSummary ApplicationController::conversationSearchSummary() const {
    return latestConversationSearchSummary_;
}

QString ApplicationController::conversationSearchQueryText() const {
    return latestConversationSearchSummary_.query.text;
}

QString ApplicationController::conversationSearchStatus() const {
    return conversationSearchStatusName(latestConversationSearchSummary_.status);
}

QString ApplicationController::conversationSearchSummaryText() const {
    return latestConversationSearchSummary_.summary;
}

int ApplicationController::conversationSearchResultCount() const {
    return latestConversationSearchSummary_.resultCount;
}

QStringList ApplicationController::conversationSearchResultSummaries() const {
    QStringList summaries;
    for (const auto& result : latestConversationSearchSummary_.results) {
        summaries.append(conversationSearchResultSummary(result));
    }
    return summaries;
}

ConversationExportReadiness ApplicationController::conversationExportReadiness() const {
    return conversationExportReadiness_;
}

ConversationExportResult ApplicationController::latestConversationExportResult() const {
    return latestConversationExportResult_;
}

bool ApplicationController::conversationExportAvailable() const {
    return conversationExportReadiness_.available;
}

QString ApplicationController::conversationExportReadinessStatus() const {
    return conversationExportReadiness_.status;
}

QString ApplicationController::conversationExportReadinessSummary() const {
    return conversationExportReadiness_.summary;
}

QStringList ApplicationController::conversationExportReadinessChecks() const {
    return conversationExportReadiness_.checks;
}

QString ApplicationController::conversationExportLastResultSummary() const {
    return latestConversationExportResult_.summary;
}

QString ApplicationController::conversationExportLastStatus() const {
    return latestConversationExportResult_.status;
}

QString ApplicationController::conversationExportLastFileName() const {
    return latestConversationExportResult_.outputFileName.isEmpty()
               ? QStringLiteral("No export file")
               : latestConversationExportResult_.outputFileName;
}

int ApplicationController::conversationExportLastMessageCount() const {
    return latestConversationExportResult_.messageCount;
}

QString ApplicationController::conversationExportLastTimestamp() const {
    return latestConversationExportResult_.exportedAtUtc.isEmpty()
               ? QStringLiteral("No export timestamp")
               : latestConversationExportResult_.exportedAtUtc;
}

ConversationDeletePolicy ApplicationController::conversationDeletePolicy() const {
    return conversationDeletePolicy_;
}

ConversationDeleteReadiness ApplicationController::conversationDeleteReadiness() const {
    ConversationDeleteReadiness readiness;
    readiness.policy = conversationDeletePolicy_;
    if (!conversationStore_ || conversationStore_->status() != ConversationStoreStatus::Ready) {
        readiness.summary =
            QStringLiteral("Permanent delete is disabled and the conversation store is "
                           "unavailable.");
        readiness.checks.append(QStringLiteral("Store: Unavailable"));
    } else {
        readiness.checks.append(QStringLiteral("Store: Ready for archive/unarchive only"));
    }
    return readiness;
}

ConversationDeleteResult ApplicationController::latestConversationDeleteResult() const {
    return latestConversationDeleteResult_;
}

bool ApplicationController::conversationDeleteAvailable() const {
    return conversationDeleteReadiness().available;
}

QString ApplicationController::conversationDeletePolicyStatus() const {
    return conversationDeletePolicyStatusName(conversationDeletePolicy_.status);
}

QString ApplicationController::conversationDeletePolicySummary() const {
    return conversationDeletePolicy_.summary;
}

QStringList ApplicationController::conversationDeletePolicyRequirements() const {
    return conversationDeletePolicy_.requirements;
}

QString ApplicationController::conversationDeleteReadinessStatus() const {
    return conversationDeleteReadiness().status;
}

QString ApplicationController::conversationDeleteReadinessSummary() const {
    return conversationDeleteReadiness().summary;
}

QStringList ApplicationController::conversationDeleteReadinessChecks() const {
    return conversationDeleteReadiness().checks;
}

QString ApplicationController::conversationDeleteLastStatus() const {
    return latestConversationDeleteResult_.status;
}

QString ApplicationController::conversationDeleteLastResultSummary() const {
    return latestConversationDeleteResult_.summary;
}

QString ApplicationController::memoryMaintenanceStatus() const {
    return memoryMaintenanceStatus_;
}

QString ApplicationController::chatMaintenanceStatus() const {
    return chatMaintenanceStatus_;
}

const QList<ChatMessage>& ApplicationController::chatHistory() const {
    return chatSession_->messages();
}

QStringList ApplicationController::chatMessages() const {
    QStringList result;
    for (const auto& message : chatSession_->messages()) {
        if (message.role == ChatRole::User) {
            result.append(QStringLiteral("You: %1").arg(message.content));
        } else {
            result.append(QStringLiteral("Sentinel: %1").arg(message.content));
        }
    }
    return result;
}

QStringList ApplicationController::memoryEntries() const {
    QStringList result;
    if (!memoryStore_) {
        return result;
    }

    for (const auto& [key, value] : memoryStore_->entries()) {
        result.append(QStringLiteral("%1: %2").arg(key, value));
    }

    return result;
}

QList<MemoryCandidate> ApplicationController::memoryCandidates() const {
    return memoryCandidateStore_ ? memoryCandidateStore_->candidates() : QList<MemoryCandidate>{};
}

int ApplicationController::memoryCandidateCountForState(MemoryReviewState state) const {
    int count = 0;
    for (const auto& candidate : memoryCandidates()) {
        if (candidate.reviewState == state) {
            ++count;
        }
    }
    return count;
}

int ApplicationController::memoryCandidateCount() const {
    return memoryCandidates().size();
}

int ApplicationController::pendingMemoryCandidateCount() const {
    return memoryCandidateCountForState(MemoryReviewState::PendingReview);
}

int ApplicationController::approvedMemoryCandidateCount() const {
    return memoryCandidateCountForState(MemoryReviewState::Approved);
}

int ApplicationController::rejectedMemoryCandidateCount() const {
    return memoryCandidateCountForState(MemoryReviewState::Rejected);
}

QStringList ApplicationController::memoryCandidateSummaries() const {
    QStringList result;
    for (const auto& candidate : memoryCandidates()) {
        const auto summary = memoryCandidateSummary(candidate);
        result.append(QStringLiteral("%1: %2").arg(summary.id, summary.summary));
    }
    return result;
}

MemoryCandidate
ApplicationController::memoryCandidateFromConversationText(const QString& text) const {
    const auto normalized = text.simplified();
    MemoryCandidate candidate;
    candidate.source = MemoryCandidateSource::ConversationText;
    candidate.category = MemoryCandidateCategory::Semantic;
    candidate.confidence = MemoryCandidateConfidence::Medium;
    candidate.reviewState = MemoryReviewState::PendingReview;
    candidate.retentionPolicy = MemoryRetentionPolicy::UserControlled;
    candidate.capturePolicy = MemoryCapturePolicy::ManualReviewRequired;
    candidate.title = QStringLiteral("Conversation Memory Candidate");
    candidate.excerpt = normalized.left(160);
    candidate.sourceSummary = QStringLiteral("Captured from conversation text metadata only.");
    candidate.reviewSummary = QStringLiteral("Pending user review.");
    return candidate;
}

bool ApplicationController::searchConversation(const QString& query) {
    const auto trimmed = query.trimmed();
    latestConversationSearchSummary_ = ConversationSearchSummary{};
    latestConversationSearchSummary_.query.text = trimmed;
    latestConversationSearchSummary_.transcriptMessageCount = chatSession_->messages().size();

    if (trimmed.isEmpty()) {
        latestConversationSearchSummary_.summary =
            QStringLiteral("No transcript search query entered.");
        emit conversationSearchChanged();
        return false;
    }

    const auto caseSensitivity = Qt::CaseInsensitive;
    for (const auto& message : chatSession_->messages()) {
        if (!latestConversationSearchSummary_.query.includeSystemMessages &&
            message.role == ChatRole::System) {
            continue;
        }

        const auto matchIndex = message.content.indexOf(trimmed, 0, caseSensitivity);
        if (matchIndex < 0) {
            continue;
        }

        latestConversationSearchSummary_.results.append(ConversationSearchResult{
            message.id,
            chatRoleName(message.role),
            static_cast<int>(matchIndex),
            conversationSearchPreview(message.content, static_cast<int>(matchIndex),
                                      static_cast<int>(trimmed.size())),
        });
    }

    latestConversationSearchSummary_.status = ConversationSearchStatus::Completed;
    latestConversationSearchSummary_.resultCount = latestConversationSearchSummary_.results.size();
    latestConversationSearchSummary_.summary =
        latestConversationSearchSummary_.resultCount == 0
            ? QStringLiteral("No in-memory transcript matches for \"%1\".").arg(trimmed)
            : QStringLiteral("Found %1 in-memory transcript %2 for \"%3\".")
                  .arg(latestConversationSearchSummary_.resultCount)
                  .arg(latestConversationSearchSummary_.resultCount == 1
                           ? QStringLiteral("match")
                           : QStringLiteral("matches"))
                  .arg(trimmed);
    emit conversationSearchChanged();
    return latestConversationSearchSummary_.resultCount > 0;
}

void ApplicationController::clearConversationSearch() {
    resetConversationSearchSummary();
    emit conversationSearchChanged();
}

bool ApplicationController::exportTranscript(const QString& format) {
    latestConversationExportResult_ = ConversationExportResult{};
    latestConversationExportResult_.request.format = conversationExportFormatFromName(format);
    const auto exportFormat = latestConversationExportResult_.request.format;
    const auto extension = conversationExportExtension(exportFormat);
    if (extension.isEmpty()) {
        latestConversationExportResult_.status = QStringLiteral("Refused");
        latestConversationExportResult_.refusalSummary =
            QStringLiteral("Unsupported transcript export format.");
        latestConversationExportResult_.summary =
            QStringLiteral("Transcript export refused: only Markdown and JSON are supported.");
        emit conversationExportChanged();
        return false;
    }

    QList<ChatMessage> messages;
    int nonSystemMessageCount = 0;
    for (const auto& message : chatSession_->messages()) {
        if (message.role != ChatRole::System) {
            ++nonSystemMessageCount;
        }
        if (latestConversationExportResult_.request.includeSystemMessages ||
            message.role != ChatRole::System) {
            messages.append(message);
        }
    }

    if (nonSystemMessageCount == 0) {
        latestConversationExportResult_.status = QStringLiteral("Refused");
        latestConversationExportResult_.refusalSummary =
            QStringLiteral("Transcript has no user or assistant messages to export.");
        latestConversationExportResult_.summary =
            QStringLiteral("Transcript export refused: the current transcript is empty.");
        emit conversationExportChanged();
        return false;
    }

    QDir exportDirectory(conversationExportDirectory_);
    if (!exportDirectory.exists() && !exportDirectory.mkpath(QStringLiteral("."))) {
        latestConversationExportResult_.status = QStringLiteral("Failed");
        latestConversationExportResult_.errorSummary =
            QStringLiteral("Could not create the controlled export directory.");
        latestConversationExportResult_.summary =
            QStringLiteral("Transcript export failed before writing a file.");
        emit conversationExportChanged();
        return false;
    }

    const auto exportedAt =
        QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyy-MM-ddTHH:mm:ss.zzzZ"));
    const auto timestampForFile =
        QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    const auto baseName =
        sanitizedExportFileStem(QStringLiteral("sentinel-transcript-%1").arg(timestampForFile));
    const auto outputPath = uniqueExportFilePath(exportDirectory, baseName, extension);

    QByteArray payload;
    if (exportFormat == ConversationExportFormat::Markdown) {
        payload = markdownTranscript(messages, exportedAt).toUtf8();
    } else {
        payload = jsonTranscript(messages, exportedAt);
    }

    QSaveFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        latestConversationExportResult_.status = QStringLiteral("Failed");
        latestConversationExportResult_.errorSummary =
            QStringLiteral("Could not open the controlled export file.");
        latestConversationExportResult_.summary =
            QStringLiteral("Transcript export failed before writing a file.");
        emit conversationExportChanged();
        return false;
    }

    if (file.write(payload) != payload.size() || !file.commit()) {
        latestConversationExportResult_.status = QStringLiteral("Failed");
        latestConversationExportResult_.errorSummary =
            QStringLiteral("Could not complete the controlled export file write.");
        latestConversationExportResult_.summary =
            QStringLiteral("Transcript export failed before completing the file.");
        emit conversationExportChanged();
        return false;
    }

    latestConversationExportResult_.success = true;
    latestConversationExportResult_.wroteFile = true;
    latestConversationExportResult_.status = QStringLiteral("Succeeded");
    latestConversationExportResult_.outputPath = outputPath;
    latestConversationExportResult_.outputFileName = QFileInfo(outputPath).fileName();
    latestConversationExportResult_.messageCount = messages.size();
    latestConversationExportResult_.exportedAtUtc = exportedAt;
    latestConversationExportResult_.summary =
        QStringLiteral("%1 transcript export saved %2 messages to %3.")
            .arg(conversationExportFormatName(exportFormat))
            .arg(messages.size())
            .arg(latestConversationExportResult_.outputFileName);
    emit conversationExportChanged();
    return true;
}

bool ApplicationController::requestConversationExport(const QString& format) {
    return exportTranscript(format);
}

bool ApplicationController::requestPermanentDeleteConversation(const QString& conversationId) {
    latestConversationDeleteResult_ = ConversationDeleteResult{};
    latestConversationDeleteResult_.conversationId = conversationId.trimmed();
    latestConversationDeleteResult_.status = QStringLiteral("Refused");
    latestConversationDeleteResult_.refusalSummary =
        QStringLiteral("Permanent delete is disabled by the archive-first conversation policy.");
    latestConversationDeleteResult_.summary =
        QStringLiteral("Permanent delete refused: archive or unarchive conversations instead.");
    emit conversationDeleteChanged();
    return false;
}

QString ApplicationController::createConversation(const QString& title) {
    if (!conversationStore_ || conversationStore_->status() != ConversationStoreStatus::Ready) {
        return {};
    }

    if (localInferenceBusy_ && !activeLocalInferenceRequestId_.isEmpty()) {
        cancelLocalInference();
    }

    const auto record = conversationStore_->createConversation(title);
    if (record.id.isEmpty()) {
        return {};
    }
    activeConversationId_ = record.id;
    loadActiveConversationTranscript();
    resetConversationRuntimeState();
    conversationStateGraph_.reset();
    refreshConversationHistorySummary();
    resetConversationSearchSummary();
    emit localInferenceChanged();
    emit conversationRuntimeChanged();
    emit conversationStateChanged();
    emit conversationSearchChanged();
    emit chatMessagesChanged();
    return activeConversationId_;
}

bool ApplicationController::switchConversation(const QString& conversationId) {
    const auto trimmedId = conversationId.trimmed();
    if (trimmedId.isEmpty() || !conversationStore_ ||
        conversationStore_->status() != ConversationStoreStatus::Ready) {
        return false;
    }

    bool found = false;
    for (const auto& record : conversationStore_->listConversations()) {
        if (record.id == trimmedId && !record.deleted) {
            found = true;
            break;
        }
    }
    if (!found) {
        return false;
    }
    if (trimmedId == activeConversationId_) {
        return true;
    }

    if (localInferenceBusy_ && !activeLocalInferenceRequestId_.isEmpty()) {
        cancelLocalInference();
    }
    activeConversationId_ = trimmedId;
    loadActiveConversationTranscript();
    resetConversationRuntimeState();
    conversationStateGraph_.reset();
    refreshConversationHistorySummary();
    resetConversationSearchSummary();
    emit localInferenceChanged();
    emit conversationRuntimeChanged();
    emit conversationStateChanged();
    emit conversationSearchChanged();
    emit chatMessagesChanged();
    return true;
}

bool ApplicationController::renameConversation(const QString& conversationId,
                                               const QString& title) {
    if (!conversationStore_ || conversationStore_->status() != ConversationStoreStatus::Ready) {
        return false;
    }
    const auto renamed = conversationStore_->renameConversation(conversationId.trimmed(), title);
    if (renamed) {
        emit chatMessagesChanged();
    }
    return renamed;
}

bool ApplicationController::archiveConversation(const QString& conversationId) {
    if (!conversationStore_ || conversationStore_->status() != ConversationStoreStatus::Ready) {
        return false;
    }
    const auto archived = conversationStore_->archiveConversation(conversationId.trimmed());
    if (!archived) {
        return false;
    }
    if (conversationId.trimmed() == activeConversationId_) {
        if (localInferenceBusy_ && !activeLocalInferenceRequestId_.isEmpty()) {
            cancelLocalInference();
        }
        activeConversationId_.clear();
        ensureActiveConversation();
        loadActiveConversationTranscript();
        resetConversationRuntimeState();
        conversationStateGraph_.reset();
        refreshConversationHistorySummary();
        resetConversationSearchSummary();
        emit localInferenceChanged();
        emit conversationRuntimeChanged();
        emit conversationStateChanged();
        emit conversationSearchChanged();
    }
    emit chatMessagesChanged();
    return true;
}

bool ApplicationController::unarchiveConversation(const QString& conversationId) {
    if (!conversationStore_ || conversationStore_->status() != ConversationStoreStatus::Ready) {
        return false;
    }
    const auto unarchived = conversationStore_->unarchiveConversation(conversationId.trimmed());
    if (unarchived) {
        emit chatMessagesChanged();
    }
    return unarchived;
}

QString ApplicationController::createMemoryCandidateFromConversationText(const QString& text) {
    if (!memoryCandidateStore_) {
        return {};
    }

    const auto trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }

    const auto candidate =
        memoryCandidateStore_->createCandidate(memoryCandidateFromConversationText(trimmed));
    emit memoryCandidatesChanged();
    return candidate.id;
}

bool ApplicationController::approveMemoryCandidate(const QString& candidateId) {
    if (!memoryCandidateStore_) {
        return false;
    }

    const auto reviewed =
        memoryCandidateStore_->setReviewState(candidateId, MemoryReviewState::Approved,
                                              QStringLiteral("Approved by user review metadata."));
    if (reviewed) {
        emit memoryCandidatesChanged();
    }
    return reviewed;
}

bool ApplicationController::rejectMemoryCandidate(const QString& candidateId) {
    if (!memoryCandidateStore_) {
        return false;
    }

    const auto reviewed =
        memoryCandidateStore_->setReviewState(candidateId, MemoryReviewState::Rejected,
                                              QStringLiteral("Rejected by user review metadata."));
    if (reviewed) {
        emit memoryCandidatesChanged();
    }
    return reviewed;
}

bool ApplicationController::sendMessage(const QString& message) {
    const auto trimmed = message.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    if (activeConversationArchived()) {
        return false;
    }

    if (localInferenceBusy_) {
        LocalInferenceRequest request;
        request.prompt = trimmed;
        request.options.model = effectiveLocalModel({});
        const auto config = ollamaRuntimeClient_ ? ollamaRuntimeClient_->config() : OllamaConfig{};
        request.options.timeoutMs =
            localInferenceStreamingEnabled_ ? config.streamTimeoutMs : config.generateTimeoutMs;
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::BusyRequest,
            QStringLiteral("Local inference request rejected: another request is already "
                           "running."));
        latestLocalInferenceResponse_.status = LocalInferenceStatus::Busy;
        latestLocalInferenceStreamResult_.accumulatedText.clear();
        emit localInferenceChanged();
        return false;
    }

    resetCompletedConversationState();
    transitionConversationState(ConversationState::Listening,
                                QStringLiteral("chat input metadata received"));
    transitionConversationState(ConversationState::Planning,
                                QStringLiteral("chat request metadata prepared"));
    transitionConversationState(ConversationState::Routing,
                                QStringLiteral("chat route metadata selected"));

    const auto userMessage = chatSession_->appendUserMessage(trimmed);
    persistActiveConversationMessage(userMessage);
    if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
        chatHistoryStore_->appendMessage(userMessage);
        conversationHistorySummary_.lastSavedStatus =
            chatHistoryStore_->lastError().isEmpty() ? QStringLiteral("Saved latest user message.")
                                                     : QStringLiteral("User message save failed.");
    } else {
        conversationHistorySummary_.lastSavedStatus =
            QStringLiteral("Runtime-only transcript; persistence unavailable.");
    }

    if (localChatInferenceEnabled_) {
        transitionConversationState(ConversationState::ReadyToRespond,
                                    QStringLiteral("local chat inference metadata ready"));
        transitionConversationState(ConversationState::Responding,
                                    QStringLiteral("local chat inference metadata active"));

        refreshConversationHistorySummary();
        emit chatMessagesChanged();
        activeLocalInferenceIsChatRequest_ = true;
        const auto startedLocalInference = localInferenceStreamingAvailable()
                                               ? runLocalInferenceStream(trimmed, {})
                                               : runLocalInference(trimmed, {});
        if (startedLocalInference || !activeLocalInferenceIsChatRequest_) {
            return latestLocalInferenceResponse_.status == LocalInferenceStatus::Succeeded ||
                   localInferenceBusy_;
        }

        finalizeLocalChatInference(false);
        return false;
    }

    if (!provider_ || provider_->status() != ChatProviderStatus::Ready) {
        transitionConversationState(ConversationState::Error,
                                    QStringLiteral("provider metadata unavailable"));
        const auto errorMessage = chatSession_->appendAssistantMessage(
            QStringLiteral("Provider unavailable. Status: %1").arg(providerStatus()),
            ChatMessageStatus::Error);
        persistActiveConversationMessage(errorMessage);
        if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
            chatHistoryStore_->appendMessage(errorMessage);
            conversationHistorySummary_.lastSavedStatus =
                chatHistoryStore_->lastError().isEmpty()
                    ? QStringLiteral("Saved latest assistant message.")
                    : QStringLiteral("Assistant message save failed.");
        } else {
            conversationHistorySummary_.lastSavedStatus =
                QStringLiteral("Runtime-only transcript; persistence unavailable.");
        }
        setConversationRuntimeRequest(QStringLiteral("provider-chat-request"),
                                      QStringLiteral("None"),
                                      QStringLiteral("Provider: %1").arg(providerName()), false);
        setConversationRuntimeResult(false, errorMessage.content);
        refreshConversationHistorySummary();
        emit chatMessagesChanged();
        return false;
    }

    transitionConversationState(ConversationState::ReadyToRespond,
                                QStringLiteral("chat response metadata ready"));
    transitionConversationState(ConversationState::Responding,
                                QStringLiteral("chat response metadata active"));
    const auto reply = provider_->sendMessage(trimmed);
    if (!reply.success) {
        transitionConversationState(ConversationState::Error,
                                    QStringLiteral("chat provider returned error metadata"));
    } else {
        transitionConversationState(ConversationState::Completed,
                                    QStringLiteral("chat response metadata completed"));
    }
    const auto assistantMessage = chatSession_->appendAssistantMessage(
        reply.success ? reply.message
                      : QStringLiteral("Provider error: %1").arg(reply.errorMessage),
        reply.success ? ChatMessageStatus::Received : ChatMessageStatus::Error);
    persistActiveConversationMessage(assistantMessage);
    if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
        chatHistoryStore_->appendMessage(assistantMessage);
        conversationHistorySummary_.lastSavedStatus =
            chatHistoryStore_->lastError().isEmpty()
                ? QStringLiteral("Saved latest assistant message.")
                : QStringLiteral("Assistant message save failed.");
    } else {
        conversationHistorySummary_.lastSavedStatus =
            QStringLiteral("Runtime-only transcript; persistence unavailable.");
    }
    setConversationRuntimeRequest(QStringLiteral("provider-chat-request"), QStringLiteral("None"),
                                  QStringLiteral("Provider: %1").arg(providerName()), false);
    setConversationRuntimeResult(reply.success, assistantMessage.content);
    refreshConversationHistorySummary();
    emit chatMessagesChanged();
    return reply.success;
}

bool ApplicationController::runLocalInference(const QString& prompt, const QString& model) {
    if (localInferenceBusy_) {
        LocalInferenceRequest request;
        request.prompt = prompt.trimmed();
        request.options.model = effectiveLocalModel(model);
        const auto config = ollamaRuntimeClient_ ? ollamaRuntimeClient_->config() : OllamaConfig{};
        request.options.timeoutMs = config.generateTimeoutMs;
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::BusyRequest,
            QStringLiteral("Local inference request rejected: another request is already "
                           "running."));
        emit localInferenceChanged();
        return false;
    }

    LocalInferenceRequest request;
    request.prompt = prompt.trimmed();
    request.options.model = effectiveLocalModel(model);
    const auto config = ollamaRuntimeClient_ ? ollamaRuntimeClient_->config() : OllamaConfig{};
    request.options.timeoutMs = config.generateTimeoutMs;
    latestLocalInferenceStreamResult_.accumulatedText.clear();

    if (request.prompt.isEmpty()) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::BlankPrompt,
            QStringLiteral("Local inference request rejected: prompt is blank."));
        latestLocalInferenceResponse_.status = LocalInferenceStatus::InvalidRequest;
        emit localInferenceChanged();
        return false;
    }

    if (request.options.model.isEmpty()) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::MissingModel,
            QStringLiteral("Local inference request rejected: model is required."));
        latestLocalInferenceResponse_.status = LocalInferenceStatus::InvalidRequest;
        emit localInferenceChanged();
        return false;
    }

    if (!localInferenceEndpointAllowed()) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::EndpointBlocked,
            QStringLiteral("Local inference blocked: endpoint must be local loopback HTTP."));
        emit localInferenceChanged();
        return false;
    }

    const auto discoveredModels = currentOllamaModels();
    if (!discoveredModels.isEmpty() &&
        !discoveredModelNamesContain(request.options.model, discoveredModels)) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::ModelUnavailable,
            QStringLiteral("Local inference request rejected: selected model is not installed."));
        latestLocalInferenceResponse_.status = LocalInferenceStatus::ModelUnavailable;
        emit localInferenceChanged();
        return false;
    }

    const auto permissionDecision = currentRuntimePermissionDecision();
    if (permissionDecision.status != RuntimePermissionDecisionStatus::Allowed) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::PermissionDenied,
            QStringLiteral("Local inference blocked by runtime permission policy."));
        latestLocalInferenceResponse_.traces.append(LocalInferenceTrace{
            2,
            QStringLiteral("Permission Policy"),
            runtimePermissionDecisionStatusName(permissionDecision.status),
            safeRuntimePermissionDecisionSummary(permissionDecision),
        });
        emit localInferenceChanged();
        return false;
    }

    const auto safetyReport = currentRuntimeSafetyReport();
    if (safetyReport.decision != RuntimeSafetyDecision::Compliant) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::SafetyBlocked,
            QStringLiteral("Local inference blocked by runtime safety policy."));
        latestLocalInferenceResponse_.traces.append(LocalInferenceTrace{
            2,
            QStringLiteral("Permission Policy"),
            runtimePermissionDecisionStatusName(permissionDecision.status),
            safeRuntimePermissionDecisionSummary(permissionDecision),
        });
        latestLocalInferenceResponse_.traces.append(LocalInferenceTrace{
            3,
            QStringLiteral("Safety Policy"),
            runtimeSafetyDecisionName(safetyReport.decision),
            safeRuntimeSafetySummary(safetyReport),
        });
        emit localInferenceChanged();
        return false;
    }

    const auto health =
        localInferenceClientIsRealOllama_ ? currentOllamaHealthCheck() : OllamaHealthCheckResult{};
    if (localInferenceClientIsRealOllama_ && health.healthStatus != OllamaHealthStatus::Healthy) {
        const auto summary = health.summary.trimmed().isEmpty()
                                 ? QStringLiteral("Local inference blocked: Ollama health check "
                                                  "did not report a healthy local endpoint.")
                                 : health.summary.trimmed();
        const auto lowered = summary.toLower();
        LocalInferenceError error = LocalInferenceError::EndpointUnreachable;
        if (lowered.contains(QStringLiteral("not running")) ||
            lowered.contains(QStringLiteral("connection refused"))) {
            error = LocalInferenceError::OllamaNotRunning;
        } else if (lowered.contains(QStringLiteral("timed out"))) {
            error = LocalInferenceError::Timeout;
        }
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(request, error, summary);
        latestLocalInferenceResponse_.status = LocalInferenceStatus::Error;
        latestLocalInferenceResponse_.timeoutMs = health.timeoutMs;
        emit localInferenceChanged();
        return false;
    }

    if (!localInferenceWorker_) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::ClientUnavailable,
            QStringLiteral("Local inference blocked: client is unavailable."));
        emit localInferenceChanged();
        return false;
    }

    request.id = QStringLiteral("local-inference-request-%1").arg(++localInferenceRequestSequence_);
    activeLocalInferenceRequestId_ = request.id;
    localInferenceBusy_ = true;
    setConversationRuntimeRequest(request.id, request.options.model, QStringLiteral("Local Ollama"),
                                  false);
    latestLocalInferenceResponse_.status = LocalInferenceStatus::Busy;
    latestLocalInferenceResponse_.summary = QStringLiteral("Local inference request is running.");
    latestLocalInferenceResponse_.timeoutMs = request.options.timeoutMs;
    emit localInferenceChanged();

    const auto started = localInferenceWorker_->startInference(
        request, [this, permissionDecision, safetyReport](const QString& requestId,
                                                          const LocalInferenceResponse& response) {
            auto completedResponse = response;
            completedResponse.traces = {
                LocalInferenceTrace{
                    1,
                    QStringLiteral("Permission Policy"),
                    runtimePermissionDecisionStatusName(permissionDecision.status),
                    safeRuntimePermissionDecisionSummary(permissionDecision),
                },
                LocalInferenceTrace{
                    2,
                    QStringLiteral("Safety Policy"),
                    runtimeSafetyDecisionName(safetyReport.decision),
                    safeRuntimeSafetySummary(safetyReport),
                },
            };
            completedResponse.traces.append(response.traces);
            for (int i = 0; i < completedResponse.traces.size(); ++i) {
                completedResponse.traces[i].sequence = i + 1;
            }
            finishLocalInferenceRequest(requestId, completedResponse);
        });
    if (!started) {
        localInferenceBusy_ = false;
        activeLocalInferenceRequestId_.clear();
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::BusyRequest,
            QStringLiteral("Local inference request rejected: worker is busy."));
        latestLocalInferenceResponse_.status = LocalInferenceStatus::Busy;
        emit localInferenceChanged();
        return false;
    }
    return localInferenceBusy_ ||
           latestLocalInferenceResponse_.status == LocalInferenceStatus::Succeeded;
}

bool ApplicationController::cancelLocalInference() {
    if (!localInferenceBusy_ || activeLocalInferenceRequestId_.isEmpty()) {
        return false;
    }

    const auto requestId = activeLocalInferenceRequestId_;
    if (localInferenceWorker_) {
        localInferenceWorker_->cancel(requestId);
    }
    activeLocalInferenceRequestId_.clear();
    activeLocalInferenceIsChatRequest_ = false;
    localInferenceBusy_ = false;
    latestLocalInferenceResponse_.status = LocalInferenceStatus::Blocked;
    latestLocalInferenceResponse_.error = LocalInferenceError::RequestFailed;
    latestLocalInferenceResponse_.text.clear();
    latestLocalInferenceResponse_.summary =
        QStringLiteral("Local inference request was cancelled; stale results will be ignored.");
    setConversationRuntimeResult(false, latestLocalInferenceResponse_.summary);
    latestLocalInferenceStreamResult_.accumulatedText.clear();
    if (latestLocalInferenceStreamResult_.status == LocalInferenceStreamStatus::Streaming) {
        latestLocalInferenceStreamResult_.status = LocalInferenceStreamStatus::Cancelled;
        latestLocalInferenceStreamResult_.summary =
            QStringLiteral("Local streaming request was cancelled.");
    }
    emit localInferenceChanged();
    return true;
}

bool ApplicationController::runLocalInferenceStream(const QString& prompt, const QString& model) {
    if (localInferenceBusy_) {
        LocalInferenceRequest request;
        request.prompt = prompt.trimmed();
        request.options.model = effectiveLocalModel(model);
        request.options.streamingRequested = true;
        const auto config = ollamaRuntimeClient_ ? ollamaRuntimeClient_->config() : OllamaConfig{};
        request.options.timeoutMs = config.streamTimeoutMs;
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::BusyRequest,
            QStringLiteral("Local streaming request rejected: another request is already "
                           "running."));
        latestLocalInferenceStreamResult_.accumulatedText.clear();
        emit localInferenceChanged();
        return false;
    }

    LocalInferenceRequest request;
    request.prompt = prompt.trimmed();
    request.options.model = effectiveLocalModel(model);
    request.options.streamingRequested = true;
    const auto config = ollamaRuntimeClient_ ? ollamaRuntimeClient_->config() : OllamaConfig{};
    request.options.timeoutMs = config.streamTimeoutMs;

    auto blockStream = [this,
                        &request](LocalInferenceError error, const QString& summary,
                                  LocalInferenceStatus status = LocalInferenceStatus::Blocked) {
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(request, error, summary);
        latestLocalInferenceResponse_.status = status;
        latestLocalInferenceResponse_.timeoutMs = request.options.timeoutMs;
        latestLocalInferenceStreamResult_ = LocalInferenceStreamResult{};
        latestLocalInferenceStreamResult_.status = LocalInferenceStreamStatus::Refused;
        latestLocalInferenceStreamResult_.error = error;
        latestLocalInferenceStreamResult_.summary = summary;
        latestLocalInferenceStreamResult_.timeoutMs = request.options.timeoutMs;
        emit localInferenceChanged();
        return false;
    };

    if (!localInferenceStreamingEnabled_) {
        return blockStream(LocalInferenceError::RequestFailed,
                           QStringLiteral("Local inference streaming is disabled; using "
                                          "non-streaming local inference when chat routing allows "
                                          "it."),
                           LocalInferenceStatus::Refused);
    }

    if (request.prompt.isEmpty()) {
        return blockStream(LocalInferenceError::BlankPrompt,
                           QStringLiteral("Local streaming request rejected: prompt is blank."),
                           LocalInferenceStatus::InvalidRequest);
    }

    if (request.options.model.isEmpty()) {
        return blockStream(LocalInferenceError::MissingModel,
                           QStringLiteral("Local streaming request rejected: model is required."),
                           LocalInferenceStatus::InvalidRequest);
    }

    if (!localInferenceEndpointAllowed()) {
        return blockStream(LocalInferenceError::EndpointBlocked,
                           QStringLiteral("Local streaming blocked: endpoint must be local "
                                          "loopback HTTP."));
    }

    const auto discoveredModels = currentOllamaModels();
    if (!discoveredModels.isEmpty() &&
        !discoveredModelNamesContain(request.options.model, discoveredModels)) {
        return blockStream(LocalInferenceError::ModelUnavailable,
                           QStringLiteral("Local streaming request rejected: selected model is "
                                          "not installed."),
                           LocalInferenceStatus::ModelUnavailable);
    }

    const auto permissionDecision = currentRuntimePermissionDecision();
    if (permissionDecision.status != RuntimePermissionDecisionStatus::Allowed) {
        const auto blocked =
            blockStream(LocalInferenceError::PermissionDenied,
                        QStringLiteral("Local streaming blocked by runtime permission policy."));
        latestLocalInferenceResponse_.traces.append(LocalInferenceTrace{
            2,
            QStringLiteral("Permission Policy"),
            runtimePermissionDecisionStatusName(permissionDecision.status),
            safeRuntimePermissionDecisionSummary(permissionDecision),
        });
        emit localInferenceChanged();
        return blocked;
    }

    const auto safetyReport = currentRuntimeSafetyReport();
    if (safetyReport.decision != RuntimeSafetyDecision::Compliant) {
        const auto blocked =
            blockStream(LocalInferenceError::SafetyBlocked,
                        QStringLiteral("Local streaming blocked by runtime safety policy."));
        latestLocalInferenceResponse_.traces.append(LocalInferenceTrace{
            2,
            QStringLiteral("Permission Policy"),
            runtimePermissionDecisionStatusName(permissionDecision.status),
            safeRuntimePermissionDecisionSummary(permissionDecision),
        });
        latestLocalInferenceResponse_.traces.append(LocalInferenceTrace{
            3,
            QStringLiteral("Safety Policy"),
            runtimeSafetyDecisionName(safetyReport.decision),
            safeRuntimeSafetySummary(safetyReport),
        });
        emit localInferenceChanged();
        return blocked;
    }

    const auto health = localInferenceStreamClientIsRealOllama_ ? currentOllamaHealthCheck()
                                                                : OllamaHealthCheckResult{};
    if (localInferenceStreamClientIsRealOllama_ &&
        health.healthStatus != OllamaHealthStatus::Healthy) {
        const auto summary = health.summary.trimmed().isEmpty()
                                 ? QStringLiteral("Local streaming blocked: Ollama health check "
                                                  "did not report a healthy local endpoint.")
                                 : health.summary.trimmed();
        const auto lowered = summary.toLower();
        LocalInferenceError error = LocalInferenceError::EndpointUnreachable;
        if (lowered.contains(QStringLiteral("not running")) ||
            lowered.contains(QStringLiteral("connection refused"))) {
            error = LocalInferenceError::OllamaNotRunning;
        } else if (lowered.contains(QStringLiteral("timed out"))) {
            error = LocalInferenceError::Timeout;
        }
        const auto blocked = blockStream(error, summary, LocalInferenceStatus::Error);
        latestLocalInferenceResponse_.timeoutMs = health.timeoutMs;
        emit localInferenceChanged();
        return blocked;
    }

    if (!localInferenceWorker_ || !localInferenceWorker_->streamingAvailable()) {
        return blockStream(LocalInferenceError::ClientUnavailable,
                           QStringLiteral("Local streaming blocked: client is unavailable."));
    }

    request.id = QStringLiteral("local-inference-request-%1").arg(++localInferenceRequestSequence_);
    activeLocalInferenceRequestId_ = request.id;
    localInferenceBusy_ = true;
    setConversationRuntimeRequest(request.id, request.options.model, QStringLiteral("Local Ollama"),
                                  true);
    latestLocalInferenceResponse_.status = LocalInferenceStatus::Busy;
    latestLocalInferenceResponse_.model = request.options.model;
    latestLocalInferenceResponse_.text.clear();
    latestLocalInferenceResponse_.summary = QStringLiteral("Local streaming request is running.");
    latestLocalInferenceStreamResult_ = LocalInferenceStreamResult{};
    latestLocalInferenceStreamResult_.status = LocalInferenceStreamStatus::Streaming;
    latestLocalInferenceStreamResult_.model = request.options.model;
    latestLocalInferenceStreamResult_.timeoutMs = request.options.timeoutMs;
    latestLocalInferenceStreamResult_.summary =
        QStringLiteral("Local streaming request is running.");
    emit localInferenceChanged();

    const auto started = localInferenceWorker_->startStream(
        request,
        [this](const QString& requestId, const LocalInferenceStreamChunk& chunk) {
            updateLocalInferenceStreamRequest(requestId, chunk);
        },
        [this, permissionDecision, safetyReport](const QString& requestId,
                                                 const LocalInferenceStreamResult& result) {
            auto completedResult = result;
            completedResult.traces = {
                LocalInferenceTrace{
                    1,
                    QStringLiteral("Permission Policy"),
                    runtimePermissionDecisionStatusName(permissionDecision.status),
                    safeRuntimePermissionDecisionSummary(permissionDecision),
                },
                LocalInferenceTrace{
                    2,
                    QStringLiteral("Safety Policy"),
                    runtimeSafetyDecisionName(safetyReport.decision),
                    safeRuntimeSafetySummary(safetyReport),
                },
            };
            completedResult.traces.append(result.traces);
            for (int i = 0; i < completedResult.traces.size(); ++i) {
                completedResult.traces[i].sequence = i + 1;
            }
            finishLocalInferenceStreamRequest(requestId, completedResult);
        });
    if (!started) {
        localInferenceBusy_ = false;
        activeLocalInferenceRequestId_.clear();
        return blockStream(LocalInferenceError::BusyRequest,
                           QStringLiteral("Local streaming request rejected: worker is busy."),
                           LocalInferenceStatus::Busy);
    }
    return localInferenceBusy_ ||
           latestLocalInferenceResponse_.status == LocalInferenceStatus::Succeeded;
}

void ApplicationController::finishLocalInferenceRequest(const QString& requestId,
                                                        const LocalInferenceResponse& response) {
    if (requestId != activeLocalInferenceRequestId_) {
        return;
    }

    localInferenceBusy_ = false;
    activeLocalInferenceRequestId_.clear();
    latestLocalInferenceResponse_ = response;
    if (latestLocalInferenceResponse_.timeoutMs <= 0) {
        latestLocalInferenceResponse_.timeoutMs =
            ollamaRuntimeClient_ ? ollamaRuntimeClient_->config().generateTimeoutMs : 30000;
    }
    if (latestLocalInferenceResponse_.status != LocalInferenceStatus::Succeeded) {
        latestLocalInferenceResponse_.text.clear();
    }
    setConversationRuntimeResult(
        latestLocalInferenceResponse_.status == LocalInferenceStatus::Succeeded,
        latestLocalInferenceResponse_.summary, latestLocalInferenceResponse_.latencyMs);
    emit localInferenceChanged();

    if (activeLocalInferenceIsChatRequest_) {
        finalizeLocalChatInference(latestLocalInferenceResponse_.status ==
                                   LocalInferenceStatus::Succeeded);
    }
}

void ApplicationController::updateLocalInferenceStreamRequest(
    const QString& requestId, const LocalInferenceStreamChunk& chunk) {
    if (requestId != activeLocalInferenceRequestId_ || !localInferenceBusy_) {
        return;
    }

    latestLocalInferenceStreamResult_.chunks.append(chunk);
    if (!chunk.malformed) {
        latestLocalInferenceStreamResult_.accumulatedText.append(chunk.text);
        latestLocalInferenceResponse_.text = latestLocalInferenceStreamResult_.accumulatedText;
    }
    latestLocalInferenceStreamResult_.summary = chunk.summary;
    emit localInferenceChanged();
}

void ApplicationController::finishLocalInferenceStreamRequest(
    const QString& requestId, const LocalInferenceStreamResult& result) {
    if (requestId != activeLocalInferenceRequestId_) {
        return;
    }

    localInferenceBusy_ = false;
    activeLocalInferenceRequestId_.clear();
    latestLocalInferenceStreamResult_ = result;
    if (latestLocalInferenceStreamResult_.timeoutMs <= 0) {
        latestLocalInferenceStreamResult_.timeoutMs =
            ollamaRuntimeClient_ ? ollamaRuntimeClient_->config().streamTimeoutMs : 30000;
    }
    latestLocalInferenceStreamResult_.accumulatedText.clear();
    latestLocalInferenceResponse_.model = result.model;
    latestLocalInferenceResponse_.endpoint = result.endpoint;
    latestLocalInferenceResponse_.text = result.accumulatedText;
    latestLocalInferenceResponse_.error = result.error;
    latestLocalInferenceResponse_.summary = result.summary;
    latestLocalInferenceResponse_.timeoutMs = latestLocalInferenceStreamResult_.timeoutMs;
    if (result.status != LocalInferenceStreamStatus::Completed) {
        latestLocalInferenceResponse_.text.clear();
    }
    latestLocalInferenceResponse_.status =
        result.status == LocalInferenceStreamStatus::Completed   ? LocalInferenceStatus::Succeeded
        : result.status == LocalInferenceStreamStatus::Refused   ? LocalInferenceStatus::Refused
        : result.status == LocalInferenceStreamStatus::Cancelled ? LocalInferenceStatus::Blocked
                                                                 : LocalInferenceStatus::Error;
    latestLocalInferenceResponse_.traces = result.traces;
    setConversationRuntimeResult(
        latestLocalInferenceResponse_.status == LocalInferenceStatus::Succeeded,
        latestLocalInferenceResponse_.summary, latestLocalInferenceResponse_.latencyMs);
    emit localInferenceChanged();

    if (activeLocalInferenceIsChatRequest_) {
        finalizeLocalChatInference(latestLocalInferenceResponse_.status ==
                                   LocalInferenceStatus::Succeeded);
    }
}

void ApplicationController::finalizeLocalChatInference(bool succeeded) {
    activeLocalInferenceIsChatRequest_ = false;
    if (conversationRuntimeRequestId_ == QStringLiteral("None")) {
        setConversationRuntimeRequest(QStringLiteral("local-inference-blocked"),
                                      latestLocalInferenceResponse_.model,
                                      QStringLiteral("Local Ollama"), false);
    }
    setConversationRuntimeResult(succeeded, latestLocalInferenceResponse_.summary,
                                 latestLocalInferenceResponse_.latencyMs);
    const auto assistantMessage = chatSession_->appendAssistantMessage(
        succeeded ? latestLocalInferenceResponse_.text
                  : localInferenceChatFailureMessage(latestLocalInferenceResponse_),
        succeeded ? ChatMessageStatus::Received : ChatMessageStatus::Error);
    persistActiveConversationMessage(assistantMessage);
    if (chatHistoryStore_ && chatHistoryStore_->isAvailable()) {
        chatHistoryStore_->appendMessage(assistantMessage);
        conversationHistorySummary_.lastSavedStatus =
            chatHistoryStore_->lastError().isEmpty()
                ? QStringLiteral("Saved latest assistant message.")
                : QStringLiteral("Assistant message save failed.");
    } else {
        conversationHistorySummary_.lastSavedStatus =
            QStringLiteral("Runtime-only transcript; persistence unavailable.");
    }
    transitionConversationState(succeeded ? ConversationState::Completed : ConversationState::Error,
                                succeeded
                                    ? QStringLiteral("local chat inference metadata completed")
                                    : QStringLiteral("local chat inference metadata blocked"));
    refreshConversationHistorySummary();
    emit chatMessagesChanged();
}

bool ApplicationController::runAgentRequest(const QString& request) {
    const auto trimmed = request.trimmed();
    if (trimmed.isEmpty()) {
        if (lastAgentResponse_ != QStringLiteral("Agent request was empty.")) {
            lastAgentResponse_ = QStringLiteral("Agent request was empty.");
            emit agentResponseChanged();
        }
        return false;
    }

    resetCompletedConversationState();
    transitionConversationState(ConversationState::Listening,
                                QStringLiteral("agent request metadata received"));
    transitionConversationState(ConversationState::Planning,
                                QStringLiteral("agent planning metadata started"));

    agentActivityLog_.append(AgentActivityType::RequestReceived, AgentActivityStatus::Recorded,
                             QStringLiteral("Agent request received."));

    if (!agentRuntime_) {
        transitionConversationState(ConversationState::Error,
                                    QStringLiteral("agent runtime metadata unavailable"));
        agentActivityLog_.append(AgentActivityType::PipelineCompleted, AgentActivityStatus::Blocked,
                                 QStringLiteral("Agent pipeline blocked: runtime unavailable."));
        refreshConversationSession();
        emit agentActivityChanged();
        emit conversationSessionChanged();
        emit orchestrationSnapshotChanged();
        if (lastAgentResponse_ != QStringLiteral("Agent runtime unavailable.")) {
            lastAgentResponse_ = QStringLiteral("Agent runtime unavailable.");
            emit agentResponseChanged();
        }
        return false;
    }

    transitionConversationState(ConversationState::Routing,
                                QStringLiteral("agent route metadata selected"));
    latestAgentPipelineResult_ = buildAgentPipelineResult(AgentRequest{trimmed});
    appendPipelineActivity(latestAgentPipelineResult_);
    runtimeSession_.attachPipelineResult(latestAgentPipelineResult_);
    if (latestAgentPipelineResult_.approvalStatus() == ApprovalStatus::RequiresApproval) {
        transitionConversationState(ConversationState::WaitingForApproval,
                                    QStringLiteral("approval metadata required"));
    } else if (latestAgentPipelineResult_.executionStatus() ==
               ToolExecutionStatus::PlaceholderSucceeded) {
        transitionConversationState(ConversationState::ReadyToRespond,
                                    QStringLiteral("agent response metadata ready"));
        transitionConversationState(ConversationState::Responding,
                                    QStringLiteral("agent response metadata active"));
        transitionConversationState(ConversationState::Completed,
                                    QStringLiteral("agent response metadata completed"));
    } else {
        transitionConversationState(ConversationState::Error,
                                    QStringLiteral("agent pipeline metadata blocked"));
    }
    refreshConversationSession();
    emit toolPlanChanged();
    emit approvalChanged();
    emit sandboxChanged();
    emit toolExecutionChanged();
    emit agentPipelineChanged();
    emit runtimeContextChanged();
    emit conversationSessionChanged();
    emit agentActivityChanged();
    emit orchestrationSnapshotChanged();

    const auto response = agentRuntime_->execute(AgentRequest{trimmed});
    const auto nextMessage =
        response.message.isEmpty() ? QStringLiteral("No agent response.") : response.message;
    if (lastAgentResponse_ != nextMessage) {
        lastAgentResponse_ = nextMessage;
        emit agentResponseChanged();
    }
    emit agentStatusChanged();
    return response.success;
}

AgentPipelineResult
ApplicationController::buildAgentPipelineResult(const AgentRequest& request) const {
    AgentPipelineResult result;
    result.plan = agentRuntime_->plan(request);
    result.approval = approvalPolicy_->evaluate(result.plan);
    result.sandbox = sandboxPolicy_->evaluate(result.plan, result.approval);
    result.execution = toolExecutor_->execute(ToolExecutionRequest{
        result.plan,
        result.approval,
        result.sandbox,
        availableToolIds(),
    });
    result.summary = safeToolExecutionSummary(result.execution);
    return result;
}

void ApplicationController::appendPipelineActivity(const AgentPipelineResult& result) {
    agentActivityLog_.append(AgentActivityType::PlanCreated,
                             planActivityStatus(result.planningStatus()),
                             QStringLiteral("Tool planning evaluated: %1")
                                 .arg(toolInvocationPlanStatusName(result.planningStatus())));
    agentActivityLog_.append(AgentActivityType::ApprovalEvaluated,
                             approvalActivityStatus(result.approvalStatus()),
                             QStringLiteral("Approval metadata evaluated: %1")
                                 .arg(approvalStatusName(result.approvalStatus())));
    agentActivityLog_.append(AgentActivityType::SandboxEvaluated,
                             sandboxActivityStatus(result.sandboxStatus()),
                             QStringLiteral("Sandbox metadata evaluated: %1")
                                 .arg(sandboxStatusName(result.sandboxStatus())));
    agentActivityLog_.append(AgentActivityType::PlaceholderExecutionEvaluated,
                             executionActivityStatus(result.executionStatus()),
                             QStringLiteral("Placeholder execution evaluated: %1")
                                 .arg(toolExecutionStatusName(result.executionStatus())));
    agentActivityLog_.append(
        AgentActivityType::PipelineCompleted, executionActivityStatus(result.executionStatus()),
        QStringLiteral("Agent pipeline finished: %1").arg(agentPipelineStatusName(result)));
}

void ApplicationController::resetCompletedConversationState() {
    if (conversationStateGraph_.currentState() == ConversationState::Completed ||
        conversationStateGraph_.currentState() == ConversationState::Error) {
        transitionConversationState(ConversationState::Idle,
                                    QStringLiteral("conversation state reset for new metadata"));
    }
}

void ApplicationController::transitionConversationState(ConversationState nextState,
                                                        const QString& reason) {
    conversationStateGraph_.transitionTo(nextState, reason);
    emit conversationStateChanged();
    emit conversationRuntimeChanged();
}

void ApplicationController::refreshLatestTaskPlan() {
    if (!taskPlanner_ || !providerCatalog_) {
        latestTaskPlan_ = TaskPlan{
            TaskPlanStatus::Blocked,
            modelRouter_ ? modelRouter_->routingMode() : RoutingMode::LocalOnly,
            TaskClassification{TaskType::Unknown},
            {},
            {},
            QStringLiteral("Task planner metadata is unavailable."),
            {},
            {},
            {},
            {},
            {},
            false,
            false,
        };
        return;
    }

    latestTaskPlan_ = taskPlanner_->plan(TaskPlanningRequest{
        TaskClassification{TaskType::Unknown},
        modelRouter_ ? modelRouter_->routingMode() : RoutingMode::LocalOnly,
        providerCatalog_->entries(),
        agentRegistry_ ? agentRegistry_->agents() : QList<AgentDescriptor>{},
        memoryCatalog_ ? memoryCatalog_->shards() : QList<MemoryShardDescriptor>{},
    });
}

void ApplicationController::refreshConversationSession() {
    conversationSession_.refreshContext(ConversationSessionContextBuilder{}.build(
        currentRoutingMode(), currentAgentSummary(), currentMemoryAffinitySummary(),
        orchestrationSnapshotSummary()));
}

void ApplicationController::resetConversationRuntimeState() {
    if (localInferenceBusy_ && localInferenceWorker_ && !activeLocalInferenceRequestId_.isEmpty()) {
        localInferenceWorker_->cancel(activeLocalInferenceRequestId_);
    }

    localInferenceBusy_ = false;
    activeLocalInferenceIsChatRequest_ = false;
    activeLocalInferenceRequestId_.clear();
    conversationRuntimeRequestId_ = QStringLiteral("None");
    conversationRuntimeActiveModel_ = QStringLiteral("None");
    conversationRuntimeActiveRoute_ = QStringLiteral("Provider");
    conversationRuntimeStreaming_ = false;
    conversationRuntimeLastSuccessSummary_ = QStringLiteral("No successful response yet.");
    conversationRuntimeLastErrorSummary_ = QStringLiteral("No error or refusal yet.");
    conversationRuntimeLastLatencySummary_ = QStringLiteral("No latency recorded.");
    latestLocalInferenceResponse_ = LocalInferenceResponse{};
    latestLocalInferenceStreamResult_ = LocalInferenceStreamResult{};
    emit localInferenceChanged();
    emit conversationRuntimeChanged();
}

void ApplicationController::refreshConversationHistorySummary() {
    ConversationHistorySummary summary;
    summary.persistenceStatus = chatHistoryStore_ && chatHistoryStore_->isAvailable()
                                    ? ConversationPersistenceStatus::Persisted
                                    : ConversationPersistenceStatus::RuntimeOnly;
    summary.lastSavedStatus = conversationHistorySummary_.lastSavedStatus;
    summary.lastRestoredStatus = conversationHistorySummary_.lastRestoredStatus;

    for (const auto& message : chatSession_->messages()) {
        ++summary.messageCount;
        switch (message.role) {
        case ChatRole::System:
            ++summary.systemMessageCount;
            break;
        case ChatRole::User:
            ++summary.userMessageCount;
            break;
        case ChatRole::Assistant:
            ++summary.assistantMessageCount;
            break;
        }
    }

    const auto messageLabel =
        summary.messageCount == 1 ? QStringLiteral("message") : QStringLiteral("messages");
    summary.summary = QStringLiteral("%1 transcript, %2 %3 (%4 user, %5 assistant, %6 system).")
                          .arg(conversationPersistenceStatusName(summary.persistenceStatus))
                          .arg(summary.messageCount)
                          .arg(messageLabel)
                          .arg(summary.userMessageCount)
                          .arg(summary.assistantMessageCount)
                          .arg(summary.systemMessageCount);
    conversationHistorySummary_ = summary;
}

void ApplicationController::resetConversationSearchSummary() {
    latestConversationSearchSummary_ = ConversationSearchSummary{};
    latestConversationSearchSummary_.transcriptMessageCount = chatSession_->messages().size();
}

void ApplicationController::setConversationExportDirectory(const QString& directoryPath) {
    const auto trimmed = directoryPath.trimmed();
    conversationExportDirectory_ =
        trimmed.isEmpty() ? defaultConversationExportDirectory() : trimmed;
}

void ApplicationController::setConversationRuntimeRequest(const QString& requestId,
                                                          const QString& model,
                                                          const QString& route, bool streaming) {
    conversationRuntimeRequestId_ =
        requestId.trimmed().isEmpty() ? QStringLiteral("None") : requestId.trimmed();
    conversationRuntimeActiveModel_ =
        model.trimmed().isEmpty() ? QStringLiteral("None") : model.trimmed();
    conversationRuntimeActiveRoute_ =
        route.trimmed().isEmpty() ? QStringLiteral("Provider") : route.trimmed();
    conversationRuntimeStreaming_ = streaming;
    emit conversationRuntimeChanged();
}

void ApplicationController::setConversationRuntimeResult(bool succeeded, const QString& summary,
                                                         qint64 latencyMs) {
    const auto safeSummary =
        summary.trimmed().isEmpty() ? QStringLiteral("No summary available.") : summary.trimmed();
    conversationRuntimeStreaming_ = false;
    if (succeeded) {
        conversationRuntimeLastSuccessSummary_ = safeSummary;
    } else {
        conversationRuntimeLastErrorSummary_ = safeSummary;
    }
    conversationRuntimeLastLatencySummary_ = latencyMs >= 0
                                                 ? QStringLiteral("%1 ms").arg(latencyMs)
                                                 : QStringLiteral("No latency recorded.");
    emit conversationRuntimeChanged();
}

RuntimePermissionRequest ApplicationController::runtimePermissionRequest() const {
    return RuntimePermissionRequest{
        RuntimePermission::LocalInference,
        RuntimePermissionLevel::Execute,
        QStringLiteral("local-runtime-request"),
        QStringLiteral("Evaluate metadata-only permission for local runtime execution request."),
    };
}

RuntimePermissionDecision ApplicationController::currentRuntimePermissionDecision() const {
    if (!runtimePermissionPolicy_) {
        return RuntimePermissionDecision{
            RuntimePermissionDecisionStatus::Denied,
            runtimePermissionRequest(),
            QStringLiteral("No runtime permission policy available."),
        };
    }

    return runtimePermissionPolicy_->evaluate(runtimePermissionRequest());
}

RuntimeSafetyReport ApplicationController::currentRuntimeSafetyReport() const {
    if (!runtimeSafetyPolicy_) {
        RuntimeSafetyReport report;
        report.policy = RuntimeSafetyPolicy{
            QStringLiteral("runtime-safety-policy-missing"),
            QStringLiteral("Missing Runtime Safety Policy"),
            true,
            true,
            QStringLiteral("No runtime safety policy available."),
        };
        report.decision = RuntimeSafetyDecision::Blocked;
        report.summary = QStringLiteral("No runtime safety policy available.");
        return report;
    }

    return runtimeSafetyPolicy_->evaluate();
}

RuntimePipelineResult ApplicationController::currentRuntimePipelineResult() const {
    if (!runtimePipeline_) {
        RuntimePipelineResult result;
        result.status = RuntimePipelineStatus::Blocked;
        result.summary = QStringLiteral("No runtime pipeline available.");
        result.traces = {RuntimePipelineTrace{
            RuntimePipelineStage::ExecutionBoundary,
            QStringLiteral("Blocked"),
            QStringLiteral("Runtime pipeline is unavailable."),
        }};
        result.executionBlocked = true;
        return result;
    }

    return runtimePipeline_->evaluate(
        RuntimePipelineRequest{
            QStringLiteral("runtime-pipeline-request-1"),
            QStringLiteral("Runtime request pipeline metadata evaluation."),
            runtimePermissionRequest(),
        },
        currentRuntimePermissionDecision(), currentRuntimeSafetyReport());
}

ExecutionRequest ApplicationController::executionRequest() const {
    return ExecutionRequest{
        QStringLiteral("execution-request-1"),
        ExecutionIntent::RuntimePlaceholder,
        ExecutionPriority::Normal,
        QStringLiteral("Coordinate future execution lifecycle metadata without enabling "
                       "execution."),
    };
}

ExecutionLifecycleResult ApplicationController::currentExecutionLifecycleResult() const {
    if (!executionLifecycle_) {
        ExecutionLifecycleResult result;
        result.request = executionRequest();
        result.state = ExecutionLifecycleState::Blocked;
        result.status = ExecutionLifecycleStatus::Blocked;
        result.summary = QStringLiteral("No execution lifecycle available; execution is blocked.");
        result.executable = false;
        result.traces = {ExecutionLifecycleTrace{
            1,
            ExecutionLifecycleState::Blocked,
            ExecutionTraceLevel::Blocked,
            QStringLiteral("Execution lifecycle is unavailable."),
        }};
        return result;
    }

    return executionLifecycle_->evaluate(executionRequest());
}

ExecutionCoordinationSnapshot ApplicationController::currentExecutionCoordinationSnapshot() const {
    if (!executionCoordinator_) {
        ExecutionCoordinationSnapshot snapshot;
        snapshot.lifecycle = currentExecutionLifecycleResult();
        snapshot.readOnly = true;
        snapshot.executable = false;
        snapshot.summary = QStringLiteral("No execution coordinator available; execution is "
                                          "blocked.");
        return snapshot;
    }

    return executionCoordinator_->coordinate(currentExecutionLifecycleResult());
}

ProviderRuntimeBridgeRequest ApplicationController::providerRuntimeBridgeRequest() const {
    return ProviderRuntimeBridgeRequest{
        QStringLiteral("provider-runtime-bridge-request-1"),
        QStringLiteral("ollama-local"),
        localRuntimeAdapter_ ? localRuntimeAdapter_->descriptor().id
                             : QStringLiteral("local-runtime-adapter-unavailable"),
        QStringLiteral("Evaluate provider runtime bridge metadata without connecting or "
                       "executing."),
    };
}

ProviderRuntimeBridgeResponse ApplicationController::currentProviderRuntimeBridgeResponse() const {
    if (!providerRuntimeBridge_) {
        ProviderRuntimeBridgeResponse response;
        response.request = providerRuntimeBridgeRequest();
        response.status = ProviderRuntimeBridgeStatus::Unavailable;
        response.summary = QStringLiteral("Provider runtime bridge is unavailable.");
        response.connected = false;
        response.executable = false;
        return response;
    }

    return providerRuntimeBridge_->evaluate(providerRuntimeBridgeRequest());
}

RuntimeIntegrationReport ApplicationController::currentRuntimeIntegrationReport() const {
    if (!runtimeIntegrationReadiness_) {
        RuntimeIntegrationReport report;
        report.readiness = RuntimeIntegrationReadiness::Blocked;
        report.summary = QStringLiteral("No runtime integration readiness metadata available.");
        report.executable = false;
        return report;
    }

    const auto adapter =
        localRuntimeAdapter_ ? localRuntimeAdapter_->descriptor() : LocalRuntimeAdapterDescriptor{};
    const auto bridge =
        providerRuntimeBridge_ ? providerRuntimeBridge_->summary() : ProviderRuntimeBridgeSummary{};
    auto report = runtimeIntegrationReadiness_->evaluate(adapter, bridge);
    const auto ollamaConfig =
        ollamaRuntimeClient_ ? ollamaRuntimeClient_->config() : OllamaConfig{};
    for (auto& check : report.checks) {
        if (check.id == QStringLiteral("runtime-integration.endpoint")) {
            check.passed = ollamaConfig.endpoint.isLoopbackHttp();
            check.summary = QStringLiteral("Safe local Ollama endpoint is configured for "
                                           "loopback-only health checks.");
        } else if (check.id == QStringLiteral("runtime-integration.model-discovery")) {
            check.passed = ollamaConfig.modelDiscoveryEnabled;
            check.summary = QStringLiteral("Installed model discovery boundary is available for "
                                           "read-only local metadata.");
        }
    }
    report.summary = QStringLiteral("Runtime integration readiness is blocked: Ollama local "
                                    "health/discovery metadata is available, but provider bridge "
                                    "execution and inference remain disabled.");
    return report;
}

OllamaHealthCheckResult ApplicationController::currentOllamaHealthCheck() const {
    if (!ollamaRuntimeClient_) {
        return NullOllamaRuntimeClient{}.healthCheck();
    }

    return ollamaRuntimeClient_->healthCheck();
}

QList<OllamaModelSummary> ApplicationController::currentOllamaModels() const {
    return ollamaRuntimeClient_ ? ollamaRuntimeClient_->installedModels()
                                : QList<OllamaModelSummary>{};
}

QString ApplicationController::effectiveLocalModel(const QString& requestedModel) const {
    const auto explicitModel = requestedModel.trimmed();
    if (!explicitModel.isEmpty()) {
        return explicitModel;
    }

    const auto selectedModel = selectedLocalModel_.trimmed();
    if (!selectedModel.isEmpty()) {
        return selectedModel;
    }

    const auto models = currentOllamaModels();
    return models.isEmpty() ? QString() : models.first().name;
}

bool ApplicationController::discoveredModelNamesContain(
    const QString& model, const QList<OllamaModelSummary>& models) const {
    for (const auto& discoveredModel : models) {
        if (discoveredModel.name == model) {
            return true;
        }
    }
    return false;
}

bool ApplicationController::localInferenceEndpointAllowed() const {
    const auto config = ollamaRuntimeClient_ ? ollamaRuntimeClient_->config() : OllamaConfig{};
    return config.endpoint.isLoopbackHttp();
}

LocalInferenceResponse ApplicationController::blockedLocalInferenceResponse(
    const LocalInferenceRequest& request, LocalInferenceError error, const QString& summary) const {
    LocalInferenceResponse response;
    response.status = LocalInferenceStatus::Blocked;
    response.error = error;
    response.model = request.options.model.trimmed();
    response.summary = summary;
    response.traces = {
        LocalInferenceTrace{
            1,
            QStringLiteral("Request"),
            QStringLiteral("Blocked"),
            summary,
        },
    };
    return response;
}

bool ApplicationController::clearMemory() {
    if (!memoryStore_ || !memoryStore_->isAvailable()) {
        setMemoryMaintenanceStatus(QStringLiteral("Unavailable"));
        return false;
    }

    memoryStore_->clear();
    const auto succeeded = memoryStore_->lastError().isEmpty();
    setMemoryMaintenanceStatus(succeeded ? QStringLiteral("Clear completed")
                                         : QStringLiteral("Clear failed"));
    emit memoryEntriesChanged();
    return succeeded;
}

bool ApplicationController::clearChat() {
    const auto persistentAvailable = chatHistoryStore_ && chatHistoryStore_->isAvailable();
    auto persistentHealthy = persistentAvailable;
    latestConversationClearResult_.persistentStoreAvailable = persistentAvailable;
    if (persistentAvailable) {
        chatHistoryStore_->clear();
        if (!chatHistoryStore_->lastError().isEmpty()) {
            persistentHealthy = false;
        }
    }

    chatSession_->clear();
    if (conversationStore_ && conversationStore_->status() == ConversationStoreStatus::Ready) {
        const auto record =
            conversationStore_->createConversation(QStringLiteral("Current Transcript"));
        if (!record.id.isEmpty()) {
            activeConversationId_ = record.id;
        }
    }
    const auto message = chatSession_->appendSystemMessage(QStringLiteral("Sentinel Core online."),
                                                           ChatMessageStatus::Received);
    persistActiveConversationMessage(message);
    if (persistentAvailable) {
        chatHistoryStore_->appendMessage(message);
        if (!chatHistoryStore_->lastError().isEmpty()) {
            persistentHealthy = false;
        }
    }

    if (!persistentAvailable) {
        setChatMaintenanceStatus(QStringLiteral("Runtime Only"));
        conversationHistorySummary_.lastSavedStatus =
            QStringLiteral("Runtime-only transcript; persistence unavailable.");
        latestConversationClearResult_.status = ConversationClearStatus::ClearedRuntimeOnly;
        latestConversationClearResult_.persistentStoreCleared = false;
        latestConversationClearResult_.summary =
            QStringLiteral("Runtime chat cleared and reset; no persistent chat store available.");
    } else if (persistentHealthy) {
        setChatMaintenanceStatus(QStringLiteral("Clear completed"));
        conversationHistorySummary_.lastSavedStatus =
            QStringLiteral("Cleared persisted chat history and saved initial system message.");
        latestConversationClearResult_.status = ConversationClearStatus::ClearedPersisted;
        latestConversationClearResult_.persistentStoreCleared = true;
        latestConversationClearResult_.summary =
            QStringLiteral("Runtime and persisted chat history cleared.");
    } else {
        setChatMaintenanceStatus(QStringLiteral("Clear failed"));
        conversationHistorySummary_.lastSavedStatus =
            QStringLiteral("Clear attempted; persistent chat history reported an error.");
        latestConversationClearResult_.status = ConversationClearStatus::Failed;
        latestConversationClearResult_.persistentStoreCleared = false;
        latestConversationClearResult_.summary =
            QStringLiteral("Runtime chat reset, but persistent chat clear reported an error.");
    }
    conversationHistorySummary_.lastRestoredStatus =
        QStringLiteral("Current transcript reset after clear.");
    latestConversationClearResult_.remainingMessageCount = chatSession_->messages().size();
    resetConversationRuntimeState();
    conversationStateGraph_.reset();
    refreshConversationHistorySummary();
    resetConversationSearchSummary();
    emit conversationStateChanged();
    emit conversationSearchChanged();
    emit chatMessagesChanged();
    return persistentHealthy;
}

void ApplicationController::remember(const QString& key, const QString& value) {
    if (!memoryStore_ || key.trimmed().isEmpty()) {
        return;
    }

    memoryStore_->put(key.trimmed(), value.trimmed());
    emit memoryEntriesChanged();
}

void ApplicationController::setMemoryMaintenanceStatus(const QString& status) {
    if (memoryMaintenanceStatus_ == status) {
        return;
    }
    memoryMaintenanceStatus_ = status;
    emit maintenanceStatusChanged();
}

void ApplicationController::setChatMaintenanceStatus(const QString& status) {
    if (chatMaintenanceStatus_ == status) {
        return;
    }
    chatMaintenanceStatus_ = status;
    emit maintenanceStatusChanged();
}

} // namespace sentinel::core
