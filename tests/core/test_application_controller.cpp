#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/IChatHistoryStore.h"
#include "sentinel/core/InMemoryConversationStore.h"
#include "sentinel/core/InMemoryMemoryCandidateStore.h"
#include "sentinel/core/InMemoryStore.h"
#include "sentinel/core/LocalEchoProvider.h"
#include "sentinel/core/LocalInference.h"
#include "sentinel/core/ModelManagement.h"
#include "sentinel/core/NullAgentRuntime.h"
#include "sentinel/core/PiperTts.h"
#include "sentinel/core/RuntimePermissions.h"
#include "sentinel/core/SQLiteConversationStore.h"
#include "sentinel/core/Voice.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include <cstdint>
#include <memory>

using sentinel::core::ApplicationController;
using sentinel::core::ChatProviderReply;
using sentinel::core::ChatProviderStatus;
using sentinel::core::IChatHistoryStore;
using sentinel::core::IChatProvider;
using sentinel::core::IConversationStore;
using sentinel::core::ILocalInferenceClient;
using sentinel::core::ILocalInferenceStreamClient;
using sentinel::core::ILocalInferenceWorker;
using sentinel::core::InMemoryStore;
using sentinel::core::LocalEchoProvider;
using sentinel::core::LocalInferenceFinishedCallback;
using sentinel::core::LocalInferenceRequest;
using sentinel::core::LocalInferenceResponse;
using sentinel::core::LocalInferenceStatus;
using sentinel::core::LocalInferenceStreamChunk;
using sentinel::core::LocalInferenceStreamChunkCallback;
using sentinel::core::LocalInferenceStreamFinishedCallback;
using sentinel::core::LocalInferenceStreamResult;
using sentinel::core::LocalInferenceStreamStatus;
using sentinel::core::ModelManagementAction;
using sentinel::core::ModelManagementRequest;
using sentinel::core::OllamaConfig;
using sentinel::core::OllamaHealthCheckResult;
using sentinel::core::OllamaModelSummary;
using sentinel::core::SQLiteConversationStore;
using sentinel::core::StaticModelManagementService;

class UnavailableProvider final : public IChatProvider {
public:
    QString name() const override {
        return QStringLiteral("UnavailableProvider");
    }

    ChatProviderStatus status() const override {
        return ChatProviderStatus::Unavailable;
    }

    ChatProviderReply sendMessage(const QString& message) override {
        Q_UNUSED(message);
        return {false, {}, QStringLiteral("not available")};
    }
};

class ErrorProvider final : public IChatProvider {
public:
    QString name() const override {
        return QStringLiteral("ErrorProvider");
    }

    ChatProviderStatus status() const override {
        return ChatProviderStatus::Ready;
    }

    ChatProviderReply sendMessage(const QString& message) override {
        Q_UNUSED(message);
        return {false, {}, QStringLiteral("deterministic failure")};
    }
};

class RecordingChatHistoryStore final : public IChatHistoryStore {
public:
    explicit RecordingChatHistoryStore(QList<sentinel::core::ChatMessage> messages = {},
                                       bool available = true)
        : messages_(std::move(messages)), available_(available) {}

    QList<sentinel::core::ChatMessage> loadMessages() const override {
        return available_ ? messages_ : QList<sentinel::core::ChatMessage>{};
    }

    void appendMessage(const sentinel::core::ChatMessage& message) override {
        if (available_) {
            messages_.append(message);
        }
    }

    void clear() override {
        if (available_) {
            wasCleared_ = true;
            messages_.clear();
        }
    }

    bool isAvailable() const override {
        return available_;
    }

    QString lastError() const override {
        return available_ ? QString() : QStringLiteral("unavailable");
    }

    QList<sentinel::core::ChatMessage> messages_;
    bool available_ = true;
    bool wasCleared_ = false;
};

class FixedPlanRuntime final : public sentinel::core::IAgentRuntime {
public:
    FixedPlanRuntime(QList<sentinel::core::ToolDescriptor> tools,
                     sentinel::core::ToolInvocationPlan plan)
        : tools_(std::move(tools)), plan_(std::move(plan)) {}

    QString name() const override {
        return QStringLiteral("FixedPlanRuntime");
    }

    sentinel::core::AgentStatus status() const override {
        return sentinel::core::AgentStatus::Ready;
    }

    QList<sentinel::core::AgentCapabilityDescriptor> capabilities() const override {
        return {};
    }

    QList<sentinel::core::ToolDescriptor> availableTools() const override {
        return tools_;
    }

    sentinel::core::ToolInvocationPlan
    plan(const sentinel::core::AgentRequest& request) const override {
        Q_UNUSED(request);
        return plan_;
    }

    sentinel::core::AgentResponse execute(const sentinel::core::AgentRequest& request) override {
        return {
            true,
            QStringLiteral("Fixed local agent placeholder processed: %1")
                .arg(request.prompt.trimmed()),
            sentinel::core::AgentStatus::Ready,
        };
    }

private:
    QList<sentinel::core::ToolDescriptor> tools_;
    sentinel::core::ToolInvocationPlan plan_;
};

class UnavailableMemoryStore final : public sentinel::core::IMemoryStore {
public:
    void put(QString key, QString value) override {
        Q_UNUSED(key);
        Q_UNUSED(value);
    }

    QString get(const QString& key) const override {
        Q_UNUSED(key);
        return {};
    }

    sentinel::core::MemoryEntries entries() const override {
        return {};
    }

    void clear() override {}

    bool isAvailable() const override {
        return false;
    }

    QString lastError() const override {
        return QStringLiteral("unavailable");
    }
};

class AllowLocalInferencePolicy final : public sentinel::core::IRuntimePermissionPolicy {
public:
    sentinel::core::RuntimePermissionDecision
    evaluate(const sentinel::core::RuntimePermissionRequest& request) const override {
        return {
            sentinel::core::RuntimePermissionDecisionStatus::Allowed,
            request,
            QStringLiteral("Local inference permission allowed for injected test policy."),
        };
    }
};

class BlockLocalInferenceSafetyPolicy final : public sentinel::core::IRuntimeSafetyPolicy {
public:
    sentinel::core::RuntimeSafetyReport evaluate() const override {
        sentinel::core::RuntimeSafetyReport report;
        report.decision = sentinel::core::RuntimeSafetyDecision::Blocked;
        report.summary = QStringLiteral("Injected local safety policy blocked execution.");
        return report;
    }
};

class FakeLocalInferenceClient final : public ILocalInferenceClient {
public:
    LocalInferenceResponse infer(const LocalInferenceRequest& request) override {
        called = true;
        lastRequest = request;
        LocalInferenceResponse response;
        response.status = LocalInferenceStatus::Succeeded;
        response.model = request.options.model;
        response.endpoint = QStringLiteral("http://127.0.0.1:11434");
        response.text = QStringLiteral("fake local completion");
        response.summary = QStringLiteral("Fake local inference completed.");
        response.traces = {
            sentinel::core::LocalInferenceTrace{
                1,
                QStringLiteral("Fake Client"),
                QStringLiteral("Succeeded"),
                QStringLiteral("Injected fake local inference client was called."),
            },
        };
        return response;
    }

    QString statusSummary() const override {
        return QStringLiteral("Fake local inference client is ready.");
    }

    bool called = false;
    LocalInferenceRequest lastRequest;
};

class ErrorLocalInferenceClient final : public ILocalInferenceClient {
public:
    LocalInferenceResponse infer(const LocalInferenceRequest& request) override {
        called = true;
        LocalInferenceResponse response;
        response.status = LocalInferenceStatus::Error;
        response.model = request.options.model;
        response.error = sentinel::core::LocalInferenceError::ClientUnavailable;
        response.summary = QStringLiteral("Injected local inference failure.");
        return response;
    }

    QString statusSummary() const override {
        return QStringLiteral("Injected local inference client returns errors.");
    }

    bool called = false;
};

class CategorizedErrorLocalInferenceClient final : public ILocalInferenceClient {
public:
    CategorizedErrorLocalInferenceClient(sentinel::core::LocalInferenceError error, QString summary)
        : error_(error), summary_(std::move(summary)) {}

    LocalInferenceResponse infer(const LocalInferenceRequest& request) override {
        called = true;
        lastRequest = request;
        LocalInferenceResponse response;
        response.status = LocalInferenceStatus::Error;
        response.model = request.options.model;
        response.error = error_;
        response.summary = summary_;
        response.timeoutMs = request.options.timeoutMs;
        return response;
    }

    QString statusSummary() const override {
        return QStringLiteral("Injected categorized local inference client returns errors.");
    }

    bool called = false;
    LocalInferenceRequest lastRequest;

private:
    sentinel::core::LocalInferenceError error_;
    QString summary_;
};

class FakeLocalInferenceStreamClient final : public ILocalInferenceStreamClient {
public:
    explicit FakeLocalInferenceStreamClient(
        QList<LocalInferenceStreamChunk> chunks,
        LocalInferenceStreamStatus finalStatus = LocalInferenceStreamStatus::Completed,
        QString summary = QStringLiteral("Fake local stream completed."))
        : chunks_(std::move(chunks)), finalStatus_(finalStatus), summary_(std::move(summary)) {}

    LocalInferenceStreamResult
    startStream(const LocalInferenceRequest& request,
                const std::function<void(const LocalInferenceStreamChunk&)>& onChunk) override {
        called = true;
        lastRequest = request;
        LocalInferenceStreamResult result;
        result.status = finalStatus_;
        result.model = request.options.model;
        result.endpoint = QStringLiteral("http://127.0.0.1:11434");
        result.summary = summary_;
        for (const auto& chunk : chunks_) {
            result.chunks.append(chunk);
            if (chunk.malformed) {
                ++result.malformedChunkCount;
            } else {
                result.accumulatedText.append(chunk.text);
            }
            if (onChunk) {
                onChunk(chunk);
            }
        }
        if (finalStatus_ != LocalInferenceStreamStatus::Completed) {
            result.error = sentinel::core::LocalInferenceError::RequestFailed;
        }
        if (finalStatus_ == LocalInferenceStreamStatus::Error &&
            summary_.contains(QStringLiteral("did not complete"))) {
            result.error = sentinel::core::LocalInferenceError::StreamInterrupted;
        }
        if (finalStatus_ == LocalInferenceStreamStatus::Cancelled) {
            result.cancelled = true;
        }
        return result;
    }

    QString statusSummary() const override {
        return QStringLiteral("Fake local stream client is ready.");
    }

    bool isAvailable() const override {
        return true;
    }

    bool called = false;
    LocalInferenceRequest lastRequest;

private:
    QList<LocalInferenceStreamChunk> chunks_;
    LocalInferenceStreamStatus finalStatus_ = LocalInferenceStreamStatus::Completed;
    QString summary_;
};

class AsyncLocalInferenceWorker final : public ILocalInferenceWorker {
public:
    enum class Mode : std::uint8_t {
        Success,
        TimeoutError,
    };

    explicit AsyncLocalInferenceWorker(Mode mode = Mode::Success) : mode_(mode) {}

    bool startInference(const LocalInferenceRequest& request,
                        LocalInferenceFinishedCallback onFinished) override {
        if (busy) {
            return false;
        }
        busy = true;
        called = true;
        lastRequest = request;
        QTimer::singleShot(delayMs, [this, request, onFinished = std::move(onFinished)]() mutable {
            busy = false;
            LocalInferenceResponse response;
            response.model = request.options.model;
            response.endpoint = QStringLiteral("http://127.0.0.1:11434");
            response.timeoutMs = request.options.timeoutMs;
            if (mode_ == Mode::TimeoutError) {
                response.status = LocalInferenceStatus::Error;
                response.error = sentinel::core::LocalInferenceError::Timeout;
                response.summary =
                    QStringLiteral("Async fake local inference timed out after %1 ms.")
                        .arg(request.options.timeoutMs);
            } else {
                response.status = LocalInferenceStatus::Succeeded;
                response.text = QStringLiteral("async fake completion");
                response.summary = QStringLiteral("Async fake local inference completed.");
            }
            if (onFinished) {
                onFinished(request.id, response);
            }
        });
        return true;
    }

    bool startStream(const LocalInferenceRequest& request,
                     LocalInferenceStreamChunkCallback onChunk,
                     LocalInferenceStreamFinishedCallback onFinished) override {
        if (busy) {
            return false;
        }
        busy = true;
        called = true;
        lastRequest = request;
        QTimer::singleShot(delayMs, [this, request, onChunk = std::move(onChunk),
                                     onFinished = std::move(onFinished)]() mutable {
            busy = false;
            if (mode_ == Mode::Success && onChunk) {
                onChunk(request.id, LocalInferenceStreamChunk{
                                        1,
                                        QStringLiteral("async "),
                                        false,
                                        false,
                                        QStringLiteral("async chunk"),
                                    });
            }

            auto finish = [this, request, onFinished = std::move(onFinished)]() mutable {
                LocalInferenceStreamResult result;
                result.model = request.options.model;
                result.endpoint = QStringLiteral("http://127.0.0.1:11434");
                result.timeoutMs = request.options.timeoutMs;
                if (mode_ == Mode::TimeoutError) {
                    result.status = LocalInferenceStreamStatus::Error;
                    result.error = sentinel::core::LocalInferenceError::Timeout;
                    result.summary = QStringLiteral("Async fake local stream timed out.");
                } else {
                    result.status = LocalInferenceStreamStatus::Completed;
                    result.accumulatedText = QStringLiteral("async stream completion");
                    result.summary = QStringLiteral("Async fake local stream completed.");
                }
                if (onFinished) {
                    onFinished(request.id, result);
                }
            };

            if (streamFinishDelayMs > 0) {
                QTimer::singleShot(streamFinishDelayMs, std::move(finish));
            } else {
                finish();
            }
        });
        return true;
    }

    void cancel(const QString& requestId) override {
        cancelledRequestIds.append(requestId);
    }

    QString statusSummary() const override {
        return QStringLiteral("Async fake local inference worker is ready.");
    }

    QString streamStatusSummary() const override {
        return QStringLiteral("Async fake local stream worker is ready.");
    }

    bool streamingAvailable() const override {
        return true;
    }

    Mode mode_ = Mode::Success;
    int delayMs = 0;
    int streamFinishDelayMs = 0;
    bool busy = false;
    bool called = false;
    QStringList cancelledRequestIds;
    LocalInferenceRequest lastRequest;
};

class ReentrantLocalInferenceStreamClient final : public ILocalInferenceStreamClient {
public:
    LocalInferenceStreamResult
    startStream(const LocalInferenceRequest& request,
                const std::function<void(const LocalInferenceStreamChunk&)>& onChunk) override {
        called = true;
        lastRequest = request;
        if (onChunk) {
            onChunk(LocalInferenceStreamChunk{1, QStringLiteral("partial"), false, false,
                                              QStringLiteral("first chunk")});
        }
        if (controller) {
            duplicateAccepted = controller->sendMessage(QStringLiteral("duplicate"));
        }

        LocalInferenceStreamResult result;
        result.status = LocalInferenceStreamStatus::Completed;
        result.model = request.options.model;
        result.endpoint = QStringLiteral("http://127.0.0.1:11434");
        result.accumulatedText = QStringLiteral("final response");
        result.summary = QStringLiteral("Reentrant stream completed.");
        result.timeoutMs = request.options.timeoutMs;
        return result;
    }

    QString statusSummary() const override {
        return QStringLiteral("Reentrant local stream client is ready.");
    }

    bool isAvailable() const override {
        return true;
    }

    ApplicationController* controller = nullptr;
    bool called = false;
    bool duplicateAccepted = true;
    LocalInferenceRequest lastRequest;
};

class FakeOllamaRuntimeClient final : public sentinel::core::IOllamaRuntimeClient {
public:
    explicit FakeOllamaRuntimeClient(QList<OllamaModelSummary> models, OllamaConfig config = {})
        : models_(std::move(models)), config_(std::move(config)) {}

    OllamaConfig config() const override {
        return config_;
    }

    OllamaHealthCheckResult healthCheck() const override {
        OllamaHealthCheckResult result;
        if (!models_.isEmpty()) {
            result.connectionStatus = sentinel::core::OllamaConnectionStatus::Connected;
            result.healthStatus = sentinel::core::OllamaHealthStatus::Healthy;
        }
        result.summary = QStringLiteral("Fake Ollama health metadata.");
        return result;
    }

    QList<OllamaModelSummary> installedModels() const override {
        return models_;
    }

private:
    QList<OllamaModelSummary> models_;
    OllamaConfig config_;
};

class FakePiperTtsClient final : public sentinel::core::IPiperTtsClient {
public:
    enum class Mode : std::uint8_t {
        Success,
        Failure,
        Timeout,
    };

    explicit FakePiperTtsClient(Mode mode = Mode::Success) : mode_(mode) {}

    sentinel::core::PiperTtsStatus status() const override {
        return sentinel::core::PiperTtsStatus::Configured;
    }

    QString statusSummary() const override {
        return QStringLiteral("Fake Piper TTS client is deterministic.");
    }

    sentinel::core::PiperTtsResult
    synthesize(const sentinel::core::PiperTtsRequest& request,
               const sentinel::core::PiperTtsConfig& config) override {
        called = true;
        lastRequest = request;
        lastConfig = config;
        if (mode_ == Mode::Failure) {
            return {
                sentinel::core::PiperTtsStatus::Failed,
                false,
                request.outputPath,
                QStringLiteral("Controlled Piper TTS output path: %1").arg(request.outputPath),
                request.timeoutMs,
                2,
                QStringLiteral("fake failure"),
                QStringLiteral("Piper TTS failed while generating the controlled output file."),
                {QStringLiteral("Fake Piper failure path.")},
            };
        }
        if (mode_ == Mode::Timeout) {
            return {
                sentinel::core::PiperTtsStatus::Timeout,
                false,
                request.outputPath,
                QStringLiteral("Controlled Piper TTS output path: %1").arg(request.outputPath),
                request.timeoutMs,
                -1,
                QStringLiteral("fake timeout"),
                QStringLiteral("Piper TTS timed out before file-output synthesis completed."),
                {QStringLiteral("Fake Piper timeout path.")},
            };
        }

        return {
            sentinel::core::PiperTtsStatus::Succeeded,
            true,
            request.outputPath,
            QStringLiteral("Controlled Piper TTS output path: %1").arg(request.outputPath),
            request.timeoutMs,
            0,
            {},
            QStringLiteral("Piper TTS generated a local controlled audio file. Playback was not "
                           "started."),
            {QStringLiteral("Fake Piper wrote controlled output without playback.")},
        };
    }

    bool called = false;
    sentinel::core::PiperTtsRequest lastRequest;
    sentinel::core::PiperTtsConfig lastConfig;

private:
    Mode mode_ = Mode::Success;
};

class ApplicationControllerTest final : public QObject {
    Q_OBJECT

private slots:
    void exposesProviderNameAndInitialSystemMessage();
    void exposesProviderStatus();
    void exposesAgentStatusWithoutRuntime();
    void exposesModelRoutingMetadata();
    void exposesTaskPlanMetadata();
    void exposesAgentRegistryMetadata();
    void exposesProviderCatalogMetadata();
    void exposesMemoryCatalogMetadata();
    void exposesOrchestrationSnapshotMetadata();
    void exposesOrchestrationReadinessDiagnostics();
    void exposesAgentTaskRuntimeMetadata();
    void exposesLocalRuntimeMetadata();
    void exposesOllamaRuntimeBoundaryMetadata();
    void exposesLocalInferenceBoundaryMetadata();
    void exposesSelectedModelDefaultAndFallback();
    void modelManagementRecommendationsAreDeterministicAndActionsUnavailable();
    void exposesModelManagementReadinessMetadata();
    void exposesVoiceReadinessMetadata();
    void validatesConfiguredVoicePathsAsMetadataOnly();
    void piperFileOutputExecutionRequiresExplicitOptIn();
    void piperFileOutputExecutionUsesFakeClientForControlledSuccess();
    void piperFileOutputExecutionReportsFailureAndTimeout();
    void piperFileOutputExecutionBlocksInvalidBinaryOrModel();
    void rejectsInvalidSelectedModelAgainstDiscoveryMetadata();
    void exposesStreamingSkeletonDisabledMetadata();
    void streamDisabledByDefaultFallsBackToNonStreaming();
    void enabledLocalChatStreamingAppendsOrderedChunksOnce();
    void streamingPreviewClearsAfterFinalResponseIsPersisted();
    void enabledLocalChatStreamingHandlesMalformedChunk();
    void enabledLocalChatStreamingCancellationAppendsSafeRefusal();
    void streamingPreviewClearsAfterStreamError();
    void asyncLocalChatInferenceCompletesWithFakeWorker();
    void asyncLocalChatStreamingCompletesWithFakeWorker();
    void asyncLocalChatInferenceErrorResetsBusy();
    void asyncSuccessUpdatesConversationRuntimeState();
    void asyncFailureUpdatesConversationRuntimeErrorWithoutCorruptingHistory();
    void clearChatClearsConversationRuntimeAndPersistence();
    void reportsPersistedConversationHistorySummary();
    void reportsRuntimeOnlyConversationHistorySummary();
    void exposesConversationStoreReadinessWithoutSwitchingTranscriptStorage();
    void createsSwitchesRenamesArchivesAndUnarchivesConversations();
    void archivedActiveConversationBlocksSend();
    void archiveUnarchiveUpdatesBrowserSummaries();
    void pinPersistsAcrossControllerRecreation();
    void unpinPersistsAcrossControllerRecreation();
    void duplicateConversationCopiesLocalMessages();
    void conversationOrderingIsPinnedRecentArchivedDeterministic();
    void deleteReadinessIsDisabledByDefault();
    void deleteRequestRefusesSafelyWithoutMutation();
    void deleteRequestRefusesSafelyWithoutSQLiteMutation();
    void activeConversationRemainsValidAfterArchiveUnarchive();
    void persistsActiveConversationAcrossControllerRecreation();
    void switchingConversationIgnoresStaleAsyncResultAndResetsBusy();
    void switchingConversationDoesNotDuplicateTranscriptInsertion();
    void reportsSingleConversationBrowserEntryDeterministically();
    void reportsEmptyTranscriptConversationBrowserSummary();
    void reportsConversationBrowserMessageCountSummary();
    void clearChatUpdatesConversationBrowserEntry();
    void conversationBrowserReflectsSearchAndExportAvailability();
    void reportsMultiConversationPlanningReadinessWithoutStorageMutation();
    void inMemoryConversationSearchFindsUserAndAssistantMessages();
    void emptyConversationSearchDoesNotMutateHistory();
    void conversationSearchDoesNotMutateHistory();
    void clearChatResetsConversationSearchSummary();
    void markdownConversationExportWritesCurrentTranscript();
    void jsonConversationExportWritesValidStructure();
    void emptyConversationExportIsRefused();
    void conversationExportUsesSanitizedTimestampedFilenames();
    void unsupportedConversationExportFormatWritesNoFile();
    void clearChatResetsStreamingAndActiveRequestMetadata();
    void clearChatKeepsSingleInitialSystemMessage();
    void asyncDuplicateSendIsRejectedBeforeAppending();
    void staleAsyncResultIsIgnoredAfterCancellation();
    void localInferenceTimeoutAppendsConciseFailureAndResetsBusy();
    void localInferenceMalformedResponseAppendsConciseFailureAndResetsBusy();
    void ollamaUnavailablePathAppendsConciseFailureWithoutRealService();
    void duplicateSendDuringActiveStreamIsRejectedWithoutAppending();
    void streamingInterruptionClearsPreviewAndAppendsOneFailure();
    void blocksLocalInferenceByDefaultPermission();
    void blocksLocalInferenceWhenSafetyPolicyBlocks();
    void runsInjectedLocalInferenceWhenPermissionAllows();
    void usesProviderChatPathWhenLocalChatInferenceDisabled();
    void enabledLocalChatInferenceWithoutValidModelFailsSafely();
    void enabledLocalChatInferenceWithInvalidModelShowsSafeSummary();
    void enabledLocalChatInferenceAppendsFakeResponse();
    void enabledLocalChatInferenceErrorAppendsSafeRefusal();
    void localChatInferenceBlocksNonLoopbackEndpoint();
    void exposesConversationSessionMetadata();
    void exposesConversationStateMetadata();
    void updatesModelRoutingModeMetadata();
    void keepsConversationSessionSeparateFromChatAndRuntimeSessions();
    void executesDeterministicAgentRequestWithRuntime();
    void exposesAgentToolMetadata();
    void exposesLatestToolPlanStatusWithRuntime();
    void exposesLatestApprovalStatusWithRuntime();
    void exposesLatestSandboxStatusWithRuntime();
    void exposesLatestToolExecutionStatusWithRuntime();
    void exposesSuccessfulPipelineResultWithRuntime();
    void exposesRuntimeContextForPipelineResult();
    void exposesAgentActivityForPipelineResult();
    void reportsRiskyToolPlanRequiresApproval();
    void reportsSandboxBlockedPipelineResult();
    void reportsEmptyPlanPipelineResult();
    void reportsUnknownToolPipelineResult();
    void exposesMemoryStatus();
    void sendsMessageThroughProvider();
    void updatesConversationStateForChatFlow();
    void ignoresBlankChatMessages();
    void handlesUnavailableProvider();
    void handlesProviderErrorReply();
    void clearsChatHistory();
    void loadsPersistedChatHistoryAtStartup();
    void appendsNewChatMessagesToHistoryStore();
    void clearsPersistentChatHistoryWhenAvailable();
    void keepsRuntimeChatWorkingWhenHistoryStoreUnavailable();
    void storesRuntimeMemoryEntries();
    void createsMemoryCandidatesPendingReview();
    void recordsMemoryCandidateApprovalAndRejectionMetadata();
    void resetsMemoryCandidateReviewMetadata();
    void refusesInvalidMemoryCandidateReviewTransitions();
    void archivesReviewedMemoryCandidatesAsTerminalMetadata();
    void memoryCandidatesDoNotMutateKeyValueMemory();
    void approvedMemoryCandidateCommitsToKeyValueMemory();
    void pendingAndRejectedMemoryCandidatesCannotCommit();
    void archivedMemoryCandidateCannotCommit();
    void duplicateMemoryCommitKeyRefusesByDefault();
    void approvalDoesNotAutomaticallyCommitMemory();
    void localMemoryRecallFindsCommittedKeyValueMemory();
    void emptyLocalMemoryRecallDoesNotMutateMemory();
    void localMemoryRecallAfterCommitFindsCommittedCandidate();
    void clearChatKeepsCommittedMemoryRecallAvailable();
    void localMemoryRecallDoesNotInjectIntoPrompt();
    void contextAssemblyShowsCommittedMemorySource();
    void contextAssemblyPlanningDoesNotMutateOrInjectPrompt();
    void promptContextInjectionDisabledLeavesPromptUnchanged();
    void enabledPromptContextInjectionIncludesDeterministicBundle();
    void promptContextInjectionUsesOnlyCommittedMemory();
    void promptContextInjectionBudgetTruncatesDeterministically();
    void promptContextInjectionUsesBoundedRecentConversationWindow();
    void conversationSummaryUsesOlderWindowAndStaysSeparateFromMemory();
    void conversationSummaryPlanningDoesNotMutateState();
    void retrievalPlanningFeedsPromptContextWithoutMixingSources();
    void retrievalPlanningDoesNotMutateState();
    void semanticRetrievalMetadataDoesNotAffectPlanningOrPrompt();
    void semanticProviderPlanningExposesDisabledSelection();
    void semanticCandidateOrchestrationExposesSafeMetadata();
    void semanticArbitrationSimulationExposesSafeMetadata();
    void isolatedEmbeddingRuntimeExposureIsBoundedAndDisabledByDefault();
    void vectorPersistenceExposureIsDisabledAndDoesNotMutatePlanningOrPrompt();
    void semanticSearchExposureIsBoundedAndDoesNotMutatePlanningOrPrompt();
    void hybridBridgeExposureIsBoundedAndDoesNotMutatePlanningOrPrompt();
    void semanticAcceptanceExposureIsBoundedAndDoesNotMutatePlanningOrPrompt();
    void semanticSupplementAssemblyExposureIsDisabledAndDoesNotMutatePlanningOrPrompt();
    void semanticPromptAuthorityExposureIsDefaultDeniedAndDoesNotMutatePrompt();
    void semanticCandidateOrchestrationDoesNotMutatePrompt();
    void promptContextInjectionDoesNotMutateMemoryOrCandidates();
    void promptContextInjectionRespectsSafetyGateBeforeAssembly();
    void clearChatKeepsApprovedMemoryCandidateMetadata();
    void clearChatKeepsCommittedMemory();
    void clearsRuntimeMemoryEntries();
    void failsSafeWhenMemoryStoreUnavailable();
    void rejectsBlankMemoryKeys();
    void overwritesMemoryEntriesThroughStoreBackend();
    void reportsRuntimeOnlyWhenChatStoreUnavailableOnClear();
};

static std::unique_ptr<ApplicationController> makeController() {
    return std::make_unique<ApplicationController>(std::make_unique<LocalEchoProvider>(),
                                                   std::make_unique<InMemoryStore>());
}

static std::unique_ptr<ApplicationController>
makeControllerWithConversationStore(std::unique_ptr<IConversationStore> conversationStore) {
    return std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        std::move(conversationStore));
}

struct PiperControllerFixture {
    std::unique_ptr<ApplicationController> controller;
    FakePiperTtsClient* client = nullptr;
};

static PiperControllerFixture makePiperController(FakePiperTtsClient::Mode mode) {
    auto client = std::make_unique<FakePiperTtsClient>(mode);
    auto* clientPtr = client.get();
    auto provider = std::make_unique<sentinel::core::PiperTextToSpeechProvider>(
        sentinel::core::defaultDisabledPiperTtsConfig(), std::move(client));
    return {
        std::make_unique<ApplicationController>(
            std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, nullptr, std::move(provider)),
        clientPtr,
    };
}

static void configureReadyPiperPaths(ApplicationController& controller, QTemporaryDir& dir) {
    const auto piperBinaryPath = dir.filePath(QStringLiteral("piper"));
    const auto piperModelPath = dir.filePath(QStringLiteral("voice.onnx"));

    QFile piperBinary{piperBinaryPath};
    QVERIFY(piperBinary.open(QIODevice::WriteOnly));
    piperBinary.write("#!/bin/sh\nexit 0\n");
    piperBinary.close();
    QVERIFY(QFile::setPermissions(
        piperBinaryPath, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner));

    QFile piperModel{piperModelPath};
    QVERIFY(piperModel.open(QIODevice::WriteOnly));
    piperModel.write("model");
    piperModel.close();

    controller.setPiperBinaryPath(piperBinaryPath);
    controller.setPiperModelPath(piperModelPath);
}

static std::unique_ptr<ApplicationController>
makeAsyncWorkerController(std::unique_ptr<ILocalInferenceWorker> worker) {
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, std::move(worker));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    return controller;
}

void ApplicationControllerTest::exposesProviderNameAndInitialSystemMessage() {
    const auto controller = makeController();

    QCOMPARE(controller->providerName(), QStringLiteral("LocalEchoProvider"));
    QCOMPARE(controller->chatMessages().size(), 1);
    QCOMPARE(controller->chatMessages().first(), QStringLiteral("Sentinel: Sentinel Core online."));
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->chatHistory().first().role, sentinel::core::ChatRole::System);
    QVERIFY(controller->memoryEntries().isEmpty());
}

void ApplicationControllerTest::exposesProviderStatus() {
    const auto controller = makeController();

    QCOMPARE(controller->providerStatus(), QStringLiteral("Ready"));
}

void ApplicationControllerTest::exposesAgentStatusWithoutRuntime() {
    const auto controller = makeController();

    QCOMPARE(controller->agentStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(controller->lastAgentResponse(), QStringLiteral("No agent request yet."));
    QCOMPARE(controller->latestToolPlanStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller->latestToolPlanSummary(), QStringLiteral("No tool plan yet."));
    QCOMPARE(controller->latestApprovalStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller->latestApprovalSummary(), QStringLiteral("No approval decision yet."));
    QCOMPARE(controller->latestSandboxStatus(), QStringLiteral("Not Evaluated"));
    QCOMPARE(controller->latestSandboxSummary(), QStringLiteral("No sandbox evaluation yet."));
    QCOMPARE(controller->latestToolExecutionStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller->latestToolExecutionSummary(),
             QStringLiteral("No tool execution boundary result yet."));
    QCOMPARE(controller->runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller->runtimeContextStatus(), QStringLiteral("Empty"));
    QCOMPARE(controller->runtimeContextSummary(), QStringLiteral("No runtime context yet."));
    QVERIFY(controller->runtimeContextActiveToolIds().isEmpty());
    QCOMPARE(controller->agentActivityCount(), 0);
    QCOMPARE(controller->latestAgentActivitySummary(), QStringLiteral("No agent activity yet."));

    const auto ran = controller->runAgentRequest(QStringLiteral("plan"));

    QVERIFY(!ran);
    QCOMPARE(controller->lastAgentResponse(), QStringLiteral("Agent runtime unavailable."));
    QCOMPARE(controller->agentActivityCount(), 2);
    QCOMPARE(controller->latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline blocked: runtime unavailable."));
}

void ApplicationControllerTest::exposesModelRoutingMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->currentRoutingMode(), QStringLiteral("Local Only"));
    QCOMPARE(controller->modelRoutingStatus(), QStringLiteral("Routed"));
    QCOMPARE(controller->selectedModelProviderSummary(),
             QStringLiteral("Local Only -> Local Metadata Provider / Sentinel Local Placeholder"));
}

void ApplicationControllerTest::exposesTaskPlanMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->latestTaskPlanStatus(), QStringLiteral("Fallback Planned"));
    QCOMPARE(controller->plannedTaskStepCount(), 2);
    QCOMPARE(controller->latestTaskPlanSummary(),
             QStringLiteral("Unknown task uses safe local metadata fallback: Local Metadata "
                            "Provider / Sentinel Local Placeholder."));
    QCOMPARE(controller->currentMemoryAffinitySummary(),
             QStringLiteral("Ambient (Available, Public Metadata, Session)"));
}

void ApplicationControllerTest::exposesAgentRegistryMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->registeredAgentCount(), 6);
    QCOMPARE(controller->activeAgentSummaries().size(), 6);
    QVERIFY(controller->activeAgentSummaries().contains(
        QStringLiteral("Atlas (Coordinator, Available, Local)")));
    QVERIFY(controller->activeAgentSummaries().contains(
        QStringLiteral("Vela (Researcher, Available, Cloud)")));
    QCOMPARE(controller->currentAgentSummary(),
             QStringLiteral("Atlas (Coordinator, Available, Local)"));
}

void ApplicationControllerTest::exposesProviderCatalogMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->providerCatalogCount(), 4);
    QCOMPARE(controller->providerCatalogSummaries().size(), 4);
    QVERIFY(controller->providerCatalogSummaries().contains(
        QStringLiteral("Local Metadata Provider (Local, Available)")));
    QVERIFY(controller->providerCatalogSummaries().contains(
        QStringLiteral("OpenAI Cloud (Cloud, Not Configured)")));
}

void ApplicationControllerTest::exposesMemoryCatalogMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->memoryCatalogCount(), 5);
    QCOMPARE(controller->memoryCatalogSummaries().size(), 5);
    QVERIFY(controller->memoryCatalogSummaries().contains(
        QStringLiteral("Episodic (Available, Private, User Controlled)")));
    QVERIFY(controller->memoryCatalogSummaries().contains(
        QStringLiteral("Semantic (Available, Local Only, Durable)")));
    QCOMPARE(controller->currentMemoryAffinitySummary(),
             QStringLiteral("Ambient (Available, Public Metadata, Session)"));
}

void ApplicationControllerTest::exposesOrchestrationSnapshotMetadata() {
    const auto controller = makeController();
    const auto snapshot = controller->currentOrchestrationSnapshot();

    QCOMPARE(snapshot.healthStatus, sentinel::core::OrchestrationHealthStatus::Ready);
    QCOMPARE(snapshot.workspace.routingMode, QStringLiteral("Local Only"));
    QCOMPARE(snapshot.workspace.routingStatus, QStringLiteral("Routed"));
    QCOMPARE(snapshot.workspace.taskPlanStatus, QStringLiteral("Fallback Planned"));
    QCOMPARE(snapshot.workspace.providerCatalogCount, 4);
    QCOMPARE(snapshot.workspace.registeredAgentCount, 6);
    QCOMPARE(snapshot.workspace.memoryCatalogCount, 5);
    QCOMPARE(snapshot.workspace.preferredAgentSummary,
             QStringLiteral("Atlas (Coordinator, Available, Local)"));
    QCOMPARE(snapshot.workspace.memoryAffinitySummary,
             QStringLiteral("Ambient (Available, Public Metadata, Session)"));
    QVERIFY(!snapshot.executionEnabled);
    QCOMPARE(controller->orchestrationSnapshotStatus(), QStringLiteral("Ready"));
    QVERIFY(controller->orchestrationSnapshotSummary().contains(
        QStringLiteral("4 provider entries, 6 agents, 5 memory categories")));
    QVERIFY(controller->orchestrationSignals().contains(
        QStringLiteral("Catalogs: 4 providers / 6 agents / 5 memory")));
}

void ApplicationControllerTest::exposesOrchestrationReadinessDiagnostics() {
    const auto controller = makeController();
    const auto report = controller->currentOrchestrationReadinessReport();

    QCOMPARE(report.status, QStringLiteral("Ready"));
    QCOMPARE(report.checks.size(), 10);
    QCOMPARE(controller->orchestrationReadinessStatus(), QStringLiteral("Ready"));
    QCOMPARE(controller->orchestrationReadinessSummary(),
             QStringLiteral("Ready orchestration readiness: 10 deterministic metadata checks, 10 "
                            "diagnostic entries."));
    QVERIFY(controller->orchestrationDiagnostics().contains(
        QStringLiteral("Info: Routing Mode - Local Only routing mode is set.")));
    QVERIFY(controller->orchestrationDiagnostics().contains(
        QStringLiteral("Info: Cloud Providers - Cloud provider metadata remains not configured.")));
    QVERIFY(controller->orchestrationDiagnostics().contains(
        QStringLiteral("Info: Execution Capability - Execution capability remains disabled.")));
}

void ApplicationControllerTest::exposesAgentTaskRuntimeMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->agentTaskRuntimeStatus(), QStringLiteral("Refusing Execution"));
    QCOMPARE(controller->agentTaskRuntimeTaskCount(), 6);
    QCOMPARE(controller->agentTaskQueueCount(), 6);
    QCOMPARE(controller->agentTaskQueuePlannedCount(), 6);
    QCOMPARE(controller->agentTaskQueueActiveCount(), 0);
    QCOMPARE(controller->agentTaskQueueBlockedCount(), 0);
    QCOMPARE(controller->agentTaskQueueCompletedCount(), 0);
    QCOMPARE(controller->agentTaskQueueRefusedCount(), 0);
    QVERIFY(controller->agentTaskRuntimeSummary().contains(QStringLiteral("refuses execution")));
    QVERIFY(controller->latestAgentTaskSummary().contains(QStringLiteral("Prepare Export Action")));
    QVERIFY(controller->latestAgentTaskLifecycleSummary().contains(QStringLiteral("queued")));
    QCOMPARE(controller->agentTaskQueueSummaries().size(), 6);
    QCOMPARE(controller->agentTaskTraceSummaries().size(), 3);
    QVERIFY(controller->agentTaskTraceSummaries().last().contains(
        QStringLiteral("Execution Boundary")));
    QCOMPARE(controller->agentPlanningSessionStatus(), QStringLiteral("Ready"));
    QCOMPARE(controller->agentPlanningCandidateCount(), 6);
    QCOMPARE(controller->agentPlanningRefusedCount(), 0);
    QVERIFY(controller->agentPlanningSessionSummary().contains(QStringLiteral("execution "
                                                                               "attempted: no")));
    QCOMPARE(controller->agentPlanningCandidateSummaries().size(), 6);
    QCOMPARE(controller->agentPlanningArbitrationSummaries().size(), 6);
    QVERIFY(controller->agentPlanningFallbackSummary().contains(QStringLiteral("No planning "
                                                                               "fallback")));
    QCOMPARE(controller->agentCapabilityRegistryStatus(),
             QStringLiteral("Refusing Unsafe Capabilities"));
    QCOMPARE(controller->agentCapabilityCount(), 9);
    QCOMPARE(controller->agentCapabilityEnabledCount(), 6);
    QCOMPARE(controller->agentCapabilityDisabledCount(), 2);
    QCOMPARE(controller->agentCapabilityRestrictedCount(), 3);
    QVERIFY(controller->agentCapabilityRegistrySummary().contains(QStringLiteral("execution "
                                                                                 "attempted: no")));
    QCOMPARE(controller->agentCapabilitySummaries().size(), 9);
    QVERIFY(controller->agentCapabilitySummaries().at(7).contains(QStringLiteral("Shell "
                                                                                 "Execution")));
    QCOMPARE(controller->agentCapabilityReadinessSummaries().size(), 9);
    QCOMPARE(controller->agentCapabilitySafetySummaries().size(), 9);
    QVERIFY(controller->agentCapabilitySafetySummaries().at(7).contains(QStringLiteral("execution "
                                                                                       "attempted: "
                                                                                       "no")));
    QCOMPARE(controller->toolContractRegistryStatus(), QStringLiteral("Refusing Unsafe Contracts"));
    QCOMPARE(controller->toolContractCount(), 10);
    QCOMPARE(controller->toolContractEnabledCount(), 6);
    QCOMPARE(controller->toolContractDisabledCount(), 3);
    QCOMPARE(controller->toolContractRestrictedCount(), 4);
    QVERIFY(controller->toolContractRegistrySummary().contains(QStringLiteral("execution "
                                                                              "attempted: no")));
    QCOMPARE(controller->toolContractSummaries().size(), 10);
    QVERIFY(controller->toolContractSummaries().at(7).contains(QStringLiteral("Future Subprocess "
                                                                              "Execution")));
    QCOMPARE(controller->toolContractPermissionSummaries().size(), 10);
    QCOMPARE(controller->toolContractSandboxSummaries().size(), 10);
    QCOMPARE(controller->toolContractReadinessSummaries().size(), 10);
    QCOMPARE(controller->toolContractSafetySummaries().size(), 10);
    QVERIFY(controller->toolContractPermissionSummaries().at(6).contains(
        QStringLiteral("future filesystem access")));
    QVERIFY(controller->toolContractSandboxSummaries().at(7).contains(QStringLiteral("Denied")));
    QVERIFY(controller->toolContractSafetySummaries().at(7).contains(QStringLiteral("execution "
                                                                                    "attempted: "
                                                                                    "no")));
}

void ApplicationControllerTest::exposesLocalRuntimeMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->localRuntimeStatus(), QStringLiteral("Metadata Only"));
    QCOMPARE(controller->localRuntimeHealth(), QStringLiteral("Not Executable"));
    QCOMPARE(controller->localRuntimeSummary(),
             QStringLiteral("Null Local Runtime is metadata-only; local inference execution is "
                            "disabled."));
    QCOMPARE(controller->localRuntimeResponseStatus(), QStringLiteral("Refused"));
    QCOMPARE(controller->localRuntimeResponseSummary(),
             QStringLiteral("Local runtime boundary is metadata-only; execution is disabled."));
    QCOMPARE(controller->localRuntimeCapabilities().size(), 3);
    QVERIFY(controller->localRuntimeCapabilities().contains(
        QStringLiteral("Local Inference (Disabled): Inference execution is intentionally "
                       "disabled.")));
    QCOMPARE(controller->localRuntimeSessionCount(), 1);
    QCOMPARE(controller->localRuntimeSessionStatus(), QStringLiteral("Reserved"));
    QCOMPARE(controller->localRuntimeSessionHealth(), QStringLiteral("Placeholder Only"));
    QCOMPARE(controller->localRuntimeSessionSummary(),
             QStringLiteral("local-runtime-session-1: Reserved placeholder local runtime "
                            "metadata."));
    QCOMPARE(controller->localRuntimeAllocationSummary(),
             QStringLiteral("Metadata-only local runtime allocation; no model or process is "
                            "started."));
    QCOMPARE(controller->localRuntimeReservationSummary(),
             QStringLiteral("Placeholder reservation is held for metadata visibility only."));
    QCOMPARE(controller->localRuntimeSessionSummaries(),
             QStringList{QStringLiteral("local-runtime-session-1: Reserved placeholder local "
                                        "runtime metadata.")});
    QCOMPARE(controller->runtimeCapabilityCount(), 13);
    QCOMPARE(controller->enabledRuntimeCapabilitySummaries().size(), 2);
    QVERIFY(controller->enabledRuntimeCapabilitySummaries().contains(
        QStringLiteral("Privacy-Safe Mode (Security, Enabled): Privacy-safe runtime metadata "
                       "mode is active.")));
    QCOMPARE(controller->disabledRuntimeCapabilitySummaries().size(), 11);
    QVERIFY(controller->disabledRuntimeCapabilitySummaries().contains(
        QStringLiteral("Tool Bridge (Integration, Unavailable): Tool bridge execution is "
                       "unavailable.")));
    QCOMPARE(controller->runtimeNegotiationProfileSummary(),
             QStringLiteral("Metadata-only negotiation profile; no runtime capability is "
                            "activated."));
    QCOMPARE(controller->runtimeNegotiationSummary(),
             QStringLiteral("Metadata-only runtime negotiation: 13 capabilities described, 2 "
                            "enabled as safety metadata."));
    QCOMPARE(controller->localOnlyRuntimeEnforcementSummary(),
             QStringLiteral("Local-only enforcement is active; cloud relay and external runtime "
                            "execution remain unavailable."));
    QCOMPARE(controller->runtimePermissionDecision(), QStringLiteral("Denied"));
    QCOMPARE(controller->runtimePermissionSummary(),
             QStringLiteral("Runtime permission policy is metadata-only and denies execution by "
                            "default."));
    QCOMPARE(controller->runtimeSafetyDecision(), QStringLiteral("Compliant"));
    QCOMPARE(controller->runtimeSafetySummary(),
             QStringLiteral("Runtime safety policy report: local-only and no-execution posture is "
                            "enforced with deterministic metadata rules."));
    QCOMPARE(controller->runtimePipelineStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->runtimePipelineSummary(),
             QStringLiteral("Runtime request pipeline blocked execution metadata by permission and "
                            "safety policy."));
    QCOMPARE(controller->runtimePipelineTraceSummaries().size(), 4);
    QVERIFY(controller->runtimePipelineTraceSummaries().contains(
        QStringLiteral("Request Received [Metadata Received]: Runtime request pipeline metadata "
                       "evaluation.")));
    QVERIFY(controller->runtimePipelineTraceSummaries().contains(
        QStringLiteral("Execution Boundary [Blocked]: Execution boundary remained blocked; no "
                       "runtime action was performed.")));
    QCOMPARE(controller->executionLifecycleState(), QStringLiteral("Blocked"));
    QCOMPARE(controller->executionLifecycleStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->executionLifecycleSummary(),
             QStringLiteral("Execution lifecycle reached blocked metadata state; no execution is "
                            "permitted."));
    QCOMPARE(controller->executionLifecycleTraceSummaries().size(), 7);
    QVERIFY(controller->executionLifecycleTraceSummaries().contains(
        QStringLiteral("6. Ready Placeholder [Warning]: Ready placeholder is descriptive and not "
                       "executable.")));
    QVERIFY(controller->executionLifecycleTraceSummaries().contains(
        QStringLiteral("7. Blocked [Blocked]: Execution remains intentionally blocked.")));
    QCOMPARE(controller->executionSessionId(), QStringLiteral("execution-session-1"));
    QCOMPARE(controller->executionSessionStatus(), QStringLiteral("Reserved"));
    QCOMPARE(controller->executionSessionOwnership(), QStringLiteral("Application Controller"));
    QCOMPARE(controller->executionCoordinationMode(), QStringLiteral("Metadata Only"));
    QCOMPARE(controller->executionSessionSummary(),
             QStringLiteral("Execution session is reserved for metadata only."));
    QCOMPARE(controller->executionCoordinationSnapshotSummary(),
             QStringLiteral("Execution coordination snapshot is read-only for "
                            "execution-session-1; lifecycle is Blocked and execution is "
                            "blocked."));
    QCOMPARE(controller->localRuntimeAdapterStatus(), QStringLiteral("Placeholder"));
    QCOMPARE(controller->localRuntimeAdapterHealth(), QStringLiteral("Metadata Only"));
    QCOMPARE(controller->localRuntimeAdapterSummary(),
             QStringLiteral("Ollama local runtime adapter contract is metadata-only; no runtime "
                            "connection is configured."));
    QCOMPARE(controller->localRuntimeAdapterCapabilitySummaries().size(), 3);
    QVERIFY(controller->localRuntimeAdapterCapabilitySummaries().contains(
        QStringLiteral("Model Discovery (Unavailable, Not Executable): Model discovery is "
                       "intentionally disabled.")));
    QCOMPARE(controller->providerRuntimeBridgeStatus(), QStringLiteral("Not Connected"));
    QCOMPARE(controller->providerRuntimeBridgeSummary(),
             QStringLiteral("Provider runtime bridge is not connected and cannot execute provider "
                            "requests."));
    QCOMPARE(controller->providerRuntimeBridgeResponseSummary(),
             QStringLiteral("Provider runtime bridge is metadata-only; no provider or local "
                            "runtime request was executed."));
    QCOMPARE(controller->runtimeIntegrationReadinessStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->runtimeIntegrationReadinessSummary(),
             QStringLiteral("Runtime integration readiness is blocked: Ollama local "
                            "health/discovery metadata is available, but provider bridge "
                            "execution and inference remain disabled."));
    QCOMPARE(controller->runtimeIntegrationReadinessChecks().size(), 5);
    QVERIFY(controller->runtimeIntegrationReadinessChecks().contains(
        QStringLiteral("Pass: Endpoint Configuration - Safe local Ollama endpoint is configured "
                       "for loopback-only health checks.")));
    QVERIFY(controller->runtimeIntegrationReadinessChecks().contains(
        QStringLiteral("Pass: Model Discovery - Installed model discovery boundary is available "
                       "for read-only local metadata.")));
    QVERIFY(controller->runtimeIntegrationReadinessChecks().contains(
        QStringLiteral("Blocked: Execution Permission - Execution lifecycle, runtime permission, "
                       "safety, and pipeline boundaries still block execution.")));
}

void ApplicationControllerTest::exposesOllamaRuntimeBoundaryMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->ollamaEndpoint(), QStringLiteral("http://127.0.0.1:11434"));
    QCOMPARE(controller->ollamaConnectionStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(controller->ollamaHealthStatus(), QStringLiteral("Unavailable"));
    QVERIFY(controller->ollamaHealthSummary().contains(QStringLiteral("no local health check")));
    QCOMPARE(controller->ollamaModelCount(), 0);
    QVERIFY(controller->ollamaModelSummaries().isEmpty());
}

void ApplicationControllerTest::exposesLocalInferenceBoundaryMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Not Requested"));
    QVERIFY(controller->localInferenceSummary().contains(QStringLiteral("loopback-only")));
    QCOMPARE(controller->localInferenceLastResponseSummary(),
             QStringLiteral("No local inference request yet."));
    QCOMPARE(controller->localInferenceRuntimeState(), QStringLiteral("Unavailable"));
    QCOMPARE(controller->localInferenceLatencySummary(),
             QStringLiteral("No local inference latency recorded."));
    QVERIFY(controller->localInferenceTraceSummaries().isEmpty());
}

void ApplicationControllerTest::exposesSelectedModelDefaultAndFallback() {
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        nullptr);

    QVERIFY(controller->selectedLocalModel().isEmpty());
    QCOMPARE(controller->ollamaModelNames(), QStringList{QStringLiteral("llama3.2")});
    QCOMPARE(controller->ollamaModelSummaries(),
             QStringList{QStringLiteral("llama3.2 (10 B, Local Only)")});
    QCOMPARE(controller->selectedLocalModelStatus(), QStringLiteral("Missing"));
    QCOMPARE(controller->selectedLocalModelSummary(),
             QStringLiteral("No local model selected. Choose an installed Ollama model in "
                            "Settings before sending."));
    QCOMPARE(controller->selectedLocalModelMetadataSummary(),
             QStringLiteral("Fallback model: llama3.2 (10 B, Local Only)"));
    QCOMPARE(controller->activeLocalRuntimeBadge(), QStringLiteral("Ollama Local / llama3.2"));

    controller->setSelectedLocalModel(QStringLiteral(" llama3.2 "));

    QCOMPARE(controller->selectedLocalModel(), QStringLiteral("llama3.2"));
    QCOMPARE(controller->selectedLocalModelStatus(), QStringLiteral("Available"));
    QCOMPARE(controller->selectedLocalModelSummary(),
             QStringLiteral("Selected local model llama3.2 is available in local discovery "
                            "metadata."));
    QCOMPARE(controller->selectedLocalModelMetadataSummary(),
             QStringLiteral("Selected model: llama3.2 (10 B, Local Only)"));
    QCOMPARE(controller->activeLocalRuntimeBadge(), QStringLiteral("Ollama Local / llama3.2"));
}

void ApplicationControllerTest::
    modelManagementRecommendationsAreDeterministicAndActionsUnavailable() {
    StaticModelManagementService service;

    QCOMPARE(modelManagementStatusName(service.status()), QStringLiteral("Metadata Only"));
    const auto recommendations = service.recommendations();
    QCOMPARE(recommendations.size(), 3);
    QCOMPARE(recommendations.at(0).modelName, QStringLiteral("llama3.2:3b"));
    QCOMPARE(recommendations.at(1).modelName, QStringLiteral("mistral:7b"));
    QCOMPARE(recommendations.at(2).modelName, QStringLiteral("qwen2.5-coder:7b"));

    const auto requirements = service.requirementSummaries();
    QCOMPARE(requirements.size(), 3);
    QCOMPARE(requirements.at(0).approximateRam, QStringLiteral("about 8 GB"));
    QCOMPARE(requirements.at(0).approximateDisk, QStringLiteral("about 3 GB"));

    const auto result = service.evaluate(
        ModelManagementRequest{ModelManagementAction::Pull, QStringLiteral("llama3.2:3b")});
    QVERIFY(!result.available);
    QCOMPARE(modelManagementStatusName(result.status), QStringLiteral("Not Implemented"));
    QVERIFY(result.summary.contains(QStringLiteral("future scoped")));
    QCOMPARE(service.recommendations().at(0).modelName, recommendations.at(0).modelName);
    QCOMPARE(service.requirementSummaries().at(0).summary, requirements.at(0).summary);
}

void ApplicationControllerTest::exposesModelManagementReadinessMetadata() {
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        nullptr);

    QCOMPARE(controller->modelManagementStatus(), QStringLiteral("Metadata Only"));
    QCOMPARE(controller->modelManagementSummary(),
             QStringLiteral("Model management readiness is metadata-only: 1 installed local "
                            "models reported, selected model llama3.2, actions unavailable."));
    QVERIFY(controller->modelManagementActionAvailability().contains(
        QStringLiteral("Pull for llama3.2 is unavailable")));
    QVERIFY(controller->modelManagementActionAvailability().contains(
        QStringLiteral("Delete for llama3.2 is unavailable")));
    QVERIFY(controller->modelManagementActionAvailability().contains(
        QStringLiteral("Install for llama3.2 is unavailable")));
    QCOMPARE(controller->modelRecommendationSummaries().size(), 3);
    QCOMPARE(controller->modelRequirementSummaries().size(), 3);
    QVERIFY(controller->modelRequirementSummaries().first().contains(
        QStringLiteral("approx RAM about 8 GB")));
}

void ApplicationControllerTest::exposesVoiceReadinessMetadata() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};

    QCOMPARE(controller.voiceRuntimeMode(), QStringLiteral("Disabled"));
    QVERIFY(!controller.voiceEnabled());
    QCOMPARE(controller.textToSpeechStatus(), QStringLiteral("Disabled"));
    QCOMPARE(controller.speechToTextStatus(), QStringLiteral("Disabled"));
    QVERIFY(controller.textToSpeechSummary().contains(QStringLiteral("disabled placeholder")));
    QVERIFY(controller.speechToTextSummary().contains(QStringLiteral("disabled placeholder")));
    QCOMPARE(controller.voiceReadinessStatus(), QStringLiteral("Disabled"));
    QVERIFY(controller.voiceReadinessSummary().contains(QStringLiteral("metadata-only")));
    QVERIFY(controller.voiceReadinessSummary().contains(QStringLiteral("no microphone")));
    QCOMPARE(controller.voiceCapabilitySummaries().size(), 2);
    QVERIFY(
        controller.voiceCapabilitySummaries().first().contains(QStringLiteral("Text To Speech")));
    QVERIFY(controller.voiceReadinessChecks()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("No Piper")));
    QVERIFY(controller.voiceReadinessChecks()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("No microphone access")));
    QCOMPARE(controller.voiceSessionId(), QStringLiteral("voice-session-1"));
    QCOMPARE(controller.voiceSessionStatus(), QStringLiteral("completed"));
    QVERIFY(controller.voiceSessionSummary().contains(QStringLiteral("metadata-only")));
    QCOMPARE(controller.voicePipelineStatus(), QStringLiteral("completed"));
    QVERIFY(controller.voicePipelineSummary().contains(QStringLiteral("metadata-only")));
    QCOMPARE(controller.voicePipelineTraceSummaries().size(), 7);
    QVERIFY(controller.voicePipelineTraceSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("transcribing-placeholder")));
    const auto chatCountBeforeVoicePipeline = controller.chatHistory().size();
    const auto voiceSession = controller.currentVoicePipelineSessionResult();
    QCOMPARE(voiceSession.executionAttempted, false);
    QVERIFY(!voiceSession.safetyReport.executionAttempted);
    QVERIFY(!voiceSession.safetyReport.microphoneCaptureAllowed);
    QVERIFY(!voiceSession.safetyReport.audioPlaybackAllowed);
    QVERIFY(!voiceSession.safetyReport.whisperExecutionAllowed);
    QVERIFY(!voiceSession.safetyReport.piperExecutionAllowed);
    QCOMPARE(controller.chatHistory().size(), chatCountBeforeVoicePipeline);
    QCOMPARE(controller.voicePipelineSessionStatus(), QStringLiteral("fallback"));
    QVERIFY(controller.voicePipelineSessionSummary().contains(QStringLiteral("execution attempted: no")));
    QCOMPARE(controller.voicePipelineSessionStageReadinessSummaries().size(), 5);
    QVERIFY(controller.voicePipelineSessionStageReadinessSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("transcription-readiness")));
    QVERIFY(controller.voicePipelineSessionTraceSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("no transcript")));
    QVERIFY(controller.voicePipelineSessionFallbackSummary().contains(
        QStringLiteral("no chat send")));
    QVERIFY(controller.voicePipelineSessionSafetySummary().contains(
        QStringLiteral("no-execution guarantees")));
    QCOMPARE(controller.voicePipelineSessionSafetyChecks().size(), 8);
    QCOMPARE(controller.voicePipelineSessionReadyStageCount(), 2);
    QCOMPARE(controller.voicePipelineSessionBlockedStageCount(), 2);
    QCOMPARE(controller.voiceRuntimeStatus(), QStringLiteral("Unavailable"));
    QVERIFY(controller.voiceRuntimeSummary().contains(QStringLiteral("microphone disabled")));
    QCOMPARE(controller.voiceRuntimeCheckSummaries().size(), 7);
    QVERIFY(!controller.voiceRuntimeAvailable());
    QVERIFY(!controller.voiceTextToSpeechAvailable());
    QVERIFY(!controller.voiceSpeechToTextAvailable());
    QVERIFY(!controller.voiceMicrophoneEnabled());
    QVERIFY(!controller.voicePlaybackEnabled());
    QVERIFY(controller.voiceLocalOnlyPolicy());
    QVERIFY(!controller.voiceProcessExecutionEnabled());
    QCOMPARE(controller.voiceRuntimeEnvironmentStatus(), QStringLiteral("Blocked"));
    QVERIFY(controller.voiceRuntimeEnvironmentSummary().contains(QStringLiteral("metadata-only")));
    QCOMPARE(controller.voiceBinarySummaries().size(), 2);
    QVERIFY(controller.voiceBinarySummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper Binary: Missing")));
    QCOMPARE(controller.voiceModelSummaries().size(), 2);
    QVERIFY(controller.voiceModelSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Whisper Model: Missing")));
    QCOMPARE(controller.voiceRuntimePermissionSummaries().size(), 4);
    QVERIFY(controller.voiceRuntimePermissionSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Process Execution: Denied")));
    QCOMPARE(controller.voiceRuntimeSafetyStatus(), QStringLiteral("Blocked"));
    QVERIFY(controller.voiceRuntimeSafetySummary().contains(QStringLiteral("blocks execution")));
    QCOMPARE(controller.voiceRuntimeSafetyChecks().size(), 7);
    QVERIFY(!controller.voiceRuntimeExecutionAllowed());
    QVERIFY(controller.voiceRuntimeReadinessSummary().contains(
        QStringLiteral("Missing Configuration voice runtime readiness")));
    QCOMPARE(controller.voiceRuntimeConfiguredCount(), 0);
    QCOMPARE(controller.voiceRuntimeMissingCount(), 4);
    QCOMPARE(controller.voiceRuntimeRefusedCount(), 0);
    QCOMPARE(controller.voiceRuntimeHealth(), QStringLiteral("Degraded Metadata"));
    QVERIFY(controller.voiceRuntimePermissionFoundationSummary().contains(
        QStringLiteral("future microphone access")));
    QVERIFY(controller.voiceRuntimeSandboxSummary().contains(QStringLiteral("Required Metadata")));
    QVERIFY(controller.voiceRuntimeSafetyReportSummary().contains(
        QStringLiteral("execution attempted: no")));
    QCOMPARE(controller.voiceRuntimeReadinessChecks().size(), 5);
    QVERIFY(controller.piperRuntimePathSummary().contains(QStringLiteral("2 missing")));
    QVERIFY(controller.whisperRuntimePathSummary().contains(QStringLiteral("2 missing")));
    QCOMPARE(controller.piperTtsStatus(), QStringLiteral("Disabled"));
    QVERIFY(controller.piperTtsSummary().contains(QStringLiteral("disabled by default")));
    QCOMPARE(controller.piperTtsReadinessChecks().size(), 9);
    QVERIFY(controller.piperTtsReadinessChecks()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper binary")));
    QVERIFY(!controller.piperTtsReady());
    QCOMPARE(controller.piperTtsFileOutputStatus(), QStringLiteral("Disabled"));
    QVERIFY(controller.piperTtsFileOutputSummary().contains(
        QStringLiteral("No playback or microphone access")));
    QCOMPARE(controller.whisperTranscriptionStatus(), QStringLiteral("Disabled"));
    QVERIFY(controller.whisperTranscriptionReadinessSummary().contains(
        QStringLiteral("execution attempted: no")));
    QVERIFY(controller.whisperTranscriptionLastSummary().contains(
        QStringLiteral("No Whisper transcription request")));
    QVERIFY(controller.whisperTranscriptionFallbackSummary().contains(
        QStringLiteral("no transcript")));
    QVERIFY(controller.whisperTranscriptionSafetySummary().contains(
        QStringLiteral("execution attempted: no")));
    QVERIFY(controller.whisperTranscriptionTraceSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("No microphone capture")));
}

void ApplicationControllerTest::validatesConfiguredVoicePathsAsMetadataOnly() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto piperBinaryPath = dir.filePath(QStringLiteral("piper"));
    const auto piperModelPath = dir.filePath(QStringLiteral("voice.onnx"));
    const auto whisperBinaryPath = dir.filePath(QStringLiteral("whisper"));
    const auto whisperModelPath = dir.filePath(QStringLiteral("whisper-models"));
    const auto missingPath = dir.filePath(QStringLiteral("missing-piper"));

    QFile piperBinary{piperBinaryPath};
    QVERIFY(piperBinary.open(QIODevice::WriteOnly));
    piperBinary.close();
    QVERIFY(QFile::setPermissions(piperBinaryPath, QFile::ReadOwner | QFile::ExeOwner));

    QFile piperModel{piperModelPath};
    QVERIFY(piperModel.open(QIODevice::WriteOnly));
    piperModel.close();

    QFile whisperBinary{whisperBinaryPath};
    QVERIFY(whisperBinary.open(QIODevice::WriteOnly));
    whisperBinary.close();
    QVERIFY(QFile::setPermissions(whisperBinaryPath, QFile::ReadOwner));

    QVERIFY(QDir{}.mkpath(whisperModelPath));

    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QSignalSpy spy(&controller, &ApplicationController::voiceConfigurationChanged);

    controller.setPiperBinaryPath(piperBinaryPath);
    controller.setPiperModelPath(piperModelPath);
    controller.setWhisperBinaryPath(whisperBinaryPath);
    controller.setWhisperModelPath(whisperModelPath);

    QCOMPARE(spy.count(), 4);
    QCOMPARE(controller.voiceRuntimeEnvironmentStatus(), QStringLiteral("Configured Metadata"));
    QCOMPARE(controller.voiceRuntimeConfiguredCount(), 4);
    QCOMPARE(controller.voiceRuntimeMissingCount(), 0);
    QCOMPARE(controller.voiceRuntimeRefusedCount(), 0);
    QCOMPARE(controller.piperRuntimeStatus(), QStringLiteral("Ready Metadata"));
    QCOMPARE(controller.whisperRuntimeStatus(), QStringLiteral("Ready Metadata"));
    QCOMPARE(controller.whisperTranscriptionStatus(), QStringLiteral("Missing Binary"));
    QVERIFY(controller.whisperTranscriptionReadinessSummary().contains(
        QStringLiteral("2 configured, 1 missing")));
    QVERIFY(controller.whisperTranscriptionSafetySummary().contains(
        QStringLiteral("automatic chat send")));
    QVERIFY(controller.piperRuntimeReadinessSummary().contains(QStringLiteral("disabled by default")));
    QVERIFY(controller.whisperRuntimeReadinessSummary().contains(
        QStringLiteral("readiness-only")));
    QVERIFY(controller.voiceConfigurationReadinessSummary().contains(
        QStringLiteral("Piper file-output TTS: Ready")));
    QVERIFY(controller.voiceConfigurationReadinessSummary().contains(
        QStringLiteral("Whisper STT preparation: Blocked")));
    QVERIFY(controller.voiceBinarySummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper binary metadata only: path exists, readable, "
                                         "executable")));
    QVERIFY(controller.voiceBinarySummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Whisper binary metadata only: path exists, readable, "
                                         "non-executable")));
    QVERIFY(controller.voiceModelSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Whisper model metadata only: path exists, readable")));
    QVERIFY(controller.voiceConfigurationStatusBadges().contains(
        QStringLiteral("Piper file-output TTS: Ready")));
    QVERIFY(controller.voiceConfigurationStatusBadges().contains(
        QStringLiteral("Piper binary: Ready")));
    QVERIFY(controller.voiceConfigurationStatusBadges().contains(
        QStringLiteral("Whisper binary: Blocked (path is not executable)")));
    QVERIFY(controller.voiceConfigurationValidationSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper binary: Ready - exists, readable, file, "
                                         "executable")));
    QVERIFY(controller.voiceConfigurationValidationSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Whisper binary: Blocked - exists, readable, file, "
                                         "non-executable")));
    QVERIFY(controller.voiceConfigurationHintSummaries().contains(
        QStringLiteral("Piper binary hint: configured path is executable; no "
                       "suggestion needed.")));
    QVERIFY(controller.voiceConfigurationHintSummaries().contains(
        QStringLiteral("Whisper model hint: configured path is readable; no "
                       "suggestion needed.")));
    QVERIFY(controller.voiceConfigurationHintSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Hints are read-only")));
    QVERIFY(controller.piperTtsReadinessChecks()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Null Piper TTS client is disabled and never launches "
                                         "Piper")));
    QCOMPARE(controller.piperTtsStatus(), QStringLiteral("Ready Metadata"));
    QCOMPARE(controller.piperSynthesisStatus(), QStringLiteral("Ready Metadata"));
    QVERIFY(controller.piperSynthesisReadinessSummary().contains(
        QStringLiteral("2 configured, 0 missing")));
    QCOMPARE(controller.piperFileOutputReadinessStatus(), QStringLiteral("Ready"));
    QVERIFY(controller.piperFileOutputReadinessSummary().contains(
        QStringLiteral("Ready for a later controlled file-output TTS phase")));
    QCOMPARE(controller.whisperPreparationReadinessStatus(), QStringLiteral("Blocked"));
    QVERIFY(controller.whisperPreparationReadinessSummary().contains(
        QStringLiteral("Whisper binary path is not executable")));
    QVERIFY(controller.piperTtsReady());

    controller.setPiperBinaryPath(missingPath);

    QVERIFY(controller.voiceBinarySummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("Piper Binary: Missing")));
    QVERIFY(controller.voiceBinarySummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("path missing")));
    QVERIFY(controller.voiceConfigurationStatusBadges().contains(
        QStringLiteral("Piper binary: Blocked (path is missing)")));
    QCOMPARE(controller.piperFileOutputReadinessStatus(), QStringLiteral("Blocked"));
    QVERIFY(controller.piperFileOutputReadinessSummary().contains(
        QStringLiteral("Piper binary path is missing")));
    QCOMPARE(controller.piperTtsStatus(), QStringLiteral("Missing Binary"));

    controller.setWhisperBinaryPath(QStringLiteral("https://example.invalid/whisper"));
    QCOMPARE(controller.whisperRuntimeStatus(), QStringLiteral("Refused"));
    QCOMPARE(controller.voiceRuntimeRefusedCount(), 1);
    QVERIFY(controller.whisperRuntimeReadinessSummary().contains(QStringLiteral("unsafe/non-local")));
    QVERIFY(!controller.whisperRuntimeReadinessSummary().contains(
        QStringLiteral("https://example.invalid")));
    QVERIFY(!controller.voiceRuntimeSafetyReportSummary().contains(
        QStringLiteral("https://example.invalid")));
}

void ApplicationControllerTest::piperFileOutputExecutionRequiresExplicitOptIn() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    auto fixture = makePiperController(FakePiperTtsClient::Mode::Success);
    configureReadyPiperPaths(*fixture.controller, dir);

    QCOMPARE(fixture.controller->piperFileOutputReadinessStatus(), QStringLiteral("Ready"));
    QVERIFY(!fixture.controller->piperFileOutputExecutionEnabled());
    QCOMPARE(fixture.controller->piperFileOutputExecutionStatus(), QStringLiteral("Disabled"));
    QVERIFY(fixture.controller->piperFileOutputExecutionSummary().contains(
        QStringLiteral("readiness")));

    const auto generated = fixture.controller->generatePiperTtsFile(QStringLiteral("hello"));

    QVERIFY(!generated);
    QVERIFY(!fixture.client->called);
    QCOMPARE(fixture.controller->piperFileOutputExecutionStatus(), QStringLiteral("Disabled"));
    QVERIFY(fixture.controller->piperFileOutputExecutionSummary().contains(
        QStringLiteral("refused")));
    QCOMPARE(fixture.controller->piperFileOutputAudioPathSummary(),
             QStringLiteral("No generated Piper audio file."));
}

void ApplicationControllerTest::piperFileOutputExecutionUsesFakeClientForControlledSuccess() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    auto fixture = makePiperController(FakePiperTtsClient::Mode::Success);
    configureReadyPiperPaths(*fixture.controller, dir);

    fixture.controller->setPiperFileOutputExecutionEnabled(true);
    const auto generated = fixture.controller->generatePiperTtsFile(QStringLiteral("hello"));

    QVERIFY(!generated);
    QVERIFY(!fixture.client->called);
    QCOMPARE(fixture.controller->piperFileOutputExecutionStatus(), QStringLiteral("Disabled"));
    QVERIFY(fixture.controller->piperFileOutputExecutionSummary().contains(
        QStringLiteral("refused")));
    QCOMPARE(fixture.controller->piperFileOutputAudioPathSummary(),
             QStringLiteral("No generated Piper audio file."));
    QVERIFY(fixture.controller->piperSynthesisSafetySummary().contains(
        QStringLiteral("execution attempted: no")));
}

void ApplicationControllerTest::piperFileOutputExecutionReportsFailureAndTimeout() {
    QTemporaryDir failureDir;
    QVERIFY(failureDir.isValid());
    auto failed = makePiperController(FakePiperTtsClient::Mode::Failure);
    configureReadyPiperPaths(*failed.controller, failureDir);
    failed.controller->setPiperFileOutputExecutionEnabled(true);

    QVERIFY(!failed.controller->generatePiperTtsFile(QStringLiteral("hello")));
    QVERIFY(!failed.client->called);
    QCOMPARE(failed.controller->piperFileOutputExecutionStatus(), QStringLiteral("Disabled"));
    QVERIFY(failed.controller->piperFileOutputExecutionSummary().contains(
        QStringLiteral("refused")));

    QTemporaryDir timeoutDir;
    QVERIFY(timeoutDir.isValid());
    auto timedOut = makePiperController(FakePiperTtsClient::Mode::Timeout);
    configureReadyPiperPaths(*timedOut.controller, timeoutDir);
    timedOut.controller->setPiperFileOutputExecutionEnabled(true);

    QVERIFY(!timedOut.controller->generatePiperTtsFile(QStringLiteral("hello")));
    QVERIFY(!timedOut.client->called);
    QCOMPARE(timedOut.controller->piperFileOutputExecutionStatus(), QStringLiteral("Disabled"));
    QVERIFY(timedOut.controller->piperFileOutputExecutionSummary().contains(
        QStringLiteral("refused")));
}

void ApplicationControllerTest::piperFileOutputExecutionBlocksInvalidBinaryOrModel() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    auto fixture = makePiperController(FakePiperTtsClient::Mode::Success);
    const auto piperBinaryPath = dir.filePath(QStringLiteral("piper"));
    const auto missingModelPath = dir.filePath(QStringLiteral("missing.onnx"));

    QFile piperBinary{piperBinaryPath};
    QVERIFY(piperBinary.open(QIODevice::WriteOnly));
    piperBinary.write("not executable");
    piperBinary.close();

    fixture.controller->setPiperBinaryPath(piperBinaryPath);
    fixture.controller->setPiperModelPath(missingModelPath);
    fixture.controller->setPiperFileOutputExecutionEnabled(true);

    QVERIFY(!fixture.controller->generatePiperTtsFile(QStringLiteral("hello")));
    QVERIFY(!fixture.client->called);
    QCOMPARE(fixture.controller->piperFileOutputExecutionStatus(),
             QStringLiteral("Disabled"));
    QVERIFY(fixture.controller->piperFileOutputExecutionSummary().contains(
        QStringLiteral("refused")));
}

void ApplicationControllerTest::rejectsInvalidSelectedModelAgainstDiscoveryMetadata() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));

    controller->setSelectedLocalModel(QStringLiteral("missing-model"));
    const auto ran = controller->runLocalInference(QStringLiteral("hello"), {});

    QVERIFY(!ran);
    QVERIFY(!fakeClientPtr->called);
    QCOMPARE(controller->selectedLocalModelStatus(), QStringLiteral("Invalid"));
    QCOMPARE(controller->selectedLocalModelSummary(),
             QStringLiteral("Selected local model missing-model was not found in discovered local "
                            "models."));
    QCOMPARE(controller->selectedLocalModelMetadataSummary(),
             QStringLiteral("Invalid selection: missing-model is not in discovered local model "
                            "metadata."));
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Model Unavailable"));
    QCOMPARE(controller->localInferenceSummary(),
             QStringLiteral("Local inference request rejected: selected model is not installed."));
}

void ApplicationControllerTest::exposesStreamingSkeletonDisabledMetadata() {
    const auto controller = makeController();

    QVERIFY(!controller->localInferenceStreamingAvailable());
    QVERIFY(!controller->localInferenceStreamingEnabled());
    QCOMPARE(controller->localInferenceStreamStatus(), QStringLiteral("Disabled"));
    QCOMPARE(controller->localInferenceStreamSummary(),
             QStringLiteral("Local inference streaming is disabled; responses finalize through "
                            "normal chat history."));
    QVERIFY(controller->localInferenceStreamingText().isEmpty());
}

void ApplicationControllerTest::streamDisabledByDefaultFallsBackToNonStreaming() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto streamClient = std::make_unique<FakeLocalInferenceStreamClient>(
        QList<LocalInferenceStreamChunk>{{1, QStringLiteral("stream"), false, false, {}}});
    auto* streamClientPtr = streamClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient), std::move(streamClient));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(fakeClientPtr->called);
    QVERIFY(!streamClientPtr->called);
    QCOMPARE(controller->chatHistory().at(2).content, QStringLiteral("fake local completion"));
}

void ApplicationControllerTest::enabledLocalChatStreamingAppendsOrderedChunksOnce() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto streamClient =
        std::make_unique<FakeLocalInferenceStreamClient>(QList<LocalInferenceStreamChunk>{
            {1, QStringLiteral("hello "), false, false, QStringLiteral("first chunk")},
            {2, QStringLiteral("stream"), true, false, QStringLiteral("final chunk")},
        });
    auto* streamClientPtr = streamClient.get();
    auto historyStore = std::make_unique<RecordingChatHistoryStore>();
    auto* historyStorePtr = historyStore.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr,
        std::move(historyStore), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(),
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient), std::move(streamClient));
    QSignalSpy localSpy(controller.get(), &ApplicationController::localInferenceChanged);
    QSignalSpy chatSpy(controller.get(), &ApplicationController::chatMessagesChanged);

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setLocalInferenceStreamingEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(!fakeClientPtr->called);
    QVERIFY(streamClientPtr->called);
    QCOMPARE(streamClientPtr->lastRequest.options.streamingRequested, true);
    QCOMPARE(controller->localInferenceStreamStatus(), QStringLiteral("Completed"));
    QVERIFY(controller->localInferenceStreamingText().isEmpty());
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(1).content, QStringLiteral("hello"));
    QCOMPARE(controller->chatHistory().at(2).content, QStringLiteral("hello stream"));
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Received);
    QCOMPARE(historyStorePtr->messages_.size(), 3);
    QCOMPARE(chatSpy.count(), 2);
    QVERIFY(localSpy.count() >= 4);
}

void ApplicationControllerTest::streamingPreviewClearsAfterFinalResponseIsPersisted() {
    auto streamClient =
        std::make_unique<FakeLocalInferenceStreamClient>(QList<LocalInferenceStreamChunk>{
            {1, QStringLiteral("partial "), false, false, QStringLiteral("first chunk")},
            {2, QStringLiteral("final"), true, false, QStringLiteral("final chunk")},
        });
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::make_unique<FakeLocalInferenceClient>(), std::move(streamClient));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setLocalInferenceStreamingEnabled(true);

    QVERIFY(controller->sendMessage(QStringLiteral("hello")));
    QVERIFY(controller->localInferenceStreamingText().isEmpty());
    QCOMPARE(controller->localInferenceStreamStatus(), QStringLiteral("Completed"));
    QCOMPARE(controller->localInferenceSummary(), QStringLiteral("Fake local stream completed."));
    QCOMPARE(controller->chatHistory().at(2).content, QStringLiteral("partial final"));
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Received);
}

void ApplicationControllerTest::enabledLocalChatStreamingHandlesMalformedChunk() {
    auto streamClient = std::make_unique<FakeLocalInferenceStreamClient>(
        QList<LocalInferenceStreamChunk>{
            {1, {}, false, true, QStringLiteral("Malformed local stream chunk ignored.")},
            {2, QStringLiteral("usable text"), true, false, QStringLiteral("final chunk")},
        },
        LocalInferenceStreamStatus::Completed,
        QStringLiteral("Fake local stream completed with malformed chunk ignored."));
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::make_unique<FakeLocalInferenceClient>(), std::move(streamClient));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setLocalInferenceStreamingEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(controller->localInferenceStreamingText().isEmpty());
    QCOMPARE(controller->chatHistory().at(2).content, QStringLiteral("usable text"));
    QCOMPARE(controller->localInferenceStreamSummary(),
             QStringLiteral("Fake local stream completed with malformed chunk ignored."));
}

void ApplicationControllerTest::enabledLocalChatStreamingCancellationAppendsSafeRefusal() {
    auto streamClient = std::make_unique<FakeLocalInferenceStreamClient>(
        QList<LocalInferenceStreamChunk>{}, LocalInferenceStreamStatus::Cancelled,
        QStringLiteral("Injected local stream cancellation."));
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::make_unique<FakeLocalInferenceClient>(), std::move(streamClient));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setLocalInferenceStreamingEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QCOMPARE(controller->localInferenceStreamStatus(), QStringLiteral("Cancelled"));
    QCOMPARE(controller->chatSendLifecycleState(), QStringLiteral("failed"));
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(controller->chatHistory().at(2).content,
             QStringLiteral("Local inference failed: Injected local stream cancellation."));
}

void ApplicationControllerTest::streamingPreviewClearsAfterStreamError() {
    auto streamClient = std::make_unique<FakeLocalInferenceStreamClient>(
        QList<LocalInferenceStreamChunk>{
            {1, QStringLiteral("partial "), false, false, QStringLiteral("first chunk")},
            {2, QStringLiteral("preview"), false, false, QStringLiteral("second chunk")},
        },
        LocalInferenceStreamStatus::Error, QStringLiteral("Injected local stream error."));
    auto historyStore = std::make_unique<RecordingChatHistoryStore>();
    auto* historyStorePtr = historyStore.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr,
        std::move(historyStore), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(),
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::make_unique<FakeLocalInferenceClient>(), std::move(streamClient));
    QSignalSpy chatSpy(controller.get(), &ApplicationController::chatMessagesChanged);

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setLocalInferenceStreamingEnabled(true);

    QVERIFY(controller->sendMessage(QStringLiteral("hello")));
    QVERIFY(controller->localInferenceStreamingText().isEmpty());
    QCOMPARE(controller->localInferenceStreamStatus(), QStringLiteral("Error"));
    QCOMPARE(controller->chatSendLifecycleState(), QStringLiteral("failed"));
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(controller->chatHistory().at(2).content,
             QStringLiteral("Local inference failed: Injected local stream error."));
    QCOMPARE(historyStorePtr->messages_.size(), 3);
    QCOMPARE(chatSpy.count(), 2);
}

void ApplicationControllerTest::asyncLocalChatInferenceCompletesWithFakeWorker() {
    auto worker = std::make_unique<AsyncLocalInferenceWorker>();
    auto* workerPtr = worker.get();
    auto controller = makeAsyncWorkerController(std::move(worker));
    QSignalSpy chatSpy(controller.get(), &ApplicationController::chatMessagesChanged);

    controller->setLocalChatInferenceEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(workerPtr->called);
    QVERIFY(controller->localInferenceBusy());
    QCOMPARE(controller->chatHistory().size(), 2);

    QTRY_VERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Succeeded"));
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(1).content, QStringLiteral("hello"));
    QCOMPARE(controller->chatHistory().at(2).content, QStringLiteral("async fake completion"));
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Received);
    QCOMPARE(chatSpy.count(), 2);
}

void ApplicationControllerTest::asyncLocalChatStreamingCompletesWithFakeWorker() {
    auto worker = std::make_unique<AsyncLocalInferenceWorker>();
    auto* workerPtr = worker.get();
    worker->delayMs = 10;
    worker->streamFinishDelayMs = 250;
    auto controller = makeAsyncWorkerController(std::move(worker));
    QSignalSpy chatSpy(controller.get(), &ApplicationController::chatMessagesChanged);

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    controller->setLocalInferenceStreamingEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(workerPtr->called);
    QVERIFY(controller->localInferenceBusy());
    QCOMPARE(controller->chatHistory().size(), 2);
    QCOMPARE(workerPtr->lastRequest.options.streamingRequested, true);

    QTRY_COMPARE(controller->localInferenceStreamingText(), QStringLiteral("async "));
    QCOMPARE(controller->localInferenceStreamStatus(), QStringLiteral("Streaming"));

    QTRY_VERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Succeeded"));
    QCOMPARE(controller->localInferenceStreamStatus(), QStringLiteral("Completed"));
    QVERIFY(controller->localInferenceStreamingText().isEmpty());
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(1).content, QStringLiteral("hello"));
    QCOMPARE(controller->chatHistory().at(2).content, QStringLiteral("async stream completion"));
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Received);
    QCOMPARE(chatSpy.count(), 2);
}

void ApplicationControllerTest::asyncLocalChatInferenceErrorResetsBusy() {
    auto worker =
        std::make_unique<AsyncLocalInferenceWorker>(AsyncLocalInferenceWorker::Mode::TimeoutError);
    auto controller = makeAsyncWorkerController(std::move(worker));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(controller->localInferenceBusy());
    QTRY_VERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->localInferenceRuntimeState(), QStringLiteral("Failed"));
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(controller->chatHistory().at(2).content,
             QStringLiteral("Local inference failed: the local Ollama request timed out."));
}

void ApplicationControllerTest::asyncSuccessUpdatesConversationRuntimeState() {
    auto worker = std::make_unique<AsyncLocalInferenceWorker>();
    auto controller = makeAsyncWorkerController(std::move(worker));
    QSignalSpy runtimeSpy(controller.get(), &ApplicationController::conversationRuntimeChanged);

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    QVERIFY(controller->sendMessage(QStringLiteral("hello")));

    QCOMPARE(controller->conversationRuntimeRequestId(),
             QStringLiteral("local-inference-request-1"));
    QCOMPARE(controller->conversationRuntimeActiveModel(), QStringLiteral("llama3.2"));
    QCOMPARE(controller->conversationRuntimeActiveRoute(), QStringLiteral("Local Ollama"));
    QVERIFY(!controller->conversationRuntimeStreaming());

    QTRY_VERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->conversationState(), QStringLiteral("Completed"));
    QVERIFY(
        controller->conversationRuntimeLastSuccessSummary().contains(QStringLiteral("completed")));
    QCOMPARE(controller->conversationRuntimeLastErrorSummary(),
             QStringLiteral("No error or refusal yet."));
    QVERIFY(runtimeSpy.count() >= 2);
}

void ApplicationControllerTest::
    asyncFailureUpdatesConversationRuntimeErrorWithoutCorruptingHistory() {
    auto worker =
        std::make_unique<AsyncLocalInferenceWorker>(AsyncLocalInferenceWorker::Mode::TimeoutError);
    auto controller = makeAsyncWorkerController(std::move(worker));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    QVERIFY(controller->sendMessage(QStringLiteral("hello")));

    QTRY_VERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->conversationState(), QStringLiteral("Error"));
    QVERIFY(
        controller->conversationRuntimeLastErrorSummary().contains(QStringLiteral("timed out")));
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(0).role, sentinel::core::ChatRole::System);
    QCOMPARE(controller->chatHistory().at(1).role, sentinel::core::ChatRole::User);
    QCOMPARE(controller->chatHistory().at(1).content, QStringLiteral("hello"));
    QCOMPARE(controller->chatHistory().at(2).role, sentinel::core::ChatRole::Assistant);
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Error);
}

void ApplicationControllerTest::clearChatClearsConversationRuntimeAndPersistence() {
    auto store = std::make_unique<RecordingChatHistoryStore>();
    const auto storePtr = store.get();
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    QVERIFY(controller.sendMessage(QStringLiteral("status")));
    QCOMPARE(storePtr->messages_.size(), 3);
    QVERIFY(controller.clearChat());

    QCOMPARE(controller.conversationState(), QStringLiteral("Idle"));
    QCOMPARE(controller.conversationRuntimeRequestId(), QStringLiteral("None"));
    QCOMPARE(controller.conversationRuntimeActiveModel(), QStringLiteral("None"));
    QCOMPARE(controller.conversationRuntimeLastSuccessSummary(),
             QStringLiteral("No successful response yet."));
    QCOMPARE(controller.conversationRuntimeLastErrorSummary(),
             QStringLiteral("No error or refusal yet."));
    QVERIFY(storePtr->wasCleared_);
    QCOMPARE(storePtr->messages_.size(), 1);
    QCOMPARE(storePtr->messages_.first().role, sentinel::core::ChatRole::System);
}

void ApplicationControllerTest::reportsPersistedConversationHistorySummary() {
    auto store = std::make_unique<RecordingChatHistoryStore>();
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    QVERIFY(controller.sendMessage(QStringLiteral("status")));

    QCOMPARE(controller.conversationPersistenceStatus(), QStringLiteral("Persisted"));
    QCOMPARE(controller.conversationHistoryMessageCount(), 3);
    QVERIFY(controller.conversationHistorySummaryText().contains(QStringLiteral("3 messages")));
    QVERIFY(controller.conversationHistorySummaryText().contains(QStringLiteral("1 user")));
    QVERIFY(controller.conversationHistorySummaryText().contains(QStringLiteral("1 assistant")));
    QCOMPARE(controller.conversationLastSavedStatus(),
             QStringLiteral("Saved latest assistant message."));
}

void ApplicationControllerTest::reportsRuntimeOnlyConversationHistorySummary() {
    auto store =
        std::make_unique<RecordingChatHistoryStore>(QList<sentinel::core::ChatMessage>{}, false);
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    QVERIFY(controller.sendMessage(QStringLiteral("status")));

    QCOMPARE(controller.conversationPersistenceStatus(), QStringLiteral("Runtime Only"));
    QCOMPARE(controller.conversationHistoryMessageCount(), 3);
    QVERIFY(controller.conversationHistorySummaryText().contains(
        QStringLiteral("Runtime Only transcript")));
    QCOMPARE(controller.conversationLastSavedStatus(),
             QStringLiteral("Runtime-only transcript; persistence unavailable."));
    QVERIFY(controller.conversationLastRestoredStatus().contains(QStringLiteral("unavailable")));
}

void ApplicationControllerTest::
    exposesConversationStoreReadinessWithoutSwitchingTranscriptStorage() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};

    QCOMPARE(controller.conversationStoreStatus(), QStringLiteral("Ready"));
    QCOMPARE(controller.conversationStoreConversationCount(), 1);
    QVERIFY(controller.activeConversationSummary().contains(QStringLiteral("Current Transcript")));
    QVERIFY(controller.activeConversationSummary().contains(QStringLiteral("1 message")));
    QCOMPARE(controller.conversationStoreSummaries().size(), 1);
    QVERIFY(!controller.activeConversationId().isEmpty());
    QCOMPARE(controller.conversationIds().size(), 1);
    QCOMPARE(controller.conversationTitles().first(), QStringLiteral("Current Transcript"));
    QCOMPARE(controller.conversationArchivedSummaries().first(), QStringLiteral("Active"));
    QCOMPARE(controller.conversationListCurrentTitle(), QStringLiteral("Current Transcript"));

    QVERIFY(controller.sendMessage(QStringLiteral("store exposure")));

    QCOMPARE(controller.conversationStoreConversationCount(), 1);
    QVERIFY(controller.activeConversationSummary().contains(QStringLiteral("3 messages")));
    QCOMPARE(controller.conversationListCurrentTitle(), QStringLiteral("Current Transcript"));
    QCOMPARE(controller.conversationMessageCountSummaries().first(), QStringLiteral("3 messages"));
}

void ApplicationControllerTest::createsSwitchesRenamesArchivesAndUnarchivesConversations() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    const auto firstId = controller.activeConversationId();
    QVERIFY(controller.sendMessage(QStringLiteral("first transcript token")));

    const auto secondId = controller.createConversation(QStringLiteral("Second Thread"));
    QVERIFY(!secondId.isEmpty());
    QVERIFY(secondId != firstId);
    QCOMPARE(controller.activeConversationId(), secondId);
    QCOMPARE(controller.chatHistory().size(), 1);
    QVERIFY(controller.sendMessage(QStringLiteral("second transcript token")));
    QCOMPARE(controller.chatHistory().size(), 3);

    QVERIFY(controller.switchConversation(firstId));
    QCOMPARE(controller.activeConversationId(), firstId);
    QCOMPARE(controller.chatHistory().size(), 3);
    QVERIFY(controller.chatHistory().at(1).content.contains(QStringLiteral("first transcript")));

    QVERIFY(controller.renameConversation(firstId, QStringLiteral("Renamed First")));
    QVERIFY(controller.conversationTitles().contains(QStringLiteral("Renamed First")));
    QVERIFY(controller.archiveConversation(firstId));
    QVERIFY(!controller.activeConversationArchived());
    QVERIFY(controller.conversationArchivedSummaries().contains(QStringLiteral("Archived")));
    QVERIFY(controller.unarchiveConversation(firstId));
    QVERIFY(controller.conversationArchivedSummaries().contains(QStringLiteral("Active")));
}

void ApplicationControllerTest::archivedActiveConversationBlocksSend() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    const auto firstId = controller.activeConversationId();
    QVERIFY(controller.sendMessage(QStringLiteral("archived first token")));
    const auto secondId = controller.createConversation(QStringLiteral("Second"));
    QVERIFY(!secondId.isEmpty());
    QVERIFY(controller.archiveConversation(firstId));
    QVERIFY(controller.switchConversation(firstId));
    QVERIFY(controller.activeConversationArchived());
    QVERIFY(controller.activeConversationStateSummary().contains(
        QStringLiteral("sending is disabled")));
    const auto beforeCount = controller.chatHistory().size();

    QVERIFY(!controller.sendMessage(QStringLiteral("blocked archived send")));

    QCOMPARE(controller.activeConversationId(), firstId);
    QCOMPARE(controller.chatHistory().size(), beforeCount);
    QVERIFY(controller.conversationMessageCountSummaries().contains(QStringLiteral("3 messages")));
}

void ApplicationControllerTest::archiveUnarchiveUpdatesBrowserSummaries() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    const auto firstId = controller.activeConversationId();
    const auto secondId = controller.createConversation(QStringLiteral("Second"));
    QVERIFY(!secondId.isEmpty());

    QCOMPARE(controller.activeConversationCount(), 2);
    QCOMPARE(controller.archivedConversationCount(), 0);
    QVERIFY(controller.conversationActiveSummaries().contains(QStringLiteral("Current")));
    QVERIFY(controller.conversationArchivedSummaries().contains(QStringLiteral("Active")));

    QVERIFY(controller.archiveConversation(firstId));

    QCOMPARE(controller.activeConversationCount(), 1);
    QCOMPARE(controller.archivedConversationCount(), 1);
    QVERIFY(controller.conversationArchivedSummaries().contains(QStringLiteral("Archived")));

    QVERIFY(controller.unarchiveConversation(firstId));

    QCOMPARE(controller.activeConversationCount(), 2);
    QCOMPARE(controller.archivedConversationCount(), 0);
    QVERIFY(controller.conversationArchivedSummaries().contains(QStringLiteral("Active")));
}

void ApplicationControllerTest::pinPersistsAcrossControllerRecreation() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto databasePath = dir.filePath(QStringLiteral("conversations.sqlite3"));
    QString pinnedId;

    {
        auto controller = makeControllerWithConversationStore(
            std::make_unique<SQLiteConversationStore>(databasePath));
        pinnedId = controller->activeConversationId();

        QVERIFY(controller->pinConversation(pinnedId));
        QCOMPARE(controller->conversationPinnedSummaries().first(), QStringLiteral("Pinned"));
    }

    auto reloaded = makeControllerWithConversationStore(
        std::make_unique<SQLiteConversationStore>(databasePath));
    QCOMPARE(reloaded->activeConversationId(), pinnedId);
    QCOMPARE(reloaded->conversationIds().first(), pinnedId);
    QCOMPARE(reloaded->conversationPinnedSummaries().first(), QStringLiteral("Pinned"));
}

void ApplicationControllerTest::unpinPersistsAcrossControllerRecreation() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto databasePath = dir.filePath(QStringLiteral("conversations.sqlite3"));
    QString conversationId;

    {
        auto controller = makeControllerWithConversationStore(
            std::make_unique<SQLiteConversationStore>(databasePath));
        conversationId = controller->activeConversationId();

        QVERIFY(controller->pinConversation(conversationId));
        QVERIFY(controller->unpinConversation(conversationId));
        QCOMPARE(controller->conversationPinnedSummaries().first(), QStringLiteral("Unpinned"));
    }

    auto reloaded = makeControllerWithConversationStore(
        std::make_unique<SQLiteConversationStore>(databasePath));
    QCOMPARE(reloaded->conversationIds().first(), conversationId);
    QCOMPARE(reloaded->conversationPinnedSummaries().first(), QStringLiteral("Unpinned"));
}

void ApplicationControllerTest::duplicateConversationCopiesLocalMessages() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    const auto sourceId = controller.activeConversationId();
    QVERIFY(controller.sendMessage(QStringLiteral("duplicate source token")));

    const auto duplicateId = controller.duplicateConversation(sourceId);

    QVERIFY(!duplicateId.isEmpty());
    QVERIFY(duplicateId != sourceId);
    QCOMPARE(controller.conversationDuplicateLastStatus(), QStringLiteral("Succeeded"));
    QVERIFY(controller.conversationDuplicateLastResultSummary().contains(
        QStringLiteral("Current Transcript Copy")));
    QVERIFY(controller.conversationTitles().contains(QStringLiteral("Current Transcript Copy")));
    QCOMPARE(controller.activeConversationId(), sourceId);

    QVERIFY(controller.switchConversation(duplicateId));
    QCOMPARE(controller.chatHistory().size(), 3);
    QCOMPARE(controller.chatHistory().at(1).content, QStringLiteral("duplicate source token"));
}

void ApplicationControllerTest::conversationOrderingIsPinnedRecentArchivedDeterministic() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    const auto firstId = controller.activeConversationId();
    QVERIFY(controller.renameConversation(firstId, QStringLiteral("Recent A")));
    const auto pinnedId = controller.createConversation(QStringLiteral("Pinned B"));
    const auto archivedId = controller.createConversation(QStringLiteral("Archived C"));
    QVERIFY(controller.pinConversation(pinnedId));
    QVERIFY(controller.archiveConversation(archivedId));

    const auto ids = controller.conversationIds();
    const auto titles = controller.conversationTitles();
    const auto pinned = controller.conversationPinnedSummaries();
    const auto archived = controller.conversationArchivedSummaries();

    QCOMPARE(ids.size(), 3);
    QCOMPARE(ids.at(0), pinnedId);
    QCOMPARE(pinned.at(0), QStringLiteral("Pinned"));
    QCOMPARE(titles.at(1), QStringLiteral("Recent A"));
    QCOMPARE(archived.at(1), QStringLiteral("Active"));
    QCOMPARE(ids.at(2), archivedId);
    QCOMPARE(archived.at(2), QStringLiteral("Archived"));
}

void ApplicationControllerTest::deleteReadinessIsDisabledByDefault() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};

    QVERIFY(!controller.conversationDeleteAvailable());
    QCOMPARE(controller.conversationDeletePolicyStatus(), QStringLiteral("Disabled By Default"));
    QCOMPARE(controller.conversationDeleteReadinessStatus(), QStringLiteral("Disabled"));
    QVERIFY(controller.conversationDeleteReadinessSummary().contains(
        QStringLiteral("Permanent delete is not enabled yet")));
    QVERIFY(controller.conversationDeleteReadinessChecks().contains(
        QStringLiteral("Permanent delete: Not enabled yet")));
}

void ApplicationControllerTest::deleteRequestRefusesSafelyWithoutMutation() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    const auto firstId = controller.activeConversationId();
    QVERIFY(controller.sendMessage(QStringLiteral("delete safety token")));
    const auto beforeCount = controller.conversationStoreConversationCount();
    const auto beforeMessages = controller.chatHistory().size();

    QVERIFY(!controller.requestPermanentDeleteConversation(firstId));

    QCOMPARE(controller.conversationDeleteLastStatus(), QStringLiteral("Refused"));
    QVERIFY(controller.conversationDeleteLastResultSummary().contains(
        QStringLiteral("Permanent delete is not enabled yet")));
    QCOMPARE(controller.conversationStoreConversationCount(), beforeCount);
    QCOMPARE(controller.activeConversationId(), firstId);
    QCOMPARE(controller.chatHistory().size(), beforeMessages);
    QVERIFY(controller.switchConversation(firstId));
}

void ApplicationControllerTest::deleteRequestRefusesSafelyWithoutSQLiteMutation() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto databasePath = dir.filePath(QStringLiteral("conversations.sqlite3"));
    QString firstId;

    {
        auto controller = makeControllerWithConversationStore(
            std::make_unique<SQLiteConversationStore>(databasePath));
        firstId = controller->activeConversationId();
        QVERIFY(controller->sendMessage(QStringLiteral("sqlite delete safety token")));
        const auto beforeCount = controller->conversationStoreConversationCount();
        const auto beforeMessages = controller->chatHistory().size();

        QVERIFY(!controller->requestPermanentDeleteConversation(firstId));

        QCOMPARE(controller->conversationDeleteLastStatus(), QStringLiteral("Refused"));
        QVERIFY(controller->conversationDeleteLastResultSummary().contains(
            QStringLiteral("Permanent delete is not enabled yet")));
        QCOMPARE(controller->conversationStoreConversationCount(), beforeCount);
        QCOMPARE(controller->activeConversationId(), firstId);
        QCOMPARE(controller->chatHistory().size(), beforeMessages);
    }

    SQLiteConversationStore reloaded(databasePath);
    const auto conversations = reloaded.listConversations();
    QCOMPARE(conversations.size(), 1);
    QCOMPARE(conversations.first().id, firstId);
    QVERIFY(!conversations.first().archived);
    QVERIFY(!conversations.first().deleted);

    const auto messages = reloaded.loadMessages(firstId);
    QCOMPARE(messages.size(), 3);
    QCOMPARE(messages.at(1).content, QStringLiteral("sqlite delete safety token"));
}

void ApplicationControllerTest::activeConversationRemainsValidAfterArchiveUnarchive() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    const auto firstId = controller.activeConversationId();

    QVERIFY(controller.archiveConversation(firstId));

    QVERIFY(!controller.activeConversationId().isEmpty());
    QVERIFY(controller.activeConversationId() != QStringLiteral("single-transcript"));
    QVERIFY(!controller.activeConversationArchived());
    QVERIFY(controller.conversationIds().contains(controller.activeConversationId()));

    QVERIFY(controller.unarchiveConversation(firstId));
    QVERIFY(controller.switchConversation(firstId));

    QCOMPARE(controller.activeConversationId(), firstId);
    QVERIFY(!controller.activeConversationArchived());
}

void ApplicationControllerTest::persistsActiveConversationAcrossControllerRecreation() {
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto databasePath = dir.filePath(QStringLiteral("conversations.sqlite3"));
    QString firstId;
    QString secondId;

    {
        auto controller = makeControllerWithConversationStore(
            std::make_unique<SQLiteConversationStore>(databasePath));
        firstId = controller->activeConversationId();
        QVERIFY(controller->sendMessage(QStringLiteral("persisted first token")));
        secondId = controller->createConversation(QStringLiteral("Persisted Second"));
        QVERIFY(controller->sendMessage(QStringLiteral("persisted second token")));
        QVERIFY(!firstId.isEmpty());
        QVERIFY(!secondId.isEmpty());
    }

    auto reloaded = makeControllerWithConversationStore(
        std::make_unique<SQLiteConversationStore>(databasePath));
    QCOMPARE(reloaded->conversationStoreConversationCount(), 2);
    QCOMPARE(reloaded->activeConversationId(), firstId);
    QCOMPARE(reloaded->chatHistory().size(), 3);
    QVERIFY(reloaded->chatHistory().at(1).content.contains(QStringLiteral("persisted first")));
    QVERIFY(reloaded->switchConversation(secondId));
    QCOMPARE(reloaded->chatHistory().size(), 3);
    QVERIFY(reloaded->chatHistory().at(1).content.contains(QStringLiteral("persisted second")));
}

void ApplicationControllerTest::switchingConversationIgnoresStaleAsyncResultAndResetsBusy() {
    auto worker = std::make_unique<AsyncLocalInferenceWorker>();
    worker->delayMs = 20;
    auto* workerPtr = worker.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, std::move(worker));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    const auto firstId = controller->activeConversationId();
    const auto secondId = controller->createConversation(QStringLiteral("After Switch"));
    QVERIFY(controller->switchConversation(firstId));

    QVERIFY(controller->sendMessage(QStringLiteral("async stale token")));
    QVERIFY(controller->localInferenceBusy());
    QVERIFY(workerPtr->called);
    QVERIFY(controller->switchConversation(secondId));
    QVERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->conversationRuntimeRequestId(), QStringLiteral("None"));
    QCOMPARE(controller->chatHistory().size(), 1);

    QTRY_VERIFY(!workerPtr->busy);
    QCOMPARE(controller->chatHistory().size(), 1);
    QVERIFY(controller->switchConversation(firstId));
    QCOMPARE(controller->chatHistory().size(), 2);
    QVERIFY(controller->chatHistory().at(1).content.contains(QStringLiteral("async stale token")));
}

void ApplicationControllerTest::switchingConversationDoesNotDuplicateTranscriptInsertion() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    const auto firstId = controller.activeConversationId();
    QVERIFY(controller.sendMessage(QStringLiteral("duplicate guard")));
    const auto secondId = controller.createConversation(QStringLiteral("Second"));
    QVERIFY(controller.sendMessage(QStringLiteral("other guard")));

    QVERIFY(controller.switchConversation(firstId));
    QCOMPARE(controller.chatHistory().size(), 3);
    QVERIFY(controller.switchConversation(secondId));
    QCOMPARE(controller.chatHistory().size(), 3);
    QVERIFY(controller.switchConversation(firstId));
    QCOMPARE(controller.chatHistory().size(), 3);
    QVERIFY(controller.conversationStoreSummaries().first().contains(QStringLiteral("3 messages")));
}

void ApplicationControllerTest::reportsSingleConversationBrowserEntryDeterministically() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};

    QCOMPARE(controller.conversationListEntryCount(), 1);
    QCOMPARE(controller.conversationListCurrentTitle(), QStringLiteral("Current Transcript"));
    QCOMPARE(controller.conversationListCurrentMessageCount(), 1);
    QCOMPARE(controller.conversationListCurrentPersistenceStatus(), QStringLiteral("Runtime Only"));
}

void ApplicationControllerTest::reportsEmptyTranscriptConversationBrowserSummary() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};

    QCOMPARE(controller.conversationBrowserStatus(), QStringLiteral("Empty Transcript"));
    QVERIFY(controller.conversationBrowserSummaryText().contains(
        QStringLiteral("current transcript entry")));
}

void ApplicationControllerTest::reportsConversationBrowserMessageCountSummary() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QVERIFY(controller.sendMessage(QStringLiteral("count token")));

    QCOMPARE(controller.conversationListCurrentMessageCount(), 3);
    QVERIFY(controller.conversationListCurrentSummary().contains(QStringLiteral("3 messages")));
}

void ApplicationControllerTest::clearChatUpdatesConversationBrowserEntry() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QVERIFY(controller.sendMessage(QStringLiteral("clear token")));
    QCOMPARE(controller.conversationListCurrentMessageCount(), 3);

    QVERIFY(!controller.clearChat());

    QCOMPARE(controller.conversationBrowserStatus(), QStringLiteral("Empty Transcript"));
    QCOMPARE(controller.conversationListCurrentMessageCount(), 1);
    QVERIFY(controller.conversationListCurrentLastUpdatedSummary().contains(
        QStringLiteral("Runtime-only transcript")));
}

void ApplicationControllerTest::conversationBrowserReflectsSearchAndExportAvailability() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    controller.setConversationExportDirectory(dir.path());
    QVERIFY(controller.sendMessage(QStringLiteral("availability token")));

    QVERIFY(controller.searchConversation(QStringLiteral("token")));
    QVERIFY(controller.conversationListCurrentSearchAvailabilitySummary().contains(
        QStringLiteral("Completed")));

    QVERIFY(controller.exportTranscript(QStringLiteral("json")));
    QVERIFY(controller.conversationListCurrentExportAvailabilitySummary().contains(
        QStringLiteral("Last export: Succeeded")));
}

void ApplicationControllerTest::reportsMultiConversationPlanningReadinessWithoutStorageMutation() {
    auto store = std::make_unique<RecordingChatHistoryStore>();
    const auto* storePtr = store.get();
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));
    const auto beforeCount = storePtr->messages_.size();

    QCOMPARE(controller.conversationCurrentStorageMode(), QStringLiteral("Single Transcript"));
    QCOMPARE(controller.conversationFutureStorageMode(), QStringLiteral("Multi Conversation"));
    QCOMPARE(controller.conversationMigrationReadiness(), QStringLiteral("Not Started"));
    QCOMPARE(controller.conversationMigrationStatusSummary(),
             QStringLiteral("Not Started / Planned"));
    QVERIFY(controller.conversationSchemaStatusSummary().contains(
        QStringLiteral("No conversation schema migration applied")));
    QCOMPARE(controller.conversationListEntryCount(), 1);
    QCOMPARE(controller.conversationListCurrentTitle(), QStringLiteral("Current Transcript"));

    QCOMPARE(storePtr->messages_.size(), beforeCount);
    QVERIFY(!storePtr->wasCleared_);
}

void ApplicationControllerTest::inMemoryConversationSearchFindsUserAndAssistantMessages() {
    ApplicationController controller{std::make_unique<ErrorProvider>(),
                                     std::make_unique<InMemoryStore>()};

    QVERIFY(controller.sendMessage(QStringLiteral("alpha deterministic user search")));

    QVERIFY(controller.searchConversation(QStringLiteral("deterministic")));

    QCOMPARE(controller.conversationSearchStatus(), QStringLiteral("Completed"));
    QCOMPARE(controller.conversationSearchQueryText(), QStringLiteral("deterministic"));
    QCOMPARE(controller.conversationSearchResultCount(), 2);
    QVERIFY(controller.conversationSearchSummaryText().contains(QStringLiteral("2")));
    QVERIFY(controller.conversationSearchResultSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("user #2")));
    QVERIFY(controller.conversationSearchResultSummaries()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("assistant #3")));
}

void ApplicationControllerTest::emptyConversationSearchDoesNotMutateHistory() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QVERIFY(controller.sendMessage(QStringLiteral("status")));
    const auto before = controller.chatHistory();

    QVERIFY(!controller.searchConversation(QStringLiteral("   ")));

    QCOMPARE(controller.conversationSearchStatus(), QStringLiteral("Empty Query"));
    QCOMPARE(controller.conversationSearchResultCount(), 0);
    QCOMPARE(controller.chatHistory().size(), before.size());
    for (int i = 0; i < before.size(); ++i) {
        QCOMPARE(controller.chatHistory().at(i).id, before.at(i).id);
        QCOMPARE(controller.chatHistory().at(i).role, before.at(i).role);
        QCOMPARE(controller.chatHistory().at(i).content, before.at(i).content);
    }
}

void ApplicationControllerTest::conversationSearchDoesNotMutateHistory() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QVERIFY(controller.sendMessage(QStringLiteral("status")));
    const auto before = controller.chatHistory();

    QVERIFY(controller.searchConversation(QStringLiteral("status")));

    QCOMPARE(controller.chatHistory().size(), before.size());
    for (int i = 0; i < before.size(); ++i) {
        QCOMPARE(controller.chatHistory().at(i).id, before.at(i).id);
        QCOMPARE(controller.chatHistory().at(i).role, before.at(i).role);
        QCOMPARE(controller.chatHistory().at(i).content, before.at(i).content);
        QCOMPARE(controller.chatHistory().at(i).status, before.at(i).status);
    }
}

void ApplicationControllerTest::clearChatResetsConversationSearchSummary() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QVERIFY(controller.sendMessage(QStringLiteral("status")));
    QVERIFY(controller.searchConversation(QStringLiteral("status")));
    QCOMPARE(controller.conversationSearchResultCount(), 1);

    QVERIFY(!controller.clearChat());

    QCOMPARE(controller.conversationSearchStatus(), QStringLiteral("Empty Query"));
    QCOMPARE(controller.conversationSearchQueryText(), QString());
    QCOMPARE(controller.conversationSearchResultCount(), 0);
    QCOMPARE(controller.chatHistory().size(), 1);
}

void ApplicationControllerTest::markdownConversationExportWritesCurrentTranscript() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    controller.setConversationExportDirectory(dir.path());

    QVERIFY(controller.sendMessage(QStringLiteral("export markdown token")));
    QVERIFY(controller.exportTranscript(QStringLiteral("Markdown")));

    const auto result = controller.latestConversationExportResult();
    QVERIFY(result.success);
    QVERIFY(result.wroteFile);
    QCOMPARE(result.messageCount, controller.chatHistory().size());
    QVERIFY(result.outputFileName.endsWith(QStringLiteral(".md")));
    QVERIFY(QFileInfo(result.outputPath).absolutePath() ==
            QFileInfo(dir.path()).absoluteFilePath());
    QFile file(result.outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    const auto content = QString::fromUtf8(file.readAll());
    QVERIFY(content.contains(QStringLiteral("# Sentinel Transcript")));
    QVERIFY(content.contains(QStringLiteral("## user #2")));
    QVERIFY(content.contains(QStringLiteral("export markdown token")));
    QCOMPARE(controller.conversationExportLastStatus(), QStringLiteral("Succeeded"));
    QCOMPARE(controller.conversationExportLastFileName(), result.outputFileName);
}

void ApplicationControllerTest::jsonConversationExportWritesValidStructure() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    controller.setConversationExportDirectory(dir.path());

    QVERIFY(controller.sendMessage(QStringLiteral("export json token")));
    QVERIFY(controller.exportTranscript(QStringLiteral("json")));

    const auto result = controller.latestConversationExportResult();
    QVERIFY(result.success);
    QVERIFY(result.outputFileName.endsWith(QStringLiteral(".json")));
    QFile file(result.outputPath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto document = QJsonDocument::fromJson(file.readAll());
    QVERIFY(document.isObject());
    const auto root = document.object();
    QCOMPARE(root.value(QStringLiteral("format")).toString(),
             QStringLiteral("sentinel.transcript.v1"));
    QCOMPARE(root.value(QStringLiteral("messageCount")).toInt(), controller.chatHistory().size());
    const auto messages = root.value(QStringLiteral("messages")).toArray();
    QCOMPARE(messages.size(), controller.chatHistory().size());
    QCOMPARE(messages.at(1).toObject().value(QStringLiteral("role")).toString(),
             QStringLiteral("user"));
    QCOMPARE(messages.at(1).toObject().value(QStringLiteral("content")).toString(),
             QStringLiteral("export json token"));
}

void ApplicationControllerTest::emptyConversationExportIsRefused() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    controller.setConversationExportDirectory(dir.path());

    QVERIFY(!controller.exportTranscript(QStringLiteral("markdown")));

    const auto result = controller.latestConversationExportResult();
    QCOMPARE(result.status, QStringLiteral("Refused"));
    QVERIFY(!result.wroteFile);
    QVERIFY(result.refusalSummary.contains(QStringLiteral("no user or assistant messages")));
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 0);
}

void ApplicationControllerTest::conversationExportUsesSanitizedTimestampedFilenames() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    controller.setConversationExportDirectory(dir.path());

    QVERIFY(controller.sendMessage(QStringLiteral("filename token")));
    QVERIFY(controller.exportTranscript(QStringLiteral("markdown")));
    QVERIFY(controller.exportTranscript(QStringLiteral("markdown")));

    const auto result = controller.latestConversationExportResult();
    QVERIFY(result.outputFileName.startsWith(QStringLiteral("sentinel-transcript-")));
    QVERIFY(result.outputFileName.endsWith(QStringLiteral(".md")));
    QVERIFY(!result.outputFileName.contains(QLatin1Char('/')));
    QVERIFY(!result.outputFileName.contains(QLatin1Char(':')));
    QVERIFY(result.outputFileName.contains(QRegularExpression(
        QStringLiteral("^sentinel-transcript-[0-9]{8}-[0-9]{6}-[0-9]{3}(-[0-9]+)?\\.md$"))));
    QCOMPARE(QDir(dir.path()).entryList(QStringList{QStringLiteral("*.md")}, QDir::Files).size(),
             2);
}

void ApplicationControllerTest::unsupportedConversationExportFormatWritesNoFile() {
    ApplicationController controller{std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>()};
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    controller.setConversationExportDirectory(dir.path());

    QVERIFY(controller.sendMessage(QStringLiteral("plain text should not export")));
    QVERIFY(!controller.exportTranscript(QStringLiteral("plain text")));

    QCOMPARE(controller.latestConversationExportResult().status, QStringLiteral("Refused"));
    QCOMPARE(QDir(dir.path()).entryList(QDir::Files).size(), 0);
}

void ApplicationControllerTest::clearChatResetsStreamingAndActiveRequestMetadata() {
    auto worker = std::make_unique<AsyncLocalInferenceWorker>();
    worker->streamFinishDelayMs = 100;
    auto* workerPtr = worker.get();
    auto controller = makeAsyncWorkerController(std::move(worker));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setLocalInferenceStreamingEnabled(true);

    QVERIFY(controller->sendMessage(QStringLiteral("hello")));
    QTRY_VERIFY(controller->localInferenceStreamingText().contains(QStringLiteral("async")));

    QVERIFY(!controller->clearChat());

    QVERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->localInferenceStreamingText(), QString());
    QCOMPARE(controller->conversationRuntimeRequestId(), QStringLiteral("None"));
    QCOMPARE(controller->conversationRuntimeActiveModel(), QStringLiteral("None"));
    QCOMPARE(controller->conversationRuntimeActiveRoute(), QStringLiteral("Provider"));
    QCOMPARE(workerPtr->cancelledRequestIds.size(), 1);
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->chatHistory().first().role, sentinel::core::ChatRole::System);
}

void ApplicationControllerTest::clearChatKeepsSingleInitialSystemMessage() {
    auto store = std::make_unique<RecordingChatHistoryStore>();
    const auto storePtr = store.get();
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    QVERIFY(controller.clearChat());
    QVERIFY(controller.clearChat());

    QCOMPARE(controller.chatHistory().size(), 1);
    QCOMPARE(controller.chatHistory().first().role, sentinel::core::ChatRole::System);
    QCOMPARE(controller.chatHistory().first().content, QStringLiteral("Sentinel Core online."));
    QCOMPARE(storePtr->messages_.size(), 1);
    QCOMPARE(storePtr->messages_.first().role, sentinel::core::ChatRole::System);
}

void ApplicationControllerTest::asyncDuplicateSendIsRejectedBeforeAppending() {
    auto worker = std::make_unique<AsyncLocalInferenceWorker>();
    worker->delayMs = 25;
    auto controller = makeAsyncWorkerController(std::move(worker));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    QVERIFY(controller->sendMessage(QStringLiteral("hello")));
    const auto duplicateAccepted = controller->sendMessage(QStringLiteral("duplicate"));

    QVERIFY(!duplicateAccepted);
    QCOMPARE(controller->chatHistory().size(), 2);
    QCOMPARE(controller->chatHistory().at(1).content, QStringLiteral("hello"));
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Busy"));

    QTRY_VERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(2).content, QStringLiteral("async fake completion"));
}

void ApplicationControllerTest::staleAsyncResultIsIgnoredAfterCancellation() {
    auto worker = std::make_unique<AsyncLocalInferenceWorker>();
    auto* workerPtr = worker.get();
    worker->delayMs = 25;
    auto controller = makeAsyncWorkerController(std::move(worker));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    QVERIFY(controller->sendMessage(QStringLiteral("hello")));
    QVERIFY(controller->localInferenceBusy());
    QVERIFY(controller->cancelLocalInference());

    QVERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->chatHistory().size(), 2);
    QVERIFY(!workerPtr->cancelledRequestIds.isEmpty());

    QTest::qWait(50);
    QCOMPARE(controller->chatHistory().size(), 2);
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->localInferenceSummary(),
             QStringLiteral("Local inference request was cancelled; stale results will be "
                            "ignored."));
    QCOMPARE(controller->conversationRuntimeLastErrorSummary(),
             QStringLiteral("Local inference request was cancelled; stale results will be "
                            "ignored."));
}

void ApplicationControllerTest::localInferenceTimeoutAppendsConciseFailureAndResetsBusy() {
    auto timeoutClient = std::make_unique<CategorizedErrorLocalInferenceClient>(
        sentinel::core::LocalInferenceError::Timeout,
        QStringLiteral("Local Ollama generation timed out after 30000 ms."));
    auto* timeoutClientPtr = timeoutClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(timeoutClient));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(timeoutClientPtr->called);
    QVERIFY(!controller->localInferenceBusy());
    QCOMPARE(timeoutClientPtr->lastRequest.options.timeoutMs, 30000);
    QCOMPARE(controller->localInferenceRuntimeState(), QStringLiteral("Failed"));
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(controller->chatHistory().at(2).content,
             QStringLiteral("Local inference failed: the local Ollama request timed out."));
    QCOMPARE(controller->chatSendLifecycleState(), QStringLiteral("failed"));
}

void ApplicationControllerTest::
    localInferenceMalformedResponseAppendsConciseFailureAndResetsBusy() {
    auto malformedClient = std::make_unique<CategorizedErrorLocalInferenceClient>(
        sentinel::core::LocalInferenceError::InvalidResponse,
        QStringLiteral("Local Ollama generation failed: Ollama returned malformed JSON."));
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(malformedClient));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->localInferenceRuntimeState(), QStringLiteral("Failed"));
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(2).content,
             QStringLiteral("Local inference failed: Ollama returned an invalid response."));
    QCOMPARE(controller->chatSendLifecycleState(), QStringLiteral("failed"));
}

void ApplicationControllerTest::ollamaUnavailablePathAppendsConciseFailureWithoutRealService() {
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(QList<OllamaModelSummary>{}));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(!sent);
    QVERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->localInferenceRuntimeState(), QStringLiteral("Failed"));
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->localChatInferenceStatus(), QStringLiteral("Ollama Unreachable"));
    QCOMPARE(controller->localChatSendAvailable(), false);
    QCOMPARE(controller->localChatSendAvailabilitySummary(),
             QStringLiteral("Ollama is not reachable. Start Ollama locally, then try again."));
}

void ApplicationControllerTest::duplicateSendDuringActiveStreamIsRejectedWithoutAppending() {
    auto streamClient = std::make_unique<ReentrantLocalInferenceStreamClient>();
    auto* streamClientPtr = streamClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::make_unique<FakeLocalInferenceClient>(), std::move(streamClient));
    streamClientPtr->controller = controller.get();

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    controller->setLocalInferenceStreamingEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(streamClientPtr->called);
    QVERIFY(!streamClientPtr->duplicateAccepted);
    QVERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(1).content, QStringLiteral("hello"));
    QCOMPARE(controller->chatHistory().at(2).content, QStringLiteral("final response"));
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Succeeded"));
}

void ApplicationControllerTest::streamingInterruptionClearsPreviewAndAppendsOneFailure() {
    auto streamClient = std::make_unique<FakeLocalInferenceStreamClient>(
        QList<LocalInferenceStreamChunk>{
            {1, QStringLiteral("partial "), false, false, QStringLiteral("first chunk")},
        },
        LocalInferenceStreamStatus::Error,
        QStringLiteral("Local Ollama streaming response did not complete with assistant text."));
    auto historyStore = std::make_unique<RecordingChatHistoryStore>();
    auto* historyStorePtr = historyStore.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr,
        std::move(historyStore), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(),
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::make_unique<FakeLocalInferenceClient>(), std::move(streamClient));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    controller->setLocalInferenceStreamingEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(!controller->localInferenceBusy());
    QVERIFY(controller->localInferenceStreamingText().isEmpty());
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(controller->chatHistory().at(2).content,
             QStringLiteral("Local inference failed: the Ollama stream ended before a complete "
                            "assistant response was received."));
    QCOMPARE(historyStorePtr->messages_.size(), 3);
    QCOMPARE(controller->chatSendLifecycleState(), QStringLiteral("failed"));
}

void ApplicationControllerTest::blocksLocalInferenceByDefaultPermission() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::localInferenceChanged);

    const auto ran =
        controller->runLocalInference(QStringLiteral("hello"), QStringLiteral("llama3.2"));

    QVERIFY(!ran);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->localInferenceSummary(),
             QStringLiteral("Local inference blocked by runtime permission policy."));
    QVERIFY(controller->localInferenceTraceSummaries().contains(
        QStringLiteral("2. Permission Policy [Denied]: Runtime permission policy is "
                       "metadata-only and denies execution by default.")));
}

void ApplicationControllerTest::blocksLocalInferenceWhenSafetyPolicyBlocks() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(),
        std::make_unique<BlockLocalInferenceSafetyPolicy>(), nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));

    const auto ran =
        controller->runLocalInference(QStringLiteral("hello"), QStringLiteral("llama3.2"));

    QVERIFY(!ran);
    QVERIFY(!fakeClientPtr->called);
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->localInferenceSummary(),
             QStringLiteral("Local inference blocked by runtime safety policy."));
    QVERIFY(controller->localInferenceTraceSummaries().contains(
        QStringLiteral("3. Safety Policy [Blocked]: Injected local safety policy blocked "
                       "execution.")));
}

void ApplicationControllerTest::runsInjectedLocalInferenceWhenPermissionAllows() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, std::move(fakeClient));
    QSignalSpy spy(controller.get(), &ApplicationController::localInferenceChanged);

    const auto ran =
        controller->runLocalInference(QStringLiteral("hello"), QStringLiteral("local-model"));

    QVERIFY(ran);
    QVERIFY(fakeClientPtr->called);
    QCOMPARE(fakeClientPtr->lastRequest.prompt, QStringLiteral("hello"));
    QCOMPARE(fakeClientPtr->lastRequest.options.model, QStringLiteral("local-model"));
    QCOMPARE(spy.count(), 2);
    QVERIFY(!controller->localInferenceBusy());
    QCOMPARE(controller->localInferenceRuntimeState(), QStringLiteral("Idle"));
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Succeeded"));
    QCOMPARE(controller->localInferenceLastResponseSummary(),
             QStringLiteral("Local inference completed for local-model (21 characters)."));
    QVERIFY(controller->localInferenceLatencySummary().contains(QStringLiteral("ms")));
    QVERIFY(controller->localInferenceTraceSummaries().contains(
        QStringLiteral("1. Permission Policy [Allowed]: Local inference permission allowed for "
                       "injected test policy.")));
    QVERIFY(controller->localInferenceTraceSummaries().contains(
        QStringLiteral("2. Safety Policy [Compliant]: Runtime safety policy report: local-only "
                       "and no-execution posture is enforced with deterministic metadata rules.")));
}

void ApplicationControllerTest::usesProviderChatPathWhenLocalChatInferenceDisabled() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));

    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(!fakeClientPtr->called);
    QCOMPARE(controller->localChatInferenceStatus(), QStringLiteral("Disabled"));
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(1).role, sentinel::core::ChatRole::User);
    QCOMPARE(controller->chatHistory().at(2).content,
             QStringLiteral("Sentinel Core online. Local chat pipeline is active."));
}

void ApplicationControllerTest::enabledLocalChatInferenceWithoutValidModelFailsSafely() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(QList<OllamaModelSummary>{}),
        std::move(fakeClient));

    controller->setLocalChatInferenceEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(!sent);
    QCOMPARE(controller->chatSendLifecycleState(), QStringLiteral("refused"));
    QVERIFY(!fakeClientPtr->called);
    QCOMPARE(controller->localChatInferenceStatus(), QStringLiteral("Missing Model"));
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->localChatSendAvailable(), false);
    QCOMPARE(controller->localChatSendAvailabilitySummary(),
             QStringLiteral("Select an installed Ollama model in Settings before sending."));
}

void ApplicationControllerTest::enabledLocalChatInferenceWithInvalidModelShowsSafeSummary() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));

    controller->setSelectedLocalModel(QStringLiteral("missing-model"));
    controller->setLocalChatInferenceEnabled(true);

    QVERIFY(!controller->sendMessage(QStringLiteral("hello")));
    QCOMPARE(controller->chatSendLifecycleState(), QStringLiteral("refused"));
    QVERIFY(!fakeClientPtr->called);
    QCOMPARE(controller->localChatInferenceStatus(), QStringLiteral("Invalid Model"));
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Model Unavailable"));
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->localChatSendAvailable(), false);
    QCOMPARE(controller->localChatSendAvailabilitySummary(),
             QStringLiteral("Selected model missing-model is not installed in Ollama. Choose an "
                            "available model in Settings."));
}

void ApplicationControllerTest::enabledLocalChatInferenceAppendsFakeResponse() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto historyStore = std::make_unique<RecordingChatHistoryStore>();
    auto* historyStorePtr = historyStore.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr,
        std::move(historyStore), nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(),
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));
    QSignalSpy chatSpy(controller.get(), &ApplicationController::chatMessagesChanged);

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(fakeClientPtr->called);
    QCOMPARE(fakeClientPtr->lastRequest.options.model, QStringLiteral("llama3.2"));
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(1).content, QStringLiteral("hello"));
    QCOMPARE(controller->chatHistory().at(2).content, QStringLiteral("fake local completion"));
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Received);
    QCOMPARE(historyStorePtr->messages_.size(), 3);
    QCOMPARE(chatSpy.count(), 2);
}

void ApplicationControllerTest::enabledLocalChatInferenceErrorAppendsSafeRefusal() {
    auto errorClient = std::make_unique<ErrorLocalInferenceClient>();
    auto* errorClientPtr = errorClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(errorClient));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(sent);
    QVERIFY(errorClientPtr->called);
    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(controller->chatHistory().at(2).content,
             QStringLiteral("Local inference unavailable: the local Ollama client is not ready."));
    QCOMPARE(controller->chatSendLifecycleState(), QStringLiteral("failed"));
}

void ApplicationControllerTest::localChatInferenceBlocksNonLoopbackEndpoint() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    OllamaConfig config;
    config.endpoint.url = QUrl(QStringLiteral("https://example.com"));
    config.endpoint.valid = false;
    config.endpoint.normalizedFromInvalid = false;
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}, config),
        std::move(fakeClient));

    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));

    controller->setLocalChatInferenceEnabled(true);
    const auto sent = controller->sendMessage(QStringLiteral("hello"));

    QVERIFY(!sent);
    QCOMPARE(controller->chatSendLifecycleState(), QStringLiteral("refused"));
    QVERIFY(!fakeClientPtr->called);
    QCOMPARE(controller->localChatInferenceStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->localChatSendAvailable(), false);
    QCOMPARE(controller->localChatSendAvailabilitySummary(),
             QStringLiteral("Ollama must use a local loopback HTTP endpoint."));
}

void ApplicationControllerTest::exposesConversationSessionMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->conversationSessionId(), QStringLiteral("conversation-session-1"));
    QCOMPARE(controller->conversationSessionStatus(), QStringLiteral("Active"));
    QCOMPARE(controller->interactionMode(), QStringLiteral("Companion"));
    QCOMPARE(controller->attentionState(), QStringLiteral("Observing"));
    QCOMPARE(controller->currentConversationSession().revision, 1);
    QCOMPARE(controller->contextWindowSummary(),
             QStringLiteral("Workspace context window: Local Only route, Atlas (Coordinator, "
                            "Available, Local), Ambient (Available, Public Metadata, Session)."));
    QCOMPARE(controller->currentConversationSession().contextWindow.currentRoutingMode,
             QStringLiteral("Local Only"));
    QCOMPARE(controller->currentConversationSession().contextWindow.preferredAgentSummary,
             QStringLiteral("Atlas (Coordinator, Available, Local)"));
}

void ApplicationControllerTest::exposesConversationStateMetadata() {
    const auto controller = makeController();

    QCOMPARE(controller->conversationState(), QStringLiteral("Idle"));
    QCOMPARE(controller->conversationTransitionStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller->conversationTransitionSummary(),
             QStringLiteral("No conversation transition yet."));
}

void ApplicationControllerTest::updatesModelRoutingModeMetadata() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::modelRoutingChanged);
    QSignalSpy taskPlanSpy(controller.get(), &ApplicationController::taskPlanChanged);
    QSignalSpy conversationSpy(controller.get(),
                               &ApplicationController::conversationSessionChanged);
    QSignalSpy snapshotSpy(controller.get(), &ApplicationController::orchestrationSnapshotChanged);

    controller->setRoutingModeByName(QStringLiteral("Balanced"));

    QCOMPARE(controller->currentRoutingMode(), QStringLiteral("Balanced"));
    QCOMPARE(controller->modelRoutingStatus(), QStringLiteral("Routed"));
    QCOMPARE(controller->selectedModelProviderSummary(),
             QStringLiteral("Balanced -> Local Metadata Provider / Sentinel Local Placeholder"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(taskPlanSpy.count(), 1);
    QCOMPARE(controller->latestTaskPlanStatus(), QStringLiteral("Fallback Planned"));
    QCOMPARE(controller->currentOrchestrationSnapshot().workspace.routingMode,
             QStringLiteral("Balanced"));
    QCOMPARE(controller->currentConversationSession().revision, 2);
    QCOMPARE(controller->currentConversationSession().contextWindow.currentRoutingMode,
             QStringLiteral("Balanced"));
    QVERIFY(controller->contextWindowSummary().contains(QStringLiteral("Balanced route")));
    QCOMPARE(conversationSpy.count(), 1);
    QCOMPARE(controller->orchestrationSnapshotStatus(), QStringLiteral("Ready"));
    QCOMPARE(snapshotSpy.count(), 1);

    controller->setRoutingModeByName(QStringLiteral("unknown"));
    QCOMPARE(controller->currentRoutingMode(), QStringLiteral("Local Only"));
    QCOMPARE(spy.count(), 2);
    QCOMPARE(taskPlanSpy.count(), 2);
    QCOMPARE(conversationSpy.count(), 2);
    QCOMPARE(snapshotSpy.count(), 2);
}

void ApplicationControllerTest::keepsConversationSessionSeparateFromChatAndRuntimeSessions() {
    const auto controller = makeController();

    QCOMPARE(controller->conversationSessionId(), QStringLiteral("conversation-session-1"));
    QCOMPARE(controller->runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->conversationState(), QStringLiteral("Idle"));
    QCOMPARE(controller->conversationTransitionStatus(), QStringLiteral("Not Requested"));

    controller->setRoutingModeByName(QStringLiteral("Quality"));

    QCOMPARE(controller->conversationSessionId(), QStringLiteral("conversation-session-1"));
    QCOMPARE(controller->runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller->runtimeContextStatus(), QStringLiteral("Empty"));
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->chatMessages().first(), QStringLiteral("Sentinel: Sentinel Core online."));
    QCOMPARE(controller->conversationState(), QStringLiteral("Idle"));
    QCOMPARE(controller->conversationTransitionStatus(), QStringLiteral("Not Requested"));
}

void ApplicationControllerTest::executesDeterministicAgentRequestWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());
    QSignalSpy statusSpy(&controller, &ApplicationController::agentStatusChanged);
    QSignalSpy responseSpy(&controller, &ApplicationController::agentResponseChanged);
    QSignalSpy planSpy(&controller, &ApplicationController::toolPlanChanged);
    QSignalSpy approvalSpy(&controller, &ApplicationController::approvalChanged);
    QSignalSpy sandboxSpy(&controller, &ApplicationController::sandboxChanged);
    QSignalSpy toolExecutionSpy(&controller, &ApplicationController::toolExecutionChanged);
    QSignalSpy runtimeContextSpy(&controller, &ApplicationController::runtimeContextChanged);
    QSignalSpy activitySpy(&controller, &ApplicationController::agentActivityChanged);
    QSignalSpy conversationStateSpy(&controller, &ApplicationController::conversationStateChanged);

    const auto ran = controller.runAgentRequest(QStringLiteral("check local plan"));

    QVERIFY(ran);
    QCOMPARE(controller.agentStatus(), QStringLiteral("Ready"));
    QCOMPARE(controller.lastAgentResponse(),
             QStringLiteral("Local agent placeholder processed: check local plan"));
    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestToolPlanSummary(),
             QStringLiteral("Metadata-only tool plan prepared."));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(controller.latestApprovalSummary(),
             QStringLiteral("Planned tool invocations do not require approval."));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Allowed"));
    QCOMPARE(controller.latestSandboxSummary(),
             QStringLiteral("Planned tool capabilities are allowed by sandbox metadata policy."));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(controller.runtimeContextStatus(), QStringLiteral("Active"));
    QCOMPARE(controller.runtimeContextSummary(),
             QStringLiteral("Runtime context captured pipeline result: Placeholder Succeeded"));
    QCOMPARE(controller.runtimeContextActiveToolIds(),
             QStringList{QStringLiteral("local-plan-summary")});
    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Placeholder Succeeded"));
    QCOMPARE(controller.conversationState(), QStringLiteral("Completed"));
    QCOMPARE(controller.conversationTransitionStatus(), QStringLiteral("Accepted"));
    QCOMPARE(controller.conversationTransitionSummary(),
             QStringLiteral("Accepted conversation transition: Responding -> Completed: agent "
                            "response metadata completed"));
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(responseSpy.count(), 1);
    QCOMPARE(planSpy.count(), 1);
    QCOMPARE(approvalSpy.count(), 1);
    QCOMPARE(sandboxSpy.count(), 1);
    QCOMPARE(toolExecutionSpy.count(), 1);
    QCOMPARE(runtimeContextSpy.count(), 1);
    QCOMPARE(activitySpy.count(), 1);
    QVERIFY(conversationStateSpy.count() >= 6);
}

void ApplicationControllerTest::exposesAgentToolMetadata() {
    const auto controllerWithoutRuntime = makeController();
    QCOMPARE(controllerWithoutRuntime->availableToolCount(), 0);
    QVERIFY(controllerWithoutRuntime->availableToolIds().isEmpty());

    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());

    QCOMPARE(controller.availableToolCount(), 1);
    QCOMPARE(controller.availableToolIds(), QStringList{QStringLiteral("local-plan-summary")});
}

void ApplicationControllerTest::exposesLatestToolPlanStatusWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());

    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestToolPlanSummary(), QStringLiteral("No tool plan yet."));

    QVERIFY(!controller.runAgentRequest(QStringLiteral("   ")));
    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestToolPlanSummary(), QStringLiteral("No tool plan yet."));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestToolPlanSummary(),
             QStringLiteral("Metadata-only tool plan prepared."));
}

void ApplicationControllerTest::exposesLatestApprovalStatusWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());

    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestApprovalSummary(), QStringLiteral("No approval decision yet."));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(controller.latestApprovalSummary(),
             QStringLiteral("Planned tool invocations do not require approval."));
}

void ApplicationControllerTest::exposesLatestSandboxStatusWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());

    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Not Evaluated"));
    QCOMPARE(controller.latestSandboxSummary(), QStringLiteral("No sandbox evaluation yet."));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Allowed"));
    QCOMPARE(controller.latestSandboxSummary(),
             QStringLiteral("Planned tool capabilities are allowed by sandbox metadata policy."));
}

void ApplicationControllerTest::exposesLatestToolExecutionStatusWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());

    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("No tool execution boundary result yet."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("No agent pipeline result yet."));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
}

void ApplicationControllerTest::exposesSuccessfulPipelineResultWithRuntime() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());
    QSignalSpy pipelineSpy(&controller, &ApplicationController::agentPipelineChanged);

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));

    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Allowed"));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Placeholder Succeeded"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Placeholder tool execution completed without performing actions."));
    QCOMPARE(pipelineSpy.count(), 1);
}

void ApplicationControllerTest::exposesRuntimeContextForPipelineResult() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());
    QSignalSpy runtimeContextSpy(&controller, &ApplicationController::runtimeContextChanged);

    QCOMPARE(controller.runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller.runtimeContextStatus(), QStringLiteral("Empty"));
    QCOMPARE(controller.runtimeContextSummary(), QStringLiteral("No runtime context yet."));
    QVERIFY(controller.runtimeContextActiveToolIds().isEmpty());

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));

    QCOMPARE(controller.runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller.runtimeContextStatus(), QStringLiteral("Active"));
    QCOMPARE(controller.runtimeContextSummary(),
             QStringLiteral("Runtime context captured pipeline result: Placeholder Succeeded"));
    QCOMPARE(controller.runtimeContextActiveToolIds(),
             QStringList{QStringLiteral("local-plan-summary")});
    QCOMPARE(runtimeContextSpy.count(), 1);
}

void ApplicationControllerTest::exposesAgentActivityForPipelineResult() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::make_unique<sentinel::core::NullAgentRuntime>());
    QSignalSpy activitySpy(&controller, &ApplicationController::agentActivityChanged);

    QCOMPARE(controller.agentActivityCount(), 0);
    QCOMPARE(controller.latestAgentActivitySummary(), QStringLiteral("No agent activity yet."));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft local plan")));

    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Placeholder Succeeded"));
    QCOMPARE(activitySpy.count(), 1);
}

void ApplicationControllerTest::reportsRiskyToolPlanRequiresApproval() {
    auto runtime =
        std::make_unique<sentinel::core::NullAgentRuntime>(QList<sentinel::core::ToolDescriptor>{
            sentinel::core::ToolDescriptor{
                QStringLiteral("risky-tool"),
                QStringLiteral("Risky Tool"),
                QStringLiteral("Risk metadata only."),
                sentinel::core::ToolRiskLevel::High,
                sentinel::core::ToolExecutionMode::MetadataOnly,
                {},
            },
        });
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, nullptr,
                                     std::move(runtime));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft risky plan")));
    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Requires Approval"));
    QCOMPARE(controller.latestApprovalSummary(),
             QStringLiteral("One or more planned tool invocations require approval."));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Blocked By Approval"));
    QCOMPARE(controller.latestSandboxSummary(),
             QStringLiteral("Sandbox capability evaluation is blocked by approval metadata."));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("Execution boundary blocked by approval metadata."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Execution boundary blocked by approval metadata."));
    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Blocked"));
    QCOMPARE(controller.conversationState(), QStringLiteral("Waiting For Approval"));
    QCOMPARE(controller.conversationTransitionStatus(), QStringLiteral("Accepted"));
    QCOMPARE(controller.conversationTransitionSummary(),
             QStringLiteral("Accepted conversation transition: Routing -> Waiting For Approval: "
                            "approval metadata required"));
}

void ApplicationControllerTest::reportsSandboxBlockedPipelineResult() {
    const sentinel::core::ToolDescriptor tool{
        QStringLiteral("local-tool"),
        QStringLiteral("Local Tool"),
        QStringLiteral("Metadata only."),
        sentinel::core::ToolRiskLevel::Low,
        sentinel::core::ToolExecutionMode::MetadataOnly,
        {},
    };
    sentinel::core::ToolInvocationPlan plan{
        sentinel::core::ToolInvocationPlanStatus::Planned,
        QStringLiteral("Metadata-only tool plan prepared."),
        {
            sentinel::core::PlannedToolInvocation{
                tool.id,
                tool.name,
                QStringLiteral("Plan metadata."),
                QStringLiteral("Metadata-only rationale."),
                tool.riskLevel,
                tool.executionMode,
                {},
                {
                    sentinel::core::CapabilityDescriptor{
                        QStringLiteral("tool.blocked.capability"),
                        QStringLiteral("Blocked metadata capability."),
                    },
                },
            },
        },
    };
    ApplicationController controller(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        std::make_unique<FixedPlanRuntime>(QList<sentinel::core::ToolDescriptor>{tool}, plan));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft sandbox blocked plan")));

    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Denied"));
    QCOMPARE(
        controller.latestSandboxSummary(),
        QStringLiteral("One or more planned capabilities are outside sandbox metadata policy."));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("Execution boundary blocked by sandbox capability metadata."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Blocked"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Execution boundary blocked by sandbox capability metadata."));
    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Blocked"));
}

void ApplicationControllerTest::reportsEmptyPlanPipelineResult() {
    sentinel::core::ToolInvocationPlan plan{
        sentinel::core::ToolInvocationPlanStatus::NoToolsAvailable,
        QStringLiteral("No tool metadata is available for planning."),
        {},
    };
    ApplicationController controller(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        std::make_unique<FixedPlanRuntime>(QList<sentinel::core::ToolDescriptor>{}, plan));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft empty plan")));

    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("No Tools Available"));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Requested"));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Not Evaluated"));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Empty Plan"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("No planned tool invocation reached the execution boundary."));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Empty Plan"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("No planned tool invocation reached the execution boundary."));
    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Empty Plan"));
}

void ApplicationControllerTest::reportsUnknownToolPipelineResult() {
    const sentinel::core::ToolDescriptor knownTool{
        QStringLiteral("known-tool"),
        QStringLiteral("Known Tool"),
        QStringLiteral("Metadata only."),
        sentinel::core::ToolRiskLevel::Low,
        sentinel::core::ToolExecutionMode::MetadataOnly,
        {},
    };
    sentinel::core::ToolInvocationPlan plan{
        sentinel::core::ToolInvocationPlanStatus::Planned,
        QStringLiteral("Metadata-only tool plan prepared."),
        {
            sentinel::core::PlannedToolInvocation{
                QStringLiteral("missing-tool"),
                QStringLiteral("Missing Tool"),
                QStringLiteral("Plan metadata."),
                QStringLiteral("Metadata-only rationale."),
                sentinel::core::ToolRiskLevel::Low,
                sentinel::core::ToolExecutionMode::MetadataOnly,
                {},
                {},
            },
        },
    };
    ApplicationController controller(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        std::make_unique<FixedPlanRuntime>(QList<sentinel::core::ToolDescriptor>{knownTool}, plan));

    QVERIFY(controller.runAgentRequest(QStringLiteral("draft unknown tool plan")));

    QCOMPARE(controller.latestToolPlanStatus(), QStringLiteral("Planned"));
    QCOMPARE(controller.latestApprovalStatus(), QStringLiteral("Not Required"));
    QCOMPARE(controller.latestSandboxStatus(), QStringLiteral("Allowed"));
    QCOMPARE(controller.latestToolExecutionStatus(), QStringLiteral("Unknown Tool"));
    QCOMPARE(controller.latestToolExecutionSummary(),
             QStringLiteral("Execution boundary rejected unknown tool metadata: missing-tool"));
    QCOMPARE(controller.latestAgentPipelineStatus(), QStringLiteral("Unknown Tool"));
    QCOMPARE(controller.latestAgentPipelineSummary(),
             QStringLiteral("Execution boundary rejected unknown tool metadata: missing-tool"));
    QCOMPARE(controller.agentActivityCount(), 6);
    QCOMPARE(controller.latestAgentActivitySummary(),
             QStringLiteral("Agent pipeline finished: Unknown Tool"));
}

void ApplicationControllerTest::exposesMemoryStatus() {
    const auto controller = makeController();

    QCOMPARE(controller->memoryStatus(), QStringLiteral("Available"));
}

void ApplicationControllerTest::sendsMessageThroughProvider() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    const auto messages = controller->chatMessages();
    QVERIFY(sent);
    QCOMPARE(messages.size(), 3);
    QCOMPARE(messages.at(1), QStringLiteral("You: status"));
    QCOMPARE(messages.at(2),
             QStringLiteral("Sentinel: Sentinel Core online. Local chat pipeline is active."));
    QCOMPARE(controller->chatHistory().at(1).status, sentinel::core::ChatMessageStatus::Sent);
    QCOMPARE(controller->chatHistory().at(2).status, sentinel::core::ChatMessageStatus::Received);
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::updatesConversationStateForChatFlow() {
    const auto controller = makeController();
    QSignalSpy stateSpy(controller.get(), &ApplicationController::conversationStateChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(controller->conversationState(), QStringLiteral("Completed"));
    QCOMPARE(controller->conversationTransitionStatus(), QStringLiteral("Accepted"));
    QCOMPARE(controller->conversationTransitionSummary(),
             QStringLiteral("Accepted conversation transition: Responding -> Completed: chat "
                            "response metadata completed"));
    QCOMPARE(controller->runtimeSessionId(), QStringLiteral("runtime-session-1"));
    QCOMPARE(controller->runtimeContextStatus(), QStringLiteral("Empty"));
    QVERIFY(stateSpy.count() >= 6);
}

void ApplicationControllerTest::ignoresBlankChatMessages() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("   "));

    QVERIFY(!sent);
    QCOMPARE(controller->chatMessages().size(), 1);
    QCOMPARE(spy.count(), 0);
}

void ApplicationControllerTest::handlesUnavailableProvider() {
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<UnavailableProvider>(), std::make_unique<InMemoryStore>());
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(controller->providerName(), QStringLiteral("UnavailableProvider"));
    QCOMPARE(controller->providerStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(controller->chatMessages().size(), 3);
    QCOMPARE(controller->chatMessages().last(),
             QStringLiteral("Sentinel: Provider unavailable. Status: Unavailable"));
    QCOMPARE(controller->chatHistory().last().status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::handlesProviderErrorReply() {
    auto controller = std::make_unique<ApplicationController>(std::make_unique<ErrorProvider>(),
                                                              std::make_unique<InMemoryStore>());
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    const auto sent = controller->sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(controller->chatMessages().size(), 3);
    QCOMPARE(controller->chatMessages().last(),
             QStringLiteral("Sentinel: Provider error: deterministic failure"));
    QCOMPARE(controller->chatHistory().last().status, sentinel::core::ChatMessageStatus::Error);
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::clearsChatHistory() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::chatMessagesChanged);

    controller->sendMessage(QStringLiteral("status"));
    controller->clearChat();

    QCOMPARE(controller->chatMessages(),
             QStringList{QStringLiteral("Sentinel: Sentinel Core online.")});
    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->chatHistory().first().id, 1);
    QCOMPARE(spy.count(), 2);
}

void ApplicationControllerTest::loadsPersistedChatHistoryAtStartup() {
    auto store = std::make_unique<RecordingChatHistoryStore>(QList<sentinel::core::ChatMessage>{
        {4, sentinel::core::ChatRole::System, QStringLiteral("previous system"),
         QDateTime::fromString(QStringLiteral("2026-05-15T12:00:00.000Z"), Qt::ISODateWithMs),
         sentinel::core::ChatMessageStatus::Received},
        {5, sentinel::core::ChatRole::User, QStringLiteral("previous user"),
         QDateTime::fromString(QStringLiteral("2026-05-15T12:01:00.000Z"), Qt::ISODateWithMs),
         sentinel::core::ChatMessageStatus::Sent},
    });
    const auto storePtr = store.get();

    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    QCOMPARE(controller.chatHistory().size(), 2);
    QCOMPARE(controller.chatHistory().first().id, 4);
    QCOMPARE(controller.chatMessages().first(), QStringLiteral("Sentinel: previous system"));
    QCOMPARE(storePtr->messages_.size(), 2);
}

void ApplicationControllerTest::appendsNewChatMessagesToHistoryStore() {
    auto store = std::make_unique<RecordingChatHistoryStore>();
    const auto storePtr = store.get();
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    const auto sent = controller.sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(storePtr->messages_.size(), 3);
    QCOMPARE(storePtr->messages_.at(0).role, sentinel::core::ChatRole::System);
    QCOMPARE(storePtr->messages_.at(1).role, sentinel::core::ChatRole::User);
    QCOMPARE(storePtr->messages_.at(1).content, QStringLiteral("status"));
    QCOMPARE(storePtr->messages_.at(2).role, sentinel::core::ChatRole::Assistant);
}

void ApplicationControllerTest::clearsPersistentChatHistoryWhenAvailable() {
    auto store = std::make_unique<RecordingChatHistoryStore>();
    const auto storePtr = store.get();
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    controller.sendMessage(QStringLiteral("status"));
    const auto cleared = controller.clearChat();

    QVERIFY(cleared);
    QVERIFY(storePtr->wasCleared_);
    QCOMPARE(storePtr->messages_.size(), 1);
    QCOMPARE(storePtr->messages_.first().id, 1);
    QCOMPARE(storePtr->messages_.first().role, sentinel::core::ChatRole::System);
    QCOMPARE(controller.chatHistory().size(), 1);
    QCOMPARE(controller.chatMaintenanceStatus(), QStringLiteral("Clear completed"));
}

void ApplicationControllerTest::keepsRuntimeChatWorkingWhenHistoryStoreUnavailable() {
    auto store =
        std::make_unique<RecordingChatHistoryStore>(QList<sentinel::core::ChatMessage>{}, false);
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));

    const auto sent = controller.sendMessage(QStringLiteral("status"));

    QVERIFY(sent);
    QCOMPARE(controller.chatHistory().size(), 3);
    QCOMPARE(controller.chatHistory().at(1).content, QStringLiteral("status"));
}

void ApplicationControllerTest::storesRuntimeMemoryEntries() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);

    controller->remember(QStringLiteral(" callsign "), QStringLiteral(" Sentinel "));

    QCOMPARE(controller->memoryEntries(), QStringList{QStringLiteral("callsign: Sentinel")});
    QCOMPARE(spy.count(), 1);

    controller->remember(QString(), QStringLiteral("ignored"));
    QCOMPARE(controller->memoryEntries(), QStringList{QStringLiteral("callsign: Sentinel")});
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::createsMemoryCandidatesPendingReview() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryCandidatesChanged);

    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("The user prefers concise local-only architecture notes."));

    QCOMPARE(id, QStringLiteral("memory-candidate-1"));
    QCOMPARE(controller->memoryCandidateCount(), 1);
    QCOMPARE(controller->pendingMemoryCandidateCount(), 1);
    QCOMPARE(controller->approvedMemoryCandidateCount(), 0);
    QVERIFY(
        controller->memoryCandidateSummaries().first().contains(QStringLiteral("Pending Review")));
    QCOMPARE(spy.count(), 1);
}

void ApplicationControllerTest::recordsMemoryCandidateApprovalAndRejectionMetadata() {
    const auto controller = makeController();

    const auto approvedId = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Remember this project uses Qt and QML."));
    const auto rejectedId = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Temporary phrasing should not become durable memory."));

    QVERIFY(controller->approveMemoryCandidate(approvedId));
    QVERIFY(controller->rejectMemoryCandidate(rejectedId));
    QVERIFY(!controller->approveMemoryCandidate(QStringLiteral("missing")));
    QCOMPARE(controller->pendingMemoryCandidateCount(), 0);
    QCOMPARE(controller->approvedMemoryCandidateCount(), 1);
    QCOMPARE(controller->rejectedMemoryCandidateCount(), 1);
    QVERIFY(controller->memoryCandidateSummaries().at(0).contains(QStringLiteral("Approved")));
    QVERIFY(controller->memoryCandidateSummaries().at(1).contains(QStringLiteral("Rejected")));
    QCOMPARE(controller->lastMemoryCandidateReviewStatus(), QStringLiteral("Refused"));
    QVERIFY(controller->lastMemoryCandidateReviewSummary().contains(QStringLiteral("not found")));
}

void ApplicationControllerTest::resetsMemoryCandidateReviewMetadata() {
    const auto controller = makeController();
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Reset this reviewed candidate to pending."));

    QVERIFY(controller->approveMemoryCandidate(id));
    QVERIFY(controller->resetMemoryCandidate(id));

    QCOMPARE(controller->pendingMemoryCandidateCount(), 1);
    QCOMPARE(controller->approvedMemoryCandidateCount(), 0);
    QCOMPARE(controller->rejectedMemoryCandidateCount(), 0);
    QVERIFY(
        controller->memoryCandidateSummaries().first().contains(QStringLiteral("Pending Review")));
    QVERIFY(controller->memoryCandidateSummaries().first().contains(
        QStringLiteral("Reviewer: User review")));
}

void ApplicationControllerTest::refusesInvalidMemoryCandidateReviewTransitions() {
    const auto controller = makeController();
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Invalid transitions must not change candidate state."));

    QVERIFY(!controller->resetMemoryCandidate(id));
    QVERIFY(!controller->archiveMemoryCandidate(id));
    QVERIFY(controller->approveMemoryCandidate(id));
    QVERIFY(!controller->rejectMemoryCandidate(id));

    QCOMPARE(controller->pendingMemoryCandidateCount(), 0);
    QCOMPARE(controller->approvedMemoryCandidateCount(), 1);
    QCOMPARE(controller->rejectedMemoryCandidateCount(), 0);
    QCOMPARE(controller->archivedMemoryCandidateCount(), 0);
    QCOMPARE(controller->lastMemoryCandidateReviewStatus(), QStringLiteral("Refused"));
}

void ApplicationControllerTest::archivesReviewedMemoryCandidatesAsTerminalMetadata() {
    const auto controller = makeController();
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Archived candidate metadata should be terminal."));

    QVERIFY(controller->rejectMemoryCandidate(id));
    QVERIFY(controller->archiveMemoryCandidate(id));
    QVERIFY(!controller->resetMemoryCandidate(id));

    QCOMPARE(controller->memoryCandidateCount(), 1);
    QCOMPARE(controller->pendingMemoryCandidateCount(), 0);
    QCOMPARE(controller->approvedMemoryCandidateCount(), 0);
    QCOMPARE(controller->rejectedMemoryCandidateCount(), 0);
    QCOMPARE(controller->archivedMemoryCandidateCount(), 1);
    QVERIFY(controller->archivedMemoryCandidateSummaries().first().contains(
        QStringLiteral("Archived")));
    QCOMPARE(controller->lastMemoryCandidateReviewStatus(), QStringLiteral("Refused"));
}

void ApplicationControllerTest::memoryCandidatesDoNotMutateKeyValueMemory() {
    const auto controller = makeController();
    controller->remember(QStringLiteral("mode"), QStringLiteral("Companion"));

    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Candidate metadata should not write to IMemoryStore."));
    QVERIFY(controller->approveMemoryCandidate(id));

    QCOMPARE(controller->memoryEntries(), QStringList{QStringLiteral("mode: Companion")});
}

void ApplicationControllerTest::approvedMemoryCandidateCommitsToKeyValueMemory() {
    const auto controller = makeController();
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("The project keeps memory commits behind explicit user action."));

    QVERIFY(controller->approveMemoryCandidate(id));

    QCOMPARE(controller->memoryCommitPlanCount(), 1);
    QCOMPARE(controller->memoryCommitReadinessStatus(), QStringLiteral("Ready"));
    QVERIFY(controller->memoryCommitReadinessSummary().contains(
        QStringLiteral("explicit user action")));
    QVERIFY(controller->memoryCommitTargetSummary().contains(QStringLiteral("Key-value Memory")));
    QVERIFY(
        controller->memoryCommitTargetSummary().contains(QStringLiteral("Refuse Existing Key")));
    QVERIFY(controller->memoryCommitCandidateSummaries().first().contains(id));
    QVERIFY(controller->memoryCommitCandidateSummaries().first().contains(
        QStringLiteral("Key-value Memory")));
    QVERIFY(controller->memoryCommitReadinessChecks()
                .join(QStringLiteral(" "))
                .contains(QStringLiteral("explicit user action")));
    QVERIFY(controller->requestMemoryCandidateCommit(id));
    QCOMPARE(controller->lastMemoryCommitStatus(), QStringLiteral("Committed"));
    QVERIFY(controller->lastMemoryCommitResultSummary().contains(
        QStringLiteral("memory.semantic.conversation-memory-candidate.memory-candidate-1")));
    QCOMPARE(controller->committedMemoryCandidateCount(), 1);
    QVERIFY(controller->memoryCandidateSummaries().first().contains(QStringLiteral("Committed")));
    QCOMPARE(controller->memoryEntries(),
             QStringList{QStringLiteral("memory.semantic.conversation-memory-candidate."
                                        "memory-candidate-1: The project keeps memory commits "
                                        "behind explicit user action.")});
}

void ApplicationControllerTest::pendingAndRejectedMemoryCandidatesCannotCommit() {
    const auto controller = makeController();
    const auto pendingId = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Pending candidates are not commit-ready."));
    const auto rejectedId = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Rejected candidates are not commit-ready."));

    QVERIFY(!controller->requestMemoryCandidateCommit(pendingId));
    QCOMPARE(controller->lastMemoryCommitStatus(), QStringLiteral("Refused"));
    QVERIFY(
        controller->lastMemoryCommitResultSummary().contains(QStringLiteral("requires review")));

    QVERIFY(controller->rejectMemoryCandidate(rejectedId));
    QVERIFY(!controller->requestMemoryCandidateCommit(rejectedId));
    QCOMPARE(controller->lastMemoryCommitStatus(), QStringLiteral("Refused"));
    QVERIFY(controller->lastMemoryCommitResultSummary().contains(QStringLiteral("rejected")));
}

void ApplicationControllerTest::archivedMemoryCandidateCannotCommit() {
    const auto controller = makeController();
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Archived candidates cannot be committed."));

    QVERIFY(controller->approveMemoryCandidate(id));
    QVERIFY(controller->archiveMemoryCandidate(id));
    QVERIFY(!controller->requestMemoryCandidateCommit(id));

    QCOMPARE(controller->lastMemoryCommitStatus(), QStringLiteral("Refused"));
    QVERIFY(controller->lastMemoryCommitResultSummary().contains(QStringLiteral("archived")));
    QVERIFY(controller->memoryEntries().isEmpty());
}

void ApplicationControllerTest::duplicateMemoryCommitKeyRefusesByDefault() {
    const auto controller = makeController();
    controller->remember(QStringLiteral("memory.semantic.conversation-memory-candidate."
                                        "memory-candidate-1"),
                         QStringLiteral("Existing value"));
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Duplicate keys should refuse before overwrite."));

    QVERIFY(controller->approveMemoryCandidate(id));
    QVERIFY(!controller->requestMemoryCandidateCommit(id));

    QCOMPARE(controller->lastMemoryCommitStatus(), QStringLiteral("Refused"));
    QVERIFY(controller->lastMemoryCommitResultSummary().contains(QStringLiteral("already exists")));
    QCOMPARE(controller->committedMemoryCandidateCount(), 0);
    QCOMPARE(controller->memoryEntries(),
             QStringList{QStringLiteral("memory.semantic.conversation-memory-candidate."
                                        "memory-candidate-1: Existing value")});
}

void ApplicationControllerTest::approvalDoesNotAutomaticallyCommitMemory() {
    const auto controller = makeController();
    controller->remember(QStringLiteral("mode"), QStringLiteral("Companion"));
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Approved candidates wait for explicit Commit."));

    QVERIFY(controller->approveMemoryCandidate(id));

    QCOMPARE(controller->lastMemoryCandidateReviewStatus(), QStringLiteral("Accepted"));
    QCOMPARE(controller->lastMemoryCommitStatus(), QStringLiteral("No Commit"));
    QCOMPARE(controller->committedMemoryCandidateCount(), 0);
    QCOMPARE(controller->memoryEntries(), QStringList{QStringLiteral("mode: Companion")});
}

void ApplicationControllerTest::localMemoryRecallFindsCommittedKeyValueMemory() {
    const auto controller = makeController();
    controller->remember(QStringLiteral("project.mode"), QStringLiteral("Companion local mode"));
    controller->remember(QStringLiteral("preference.tone"), QStringLiteral("Concise answers"));

    QVERIFY(controller->recallLocalMemory(QStringLiteral("concise")));

    QCOMPARE(controller->memoryRecallPolicyStatus(), QStringLiteral("Local Literal Recall"));
    QCOMPARE(controller->memoryRecallStatus(), QStringLiteral("Completed"));
    QCOMPARE(controller->memoryRecallResultCount(), 1);
    QCOMPARE(controller->memoryEntryCount(), 2);
    QVERIFY(controller->memoryRecallSummaryText().contains(QStringLiteral("Found 1")));
    QVERIFY(controller->memoryRecallResultSummaries().first().contains(
        QStringLiteral("preference.tone")));
    QVERIFY(controller->memoryRecallResultSummaries().first().contains(
        QStringLiteral("matched value")));
}

void ApplicationControllerTest::emptyLocalMemoryRecallDoesNotMutateMemory() {
    const auto controller = makeController();
    controller->remember(QStringLiteral("mode"), QStringLiteral("Companion"));
    const auto beforeEntries = controller->memoryEntries();
    const auto beforeMessages = controller->chatHistory().size();

    QVERIFY(!controller->recallLocalMemory(QStringLiteral("   ")));

    QCOMPARE(controller->memoryRecallStatus(), QStringLiteral("Empty Query"));
    QCOMPARE(controller->memoryRecallResultCount(), 0);
    QCOMPARE(controller->memoryEntries(), beforeEntries);
    QCOMPARE(controller->chatHistory().size(), beforeMessages);
}

void ApplicationControllerTest::localMemoryRecallAfterCommitFindsCommittedCandidate() {
    const auto controller = makeController();
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Committed recall should find this local value."));
    QVERIFY(controller->approveMemoryCandidate(id));
    QVERIFY(controller->requestMemoryCandidateCommit(id));

    QVERIFY(controller->recallLocalMemory(QStringLiteral("recall")));

    QCOMPARE(controller->memoryRecallStatus(), QStringLiteral("Completed"));
    QCOMPARE(controller->memoryRecallResultCount(), 1);
    QVERIFY(controller->memoryRecallResultSummaries().first().contains(
        QStringLiteral("memory.semantic.conversation-memory-candidate.memory-candidate-1")));
}

void ApplicationControllerTest::clearChatKeepsCommittedMemoryRecallAvailable() {
    const auto controller = makeController();
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Committed memory recall survives clear chat."));
    QVERIFY(controller->approveMemoryCandidate(id));
    QVERIFY(controller->requestMemoryCandidateCommit(id));

    QVERIFY(controller->sendMessage(QStringLiteral("status")));
    controller->clearChat();
    QVERIFY(controller->recallLocalMemory(QStringLiteral("survives")));

    QCOMPARE(controller->chatHistory().size(), 1);
    QCOMPARE(controller->memoryRecallResultCount(), 1);
    QCOMPARE(controller->committedMemoryCandidateCount(), 1);
}

void ApplicationControllerTest::localMemoryRecallDoesNotInjectIntoPrompt() {
    const auto controller = makeController();
    controller->remember(QStringLiteral("secret.local"), QStringLiteral("do not inject"));
    QVERIFY(controller->recallLocalMemory(QStringLiteral("secret")));

    QVERIFY(controller->sendMessage(QStringLiteral("hello")));

    QCOMPARE(controller->chatHistory().size(), 3);
    QCOMPARE(controller->chatHistory().at(1).content, QStringLiteral("hello"));
    QVERIFY(!controller->chatHistory().at(2).content.contains(QStringLiteral("do not inject")));
    QVERIFY(!controller->conversationRuntimeLastSuccessSummary().contains(
        QStringLiteral("do not inject")));
}

void ApplicationControllerTest::contextAssemblyShowsCommittedMemorySource() {
    const auto controller = makeController();
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Context assembly should see committed local memory."));
    QVERIFY(controller->approveMemoryCandidate(id));
    QVERIFY(controller->requestMemoryCandidateCommit(id));

    QCOMPARE(controller->contextAssemblyPolicyStatus(), QStringLiteral("Planning Only"));
    QCOMPARE(controller->contextAssemblyStatus(), QStringLiteral("Ready"));
    QCOMPARE(controller->committedMemoryContextAvailability(), QStringLiteral("Available"));
    QVERIFY(controller->contextAssemblySourceSummaries()
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Committed Memory Context: Available")));
    QVERIFY(controller->contextAssemblyReadinessChecks().contains(
        QStringLiteral("Prompt assembly: disabled")));
    QVERIFY(controller->contextAssemblyReadinessChecks().contains(
        QStringLiteral("Automatic context attachment: disabled")));
}

void ApplicationControllerTest::contextAssemblyPlanningDoesNotMutateOrInjectPrompt() {
    const auto controller = makeController();
    controller->remember(QStringLiteral("secret.local"), QStringLiteral("planning only"));
    const auto beforeEntries = controller->memoryEntries();
    const auto beforeMessages = controller->chatHistory().size();

    const auto summary = controller->contextAssemblySummary();

    QCOMPARE(summary.status, sentinel::core::ContextAssemblyStatus::Ready);
    QCOMPARE(controller->memoryEntries(), beforeEntries);
    QCOMPARE(controller->chatHistory().size(), beforeMessages);

    QVERIFY(controller->sendMessage(QStringLiteral("hello")));
    QVERIFY(!controller->chatHistory().last().content.contains(QStringLiteral("planning only")));
}

void ApplicationControllerTest::promptContextInjectionDisabledLeavesPromptUnchanged() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);

    QVERIFY(controller->sendMessage(QStringLiteral("hello")));

    QVERIFY(fakeClientPtr->called);
    QCOMPARE(fakeClientPtr->lastRequest.prompt, QStringLiteral("hello"));
    QCOMPARE(controller->promptContextInjectionStatus(), QStringLiteral("Disabled"));
    QCOMPARE(controller->promptContextInjectedBlockCount(), 0);
}

void ApplicationControllerTest::enabledPromptContextInjectionIncludesDeterministicBundle() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));
    controller->remember(QStringLiteral("preference.tone"), QStringLiteral("Concise answers"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setPromptContextInjectionEnabled(true);

    QVERIFY(controller->sendMessage(QStringLiteral("hello")));

    const auto prompt = fakeClientPtr->lastRequest.prompt;
    QVERIFY(prompt.startsWith(QStringLiteral("[Sentinel Local Context]")));
    QVERIFY(prompt.contains(QStringLiteral("--- Bounded Conversation History ---")));
    QVERIFY(prompt.contains(QStringLiteral("--- Committed Local Memory ---")));
    QVERIFY(prompt.contains(QStringLiteral("preference.tone = Concise answers")));
    QVERIFY(prompt.contains(QStringLiteral("[/Sentinel Local Context]\n\nUser prompt:\nhello")));
    QCOMPARE(controller->promptContextInjectionStatus(), QStringLiteral("Injected"));
    QVERIFY(controller->promptContextInjectedBlockCount() >= 3);
    QVERIFY(controller->promptContextSourceSummary().contains(QStringLiteral("Committed Memory")));
}

void ApplicationControllerTest::promptContextInjectionUsesOnlyCommittedMemory() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));
    const auto pendingId = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Pending private candidate must not be injected."));
    const auto rejectedId = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Rejected private candidate must not be injected."));
    QVERIFY(controller->rejectMemoryCandidate(rejectedId));
    Q_UNUSED(pendingId);
    controller->remember(QStringLiteral("approved.local"), QStringLiteral("committed only"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setPromptContextInjectionEnabled(true);

    QVERIFY(controller->sendMessage(QStringLiteral("hello")));

    const auto prompt = fakeClientPtr->lastRequest.prompt;
    QVERIFY(prompt.contains(QStringLiteral("approved.local = committed only")));
    QVERIFY(!prompt.contains(QStringLiteral("Pending private candidate")));
    QVERIFY(!prompt.contains(QStringLiteral("Rejected private candidate")));
}

void ApplicationControllerTest::promptContextInjectionBudgetTruncatesDeterministically() {
    const auto result = sentinel::core::injectPromptContext(
        QStringLiteral("hello"),
        QList<sentinel::core::PromptContextBlock>{
            {sentinel::core::ContextAssemblySourceKind::Conversation,
             QStringLiteral("Current Conversation Context"), QStringLiteral("abcdefghij")},
            {sentinel::core::ContextAssemblySourceKind::CommittedMemory,
             QStringLiteral("Committed Local Memory"), QStringLiteral("klmnopqrst")},
        },
        sentinel::core::PromptContextInjectionPolicy{true, 12});

    QCOMPARE(result.status, sentinel::core::PromptContextInjectionStatus::Truncated);
    QCOMPARE(result.injectedBlockCount, 2);
    QVERIFY(result.prompt.contains(QStringLiteral("abcdefghij")));
    QVERIFY(result.prompt.contains(QStringLiteral("kl")));
    QVERIFY(!result.prompt.contains(QStringLiteral("klm")));
    QVERIFY(result.sizeSummary.contains(QStringLiteral("12 of 20")));
}

void ApplicationControllerTest::promptContextInjectionUsesBoundedRecentConversationWindow() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));

    for (int i = 0; i < 14; ++i) {
        QVERIFY(controller->sendMessage(
            QStringLiteral("history marker %1 %2").arg(i).arg(QString(120, QLatin1Char('x')))));
    }
    const auto messageCountBeforePrompt = controller->conversationHistoryMessageCount();
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setPromptContextInjectionEnabled(true);
    controller->remember(QStringLiteral("stable.memory"), QStringLiteral("kept separate"));

    QVERIFY(controller->sendMessage(QStringLiteral("final question")));

    const auto prompt = fakeClientPtr->lastRequest.prompt;
    QVERIFY(prompt.contains(QStringLiteral("--- Bounded Conversation History ---")));
    QVERIFY(prompt.contains(QStringLiteral("[Conversation History]")));
    QVERIFY(prompt.contains(QStringLiteral("[/Conversation History]")));
    QVERIFY(prompt.contains(QStringLiteral("--- Older Conversation Summary ---")));
    QVERIFY(prompt.contains(QStringLiteral("[Conversation Summary]")));
    QVERIFY(prompt.contains(QStringLiteral("[/Conversation Summary]")));
    QVERIFY(prompt.contains(QStringLiteral("--- Committed Local Memory ---")));
    QVERIFY(prompt.indexOf(QStringLiteral("[/Conversation History]")) <
            prompt.indexOf(QStringLiteral("--- Older Conversation Summary ---")));
    QVERIFY(prompt.indexOf(QStringLiteral("[/Conversation Summary]")) <
            prompt.indexOf(QStringLiteral("--- Committed Local Memory ---")));
    QVERIFY(prompt.contains(QStringLiteral("history marker 0")));
    QVERIFY(prompt.contains(QStringLiteral("history marker 13")));
    QCOMPARE(prompt.count(QStringLiteral("final question")), 1);
    QCOMPARE(controller->conversationHistoryMessageCount(), messageCountBeforePrompt + 2);
    QCOMPARE(controller->conversationWindowStatus(), QStringLiteral("Truncated"));
    QVERIFY(controller->conversationWindowIncludedMessageCount() > 0);
    QVERIFY(controller->conversationWindowOmittedMessageCount() > 0);
    QVERIFY(controller->conversationSummaryBlockCount() > 0);
    QVERIFY(controller->conversationSummaryMessageCount() > 0);
}

void ApplicationControllerTest::conversationSummaryUsesOlderWindowAndStaysSeparateFromMemory() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));

    for (int i = 0; i < 10; ++i) {
        QVERIFY(controller->sendMessage(
            QStringLiteral("summary marker %1 %2").arg(i).arg(QString(100, QLatin1Char('s')))));
    }
    controller->remember(QStringLiteral("summary.memory"), QStringLiteral("memory stays distinct"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setPromptContextInjectionEnabled(true);

    QVERIFY(controller->sendMessage(QStringLiteral("summary final question")));

    const auto prompt = fakeClientPtr->lastRequest.prompt;
    const auto summaryStart = prompt.indexOf(QStringLiteral("--- Older Conversation Summary ---"));
    const auto memoryStart = prompt.indexOf(QStringLiteral("--- Committed Local Memory ---"));
    QVERIFY(summaryStart >= 0);
    QVERIFY(memoryStart > summaryStart);
    QVERIFY(prompt.mid(summaryStart, memoryStart - summaryStart)
                .contains(QStringLiteral("summary marker 0")));
    QVERIFY(!prompt.mid(summaryStart, memoryStart - summaryStart)
                 .contains(QStringLiteral("summary.memory = memory stays distinct")));
    QVERIFY(
        prompt.mid(memoryStart).contains(QStringLiteral("summary.memory = memory stays distinct")));
    QCOMPARE(controller->conversationSummaryStatus(), QStringLiteral("Truncated"));
    QVERIFY(controller->conversationSummaryBudgetSummary().contains(QStringLiteral("700")));
    QVERIFY(!controller->conversationSummaryBlockSummaries().isEmpty());
}

void ApplicationControllerTest::conversationSummaryPlanningDoesNotMutateState() {
    const auto controller = makeController();
    for (int i = 0; i < 8; ++i) {
        QVERIFY(controller->sendMessage(QStringLiteral("summary planning marker %1 %2")
                                            .arg(i)
                                            .arg(QString(120, QLatin1Char('p')))));
    }
    controller->remember(QStringLiteral("summary.local"), QStringLiteral("unchanged"));
    const auto beforeEntries = controller->memoryEntries();
    const auto beforeMessages = controller->conversationHistoryMessageCount();

    const auto summary = controller->conversationSummaryResult();

    QVERIFY(summary.status == sentinel::core::ConversationSummaryStatus::Ready ||
            summary.status == sentinel::core::ConversationSummaryStatus::Truncated);
    QCOMPARE(controller->memoryEntries(), beforeEntries);
    QCOMPARE(controller->conversationHistoryMessageCount(), beforeMessages);
    QCOMPARE(controller->promptContextInjectionStatus(), QStringLiteral("Disabled"));
}

void ApplicationControllerTest::retrievalPlanningFeedsPromptContextWithoutMixingSources() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));

    for (int i = 0; i < 10; ++i) {
        QVERIFY(controller->sendMessage(
            QStringLiteral("retrieval marker %1 %2").arg(i).arg(QString(100, QLatin1Char('r')))));
    }
    controller->remember(QStringLiteral("retrieval.memory"),
                         QStringLiteral("committed memory remains distinct"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setPromptContextInjectionEnabled(true);

    const auto planningBeforePrompt = controller->retrievalPlanningResult();
    QVERIFY(planningBeforePrompt.status == sentinel::core::RetrievalPlanningStatus::Ready ||
            planningBeforePrompt.status == sentinel::core::RetrievalPlanningStatus::Truncated);
    QVERIFY(planningBeforePrompt.selectedSourceCount >= 4);
    QCOMPARE(controller->retrievalPlanningReadiness(), QStringLiteral("Ready"));

    QVERIFY(controller->sendMessage(QStringLiteral("retrieval final question")));

    const auto prompt = fakeClientPtr->lastRequest.prompt;
    const auto windowStart = prompt.indexOf(QStringLiteral("--- Bounded Conversation History ---"));
    const auto summaryStart = prompt.indexOf(QStringLiteral("--- Older Conversation Summary ---"));
    const auto memoryStart = prompt.indexOf(QStringLiteral("--- Committed Local Memory ---"));
    QVERIFY(windowStart >= 0);
    QVERIFY(summaryStart > windowStart);
    QVERIFY(memoryStart > summaryStart);
    QVERIFY(prompt.mid(summaryStart, memoryStart - summaryStart)
                .contains(QStringLiteral("retrieval marker 0")));
    QVERIFY(!prompt.mid(summaryStart, memoryStart - summaryStart)
                 .contains(QStringLiteral("retrieval.memory = committed memory remains distinct")));
    QVERIFY(prompt.mid(memoryStart)
                .contains(QStringLiteral("retrieval.memory = committed memory remains distinct")));
    QVERIFY(controller->retrievalPlanningSourceSummary().contains(
        QStringLiteral("Conversation Context")));
}

void ApplicationControllerTest::retrievalPlanningDoesNotMutateState() {
    const auto controller = makeController();
    for (int i = 0; i < 8; ++i) {
        QVERIFY(controller->sendMessage(QStringLiteral("retrieval planning marker %1 %2")
                                            .arg(i)
                                            .arg(QString(120, QLatin1Char('p')))));
    }
    controller->remember(QStringLiteral("retrieval.local"), QStringLiteral("unchanged"));
    const auto beforeEntries = controller->memoryEntries();
    const auto beforeMessages = controller->conversationHistoryMessageCount();
    const auto beforeInjectionStatus = controller->promptContextInjectionStatus();

    const auto planning = controller->retrievalPlanningResult();

    QVERIFY(planning.status == sentinel::core::RetrievalPlanningStatus::Ready ||
            planning.status == sentinel::core::RetrievalPlanningStatus::Truncated);
    QCOMPARE(controller->memoryEntries(), beforeEntries);
    QCOMPARE(controller->conversationHistoryMessageCount(), beforeMessages);
    QCOMPARE(controller->promptContextInjectionStatus(), beforeInjectionStatus);
}

void ApplicationControllerTest::semanticRetrievalMetadataDoesNotAffectPlanningOrPrompt() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));
    controller->remember(QStringLiteral("semantic.local"), QStringLiteral("deterministic only"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setPromptContextInjectionEnabled(true);

    const auto planningBefore = controller->retrievalPlanningResult();
    QCOMPARE(controller->semanticRetrievalEnabled(), false);
    QCOMPARE(controller->semanticRetrievalStatus(), QStringLiteral("Disabled"));
    QCOMPARE(controller->embeddingProviderReadiness(), QStringLiteral("Not Configured"));
    QCOMPARE(controller->vectorIndexReadiness(), QStringLiteral("Not Configured"));
    QCOMPARE(controller->vectorIndexedItemCount(), 0);
    QVERIFY(controller->semanticRetrievalReadinessChecks().contains(
        QStringLiteral("Semantic ranking: disabled")));
    QVERIFY(controller->semanticRetrievalReadinessChecks().contains(
        QStringLiteral("Raw vectors exposed to QML: no")));

    QVERIFY(controller->sendMessage(QStringLiteral("semantic final question")));

    const auto planningAfter = controller->retrievalPlanningResult();
    QCOMPARE(planningAfter.selectedSourceCount, planningBefore.selectedSourceCount);
    const auto prompt = fakeClientPtr->lastRequest.prompt;
    QVERIFY(prompt.contains(QStringLiteral("--- Committed Local Memory ---")));
    QVERIFY(prompt.contains(QStringLiteral("semantic.local = deterministic only")));
    QVERIFY(!prompt.contains(QStringLiteral("EmbeddingVector")));
    QVERIFY(!prompt.contains(QStringLiteral("VectorSearch")));
    QVERIFY(!prompt.contains(QStringLiteral("score")));
}

void ApplicationControllerTest::semanticProviderPlanningExposesDisabledSelection() {
    const auto controller = makeController();

    QCOMPARE(controller->semanticProviderMode(), QStringLiteral("Disabled"));
    QCOMPARE(controller->selectedSemanticProviderName(), QStringLiteral("Disabled"));
    QCOMPARE(controller->semanticProviderReadiness(), QStringLiteral("Disabled"));
    QCOMPARE(controller->semanticProviderHealth(), QStringLiteral("Not Checked"));
    QCOMPARE(controller->semanticActivationReadiness(), QStringLiteral("Refused"));
    QVERIFY(controller->semanticProviderStatusSummary().contains(
        QStringLiteral("Selected semantic provider: Disabled")));
    QVERIFY(controller->semanticActivationSummary().contains(
        QStringLiteral("Semantic activation refused")));
    QVERIFY(controller->semanticProviderCapabilitySummaries().contains(
        QStringLiteral("Prompt mutation blocked")));
    QVERIFY(controller->semanticActivationRequiredSteps()
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("deterministic retrieval")));
}

void ApplicationControllerTest::semanticCandidateOrchestrationExposesSafeMetadata() {
    const auto controller = makeController();
    for (int i = 0; i < 8; ++i) {
        QVERIFY(controller->sendMessage(QStringLiteral("semantic candidate marker %1 %2")
                                            .arg(i)
                                            .arg(QString(110, QLatin1Char('c')))));
    }
    controller->remember(QStringLiteral("candidate.local"), QStringLiteral("source isolated"));

    const auto arbitration = controller->semanticCandidateArbitration();

    QVERIFY(arbitration.status == sentinel::core::SemanticCandidateStatus::Ready ||
            arbitration.status == sentinel::core::SemanticCandidateStatus::Truncated);
    QVERIFY(controller->semanticCandidateCount() >= controller->semanticCandidateSelectedCount());
    QVERIFY(controller->semanticCandidateSelectedCount() >= 4);
    QVERIFY(controller->semanticCandidateExcludedCount() >= 1);
    QVERIFY(controller->semanticCandidateParticipationSummaries()
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Recent Conversation Window")));
    QVERIFY(controller->semanticCandidateParticipationSummaries()
                .join(QStringLiteral("\n"))
                .contains(QStringLiteral("Future Semantic/Vector Candidates")));
    QCOMPARE(controller->hybridRetrievalStatus(), QStringLiteral("Deterministic Only"));
    QVERIFY(
        controller->hybridRetrievalSummary().contains(QStringLiteral("semantic path disabled")));
    QVERIFY(controller->hybridRetrievalReadinessChecks().contains(
        QStringLiteral("Vector database activation: disabled")));
}

void ApplicationControllerTest::semanticArbitrationSimulationExposesSafeMetadata() {
    const auto controller = makeController();
    for (int i = 0; i < 6; ++i) {
        QVERIFY(controller->sendMessage(QStringLiteral("semantic arbitration marker %1 %2")
                                            .arg(i)
                                            .arg(QString(80, QLatin1Char('a')))));
    }
    controller->remember(QStringLiteral("arbitration.local"), QStringLiteral("metadata only"));

    const auto planningBefore = controller->retrievalPlanningResult();
    const auto simulated = controller->semanticArbitrationResult();
    const auto planningAfter = controller->retrievalPlanningResult();

    QCOMPARE(planningAfter.selectedCandidateCount, planningBefore.selectedCandidateCount);
    QCOMPARE(controller->semanticArbitrationStatus(), QStringLiteral("Simulated"));
    QVERIFY(controller->semanticArbitrationReadiness().contains(QStringLiteral("cannot change "
                                                                               "prompts")));
    QVERIFY(controller->semanticArbitrationSummary().contains(
        QStringLiteral("Deterministic retrieval remains final authority")));
    QVERIFY(controller->semanticArbitrationBudgetSummary().contains(
        QStringLiteral("0 semantic candidates selected")));
    QVERIFY(!controller->semanticArbitrationSelectionSummaries().isEmpty());
    QVERIFY(controller->semanticArbitrationChecks().contains(
        QStringLiteral("Real embeddings: disabled")));
    QVERIFY(controller->semanticArbitrationChecks().contains(
        QStringLiteral("Vector search: disabled")));
    QCOMPARE(simulated.budget.semanticSelectedCandidateCount, 0);
    QCOMPARE(controller->embeddingRuntimeReadiness(), QStringLiteral("Blocked"));
    QVERIFY(controller->embeddingRuntimeSummary().contains(QStringLiteral("no embeddings")));
    QVERIFY(controller->embeddingRuntimeBudgetSummary().contains(
        QStringLiteral("planned embedding jobs")));
    QVERIFY(controller->embeddingRuntimeRequirementSummaries().contains(
        QStringLiteral("Committed-memory-only indexing policy")));
    QVERIFY(controller->embeddingRuntimeConstraintSummaries().contains(
        QStringLiteral("No Ollama embedding calls while disabled")));
}

void ApplicationControllerTest::isolatedEmbeddingRuntimeExposureIsBoundedAndDisabledByDefault() {
    const auto controller = makeController();
    const auto planningBefore = controller->retrievalPlanningResult();
    const auto promptBefore = controller->latestPromptContextInjectionResult();

    QCOMPARE(controller->isolatedEmbeddingRuntimeStatus(), QStringLiteral("Refused"));
    QCOMPARE(controller->isolatedEmbeddingRuntimeHealth(), QStringLiteral("Blocked"));
    QCOMPARE(controller->isolatedEmbeddingRuntimeReadiness(), QStringLiteral("Refused"));
    QVERIFY(controller->isolatedEmbeddingRuntimeSummary().contains(
        QStringLiteral("readiness metadata only")));
    QVERIFY(controller->isolatedEmbeddingRuntimeBoundedState().contains(
        QStringLiteral("no vectors persisted")));
    QVERIFY(controller->isolatedEmbeddingRuntimeChecks().contains(
        QStringLiteral("Prompt integration: disabled")));
    QVERIFY(controller->isolatedEmbeddingRuntimeChecks().contains(
        QStringLiteral("Retrieval ranking mutation: disabled")));
    QVERIFY(controller->isolatedEmbeddingRuntimeChecks().contains(
        QStringLiteral("Vector DB persistence: disabled")));

    const auto planningAfter = controller->retrievalPlanningResult();
    const auto promptAfter = controller->latestPromptContextInjectionResult();
    QCOMPARE(planningAfter.selectedCandidateCount, planningBefore.selectedCandidateCount);
    QCOMPARE(promptAfter.injectedBlockCount, promptBefore.injectedBlockCount);
}

void ApplicationControllerTest::
    vectorPersistenceExposureIsDisabledAndDoesNotMutatePlanningOrPrompt() {
    const auto controller = makeController();
    const auto planningBefore = controller->retrievalPlanningResult();
    const auto promptBefore = controller->latestPromptContextInjectionResult();

    QCOMPARE(controller->vectorPersistenceStatus(), QStringLiteral("Disabled"));
    QCOMPARE(controller->vectorPersistenceHealth(), QStringLiteral("Blocked"));
    QCOMPARE(controller->vectorPersistenceReadiness(), QStringLiteral("Disabled"));
    QCOMPARE(controller->vectorPersistenceIndexedItemCount(), 0);
    QVERIFY(controller->vectorPersistenceSummary().contains(QStringLiteral("disabled by default")));
    QVERIFY(controller->vectorPersistenceBoundedState().contains(QStringLiteral("local-only")));
    QVERIFY(controller->vectorPersistenceChecks().contains(
        QStringLiteral("Automatic indexing: disabled")));
    QVERIFY(controller->vectorPersistenceChecks().contains(
        QStringLiteral("Filesystem scanning: disabled")));
    QVERIFY(controller->vectorPersistenceChecks().contains(
        QStringLiteral("Semantic retrieval authority: disabled")));
    QVERIFY(controller->vectorPersistenceChecks().contains(
        QStringLiteral("Prompt mutation: disabled")));

    const auto planningAfter = controller->retrievalPlanningResult();
    const auto promptAfter = controller->latestPromptContextInjectionResult();
    QCOMPARE(planningAfter.selectedCandidateCount, planningBefore.selectedCandidateCount);
    QCOMPARE(promptAfter.injectedBlockCount, promptBefore.injectedBlockCount);
    QCOMPARE(controller->semanticRetrievalEnabled(), false);
}

void ApplicationControllerTest::semanticSearchExposureIsBoundedAndDoesNotMutatePlanningOrPrompt() {
    const auto controller = makeController();
    const auto planningBefore = controller->retrievalPlanningResult();
    const auto promptBefore = controller->latestPromptContextInjectionResult();

    QCOMPARE(controller->semanticSearchStatus(), QStringLiteral("Disabled"));
    QCOMPARE(controller->semanticSearchCandidateCount(), 0);
    QVERIFY(controller->semanticSearchReadiness().contains(QStringLiteral("disabled")));
    QVERIFY(controller->semanticSearchSummary().contains(QStringLiteral("disabled")));
    QVERIFY(controller->semanticSearchRuntimeState().contains(QStringLiteral("local-only")));
    QVERIFY(controller->semanticSearchBudgetSummary().contains(
        QStringLiteral("0 bounded semantic candidates")));
    QVERIFY(controller->semanticSearchArbitrationSummary().contains(
        QStringLiteral("deterministic candidates remain final authority")));
    QVERIFY(controller->semanticSearchChecks().contains(
        QStringLiteral("Filesystem indexing: disabled")));
    QVERIFY(controller->semanticSearchChecks().contains(
        QStringLiteral("Semantic prompt authority: disabled")));
    QVERIFY(controller->semanticSearchChecks().contains(
        QStringLiteral("RetrievalPlanningResult mutation: no")));

    const auto planningAfter = controller->retrievalPlanningResult();
    const auto promptAfter = controller->latestPromptContextInjectionResult();
    QCOMPARE(planningAfter.selectedCandidateCount, planningBefore.selectedCandidateCount);
    QCOMPARE(planningAfter.selectedSourceCount, planningBefore.selectedSourceCount);
    QCOMPARE(promptAfter.injectedBlockCount, promptBefore.injectedBlockCount);
    QCOMPARE(controller->semanticRetrievalEnabled(), false);
}

void ApplicationControllerTest::hybridBridgeExposureIsBoundedAndDoesNotMutatePlanningOrPrompt() {
    const auto controller = makeController();
    controller->sendMessage(QStringLiteral("hybrid bridge deterministic context"));
    controller->remember(QStringLiteral("hybrid.bridge"),
                         QStringLiteral("deterministic memory authority"));
    const auto planningBefore = controller->retrievalPlanningResult();
    const auto promptBefore = controller->latestPromptContextInjectionResult();

    QCOMPARE(controller->hybridBridgeStatus(), QStringLiteral("Deterministic Only"));
    QVERIFY(controller->hybridBridgeReadiness().contains(QStringLiteral("deterministic fallback")));
    QVERIFY(controller->hybridBridgeSummary().contains(QStringLiteral("deterministic fallback")));
    QVERIFY(controller->hybridBridgeBudgetSummary().contains(QStringLiteral("bridge candidates")));
    QVERIFY(controller->hybridBridgeSourceSummary().contains(QStringLiteral("deterministic")));
    QVERIFY(controller->hybridBridgeArbitrationSummary().contains(
        QStringLiteral("Deterministic-first")));
    QVERIFY(controller->hybridBridgeFallbackSummary().contains(QStringLiteral("deterministic")));
    QVERIFY(controller->hybridBridgeCandidateCount() >= 1);
    QCOMPARE(controller->hybridBridgeSemanticFillCount(), 0);
    QVERIFY(controller->hybridBridgeChecks().contains(
        QStringLiteral("RetrievalPlanningResult mutation: no")));
    QVERIFY(controller->hybridBridgeChecks().contains(
        QStringLiteral("PromptContextBlocks mutation: no")));
    QVERIFY(controller->hybridBridgeChecks().contains(
        QStringLiteral("Prompt content injection: disabled")));

    const auto planningAfter = controller->retrievalPlanningResult();
    const auto promptAfter = controller->latestPromptContextInjectionResult();
    QCOMPARE(planningAfter.selectedCandidateCount, planningBefore.selectedCandidateCount);
    QCOMPARE(planningAfter.selectedSourceCount, planningBefore.selectedSourceCount);
    QCOMPARE(promptAfter.injectedBlockCount, promptBefore.injectedBlockCount);
    QCOMPARE(controller->semanticRetrievalEnabled(), false);
}

void ApplicationControllerTest::
    semanticAcceptanceExposureIsBoundedAndDoesNotMutatePlanningOrPrompt() {
    const auto controller = makeController();
    controller->sendMessage(QStringLiteral("semantic acceptance deterministic context"));
    controller->remember(QStringLiteral("semantic.acceptance"),
                         QStringLiteral("deterministic memory authority"));
    const auto planningBefore = controller->retrievalPlanningResult();
    const auto promptBefore = controller->latestPromptContextInjectionResult();

    QCOMPARE(controller->semanticAcceptanceStatus(), QStringLiteral("Deterministic Only"));
    QVERIFY(controller->semanticAcceptanceReadiness().contains(QStringLiteral("not ready")) ||
            controller->semanticAcceptanceReadiness().contains(QStringLiteral("Semantic")));
    QVERIFY(controller->semanticAcceptanceSummary().contains(QStringLiteral("deterministic")));
    QVERIFY(controller->semanticAcceptanceBudgetSummary().contains(
        QStringLiteral("semantic supplements")));
    QVERIFY(
        controller->semanticAcceptanceSourceSummary().contains(QStringLiteral("deterministic")));
    QVERIFY(
        controller->semanticAcceptanceArbitrationSummary().contains(QStringLiteral("acceptance")));
    QVERIFY(
        controller->semanticAcceptanceFallbackSummary().contains(QStringLiteral("deterministic")));
    QCOMPARE(controller->semanticAcceptanceAcceptedCount(), 0);
    QVERIFY(controller->semanticAcceptanceBudgetCharacters() > 0);
    QVERIFY(controller->semanticAcceptanceChecks().contains(
        QStringLiteral("RetrievalPlanningResult mutation: no")));
    QVERIFY(controller->semanticAcceptanceChecks().contains(
        QStringLiteral("PromptContextBlocks mutation: no")));
    QVERIFY(controller->semanticAcceptanceChecks().contains(
        QStringLiteral("Deterministic candidate replacement: no")));

    const auto planningAfter = controller->retrievalPlanningResult();
    const auto promptAfter = controller->latestPromptContextInjectionResult();
    QCOMPARE(planningAfter.selectedCandidateCount, planningBefore.selectedCandidateCount);
    QCOMPARE(planningAfter.selectedSourceCount, planningBefore.selectedSourceCount);
    QCOMPARE(promptAfter.injectedBlockCount, promptBefore.injectedBlockCount);
    QCOMPARE(controller->semanticRetrievalEnabled(), false);
}

void ApplicationControllerTest::
    semanticSupplementAssemblyExposureIsDisabledAndDoesNotMutatePlanningOrPrompt() {
    const auto controller = makeController();
    controller->sendMessage(QStringLiteral("semantic supplement assembly deterministic context"));
    controller->remember(QStringLiteral("semantic.supplement"),
                         QStringLiteral("deterministic memory remains authoritative"));
    const auto planningBefore = controller->retrievalPlanningResult();
    const auto promptBefore = controller->latestPromptContextInjectionResult();

    QCOMPARE(controller->semanticSupplementAssemblyStatus(), QStringLiteral("Disabled"));
    QCOMPARE(controller->semanticSupplementAssemblyBlockCount(), 0);
    QVERIFY(controller->semanticSupplementAssemblySummary().contains(QStringLiteral("disabled")));
    QVERIFY(controller->semanticSupplementAssemblyBudgetSummary().contains(QStringLiteral("0 of")));
    QVERIFY(controller->semanticSupplementAssemblySafetySummary().contains(
        QStringLiteral("non-authoritative")));
    QVERIFY(controller->semanticSupplementAssemblyChecks().contains(
        QStringLiteral("Live prompt inclusion: blocked")));
    QVERIFY(controller->semanticSupplementAssemblyChecks().contains(
        QStringLiteral("PromptContextBlock mutation: no")));
    QVERIFY(controller->semanticSupplementAssemblyChecks().contains(
        QStringLiteral("RetrievalPlanningResult mutation: no")));
    QVERIFY(controller->semanticSupplementAssemblyChecks().contains(
        QStringLiteral("Deterministic context replacement: no")));

    const auto planningAfter = controller->retrievalPlanningResult();
    const auto promptAfter = controller->latestPromptContextInjectionResult();
    QCOMPARE(planningAfter.selectedCandidateCount, planningBefore.selectedCandidateCount);
    QCOMPARE(planningAfter.selectedSourceCount, planningBefore.selectedSourceCount);
    QCOMPARE(promptAfter.injectedBlockCount, promptBefore.injectedBlockCount);
    QCOMPARE(promptAfter.status, promptBefore.status);
}

void ApplicationControllerTest::
    semanticPromptAuthorityExposureIsDefaultDeniedAndDoesNotMutatePrompt() {
    const auto controller = makeController();
    controller->sendMessage(QStringLiteral("semantic prompt authority deterministic context"));
    controller->remember(QStringLiteral("semantic.authority"),
                         QStringLiteral("deterministic retrieval remains authoritative"));
    const auto planningBefore = controller->retrievalPlanningResult();
    const auto promptBefore = controller->latestPromptContextInjectionResult();

    QCOMPARE(controller->semanticPromptAuthorityStatus(), QStringLiteral("Disabled"));
    QCOMPARE(controller->semanticPromptAuthorityWouldIncludeBlockCount(), 0);
    QCOMPARE(controller->semanticPromptInclusionEnabled(), false);
    QCOMPARE(controller->semanticPromptInclusionStatus(), QStringLiteral("Disabled"));
    QCOMPARE(controller->semanticPromptInclusionIncludedCount(), 0);
    QVERIFY(
        controller->semanticPromptAuthorityDecisionSummary().contains(QStringLiteral("Denied")));
    QVERIFY(controller->semanticPromptAuthoritySafetySummary().contains(
        QStringLiteral("deterministic-only fallback")));
    QVERIFY(controller->semanticPromptAuthorityFallbackSummary().contains(
        QStringLiteral("deterministic prompt assembly remains unchanged")));
    QVERIFY(controller->semanticPromptAuthorityAuditSummary().contains(QStringLiteral("disabled")));
    QVERIFY(controller->semanticPromptAuthorityChecks().contains(
        QStringLiteral("Live prompt mutation: blocked")));
    QVERIFY(controller->semanticPromptAuthorityChecks().contains(
        QStringLiteral("PromptContextBlock mutation: no")));
    QVERIFY(controller->semanticPromptAuthorityChecks().contains(
        QStringLiteral("RetrievalPlanningResult mutation: no")));
    QVERIFY(controller->semanticPromptAuthorityChecks().contains(
        QStringLiteral("Provider handles exposed: no")));
    QVERIFY(controller->semanticPromptInclusionSummary().contains(QStringLiteral("disabled")));
    QVERIFY(controller->semanticPromptInclusionBudgetSummary().contains(QStringLiteral("0 of")));
    QVERIFY(controller->semanticPromptInclusionFallbackSummary().contains(
        QStringLiteral("deterministic-only prompt assembly")));
    QVERIFY(controller->semanticPromptInclusionAuditSummary().contains(QStringLiteral("disabled")));
    QVERIFY(controller->semanticPromptInclusionDeterministicAuthorityPreserved());
    QVERIFY(controller->semanticPromptInclusionChecks().contains(
        QStringLiteral("Deterministic retrieval authoritative: yes")));
    QVERIFY(controller->semanticPromptInclusionChecks().contains(
        QStringLiteral("Raw prompt payloads exposed: no")));

    const auto planningAfter = controller->retrievalPlanningResult();
    const auto promptAfter = controller->latestPromptContextInjectionResult();
    QCOMPARE(planningAfter.selectedCandidateCount, planningBefore.selectedCandidateCount);
    QCOMPARE(planningAfter.selectedSourceCount, planningBefore.selectedSourceCount);
    QCOMPARE(promptAfter.injectedBlockCount, promptBefore.injectedBlockCount);
    QCOMPARE(promptAfter.status, promptBefore.status);
}

void ApplicationControllerTest::semanticCandidateOrchestrationDoesNotMutatePrompt() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));
    controller->remember(QStringLiteral("candidate.prompt"), QStringLiteral("deterministic"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setPromptContextInjectionEnabled(true);

    const auto candidateStatusBefore = controller->semanticCandidateStatus();

    QVERIFY(controller->sendMessage(QStringLiteral("semantic candidate question")));

    QCOMPARE(controller->semanticCandidateStatus(), candidateStatusBefore);
    QVERIFY(fakeClientPtr->lastRequest.prompt.contains(QStringLiteral("candidate.prompt = "
                                                                      "deterministic")));
    QVERIFY(!fakeClientPtr->lastRequest.prompt.contains(QStringLiteral("Future Semantic/Vector")));
    QVERIFY(!fakeClientPtr->lastRequest.prompt.contains(QStringLiteral("SemanticCandidate")));
    QVERIFY(!fakeClientPtr->lastRequest.prompt.contains(QStringLiteral("Hybrid retrieval")));
    QVERIFY(!fakeClientPtr->lastRequest.prompt.contains(QStringLiteral("simulated score")));
    QVERIFY(!fakeClientPtr->lastRequest.prompt.contains(QStringLiteral("Embedding runtime")));
}

void ApplicationControllerTest::promptContextInjectionDoesNotMutateMemoryOrCandidates() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(), nullptr, nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Candidate remains pending."));
    controller->remember(QStringLiteral("stable.local"), QStringLiteral("unchanged"));
    const auto beforeEntries = controller->memoryEntries();
    const auto beforeCandidateCount = controller->memoryCandidateCount();
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setPromptContextInjectionEnabled(true);

    QVERIFY(controller->sendMessage(QStringLiteral("hello")));

    QCOMPARE(controller->memoryEntries(), beforeEntries);
    QCOMPARE(controller->memoryCandidateCount(), beforeCandidateCount);
    QCOMPARE(controller->pendingMemoryCandidateCount(), 1);
    QCOMPARE(controller->memoryCandidateIds().first(), id);
}

void ApplicationControllerTest::promptContextInjectionRespectsSafetyGateBeforeAssembly() {
    auto fakeClient = std::make_unique<FakeLocalInferenceClient>();
    auto* fakeClientPtr = fakeClient.get();
    auto controller = std::make_unique<ApplicationController>(
        std::make_unique<LocalEchoProvider>(), std::make_unique<InMemoryStore>(), nullptr, nullptr,
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr, std::make_unique<AllowLocalInferencePolicy>(),
        std::make_unique<BlockLocalInferenceSafetyPolicy>(), nullptr, nullptr, nullptr, nullptr,
        nullptr, nullptr,
        std::make_unique<FakeOllamaRuntimeClient>(
            QList<OllamaModelSummary>{{QStringLiteral("llama3.2"), {}, 10}}),
        std::move(fakeClient));
    controller->remember(QStringLiteral("stable.local"), QStringLiteral("do not assemble"));
    controller->setSelectedLocalModel(QStringLiteral("llama3.2"));
    controller->setLocalChatInferenceEnabled(true);
    controller->setPromptContextInjectionEnabled(true);

    QVERIFY(controller->sendMessage(QStringLiteral("hello")));

    QVERIFY(!fakeClientPtr->called);
    QCOMPARE(controller->promptContextInjectionStatus(), QStringLiteral("Empty"));
    QCOMPARE(controller->localInferenceStatus(), QStringLiteral("Blocked"));
}

void ApplicationControllerTest::clearChatKeepsApprovedMemoryCandidateMetadata() {
    const auto controller = makeController();
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Keep this approved metadata across chat clear."));
    QVERIFY(controller->approveMemoryCandidate(id));

    controller->sendMessage(QStringLiteral("status"));
    controller->clearChat();

    QCOMPARE(controller->memoryCandidateCount(), 1);
    QCOMPARE(controller->approvedMemoryCandidateCount(), 1);
    QVERIFY(controller->memoryCandidateSummaries().first().contains(QStringLiteral("Approved")));
}

void ApplicationControllerTest::clearChatKeepsCommittedMemory() {
    const auto controller = makeController();
    const auto id = controller->createMemoryCandidateFromConversationText(
        QStringLiteral("Committed memory survives chat clear."));
    QVERIFY(controller->approveMemoryCandidate(id));
    QVERIFY(controller->requestMemoryCandidateCommit(id));

    controller->sendMessage(QStringLiteral("status"));
    controller->clearChat();

    QCOMPARE(controller->committedMemoryCandidateCount(), 1);
    QCOMPARE(controller->memoryEntries(),
             QStringList{QStringLiteral("memory.semantic.conversation-memory-candidate."
                                        "memory-candidate-1: Committed memory survives chat "
                                        "clear.")});
}

void ApplicationControllerTest::clearsRuntimeMemoryEntries() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);
    QSignalSpy maintenanceSpy(controller.get(), &ApplicationController::maintenanceStatusChanged);
    controller->remember(QStringLiteral("mode"), QStringLiteral("Companion"));

    const auto cleared = controller->clearMemory();

    QVERIFY(cleared);
    QVERIFY(controller->memoryEntries().isEmpty());
    QCOMPARE(controller->memoryMaintenanceStatus(), QStringLiteral("Clear completed"));
    QCOMPARE(spy.count(), 2);
    QCOMPARE(maintenanceSpy.count(), 1);
}

void ApplicationControllerTest::failsSafeWhenMemoryStoreUnavailable() {
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<UnavailableMemoryStore>());
    QSignalSpy spy(&controller, &ApplicationController::memoryEntriesChanged);
    QSignalSpy maintenanceSpy(&controller, &ApplicationController::maintenanceStatusChanged);

    const auto cleared = controller.clearMemory();

    QVERIFY(!cleared);
    QCOMPARE(controller.memoryStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(controller.memoryMaintenanceStatus(), QStringLiteral("Unavailable"));
    QCOMPARE(spy.count(), 0);
    QCOMPARE(maintenanceSpy.count(), 1);
}

void ApplicationControllerTest::rejectsBlankMemoryKeys() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);

    controller->remember(QStringLiteral("   "), QStringLiteral("ignored"));

    QVERIFY(controller->memoryEntries().isEmpty());
    QCOMPARE(spy.count(), 0);
}

void ApplicationControllerTest::overwritesMemoryEntriesThroughStoreBackend() {
    const auto controller = makeController();
    QSignalSpy spy(controller.get(), &ApplicationController::memoryEntriesChanged);

    controller->remember(QStringLiteral("mode"), QStringLiteral("Companion"));
    controller->remember(QStringLiteral("provider"), QStringLiteral("LocalEchoProvider"));
    controller->remember(QStringLiteral("mode"), QStringLiteral("Tactical"));

    const QStringList expected{
        QStringLiteral("mode: Tactical"),
        QStringLiteral("provider: LocalEchoProvider"),
    };

    QCOMPARE(controller->memoryEntries(), expected);
    QCOMPARE(spy.count(), 3);
}

void ApplicationControllerTest::reportsRuntimeOnlyWhenChatStoreUnavailableOnClear() {
    auto store =
        std::make_unique<RecordingChatHistoryStore>(QList<sentinel::core::ChatMessage>{}, false);
    ApplicationController controller(std::make_unique<LocalEchoProvider>(),
                                     std::make_unique<InMemoryStore>(), nullptr, std::move(store));
    QSignalSpy maintenanceSpy(&controller, &ApplicationController::maintenanceStatusChanged);

    controller.sendMessage(QStringLiteral("status"));
    const auto cleared = controller.clearChat();

    QVERIFY(!cleared);
    QCOMPARE(controller.chatHistory().size(), 1);
    QCOMPARE(controller.chatMaintenanceStatus(), QStringLiteral("Runtime Only"));
    QCOMPARE(maintenanceSpy.count(), 1);
}

QTEST_MAIN(ApplicationControllerTest)

#include "test_application_controller.moc"
