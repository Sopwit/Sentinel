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
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTextStream>

#include <algorithm>
#include <cstdint>

namespace sentinel::core {

namespace {

int toInt(qsizetype value) {
    return static_cast<int>(value);
}

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

QStringList literalContextTokens(const QString& text) {
    QStringList tokens;
    const auto parts = text.toCaseFolded().split(QRegularExpression(QStringLiteral("[^a-z0-9]+")),
                                                 Qt::SkipEmptyParts);
    for (const auto& part : parts) {
        if (part.size() >= 3 && !tokens.contains(part)) {
            tokens.append(part);
        }
    }
    return tokens;
}

bool hasLiteralContextOverlap(const QString& prompt, const QString& key, const QString& value) {
    const auto promptTokens = literalContextTokens(prompt);
    if (promptTokens.isEmpty()) {
        return false;
    }

    const auto candidateTokens = literalContextTokens(QStringLiteral("%1 %2").arg(key, value));
    for (const auto& token : candidateTokens) {
        if (promptTokens.contains(token)) {
            return true;
        }
    }
    return false;
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
    if (normalized == QStringLiteral("txt") || normalized == QStringLiteral("text")) {
        return ConversationExportFormat::PlainText;
    }
    if (normalized == QStringLiteral("pdf")) {
        return ConversationExportFormat::Pdf;
    }
    return ConversationExportFormat::Unsupported;
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
    case ConversationExportFormat::Unsupported:
        break;
    case ConversationExportFormat::Markdown:
        return QStringLiteral("md");
    case ConversationExportFormat::Json:
        return QStringLiteral("json");
    case ConversationExportFormat::PlainText:
        return QStringLiteral("txt");
    case ConversationExportFormat::Pdf:
        return QStringLiteral("pdf");
    }

    return QString();
}

QString plainTextTranscript(const QList<ChatMessage>& messages, const QString& exportedAtUtc) {
    QString output;
    QTextStream stream(&output);
    stream << "Sentinel Transcript\n";
    stream << "Exported: " << exportedAtUtc << "\n";
    stream << "Messages: " << messages.size() << "\n\n";
    for (const auto& message : messages) {
        stream << chatRoleName(message.role) << " #" << message.id << "\n";
        stream << message.timestamp.toUTC().toString(Qt::ISODate) << " / "
               << chatMessageStatusName(message.status) << "\n";
        stream << message.content.trimmed() << "\n\n";
    }
    return output;
}

QByteArray pdfTranscript(const QList<ChatMessage>& messages, const QString& exportedAtUtc) {
    const auto text = plainTextTranscript(messages, exportedAtUtc)
                          .left(12000)
                          .replace(QLatin1Char('\\'), QStringLiteral("\\\\"))
                          .replace(QLatin1Char('('), QStringLiteral("\\("))
                          .replace(QLatin1Char(')'), QStringLiteral("\\)"));
    const auto lines = text.split(QLatin1Char('\n'));
    QString content;
    QTextStream contentStream(&content);
    contentStream << "BT /F1 10 Tf 48 792 Td 12 TL\n";
    for (const auto& line : lines) {
        contentStream << "(" << line.left(96) << ") Tj T*\n";
    }
    contentStream << "ET\n";

    QByteArray pdf;
    QList<int> offsets;
    auto appendObject = [&pdf, &offsets](const QByteArray& object) {
        offsets.append(pdf.size());
        pdf.append(object);
    };

    pdf.append("%PDF-1.4\n");
    appendObject("1 0 obj << /Type /Catalog /Pages 2 0 R >> endobj\n");
    appendObject("2 0 obj << /Type /Pages /Kids [3 0 R] /Count 1 >> endobj\n");
    appendObject("3 0 obj << /Type /Page /Parent 2 0 R /MediaBox [0 0 612 842] "
                 "/Resources << /Font << /F1 4 0 R >> >> /Contents 5 0 R >> endobj\n");
    appendObject("4 0 obj << /Type /Font /Subtype /Type1 /BaseFont /Helvetica >> endobj\n");
    const auto contentBytes = content.toUtf8();
    appendObject(QByteArray("5 0 obj << /Length ") + QByteArray::number(contentBytes.size()) +
                 QByteArray(" >> stream\n") + contentBytes + QByteArray("\nendstream endobj\n"));
    const auto xrefOffset = pdf.size();
    pdf.append("xref\n0 6\n0000000000 65535 f \n");
    for (const auto offset : offsets) {
        pdf.append(QByteArray::number(offset).rightJustified(10, '0'));
        pdf.append(" 00000 n \n");
    }
    pdf.append("trailer << /Size 6 /Root 1 0 R >>\nstartxref\n");
    pdf.append(QByteArray::number(xrefOffset));
    pdf.append("\n%%EOF\n");
    return pdf;
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
        if (!message.providerUsed.isEmpty() || !message.modelUsed.isEmpty() ||
            !message.roleUsed.isEmpty()) {
            QJsonObject metadata;
            metadata.insert(QStringLiteral("provider"), message.providerUsed);
            metadata.insert(QStringLiteral("model"), message.modelUsed);
            metadata.insert(QStringLiteral("role"), message.roleUsed);
            if (message.responseDurationMs >= 0) {
                metadata.insert(QStringLiteral("responseDurationMs"), message.responseDurationMs);
            }
            if (message.firstTokenLatencyMs >= 0) {
                metadata.insert(QStringLiteral("firstTokenLatencyMs"),
                                message.firstTokenLatencyMs);
            }
            if (message.approximateTokensPerSecond > 0.0) {
                metadata.insert(QStringLiteral("approxTokensPerSecond"),
                                message.approximateTokensPerSecond);
            }
            object.insert(QStringLiteral("execution"), metadata);
        }
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
    Q_UNUSED(executionAllowed);
    report.status = QStringLiteral("Blocked");
    report.summary =
        QStringLiteral("Voice runtime safety blocks Piper synthesis, file output, subprocess "
                       "execution, and playback in this readiness-only phase.");
    report.executionAllowed = false;
    report.processExecutionAllowed = false;
    report.microphoneAllowed = false;
    report.playbackAllowed = false;
    report.filesystemWideScanAllowed = false;
    report.downloadsAllowed = false;
    report.cloudAllowed = false;
    report.checks = {
        QStringLiteral("Execution: Blocked"),
        QStringLiteral("Process execution: Blocked"),
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
    Q_UNUSED(executionAllowed);
    config.processExecutionAllowed = false;
    config.fileOutputAllowed = false;
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
                         ? QStringLiteral("Piper path configuration is stored as metadata only; "
                                          "Piper synthesis execution, file output, and playback "
                                          "remain blocked.")
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
    std::unique_ptr<IConversationStore> conversationStore,
    std::unique_ptr<IAgentTaskRuntime> agentTaskRuntime, QObject* parent)
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
      agentTaskRuntime_(agentTaskRuntime ? std::move(agentTaskRuntime)
                                         : std::make_unique<StaticAgentTaskRuntime>()),
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
                    defaultDisabledPiperTtsConfig(), std::make_unique<NullPiperTtsClient>())),
      piperSynthesisClient_(std::make_unique<LocalPiperSynthesisClient>()),
      whisperTranscriptionClient_(std::make_unique<LocalWhisperTranscriptionClient>()),
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
    latestSemanticPromptInclusionResult_ = sentinel::core::includeSemanticPromptSupplements(
        latestPromptContextInjectionResult_, semanticSupplementAssemblyResult(),
        semanticPromptAuthorityResult(), semanticPromptInclusionPolicy());
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

QString ApplicationController::agentTaskRuntimeStatus() const {
    if (!agentTaskRuntime_) {
        return agentTaskRuntimeStateName(AgentTaskRuntimeState::RefusingExecution);
    }
    return agentTaskRuntimeStateName(agentTaskRuntime_->runtimeStatus().state);
}

QString ApplicationController::agentTaskRuntimeSummary() const {
    if (!agentTaskRuntime_) {
        return QStringLiteral("No agent task runtime boundary available.");
    }
    return agentTaskRuntime_->runtimeStatus().summary;
}

int ApplicationController::agentTaskRuntimeTaskCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->runtimeStatus().taskCount : 0;
}

int ApplicationController::agentTaskQueueCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->queue().summary.totalCount : 0;
}

int ApplicationController::agentTaskQueueActiveCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->queue().summary.activeCount : 0;
}

int ApplicationController::agentTaskQueuePlannedCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->queue().summary.plannedCount : 0;
}

int ApplicationController::agentTaskQueueBlockedCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->queue().summary.blockedCount : 0;
}

int ApplicationController::agentTaskQueueCompletedCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->queue().summary.completedCount : 0;
}

int ApplicationController::agentTaskQueueRefusedCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->queue().summary.refusedCount : 0;
}

QString ApplicationController::latestAgentTaskSummary() const {
    if (!agentTaskRuntime_ || agentTaskRuntime_->tasks().isEmpty()) {
        return QStringLiteral("No agent task metadata available.");
    }
    return agentTaskSummary(agentTaskRuntime_->tasks().last());
}

QString ApplicationController::latestAgentTaskLifecycleSummary() const {
    if (!agentTaskRuntime_) {
        return QStringLiteral("No agent task lifecycle metadata available.");
    }

    const auto latest = agentTaskRuntime_->queue().summary.latestLifecycleSummary;
    return latest.isEmpty() ? QStringLiteral("No agent task lifecycle metadata available.")
                            : latest;
}

QStringList ApplicationController::agentTaskQueueSummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return agentTaskQueueTaskSummaries(agentTaskRuntime_->queue());
}

QStringList ApplicationController::agentTaskTraceSummaries() const {
    if (!agentTaskRuntime_ || agentTaskRuntime_->tasks().isEmpty()) {
        return {};
    }
    return sentinel::core::agentTaskTraceSummaries(agentTaskRuntime_->tasks().last().traces);
}

QString ApplicationController::agentPlanningSessionStatus() const {
    if (!agentTaskRuntime_) {
        return agentPlanningSessionStatusName(AgentPlanningSessionStatus::Refused);
    }
    return agentPlanningSessionStatusName(agentTaskRuntime_->planningSession().status);
}

QString ApplicationController::agentPlanningSessionSummary() const {
    if (!agentTaskRuntime_) {
        return QStringLiteral("No agent planning session metadata available.");
    }
    return agentTaskRuntime_->planningSession().summary.summary;
}

int ApplicationController::agentPlanningCandidateCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->planningSession().summary.candidateCount : 0;
}

int ApplicationController::agentPlanningRefusedCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->planningSession().summary.refusedCount : 0;
}

QStringList ApplicationController::agentPlanningCandidateSummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::agentPlanningCandidateSummaries(agentTaskRuntime_->planningSession());
}

QStringList ApplicationController::agentPlanningArbitrationSummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::agentPlanningArbitrationSummaries(agentTaskRuntime_->planningSession());
}

QStringList ApplicationController::agentPlanningRefusalSummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::agentPlanningRefusalSummaries(agentTaskRuntime_->planningSession());
}

QString ApplicationController::agentPlanningFallbackSummary() const {
    if (!agentTaskRuntime_) {
        return QStringLiteral("No planning fallback metadata available.");
    }
    return sentinel::core::agentPlanningFallbackSummary(agentTaskRuntime_->planningSession());
}

QString ApplicationController::agentCapabilityRegistryStatus() const {
    if (!agentTaskRuntime_) {
        return agentCapabilityRegistryStatusName(
            AgentCapabilityRegistryStatus::RefusingUnsafeCapabilities);
    }
    return agentCapabilityRegistryStatusName(agentTaskRuntime_->capabilityRegistry().status);
}

QString ApplicationController::agentCapabilityRegistrySummary() const {
    if (!agentTaskRuntime_) {
        return QStringLiteral("No agent capability registry metadata available.");
    }
    return agentTaskRuntime_->capabilityRegistry().summary.summary;
}

int ApplicationController::agentCapabilityCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->capabilityRegistry().summary.totalCount : 0;
}

int ApplicationController::agentCapabilityEnabledCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->capabilityRegistry().summary.enabledCount : 0;
}

int ApplicationController::agentCapabilityDisabledCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->capabilityRegistry().summary.disabledCount : 0;
}

int ApplicationController::agentCapabilityRestrictedCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->capabilityRegistry().summary.restrictedCount : 0;
}

QStringList ApplicationController::agentCapabilitySummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::agentCapabilitySummaries(agentTaskRuntime_->capabilityRegistry());
}

QStringList ApplicationController::agentCapabilityReadinessSummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::agentCapabilityReadinessSummaries(
        agentTaskRuntime_->capabilityRegistry());
}

QStringList ApplicationController::agentCapabilitySafetySummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::agentCapabilitySafetySummaries(agentTaskRuntime_->capabilityRegistry());
}

QString ApplicationController::toolContractRegistryStatus() const {
    if (!agentTaskRuntime_) {
        return toolContractRegistryStatusName(ToolContractRegistryStatus::RefusingUnsafeContracts);
    }
    return toolContractRegistryStatusName(agentTaskRuntime_->toolContractRegistry().status);
}

QString ApplicationController::toolContractRegistrySummary() const {
    if (!agentTaskRuntime_) {
        return QStringLiteral("No tool contract registry metadata available.");
    }
    return agentTaskRuntime_->toolContractRegistry().summary.summary;
}

int ApplicationController::toolContractCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->toolContractRegistry().summary.totalCount : 0;
}

int ApplicationController::toolContractEnabledCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->toolContractRegistry().summary.enabledCount : 0;
}

int ApplicationController::toolContractDisabledCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->toolContractRegistry().summary.disabledCount : 0;
}

int ApplicationController::toolContractRestrictedCount() const {
    return agentTaskRuntime_ ? agentTaskRuntime_->toolContractRegistry().summary.restrictedCount
                             : 0;
}

QStringList ApplicationController::toolContractSummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::toolContractSummaries(agentTaskRuntime_->toolContractRegistry());
}

QStringList ApplicationController::toolContractPermissionSummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::toolContractPermissionSummaries(
        agentTaskRuntime_->toolContractRegistry());
}

QStringList ApplicationController::toolContractSandboxSummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::toolContractSandboxSummaries(agentTaskRuntime_->toolContractRegistry());
}

QStringList ApplicationController::toolContractReadinessSummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::toolContractReadinessSummaries(
        agentTaskRuntime_->toolContractRegistry());
}

QStringList ApplicationController::toolContractSafetySummaries() const {
    if (!agentTaskRuntime_) {
        return {};
    }
    return sentinel::core::toolContractSafetySummaries(agentTaskRuntime_->toolContractRegistry());
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

QString ApplicationController::selectedRuntimeProvider() const {
    return selectedRuntimeProvider_;
}

void ApplicationController::setSelectedRuntimeProvider(const QString& providerId) {
    const auto normalized = providerId.trimmed().toLower();
    const auto selected = (normalized == QStringLiteral("ollama") ||
                           normalized == QStringLiteral("openai-compatible-local") ||
                           normalized == QStringLiteral("lm-studio") ||
                           normalized == QStringLiteral("llama-cpp-server") ||
                           normalized == QStringLiteral("openai-compatible") ||
                           normalized == QStringLiteral("claude") ||
                           normalized == QStringLiteral("gemini"))
                              ? normalized
                              : QStringLiteral("ollama");
    if (selected == selectedRuntimeProvider_) {
        return;
    }

    selectedRuntimeProvider_ = selected;
    emit runtimeProviderRegistryChanged();
    emit localModelSelectionChanged();
    emit localChatInferenceRoutingChanged();
}

QString ApplicationController::activeRuntimeProviderId() const {
    return currentRuntimeProviderRegistry().activeProviderId();
}

QString ApplicationController::activeRuntimeProviderLabel() const {
    return currentRuntimeProviderRegistry().activeProviderDisplayName();
}

QString ApplicationController::activeRuntimeModelLabel() const {
    return currentRuntimeProviderRegistry().activeModelLabel();
}

QString ApplicationController::activeRuntimeReadinessState() const {
    return currentRuntimeProviderRegistry().activeReadinessState();
}

QString ApplicationController::activeRuntimeReadinessSummary() const {
    return currentRuntimeProviderRegistry().activeReadinessSummary();
}

QString ApplicationController::activeRuntimeLocalOnlySummary() const {
    return currentRuntimeProviderRegistry().activeLocalOnlySummary();
}

QStringList ApplicationController::selectableRuntimeProviderIds() const {
    QStringList ids;
    for (const auto& provider : currentRuntimeProviderRegistry().providers()) {
        if (provider.enabled) {
            ids.append(provider.providerId);
        }
    }
    return ids;
}

QStringList ApplicationController::selectableRuntimeProviderLabels() const {
    QStringList labels;
    for (const auto& provider : currentRuntimeProviderRegistry().providers()) {
        if (provider.enabled) {
            labels.append(provider.displayName);
        }
    }
    return labels;
}

QStringList ApplicationController::runtimeProviderCardSummaries() const {
    QStringList summaries;
    for (const auto& provider : currentRuntimeProviderRegistry().providers()) {
        summaries.append(runtimeProviderCardSummary(provider));
    }
    return summaries;
}

QStringList ApplicationController::runtimeProviderCapabilitySummaries() const {
    return sentinel::core::runtimeProviderCapabilitySummaries(
        currentRuntimeProviderRegistry().providers());
}

QStringList ApplicationController::runtimeProviderValidationTraces() const {
    return currentRuntimeProviderRegistry().validationTraceSummaries();
}

QStringList ApplicationController::installedRuntimeProviderSummaries() const {
    return currentRuntimeProviderRegistry().installedProviderSummaries();
}

QStringList ApplicationController::configuredRuntimeProviderSummaries() const {
    return currentRuntimeProviderRegistry().configuredProviderSummaries();
}

QStringList ApplicationController::availableLocalRuntimeSummaries() const {
    return currentRuntimeProviderRegistry().availableLocalRuntimeSummaries();
}

QString ApplicationController::providerCredentialRegistryStatus() const {
    return providerCredentialStatusName(currentProviderCredentialRegistry().summary().status);
}

QString ApplicationController::providerCredentialRegistrySummary() const {
    return currentProviderCredentialRegistry().summary().summary;
}

QStringList ApplicationController::providerCredentialSummaries() const {
    return currentProviderCredentialRegistry().requirementSummaries();
}

QStringList ApplicationController::providerCredentialReadinessSummaries() const {
    return currentProviderCredentialRegistry().readinessSummaries();
}

QStringList ApplicationController::providerCredentialSafetySummaries() const {
    return currentProviderCredentialRegistry().safetySummaries();
}

QString ApplicationController::credentialStoreSummary() const {
    return currentCredentialStore().summary().summary;
}

QString ApplicationController::credentialStoreBackendSummary() const {
    return currentCredentialStore().summary().backendSummary;
}

QString ApplicationController::credentialStoreSafetySummary() const {
    return currentCredentialStore().summary().safetySummary;
}

QStringList ApplicationController::credentialStoreTraceSummaries() const {
    return currentCredentialStore().traceSummaries();
}

QString ApplicationController::credentialActionReadiness() const {
    return QStringLiteral("Add API Key, Update API Key, and Remove API Key are disabled "
                          "placeholders; no credential state can be mutated.");
}

QString ApplicationController::credentialExecutionStatus() const {
    return QStringLiteral("Execution disabled: credential storage does not enable cloud/API "
                          "provider calls.");
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
    emit runtimeProviderRegistryChanged();
    emit localChatInferenceRoutingChanged();
}

QString ApplicationController::selectedLocalModelStatus() const {
    const auto models = currentOllamaModels();
    const auto selected = selectedLocalModel_.trimmed();
    if (selected.isEmpty()) {
        return QStringLiteral("Missing");
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
            return QStringLiteral("No local model selected. Choose an installed Ollama model in "
                                  "Settings before sending.");
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

QString ApplicationController::modelRegistryStatus() const {
    return modelRegistryStatusName(currentModelRegistry().summary().status);
}

QString ApplicationController::modelRegistrySummary() const {
    return currentModelRegistry().summary().summary;
}

QStringList ApplicationController::modelRegistryModelSummaries() const {
    return currentModelRegistry().modelDisplaySummaries();
}

QStringList ApplicationController::modelLibraryInstalledSummaries() const {
    return currentModelRegistry().installedModelLibrarySummaries();
}

QStringList ApplicationController::modelLibraryAvailableSummaries() const {
    return currentModelRegistry().availableModelLibrarySummaries();
}

QStringList ApplicationController::modelLibraryRecommendedSummaries() const {
    return currentModelRegistry().recommendedModelLibrarySummaries();
}

QStringList ApplicationController::modelLibraryDetailSummaries() const {
    return currentModelRegistry().modelDetailSummaries();
}

QStringList ApplicationController::providerDiscoverySummaries() const {
    const auto registry = currentRuntimeProviderRegistry();
    QStringList summaries;
    for (const auto& provider : registry.providers()) {
        const auto state = provider.enabled && provider.readiness == RuntimeReadinessState::Ready
                               ? QStringLiteral("connected")
                               : provider.enabled ? QStringLiteral("unavailable")
                                                  : QStringLiteral("disabled");
        summaries.append(QStringLiteral("%1 - %2 - endpoint %3 - selected model %4 - reason %5")
                             .arg(provider.displayName, state, provider.endpointSummary,
                                  provider.modelSummary, provider.readinessReason));
    }
    summaries.append(QStringLiteral("LM Studio - disabled - endpoint loopback only, not configured "
                                    "- selected model none - reason explicit foreground local "
                                    "endpoint checks are future-scoped."));
    summaries.append(QStringLiteral("llama.cpp server - disabled - endpoint loopback only, not "
                                    "configured - selected model none - reason no probing, "
                                    "subprocess, or filesystem scanning is enabled."));
    summaries.append(QStringLiteral("OpenAI-compatible local endpoint - disabled - endpoint "
                                    "loopback only, not configured - selected model none - reason "
                                    "local endpoint activation requires a later explicit phase."));
    return summaries;
}

QStringList ApplicationController::modelAdvisorRecommendationSummaries() const {
    return deterministicModelAdvisorRecommendations(
        ModelAdvisorInput{QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture(),
                          QStringLiteral("Unknown"), QStringLiteral("General Assistant"),
                          QStringLiteral("Balanced"), QStringLiteral("system")},
        currentModelRegistry());
}

QStringList ApplicationController::modelAdvisorAvoidSummaries() const {
    return deterministicModelAdvisorAvoidList(
        ModelAdvisorInput{QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture(),
                          QStringLiteral("Unknown"), QStringLiteral("General Assistant"),
                          QStringLiteral("Balanced"), QStringLiteral("system")});
}

QStringList ApplicationController::downloadsCenterSummaries() const {
    return downloadCenterPlaceholderSummaries(currentModelRegistry().models());
}

QStringList ApplicationController::benchmarkHubSummaries() const {
    return benchmarkHubPlaceholderSummaries(currentModelRegistry().models());
}

QStringList ApplicationController::selectedModelCapabilityLabels() const {
    return currentModelRegistry().selectedModelCapabilityLabels();
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
        safeModelManagementResultSummary(modelManagementService_->evaluate(
            ModelManagementRequest{ModelManagementAction::Refresh, model})),
        safeModelManagementResultSummary(modelManagementService_->evaluate(
            ModelManagementRequest{ModelManagementAction::Import, model})),
        safeModelManagementResultSummary(modelManagementService_->evaluate(
            ModelManagementRequest{ModelManagementAction::Export, model})),
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

VoicePipelineSessionResult ApplicationController::currentVoicePipelineSessionResult() const {
    const auto whisperReadiness = currentWhisperTranscriptionReadiness();
    VoicePipelineSessionReadiness transcription;
    transcription.step = VoicePipelineSessionStep::TranscriptionReadiness;
    transcription.ready = whisperReadiness.ready;
    transcription.status =
        whisperReadiness.ready
            ? VoicePipelineSessionStatus::ReadyMetadata
            : (whisperReadiness.status == WhisperTranscriptionStatus::UnsafePath ||
                       whisperReadiness.status == WhisperTranscriptionStatus::Refused ||
                       whisperReadiness.status == WhisperTranscriptionStatus::SafetyBlocked
                   ? VoicePipelineSessionStatus::Refused
                   : VoicePipelineSessionStatus::Blocked);
    transcription.summary =
        whisperReadiness.ready
            ? QStringLiteral("Whisper transcription readiness is available as metadata only; "
                             "Whisper execution, microphone capture, prompt injection, and chat "
                             "auto-send remain disabled.")
            : QStringLiteral("Whisper transcription readiness blocks the voice pipeline: %1. No "
                             "Whisper execution, microphone capture, transcript injection, or "
                             "chat auto-send occurs.")
                  .arg(whisperTranscriptionStatusName(whisperReadiness.status));

    const auto chatStatus = localChatInferenceStatus();
    VoicePipelineSessionReadiness chat;
    chat.step = VoicePipelineSessionStep::ChatInferenceReadiness;
    chat.ready = chatStatus == QStringLiteral("Enabled");
    chat.status = chat.ready ? VoicePipelineSessionStatus::ReadyMetadata
                             : (chatStatus == QStringLiteral("Blocked") ||
                                        chatStatus == QStringLiteral("Invalid Model")
                                    ? VoicePipelineSessionStatus::Refused
                                    : VoicePipelineSessionStatus::Blocked);
    chat.summary =
        chat.ready
            ? QStringLiteral("Local chat inference readiness is available for guarded metadata "
                             "orchestration only; voice never auto-sends a transcript.")
            : QStringLiteral("Local chat inference readiness blocks the voice pipeline: %1. %2")
                  .arg(chatStatus, localChatInferenceSummary());

    const auto piperReadiness = currentPiperSynthesisReadiness();
    VoicePipelineSessionReadiness synthesis;
    synthesis.step = VoicePipelineSessionStep::SynthesisReadiness;
    synthesis.ready = piperReadiness.ready;
    synthesis.status = piperReadiness.ready
                           ? VoicePipelineSessionStatus::ReadyMetadata
                           : (piperReadiness.status == PiperSynthesisStatus::UnsafePath ||
                                      piperReadiness.status == PiperSynthesisStatus::Refused ||
                                      piperReadiness.status == PiperSynthesisStatus::SafetyBlocked
                                  ? VoicePipelineSessionStatus::Refused
                                  : VoicePipelineSessionStatus::Blocked);
    synthesis.summary =
        piperReadiness.ready
            ? QStringLiteral("Piper synthesis readiness is available as metadata only; Piper "
                             "execution and playback remain disabled.")
            : QStringLiteral("Piper synthesis readiness blocks the voice pipeline: %1. No Piper "
                             "execution, audio generation, playback, or chat/audio injection "
                             "occurs.")
                  .arg(piperSynthesisStatusName(piperReadiness.status));

    return buildVoicePipelineSessionResult(transcription, chat, synthesis);
}

AudioFileSessionResult ApplicationController::currentAudioFileSessionResult() const {
    return buildAudioFileSessionResult();
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

QString ApplicationController::voicePipelineSessionStatus() const {
    return voicePipelineSessionStatusName(currentVoicePipelineSessionResult().status);
}

QString ApplicationController::voicePipelineSessionSummary() const {
    return voicePipelineSessionSummaryText(currentVoicePipelineSessionResult().summary);
}

QStringList ApplicationController::voicePipelineSessionStageReadinessSummaries() const {
    return voicePipelineSessionStepSummaries(currentVoicePipelineSessionResult().steps);
}

QStringList ApplicationController::voicePipelineSessionTraceSummaries() const {
    return sentinel::core::voicePipelineSessionTraceSummaries(
        currentVoicePipelineSessionResult().traces);
}

QString ApplicationController::voicePipelineSessionFallbackSummary() const {
    return sentinel::core::voicePipelineSessionFallbackSummary(
        currentVoicePipelineSessionResult().fallback);
}

QString ApplicationController::voicePipelineSessionSafetySummary() const {
    return sentinel::core::voicePipelineSessionSafetySummary(
        currentVoicePipelineSessionResult().safetyReport);
}

QStringList ApplicationController::voicePipelineSessionSafetyChecks() const {
    return sentinel::core::voicePipelineSessionSafetyChecks(
        currentVoicePipelineSessionResult().safetyReport);
}

int ApplicationController::voicePipelineSessionReadyStageCount() const {
    return currentVoicePipelineSessionResult().summary.readyStageCount;
}

int ApplicationController::voicePipelineSessionBlockedStageCount() const {
    return currentVoicePipelineSessionResult().summary.blockedStageCount;
}

QString ApplicationController::audioFileSessionStatus() const {
    return audioFileSessionStatusName(currentAudioFileSessionResult().status);
}

QString ApplicationController::audioFileSessionSummary() const {
    return audioFileSessionSummaryText(currentAudioFileSessionResult().summary);
}

QString ApplicationController::audioFileSessionReadinessSummary() const {
    return sentinel::core::audioFileSessionReadinessSummary(
        currentAudioFileSessionResult().readiness);
}

QStringList ApplicationController::audioFileValidationSummaries() const {
    return sentinel::core::audioFileValidationSummaries(
        currentAudioFileSessionResult().validations);
}

QStringList ApplicationController::audioFileSupportedExtensionSummaries() const {
    return sentinel::core::supportedAudioFileExtensionSummaries();
}

QString ApplicationController::audioFileSessionFallbackSummary() const {
    return sentinel::core::audioFileSessionFallbackSummary(
        currentAudioFileSessionResult().fallback);
}

QString ApplicationController::audioFileSessionSafetySummary() const {
    return sentinel::core::audioFileSessionSafetySummary(
        currentAudioFileSessionResult().safetyReport);
}

QStringList ApplicationController::audioFileSessionSafetyChecks() const {
    return sentinel::core::audioFileSessionSafetyChecks(
        currentAudioFileSessionResult().safetyReport);
}

QStringList ApplicationController::audioFileSessionRefusalSummaries() const {
    return sentinel::core::audioFileSessionRefusalSummaries(currentAudioFileSessionResult());
}

QStringList ApplicationController::audioFileTraceSummaries() const {
    return sentinel::core::audioFileTraceSummaries(currentAudioFileSessionResult().traces);
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
        return provider.status() == PiperTtsStatus::ReadyMetadata;
    }

    return piperTextToSpeechProvider_ &&
           piperTextToSpeechProvider_->status() == PiperTtsStatus::ReadyMetadata;
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

PiperSynthesisConfig ApplicationController::currentPiperSynthesisConfig() const {
    return configuredPiperSynthesisConfig(piperBinaryPath_, piperModelPath_);
}

PiperSynthesisRequest ApplicationController::currentPiperSynthesisRequest() const {
    return PiperSynthesisRequest{};
}

PiperSynthesisReadiness ApplicationController::currentPiperSynthesisReadiness() const {
    return piperSynthesisReadiness(currentPiperSynthesisConfig(), currentPiperSynthesisRequest());
}

QString ApplicationController::piperSynthesisStatus() const {
    return piperSynthesisStatusName(currentPiperSynthesisReadiness().status);
}

QString ApplicationController::piperSynthesisReadinessSummary() const {
    return sentinel::core::piperSynthesisReadinessSummary(currentPiperSynthesisReadiness());
}

QString ApplicationController::piperSynthesisLastSummary() const {
    if (latestPiperSynthesisResult_.status == PiperSynthesisStatus::Disabled &&
        latestPiperSynthesisResult_.traces.isEmpty()) {
        return QStringLiteral("No Piper synthesis request has run. No audio is produced, played, "
                              "or injected.");
    }
    if (!latestPiperSynthesisResult_.summary.trimmed().isEmpty()) {
        return safePiperSynthesisResultSummary(latestPiperSynthesisResult_);
    }
    return QStringLiteral("No Piper synthesis request has run. No audio is produced, played, or "
                          "injected.");
}

QString ApplicationController::piperSynthesisFallbackSummary() const {
    if (!latestPiperSynthesisResult_.fallback.summary.trimmed().isEmpty()) {
        return latestPiperSynthesisResult_.fallback.summary;
    }
    return QStringLiteral("Piper synthesis fallback is no audio; local synthesis execution is not "
                          "enabled.");
}

QString ApplicationController::piperSynthesisSafetySummary() const {
    return sentinel::core::piperSynthesisSafetySummary(
        piperSynthesisSafetyReport(currentPiperSynthesisConfig().policy));
}

QStringList ApplicationController::piperSynthesisTraceSummaries() const {
    if (!latestPiperSynthesisResult_.traces.isEmpty()) {
        return latestPiperSynthesisResult_.traces;
    }
    return currentPiperSynthesisReadiness().checks;
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
    Q_UNUSED(enabled);
    if (!piperFileOutputExecutionEnabled_) {
        return;
    }

    piperFileOutputExecutionEnabled_ = false;
    latestPiperTtsResult_ = PiperTtsResult{
        PiperTtsStatus::Disabled,
        false,
        {},
        {},
        0,
        -1,
        {},
        QStringLiteral("Piper file-output execution is disabled in this readiness-only phase."),
        {QStringLiteral("Piper execution opt-in was refused.")},
    };
    emit voiceConfigurationChanged();
}

QString ApplicationController::piperFileOutputExecutionStatus() const {
    return QStringLiteral("Disabled");
}

QString ApplicationController::piperFileOutputExecutionSummary() const {
    if (!latestPiperTtsResult_.summary.trimmed().isEmpty()) {
        return safePiperTtsResultSummary(latestPiperTtsResult_);
    }
    return QStringLiteral("Piper file-output execution is disabled. Phase 18 Piper TTS exposes "
                          "readiness, safety, fallback, and trace metadata only.");
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
    Q_UNUSED(text);
    latestPiperTtsResult_ = PiperTtsResult{
        PiperTtsStatus::Refused,
        false,
        {},
        {},
        0,
        -1,
        {},
        QStringLiteral("Piper file-output generation refused: no subprocess execution, audio "
                       "file creation, or playback is enabled in this phase."),
        {QStringLiteral("Piper file-output generation refused without side effects.")},
    };
    emit voiceConfigurationChanged();
    return false;
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

WhisperRuntimeDescriptor ApplicationController::currentWhisperRuntimeDescriptor() const {
    return whisperRuntimeDescriptorFromConfiguration(whisperBinaryPath_, whisperModelPath_);
}

PiperRuntimeDescriptor ApplicationController::currentPiperRuntimeDescriptor() const {
    return piperRuntimeDescriptorFromConfiguration(piperBinaryPath_, piperModelPath_);
}

VoiceRuntimeReadinessReport ApplicationController::currentVoiceRuntimeReadinessReport() const {
    return voiceRuntimeReadinessReport(currentWhisperRuntimeDescriptor(),
                                       currentPiperRuntimeDescriptor());
}

QString ApplicationController::voiceRuntimeReadinessSummary() const {
    return voiceRuntimeReadinessSummaryText(currentVoiceRuntimeReadinessReport());
}

QString ApplicationController::voiceRuntimeHealth() const {
    return voiceRuntimeHealthName(currentVoiceRuntimeReadinessReport().health);
}

int ApplicationController::voiceRuntimeConfiguredCount() const {
    return currentVoiceRuntimeReadinessReport().configuredCount;
}

int ApplicationController::voiceRuntimeMissingCount() const {
    return currentVoiceRuntimeReadinessReport().missingCount;
}

int ApplicationController::voiceRuntimeRefusedCount() const {
    return currentVoiceRuntimeReadinessReport().refusedCount;
}

QString ApplicationController::voiceRuntimePermissionFoundationSummary() const {
    return sentinel::core::voiceRuntimePermissionFoundationSummary();
}

QString ApplicationController::voiceRuntimeSandboxSummary() const {
    return sentinel::core::voiceRuntimeSandboxSummary(currentVoiceRuntimeReadinessReport());
}

QString ApplicationController::voiceRuntimeSafetyReportSummary() const {
    return voiceRuntimeSafetySummaryText(
        voiceRuntimeSafetyReportForReadiness(currentVoiceRuntimeReadinessReport()));
}

QStringList ApplicationController::voiceRuntimeReadinessChecks() const {
    return sentinel::core::voiceRuntimeReadinessChecks(currentVoiceRuntimeReadinessReport());
}

QString ApplicationController::whisperRuntimeStatus() const {
    return whisperRuntimeStatusName(currentWhisperRuntimeDescriptor().status);
}

QString ApplicationController::whisperRuntimeReadinessSummary() const {
    return whisperRuntimeDescriptorSummary(currentWhisperRuntimeDescriptor());
}

QString ApplicationController::whisperRuntimePathSummary() const {
    return whisperRuntimePathSummaryText(currentWhisperRuntimeDescriptor().pathSummary);
}

WhisperTranscriptionConfig ApplicationController::currentWhisperTranscriptionConfig() const {
    return configuredWhisperTranscriptionConfig(whisperBinaryPath_, whisperModelPath_);
}

WhisperTranscriptionRequest ApplicationController::currentWhisperTranscriptionRequest() const {
    return WhisperTranscriptionRequest{};
}

WhisperTranscriptionReadiness ApplicationController::currentWhisperTranscriptionReadiness() const {
    return whisperTranscriptionReadiness(currentWhisperTranscriptionConfig(),
                                         currentWhisperTranscriptionRequest());
}

QString ApplicationController::whisperTranscriptionStatus() const {
    const auto status = currentWhisperTranscriptionReadiness().status;
    return whisperTranscriptionStatusName(status);
}

QString ApplicationController::whisperTranscriptionReadinessSummary() const {
    return sentinel::core::whisperTranscriptionReadinessSummary(
        currentWhisperTranscriptionReadiness());
}

QString ApplicationController::whisperTranscriptionLastSummary() const {
    if (latestWhisperTranscriptionResult_.status == WhisperTranscriptionStatus::Disabled &&
        latestWhisperTranscriptionResult_.traces.isEmpty()) {
        return QStringLiteral("No Whisper transcription request has run. No transcript is "
                              "injected into chat.");
    }
    if (!latestWhisperTranscriptionResult_.summary.trimmed().isEmpty()) {
        return safeWhisperTranscriptionResultSummary(latestWhisperTranscriptionResult_);
    }
    return QStringLiteral("No Whisper transcription request has run. No transcript is injected "
                          "into chat.");
}

QString ApplicationController::whisperTranscriptionFallbackSummary() const {
    if (!latestWhisperTranscriptionResult_.fallback.summary.trimmed().isEmpty()) {
        return latestWhisperTranscriptionResult_.fallback.summary;
    }
    return QStringLiteral("Whisper transcription fallback is no transcript; audio-file "
                          "transcription is not enabled.");
}

QString ApplicationController::whisperTranscriptionSafetySummary() const {
    return sentinel::core::whisperTranscriptionSafetySummary(
        whisperTranscriptionSafetyReport(currentWhisperTranscriptionConfig().policy));
}

QStringList ApplicationController::whisperTranscriptionTraceSummaries() const {
    if (!latestWhisperTranscriptionResult_.traces.isEmpty()) {
        return latestWhisperTranscriptionResult_.traces;
    }
    return currentWhisperTranscriptionReadiness().checks;
}

QString ApplicationController::piperRuntimeStatus() const {
    return piperRuntimeStatusName(currentPiperRuntimeDescriptor().status);
}

QString ApplicationController::piperRuntimeReadinessSummary() const {
    return piperRuntimeDescriptorSummary(currentPiperRuntimeDescriptor());
}

QString ApplicationController::piperRuntimePathSummary() const {
    return piperRuntimePathSummaryText(currentPiperRuntimeDescriptor().pathSummary);
}

bool ApplicationController::localChatInferenceEnabled() const {
    return localChatInferenceEnabled_;
}

void ApplicationController::setLocalChatInferenceEnabled(bool enabled) {
    if (enabled == localChatInferenceEnabled_) {
        return;
    }

    localChatInferenceEnabled_ = enabled;
    emit runtimeProviderRegistryChanged();
    emit localChatInferenceRoutingChanged();
}

QString ApplicationController::localChatInferenceStatus() const {
    if (selectedRuntimeProvider_ != QStringLiteral("ollama")) {
        return QStringLiteral("Provider Disabled");
    }
    if (!localChatInferenceEnabled_) {
        return QStringLiteral("Disabled");
    }
    if (localInferenceBusy_) {
        return QStringLiteral("Runtime Busy");
    }
    if (!localInferenceEndpointAllowed()) {
        return QStringLiteral("Blocked");
    }
    const auto selected = selectedLocalModel_.trimmed();
    if (selected.isEmpty()) {
        return QStringLiteral("Missing Model");
    }
    const auto health = currentOllamaHealthCheck();
    if (health.healthStatus != OllamaHealthStatus::Healthy) {
        return health.connectionStatus == OllamaConnectionStatus::Blocked
                   ? QStringLiteral("Blocked")
                   : QStringLiteral("Ollama Unreachable");
    }
    const auto models = currentOllamaModels();
    if (models.isEmpty()) {
        return QStringLiteral("Model List Unavailable");
    }
    if (!discoveredModelNamesContain(selected, models)) {
        return QStringLiteral("Invalid Model");
    }
    return QStringLiteral("Generation Ready");
}

QString ApplicationController::localChatInferenceSummary() const {
    if (selectedRuntimeProvider_ != QStringLiteral("ollama")) {
        return QStringLiteral("Selected runtime provider is disabled for execution. Choose Local "
                              "Ollama to send.");
    }
    if (!localChatInferenceEnabled_) {
        return QStringLiteral("Local chat inference is disabled; chat stays on the local safe "
                              "provider path and no Ollama prompt is sent.");
    }
    if (localInferenceBusy_) {
        return QStringLiteral("Sentinel is waiting for the current local response to finish.");
    }
    if (!localInferenceEndpointAllowed()) {
        return QStringLiteral("Local chat inference is blocked: Ollama endpoint must be local "
                              "loopback HTTP.");
    }
    const auto selected = selectedLocalModel_.trimmed();
    if (selected.isEmpty()) {
        return QStringLiteral("Select an installed Ollama model in Settings before sending.");
    }
    const auto health = currentOllamaHealthCheck();
    if (health.healthStatus != OllamaHealthStatus::Healthy) {
        return QStringLiteral("Ollama is not reachable. Start Ollama locally, then try again.");
    }
    const auto models = currentOllamaModels();
    if (models.isEmpty()) {
        return QStringLiteral("Ollama is reachable, but no installed model list is available.");
    }
    if (!discoveredModelNamesContain(selected, models)) {
        return QStringLiteral("Local chat inference is enabled but the selected model is missing "
                              "from discovered Ollama metadata.");
    }
    return QStringLiteral("Local Ollama generation is ready.");
}

bool ApplicationController::localChatSendAvailable() const {
    if (selectedRuntimeProvider_ != QStringLiteral("ollama") || !localChatInferenceEnabled_ ||
        localInferenceBusy_ || activeConversationArchived() || !localInferenceEndpointAllowed()) {
        return false;
    }

    const auto selected = selectedLocalModel_.trimmed();
    if (selected.isEmpty()) {
        return false;
    }

    const auto health = currentOllamaHealthCheck();
    if (health.healthStatus != OllamaHealthStatus::Healthy) {
        return false;
    }

    const auto models = currentOllamaModels();
    return !models.isEmpty() && discoveredModelNamesContain(selected, models);
}

QString ApplicationController::localChatSendAvailabilitySummary() const {
    if (activeConversationArchived()) {
        return activeConversationStateSummary();
    }
    if (selectedRuntimeProvider_ != QStringLiteral("ollama")) {
        return QStringLiteral("Selected runtime provider is disabled for execution. Choose Local "
                              "Ollama to send.");
    }
    if (!localChatInferenceEnabled_) {
        return QStringLiteral("Enable Local chat inference in Settings to send with Ollama.");
    }
    if (localInferenceBusy_) {
        return QStringLiteral("Sentinel is responding. Wait for the current local request to "
                              "finish.");
    }
    if (!localInferenceEndpointAllowed()) {
        return QStringLiteral("Ollama must use a local loopback HTTP endpoint.");
    }

    const auto selected = selectedLocalModel_.trimmed();
    if (selected.isEmpty()) {
        return QStringLiteral("Select an installed Ollama model in Settings before sending.");
    }

    const auto health = currentOllamaHealthCheck();
    if (health.healthStatus != OllamaHealthStatus::Healthy) {
        return QStringLiteral("Ollama is not reachable. Start Ollama locally, then try again.");
    }

    const auto models = currentOllamaModels();
    if (models.isEmpty()) {
        return QStringLiteral("Ollama is reachable, but no installed model list is available.");
    }
    if (!discoveredModelNamesContain(selected, models)) {
        return QStringLiteral("Selected model %1 is not installed in Ollama. Choose an available "
                              "model in Settings.")
            .arg(selected);
    }

    return QStringLiteral("Ready to send with Local Ollama.");
}

QString ApplicationController::chatSendLifecycleState() const {
    return chatSendLifecycleState_;
}

QString ApplicationController::chatSendLifecycleSummary() const {
    return chatSendLifecycleSummary_;
}

bool ApplicationController::promptContextInjectionEnabled() const {
    return promptContextInjectionEnabled_;
}

void ApplicationController::setPromptContextInjectionEnabled(bool enabled) {
    if (enabled == promptContextInjectionEnabled_) {
        return;
    }

    promptContextInjectionEnabled_ = enabled;
    promptContextInjectionPolicy_.enabled = enabled;
    semanticPromptInclusionPolicy_.contextInjectionEnabled = enabled;
    promptContextInjectionPolicy_.status =
        enabled ? QStringLiteral("Enabled") : QStringLiteral("Disabled");
    promptContextInjectionPolicy_.summary =
        enabled ? QStringLiteral("Prompt context injection is enabled for explicit local Ollama "
                                 "requests with bounded local context only.")
                : QStringLiteral("Prompt context injection is disabled; local prompts are sent "
                                 "without assembled memory/context.");
    if (!enabled) {
        latestPromptContextInjectionResult_ = PromptContextInjectionResult{};
        latestSemanticPromptInclusionResult_ = SemanticPromptInclusionResult{};
    } else {
        latestPromptContextInjectionResult_ = PromptContextInjectionResult{};
        latestPromptContextInjectionResult_.policy = promptContextInjectionPolicy_;
        latestPromptContextInjectionResult_.status = PromptContextInjectionStatus::Empty;
        latestPromptContextInjectionResult_.summary =
            QStringLiteral("Prompt context injection is enabled; no local prompt has been "
                           "assembled yet.");
    }
    latestSemanticPromptInclusionResult_ = sentinel::core::includeSemanticPromptSupplements(
        latestPromptContextInjectionResult_, semanticSupplementAssemblyResult(),
        semanticPromptAuthorityResult(), semanticPromptInclusionPolicy());
    emit promptContextInjectionChanged();
    emit contextAssemblyChanged();
}

PromptContextInjectionResult ApplicationController::latestPromptContextInjectionResult() const {
    return latestPromptContextInjectionResult_;
}

bool ApplicationController::semanticPromptInclusionEnabled() const {
    return semanticPromptInclusionPolicy_.enabled;
}

void ApplicationController::setSemanticPromptInclusionEnabled(bool enabled) {
    if (enabled == semanticPromptInclusionPolicy_.enabled) {
        return;
    }

    semanticPromptInclusionPolicy_.enabled = enabled;
    semanticPromptInclusionPolicy_.contextInjectionEnabled = promptContextInjectionEnabled_;
    latestSemanticPromptInclusionResult_ = sentinel::core::includeSemanticPromptSupplements(
        latestPromptContextInjectionResult_, semanticSupplementAssemblyResult(),
        semanticPromptAuthorityResult(), semanticPromptInclusionPolicy());
    emit promptContextInjectionChanged();
    emit contextAssemblyChanged();
}

SemanticPromptInclusionPolicy ApplicationController::semanticPromptInclusionPolicy() const {
    auto policy = semanticPromptInclusionPolicy_;
    policy.contextInjectionEnabled = promptContextInjectionEnabled_;
    return policy;
}

SemanticPromptInclusionResult ApplicationController::latestSemanticPromptInclusionResult() const {
    return latestSemanticPromptInclusionResult_;
}

QString ApplicationController::promptContextInjectionStatus() const {
    return promptContextInjectionStatusName(latestPromptContextInjectionResult_.status);
}

QString ApplicationController::promptContextInjectionSummary() const {
    return latestPromptContextInjectionResult_.summary;
}

int ApplicationController::promptContextInjectedBlockCount() const {
    return latestPromptContextInjectionResult_.injectedBlockCount;
}

QString ApplicationController::promptContextSourceSummary() const {
    return latestPromptContextInjectionResult_.sourceSummary;
}

QString ApplicationController::promptContextSizeSummary() const {
    return latestPromptContextInjectionResult_.sizeSummary;
}

QString ApplicationController::promptContextUsedSummary() const {
    if (!promptContextInjectionEnabled_) {
        return QStringLiteral("Context used: disabled");
    }
    return QStringLiteral("Context used: %1 %2 / %3 chars")
        .arg(promptContextUsedMemoryCount())
        .arg(promptContextUsedMemoryCount() == 1 ? QStringLiteral("memory")
                                                 : QStringLiteral("memories"))
        .arg(latestPromptContextInjectionResult_.injectedCharacterCount);
}

int ApplicationController::promptContextUsedMemoryCount() const {
    int count = 0;
    for (const auto& block : latestPromptContextInjectionResult_.bundle.blocks) {
        if (block.source == ContextAssemblySourceKind::CommittedMemory) {
            ++count;
        }
    }
    return count;
}

QString ApplicationController::contextBudgetUsageSummary() const {
    if (latestPromptContextInjectionResult_.status == PromptContextInjectionStatus::Disabled) {
        return QStringLiteral("0 of %1 context characters used.")
            .arg(promptContextInjectionPolicy_.maxCharacters);
    }
    return latestPromptContextInjectionResult_.sizeSummary;
}

int ApplicationController::contextIncludedCandidateCount() const {
    return latestPromptContextInjectionResult_.injectedBlockCount;
}

int ApplicationController::contextExcludedCandidateCount() const {
    if (latestPromptContextInjectionResult_.originalPrompt.trimmed().isEmpty()) {
        return 0;
    }
    return conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .excludedCount;
}

QStringList ApplicationController::contextAssemblyTraceSummaries() const {
    if (latestPromptContextInjectionResult_.originalPrompt.trimmed().isEmpty()) {
        return {};
    }
    return sentinel::core::conversationSalienceTraceSummaries(
        conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt));
}

QStringList ApplicationController::promptContextBlockSummaries() const {
    return sentinel::core::promptContextBlockSummaries(latestPromptContextInjectionResult_);
}

ConversationWindowPolicy ApplicationController::conversationWindowPolicy() const {
    return conversationWindowPolicy_;
}

ConversationWindowResult ApplicationController::conversationWindowResult() const {
    return conversationWindowForPrompt({});
}

QString ApplicationController::conversationWindowStatus() const {
    return conversationWindowStatusName(conversationWindowResult().status);
}

QString ApplicationController::conversationWindowSummary() const {
    return conversationWindowResult().summary.summary;
}

QString ApplicationController::conversationWindowBudgetSummary() const {
    return conversationWindowResult().budget.summary;
}

int ApplicationController::conversationWindowBudgetCharacters() const {
    return conversationWindowPolicy_.maxCharacters;
}

int ApplicationController::conversationWindowIncludedMessageCount() const {
    return conversationWindowResult().summary.includedMessageCount;
}

int ApplicationController::conversationWindowTruncatedMessageCount() const {
    return conversationWindowResult().summary.truncatedMessageCount;
}

int ApplicationController::conversationWindowOmittedMessageCount() const {
    return conversationWindowResult().summary.omittedMessageCount;
}

ConversationSummaryPolicy ApplicationController::conversationSummaryPolicy() const {
    return conversationSummaryPolicy_;
}

ConversationSummaryResult ApplicationController::conversationSummaryResult() const {
    return conversationSummaryForPrompt({});
}

QString ApplicationController::conversationSummaryStatus() const {
    return conversationSummaryStatusName(conversationSummaryResult().status);
}

QString ApplicationController::conversationSummaryText() const {
    return conversationSummaryResult().summary;
}

QString ApplicationController::conversationSummaryBudgetSummary() const {
    return conversationSummaryResult().budget.summary;
}

int ApplicationController::conversationSummaryBudgetCharacters() const {
    return conversationSummaryPolicy_.maxCharacters;
}

int ApplicationController::conversationSummaryBlockCount() const {
    return conversationSummaryResult().window.blockCount;
}

int ApplicationController::conversationSummaryMessageCount() const {
    return conversationSummaryResult().window.summarizedMessageCount;
}

int ApplicationController::conversationSummaryOmittedMessageCount() const {
    return conversationSummaryResult().window.omittedFromSummaryCount;
}

int ApplicationController::conversationSummaryTruncatedBlockCount() const {
    return conversationSummaryResult().budget.truncatedBlockCount;
}

QStringList ApplicationController::conversationSummaryBlockSummaries() const {
    return sentinel::core::conversationSummaryBlockSummaries(conversationSummaryResult());
}

RetrievalPlanningPolicy ApplicationController::retrievalPlanningPolicy() const {
    return retrievalPlanningPolicy_;
}

RetrievalPlanningResult ApplicationController::retrievalPlanningResult() const {
    return retrievalPlanningForPrompt({});
}

QString ApplicationController::retrievalPlanningStatus() const {
    return retrievalPlanningStatusName(retrievalPlanningResult().status);
}

QString ApplicationController::retrievalPlanningSummary() const {
    return retrievalPlanningResult().summary;
}

QString ApplicationController::retrievalPlanningReadiness() const {
    const auto result = retrievalPlanningResult();
    if (result.status == RetrievalPlanningStatus::Ready ||
        result.status == RetrievalPlanningStatus::Truncated) {
        return QStringLiteral("Ready");
    }
    return retrievalPlanningStatusName(result.status);
}

QString ApplicationController::retrievalPlanningBudgetSummary() const {
    return retrievalPlanningResult().budgetSummary;
}

QString ApplicationController::retrievalPlanningSourceSummary() const {
    return retrievalPlanningResult().sourceSummary;
}

int ApplicationController::retrievalPlanningSelectedSourceCount() const {
    return retrievalPlanningResult().selectedSourceCount;
}

int ApplicationController::retrievalPlanningExcludedSourceCount() const {
    return retrievalPlanningResult().excludedSourceCount;
}

int ApplicationController::retrievalPlanningSelectedCandidateCount() const {
    return retrievalPlanningResult().selectedCandidateCount;
}

int ApplicationController::retrievalPlanningExcludedCandidateCount() const {
    return retrievalPlanningResult().excludedCandidateCount;
}

int ApplicationController::retrievalPlanningTruncatedCandidateCount() const {
    return retrievalPlanningResult().truncatedCandidateCount;
}

QStringList ApplicationController::retrievalPlanningSourceSummaries() const {
    return sentinel::core::retrievalSourceSummaries(retrievalPlanningResult());
}

QString ApplicationController::memoryRelevanceSummaryText() const {
    return memoryRelevanceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .summary;
}

QString ApplicationController::memoryRelevanceBudgetSummary() const {
    return memoryRelevanceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .budget.summary;
}

int ApplicationController::memoryRelevanceIncludedCount() const {
    return memoryRelevanceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .includedCount;
}

int ApplicationController::memoryRelevanceExcludedCount() const {
    return memoryRelevanceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .excludedCount;
}

QStringList ApplicationController::memoryRelevanceTraceSummaries() const {
    return sentinel::core::memoryRelevanceTraceSummaries(
        memoryRelevanceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt));
}

QStringList ApplicationController::memoryRelevanceExclusionSummaries() const {
    return sentinel::core::memoryRelevanceExclusionSummaries(
        memoryRelevanceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt));
}

QString ApplicationController::conversationSalienceSummaryText() const {
    return conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .summary;
}

QString ApplicationController::conversationSalienceBudgetSummary() const {
    return conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .budget.summary;
}

QString ApplicationController::conversationSalienceAllocationSummary() const {
    return conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .budget.allocationSummary;
}

int ApplicationController::conversationSalienceIncludedCount() const {
    return conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .includedCount;
}

int ApplicationController::conversationSalienceExcludedCount() const {
    return conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .excludedCount;
}

int ApplicationController::conversationSalienceTruncatedCount() const {
    return conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .truncatedCount;
}

QStringList ApplicationController::conversationSalienceTraceSummaries() const {
    return sentinel::core::conversationSalienceTraceSummaries(
        conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt));
}

QStringList ApplicationController::conversationSalienceExclusionSummaries() const {
    return sentinel::core::conversationSalienceExclusionSummaries(
        conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt));
}

ConversationCompressionSummary ApplicationController::conversationCompressionSummary() const {
    return conversationCompressionSummaryForPrompt(
        latestPromptContextInjectionResult_.originalPrompt);
}

QString ApplicationController::conversationCompressionStatus() const {
    return conversationCompressionStatusName(conversationCompressionSummary().status);
}

QString ApplicationController::conversationCompressionReadinessSummary() const {
    return conversationCompressionSummary().readiness.summary;
}

QString ApplicationController::conversationCompressionPressureSummary() const {
    const auto summary = conversationCompressionSummary();
    return QStringLiteral("%1% pressure / %2 messages / %3 chars / approx %4 tokens")
        .arg(summary.readiness.pressurePercent)
        .arg(summary.readiness.messageCount)
        .arg(summary.readiness.estimatedCharacters)
        .arg(summary.readiness.estimatedTokens);
}

int ApplicationController::conversationCompressionCandidateCount() const {
    return conversationCompressionSummary().selection.candidateCount;
}

int ApplicationController::conversationCompressionSelectedCandidateCount() const {
    return conversationCompressionSummary().selection.selectedCandidateCount;
}

QString ApplicationController::conversationCompressionFallbackReason() const {
    return conversationCompressionSummary().fallback.summary;
}

QString ApplicationController::conversationCompressionTraceSummary() const {
    return conversationCompressionSummary().trace.summary;
}

QString ApplicationController::conversationCompressionBudgetSummary() const {
    return conversationCompressionSummary().budget.summary;
}

QStringList ApplicationController::conversationCompressionCandidateSummaries() const {
    return sentinel::core::conversationCompressionCandidateSummaries(
        conversationCompressionSummary());
}

QStringList ApplicationController::conversationCompressionTraceSummaries() const {
    return sentinel::core::conversationCompressionTraceSummaries(conversationCompressionSummary());
}

ConversationSummaryResult ApplicationController::conversationSummaryGenerationResult() const {
    if (latestConversationSummaryGenerationResult_.request.explicitUserAction ||
        latestConversationSummaryGenerationResult_.summaryTimestampUtc.isValid()) {
        return latestConversationSummaryGenerationResult_;
    }
    return planConversationSummaryGenerationForActiveConversation(false);
}

bool ApplicationController::conversationSummaryAvailable() const {
    const auto result = conversationSummaryGenerationResult();
    return result.readiness.available && result.preview.available;
}

QString ApplicationController::conversationSummaryGenerationStatus() const {
    return conversationSummaryStatusName(conversationSummaryGenerationResult().status);
}

QString ApplicationController::conversationSummaryReadinessSummary() const {
    return conversationSummaryGenerationResult().readiness.summary;
}

QString ApplicationController::conversationSummaryBlockedReason() const {
    return conversationSummaryGenerationResult().readiness.blockedReason;
}

QString ApplicationController::conversationSummaryEstimatedCompressionGain() const {
    const auto result = conversationSummaryGenerationResult();
    return QStringLiteral("%1% estimated gain / messages %2-%3")
        .arg(result.estimatedReductionPercent)
        .arg(result.coveredFirstMessageIndex)
        .arg(result.coveredLastMessageIndex);
}

QString ApplicationController::conversationSummaryPreviewSummary() const {
    return conversationSummaryGenerationResult().preview.summary;
}

QString ApplicationController::conversationSummaryPersistenceSummary() const {
    if (latestConversationSummaryMetadata_.conversationId.trimmed().isEmpty()) {
        return QStringLiteral("No local summary metadata persisted.");
    }
    return latestConversationSummaryMetadata_.summary;
}

QString ApplicationController::conversationSummaryInjectionSummary() const {
    QString exclusionReason;
    const bool persistedSummaryAvailable = persistedSummaryReadyForContinuity(&exclusionReason);
    const bool injected =
        latestPromptContextInjectionResult_.bundle.blocks.end() !=
        std::find_if(latestPromptContextInjectionResult_.bundle.blocks.begin(),
                     latestPromptContextInjectionResult_.bundle.blocks.end(),
                     [](const PromptContextBlock& block) {
                         return block.source == ContextAssemblySourceKind::ConversationSummary;
                     });
    if (!promptContextInjectionEnabled_) {
        return persistedSummaryAvailable
                   ? QStringLiteral("Summary available; context injection is disabled.")
                   : QStringLiteral("Summary excluded; %1").arg(exclusionReason);
    }
    if (injected) {
        return QStringLiteral("Summary assisting continuity in local prompt context.");
    }
    return persistedSummaryAvailable
               ? QStringLiteral("Summary available but not selected within current context budget.")
               : QStringLiteral("Transcript-only fallback; %1").arg(exclusionReason);
}

QString ApplicationController::summaryContinuityStatus() const {
    const auto summary = conversationSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt);
    if (summary.activeContextUsage) {
        return QStringLiteral("Active");
    }
    if (summary.stale) {
        return QStringLiteral("Stale");
    }
    if (!promptContextInjectionEnabled_) {
        return QStringLiteral("Disabled");
    }
    return QStringLiteral("Fallback");
}

QString ApplicationController::summaryContinuityFreshnessSummary() const {
    return conversationSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .freshnessSummary;
}

QString ApplicationController::summaryContinuityCoverageSummary() const {
    return conversationSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt)
        .coverageSummary;
}

QString ApplicationController::summaryContinuityContributionSummary() const {
    const auto summary = conversationSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt);
    const bool injected =
        latestPromptContextInjectionResult_.bundle.blocks.end() !=
        std::find_if(latestPromptContextInjectionResult_.bundle.blocks.begin(),
                     latestPromptContextInjectionResult_.bundle.blocks.end(),
                     [](const PromptContextBlock& block) {
                         return block.source == ContextAssemblySourceKind::ConversationSummary;
                     });
    if (injected) {
        return QStringLiteral("Continuity contribution active: %1% estimated compression gain.")
            .arg(summary.continuityGainEstimatePercent);
    }
    return QStringLiteral("Continuity contribution inactive: %1")
        .arg(summary.fallback.reason.trimmed().isEmpty() ? persistedSummaryExclusionReason()
                                                         : summary.fallback.reason);
}

QString ApplicationController::summaryContinuityFallbackSummary() const {
    const auto summary = conversationSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt);
    if (summary.activeContextUsage) {
        return QStringLiteral("No fallback required; summary passed deterministic validation.");
    }
    return summary.fallback.summary.trimmed().isEmpty()
               ? QStringLiteral("Transcript-only fallback is active.")
               : summary.fallback.summary;
}

QString ApplicationController::summaryContinuityOrderingSummary() const {
    return QStringLiteral("Injection order: active conversation recency -> summary continuity "
                          "value -> committed memory relevance -> runtime metadata -> budget.");
}

QString ApplicationController::summaryContinuityBudgetTrace() const {
    const auto summary = conversationSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt);
    const auto salience =
        conversationSalienceSummaryForPrompt(latestPromptContextInjectionResult_.originalPrompt);
    return QStringLiteral("Continuity budget: summary %1 chars / active conversation group %2 of "
                          "%3 chars / gain %4% / replacement eligible %5.")
        .arg(summary.budget.includedCharacters)
        .arg(salience.budget.activeConversationCharacters)
        .arg(salience.budget.activeConversationBudget)
        .arg(summary.continuityGainEstimatePercent)
        .arg(summary.replacementEligible ? QStringLiteral("yes") : QStringLiteral("no"));
}

ContextDecisionSummary ApplicationController::contextDecisionSummary() const {
    const auto prompt = latestPromptContextInjectionResult_.originalPrompt;
    return sentinel::core::explainContextDecision(latestPromptContextInjectionResult_,
                                                  conversationSalienceSummaryForPrompt(prompt),
                                                  memoryRelevanceSummaryForPrompt(prompt),
                                                  conversationSummaryForPrompt(prompt));
}

bool ApplicationController::contextExplainabilityEnabled() const {
    return true;
}

QString ApplicationController::contextReasoningSummary() const {
    return contextDecisionSummary().summary;
}

QString ApplicationController::contextReasoningBudgetSummary() const {
    return contextDecisionSummary().budget.summary;
}

QString ApplicationController::contextReasoningOrderingSummary() const {
    return QStringLiteral("Ordering: %1")
        .arg(contextDecisionSummary().trace.orderingStages.join(QStringLiteral(" -> ")));
}

QString ApplicationController::contextReasoningFallbackSummary() const {
    return contextDecisionSummary().fallback.summary;
}

QStringList ApplicationController::contextReasoningContributionSummaries() const {
    return sentinel::core::contextDecisionContributionSummaries(contextDecisionSummary());
}

QStringList ApplicationController::contextReasoningInclusionHints() const {
    return sentinel::core::contextDecisionInclusionSummaries(contextDecisionSummary());
}

QStringList ApplicationController::contextReasoningExclusionHints() const {
    return sentinel::core::contextDecisionExclusionSummaries(contextDecisionSummary());
}

QStringList ApplicationController::contextReasoningDeveloperTraces() const {
    return sentinel::core::contextDecisionDeveloperTraceSummaries(contextDecisionSummary());
}

QStringList ApplicationController::conversationSummaryCandidateSegments() const {
    return sentinel::core::conversationSummarySegmentSummaries(
        conversationSummaryGenerationResult());
}

QStringList ApplicationController::conversationSummaryGenerationTraceSummaries() const {
    return sentinel::core::conversationSummaryTraceSummaries(conversationSummaryGenerationResult());
}

SemanticRetrievalPolicy ApplicationController::semanticRetrievalPolicy() const {
    return semanticRetrievalPolicy_;
}

bool ApplicationController::semanticRetrievalEnabled() const {
    return semanticRetrievalPolicy_.enabled;
}

QString ApplicationController::semanticRetrievalStatus() const {
    return semanticRetrievalStatusName(SemanticRetrievalStatus::Disabled);
}

QString ApplicationController::semanticRetrievalSummary() const {
    return semanticRetrievalPolicy_.summary;
}

QString ApplicationController::semanticReadiness() const {
    return QStringLiteral("Semantic-ready metadata only; semantic retrieval is not active.");
}

QString ApplicationController::embeddingProviderReadiness() const {
    return embeddingProviderStatusName(EmbeddingProviderStatus::NotConfigured);
}

QString ApplicationController::embeddingProviderSummary() const {
    return QStringLiteral(
        "No runtime embedding provider is configured. Deterministic fake embeddings are available "
        "to tests only.");
}

QString ApplicationController::vectorIndexReadiness() const {
    return vectorIndexStatusName(VectorIndexStatus::NotConfigured);
}

QString ApplicationController::vectorIndexSummary() const {
    return QStringLiteral(
        "No runtime vector index is configured. Indexed items: 0. Semantic ranking is disabled.");
}

int ApplicationController::vectorIndexedItemCount() const {
    return 0;
}

SemanticProviderPolicy ApplicationController::semanticProviderPolicy() const {
    return semanticProviderPolicy_;
}

SemanticProviderSelection ApplicationController::semanticProviderSelection() const {
    return sentinel::core::selectSemanticProvider(selectedSemanticProviderMode_,
                                                  semanticProviderPolicy_);
}

SemanticActivationReadiness ApplicationController::semanticActivationReadinessResult() const {
    return sentinel::core::semanticActivationReadiness(semanticProviderSelection(),
                                                       semanticProviderPolicy_);
}

QString ApplicationController::semanticProviderMode() const {
    return semanticProviderModeName(semanticProviderSelection().mode);
}

QString ApplicationController::selectedSemanticProviderName() const {
    return semanticProviderSelection().descriptor.name;
}

QString ApplicationController::semanticProviderReadiness() const {
    return semanticProviderReadinessName(semanticProviderSelection().readiness);
}

QString ApplicationController::semanticProviderHealth() const {
    return semanticProviderHealthName(semanticProviderSelection().health);
}

QString ApplicationController::semanticProviderStatusSummary() const {
    return semanticProviderSelection().summary;
}

QString ApplicationController::semanticActivationReadiness() const {
    return semanticActivationReadinessResult().status;
}

QString ApplicationController::semanticActivationSummary() const {
    return semanticActivationReadinessResult().summary;
}

QStringList ApplicationController::semanticProviderCapabilitySummaries() const {
    return semanticProviderSelection().capabilitySummaries;
}

QStringList ApplicationController::semanticActivationRequiredSteps() const {
    return semanticActivationReadinessResult().requiredSteps;
}

QStringList ApplicationController::semanticRetrievalReadinessChecks() const {
    return sentinel::core::semanticRetrievalReadinessChecks(
        semanticRetrievalPolicy_, EmbeddingProviderStatus::NotConfigured,
        VectorIndexStatus::NotConfigured, vectorIndexedItemCount());
}

SemanticCandidatePolicy ApplicationController::semanticCandidatePolicy() const {
    SemanticCandidatePolicy policy = semanticCandidatePolicy_;
    policy.maxCharacters = promptContextInjectionPolicy_.maxCharacters;
    return policy;
}

SemanticCandidateArbitration ApplicationController::semanticCandidateArbitration() const {
    return semanticCandidateArbitrationForPrompt({});
}

QString ApplicationController::semanticCandidateStatus() const {
    return semanticCandidateStatusName(semanticCandidateArbitration().status);
}

QString ApplicationController::semanticCandidateSummary() const {
    return semanticCandidateArbitration().summary;
}

QString ApplicationController::semanticCandidateBudgetSummary() const {
    return semanticCandidateArbitration().budgetSummary;
}

QString ApplicationController::semanticCandidateArbitrationSummary() const {
    const auto arbitration = semanticCandidateArbitration();
    return QStringLiteral("%1 %2").arg(arbitration.orderingSummary, arbitration.exclusionSummary);
}

int ApplicationController::semanticCandidateCount() const {
    return toInt(semanticCandidateArbitration().candidates.size());
}

int ApplicationController::semanticCandidateSelectedCount() const {
    return toInt(semanticCandidateArbitration().selectedCandidates.size());
}

int ApplicationController::semanticCandidateExcludedCount() const {
    int count = 0;
    for (const auto& candidate : semanticCandidateArbitration().candidates) {
        if (!candidate.selected) {
            ++count;
        }
    }
    return count;
}

int ApplicationController::semanticCandidateTruncatedCount() const {
    int count = 0;
    for (const auto& candidate : semanticCandidateArbitration().candidates) {
        if (candidate.truncated) {
            ++count;
        }
    }
    return count;
}

QStringList ApplicationController::semanticCandidateParticipationSummaries() const {
    return sentinel::core::semanticCandidateParticipationSummaries(semanticCandidateArbitration());
}

HybridRetrievalPolicy ApplicationController::hybridRetrievalPolicy() const {
    return hybridRetrievalPolicy_;
}

HybridRetrievalReadiness ApplicationController::hybridRetrievalReadinessResult() const {
    return sentinel::core::hybridRetrievalReadiness(hybridRetrievalPolicy_,
                                                    semanticCandidateArbitration());
}

QString ApplicationController::hybridRetrievalStatus() const {
    return hybridRetrievalStatusName(hybridRetrievalReadinessResult().status);
}

QString ApplicationController::hybridRetrievalReadiness() const {
    return QStringLiteral(
        "Ready metadata only; semantic retrieval is disabled and deterministic retrieval remains "
        "authoritative.");
}

QString ApplicationController::hybridRetrievalSummary() const {
    return hybridRetrievalReadinessResult().summary;
}

QStringList ApplicationController::hybridRetrievalReadinessChecks() const {
    return hybridRetrievalReadinessResult().checks;
}

SemanticArbitrationPolicy ApplicationController::semanticArbitrationPolicy() const {
    return semanticArbitrationPolicy_;
}

SemanticArbitrationResult ApplicationController::semanticArbitrationResult() const {
    return sentinel::core::simulateSemanticArbitration(semanticCandidateArbitration(),
                                                       semanticArbitrationPolicy_);
}

QString ApplicationController::semanticArbitrationStatus() const {
    return semanticArbitrationStatusName(semanticArbitrationResult().status);
}

QString ApplicationController::semanticArbitrationReadiness() const {
    return semanticArbitrationResult().readiness;
}

QString ApplicationController::semanticArbitrationSummary() const {
    return semanticArbitrationResult().summary;
}

QString ApplicationController::semanticArbitrationBudgetSummary() const {
    return semanticArbitrationResult().budget.summary;
}

QStringList ApplicationController::semanticArbitrationSelectionSummaries() const {
    return semanticArbitrationResult().selectionSummaries;
}

QStringList ApplicationController::semanticArbitrationChecks() const {
    return semanticArbitrationResult().checks;
}

EmbeddingRuntimePlan ApplicationController::embeddingRuntimePlanResult() const {
    return sentinel::core::embeddingRuntimePlan(semanticArbitrationResult());
}

QString ApplicationController::embeddingRuntimeReadiness() const {
    return embeddingRuntimeReadinessName(embeddingRuntimePlanResult().readiness);
}

QString ApplicationController::embeddingRuntimeSummary() const {
    return embeddingRuntimePlanResult().summary;
}

QString ApplicationController::embeddingRuntimeBudgetSummary() const {
    return embeddingRuntimePlanResult().budget.summary;
}

QStringList ApplicationController::embeddingRuntimeRequirementSummaries() const {
    return embeddingRuntimePlanResult().requirements;
}

QStringList ApplicationController::embeddingRuntimeConstraintSummaries() const {
    return embeddingRuntimePlanResult().constraints;
}

EmbeddingGenerationResult ApplicationController::isolatedEmbeddingRuntimeResult() const {
    EmbeddingGenerationResult result;
    EmbeddingIsolationPolicy isolationPolicy;
    EmbeddingGenerationPolicy generationPolicy;
    EmbeddingRuntimeSession session;

    generationPolicy.providerMode = selectedSemanticProviderMode_;
    generationPolicy.allowFakeInMemoryProvider = semanticProviderPolicy_.allowFakeInMemoryProvider;
    generationPolicy.allowLocalOllamaEmbeddingsProvider =
        semanticProviderPolicy_.allowLocalOllamaEmbeddingsProvider;
    generationPolicy.realCloudProvidersAllowed = false;
    result.readiness = sentinel::core::embeddingGenerationReadiness(
        isolationPolicy, generationPolicy, session, EmbeddingProviderStatus::NotConfigured);
    result.status = EmbeddingRuntimeStatus::Refused;
    result.health = EmbeddingRuntimeHealth::Blocked;
    result.summary =
        QStringLiteral("Isolated embedding runtime is available as readiness metadata only. No "
                       "embedding test has run in the desktop controller.");
    result.failureReason =
        QStringLiteral("Explicit local-only semantic readiness and a local/fake provider are not "
                       "active in the desktop runtime.");
    result.checks = {
        QStringLiteral("Local-only mode: yes"),
        QStringLiteral("Explicit semantic readiness gate: no"),
        QStringLiteral("Provider mode: %1")
            .arg(semanticProviderModeName(selectedSemanticProviderMode_)),
        QStringLiteral("Cloud/API providers: blocked"),
        QStringLiteral("Filesystem indexing: disabled"),
        QStringLiteral("Prompt integration: disabled"),
        QStringLiteral("Retrieval ranking mutation: disabled"),
        QStringLiteral("Automatic memory writes: disabled"),
        QStringLiteral("Vector DB persistence: disabled"),
        QStringLiteral("Background indexing jobs: disabled"),
    };
    return result;
}

QString ApplicationController::isolatedEmbeddingRuntimeStatus() const {
    return embeddingRuntimeStatusName(isolatedEmbeddingRuntimeResult().status);
}

QString ApplicationController::isolatedEmbeddingRuntimeHealth() const {
    return embeddingRuntimeHealthName(isolatedEmbeddingRuntimeResult().health);
}

QString ApplicationController::isolatedEmbeddingRuntimeReadiness() const {
    return embeddingGenerationReadinessName(isolatedEmbeddingRuntimeResult().readiness);
}

QString ApplicationController::isolatedEmbeddingRuntimeSummary() const {
    return isolatedEmbeddingRuntimeResult().summary;
}

QString ApplicationController::isolatedEmbeddingRuntimeBoundedState() const {
    const auto result = isolatedEmbeddingRuntimeResult();
    return QStringLiteral("local-only / timeout %1 ms / %2 bounded docs / %3 chars / no vectors "
                          "persisted")
        .arg(result.session.timeoutMs)
        .arg(result.session.boundedDocumentCount)
        .arg(result.session.boundedCharacterCount);
}

QStringList ApplicationController::isolatedEmbeddingRuntimeChecks() const {
    return isolatedEmbeddingRuntimeResult().checks;
}

VectorPersistencePolicy ApplicationController::vectorPersistencePolicy() const {
    return vectorPersistencePolicy_;
}

VectorIndexSnapshotSummary ApplicationController::vectorIndexSnapshotSummary() const {
    LocalVectorPersistenceIndex index{vectorPersistencePolicy_};
    return index.snapshot();
}

QString ApplicationController::vectorPersistenceStatus() const {
    return vectorPersistenceStatusName(vectorIndexSnapshotSummary().status);
}

QString ApplicationController::vectorPersistenceHealth() const {
    return vectorPersistenceHealthName(vectorIndexSnapshotSummary().health);
}

QString ApplicationController::vectorPersistenceReadiness() const {
    return vectorPersistenceReadinessName(sentinel::core::vectorPersistenceReadiness(
        vectorPersistencePolicy_, VectorPersistenceSession{}));
}

QString ApplicationController::vectorPersistenceSummary() const {
    return QStringLiteral(
        "Local vector persistence is disabled by default. Lifecycle metadata is local-only, "
        "bounded, isolated, and cannot change retrieval planning or prompt assembly.");
}

QString ApplicationController::vectorPersistenceBoundedState() const {
    return vectorIndexSnapshotSummary().boundedState;
}

int ApplicationController::vectorPersistenceIndexedItemCount() const {
    return vectorIndexSnapshotSummary().indexedItemCount;
}

QStringList ApplicationController::vectorPersistenceChecks() const {
    return vectorIndexSnapshotSummary().checks;
}

SemanticSearchPolicy ApplicationController::semanticSearchPolicy() const {
    return semanticSearchPolicy_;
}

SemanticSearchResult ApplicationController::semanticSearchResult() const {
    LocalVectorPersistenceIndex index{vectorPersistencePolicy_};
    SemanticSearchSession session;
    session.requestId = QStringLiteral("semantic-search-desktop-readiness");
    session.timeoutMs = 1000;
    return index.searchLocalSemanticCandidates(QStringLiteral("desktop semantic readiness"),
                                               isolatedEmbeddingRuntimeResult(),
                                               semanticSearchPolicy_, session);
}

QString ApplicationController::semanticSearchStatus() const {
    return semanticSearchStatusName(semanticSearchResult().status);
}

QString ApplicationController::semanticSearchReadiness() const {
    return semanticSearchResult().readiness.summary;
}

QString ApplicationController::semanticSearchSummary() const {
    return semanticSearchResult().summary;
}

QString ApplicationController::semanticSearchBudgetSummary() const {
    return semanticSearchResult().budget.summary;
}

QString ApplicationController::semanticSearchRuntimeState() const {
    const auto result = semanticSearchResult();
    return QStringLiteral("local-only / timeout %1 ms / %2 candidates / non-authoritative")
        .arg(result.budget.timeoutMs)
        .arg(result.budget.returnedCandidateCount);
}

int ApplicationController::semanticSearchCandidateCount() const {
    return toInt(semanticSearchResult().candidates.size());
}

QString ApplicationController::semanticSearchArbitrationSummary() const {
    return sentinel::core::semanticSearchArbitrationSummary(semanticSearchResult(),
                                                            semanticCandidateArbitration())
        .summary;
}

QStringList ApplicationController::semanticSearchCandidateSummaries() const {
    QStringList summaries;
    for (const auto& candidate : semanticSearchResult().candidates) {
        summaries.append(QStringLiteral("%1. %2 / similarity %3")
                             .arg(candidate.rank)
                             .arg(candidate.summary)
                             .arg(candidate.similarity, 0, 'f', 3));
    }
    return summaries;
}

QStringList ApplicationController::semanticSearchChecks() const {
    const auto result = semanticSearchResult();
    auto checks = result.checks;
    checks.append(
        sentinel::core::semanticSearchArbitrationSummary(result, semanticCandidateArbitration())
            .checks);
    return checks;
}

HybridRetrievalBridgePolicy ApplicationController::hybridRetrievalBridgePolicy() const {
    return hybridRetrievalBridgePolicy_;
}

HybridRetrievalBridgeResult ApplicationController::hybridRetrievalBridgeResult() const {
    return sentinel::core::hybridRetrievalBridge(retrievalPlanningResult(), semanticSearchResult(),
                                                 hybridRetrievalBridgePolicy_);
}

SemanticAcceptancePolicy ApplicationController::semanticAcceptancePolicy() const {
    return semanticAcceptancePolicy_;
}

SemanticAcceptanceResult ApplicationController::semanticAcceptanceResult() const {
    const auto searchResult = semanticSearchResult();
    const auto bridgeResult = sentinel::core::hybridRetrievalBridge(
        retrievalPlanningResult(), searchResult, hybridRetrievalBridgePolicy_);
    return sentinel::core::semanticAcceptance(retrievalPlanningResult(), bridgeResult, searchResult,
                                              semanticAcceptancePolicy_);
}

SemanticSupplementAssemblyPolicy ApplicationController::semanticSupplementAssemblyPolicy() const {
    return semanticSupplementAssemblyPolicy_;
}

SemanticSupplementAssemblyResult ApplicationController::semanticSupplementAssemblyResult() const {
    return sentinel::core::assembleSemanticSupplements(semanticAcceptanceResult(),
                                                       semanticSupplementAssemblyPolicy_);
}

SemanticPromptAuthorityPolicy ApplicationController::semanticPromptAuthorityPolicy() const {
    return semanticPromptAuthorityPolicy_;
}

SemanticPromptAuthorityResult ApplicationController::semanticPromptAuthorityResult() const {
    return sentinel::core::evaluateSemanticPromptAuthority(semanticSupplementAssemblyResult(),
                                                           semanticPromptAuthorityPolicy_);
}

QString ApplicationController::semanticAcceptanceStatus() const {
    return semanticAcceptanceStatusName(semanticAcceptanceResult().status);
}

QString ApplicationController::semanticAcceptanceReadiness() const {
    return semanticAcceptanceResult().readiness.summary;
}

QString ApplicationController::semanticAcceptanceSummary() const {
    return semanticAcceptanceResult().summary;
}

QString ApplicationController::semanticAcceptanceBudgetSummary() const {
    return semanticAcceptanceResult().budget.summary;
}

QString ApplicationController::semanticAcceptanceSourceSummary() const {
    return semanticAcceptanceResult().sourceSummary.summary;
}

QString ApplicationController::semanticAcceptanceArbitrationSummary() const {
    return semanticAcceptanceResult().arbitration.summary;
}

QString ApplicationController::semanticAcceptanceFallbackSummary() const {
    return semanticAcceptanceResult().fallback.summary;
}

int ApplicationController::semanticAcceptanceAcceptedCount() const {
    return toInt(semanticAcceptanceResult().acceptedCandidates.size());
}

int ApplicationController::semanticAcceptanceBudgetCharacters() const {
    return semanticAcceptanceResult().budget.maxSupplementCharacters;
}

QStringList ApplicationController::semanticAcceptanceCandidateSummaries() const {
    QStringList summaries;
    for (const auto& candidate : semanticAcceptanceResult().acceptedCandidates) {
        summaries.append(QStringLiteral("%1. semantic supplement / %2 / %3 chars")
                             .arg(candidate.supplementRank)
                             .arg(candidate.title)
                             .arg(candidate.estimatedCharacters));
    }
    return summaries;
}

QStringList ApplicationController::semanticAcceptanceChecks() const {
    return semanticAcceptanceResult().checks;
}

QString ApplicationController::semanticSupplementAssemblyStatus() const {
    return semanticSupplementAssemblyStatusName(semanticSupplementAssemblyResult().status);
}

QString ApplicationController::semanticSupplementAssemblyReadiness() const {
    return semanticSupplementAssemblyResult().readiness.summary;
}

QString ApplicationController::semanticSupplementAssemblySummary() const {
    return semanticSupplementAssemblyResult().summary;
}

QString ApplicationController::semanticSupplementAssemblyBudgetSummary() const {
    return semanticSupplementAssemblyResult().budget.summary;
}

QString ApplicationController::semanticSupplementAssemblySafetySummary() const {
    return semanticSupplementAssemblyResult().safety.summary;
}

int ApplicationController::semanticSupplementAssemblyBlockCount() const {
    return semanticSupplementAssemblyResult().bundle.blockCount;
}

int ApplicationController::semanticSupplementAssemblyBudgetCharacters() const {
    return semanticSupplementAssemblyResult().budget.maxCharacters;
}

QStringList ApplicationController::semanticSupplementAssemblyChecks() const {
    return semanticSupplementAssemblyResult().checks;
}

QString ApplicationController::semanticPromptAuthorityStatus() const {
    return semanticPromptAuthorityStatusName(semanticPromptAuthorityResult().status);
}

QString ApplicationController::semanticPromptAuthorityDecisionSummary() const {
    return semanticPromptAuthorityResult().audit.decisionSummary;
}

QString ApplicationController::semanticPromptAuthoritySafetySummary() const {
    return semanticPromptAuthorityResult().safety.summary;
}

QString ApplicationController::semanticPromptAuthorityReadinessSummary() const {
    return semanticPromptAuthorityResult().readiness.summary;
}

QString ApplicationController::semanticPromptAuthorityFallbackSummary() const {
    return semanticPromptAuthorityResult().fallback.summary;
}

QString ApplicationController::semanticPromptAuthorityAuditSummary() const {
    const auto result = semanticPromptAuthorityResult();
    return result.audit.denialReason.isEmpty() ? result.audit.decisionSummary
                                               : result.audit.denialReason;
}

int ApplicationController::semanticPromptAuthorityWouldIncludeBlockCount() const {
    return semanticPromptAuthorityResult().wouldIncludeBlockCount;
}

QStringList ApplicationController::semanticPromptAuthorityChecks() const {
    return semanticPromptAuthorityResult().checks;
}

QString ApplicationController::semanticPromptInclusionStatus() const {
    return semanticPromptInclusionStatusName(latestSemanticPromptInclusionResult_.status);
}

QString ApplicationController::semanticPromptInclusionSummary() const {
    return latestSemanticPromptInclusionResult_.summary;
}

int ApplicationController::semanticPromptInclusionIncludedCount() const {
    return latestSemanticPromptInclusionResult_.budget.includedSupplementBlocks;
}

QString ApplicationController::semanticPromptInclusionBudgetSummary() const {
    return latestSemanticPromptInclusionResult_.budget.summary;
}

QString ApplicationController::semanticPromptInclusionFallbackSummary() const {
    return latestSemanticPromptInclusionResult_.fallback.summary;
}

QString ApplicationController::semanticPromptInclusionAuditSummary() const {
    return latestSemanticPromptInclusionResult_.audit.decisionSummary;
}

bool ApplicationController::semanticPromptInclusionDeterministicAuthorityPreserved() const {
    return latestSemanticPromptInclusionResult_.safety.deterministicRetrievalAuthoritative;
}

QStringList ApplicationController::semanticPromptInclusionChecks() const {
    return latestSemanticPromptInclusionResult_.checks;
}

QString ApplicationController::hybridBridgeStatus() const {
    return hybridRetrievalBridgeStatusName(hybridRetrievalBridgeResult().status);
}

QString ApplicationController::hybridBridgeReadiness() const {
    return hybridRetrievalBridgeResult().readiness.summary;
}

QString ApplicationController::hybridBridgeSummary() const {
    return hybridRetrievalBridgeResult().summary;
}

QString ApplicationController::hybridBridgeBudgetSummary() const {
    return hybridRetrievalBridgeResult().budget.summary;
}

QString ApplicationController::hybridBridgeSourceSummary() const {
    return hybridRetrievalBridgeResult().sourceSummary.summary;
}

QString ApplicationController::hybridBridgeArbitrationSummary() const {
    return hybridRetrievalBridgeResult().arbitration.summary;
}

QString ApplicationController::hybridBridgeFallbackSummary() const {
    return hybridRetrievalBridgeResult().fallbackSummary;
}

int ApplicationController::hybridBridgeCandidateCount() const {
    return toInt(hybridRetrievalBridgeResult().candidates.size());
}

int ApplicationController::hybridBridgeSemanticFillCount() const {
    return hybridRetrievalBridgeResult().budget.semanticFillCount;
}

QStringList ApplicationController::hybridBridgeCandidateSummaries() const {
    QStringList summaries;
    for (const auto& candidate : hybridRetrievalBridgeResult().candidates) {
        summaries.append(QStringLiteral("%1. %2 / %3 / %4")
                             .arg(candidate.rank)
                             .arg(candidate.deterministic ? QStringLiteral("deterministic")
                                                          : QStringLiteral("semantic advisory"))
                             .arg(candidate.source, candidate.title));
    }
    return summaries;
}

QStringList ApplicationController::hybridBridgeChecks() const {
    return hybridRetrievalBridgeResult().checks;
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

int ApplicationController::localInferenceTimeoutMs() const {
    return localInferenceTimeoutMs_;
}

void ApplicationController::setLocalInferenceTimeoutMs(int timeoutMs) {
    const auto normalized = std::clamp(timeoutMs, 1000, 300000);
    if (normalized == localInferenceTimeoutMs_) {
        return;
    }

    localInferenceTimeoutMs_ = normalized;
    emit localInferenceChanged();
    emit localChatInferenceRoutingChanged();
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
    if (latestLocalInferenceResponse_.latencyMs < 0) {
        return QStringLiteral("No local inference latency recorded.");
    }

    QStringList parts{
        QStringLiteral("Total: %1 ms").arg(latestLocalInferenceResponse_.latencyMs),
    };
    if (latestLocalInferenceResponse_.firstTokenLatencyMs >= 0) {
        parts.append(QStringLiteral("First token: %1 ms")
                         .arg(latestLocalInferenceResponse_.firstTokenLatencyMs));
    }
    if (latestLocalInferenceResponse_.approximateOutputTokens > 0) {
        parts.append(QStringLiteral("Approx: %1 tokens, %2 tok/s")
                         .arg(latestLocalInferenceResponse_.approximateOutputTokens)
                         .arg(QString::number(
                             latestLocalInferenceResponse_.approximateTokensPerSecond, 'f', 1)));
    }
    return parts.join(QStringLiteral(" / "));
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
    auto records =
        conversationStore_ ? conversationStore_->listConversations() : QList<ConversationRecord>{};
    std::stable_sort(records.begin(), records.end(), [this](const auto& lhs, const auto& rhs) {
        const auto rank = [](const ConversationRecord& record) {
            if (record.archived) {
                return 2;
            }
            return record.pinned ? 0 : 1;
        };
        const auto lhsRank = rank(lhs);
        const auto rhsRank = rank(rhs);
        if (lhsRank != rhsRank) {
            return lhsRank < rhsRank;
        }
        if (lhs.updatedAtUtc != rhs.updatedAtUtc) {
            return lhs.updatedAtUtc > rhs.updatedAtUtc;
        }
        const auto titleCompare = QString::localeAwareCompare(lhs.title, rhs.title);
        if (titleCompare != 0) {
            return titleCompare < 0;
        }
        return lhs.id < rhs.id;
    });
    return records;
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
        latestConversationSummaryMetadata_ =
            conversationStore_->loadSummaryMetadata(activeConversationId_);
        return;
    }

    for (const auto& message : chatSession_->messages()) {
        persistActiveConversationMessage(message);
    }
    latestConversationSummaryMetadata_ =
        conversationStore_->loadSummaryMetadata(activeConversationId_);
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
    latestConversationSummaryGenerationResult_ = {};
    latestConversationSummaryMetadata_ =
        conversationStore_->loadSummaryMetadata(activeConversationId_);
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
    return conversationStore_ ? toInt(conversationStore_->listConversations().size()) : 0;
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

QStringList ApplicationController::conversationPinnedSummaries() const {
    QStringList summaries;
    for (const auto& record : conversationRecords()) {
        summaries.append(record.pinned ? QStringLiteral("Pinned") : QStringLiteral("Unpinned"));
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

ConversationDuplicateResult ApplicationController::latestConversationDuplicateResult() const {
    return latestConversationDuplicateResult_;
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

QString ApplicationController::conversationDuplicateLastStatus() const {
    return latestConversationDuplicateResult_.status;
}

QString ApplicationController::conversationDuplicateLastResultSummary() const {
    return latestConversationDuplicateResult_.summary;
}

ConversationDeletePolicy ApplicationController::conversationDeletePolicy() const {
    return conversationDeletePolicy_;
}

ConversationDeleteReadiness ApplicationController::conversationDeleteReadiness() const {
    ConversationDeleteReadiness readiness;
    readiness.policy = conversationDeletePolicy_;
    if (!conversationStore_ || conversationStore_->status() != ConversationStoreStatus::Ready) {
        readiness.summary =
            QStringLiteral("Permanent delete is not enabled yet. Archive is available when the "
                           "conversation store is ready.");
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
    for (const auto& [key, value] : currentMemoryEntries()) {
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
    return toInt(memoryCandidates().size());
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

int ApplicationController::archivedMemoryCandidateCount() const {
    return memoryCandidateCountForState(MemoryReviewState::Archived);
}

int ApplicationController::committedMemoryCandidateCount() const {
    int count = 0;
    for (const auto& candidate : memoryCandidates()) {
        if (candidate.commitStatus == MemoryCommitStatus::Committed) {
            ++count;
        }
    }
    return count;
}

QStringList ApplicationController::memoryCandidateIds() const {
    QStringList result;
    for (const auto& candidate : memoryCandidates()) {
        result.append(candidate.id);
    }
    return result;
}

QStringList ApplicationController::memoryCandidateReviewStates() const {
    QStringList result;
    for (const auto& candidate : memoryCandidates()) {
        result.append(memoryReviewStateName(candidate.reviewState));
    }
    return result;
}

QStringList ApplicationController::memoryCandidateCommitStatuses() const {
    QStringList result;
    for (const auto& candidate : memoryCandidates()) {
        result.append(memoryCommitStatusName(candidate.commitStatus));
    }
    return result;
}

QStringList ApplicationController::memoryCandidateSummaries() const {
    QStringList result;
    for (const auto& candidate : memoryCandidates()) {
        const auto summary = memoryCandidateSummary(candidate);
        result.append(QStringLiteral("%1: %2").arg(summary.id, summary.summary));
    }
    return result;
}

QStringList ApplicationController::memoryCandidateSummariesForState(MemoryReviewState state) const {
    QStringList result;
    for (const auto& candidate : memoryCandidates()) {
        if (candidate.reviewState != state) {
            continue;
        }
        const auto summary = memoryCandidateSummary(candidate);
        result.append(QStringLiteral("%1: %2").arg(summary.id, summary.summary));
    }
    return result;
}

QStringList ApplicationController::pendingMemoryCandidateSummaries() const {
    return memoryCandidateSummariesForState(MemoryReviewState::PendingReview);
}

QStringList ApplicationController::approvedMemoryCandidateSummaries() const {
    return memoryCandidateSummariesForState(MemoryReviewState::Approved);
}

QStringList ApplicationController::rejectedMemoryCandidateSummaries() const {
    return memoryCandidateSummariesForState(MemoryReviewState::Rejected);
}

QStringList ApplicationController::archivedMemoryCandidateSummaries() const {
    return memoryCandidateSummariesForState(MemoryReviewState::Archived);
}

MemoryCandidateReviewResult ApplicationController::latestMemoryCandidateReviewResult() const {
    return latestMemoryCandidateReviewResult_;
}

QString ApplicationController::lastMemoryCandidateReviewStatus() const {
    return latestMemoryCandidateReviewResult_.status.isEmpty()
               ? QStringLiteral("No Review")
               : latestMemoryCandidateReviewResult_.status;
}

QString ApplicationController::lastMemoryCandidateReviewSummary() const {
    return latestMemoryCandidateReviewResult_.summary.isEmpty()
               ? QStringLiteral("No memory candidate review action yet.")
               : latestMemoryCandidateReviewResult_.summary;
}

MemoryCommitPolicy ApplicationController::memoryCommitPolicy() const {
    return memoryCommitPolicy_;
}

const MemoryCandidate*
ApplicationController::findMemoryCandidate(const QList<MemoryCandidate>& candidates,
                                           const QString& candidateId) const {
    const auto trimmed = candidateId.trimmed();
    for (const auto& candidate : candidates) {
        if (candidate.id == trimmed) {
            return &candidate;
        }
    }
    return nullptr;
}

MemoryCommitReadiness
ApplicationController::memoryCommitReadinessForCandidateId(const QString& candidateId) const {
    const auto candidates = memoryCandidates();
    const auto* candidate = findMemoryCandidate(candidates, candidateId);
    const auto storeAvailable = memoryStore_ && memoryStore_->isAvailable();
    return memoryCommitReadinessForCandidate(candidate, storeAvailable, memoryCommitPolicy_);
}

MemoryCommitReadiness ApplicationController::memoryCommitReadiness() const {
    const auto candidates = memoryCandidates();
    for (const auto& candidate : candidates) {
        if (candidate.reviewState == MemoryReviewState::Approved) {
            const auto storeAvailable = memoryStore_ && memoryStore_->isAvailable();
            return memoryCommitReadinessForCandidate(&candidate, storeAvailable,
                                                     memoryCommitPolicy_);
        }
    }

    if (!candidates.isEmpty()) {
        const auto storeAvailable = memoryStore_ && memoryStore_->isAvailable();
        return memoryCommitReadinessForCandidate(&candidates.first(), storeAvailable,
                                                 memoryCommitPolicy_);
    }

    MemoryCommitReadiness readiness;
    readiness.status = MemoryCommitReadinessStatus::MissingCandidate;
    readiness.summary = QStringLiteral(
        "Memory commit pending: no approved candidates are available for commit planning.");
    readiness.checks.append(QStringLiteral("Candidate: none"));
    readiness.checks.append(QStringLiteral("Commit: approved candidate required"));
    return readiness;
}

MemoryCommitResult ApplicationController::latestMemoryCommitResult() const {
    return latestMemoryCommitResult_;
}

QString ApplicationController::memoryCommitReadinessStatus() const {
    return memoryCommitReadinessStatusName(memoryCommitReadiness().status);
}

QString ApplicationController::memoryCommitReadinessSummary() const {
    return memoryCommitReadiness().summary;
}

QStringList ApplicationController::memoryCommitReadinessChecks() const {
    return memoryCommitReadiness().checks;
}

int ApplicationController::memoryCommitPlanCount() const {
    return approvedMemoryCandidateCount();
}

QString ApplicationController::memoryCommitTargetSummary() const {
    return QStringLiteral("%1 / %2 / %3")
        .arg(memoryCommitTargetName(memoryCommitPolicy_.target),
             memoryCommitConflictPolicyName(memoryCommitPolicy_.conflictPolicy),
             memoryCommitPolicy_.summary);
}

QStringList ApplicationController::memoryCommitCandidateSummaries() const {
    QStringList summaries;
    const auto candidates = memoryCandidates();
    const auto storeAvailable = memoryStore_ && memoryStore_->isAvailable();
    for (const auto& candidate : candidates) {
        const auto readiness =
            memoryCommitReadinessForCandidate(&candidate, storeAvailable, memoryCommitPolicy_);
        summaries.append(QStringLiteral("%1: %2 - %3")
                             .arg(candidate.id, memoryCommitReadinessStatusName(readiness.status),
                                  readiness.plan.summary.isEmpty() ? readiness.summary
                                                                   : readiness.plan.summary));
    }
    return summaries;
}

QString ApplicationController::lastMemoryCommitStatus() const {
    return latestMemoryCommitResult_.status.isEmpty() ? QStringLiteral("No Commit")
                                                      : latestMemoryCommitResult_.status;
}

QString ApplicationController::lastMemoryCommitResultSummary() const {
    return latestMemoryCommitResult_.summary.isEmpty()
               ? QStringLiteral("No memory commit request yet.")
               : latestMemoryCommitResult_.summary;
}

MemoryRecallPolicy ApplicationController::memoryRecallPolicy() const {
    return memoryRecallPolicy_;
}

MemoryRecallSummary ApplicationController::latestMemoryRecallSummary() const {
    return latestMemoryRecallSummary_;
}

ContextAssemblyPolicy ApplicationController::contextAssemblyPolicy() const {
    return contextAssemblyPolicy_;
}

ContextAssemblySource
ApplicationController::contextAssemblySource(ContextAssemblySourceKind kind) const {
    switch (kind) {
    case ContextAssemblySourceKind::Conversation: {
        int estimatedSize = 0;
        for (const auto& message : chatSession_->messages()) {
            estimatedSize += toInt(message.content.simplified().size());
        }
        return makeContextAssemblySource(kind, contextAssemblyPolicy_.includeConversationContext,
                                         !chatSession_->messages().isEmpty(),
                                         toInt(chatSession_->messages().size()), estimatedSize,
                                         QStringLiteral("%1 active transcript messages available.")
                                             .arg(chatSession_->messages().size()));
    }
    case ContextAssemblySourceKind::ConversationSummary: {
        const auto summary = conversationSummaryResult();
        return makeContextAssemblySource(
            kind, contextAssemblyPolicy_.includeConversationContext, !summary.blocks.isEmpty(),
            toInt(summary.blocks.size()), summary.budget.includedCharacters,
            QStringLiteral("%1 deterministic local summary blocks available.")
                .arg(summary.blocks.size()));
    }
    case ContextAssemblySourceKind::CommittedMemory: {
        const auto entries = currentMemoryEntries();
        int estimatedSize = 0;
        for (const auto& entry : entries) {
            estimatedSize +=
                toInt(entry.first.simplified().size() + entry.second.simplified().size());
        }
        return makeContextAssemblySource(
            kind, contextAssemblyPolicy_.includeCommittedMemoryContext, !entries.isEmpty(),
            toInt(entries.size()), estimatedSize,
            QStringLiteral("%1 committed key-value memory entries available.").arg(entries.size()));
    }
    case ContextAssemblySourceKind::RuntimeMetadata: {
        const auto runtimeSummary = conversationRuntimeSummary();
        return makeContextAssemblySource(
            kind, contextAssemblyPolicy_.includeRuntimeMetadataContext,
            !runtimeSummary.trimmed().isEmpty(), 1, toInt(runtimeSummary.simplified().size()),
            QStringLiteral("Conversation runtime metadata is available."));
    }
    case ContextAssemblySourceKind::Orchestration: {
        const auto orchestrationSummary = orchestrationSnapshotSummary();
        return makeContextAssemblySource(
            kind, contextAssemblyPolicy_.includeOrchestrationContext,
            !orchestrationSummary.trimmed().isEmpty(), 1,
            toInt(orchestrationSummary.simplified().size()),
            QStringLiteral("Orchestration snapshot metadata is available."));
    }
    case ContextAssemblySourceKind::SelectedConversationMetadata: {
        const auto summary = activeConversationSummary();
        return makeContextAssemblySource(
            kind, contextAssemblyPolicy_.includeSelectedConversationMetadata,
            !summary.trimmed().isEmpty(), 1, toInt(summary.simplified().size()),
            QStringLiteral("Selected conversation metadata is available."));
    }
    }

    return {};
}

ContextAssemblySummary ApplicationController::contextAssemblySummary() const {
    ContextAssemblyRequest request;
    request.includeConversationContext = contextAssemblyPolicy_.includeConversationContext;
    request.includeCommittedMemoryContext = contextAssemblyPolicy_.includeCommittedMemoryContext;
    request.includeRuntimeMetadataContext = contextAssemblyPolicy_.includeRuntimeMetadataContext;
    request.includeOrchestrationContext = contextAssemblyPolicy_.includeOrchestrationContext;
    request.includeSelectedConversationMetadata =
        contextAssemblyPolicy_.includeSelectedConversationMetadata;
    return contextAssemblySummaryForRequest(
        request,
        {
            contextAssemblySource(ContextAssemblySourceKind::Conversation),
            contextAssemblySource(ContextAssemblySourceKind::ConversationSummary),
            contextAssemblySource(ContextAssemblySourceKind::CommittedMemory),
            contextAssemblySource(ContextAssemblySourceKind::RuntimeMetadata),
            contextAssemblySource(ContextAssemblySourceKind::Orchestration),
            contextAssemblySource(ContextAssemblySourceKind::SelectedConversationMetadata),
        },
        contextAssemblyPolicy_);
}

ConversationWindowResult
ApplicationController::conversationWindowForPrompt(const QString& prompt) const {
    QList<ConversationWindowMessage> messages;
    if (!chatSession_) {
        return assembleConversationWindow(messages, conversationWindowPolicy_);
    }

    const auto promptText = prompt.simplified();
    const auto& history = chatSession_->messages();
    messages.reserve(history.size());
    for (int i = 0; i < history.size(); ++i) {
        const auto& message = history.at(i);
        if (!promptText.isEmpty() && i == history.size() - 1 && message.role == ChatRole::User &&
            message.content.simplified() == promptText) {
            continue;
        }

        messages.append(ConversationWindowMessage{
            i + 1,
            chatRoleName(message.role),
            message.content,
        });
    }

    return assembleConversationWindow(messages, conversationWindowPolicy_);
}

ConversationSummaryResult
ApplicationController::conversationSummaryForPrompt(const QString& prompt) const {
    QString exclusionReason;
    if (persistedSummaryReadyForContinuity(&exclusionReason)) {
        ConversationSummaryResult result;
        result.policy = conversationSummaryPolicy_;
        result.status = ConversationSummaryStatus::Ready;
        result.readiness.status = ConversationSummaryStatus::Ready;
        result.readiness.available = true;
        result.readiness.blockedReason.clear();
        result.readiness.summary = QStringLiteral("Manual local summary is available.");
        result.sourceConversationId = latestConversationSummaryMetadata_.conversationId;
        result.summaryTimestampUtc = latestConversationSummaryMetadata_.summaryTimestampUtc;
        result.coveredFirstMessageIndex = latestConversationSummaryMetadata_.coveredFirstMessageId;
        result.coveredLastMessageIndex = latestConversationSummaryMetadata_.coveredLastMessageId;
        result.estimatedReductionPercent =
            latestConversationSummaryMetadata_.estimatedReductionPercent;
        result.continuityGainEstimatePercent =
            latestConversationSummaryMetadata_.estimatedReductionPercent;
        result.activeContextUsage = promptContextInjectionEnabled_;
        result.replacementEligible = false;
        result.stale = false;
        result.freshMessageCount =
            std::max(0, currentConversationMessageCountForSummary() -
                            latestConversationSummaryMetadata_.coveredLastMessageId);
        result.text = QStringLiteral("%1\n%2\n%3")
                          .arg(conversationSummaryPolicy_.delimiterStart,
                               latestConversationSummaryMetadata_.summaryText.trimmed(),
                               conversationSummaryPolicy_.delimiterEnd);
        result.summary =
            QStringLiteral("Generated local summary covers messages %1-%2 with %3% estimated "
                           "reduction.")
                .arg(result.coveredFirstMessageIndex)
                .arg(result.coveredLastMessageIndex)
                .arg(result.estimatedReductionPercent);
        result.coverageSummary =
            QStringLiteral("Summary coverage: messages %1-%2 of %3 visible transcript messages.")
                .arg(result.coveredFirstMessageIndex)
                .arg(result.coveredLastMessageIndex)
                .arg(currentConversationMessageCountForSummary());
        result.freshnessSummary =
            result.freshMessageCount == 0
                ? QStringLiteral("Summary freshness: current with no newer transcript messages.")
                : QStringLiteral("Summary freshness: %1 newer transcript messages preserved by "
                                 "recent-window context.")
                      .arg(result.freshMessageCount);
        result.continuitySummary =
            QStringLiteral("Summary continuity active: deterministic compressed context with %1% "
                           "estimated gain; transcript remains preserved.")
                .arg(result.continuityGainEstimatePercent);
        result.window.summarizedMessageCount =
            std::max(0, result.coveredLastMessageIndex - result.coveredFirstMessageIndex + 1);
        result.window.blockCount = 1;
        result.window.summary = result.summary;
        result.budget.maxCharacters = conversationSummaryPolicy_.maxCharacters;
        result.budget.includedCharacters =
            toInt(latestConversationSummaryMetadata_.summaryText.trimmed().size());
        result.budget.estimatedCharacters = result.budget.includedCharacters;
        result.budget.remainingCharacters =
            std::max(0, result.budget.maxCharacters - result.budget.includedCharacters);
        result.budget.blockCount = 1;
        result.budget.summary =
            QStringLiteral("%1 generated summary characters available within %2 character budget.")
                .arg(result.budget.includedCharacters)
                .arg(result.budget.maxCharacters);
        result.blocks.append(ConversationSummaryBlock{
            1,
            result.coveredFirstMessageIndex,
            result.coveredLastMessageIndex,
            result.window.summarizedMessageCount,
            result.budget.includedCharacters,
            result.budget.includedCharacters,
            false,
            latestConversationSummaryMetadata_.summaryText.trimmed(),
            {QStringLiteral("Generated manual local summary")},
        });
        return result;
    }

    const bool persistedSummaryPresent =
        !latestConversationSummaryMetadata_.conversationId.trimmed().isEmpty() ||
        !latestConversationSummaryMetadata_.summaryText.trimmed().isEmpty();
    if (persistedSummaryPresent && !exclusionReason.isEmpty()) {
        auto result = assembleConversationSummary({}, {}, conversationSummaryPolicy_);
        result.status = ConversationSummaryStatus::Blocked;
        result.readiness.status = ConversationSummaryStatus::Blocked;
        result.readiness.available = false;
        result.readiness.blockedReason = exclusionReason;
        result.readiness.summary = exclusionReason;
        result.fallback.reason = exclusionReason;
        result.fallback.summary =
            QStringLiteral("Summary continuity excluded; transcript-only fallback is active.");
        result.stale = exclusionReason.contains(QStringLiteral("stale"), Qt::CaseInsensitive);
        result.coverageSummary = QStringLiteral("Summary coverage unavailable: %1").arg(exclusionReason);
        result.freshnessSummary =
            result.stale ? QStringLiteral("Summary freshness: stale summary excluded.")
                         : QStringLiteral("Summary freshness: no valid summary is available.");
        result.continuitySummary =
            QStringLiteral("Summary continuity inactive: transcript-only fallback.");
        result.summary = result.fallback.summary;
        return result;
    }

    QList<ConversationWindowMessage> messages;
    if (!chatSession_) {
        return assembleConversationSummary(messages, {}, conversationSummaryPolicy_);
    }

    const auto promptText = prompt.simplified();
    const auto& history = chatSession_->messages();
    messages.reserve(history.size());
    for (int i = 0; i < history.size(); ++i) {
        const auto& message = history.at(i);
        if (!promptText.isEmpty() && i == history.size() - 1 && message.role == ChatRole::User &&
            message.content.simplified() == promptText) {
            continue;
        }

        messages.append(ConversationWindowMessage{
            i + 1,
            chatRoleName(message.role),
            message.content,
        });
    }

    const auto window = assembleConversationWindow(messages, conversationWindowPolicy_);
    return assembleConversationSummary(messages, window.messages, conversationSummaryPolicy_);
}

int ApplicationController::currentConversationMessageCountForSummary() const {
    if (!chatSession_) {
        return 0;
    }
    return toInt(chatSession_->messages().size());
}

QString ApplicationController::persistedSummaryExclusionReason() const {
    QString reason;
    persistedSummaryReadyForContinuity(&reason);
    return reason.trimmed().isEmpty() ? QStringLiteral("no valid generated summary is available")
                                      : reason;
}

bool ApplicationController::persistedSummaryReadyForContinuity(QString* reason) const {
    auto setReason = [reason](const QString& text) {
        if (reason) {
            *reason = text;
        }
    };

    if (latestConversationSummaryMetadata_.conversationId.trimmed().isEmpty() ||
        latestConversationSummaryMetadata_.summaryText.trimmed().isEmpty()) {
        setReason(QStringLiteral("no valid generated summary is available"));
        return false;
    }
    if (latestConversationSummaryMetadata_.conversationId != activeConversationId_) {
        setReason(QStringLiteral("summary belongs to another conversation"));
        return false;
    }
    if (activeConversationArchived()) {
        setReason(QStringLiteral("active conversation is archived"));
        return false;
    }
    if (latestConversationSummaryMetadata_.readinessState != QStringLiteral("Ready")) {
        setReason(QStringLiteral("summary readiness is not ready"));
        return false;
    }
    if (!latestConversationSummaryMetadata_.summaryTimestampUtc.isValid()) {
        setReason(QStringLiteral("summary timestamp is invalid"));
        return false;
    }
    if (latestConversationSummaryMetadata_.coveredFirstMessageId <= 0 ||
        latestConversationSummaryMetadata_.coveredLastMessageId <
            latestConversationSummaryMetadata_.coveredFirstMessageId) {
        setReason(QStringLiteral("summary coverage range is invalid"));
        return false;
    }

    const auto messageCount = currentConversationMessageCountForSummary();
    if (latestConversationSummaryMetadata_.coveredLastMessageId > messageCount) {
        setReason(QStringLiteral("summary coverage is incompatible with transcript"));
        return false;
    }

    const auto freshMessages =
        std::max(0, messageCount - latestConversationSummaryMetadata_.coveredLastMessageId);
    if (freshMessages > 12) {
        setReason(QStringLiteral("summary is stale after %1 newer transcript messages")
                      .arg(freshMessages));
        return false;
    }

    setReason({});
    return true;
}

MemoryRelevanceSummary
ApplicationController::memoryRelevanceSummaryForPrompt(const QString& prompt) const {
    QList<MemoryRelevanceCandidate> candidates;
    auto entries = currentMemoryEntries();
    std::sort(entries.begin(), entries.end(),
              [](const auto& left, const auto& right) { return left.first < right.first; });
    candidates.reserve(entries.size());
    int index = 0;
    for (const auto& entry : entries) {
        const auto key = entry.first.simplified();
        const auto value = entry.second.simplified();
        const auto foldedKey = key.toCaseFolded();
        candidates.append(MemoryRelevanceCandidate{
            key,
            value,
            index,
            true,
            foldedKey.startsWith(QStringLiteral("pinned.")) ||
                foldedKey.contains(QStringLiteral(".pinned")) ||
                foldedKey.contains(QStringLiteral("pinned.")),
        });
        ++index;
    }

    QString activeTitle;
    const auto active = activeConversationRecord();
    if (!active.id.isEmpty()) {
        activeTitle = active.title;
    }

    QStringList recentMessages;
    if (chatSession_) {
        const auto promptText = prompt.simplified();
        const auto& history = chatSession_->messages();
        if (!history.isEmpty()) {
            const qsizetype lastIndex = history.size() - 1;
            for (qsizetype i = lastIndex; i >= 0 && recentMessages.size() < 6; --i) {
                const auto& message = history.at(i);
                if (message.role == ChatRole::System) {
                    continue;
                }
                if (!promptText.isEmpty() && i == lastIndex && message.role == ChatRole::User &&
                    message.content.simplified() == promptText) {
                    continue;
                }
                recentMessages.prepend(message.content);
            }
        }
    }

    MemoryRelevancePolicy policy;
    policy.maxCharacters = std::min(1200, promptContextInjectionPolicy_.maxCharacters);
    policy.maxCandidates = retrievalPlanningPolicy_.maxCandidates;
    return rankMemoryRelevance(candidates, prompt, activeTitle,
                               recentMessages.join(QStringLiteral("\n")), policy);
}

QList<ConversationSalienceCandidate>
ApplicationController::conversationSalienceCandidatesForPrompt(const QString& prompt) const {
    QList<ConversationSalienceCandidate> candidates;
    int index = 0;

    if (contextAssemblyPolicy_.includeConversationContext) {
        const auto window = conversationWindowForPrompt(prompt);
        if (!window.text.trimmed().isEmpty()) {
            candidates.append(ConversationSalienceCandidate{
                ContextAssemblySourceKind::Conversation,
                QStringLiteral("Bounded Conversation History"),
                window.text,
                index++,
                window.budget.estimatedCharacters,
                activeConversationRecord().pinned,
                false,
                0,
            });
        }

        const auto summary = conversationSummaryForPrompt(prompt);
        if (!summary.text.trimmed().isEmpty()) {
            candidates.append(ConversationSalienceCandidate{
                ContextAssemblySourceKind::ConversationSummary,
                QStringLiteral("Older Conversation Summary"),
                summary.text,
                index++,
                summary.budget.estimatedCharacters,
                activeConversationRecord().pinned,
                false,
                2,
            });
        }
    }

    if (contextAssemblyPolicy_.includeCommittedMemoryContext) {
        const auto memorySummary = memoryRelevanceSummaryForPrompt(prompt);
        for (const auto& selection : memorySummary.selections) {
            if (!selection.included) {
                continue;
            }
            candidates.append(ConversationSalienceCandidate{
                ContextAssemblySourceKind::CommittedMemory,
                QStringLiteral("Committed Local Memory"),
                selection.selectedText,
                index++,
                selection.selectedCharacters,
                selection.candidate.pinned,
                true,
                1,
            });
        }
    }

    if (contextAssemblyPolicy_.includeRuntimeMetadataContext) {
        const auto runtimeSummary = conversationRuntimeSummary().simplified();
        if (!runtimeSummary.isEmpty()) {
            candidates.append(ConversationSalienceCandidate{
                ContextAssemblySourceKind::RuntimeMetadata,
                QStringLiteral("Runtime Metadata Summary"),
                runtimeSummary,
                index++,
                toInt(runtimeSummary.size()),
                false,
                false,
                4,
            });
        }
    }

    if (contextAssemblyPolicy_.includeOrchestrationContext) {
        const auto orchestrationSummary = orchestrationSnapshotSummary().simplified();
        if (!orchestrationSummary.isEmpty()) {
            candidates.append(ConversationSalienceCandidate{
                ContextAssemblySourceKind::Orchestration,
                QStringLiteral("Orchestration Metadata Summary"),
                orchestrationSummary,
                index++,
                toInt(orchestrationSummary.size()),
                false,
                false,
                5,
            });
        }
    }

    if (contextAssemblyPolicy_.includeSelectedConversationMetadata) {
        const auto activeSummary = activeConversationSummary().simplified();
        if (!activeSummary.isEmpty()) {
            candidates.append(ConversationSalienceCandidate{
                ContextAssemblySourceKind::SelectedConversationMetadata,
                QStringLiteral("Selected Conversation Metadata"),
                activeSummary,
                index++,
                toInt(activeSummary.size()),
                activeConversationRecord().pinned,
                false,
                3,
            });
        }
    }

    return candidates;
}

ConversationSalienceSummary
ApplicationController::conversationSalienceSummaryForPrompt(const QString& prompt) const {
    QString activeTitle;
    const auto active = activeConversationRecord();
    if (!active.id.isEmpty()) {
        activeTitle = active.title;
    }

    QStringList recentUserMessages;
    QStringList recentAssistantMessages;
    if (chatSession_) {
        const auto promptText = prompt.simplified();
        const auto& history = chatSession_->messages();
        if (!history.isEmpty()) {
            const qsizetype lastIndex = history.size() - 1;
            for (qsizetype i = lastIndex;
                 i >= 0 && (recentUserMessages.size() < 4 || recentAssistantMessages.size() < 4);
                 --i) {
                const auto& message = history.at(i);
                if (message.role == ChatRole::System) {
                    continue;
                }
                if (!promptText.isEmpty() && i == lastIndex && message.role == ChatRole::User &&
                    message.content.simplified() == promptText) {
                    continue;
                }
                if (message.role == ChatRole::User && recentUserMessages.size() < 4) {
                    recentUserMessages.prepend(message.content);
                } else if (message.role == ChatRole::Assistant &&
                           recentAssistantMessages.size() < 4) {
                    recentAssistantMessages.prepend(message.content);
                }
            }
        }
    }

    QStringList committedMemoryText;
    auto entries = currentMemoryEntries();
    std::sort(entries.begin(), entries.end(),
              [](const auto& left, const auto& right) { return left.first < right.first; });
    for (const auto& entry : entries) {
        committedMemoryText.append(QStringLiteral("%1 %2").arg(entry.first, entry.second));
    }

    ConversationSaliencePolicy policy = conversationSaliencePolicy_;
    policy.maxCharacters = promptContextInjectionPolicy_.maxCharacters;
    policy.maxCandidates = retrievalPlanningPolicy_.maxCandidates;
    return rankConversationSalience(conversationSalienceCandidatesForPrompt(prompt), prompt,
                                    activeTitle, recentUserMessages.join(QStringLiteral("\n")),
                                    recentAssistantMessages.join(QStringLiteral("\n")),
                                    committedMemoryText.join(QStringLiteral("\n")), policy);
}

ConversationCompressionSummary
ApplicationController::conversationCompressionSummaryForPrompt(const QString& prompt) const {
    QList<ConversationWindowMessage> messages;
    if (chatSession_) {
        const auto promptText = prompt.simplified();
        const auto& history = chatSession_->messages();
        messages.reserve(history.size());
        for (int i = 0; i < history.size(); ++i) {
            const auto& message = history.at(i);
            if (!promptText.isEmpty() && i == history.size() - 1 &&
                message.role == ChatRole::User && message.content.simplified() == promptText) {
                continue;
            }
            messages.append(ConversationWindowMessage{
                i + 1,
                chatRoleName(message.role),
                message.content,
                toInt(message.content.size()),
            });
        }
    }

    return planConversationCompression(messages, conversationSummaryForPrompt(prompt),
                                       conversationSalienceSummaryForPrompt(prompt),
                                       promptContextInjectionEnabled_,
                                       conversationCompressionPolicy_);
}

ConversationSummaryResult
ApplicationController::planConversationSummaryGenerationForActiveConversation(
    bool explicitUserAction) const {
    QList<ConversationWindowMessage> messages;
    if (chatSession_) {
        const auto& history = chatSession_->messages();
        messages.reserve(history.size());
        for (int i = 0; i < history.size(); ++i) {
            const auto& message = history.at(i);
            messages.append(ConversationWindowMessage{
                i + 1,
                chatRoleName(message.role),
                message.content,
                toInt(message.content.size()),
            });
        }
    }

    ConversationSummaryRequest request;
    request.requestId = QStringLiteral("conversation-summary-manual-%1")
                            .arg(QDateTime::currentDateTimeUtc().toMSecsSinceEpoch());
    request.sourceConversationId = activeConversationId();
    request.activeConversationId = activeConversationId();
    request.explicitUserAction = explicitUserAction;
    request.requestedAtUtc = QDateTime::currentDateTimeUtc();
    auto policy = conversationSummaryPolicy_;
    policy.generationAvailable = localChatSendAvailable();
    auto result = planConversationSummaryGeneration(
        messages, conversationCompressionSummaryForPrompt({}), request, policy);
    if (explicitUserAction && !policy.generationAvailable) {
        result.readiness.available = false;
        result.readiness.status = ConversationSummaryStatus::Blocked;
        result.status = ConversationSummaryStatus::Blocked;
        result.readiness.blockedReason = localChatSendAvailabilitySummary();
        result.readiness.summary = result.readiness.blockedReason;
        result.fallback.reason = result.readiness.blockedReason;
        result.fallback.summary = QStringLiteral("Manual summary generation did not start.");
        result.summary = result.readiness.blockedReason;
    }
    return result;
}

bool ApplicationController::persistConversationSummaryMetadata(
    const ConversationSummaryResult& result) {
    if (!conversationStore_ || result.sourceConversationId.trimmed().isEmpty()) {
        latestConversationSummaryMetadata_ = {};
        return false;
    }

    ConversationSummaryMetadataRecord metadata;
    metadata.conversationId = result.sourceConversationId;
    metadata.summaryTimestampUtc = result.summaryTimestampUtc;
    metadata.coveredFirstMessageId = result.coveredFirstMessageIndex;
    metadata.coveredLastMessageId = result.coveredLastMessageIndex;
    metadata.estimatedReductionPercent = result.estimatedReductionPercent;
    metadata.readinessState = conversationSummaryStatusName(result.status);
    metadata.summaryText =
        result.status == ConversationSummaryStatus::Ready ? result.text.trimmed() : QString{};
    metadata.summary = QStringLiteral("%1 / messages %2-%3 / %4% estimated reduction")
                           .arg(metadata.readinessState)
                           .arg(metadata.coveredFirstMessageId)
                           .arg(metadata.coveredLastMessageId)
                           .arg(metadata.estimatedReductionPercent);
    if (!conversationStore_->saveSummaryMetadata(metadata)) {
        latestConversationSummaryMetadata_ = {};
        return false;
    }

    latestConversationSummaryMetadata_ =
        conversationStore_->loadSummaryMetadata(result.sourceConversationId);
    return true;
}

QList<RetrievalCandidate>
ApplicationController::retrievalCandidatesForPrompt(const QString& prompt) const {
    QList<RetrievalCandidate> candidates;

    if (contextAssemblyPolicy_.includeConversationContext) {
        const auto window = conversationWindowForPrompt(prompt);
        if (!window.text.trimmed().isEmpty()) {
            candidates.append(RetrievalCandidate{
                ContextAssemblySourceKind::Conversation,
                retrievalSourcePriorityForKind(ContextAssemblySourceKind::Conversation),
                QStringLiteral("Bounded Conversation History"),
                window.text,
                window.budget.estimatedCharacters,
            });
        }

        const auto summary = conversationSummaryForPrompt(prompt);
        if (!summary.text.trimmed().isEmpty()) {
            candidates.append(RetrievalCandidate{
                ContextAssemblySourceKind::ConversationSummary,
                retrievalSourcePriorityForKind(ContextAssemblySourceKind::ConversationSummary),
                QStringLiteral("Older Conversation Summary"),
                summary.text,
                summary.budget.estimatedCharacters,
            });
        }
    }

    if (contextAssemblyPolicy_.includeCommittedMemoryContext) {
        const auto memorySummary = memoryRelevanceSummaryForPrompt(prompt);
        for (const auto& selection : memorySummary.selections) {
            if (!selection.included) {
                continue;
            }
            candidates.append(RetrievalCandidate{
                ContextAssemblySourceKind::CommittedMemory,
                retrievalSourcePriorityForKind(ContextAssemblySourceKind::CommittedMemory),
                QStringLiteral("Committed Local Memory: %1").arg(selection.candidate.key),
                selection.selectedText,
            });
        }
    }

    if (contextAssemblyPolicy_.includeRuntimeMetadataContext) {
        const auto runtimeSummary = conversationRuntimeSummary().simplified();
        if (!runtimeSummary.isEmpty()) {
            candidates.append(RetrievalCandidate{
                ContextAssemblySourceKind::RuntimeMetadata,
                retrievalSourcePriorityForKind(ContextAssemblySourceKind::RuntimeMetadata),
                QStringLiteral("Runtime Metadata Summary"),
                runtimeSummary,
            });
        }
    }

    if (contextAssemblyPolicy_.includeOrchestrationContext) {
        const auto orchestrationSummary = orchestrationSnapshotSummary().simplified();
        if (!orchestrationSummary.isEmpty()) {
            candidates.append(RetrievalCandidate{
                ContextAssemblySourceKind::Orchestration,
                retrievalSourcePriorityForKind(ContextAssemblySourceKind::Orchestration),
                QStringLiteral("Orchestration Metadata Summary"),
                orchestrationSummary,
            });
        }
    }

    const auto activeSummary = activeConversationSummary().simplified();
    if (!activeSummary.isEmpty()) {
        candidates.append(RetrievalCandidate{
            ContextAssemblySourceKind::SelectedConversationMetadata,
            retrievalSourcePriorityForKind(ContextAssemblySourceKind::SelectedConversationMetadata),
            QStringLiteral("Selected Conversation Metadata"),
            activeSummary,
        });
    }

    return candidates;
}

RetrievalPlanningResult
ApplicationController::retrievalPlanningForPrompt(const QString& prompt) const {
    RetrievalPlanningPolicy policy = retrievalPlanningPolicy_;
    policy.maxCharacters = promptContextInjectionPolicy_.maxCharacters;
    policy.includeRecentConversation = contextAssemblyPolicy_.includeConversationContext;
    policy.includeConversationSummary = contextAssemblyPolicy_.includeConversationContext;
    policy.includeCommittedMemory = contextAssemblyPolicy_.includeCommittedMemoryContext;
    policy.includeRuntimeMetadata = contextAssemblyPolicy_.includeRuntimeMetadataContext;
    policy.includeOrchestration = contextAssemblyPolicy_.includeOrchestrationContext;
    policy.includeSelectedConversationMetadata =
        contextAssemblyPolicy_.includeSelectedConversationMetadata;
    return planRetrieval(retrievalCandidatesForPrompt(prompt), policy);
}

QList<SemanticCandidate>
ApplicationController::semanticCandidatesForPrompt(const QString& prompt) const {
    QList<SemanticCandidate> candidates;
    const auto retrievalCandidates = retrievalCandidatesForPrompt(prompt);
    candidates.reserve(retrievalCandidates.size() + 1);
    int index = 0;
    for (const auto& retrievalCandidate : retrievalCandidates) {
        candidates.append(SemanticCandidate{
            semanticCandidateSourceForContextSource(retrievalCandidate.source),
            QStringLiteral("deterministic-%1").arg(index),
            retrievalCandidate.title,
            retrievalCandidate.content,
            retrievalCandidate.originalSize,
        });
        ++index;
    }

    candidates.append(SemanticCandidate{
        SemanticCandidateSource::FutureSemanticVector,
        QStringLiteral("future-semantic-vector-candidates"),
        QStringLiteral("Future Semantic/Vector Candidates"),
        {},
        0,
        0,
        SemanticCandidateSelection::Excluded,
        false,
        false,
        QStringLiteral("Semantic/vector candidate path disabled"),
        QStringLiteral("Future semantic/vector candidates are disabled."),
    });
    return candidates;
}

SemanticCandidateArbitration
ApplicationController::semanticCandidateArbitrationForPrompt(const QString& prompt) const {
    return orchestrateSemanticCandidates(semanticCandidatesForPrompt(prompt),
                                         semanticCandidatePolicy());
}

QList<PromptContextBlock> ApplicationController::promptContextBlocks(const QString& prompt) const {
    QList<PromptContextBlock> blocks;
    const auto salience = conversationSalienceSummaryForPrompt(prompt);
    blocks.reserve(salience.includedCount);
    for (const auto& selection : salience.selections) {
        if (!selection.included) {
            continue;
        }
        blocks.append(PromptContextBlock{
            selection.candidate.source,
            selection.candidate.title,
            selection.selectedText,
            selection.candidate.originalSize,
            selection.selectedCharacters,
            selection.truncated,
        });
    }

    return blocks;
}

PromptContextInjectionResult
ApplicationController::preparePromptContextInjection(const QString& prompt) {
    auto result =
        injectPromptContext(prompt, promptContextBlocks(prompt), promptContextInjectionPolicy_);
    const auto assembly = semanticSupplementAssemblyResult();
    const auto authority =
        sentinel::core::evaluateSemanticPromptAuthority(assembly, semanticPromptAuthorityPolicy_);
    latestSemanticPromptInclusionResult_ = sentinel::core::includeSemanticPromptSupplements(
        result, assembly, authority, semanticPromptInclusionPolicy());
    result.prompt = latestSemanticPromptInclusionResult_.prompt;
    return result;
}

ConversationSummaryResult
ApplicationController::blockedConversationSummaryResult(const QString& reason,
                                                        const QString& fallback) const {
    ConversationSummaryResult result;
    result.policy = conversationSummaryPolicy_;
    result.status = ConversationSummaryStatus::Blocked;
    result.sourceConversationId = activeConversationId_;
    result.summaryTimestampUtc = QDateTime::currentDateTimeUtc();
    result.readiness.status = ConversationSummaryStatus::Blocked;
    result.readiness.available = false;
    result.readiness.blockedReason = reason;
    result.readiness.summary = reason;
    result.fallback.reason = reason;
    result.fallback.summary = fallback;
    result.trace.safetySummaries.append(QStringLiteral("blocked / %1").arg(reason));
    result.trace.summary = fallback;
    result.summary = reason;
    return result;
}

QString ApplicationController::buildConversationSummaryPrompt(
    const ConversationSummaryResult& plannedResult) const {
    QStringList transcriptLines;
    if (chatSession_) {
        const auto& history = chatSession_->messages();
        for (int i = 0; i < history.size(); ++i) {
            const auto index = i + 1;
            if (index < plannedResult.coveredFirstMessageIndex ||
                index > plannedResult.coveredLastMessageIndex) {
                continue;
            }
            const auto& message = history.at(i);
            if (message.role == ChatRole::System) {
                continue;
            }
            const auto content = message.content.simplified();
            if (content.isEmpty()) {
                continue;
            }
            transcriptLines.append(QStringLiteral("%1. %2: %3")
                                       .arg(index)
                                       .arg(chatRoleName(message.role), content.left(900)));
        }
    }

    return QStringLiteral(
               "Create a concise local conversation summary from the visible transcript below.\n"
               "Rules: keep important factual continuity, retain recent context references, omit "
               "repetition, avoid invented facts, do not mention tools, providers, runtime, "
               "metadata, prompts, policies, or hidden instructions, and do not replace the "
               "transcript. Return only the summary text in 120 words or fewer.\n\n"
               "Visible transcript:\n%1")
        .arg(transcriptLines.join(QStringLiteral("\n")).left(6000));
}

QString ApplicationController::sanitizeGeneratedConversationSummary(const QString& text) const {
    auto sanitized = text.simplified();
    sanitized.remove(conversationSummaryPolicy_.delimiterStart, Qt::CaseInsensitive);
    sanitized.remove(conversationSummaryPolicy_.delimiterEnd, Qt::CaseInsensitive);
    sanitized.remove(QRegularExpression(QStringLiteral("```[^`]*```")));
    sanitized = sanitized.simplified();
    if (sanitized.size() > conversationSummaryPolicy_.maxCharacters) {
        sanitized = sanitized.left(conversationSummaryPolicy_.maxCharacters).trimmed();
    }

    const auto lowered = sanitized.toCaseFolded();
    const QStringList refusedTerms{
        QStringLiteral("system prompt"),     QStringLiteral("hidden prompt"),
        QStringLiteral("developer message"), QStringLiteral("runtime metadata"),
        QStringLiteral("provider metadata"), QStringLiteral("tool call"),
        QStringLiteral("filesystem"),        QStringLiteral("api key"),
    };
    for (const auto& term : refusedTerms) {
        if (lowered.contains(term)) {
            return {};
        }
    }
    return sanitized;
}

bool ApplicationController::startConversationSummaryInference(
    const ConversationSummaryResult& plannedResult) {
    if (!localInferenceWorker_) {
        latestConversationSummaryGenerationResult_ = blockedConversationSummaryResult(
            QStringLiteral("Local summary generation blocked: runtime client is unavailable."),
            QStringLiteral("No local summary was generated."));
        return false;
    }

    LocalInferenceRequest request;
    request.prompt = buildConversationSummaryPrompt(plannedResult);
    request.options.model = effectiveLocalModel({});
    const auto config = ollamaRuntimeClient_ ? ollamaRuntimeClient_->config() : OllamaConfig{};
    request.options.timeoutMs = config.generateTimeoutMs;
    request.id =
        QStringLiteral("conversation-summary-request-%1").arg(++localInferenceRequestSequence_);

    if (request.prompt.trimmed().isEmpty() || request.options.model.trimmed().isEmpty()) {
        latestConversationSummaryGenerationResult_ = blockedConversationSummaryResult(
            QStringLiteral("Local summary generation blocked: missing model or transcript."),
            QStringLiteral("No local summary was generated."));
        return false;
    }

    latestConversationSummaryGenerationResult_ = plannedResult;
    latestConversationSummaryGenerationResult_.status = ConversationSummaryStatus::Planned;
    latestConversationSummaryGenerationResult_.summary =
        QStringLiteral("Manual local summary generation is running.");
    latestConversationSummaryGenerationResult_.readiness.summary =
        QStringLiteral("Manual local summary generation is running.");
    latestConversationSummaryGenerationResult_.request.requestId = request.id;
    activeLocalInferenceRequestId_ = request.id;
    activeLocalInferenceConversationId_ = activeConversationId_;
    activeLocalInferenceIsSummaryRequest_ = true;
    activeLocalInferenceIsChatRequest_ = false;
    localInferenceBusy_ = true;
    latestLocalInferenceResponse_.status = LocalInferenceStatus::Busy;
    latestLocalInferenceResponse_.model = request.options.model;
    latestLocalInferenceResponse_.text.clear();
    latestLocalInferenceResponse_.summary =
        QStringLiteral("Manual local summary generation is running.");
    setConversationRuntimeRequest(request.id, request.options.model,
                                  QStringLiteral("Local Ollama Summary"), false);
    emit localInferenceChanged();
    emit localChatInferenceRoutingChanged();

    const auto started = localInferenceWorker_->startInference(
        request, [this](const QString& requestId, const LocalInferenceResponse& response) {
            finishLocalInferenceRequest(requestId, response);
        });
    if (!started) {
        localInferenceBusy_ = false;
        activeLocalInferenceIsSummaryRequest_ = false;
        activeLocalInferenceRequestId_.clear();
        activeLocalInferenceConversationId_.clear();
        latestConversationSummaryGenerationResult_ = blockedConversationSummaryResult(
            QStringLiteral("Local summary generation rejected: worker is busy."),
            QStringLiteral("No local summary was generated."));
        emit localInferenceChanged();
        emit localChatInferenceRoutingChanged();
        return false;
    }
    return true;
}

void ApplicationController::finishConversationSummaryInference(
    const QString& requestId, const LocalInferenceResponse& response) {
    if (requestId != latestConversationSummaryGenerationResult_.request.requestId) {
        return;
    }

    latestLocalInferenceResponse_ = response;
    latestLocalInferenceResponse_.text.clear();
    if (response.status != LocalInferenceStatus::Succeeded) {
        latestConversationSummaryGenerationResult_.status = ConversationSummaryStatus::Blocked;
        latestConversationSummaryGenerationResult_.readiness.status =
            ConversationSummaryStatus::Blocked;
        latestConversationSummaryGenerationResult_.readiness.available = false;
        latestConversationSummaryGenerationResult_.readiness.blockedReason =
            response.summary.trimmed().isEmpty()
                ? QStringLiteral("Local summary generation failed.")
                : response.summary.trimmed();
        latestConversationSummaryGenerationResult_.readiness.summary =
            latestConversationSummaryGenerationResult_.readiness.blockedReason;
        latestConversationSummaryGenerationResult_.fallback.reason =
            latestConversationSummaryGenerationResult_.readiness.blockedReason;
        latestConversationSummaryGenerationResult_.fallback.summary =
            QStringLiteral("Transcript, memory, and persisted summary metadata were unchanged.");
        latestConversationSummaryGenerationResult_.summary =
            latestConversationSummaryGenerationResult_.readiness.blockedReason;
        emit contextAssemblyChanged();
        return;
    }

    const auto summaryText = sanitizeGeneratedConversationSummary(response.text);
    if (summaryText.isEmpty()) {
        latestConversationSummaryGenerationResult_ = blockedConversationSummaryResult(
            QStringLiteral("Local summary generation returned an invalid safe summary."),
            QStringLiteral("Transcript, memory, and persisted summary metadata were unchanged."));
        emit contextAssemblyChanged();
        return;
    }

    latestConversationSummaryGenerationResult_.status = ConversationSummaryStatus::Ready;
    latestConversationSummaryGenerationResult_.readiness.status = ConversationSummaryStatus::Ready;
    latestConversationSummaryGenerationResult_.readiness.available = true;
    latestConversationSummaryGenerationResult_.readiness.blockedReason.clear();
    latestConversationSummaryGenerationResult_.readiness.summary =
        QStringLiteral("Manual local summary generated and persisted.");
    latestConversationSummaryGenerationResult_.summaryTimestampUtc =
        QDateTime::currentDateTimeUtc();
    latestConversationSummaryGenerationResult_.text = summaryText;
    latestConversationSummaryGenerationResult_.summary =
        QStringLiteral("Manual local summary covers messages %1-%2 with %3% estimated reduction.")
            .arg(latestConversationSummaryGenerationResult_.coveredFirstMessageIndex)
            .arg(latestConversationSummaryGenerationResult_.coveredLastMessageIndex)
            .arg(latestConversationSummaryGenerationResult_.estimatedReductionPercent);
    latestConversationSummaryGenerationResult_.preview.available = true;
    latestConversationSummaryGenerationResult_.preview.summary =
        QStringLiteral("Manual local summary is available for optional context injection.");
    persistConversationSummaryMetadata(latestConversationSummaryGenerationResult_);
    emit contextAssemblyChanged();
}

QString ApplicationController::memoryRecallPolicyStatus() const {
    return memoryRecallPolicy_.status;
}

QString ApplicationController::memoryRecallPolicySummary() const {
    return memoryRecallPolicy_.summary;
}

QString ApplicationController::memoryRecallQueryText() const {
    return latestMemoryRecallSummary_.query.text;
}

QString ApplicationController::memoryRecallStatus() const {
    return memoryRecallStatusName(latestMemoryRecallSummary_.status);
}

QString ApplicationController::memoryRecallSummaryText() const {
    if (latestMemoryRecallSummary_.memoryEntryCount != memoryEntryCount()) {
        auto summary = latestMemoryRecallSummary_;
        summary.memoryEntryCount = memoryEntryCount();
        if (summary.status == MemoryRecallStatus::NotSearched) {
            summary.summary =
                QStringLiteral("No local memory recall query has been run. %1 committed %2 "
                               "available.")
                    .arg(summary.memoryEntryCount)
                    .arg(summary.memoryEntryCount == 1 ? QStringLiteral("entry")
                                                       : QStringLiteral("entries"));
        }
        return summary.summary;
    }
    return latestMemoryRecallSummary_.summary;
}

int ApplicationController::memoryRecallResultCount() const {
    return latestMemoryRecallSummary_.resultCount;
}

QStringList ApplicationController::memoryRecallResultSummaries() const {
    return sentinel::core::memoryRecallResultSummaries(latestMemoryRecallSummary_);
}

QString ApplicationController::contextAssemblyPolicyStatus() const {
    return contextAssemblyPolicy_.status;
}

QString ApplicationController::contextAssemblyPolicySummary() const {
    return contextAssemblyPolicy_.summary;
}

QString ApplicationController::contextAssemblyStatus() const {
    return contextAssemblyStatusName(contextAssemblySummary().status);
}

QString ApplicationController::contextAssemblySummaryText() const {
    return contextAssemblySummary().summary;
}

int ApplicationController::contextAssemblySourceCount() const {
    return contextAssemblySummary().requestedSourceCount;
}

int ApplicationController::contextAssemblyAvailableSourceCount() const {
    return contextAssemblySummary().availableSourceCount;
}

int ApplicationController::contextAssemblyCandidateBlockCount() const {
    return contextAssemblySummary().candidateBlockCount;
}

int ApplicationController::contextAssemblyEstimatedSize() const {
    return contextAssemblySummary().estimatedSize;
}

QString ApplicationController::conversationContextAvailability() const {
    return contextAssemblySource(ContextAssemblySourceKind::Conversation).status;
}

QString ApplicationController::committedMemoryContextAvailability() const {
    return contextAssemblySource(ContextAssemblySourceKind::CommittedMemory).status;
}

QString ApplicationController::runtimeMetadataContextAvailability() const {
    return contextAssemblySource(ContextAssemblySourceKind::RuntimeMetadata).status;
}

QString ApplicationController::orchestrationContextAvailability() const {
    return contextAssemblySource(ContextAssemblySourceKind::Orchestration).status;
}

QStringList ApplicationController::contextAssemblySourceSummaries() const {
    return sentinel::core::contextAssemblySourceSummaries(contextAssemblySummary());
}

QStringList ApplicationController::contextAssemblyReadinessChecks() const {
    return contextAssemblySummary().checks;
}

int ApplicationController::memoryEntryCount() const {
    return toInt(currentMemoryEntries().size());
}

bool ApplicationController::memoryKeyExists(const QString& key) const {
    if (!memoryStore_) {
        return false;
    }

    const auto trimmed = key.trimmed();
    for (const auto& entry : memoryStore_->entries()) {
        if (entry.first == trimmed) {
            return true;
        }
    }
    return false;
}

MemoryEntries ApplicationController::currentMemoryEntries() const {
    if (!memoryStore_ || !memoryStore_->isAvailable()) {
        return {};
    }
    return memoryStore_->entries();
}

void ApplicationController::refreshMemoryRecallForCurrentEntries() {
    if (latestMemoryRecallSummary_.query.text.trimmed().isEmpty()) {
        latestMemoryRecallSummary_.memoryEntryCount = memoryEntryCount();
        return;
    }

    latestMemoryRecallSummary_ = memoryRecallSummaryForEntries(
        latestMemoryRecallSummary_.query, currentMemoryEntries(),
        memoryStore_ && memoryStore_->isAvailable(), memoryRecallPolicy_);
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

bool ApplicationController::reviewMemoryCandidate(const QString& candidateId,
                                                  MemoryCandidateReviewAction action) {
    latestMemoryCandidateReviewResult_ = MemoryCandidateReviewResult{};
    if (!memoryCandidateStore_) {
        latestMemoryCandidateReviewResult_.status = QStringLiteral("Refused");
        latestMemoryCandidateReviewResult_.summary =
            QStringLiteral("Memory candidate review refused: candidate store is unavailable.");
        emit memoryCandidatesChanged();
        return false;
    }

    latestMemoryCandidateReviewResult_ = memoryCandidateStore_->reviewCandidate(
        candidateId, action, QStringLiteral("Reviewer: User review"), {});
    emit memoryCandidatesChanged();
    return latestMemoryCandidateReviewResult_.accepted;
}

bool ApplicationController::searchConversation(const QString& query) {
    const auto trimmed = query.trimmed();
    latestConversationSearchSummary_ = ConversationSearchSummary{};
    latestConversationSearchSummary_.query.text = trimmed;
    latestConversationSearchSummary_.transcriptMessageCount =
        toInt(chatSession_->messages().size());

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
    latestConversationSearchSummary_.resultCount =
        toInt(latestConversationSearchSummary_.results.size());
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
            QStringLiteral("Transcript export refused: unsupported format.");
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
    } else if (exportFormat == ConversationExportFormat::Json) {
        payload = jsonTranscript(messages, exportedAt);
    } else if (exportFormat == ConversationExportFormat::Pdf) {
        payload = pdfTranscript(messages, exportedAt);
    } else {
        payload = plainTextTranscript(messages, exportedAt).toUtf8();
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
    latestConversationExportResult_.messageCount = toInt(messages.size());
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
        QStringLiteral("Permanent delete is not enabled yet. Archive is available.");
    latestConversationDeleteResult_.summary =
        QStringLiteral("Permanent delete is not enabled yet. Archive is available.");
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
    emit contextAssemblyChanged();
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
    emit contextAssemblyChanged();
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

bool ApplicationController::pinConversation(const QString& conversationId) {
    if (!conversationStore_ || conversationStore_->status() != ConversationStoreStatus::Ready) {
        return false;
    }
    const auto pinned = conversationStore_->pinConversation(conversationId.trimmed());
    if (pinned) {
        emit chatMessagesChanged();
    }
    return pinned;
}

bool ApplicationController::unpinConversation(const QString& conversationId) {
    if (!conversationStore_ || conversationStore_->status() != ConversationStoreStatus::Ready) {
        return false;
    }
    const auto unpinned = conversationStore_->unpinConversation(conversationId.trimmed());
    if (unpinned) {
        emit chatMessagesChanged();
    }
    return unpinned;
}

QString ApplicationController::duplicateConversation(const QString& conversationId) {
    latestConversationDuplicateResult_ = ConversationDuplicateResult{};
    const auto trimmedId = conversationId.trimmed();
    latestConversationDuplicateResult_.sourceConversationId = trimmedId;

    if (trimmedId.isEmpty() || !conversationStore_ ||
        conversationStore_->status() != ConversationStoreStatus::Ready) {
        latestConversationDuplicateResult_.status = QStringLiteral("Refused");
        latestConversationDuplicateResult_.limitationSummary =
            QStringLiteral("Conversation store is unavailable.");
        latestConversationDuplicateResult_.summary =
            QStringLiteral("Duplicate refused: conversation store is unavailable.");
        emit conversationDuplicateChanged();
        return {};
    }

    ConversationRecord source;
    for (const auto& record : conversationStore_->listConversations()) {
        if (record.id == trimmedId && !record.deleted) {
            source = record;
            break;
        }
    }
    if (source.id.isEmpty()) {
        latestConversationDuplicateResult_.status = QStringLiteral("Refused");
        latestConversationDuplicateResult_.limitationSummary =
            QStringLiteral("Source conversation was not found.");
        latestConversationDuplicateResult_.summary =
            QStringLiteral("Duplicate refused: source conversation was not found.");
        emit conversationDuplicateChanged();
        return {};
    }

    const auto duplicateTitle = QStringLiteral("%1 Copy").arg(
        source.title.trimmed().isEmpty() ? QStringLiteral("Untitled Conversation")
                                         : source.title.trimmed());
    const auto duplicate = conversationStore_->createConversation(duplicateTitle);
    if (duplicate.id.isEmpty()) {
        latestConversationDuplicateResult_.status = QStringLiteral("Refused");
        latestConversationDuplicateResult_.limitationSummary =
            QStringLiteral("Conversation metadata could not be created.");
        latestConversationDuplicateResult_.summary =
            QStringLiteral("Duplicate refused: local conversation metadata could not be created.");
        emit conversationDuplicateChanged();
        return {};
    }

    const auto messages = conversationStore_->loadMessages(source.id);
    int copiedCount = 0;
    bool copyFailed = false;
    for (const auto& message : messages) {
        auto duplicateMessage = message;
        duplicateMessage.conversationId = duplicate.id;
        if (!conversationStore_->appendMessage(duplicateMessage)) {
            copyFailed = true;
            break;
        }
        ++copiedCount;
    }

    latestConversationDuplicateResult_.accepted = !copyFailed;
    latestConversationDuplicateResult_.copiedMessages = !copyFailed;
    latestConversationDuplicateResult_.status =
        copyFailed ? QStringLiteral("Partial") : QStringLiteral("Succeeded");
    latestConversationDuplicateResult_.duplicateConversationId = duplicate.id;
    latestConversationDuplicateResult_.duplicateTitle = duplicate.title;
    latestConversationDuplicateResult_.copiedMessageCount = copiedCount;
    latestConversationDuplicateResult_.limitationSummary =
        copyFailed ? QStringLiteral("Message copy stopped before all local messages were copied.")
                   : QStringLiteral("Transcript messages copied locally.");
    latestConversationDuplicateResult_.summary =
        copyFailed
            ? QStringLiteral("%1 created, but only %2 of %3 local messages copied.")
                  .arg(duplicate.title)
                  .arg(copiedCount)
                  .arg(messages.size())
            : QStringLiteral("%1 created with %2 copied local %3.")
                  .arg(duplicate.title)
                  .arg(copiedCount)
                  .arg(copiedCount == 1 ? QStringLiteral("message") : QStringLiteral("messages"));

    emit conversationDuplicateChanged();
    emit chatMessagesChanged();
    return duplicate.id;
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
    emit contextAssemblyChanged();
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
    return reviewMemoryCandidate(candidateId, MemoryCandidateReviewAction::Approve);
}

bool ApplicationController::rejectMemoryCandidate(const QString& candidateId) {
    return reviewMemoryCandidate(candidateId, MemoryCandidateReviewAction::Reject);
}

bool ApplicationController::resetMemoryCandidate(const QString& candidateId) {
    return reviewMemoryCandidate(candidateId, MemoryCandidateReviewAction::ResetToPending);
}

bool ApplicationController::archiveMemoryCandidate(const QString& candidateId) {
    return reviewMemoryCandidate(candidateId, MemoryCandidateReviewAction::Archive);
}

bool ApplicationController::requestMemoryCandidateCommit(const QString& candidateId) {
    latestMemoryCommitResult_ = MemoryCommitResult{};
    latestMemoryCommitResult_.candidateId = candidateId.trimmed();
    latestMemoryCommitResult_.target = memoryCommitPolicy_.target;
    latestMemoryCommitResult_.commitStatus = MemoryCommitStatus::Refused;
    latestMemoryCommitResult_.requestedAtUtc = QDateTime::currentDateTimeUtc();

    const auto candidates = memoryCandidates();
    const auto* candidate = findMemoryCandidate(candidates, candidateId);
    const auto readiness = memoryCommitReadinessForCandidateId(candidateId);
    latestMemoryCommitResult_.committedKey = readiness.plan.key;
    if (!readiness.ready || !candidate) {
        latestMemoryCommitResult_.status = QStringLiteral("Refused");
        latestMemoryCommitResult_.summary =
            QStringLiteral("Memory commit refused: %1").arg(readiness.summary);
        emit memoryCandidatesChanged();
        return false;
    }

    if (memoryKeyExists(readiness.plan.key) &&
        memoryCommitPolicy_.conflictPolicy == MemoryCommitConflictPolicy::Refuse) {
        latestMemoryCommitResult_.status = QStringLiteral("Refused");
        latestMemoryCommitResult_.summary =
            QStringLiteral("Memory commit refused: key already exists and overwrite is disabled "
                           "(%1).")
                .arg(readiness.plan.key);
        emit memoryCandidatesChanged();
        return false;
    }

    const auto value = candidate->excerpt.simplified();
    if (value.isEmpty()) {
        latestMemoryCommitResult_.status = QStringLiteral("Refused");
        latestMemoryCommitResult_.summary =
            QStringLiteral("Memory commit refused: reviewed candidate content is empty.");
        emit memoryCandidatesChanged();
        return false;
    }

    memoryStore_->put(readiness.plan.key, value);

    latestMemoryCommitResult_.accepted = true;
    latestMemoryCommitResult_.commitStatus = MemoryCommitStatus::Committed;
    latestMemoryCommitResult_.committedAtUtc = QDateTime::currentDateTimeUtc();
    latestMemoryCommitResult_.status = QStringLiteral("Committed");
    latestMemoryCommitResult_.summary =
        QStringLiteral("Memory commit stored approved candidate as key '%1' at %2.")
            .arg(readiness.plan.key,
                 latestMemoryCommitResult_.committedAtUtc.toString(Qt::ISODateWithMs));

    if (memoryCandidateStore_) {
        memoryCandidateStore_->recordCommitResult(latestMemoryCommitResult_);
    }

    refreshMemoryRecallForCurrentEntries();
    emit memoryEntriesChanged();
    emit memoryRecallChanged();
    emit contextAssemblyChanged();
    emit memoryCandidatesChanged();
    return true;
}

bool ApplicationController::recallLocalMemory(const QString& query) {
    latestMemoryRecallSummary_ = memoryRecallSummaryForEntries(
        MemoryRecallQuery{query.trimmed(), true, true}, currentMemoryEntries(),
        memoryStore_ && memoryStore_->isAvailable(), memoryRecallPolicy_);
    emit memoryRecallChanged();
    return latestMemoryRecallSummary_.status == MemoryRecallStatus::Completed &&
           latestMemoryRecallSummary_.resultCount > 0;
}

void ApplicationController::clearLocalMemoryRecall() {
    latestMemoryRecallSummary_ = MemoryRecallSummary{};
    latestMemoryRecallSummary_.memoryEntryCount = memoryEntryCount();
    emit memoryRecallChanged();
}

bool ApplicationController::requestConversationSummaryGeneration() {
    if (localInferenceBusy_) {
        latestConversationSummaryGenerationResult_ = blockedConversationSummaryResult(
            QStringLiteral("Summary generation is already busy."),
            QStringLiteral("Wait for the current local generation request to finish."));
        emit contextAssemblyChanged();
        return false;
    }

    if (activeConversationArchived()) {
        latestConversationSummaryGenerationResult_ = blockedConversationSummaryResult(
            QStringLiteral("Archived conversations cannot generate summaries."),
            QStringLiteral("Unarchive the active conversation before generating a summary."));
        emit contextAssemblyChanged();
        return false;
    }

    latestConversationSummaryGenerationResult_ =
        planConversationSummaryGenerationForActiveConversation(true);
    if (latestConversationSummaryGenerationResult_.status != ConversationSummaryStatus::Planned) {
        emit contextAssemblyChanged();
        return false;
    }

    if (!startConversationSummaryInference(latestConversationSummaryGenerationResult_)) {
        emit contextAssemblyChanged();
        return false;
    }
    emit contextAssemblyChanged();
    return true;
}

bool ApplicationController::sendMessage(const QString& message) {
    const auto trimmed = message.trimmed();
    if (trimmed.isEmpty()) {
        setChatSendLifecycle(QStringLiteral("refused"),
                             QStringLiteral("Enter a prompt before sending."));
        return false;
    }

    setChatSendLifecycle(QStringLiteral("validating"),
                         QStringLiteral("Checking local chat send readiness."));

    if (activeConversationArchived()) {
        setChatSendLifecycle(QStringLiteral("refused"), activeConversationStateSummary());
        return false;
    }

    if (localInferenceBusy_) {
        LocalInferenceRequest request;
        request.prompt = trimmed;
        request.options.model = effectiveLocalModel({});
        request.options.timeoutMs = localInferenceTimeoutMs_;
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::BusyRequest,
            QStringLiteral("Local inference request rejected: another request is already "
                           "running."));
        latestLocalInferenceResponse_.status = LocalInferenceStatus::Busy;
        latestLocalInferenceStreamResult_.accumulatedText.clear();
        setChatSendLifecycle(QStringLiteral("refused"),
                             QStringLiteral("A request is already active. Wait for it to finish."));
        emit localInferenceChanged();
        emit localChatInferenceRoutingChanged();
        return false;
    }

    if ((selectedRuntimeProvider_ != QStringLiteral("ollama") || localChatInferenceEnabled_) &&
        !localChatSendAvailable()) {
        const auto localChatEffectiveModel = effectiveLocalModel({});
        const auto localChatHealth = currentOllamaHealthCheck();
        LocalInferenceRequest request;
        request.prompt = trimmed;
        request.options.model = localChatEffectiveModel;
        const auto reason = localChatSendAvailabilitySummary();
        auto error = LocalInferenceError::RequestFailed;
        auto status = LocalInferenceStatus::Blocked;
        if (selectedRuntimeProvider_ != QStringLiteral("ollama") || !localChatInferenceEnabled_) {
            error = LocalInferenceError::PermissionDenied;
        } else if (localInferenceBusy_) {
            error = LocalInferenceError::BusyRequest;
            status = LocalInferenceStatus::Busy;
        } else if (!localInferenceEndpointAllowed()) {
            error = LocalInferenceError::EndpointBlocked;
        } else if (selectedLocalModel_.trimmed().isEmpty() || localChatEffectiveModel.isEmpty()) {
            error = LocalInferenceError::MissingModel;
            status = LocalInferenceStatus::InvalidRequest;
        } else if (localChatHealth.healthStatus != OllamaHealthStatus::Healthy) {
            error = LocalInferenceError::OllamaNotRunning;
            status = LocalInferenceStatus::Error;
            request.options.timeoutMs = localChatHealth.timeoutMs;
        } else {
            error = LocalInferenceError::ModelUnavailable;
            status = LocalInferenceStatus::ModelUnavailable;
        }
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(request, error, reason);
        latestLocalInferenceResponse_.status = status;
        latestLocalInferenceResponse_.timeoutMs = request.options.timeoutMs;
        latestLocalInferenceStreamResult_.accumulatedText.clear();
        setConversationRuntimeRequest(QStringLiteral("local-inference-blocked"),
                                      request.options.model, QStringLiteral("Local Ollama"), false);
        setConversationRuntimeResult(false, reason);
        setChatSendLifecycle(QStringLiteral("refused"), reason);
        emit localInferenceChanged();
        emit localChatInferenceRoutingChanged();
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
    latestConversationSummaryGenerationResult_ = {};
    setChatSendLifecycle(QStringLiteral("sending"),
                         QStringLiteral("Prompt accepted and added to the local transcript."));
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
        emit contextAssemblyChanged();
        activeLocalInferenceIsChatRequest_ = true;
        const auto startedLocalInference = localInferenceStreamingAvailable()
                                               ? runLocalInferenceStream(trimmed, {})
                                               : runLocalInference(trimmed, {});
        if (startedLocalInference || !activeLocalInferenceIsChatRequest_) {
            return true;
        }

        finalizeLocalChatInference(false);
        return true;
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
        setChatSendLifecycle(QStringLiteral("failed"), errorMessage.content);
        refreshConversationHistorySummary();
        emit chatMessagesChanged();
        emit contextAssemblyChanged();
        return true;
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
    setChatSendLifecycle(reply.success ? QStringLiteral("completed") : QStringLiteral("failed"),
                         assistantMessage.content);
    refreshConversationHistorySummary();
    emit chatMessagesChanged();
    emit contextAssemblyChanged();
    return true;
}

bool ApplicationController::runLocalInference(const QString& prompt, const QString& model) {
    if (localInferenceBusy_) {
        LocalInferenceRequest request;
        request.prompt = prompt.trimmed();
        request.options.model = effectiveLocalModel(model);
        request.options.timeoutMs = localInferenceTimeoutMs_;
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
    request.options.timeoutMs = localInferenceTimeoutMs_;
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

    latestPromptContextInjectionResult_ = preparePromptContextInjection(request.prompt);
    request.prompt = latestPromptContextInjectionResult_.prompt;
    emit promptContextInjectionChanged();

    request.id = QStringLiteral("local-inference-request-%1").arg(++localInferenceRequestSequence_);
    activeLocalInferenceRequestId_ = request.id;
    activeLocalInferenceConversationId_ = activeConversationId_;
    localInferenceBusy_ = true;
    setChatSendLifecycle(QStringLiteral("sending"),
                         QStringLiteral("Local Ollama request accepted."));
    setConversationRuntimeRequest(request.id, request.options.model, QStringLiteral("Local Ollama"),
                                  false);
    latestLocalInferenceResponse_.status = LocalInferenceStatus::Busy;
    latestLocalInferenceResponse_.summary = QStringLiteral("Local inference request is running.");
    latestLocalInferenceResponse_.timeoutMs = request.options.timeoutMs;
    emit localInferenceChanged();
    emit localChatInferenceRoutingChanged();

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
        activeLocalInferenceConversationId_.clear();
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::BusyRequest,
            QStringLiteral("Local inference request rejected: worker is busy."));
        latestLocalInferenceResponse_.status = LocalInferenceStatus::Busy;
        emit localInferenceChanged();
        emit localChatInferenceRoutingChanged();
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
    activeLocalInferenceConversationId_.clear();
    activeLocalInferenceIsChatRequest_ = false;
    const bool wasSummaryRequest = activeLocalInferenceIsSummaryRequest_;
    activeLocalInferenceIsSummaryRequest_ = false;
    localInferenceBusy_ = false;
    latestLocalInferenceResponse_.status = LocalInferenceStatus::Blocked;
    latestLocalInferenceResponse_.error = LocalInferenceError::RequestFailed;
    latestLocalInferenceResponse_.text.clear();
    latestLocalInferenceResponse_.summary =
        QStringLiteral("Local inference request was cancelled; stale results will be ignored.");
    setConversationRuntimeResult(false, latestLocalInferenceResponse_.summary);
    setChatSendLifecycle(QStringLiteral("cancelled"), latestLocalInferenceResponse_.summary);
    latestLocalInferenceStreamResult_.accumulatedText.clear();
    if (latestLocalInferenceStreamResult_.status == LocalInferenceStreamStatus::Streaming) {
        latestLocalInferenceStreamResult_.status = LocalInferenceStreamStatus::Cancelled;
        latestLocalInferenceStreamResult_.summary =
            QStringLiteral("Local streaming request was cancelled.");
    }
    if (wasSummaryRequest) {
        latestConversationSummaryGenerationResult_ = blockedConversationSummaryResult(
            QStringLiteral("Manual summary generation was cancelled."),
            QStringLiteral("Transcript, memory, and persisted summary metadata were unchanged."));
        emit contextAssemblyChanged();
    }
    emit localInferenceChanged();
    emit localChatInferenceRoutingChanged();
    return true;
}

bool ApplicationController::runLocalInferenceStream(const QString& prompt, const QString& model) {
    if (localInferenceBusy_) {
        LocalInferenceRequest request;
        request.prompt = prompt.trimmed();
        request.options.model = effectiveLocalModel(model);
        request.options.streamingRequested = true;
        request.options.timeoutMs = localInferenceTimeoutMs_;
        latestLocalInferenceResponse_ = blockedLocalInferenceResponse(
            request, LocalInferenceError::BusyRequest,
            QStringLiteral("Local streaming request rejected: another request is already "
                           "running."));
        latestLocalInferenceStreamResult_.accumulatedText.clear();
        emit localInferenceChanged();
        emit localChatInferenceRoutingChanged();
        return false;
    }

    LocalInferenceRequest request;
    request.prompt = prompt.trimmed();
    request.options.model = effectiveLocalModel(model);
    request.options.streamingRequested = true;
    request.options.timeoutMs = localInferenceTimeoutMs_;

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

    latestPromptContextInjectionResult_ = preparePromptContextInjection(request.prompt);
    request.prompt = latestPromptContextInjectionResult_.prompt;
    emit promptContextInjectionChanged();

    request.id = QStringLiteral("local-inference-request-%1").arg(++localInferenceRequestSequence_);
    activeLocalInferenceRequestId_ = request.id;
    activeLocalInferenceConversationId_ = activeConversationId_;
    localInferenceBusy_ = true;
    setChatSendLifecycle(QStringLiteral("streaming"),
                         QStringLiteral("Local Ollama stream accepted."));
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
    emit localChatInferenceRoutingChanged();

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
        activeLocalInferenceConversationId_.clear();
        const auto blocked =
            blockStream(LocalInferenceError::BusyRequest,
                        QStringLiteral("Local streaming request rejected: worker is busy."),
                        LocalInferenceStatus::Busy);
        emit localChatInferenceRoutingChanged();
        return blocked;
    }
    return localInferenceBusy_ ||
           latestLocalInferenceResponse_.status == LocalInferenceStatus::Succeeded;
}

void ApplicationController::finishLocalInferenceRequest(const QString& requestId,
                                                        const LocalInferenceResponse& response) {
    if (requestId != activeLocalInferenceRequestId_) {
        return;
    }
    if (!activeLocalInferenceConversationId_.isEmpty() &&
        activeLocalInferenceConversationId_ != activeConversationId_) {
        localInferenceBusy_ = false;
        activeLocalInferenceIsChatRequest_ = false;
        const bool wasSummaryRequest = activeLocalInferenceIsSummaryRequest_;
        activeLocalInferenceIsSummaryRequest_ = false;
        activeLocalInferenceRequestId_.clear();
        activeLocalInferenceConversationId_.clear();
        latestLocalInferenceStreamResult_.accumulatedText.clear();
        setChatSendLifecycle(
            QStringLiteral("cancelled"),
            QStringLiteral("Ignored stale local response after conversation switch."));
        if (wasSummaryRequest) {
            latestConversationSummaryGenerationResult_ = blockedConversationSummaryResult(
                QStringLiteral("Ignored stale summary completion after conversation switch."),
                QStringLiteral("No transcript, memory, or summary metadata was changed."));
            emit contextAssemblyChanged();
        }
        emit localInferenceChanged();
        emit localChatInferenceRoutingChanged();
        return;
    }

    localInferenceBusy_ = false;
    activeLocalInferenceRequestId_.clear();
    activeLocalInferenceConversationId_.clear();
    if (activeLocalInferenceIsSummaryRequest_) {
        activeLocalInferenceIsSummaryRequest_ = false;
        finishConversationSummaryInference(requestId, response);
        setConversationRuntimeResult(response.status == LocalInferenceStatus::Succeeded,
                                     response.summary, response.latencyMs);
        emit localInferenceChanged();
        emit localChatInferenceRoutingChanged();
        return;
    }
    latestLocalInferenceResponse_ = response;
    if (latestLocalInferenceResponse_.timeoutMs <= 0) {
        latestLocalInferenceResponse_.timeoutMs = localInferenceTimeoutMs_;
    }
    if (latestLocalInferenceResponse_.status != LocalInferenceStatus::Succeeded) {
        latestLocalInferenceResponse_.text.clear();
    }
    setConversationRuntimeResult(
        latestLocalInferenceResponse_.status == LocalInferenceStatus::Succeeded,
        latestLocalInferenceResponse_.summary, latestLocalInferenceResponse_.latencyMs);
    emit localInferenceChanged();
    emit localChatInferenceRoutingChanged();

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
    if (!activeLocalInferenceConversationId_.isEmpty() &&
        activeLocalInferenceConversationId_ != activeConversationId_) {
        localInferenceBusy_ = false;
        activeLocalInferenceIsChatRequest_ = false;
        activeLocalInferenceRequestId_.clear();
        activeLocalInferenceConversationId_.clear();
        latestLocalInferenceStreamResult_.accumulatedText.clear();
        setChatSendLifecycle(
            QStringLiteral("cancelled"),
            QStringLiteral("Ignored stale local stream after conversation switch."));
        emit localInferenceChanged();
        emit localChatInferenceRoutingChanged();
        return;
    }

    localInferenceBusy_ = false;
    activeLocalInferenceRequestId_.clear();
    activeLocalInferenceConversationId_.clear();
    latestLocalInferenceStreamResult_ = result;
    if (latestLocalInferenceStreamResult_.timeoutMs <= 0) {
        latestLocalInferenceStreamResult_.timeoutMs = localInferenceTimeoutMs_;
    }
    latestLocalInferenceStreamResult_.accumulatedText.clear();
    latestLocalInferenceResponse_.model = result.model;
    latestLocalInferenceResponse_.endpoint = result.endpoint;
    latestLocalInferenceResponse_.text = result.accumulatedText;
    latestLocalInferenceResponse_.error = result.error;
    latestLocalInferenceResponse_.summary = result.summary;
    latestLocalInferenceResponse_.timeoutMs = latestLocalInferenceStreamResult_.timeoutMs;
    latestLocalInferenceResponse_.latencyMs = result.latencyMs;
    latestLocalInferenceResponse_.firstTokenLatencyMs = result.firstTokenLatencyMs;
    latestLocalInferenceResponse_.approximateOutputTokens = result.approximateOutputTokens;
    latestLocalInferenceResponse_.approximateTokensPerSecond =
        result.approximateTokensPerSecond;
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
    emit localChatInferenceRoutingChanged();

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
    setChatSendLifecycle(succeeded ? QStringLiteral("completed") : QStringLiteral("failed"),
                         latestLocalInferenceResponse_.summary);
    auto assistantMessage = chatSession_->appendAssistantMessage(
        succeeded ? latestLocalInferenceResponse_.text
                  : localInferenceChatFailureMessage(latestLocalInferenceResponse_),
        succeeded ? ChatMessageStatus::Received : ChatMessageStatus::Error);
    assistantMessage.providerUsed = QStringLiteral("ollama");
    assistantMessage.modelUsed = latestLocalInferenceResponse_.model;
    assistantMessage.roleUsed = QStringLiteral("primary");
    assistantMessage.responseDurationMs = latestLocalInferenceResponse_.latencyMs;
    assistantMessage.firstTokenLatencyMs = latestLocalInferenceResponse_.firstTokenLatencyMs;
    assistantMessage.approximateTokensPerSecond =
        latestLocalInferenceResponse_.approximateTokensPerSecond;
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
    activeLocalInferenceIsSummaryRequest_ = false;
    activeLocalInferenceRequestId_.clear();
    activeLocalInferenceConversationId_.clear();
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
    latestConversationSearchSummary_.transcriptMessageCount =
        toInt(chatSession_->messages().size());
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

void ApplicationController::setChatSendLifecycle(const QString& state, const QString& summary) {
    const auto safeState = state.trimmed().isEmpty() ? QStringLiteral("idle") : state.trimmed();
    const auto safeSummary = summary.trimmed().isEmpty()
                                 ? QStringLiteral("No send lifecycle summary.")
                                 : summary.trimmed();
    if (chatSendLifecycleState_ == safeState && chatSendLifecycleSummary_ == safeSummary) {
        return;
    }

    chatSendLifecycleState_ = safeState;
    chatSendLifecycleSummary_ = safeSummary;
    emit localChatInferenceRoutingChanged();
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

RuntimeProviderRegistry ApplicationController::currentRuntimeProviderRegistry() const {
    const auto health = currentOllamaHealthCheck();
    const auto models = currentOllamaModels();
    const OllamaRuntimeProvider ollamaProvider{
        ollamaEndpoint(), health, models, selectedLocalModel_, localChatInferenceEnabled_,
        localInferenceBusy_};
    const OpenAICompatibleLocalRuntimeProvider openAiCompatibleLocalProvider{
        QStringLiteral("openai-compatible-local"),
        QStringLiteral("OpenAI-compatible Local"),
        QStringLiteral("Loopback endpoint not configured"),
        selectedRuntimeProvider_ == QStringLiteral("openai-compatible-local") ? selectedLocalModel_
                                                                              : QString()};
    const OpenAICompatibleLocalRuntimeProvider lmStudioProvider{
        QStringLiteral("lm-studio"),
        QStringLiteral("LM Studio"),
        QStringLiteral("OpenAI-compatible loopback endpoint not configured"),
        selectedRuntimeProvider_ == QStringLiteral("lm-studio") ? selectedLocalModel_ : QString()};
    const OpenAICompatibleLocalRuntimeProvider llamaCppProvider{
        QStringLiteral("llama-cpp-server"),
        QStringLiteral("llama.cpp server"),
        QStringLiteral("OpenAI-compatible loopback endpoint not configured"),
        selectedRuntimeProvider_ == QStringLiteral("llama-cpp-server") ? selectedLocalModel_
                                                                        : QString()};

    return RuntimeProviderRegistry{
        {ollamaProvider.descriptor(), openAiCompatibleLocalProvider.descriptor(),
         lmStudioProvider.descriptor(), llamaCppProvider.descriptor()},
        selectedRuntimeProvider_,
    };
}

ProviderCredentialRegistry ApplicationController::currentProviderCredentialRegistry() const {
    return defaultProviderCredentialRegistry(currentCredentialStore().summary());
}

CredentialStore ApplicationController::currentCredentialStore() const {
    return defaultCredentialStore();
}

ModelRegistry ApplicationController::currentModelRegistry() const {
    auto models = modelSummariesFromOllama(currentOllamaModels());
    models.append(disabledProviderModelPlaceholder(QStringLiteral("openai-compatible"),
                                                   QStringLiteral("OpenAI-Compatible API")));
    models.append(disabledProviderModelPlaceholder(QStringLiteral("lm-studio"),
                                                   QStringLiteral("LM Studio")));
    models.append(disabledProviderModelPlaceholder(QStringLiteral("llama-cpp-server"),
                                                   QStringLiteral("llama.cpp server")));
    models.append(disabledProviderModelPlaceholder(QStringLiteral("openai-compatible-local"),
                                                   QStringLiteral("OpenAI-compatible local endpoint")));
    models.append(disabledProviderModelPlaceholder(QStringLiteral("huggingface-catalog"),
                                                   QStringLiteral("Hugging Face metadata catalog")));
    models.append(disabledProviderModelPlaceholder(QStringLiteral("custom-catalog"),
                                                   QStringLiteral("Future custom catalogs")));
    const auto selectedModel =
        selectedRuntimeProvider_ == QStringLiteral("ollama") ? selectedLocalModel_ : QString();
    return ModelRegistry{models, selectedRuntimeProvider_, selectedModel};
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
    auto explicitModel = requestedModel.trimmed();
    if (!explicitModel.isEmpty()) {
        return explicitModel;
    }

    auto selectedModel = selectedLocalModel_.trimmed();
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
    if (succeeded) {
        latestMemoryRecallSummary_ = MemoryRecallSummary{};
        latestMemoryRecallSummary_.memoryEntryCount = 0;
    }
    emit memoryEntriesChanged();
    emit memoryRecallChanged();
    emit contextAssemblyChanged();
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
    latestConversationClearResult_.remainingMessageCount = toInt(chatSession_->messages().size());
    resetConversationRuntimeState();
    conversationStateGraph_.reset();
    refreshConversationHistorySummary();
    resetConversationSearchSummary();
    emit conversationStateChanged();
    emit conversationSearchChanged();
    emit chatMessagesChanged();
    emit contextAssemblyChanged();
    return persistentHealthy;
}

void ApplicationController::remember(const QString& key, const QString& value) {
    if (!memoryStore_ || key.trimmed().isEmpty()) {
        return;
    }

    memoryStore_->put(key.trimmed(), value.trimmed());
    refreshMemoryRecallForCurrentEntries();
    emit memoryEntriesChanged();
    emit memoryRecallChanged();
    emit contextAssemblyChanged();
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
