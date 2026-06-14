#include "sentinel/desktop/DesktopShellViewModel.h"

#include "sentinel/core/AppMetadata.h"
#include "sentinel/core/AppSettings.h"
#include "sentinel/core/ApplicationController.h"
#include "sentinel/core/ModelRegistry.h"
#include "sentinel/core/ModeManager.h"

#include <QCryptographicHash>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QIODevice>
#include <QSaveFile>
#include <QFileInfo>
#include <QLibraryInfo>
#include <QStandardPaths>
#include <QSysInfo>

#include <algorithm>
#include <functional>

namespace sentinel::desktop {

namespace {

QString appDataPath() {
    const auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return path.isEmpty() ? QDir::currentPath() : path;
}

QString localRagPath() {
    return appDataPath() + QStringLiteral("/local_rag.sqlite3");
}

QString attachmentId(const QString& workspaceId, const QString& name, qint64 size) {
    const auto seed = workspaceId + QStringLiteral("|") + name + QStringLiteral("|") +
                      QString::number(size);
    return QString::fromLatin1(
        QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha1).toHex().left(16));
}

QString fileTypeForPath(const QString& pathOrName) {
    const auto suffix = QFileInfo(pathOrName).suffix().toLower();
    if (suffix == QStringLiteral("pdf")) {
        return QStringLiteral("PDF");
    }
    if (suffix == QStringLiteral("txt")) {
        return QStringLiteral("TXT");
    }
    if (suffix == QStringLiteral("md") || suffix == QStringLiteral("markdown")) {
        return QStringLiteral("Markdown");
    }
    if (suffix == QStringLiteral("docx")) {
        return QStringLiteral("DOCX");
    }
    if (suffix == QStringLiteral("csv")) {
        return QStringLiteral("CSV");
    }
    if (suffix == QStringLiteral("json")) {
        return QStringLiteral("JSON");
    }
    if (QStringList({QStringLiteral("cpp"), QStringLiteral("h"), QStringLiteral("hpp"),
                     QStringLiteral("qml"), QStringLiteral("js"), QStringLiteral("ts"),
                     QStringLiteral("py"), QStringLiteral("java"), QStringLiteral("cs"),
                     QStringLiteral("go"), QStringLiteral("rs"), QStringLiteral("swift")})
            .contains(suffix)) {
        return QStringLiteral("Source Code");
    }
    return QStringLiteral("Unsupported");
}

bool supportedAttachmentType(const QString& type) {
    return type != QStringLiteral("Unsupported");
}

QJsonArray defaultNotifications() {
    QJsonArray notifications;
    const auto add = [&notifications](const QString& id, const QString& category,
                                      const QString& title, const QString& body, bool pinned) {
        QJsonObject item;
        item.insert(QStringLiteral("id"), id);
        item.insert(QStringLiteral("category"), category);
        item.insert(QStringLiteral("title"), title);
        item.insert(QStringLiteral("body"), body);
        item.insert(QStringLiteral("pinned"), pinned);
        item.insert(QStringLiteral("archived"), false);
        item.insert(QStringLiteral("read"), false);
        notifications.append(item);
    };
    add(QStringLiteral("updates-manual"), QStringLiteral("Updates"),
        QStringLiteral("Manual updates only"),
        QStringLiteral("Sentinel checks for updates only after explicit user action."), true);
    add(QStringLiteral("security-privacy"), QStringLiteral("Security"),
        QStringLiteral("Privacy guarantees active"),
        QStringLiteral("No telemetry, hidden uploads, silent updates, or hidden cloud activation."),
        true);
    add(QStringLiteral("tasks-controlled"), QStringLiteral("Tasks"),
        QStringLiteral("Controlled tasks require approval"),
        QStringLiteral("Task execution advances only through visible user actions."), false);
    add(QStringLiteral("brain-local"), QStringLiteral("Brain"),
        QStringLiteral("Brain data stays local"),
        QStringLiteral("Memory, chat history, Local RAG metadata, and diagnostics remain local."),
        false);
    add(QStringLiteral("workspace-active"), QStringLiteral("Workspace"),
        QStringLiteral("Workspace scope selected"),
        QStringLiteral("Workspace metadata does not grant folder scans or filesystem authority."),
        false);
    add(QStringLiteral("models-local"), QStringLiteral("Models"),
        QStringLiteral("Local provider selected"),
        QStringLiteral("Ollama can execute foreground local chat; other local endpoints require configuration."),
        false);
    add(QStringLiteral("models-role"), QStringLiteral("Models"),
        QStringLiteral("Model Role Changed"),
        QStringLiteral("Shown when the user changes local model-role metadata."), false);
    return notifications;
}

QJsonArray notificationsFromJson(const QString& json) {
    const auto document = QJsonDocument::fromJson(json.toUtf8());
    const auto stored = document.object().value(QStringLiteral("notifications")).toArray();
    return stored.isEmpty() ? defaultNotifications() : stored;
}

QString notificationsToJson(const QJsonArray& notifications) {
    QJsonObject root;
    root.insert(QStringLiteral("notifications"), notifications);
    return QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
}

QString notificationSummary(const QJsonObject& item) {
    const auto state = item.value(QStringLiteral("archived")).toBool()
                           ? QStringLiteral("Archived")
                           : item.value(QStringLiteral("read")).toBool() ? QStringLiteral("Read")
                                                                          : QStringLiteral("Unread");
    const auto pin = item.value(QStringLiteral("pinned")).toBool() ? QStringLiteral("Pinned")
                                                                   : QStringLiteral("Unpinned");
    return QStringLiteral("%1 - %2 - %3 - %4")
        .arg(item.value(QStringLiteral("category")).toString(),
             item.value(QStringLiteral("title")).toString(), state, pin);
}

bool updateNotification(const QString& json, const QString& notificationId,
                        const std::function<void(QJsonObject&)>& update, QString* updatedJson) {
    auto notifications = notificationsFromJson(json);
    bool changed = false;
    for (qsizetype i = 0; i < notifications.size(); ++i) {
        auto item = notifications.at(i).toObject();
        if (item.value(QStringLiteral("id")).toString() != notificationId) {
            continue;
        }
        update(item);
        notifications.replace(i, item);
        changed = true;
        break;
    }
    if (!changed) {
        return false;
    }
    *updatedJson = notificationsToJson(notifications);
    return true;
}

} // namespace

DesktopShellViewModel::DesktopShellViewModel(core::ApplicationController& controller,
                                             core::ModeManager& modeManager,
                                             core::AppSettings& settings, QObject* parent)
    : QObject(parent), controller_(controller), modeManager_(modeManager), settings_(settings),
      chatMessages_(this), localRagStore_(std::make_unique<core::LocalRagStore>(localRagPath())) {
    controller_.setRoutingModeByName(settings_.routingModeName());
    chatMessages_.setMessages(controller_.chatHistory());
    connect(&controller_, &core::ApplicationController::chatMessagesChanged, this, [this]() {
        chatMessages_.setMessages(controller_.chatHistory());
        emit chatMessagesChanged();
    });
    connect(&controller_, &core::ApplicationController::memoryEntriesChanged, this,
            &DesktopShellViewModel::memoryEntriesChanged);
    connect(&controller_, &core::ApplicationController::maintenanceStatusChanged, this,
            &DesktopShellViewModel::maintenanceStatusChanged);
    connect(&controller_, &core::ApplicationController::agentStatusChanged, this,
            &DesktopShellViewModel::agentStatusChanged);
    connect(&controller_, &core::ApplicationController::agentResponseChanged, this,
            &DesktopShellViewModel::agentResponseChanged);
    connect(&controller_, &core::ApplicationController::toolPlanChanged, this,
            &DesktopShellViewModel::toolPlanChanged);
    connect(&controller_, &core::ApplicationController::approvalChanged, this,
            &DesktopShellViewModel::approvalChanged);
    connect(&controller_, &core::ApplicationController::sandboxChanged, this,
            &DesktopShellViewModel::sandboxChanged);
    connect(&controller_, &core::ApplicationController::toolExecutionChanged, this,
            &DesktopShellViewModel::toolExecutionChanged);
    connect(&controller_, &core::ApplicationController::agentPipelineChanged, this,
            &DesktopShellViewModel::agentPipelineChanged);
    connect(&controller_, &core::ApplicationController::runtimeContextChanged, this,
            &DesktopShellViewModel::runtimeContextChanged);
    connect(&controller_, &core::ApplicationController::conversationSessionChanged, this,
            &DesktopShellViewModel::conversationSessionChanged);
    connect(&controller_, &core::ApplicationController::conversationStateChanged, this,
            &DesktopShellViewModel::conversationStateChanged);
    connect(&controller_, &core::ApplicationController::conversationRuntimeChanged, this,
            &DesktopShellViewModel::conversationRuntimeChanged);
    connect(&controller_, &core::ApplicationController::conversationSearchChanged, this,
            &DesktopShellViewModel::conversationSearchChanged);
    connect(&controller_, &core::ApplicationController::conversationExportChanged, this,
            &DesktopShellViewModel::conversationExportChanged);
    connect(&controller_, &core::ApplicationController::conversationDuplicateChanged, this,
            &DesktopShellViewModel::conversationDuplicateChanged);
    connect(&controller_, &core::ApplicationController::conversationDeleteChanged, this,
            &DesktopShellViewModel::conversationDeleteChanged);
    connect(&controller_, &core::ApplicationController::memoryCandidatesChanged, this,
            &DesktopShellViewModel::memoryCandidatesChanged);
    connect(&controller_, &core::ApplicationController::memoryRecallChanged, this,
            &DesktopShellViewModel::memoryRecallChanged);
    connect(&controller_, &core::ApplicationController::contextAssemblyChanged, this,
            &DesktopShellViewModel::contextAssemblyChanged);
    connect(&controller_, &core::ApplicationController::agentActivityChanged, this,
            &DesktopShellViewModel::agentActivityChanged);
    connect(&controller_, &core::ApplicationController::modelRoutingChanged, this,
            &DesktopShellViewModel::modelRoutingChanged);
    connect(&controller_, &core::ApplicationController::taskPlanChanged, this,
            &DesktopShellViewModel::taskPlanChanged);
    connect(&controller_, &core::ApplicationController::orchestrationSnapshotChanged, this,
            &DesktopShellViewModel::orchestrationSnapshotChanged);
    connect(&controller_, &core::ApplicationController::runtimeProviderRegistryChanged, this,
            &DesktopShellViewModel::runtimeProviderRegistryChanged);
    connect(&controller_, &core::ApplicationController::localModelSelectionChanged, this,
            &DesktopShellViewModel::localModelSelectionChanged);
    connect(&controller_, &core::ApplicationController::localChatInferenceRoutingChanged, this,
            &DesktopShellViewModel::localChatInferenceRoutingChanged);
    connect(&controller_, &core::ApplicationController::localInferenceChanged, this,
            &DesktopShellViewModel::localInferenceChanged);
    connect(&controller_, &core::ApplicationController::promptContextInjectionChanged, this,
            &DesktopShellViewModel::promptContextInjectionChanged);
    connect(&controller_, &core::ApplicationController::voiceConfigurationChanged, this,
            &DesktopShellViewModel::voiceConfigurationChanged);
    connect(&modeManager_, &core::ModeManager::currentModeChanged, this,
            &DesktopShellViewModel::currentModeChanged);
    connect(&settings_, &core::AppSettings::themeNameChanged, this,
            &DesktopShellViewModel::themeNameChanged);
    connect(&settings_, &core::AppSettings::configurationProfileChanged, this,
            &DesktopShellViewModel::configurationProfileChanged);
    connect(&settings_, &core::AppSettings::appLanguageChanged, this,
            &DesktopShellViewModel::appLanguageChanged);
    connect(&settings_, &core::AppSettings::companionEnabledChanged, this,
            &DesktopShellViewModel::companionChanged);
    connect(&settings_, &core::AppSettings::developerModeEnabledChanged, this,
            &DesktopShellViewModel::developerModeChanged);
    connect(&settings_, &core::AppSettings::updateCheckPolicyChanged, this,
            &DesktopShellViewModel::nativeExperienceChanged);
    connect(&settings_, &core::AppSettings::notificationPolicyChanged, this,
            &DesktopShellViewModel::nativeExperienceChanged);
    connect(&settings_, &core::AppSettings::onboardingCompleteChanged, this,
            &DesktopShellViewModel::nativeExperienceChanged);
    connect(&settings_, &core::AppSettings::onboardingUseCaseChanged, this,
            &DesktopShellViewModel::nativeExperienceChanged);
    connect(&settings_, &core::AppSettings::recoveryDraftTextChanged, this,
            &DesktopShellViewModel::nativeExperienceChanged);
    connect(&settings_, &core::AppSettings::productExperienceChanged, this,
            &DesktopShellViewModel::nativeExperienceChanged);
    connect(&settings_, &core::AppSettings::controlledAgentTasksChanged, this,
            &DesktopShellViewModel::controlledAgentTasksChanged);
    connect(&settings_, &core::AppSettings::selectedSkillProfileChanged, this,
            &DesktopShellViewModel::skillProfileChanged);
    connect(&settings_, &core::AppSettings::selectedSkillProfileChanged, this,
            &DesktopShellViewModel::agentRuntimeChanged);
    connect(&settings_, &core::AppSettings::selectedWorkspaceIdChanged, this,
            &DesktopShellViewModel::workspaceChanged);
    connect(&settings_, &core::AppSettings::workspaceSettingsChanged, this,
            &DesktopShellViewModel::workspaceChanged);
    connect(&settings_, &core::AppSettings::selectedWorkspaceIdChanged, this,
            &DesktopShellViewModel::agentRuntimeChanged);
    connect(&settings_, &core::AppSettings::defaultPermissionPolicyStateChanged, this,
            &DesktopShellViewModel::permissionPolicyChanged);
    connect(&settings_, &core::AppSettings::defaultPermissionPolicyStateChanged, this,
            &DesktopShellViewModel::agentRuntimeChanged);
    connect(&settings_, &core::AppSettings::contextExplainabilityVisibleChanged, this,
            &DesktopShellViewModel::contextExplainabilityVisibleChanged);
    connect(&settings_, &core::AppSettings::activeConversationIdChanged, this,
            &DesktopShellViewModel::chatMessagesChanged);
    connect(&settings_, &core::AppSettings::routingModeNameChanged, this,
            &DesktopShellViewModel::modelRoutingChanged);
    connect(&settings_, &core::AppSettings::selectedLocalModelChanged, this,
            [this]() {
                controller_.setSelectedLocalModel(
                    settings_.selectedModelForProvider(settings_.selectedRuntimeProvider()));
            });
    connect(&settings_, &core::AppSettings::selectedModelRoleChanged, this,
            &DesktopShellViewModel::modelRoleChanged);
    connect(&settings_, &core::AppSettings::selectedRuntimeProviderChanged, this, [this]() {
        controller_.setSelectedRuntimeProvider(settings_.selectedRuntimeProvider());
        controller_.setSelectedLocalModel(
            settings_.selectedModelForProvider(settings_.selectedRuntimeProvider()));
    });
    connect(&settings_, &core::AppSettings::localChatInferenceEnabledChanged, this, [this]() {
        controller_.setLocalChatInferenceEnabled(settings_.localChatInferenceEnabled());
    });
    connect(&settings_, &core::AppSettings::localInferenceStreamingEnabledChanged, this, [this]() {
        controller_.setLocalInferenceStreamingEnabled(settings_.localInferenceStreamingEnabled());
    });
    connect(&settings_, &core::AppSettings::localInferenceTimeoutMsChanged, this, [this]() {
        controller_.setLocalInferenceTimeoutMs(settings_.localInferenceTimeoutMs());
    });
    connect(&settings_, &core::AppSettings::promptContextInjectionEnabledChanged, this, [this]() {
        controller_.setPromptContextInjectionEnabled(settings_.promptContextInjectionEnabled());
    });
    connect(&settings_, &core::AppSettings::semanticPromptInclusionEnabledChanged, this, [this]() {
        controller_.setSemanticPromptInclusionEnabled(settings_.semanticPromptInclusionEnabled());
    });
    connect(&settings_, &core::AppSettings::piperBinaryPathChanged, this,
            [this]() { controller_.setPiperBinaryPath(settings_.piperBinaryPath()); });
    connect(&settings_, &core::AppSettings::piperModelPathChanged, this,
            [this]() { controller_.setPiperModelPath(settings_.piperModelPath()); });
    connect(&settings_, &core::AppSettings::whisperBinaryPathChanged, this,
            [this]() { controller_.setWhisperBinaryPath(settings_.whisperBinaryPath()); });
    connect(&settings_, &core::AppSettings::whisperModelPathChanged, this,
            [this]() { controller_.setWhisperModelPath(settings_.whisperModelPath()); });
    connect(&settings_, &core::AppSettings::piperFileOutputExecutionEnabledChanged, this, [this]() {
        controller_.setPiperFileOutputExecutionEnabled(settings_.piperFileOutputExecutionEnabled());
    });
    controller_.setSelectedRuntimeProvider(settings_.selectedRuntimeProvider());
    controller_.setSelectedLocalModel(
        settings_.selectedModelForProvider(settings_.selectedRuntimeProvider()));
    controller_.setLocalChatInferenceEnabled(settings_.localChatInferenceEnabled());
    controller_.setLocalInferenceStreamingEnabled(settings_.localInferenceStreamingEnabled());
    controller_.setLocalInferenceTimeoutMs(settings_.localInferenceTimeoutMs());
    controller_.setPromptContextInjectionEnabled(settings_.promptContextInjectionEnabled());
    controller_.setSemanticPromptInclusionEnabled(settings_.semanticPromptInclusionEnabled());
    controller_.setPiperBinaryPath(settings_.piperBinaryPath());
    controller_.setPiperModelPath(settings_.piperModelPath());
    controller_.setWhisperBinaryPath(settings_.whisperBinaryPath());
    controller_.setWhisperModelPath(settings_.whisperModelPath());
    if (settings_.piperFileOutputExecutionEnabled()) {
        settings_.setPiperFileOutputExecutionEnabled(false);
    }
    controller_.setPiperFileOutputExecutionEnabled(false);
    if (!settings_.activeConversationId().isEmpty()) {
        controller_.switchConversation(settings_.activeConversationId());
    }
    if (controller_.activeConversationId() != QStringLiteral("single-transcript")) {
        settings_.setActiveConversationId(controller_.activeConversationId());
    }
}

QString DesktopShellViewModel::providerName() const {
    return controller_.providerName();
}

QString DesktopShellViewModel::providerStatus() const {
    return controller_.providerStatus();
}

QString DesktopShellViewModel::agentStatus() const {
    return controller_.agentStatus();
}

QString DesktopShellViewModel::lastAgentResponse() const {
    return controller_.lastAgentResponse();
}

QString DesktopShellViewModel::latestToolPlanStatus() const {
    return controller_.latestToolPlanStatus();
}

QString DesktopShellViewModel::latestToolPlanSummary() const {
    return controller_.latestToolPlanSummary();
}

QString DesktopShellViewModel::latestApprovalStatus() const {
    return controller_.latestApprovalStatus();
}

QString DesktopShellViewModel::latestApprovalSummary() const {
    return controller_.latestApprovalSummary();
}

QString DesktopShellViewModel::latestSandboxStatus() const {
    return controller_.latestSandboxStatus();
}

QString DesktopShellViewModel::latestSandboxSummary() const {
    return controller_.latestSandboxSummary();
}

QString DesktopShellViewModel::latestToolExecutionStatus() const {
    return controller_.latestToolExecutionStatus();
}

QString DesktopShellViewModel::latestToolExecutionSummary() const {
    return controller_.latestToolExecutionSummary();
}

QString DesktopShellViewModel::latestAgentPipelineStatus() const {
    return controller_.latestAgentPipelineStatus();
}

QString DesktopShellViewModel::latestAgentPipelineSummary() const {
    return controller_.latestAgentPipelineSummary();
}

QString DesktopShellViewModel::runtimeSessionId() const {
    return controller_.runtimeSessionId();
}

QString DesktopShellViewModel::runtimeContextStatus() const {
    return controller_.runtimeContextStatus();
}

QString DesktopShellViewModel::runtimeContextSummary() const {
    return controller_.runtimeContextSummary();
}

QStringList DesktopShellViewModel::runtimeContextActiveToolIds() const {
    return controller_.runtimeContextActiveToolIds();
}

QString DesktopShellViewModel::conversationSessionId() const {
    return controller_.conversationSessionId();
}

QString DesktopShellViewModel::conversationSessionStatus() const {
    return controller_.conversationSessionStatus();
}

QString DesktopShellViewModel::interactionMode() const {
    return controller_.interactionMode();
}

QString DesktopShellViewModel::attentionState() const {
    return controller_.attentionState();
}

QString DesktopShellViewModel::contextWindowSummary() const {
    return controller_.contextWindowSummary();
}

QString DesktopShellViewModel::conversationState() const {
    return controller_.conversationState();
}

QString DesktopShellViewModel::conversationTransitionStatus() const {
    return controller_.conversationTransitionStatus();
}

QString DesktopShellViewModel::conversationTransitionSummary() const {
    return controller_.conversationTransitionSummary();
}

QString DesktopShellViewModel::conversationRuntimeSummary() const {
    return controller_.conversationRuntimeSummary();
}

QStringList DesktopShellViewModel::conversationRuntimeSummaryLines() const {
    return controller_.conversationRuntimeSummaryLines();
}

QString DesktopShellViewModel::conversationRuntimeRequestId() const {
    return controller_.conversationRuntimeRequestId();
}

QString DesktopShellViewModel::conversationRuntimeActiveModel() const {
    return controller_.conversationRuntimeActiveModel();
}

QString DesktopShellViewModel::conversationRuntimeActiveRoute() const {
    return controller_.conversationRuntimeActiveRoute();
}

bool DesktopShellViewModel::conversationRuntimeStreaming() const {
    return controller_.conversationRuntimeStreaming();
}

QString DesktopShellViewModel::conversationRuntimeLastSuccessSummary() const {
    return controller_.conversationRuntimeLastSuccessSummary();
}

QString DesktopShellViewModel::conversationRuntimeLastErrorSummary() const {
    return controller_.conversationRuntimeLastErrorSummary();
}

QString DesktopShellViewModel::conversationRuntimeLastLatencySummary() const {
    return controller_.conversationRuntimeLastLatencySummary();
}

int DesktopShellViewModel::agentActivityCount() const {
    return controller_.agentActivityCount();
}

QString DesktopShellViewModel::latestAgentActivitySummary() const {
    return controller_.latestAgentActivitySummary();
}

QString DesktopShellViewModel::currentRoutingMode() const {
    return controller_.currentRoutingMode();
}

void DesktopShellViewModel::setRoutingModeByName(const QString& routingModeName) {
    const auto before = controller_.currentRoutingMode();
    settings_.setRoutingModeName(routingModeName);
    controller_.setRoutingModeByName(settings_.routingModeName());
    if (before == controller_.currentRoutingMode()) {
        emit modelRoutingChanged();
        emit conversationSessionChanged();
        emit orchestrationSnapshotChanged();
    }
}

QStringList DesktopShellViewModel::availableRoutingModes() const {
    return settings_.availableRoutingModes();
}

QString DesktopShellViewModel::modelRoutingStatus() const {
    return controller_.modelRoutingStatus();
}

QString DesktopShellViewModel::selectedModelProviderSummary() const {
    return controller_.selectedModelProviderSummary();
}

QString DesktopShellViewModel::latestTaskPlanStatus() const {
    return controller_.latestTaskPlanStatus();
}

QString DesktopShellViewModel::latestTaskPlanSummary() const {
    return controller_.latestTaskPlanSummary();
}

int DesktopShellViewModel::plannedTaskStepCount() const {
    return controller_.plannedTaskStepCount();
}

int DesktopShellViewModel::registeredAgentCount() const {
    return controller_.registeredAgentCount();
}

QStringList DesktopShellViewModel::activeAgentSummaries() const {
    return controller_.activeAgentSummaries();
}

QString DesktopShellViewModel::currentAgentSummary() const {
    return controller_.currentAgentSummary();
}

QString DesktopShellViewModel::currentMemoryAffinitySummary() const {
    return controller_.currentMemoryAffinitySummary();
}

int DesktopShellViewModel::providerCatalogCount() const {
    return controller_.providerCatalogCount();
}

QStringList DesktopShellViewModel::providerCatalogSummaries() const {
    return controller_.providerCatalogSummaries();
}

int DesktopShellViewModel::memoryCatalogCount() const {
    return controller_.memoryCatalogCount();
}

QStringList DesktopShellViewModel::memoryCatalogSummaries() const {
    return controller_.memoryCatalogSummaries();
}

QString DesktopShellViewModel::orchestrationSnapshotStatus() const {
    return controller_.orchestrationSnapshotStatus();
}

QString DesktopShellViewModel::orchestrationSnapshotSummary() const {
    return controller_.orchestrationSnapshotSummary();
}

QStringList DesktopShellViewModel::orchestrationSignals() const {
    return controller_.orchestrationSignals();
}

QString DesktopShellViewModel::orchestrationReadinessStatus() const {
    return controller_.orchestrationReadinessStatus();
}

QString DesktopShellViewModel::orchestrationReadinessSummary() const {
    return controller_.orchestrationReadinessSummary();
}

QStringList DesktopShellViewModel::orchestrationDiagnostics() const {
    return controller_.orchestrationDiagnostics();
}

QString DesktopShellViewModel::agentTaskRuntimeStatus() const {
    return controller_.agentTaskRuntimeStatus();
}

QString DesktopShellViewModel::agentTaskRuntimeSummary() const {
    return controller_.agentTaskRuntimeSummary();
}

int DesktopShellViewModel::agentTaskRuntimeTaskCount() const {
    return controller_.agentTaskRuntimeTaskCount();
}

int DesktopShellViewModel::agentTaskQueueCount() const {
    return controller_.agentTaskQueueCount();
}

int DesktopShellViewModel::agentTaskQueueActiveCount() const {
    return controller_.agentTaskQueueActiveCount();
}

int DesktopShellViewModel::agentTaskQueuePlannedCount() const {
    return controller_.agentTaskQueuePlannedCount();
}

int DesktopShellViewModel::agentTaskQueueBlockedCount() const {
    return controller_.agentTaskQueueBlockedCount();
}

int DesktopShellViewModel::agentTaskQueueCompletedCount() const {
    return controller_.agentTaskQueueCompletedCount();
}

int DesktopShellViewModel::agentTaskQueueRefusedCount() const {
    return controller_.agentTaskQueueRefusedCount();
}

QString DesktopShellViewModel::latestAgentTaskSummary() const {
    return controller_.latestAgentTaskSummary();
}

QString DesktopShellViewModel::latestAgentTaskLifecycleSummary() const {
    return controller_.latestAgentTaskLifecycleSummary();
}

QStringList DesktopShellViewModel::agentTaskQueueSummaries() const {
    return controller_.agentTaskQueueSummaries();
}

QStringList DesktopShellViewModel::agentTaskTraceSummaries() const {
    return controller_.agentTaskTraceSummaries();
}

QString DesktopShellViewModel::agentPlanningSessionStatus() const {
    return controller_.agentPlanningSessionStatus();
}

QString DesktopShellViewModel::agentPlanningSessionSummary() const {
    return controller_.agentPlanningSessionSummary();
}

int DesktopShellViewModel::agentPlanningCandidateCount() const {
    return controller_.agentPlanningCandidateCount();
}

int DesktopShellViewModel::agentPlanningRefusedCount() const {
    return controller_.agentPlanningRefusedCount();
}

QStringList DesktopShellViewModel::agentPlanningCandidateSummaries() const {
    return controller_.agentPlanningCandidateSummaries();
}

QStringList DesktopShellViewModel::agentPlanningArbitrationSummaries() const {
    return controller_.agentPlanningArbitrationSummaries();
}

QStringList DesktopShellViewModel::agentPlanningRefusalSummaries() const {
    return controller_.agentPlanningRefusalSummaries();
}

QString DesktopShellViewModel::agentPlanningFallbackSummary() const {
    return controller_.agentPlanningFallbackSummary();
}

QString DesktopShellViewModel::agentCapabilityRegistryStatus() const {
    return controller_.agentCapabilityRegistryStatus();
}

QString DesktopShellViewModel::agentCapabilityRegistrySummary() const {
    return controller_.agentCapabilityRegistrySummary();
}

int DesktopShellViewModel::agentCapabilityCount() const {
    return controller_.agentCapabilityCount();
}

int DesktopShellViewModel::agentCapabilityEnabledCount() const {
    return controller_.agentCapabilityEnabledCount();
}

int DesktopShellViewModel::agentCapabilityDisabledCount() const {
    return controller_.agentCapabilityDisabledCount();
}

int DesktopShellViewModel::agentCapabilityRestrictedCount() const {
    return controller_.agentCapabilityRestrictedCount();
}

QStringList DesktopShellViewModel::agentCapabilitySummaries() const {
    return controller_.agentCapabilitySummaries();
}

QStringList DesktopShellViewModel::agentCapabilityReadinessSummaries() const {
    return controller_.agentCapabilityReadinessSummaries();
}

QStringList DesktopShellViewModel::agentCapabilitySafetySummaries() const {
    return controller_.agentCapabilitySafetySummaries();
}

QString DesktopShellViewModel::toolContractRegistryStatus() const {
    return controller_.toolContractRegistryStatus();
}

QString DesktopShellViewModel::toolContractRegistrySummary() const {
    return controller_.toolContractRegistrySummary();
}

int DesktopShellViewModel::toolContractCount() const {
    return controller_.toolContractCount();
}

int DesktopShellViewModel::toolContractEnabledCount() const {
    return controller_.toolContractEnabledCount();
}

int DesktopShellViewModel::toolContractDisabledCount() const {
    return controller_.toolContractDisabledCount();
}

int DesktopShellViewModel::toolContractRestrictedCount() const {
    return controller_.toolContractRestrictedCount();
}

QStringList DesktopShellViewModel::toolContractSummaries() const {
    return controller_.toolContractSummaries();
}

QStringList DesktopShellViewModel::toolContractPermissionSummaries() const {
    return controller_.toolContractPermissionSummaries();
}

QStringList DesktopShellViewModel::toolContractSandboxSummaries() const {
    return controller_.toolContractSandboxSummaries();
}

QStringList DesktopShellViewModel::toolContractReadinessSummaries() const {
    return controller_.toolContractReadinessSummaries();
}

QStringList DesktopShellViewModel::toolContractSafetySummaries() const {
    return controller_.toolContractSafetySummaries();
}

QString DesktopShellViewModel::localRuntimeStatus() const {
    return controller_.localRuntimeStatus();
}

QString DesktopShellViewModel::localRuntimeHealth() const {
    return controller_.localRuntimeHealth();
}

QString DesktopShellViewModel::localRuntimeSummary() const {
    return controller_.localRuntimeSummary();
}

QStringList DesktopShellViewModel::localRuntimeCapabilities() const {
    return controller_.localRuntimeCapabilities();
}

QString DesktopShellViewModel::localRuntimeResponseStatus() const {
    return controller_.localRuntimeResponseStatus();
}

QString DesktopShellViewModel::localRuntimeResponseSummary() const {
    return controller_.localRuntimeResponseSummary();
}

int DesktopShellViewModel::localRuntimeSessionCount() const {
    return controller_.localRuntimeSessionCount();
}

QString DesktopShellViewModel::localRuntimeSessionStatus() const {
    return controller_.localRuntimeSessionStatus();
}

QString DesktopShellViewModel::localRuntimeSessionHealth() const {
    return controller_.localRuntimeSessionHealth();
}

QString DesktopShellViewModel::localRuntimeSessionSummary() const {
    return controller_.localRuntimeSessionSummary();
}

QString DesktopShellViewModel::localRuntimeAllocationSummary() const {
    return controller_.localRuntimeAllocationSummary();
}

QString DesktopShellViewModel::localRuntimeReservationSummary() const {
    return controller_.localRuntimeReservationSummary();
}

QStringList DesktopShellViewModel::localRuntimeSessionSummaries() const {
    return controller_.localRuntimeSessionSummaries();
}

int DesktopShellViewModel::runtimeCapabilityCount() const {
    return controller_.runtimeCapabilityCount();
}

QStringList DesktopShellViewModel::enabledRuntimeCapabilitySummaries() const {
    return controller_.enabledRuntimeCapabilitySummaries();
}

QStringList DesktopShellViewModel::disabledRuntimeCapabilitySummaries() const {
    return controller_.disabledRuntimeCapabilitySummaries();
}

QString DesktopShellViewModel::runtimeNegotiationProfileSummary() const {
    return controller_.runtimeNegotiationProfileSummary();
}

QString DesktopShellViewModel::runtimeNegotiationSummary() const {
    return controller_.runtimeNegotiationSummary();
}

QString DesktopShellViewModel::localOnlyRuntimeEnforcementSummary() const {
    return controller_.localOnlyRuntimeEnforcementSummary();
}

QString DesktopShellViewModel::runtimePermissionDecision() const {
    return controller_.runtimePermissionDecision();
}

QString DesktopShellViewModel::runtimePermissionSummary() const {
    return controller_.runtimePermissionSummary();
}

QString DesktopShellViewModel::runtimeSafetyDecision() const {
    return controller_.runtimeSafetyDecision();
}

QString DesktopShellViewModel::runtimeSafetySummary() const {
    return controller_.runtimeSafetySummary();
}

QString DesktopShellViewModel::runtimePipelineStatus() const {
    return controller_.runtimePipelineStatus();
}

QString DesktopShellViewModel::runtimePipelineSummary() const {
    return controller_.runtimePipelineSummary();
}

QStringList DesktopShellViewModel::runtimePipelineTraceSummaries() const {
    return controller_.runtimePipelineTraceSummaries();
}

QString DesktopShellViewModel::executionLifecycleState() const {
    return controller_.executionLifecycleState();
}

QString DesktopShellViewModel::executionLifecycleStatus() const {
    return controller_.executionLifecycleStatus();
}

QString DesktopShellViewModel::executionLifecycleSummary() const {
    return controller_.executionLifecycleSummary();
}

QStringList DesktopShellViewModel::executionLifecycleTraceSummaries() const {
    return controller_.executionLifecycleTraceSummaries();
}

QString DesktopShellViewModel::executionSessionId() const {
    return controller_.executionSessionId();
}

QString DesktopShellViewModel::executionSessionStatus() const {
    return controller_.executionSessionStatus();
}

QString DesktopShellViewModel::executionSessionOwnership() const {
    return controller_.executionSessionOwnership();
}

QString DesktopShellViewModel::executionCoordinationMode() const {
    return controller_.executionCoordinationMode();
}

QString DesktopShellViewModel::executionSessionSummary() const {
    return controller_.executionSessionSummary();
}

QString DesktopShellViewModel::executionCoordinationSnapshotSummary() const {
    return controller_.executionCoordinationSnapshotSummary();
}

QString DesktopShellViewModel::localRuntimeAdapterStatus() const {
    return controller_.localRuntimeAdapterStatus();
}

QString DesktopShellViewModel::localRuntimeAdapterHealth() const {
    return controller_.localRuntimeAdapterHealth();
}

QString DesktopShellViewModel::localRuntimeAdapterSummary() const {
    return controller_.localRuntimeAdapterSummary();
}

QStringList DesktopShellViewModel::localRuntimeAdapterCapabilitySummaries() const {
    return controller_.localRuntimeAdapterCapabilitySummaries();
}

QString DesktopShellViewModel::providerRuntimeBridgeStatus() const {
    return controller_.providerRuntimeBridgeStatus();
}

QString DesktopShellViewModel::providerRuntimeBridgeSummary() const {
    return controller_.providerRuntimeBridgeSummary();
}

QString DesktopShellViewModel::providerRuntimeBridgeResponseSummary() const {
    return controller_.providerRuntimeBridgeResponseSummary();
}

QString DesktopShellViewModel::runtimeIntegrationReadinessStatus() const {
    return controller_.runtimeIntegrationReadinessStatus();
}

QString DesktopShellViewModel::runtimeIntegrationReadinessSummary() const {
    return controller_.runtimeIntegrationReadinessSummary();
}

QStringList DesktopShellViewModel::runtimeIntegrationReadinessChecks() const {
    return controller_.runtimeIntegrationReadinessChecks();
}

QString DesktopShellViewModel::selectedRuntimeProvider() const {
    return settings_.selectedRuntimeProvider();
}

void DesktopShellViewModel::setSelectedRuntimeProvider(const QString& providerId) {
    settings_.setSelectedRuntimeProvider(providerId);
    if (controller_.selectedRuntimeProvider() != settings_.selectedRuntimeProvider()) {
        controller_.setSelectedRuntimeProvider(settings_.selectedRuntimeProvider());
    }
    const auto providerModel = settings_.selectedModelForProvider(settings_.selectedRuntimeProvider());
    if (controller_.selectedLocalModel() != providerModel) {
        controller_.setSelectedLocalModel(providerModel);
    }
}

QString DesktopShellViewModel::activeRuntimeProviderId() const {
    return controller_.activeRuntimeProviderId();
}

QString DesktopShellViewModel::activeRuntimeProviderLabel() const {
    return controller_.activeRuntimeProviderLabel();
}

QString DesktopShellViewModel::activeRuntimeModelLabel() const {
    return controller_.activeRuntimeModelLabel();
}

QString DesktopShellViewModel::activeRuntimeReadinessState() const {
    return controller_.activeRuntimeReadinessState();
}

QString DesktopShellViewModel::activeRuntimeReadinessSummary() const {
    return controller_.activeRuntimeReadinessSummary();
}

QString DesktopShellViewModel::activeRuntimeLocalOnlySummary() const {
    return controller_.activeRuntimeLocalOnlySummary();
}

QStringList DesktopShellViewModel::selectableRuntimeProviderIds() const {
    return controller_.selectableRuntimeProviderIds();
}

QStringList DesktopShellViewModel::selectableRuntimeProviderLabels() const {
    return controller_.selectableRuntimeProviderLabels();
}

QStringList DesktopShellViewModel::runtimeProviderCardSummaries() const {
    return controller_.runtimeProviderCardSummaries();
}

QStringList DesktopShellViewModel::runtimeProviderCapabilitySummaries() const {
    return controller_.runtimeProviderCapabilitySummaries();
}

QStringList DesktopShellViewModel::runtimeProviderValidationTraces() const {
    return controller_.runtimeProviderValidationTraces();
}

QStringList DesktopShellViewModel::installedRuntimeProviderSummaries() const {
    return controller_.installedRuntimeProviderSummaries();
}

QStringList DesktopShellViewModel::configuredRuntimeProviderSummaries() const {
    return controller_.configuredRuntimeProviderSummaries();
}

QStringList DesktopShellViewModel::availableLocalRuntimeSummaries() const {
    return controller_.availableLocalRuntimeSummaries();
}

QString DesktopShellViewModel::providerCredentialRegistryStatus() const {
    return controller_.providerCredentialRegistryStatus();
}

QString DesktopShellViewModel::providerCredentialRegistrySummary() const {
    return controller_.providerCredentialRegistrySummary();
}

QStringList DesktopShellViewModel::providerCredentialSummaries() const {
    return controller_.providerCredentialSummaries();
}

QStringList DesktopShellViewModel::providerCredentialReadinessSummaries() const {
    return controller_.providerCredentialReadinessSummaries();
}

QStringList DesktopShellViewModel::providerCredentialSafetySummaries() const {
    return controller_.providerCredentialSafetySummaries();
}

QString DesktopShellViewModel::credentialStoreSummary() const {
    return controller_.credentialStoreSummary();
}

QString DesktopShellViewModel::credentialStoreBackendSummary() const {
    return controller_.credentialStoreBackendSummary();
}

QString DesktopShellViewModel::credentialStoreSafetySummary() const {
    return controller_.credentialStoreSafetySummary();
}

QStringList DesktopShellViewModel::credentialStoreTraceSummaries() const {
    return controller_.credentialStoreTraceSummaries();
}

QString DesktopShellViewModel::credentialActionReadiness() const {
    return controller_.credentialActionReadiness();
}

QString DesktopShellViewModel::credentialExecutionStatus() const {
    return controller_.credentialExecutionStatus();
}

QString DesktopShellViewModel::ollamaEndpoint() const {
    return controller_.ollamaEndpoint();
}

QString DesktopShellViewModel::ollamaConnectionStatus() const {
    return controller_.ollamaConnectionStatus();
}

QString DesktopShellViewModel::ollamaHealthStatus() const {
    return controller_.ollamaHealthStatus();
}

QString DesktopShellViewModel::ollamaHealthSummary() const {
    return controller_.ollamaHealthSummary();
}

int DesktopShellViewModel::ollamaModelCount() const {
    return controller_.ollamaModelCount();
}

QStringList DesktopShellViewModel::ollamaModelNames() const {
    return controller_.ollamaModelNames();
}

QStringList DesktopShellViewModel::ollamaModelSummaries() const {
    return controller_.ollamaModelSummaries();
}

QString DesktopShellViewModel::selectedLocalModel() const {
    return controller_.selectedLocalModel();
}

void DesktopShellViewModel::setSelectedLocalModel(const QString& model) {
    settings_.setSelectedModelForProvider(settings_.selectedRuntimeProvider(), model);
    const auto providerModel = settings_.selectedModelForProvider(settings_.selectedRuntimeProvider());
    if (controller_.selectedLocalModel() != providerModel) {
        controller_.setSelectedLocalModel(providerModel);
    }
}

QString DesktopShellViewModel::selectedLocalModelStatus() const {
    return controller_.selectedLocalModelStatus();
}

QString DesktopShellViewModel::selectedLocalModelSummary() const {
    return controller_.selectedLocalModelSummary();
}

QString DesktopShellViewModel::selectedLocalModelMetadataSummary() const {
    return controller_.selectedLocalModelMetadataSummary();
}

QString DesktopShellViewModel::activeLocalRuntimeBadge() const {
    return controller_.activeLocalRuntimeBadge();
}

QString DesktopShellViewModel::modelRegistryStatus() const {
    return controller_.modelRegistryStatus();
}

QString DesktopShellViewModel::modelRegistrySummary() const {
    return controller_.modelRegistrySummary();
}

QStringList DesktopShellViewModel::modelRegistryModelSummaries() const {
    return controller_.modelRegistryModelSummaries();
}

QStringList DesktopShellViewModel::modelLibraryInstalledSummaries() const {
    return controller_.modelLibraryInstalledSummaries();
}

QStringList DesktopShellViewModel::modelLibraryAvailableSummaries() const {
    return controller_.modelLibraryAvailableSummaries();
}

QStringList DesktopShellViewModel::modelLibraryRecommendedSummaries() const {
    return controller_.modelLibraryRecommendedSummaries();
}

QStringList DesktopShellViewModel::modelLibraryDetailSummaries() const {
    return controller_.modelLibraryDetailSummaries();
}

QStringList DesktopShellViewModel::providerDiscoverySummaries() const {
    return controller_.providerDiscoverySummaries();
}

QStringList DesktopShellViewModel::modelRoleIds() const {
    return core::modelRoleIds();
}

QStringList DesktopShellViewModel::modelRoleAssignmentSummaries() const {
    QStringList summaries;
    const QList<core::ModelRole> roles{
        core::ModelRole::PrimaryChat, core::ModelRole::Coding, core::ModelRole::Summarizer,
        core::ModelRole::Research,    core::ModelRole::Fast,   core::ModelRole::Voice,
        core::ModelRole::Embedding,
    };
    for (const auto role : roles) {
        const auto roleId = core::modelRoleId(role);
        const auto selected = settings_.selectedModelForRole(roleId);
        summaries.append(QStringLiteral("%1 - %2 - routing metadata only; automatic multi-model "
                                        "execution is disabled.")
                             .arg(core::modelRoleDisplayName(role),
                                  selected.isEmpty() ? QStringLiteral("No model assigned")
                                                     : selected));
    }
    return summaries;
}

void DesktopShellViewModel::assignModelRole(const QString& roleId, const QString& modelId) {
    settings_.setSelectedModelForRole(roleId, modelId);
}

QStringList DesktopShellViewModel::modelAdvisorRecommendationSummaries() const {
    return controller_.modelAdvisorRecommendationSummaries();
}

QStringList DesktopShellViewModel::modelAdvisorAvoidSummaries() const {
    return controller_.modelAdvisorAvoidSummaries();
}

QStringList DesktopShellViewModel::downloadsCenterSummaries() const {
    return controller_.downloadsCenterSummaries();
}

QStringList DesktopShellViewModel::benchmarkHubSummaries() const {
    return controller_.benchmarkHubSummaries();
}

QStringList DesktopShellViewModel::selectedModelCapabilityLabels() const {
    return controller_.selectedModelCapabilityLabels();
}

QString DesktopShellViewModel::modelManagementStatus() const {
    return controller_.modelManagementStatus();
}

QString DesktopShellViewModel::modelManagementSummary() const {
    return controller_.modelManagementSummary();
}

QString DesktopShellViewModel::modelManagementActionAvailability() const {
    return controller_.modelManagementActionAvailability();
}

QStringList DesktopShellViewModel::modelRecommendationSummaries() const {
    return controller_.modelRecommendationSummaries();
}

QStringList DesktopShellViewModel::modelRequirementSummaries() const {
    return controller_.modelRequirementSummaries();
}

QString DesktopShellViewModel::voiceRuntimeMode() const {
    return controller_.voiceRuntimeMode();
}

bool DesktopShellViewModel::voiceEnabled() const {
    return controller_.voiceEnabled();
}

QString DesktopShellViewModel::voiceReadinessStatus() const {
    return controller_.voiceReadinessStatus();
}

QString DesktopShellViewModel::voiceReadinessSummary() const {
    return controller_.voiceReadinessSummary();
}

QStringList DesktopShellViewModel::voiceReadinessChecks() const {
    return controller_.voiceReadinessChecks();
}

QStringList DesktopShellViewModel::voiceCapabilitySummaries() const {
    return controller_.voiceCapabilitySummaries();
}

QString DesktopShellViewModel::textToSpeechStatus() const {
    return controller_.textToSpeechStatus();
}

QString DesktopShellViewModel::textToSpeechSummary() const {
    return controller_.textToSpeechSummary();
}

QString DesktopShellViewModel::speechToTextStatus() const {
    return controller_.speechToTextStatus();
}

QString DesktopShellViewModel::speechToTextSummary() const {
    return controller_.speechToTextSummary();
}

QString DesktopShellViewModel::voiceSessionId() const {
    return controller_.voiceSessionId();
}

QString DesktopShellViewModel::voiceSessionStatus() const {
    return controller_.voiceSessionStatus();
}

QString DesktopShellViewModel::voiceSessionSummary() const {
    return controller_.voiceSessionSummary();
}

QString DesktopShellViewModel::voicePipelineStatus() const {
    return controller_.voicePipelineStatus();
}

QString DesktopShellViewModel::voicePipelineSummary() const {
    return controller_.voicePipelineSummary();
}

QStringList DesktopShellViewModel::voicePipelineTraceSummaries() const {
    return controller_.voicePipelineTraceSummaries();
}

QString DesktopShellViewModel::voicePipelineSessionStatus() const {
    return controller_.voicePipelineSessionStatus();
}

QString DesktopShellViewModel::voicePipelineSessionSummary() const {
    return controller_.voicePipelineSessionSummary();
}

QStringList DesktopShellViewModel::voicePipelineSessionStageReadinessSummaries() const {
    return controller_.voicePipelineSessionStageReadinessSummaries();
}

QStringList DesktopShellViewModel::voicePipelineSessionTraceSummaries() const {
    return controller_.voicePipelineSessionTraceSummaries();
}

QString DesktopShellViewModel::voicePipelineSessionFallbackSummary() const {
    return controller_.voicePipelineSessionFallbackSummary();
}

QString DesktopShellViewModel::voicePipelineSessionSafetySummary() const {
    return controller_.voicePipelineSessionSafetySummary();
}

QStringList DesktopShellViewModel::voicePipelineSessionSafetyChecks() const {
    return controller_.voicePipelineSessionSafetyChecks();
}

int DesktopShellViewModel::voicePipelineSessionReadyStageCount() const {
    return controller_.voicePipelineSessionReadyStageCount();
}

int DesktopShellViewModel::voicePipelineSessionBlockedStageCount() const {
    return controller_.voicePipelineSessionBlockedStageCount();
}

QString DesktopShellViewModel::audioFileSessionStatus() const {
    return controller_.audioFileSessionStatus();
}

QString DesktopShellViewModel::audioFileSessionSummary() const {
    return controller_.audioFileSessionSummary();
}

QString DesktopShellViewModel::audioFileSessionReadinessSummary() const {
    return controller_.audioFileSessionReadinessSummary();
}

QStringList DesktopShellViewModel::audioFileValidationSummaries() const {
    return controller_.audioFileValidationSummaries();
}

QStringList DesktopShellViewModel::audioFileSupportedExtensionSummaries() const {
    return controller_.audioFileSupportedExtensionSummaries();
}

QString DesktopShellViewModel::audioFileSessionFallbackSummary() const {
    return controller_.audioFileSessionFallbackSummary();
}

QString DesktopShellViewModel::audioFileSessionSafetySummary() const {
    return controller_.audioFileSessionSafetySummary();
}

QStringList DesktopShellViewModel::audioFileSessionSafetyChecks() const {
    return controller_.audioFileSessionSafetyChecks();
}

QStringList DesktopShellViewModel::audioFileSessionRefusalSummaries() const {
    return controller_.audioFileSessionRefusalSummaries();
}

QStringList DesktopShellViewModel::audioFileTraceSummaries() const {
    return controller_.audioFileTraceSummaries();
}

QString DesktopShellViewModel::voiceRuntimeStatus() const {
    return controller_.voiceRuntimeStatus();
}

QString DesktopShellViewModel::voiceRuntimeSummary() const {
    return controller_.voiceRuntimeSummary();
}

QStringList DesktopShellViewModel::voiceRuntimeCheckSummaries() const {
    return controller_.voiceRuntimeCheckSummaries();
}

bool DesktopShellViewModel::voiceRuntimeAvailable() const {
    return controller_.voiceRuntimeAvailable();
}

bool DesktopShellViewModel::voiceTextToSpeechAvailable() const {
    return controller_.voiceTextToSpeechAvailable();
}

bool DesktopShellViewModel::voiceSpeechToTextAvailable() const {
    return controller_.voiceSpeechToTextAvailable();
}

bool DesktopShellViewModel::voiceMicrophoneEnabled() const {
    return controller_.voiceMicrophoneEnabled();
}

bool DesktopShellViewModel::voicePlaybackEnabled() const {
    return controller_.voicePlaybackEnabled();
}

bool DesktopShellViewModel::voiceLocalOnlyPolicy() const {
    return controller_.voiceLocalOnlyPolicy();
}

bool DesktopShellViewModel::voiceProcessExecutionEnabled() const {
    return controller_.voiceProcessExecutionEnabled();
}

QString DesktopShellViewModel::voiceRuntimeEnvironmentStatus() const {
    return controller_.voiceRuntimeEnvironmentStatus();
}

QString DesktopShellViewModel::voiceRuntimeEnvironmentSummary() const {
    return controller_.voiceRuntimeEnvironmentSummary();
}

QStringList DesktopShellViewModel::voiceBinarySummaries() const {
    return controller_.voiceBinarySummaries();
}

QStringList DesktopShellViewModel::voiceModelSummaries() const {
    return controller_.voiceModelSummaries();
}

QStringList DesktopShellViewModel::voiceRuntimePermissionSummaries() const {
    return controller_.voiceRuntimePermissionSummaries();
}

QString DesktopShellViewModel::voiceRuntimeSafetyStatus() const {
    return controller_.voiceRuntimeSafetyStatus();
}

QString DesktopShellViewModel::voiceRuntimeSafetySummary() const {
    return controller_.voiceRuntimeSafetySummary();
}

QStringList DesktopShellViewModel::voiceRuntimeSafetyChecks() const {
    return controller_.voiceRuntimeSafetyChecks();
}

bool DesktopShellViewModel::voiceRuntimeExecutionAllowed() const {
    return controller_.voiceRuntimeExecutionAllowed();
}

QString DesktopShellViewModel::piperTtsStatus() const {
    return controller_.piperTtsStatus();
}

QString DesktopShellViewModel::piperTtsSummary() const {
    return controller_.piperTtsSummary();
}

QStringList DesktopShellViewModel::piperTtsReadinessChecks() const {
    return controller_.piperTtsReadinessChecks();
}

bool DesktopShellViewModel::piperTtsReady() const {
    return controller_.piperTtsReady();
}

QString DesktopShellViewModel::piperTtsFileOutputStatus() const {
    return controller_.piperTtsFileOutputStatus();
}

QString DesktopShellViewModel::piperTtsFileOutputSummary() const {
    return controller_.piperTtsFileOutputSummary();
}

QString DesktopShellViewModel::piperSynthesisStatus() const {
    return controller_.piperSynthesisStatus();
}

QString DesktopShellViewModel::piperSynthesisReadinessSummary() const {
    return controller_.piperSynthesisReadinessSummary();
}

QString DesktopShellViewModel::piperSynthesisLastSummary() const {
    return controller_.piperSynthesisLastSummary();
}

QString DesktopShellViewModel::piperSynthesisFallbackSummary() const {
    return controller_.piperSynthesisFallbackSummary();
}

QString DesktopShellViewModel::piperSynthesisSafetySummary() const {
    return controller_.piperSynthesisSafetySummary();
}

QStringList DesktopShellViewModel::piperSynthesisTraceSummaries() const {
    return controller_.piperSynthesisTraceSummaries();
}

QString DesktopShellViewModel::piperBinaryPath() const {
    return controller_.piperBinaryPath();
}

void DesktopShellViewModel::setPiperBinaryPath(const QString& path) {
    settings_.setPiperBinaryPath(path);
    if (controller_.piperBinaryPath() != settings_.piperBinaryPath()) {
        controller_.setPiperBinaryPath(settings_.piperBinaryPath());
    }
}

QString DesktopShellViewModel::piperModelPath() const {
    return controller_.piperModelPath();
}

void DesktopShellViewModel::setPiperModelPath(const QString& path) {
    settings_.setPiperModelPath(path);
    if (controller_.piperModelPath() != settings_.piperModelPath()) {
        controller_.setPiperModelPath(settings_.piperModelPath());
    }
}

QString DesktopShellViewModel::whisperBinaryPath() const {
    return controller_.whisperBinaryPath();
}

void DesktopShellViewModel::setWhisperBinaryPath(const QString& path) {
    settings_.setWhisperBinaryPath(path);
    if (controller_.whisperBinaryPath() != settings_.whisperBinaryPath()) {
        controller_.setWhisperBinaryPath(settings_.whisperBinaryPath());
    }
}

QString DesktopShellViewModel::whisperModelPath() const {
    return controller_.whisperModelPath();
}

void DesktopShellViewModel::setWhisperModelPath(const QString& path) {
    settings_.setWhisperModelPath(path);
    if (controller_.whisperModelPath() != settings_.whisperModelPath()) {
        controller_.setWhisperModelPath(settings_.whisperModelPath());
    }
}

QStringList DesktopShellViewModel::voiceConfigurationSummaries() const {
    return controller_.voiceConfigurationSummaries();
}

QString DesktopShellViewModel::voiceConfigurationReadinessSummary() const {
    return controller_.voiceConfigurationReadinessSummary();
}

QStringList DesktopShellViewModel::voiceConfigurationStatusBadges() const {
    return controller_.voiceConfigurationStatusBadges();
}

QStringList DesktopShellViewModel::voiceConfigurationHintSummaries() const {
    return controller_.voiceConfigurationHintSummaries();
}

QStringList DesktopShellViewModel::voiceConfigurationValidationSummaries() const {
    return controller_.voiceConfigurationValidationSummaries();
}

QString DesktopShellViewModel::piperFileOutputReadinessStatus() const {
    return controller_.piperFileOutputReadinessStatus();
}

QString DesktopShellViewModel::piperFileOutputReadinessSummary() const {
    return controller_.piperFileOutputReadinessSummary();
}

bool DesktopShellViewModel::piperFileOutputExecutionEnabled() const {
    return controller_.piperFileOutputExecutionEnabled();
}

void DesktopShellViewModel::setPiperFileOutputExecutionEnabled(bool enabled) {
    Q_UNUSED(enabled);
    settings_.setPiperFileOutputExecutionEnabled(false);
    if (controller_.piperFileOutputExecutionEnabled() !=
        settings_.piperFileOutputExecutionEnabled()) {
        controller_.setPiperFileOutputExecutionEnabled(settings_.piperFileOutputExecutionEnabled());
    }
}

QString DesktopShellViewModel::piperFileOutputExecutionStatus() const {
    return controller_.piperFileOutputExecutionStatus();
}

QString DesktopShellViewModel::piperFileOutputExecutionSummary() const {
    return controller_.piperFileOutputExecutionSummary();
}

QString DesktopShellViewModel::piperFileOutputAudioPathSummary() const {
    return controller_.piperFileOutputAudioPathSummary();
}

QString DesktopShellViewModel::whisperPreparationReadinessStatus() const {
    return controller_.whisperPreparationReadinessStatus();
}

QString DesktopShellViewModel::whisperPreparationReadinessSummary() const {
    return controller_.whisperPreparationReadinessSummary();
}

QString DesktopShellViewModel::voiceRuntimeReadinessSummary() const {
    return controller_.voiceRuntimeReadinessSummary();
}

QString DesktopShellViewModel::voiceRuntimeHealth() const {
    return controller_.voiceRuntimeHealth();
}

int DesktopShellViewModel::voiceRuntimeConfiguredCount() const {
    return controller_.voiceRuntimeConfiguredCount();
}

int DesktopShellViewModel::voiceRuntimeMissingCount() const {
    return controller_.voiceRuntimeMissingCount();
}

int DesktopShellViewModel::voiceRuntimeRefusedCount() const {
    return controller_.voiceRuntimeRefusedCount();
}

QString DesktopShellViewModel::voiceRuntimePermissionFoundationSummary() const {
    return controller_.voiceRuntimePermissionFoundationSummary();
}

QString DesktopShellViewModel::voiceRuntimeSandboxSummary() const {
    return controller_.voiceRuntimeSandboxSummary();
}

QString DesktopShellViewModel::voiceRuntimeSafetyReportSummary() const {
    return controller_.voiceRuntimeSafetyReportSummary();
}

QStringList DesktopShellViewModel::voiceRuntimeReadinessChecks() const {
    return controller_.voiceRuntimeReadinessChecks();
}

QString DesktopShellViewModel::whisperRuntimeStatus() const {
    return controller_.whisperRuntimeStatus();
}

QString DesktopShellViewModel::whisperRuntimeReadinessSummary() const {
    return controller_.whisperRuntimeReadinessSummary();
}

QString DesktopShellViewModel::whisperRuntimePathSummary() const {
    return controller_.whisperRuntimePathSummary();
}

QString DesktopShellViewModel::whisperTranscriptionStatus() const {
    return controller_.whisperTranscriptionStatus();
}

QString DesktopShellViewModel::whisperTranscriptionReadinessSummary() const {
    return controller_.whisperTranscriptionReadinessSummary();
}

QString DesktopShellViewModel::whisperTranscriptionLastSummary() const {
    return controller_.whisperTranscriptionLastSummary();
}

QString DesktopShellViewModel::whisperTranscriptionFallbackSummary() const {
    return controller_.whisperTranscriptionFallbackSummary();
}

QString DesktopShellViewModel::whisperTranscriptionSafetySummary() const {
    return controller_.whisperTranscriptionSafetySummary();
}

QStringList DesktopShellViewModel::whisperTranscriptionTraceSummaries() const {
    return controller_.whisperTranscriptionTraceSummaries();
}

QString DesktopShellViewModel::piperRuntimeStatus() const {
    return controller_.piperRuntimeStatus();
}

QString DesktopShellViewModel::piperRuntimeReadinessSummary() const {
    return controller_.piperRuntimeReadinessSummary();
}

QString DesktopShellViewModel::piperRuntimePathSummary() const {
    return controller_.piperRuntimePathSummary();
}

// QML-facing convenience API keeps explicit path arguments to avoid a larger invokable contract
// change. NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
void DesktopShellViewModel::applyVoiceConfigurationPaths(const QString& piperBinaryPath,
                                                         const QString& piperModelPath,
                                                         const QString& whisperBinaryPath,
                                                         const QString& whisperModelPath) {
    setPiperBinaryPath(piperBinaryPath);
    setPiperModelPath(piperModelPath);
    setWhisperBinaryPath(whisperBinaryPath);
    setWhisperModelPath(whisperModelPath);
}

bool DesktopShellViewModel::generatePiperTtsFile(const QString& text) {
    return controller_.generatePiperTtsFile(text);
}

bool DesktopShellViewModel::localChatInferenceEnabled() const {
    return controller_.localChatInferenceEnabled();
}

void DesktopShellViewModel::setLocalChatInferenceEnabled(bool enabled) {
    settings_.setLocalChatInferenceEnabled(enabled);
    if (controller_.localChatInferenceEnabled() != settings_.localChatInferenceEnabled()) {
        controller_.setLocalChatInferenceEnabled(settings_.localChatInferenceEnabled());
    }
}

QString DesktopShellViewModel::localChatInferenceStatus() const {
    return controller_.localChatInferenceStatus();
}

QString DesktopShellViewModel::localChatInferenceSummary() const {
    return controller_.localChatInferenceSummary();
}

bool DesktopShellViewModel::localChatSendAvailable() const {
    return controller_.localChatSendAvailable();
}

QString DesktopShellViewModel::localChatSendAvailabilitySummary() const {
    return controller_.localChatSendAvailabilitySummary();
}

QString DesktopShellViewModel::chatSendLifecycleState() const {
    return controller_.chatSendLifecycleState();
}

QString DesktopShellViewModel::chatSendLifecycleSummary() const {
    return controller_.chatSendLifecycleSummary();
}

bool DesktopShellViewModel::promptContextInjectionEnabled() const {
    return controller_.promptContextInjectionEnabled();
}

void DesktopShellViewModel::setPromptContextInjectionEnabled(bool enabled) {
    settings_.setPromptContextInjectionEnabled(enabled);
    if (controller_.promptContextInjectionEnabled() != settings_.promptContextInjectionEnabled()) {
        controller_.setPromptContextInjectionEnabled(settings_.promptContextInjectionEnabled());
    }
}

QString DesktopShellViewModel::promptContextInjectionStatus() const {
    return controller_.promptContextInjectionStatus();
}

QString DesktopShellViewModel::promptContextInjectionSummary() const {
    return controller_.promptContextInjectionSummary();
}

int DesktopShellViewModel::promptContextInjectedBlockCount() const {
    return controller_.promptContextInjectedBlockCount();
}

QString DesktopShellViewModel::promptContextSourceSummary() const {
    return controller_.promptContextSourceSummary();
}

QString DesktopShellViewModel::promptContextSizeSummary() const {
    return controller_.promptContextSizeSummary();
}

QString DesktopShellViewModel::promptContextUsedSummary() const {
    return controller_.promptContextUsedSummary();
}

int DesktopShellViewModel::promptContextUsedMemoryCount() const {
    return controller_.promptContextUsedMemoryCount();
}

QString DesktopShellViewModel::contextBudgetUsageSummary() const {
    return controller_.contextBudgetUsageSummary();
}

int DesktopShellViewModel::contextIncludedCandidateCount() const {
    return controller_.contextIncludedCandidateCount();
}

int DesktopShellViewModel::contextExcludedCandidateCount() const {
    return controller_.contextExcludedCandidateCount();
}

QStringList DesktopShellViewModel::contextAssemblyTraceSummaries() const {
    return controller_.contextAssemblyTraceSummaries();
}

QStringList DesktopShellViewModel::promptContextBlockSummaries() const {
    return controller_.promptContextBlockSummaries();
}

QString DesktopShellViewModel::conversationWindowStatus() const {
    return controller_.conversationWindowStatus();
}

QString DesktopShellViewModel::conversationWindowSummary() const {
    return controller_.conversationWindowSummary();
}

QString DesktopShellViewModel::conversationWindowBudgetSummary() const {
    return controller_.conversationWindowBudgetSummary();
}

int DesktopShellViewModel::conversationWindowBudgetCharacters() const {
    return controller_.conversationWindowBudgetCharacters();
}

int DesktopShellViewModel::conversationWindowIncludedMessageCount() const {
    return controller_.conversationWindowIncludedMessageCount();
}

int DesktopShellViewModel::conversationWindowTruncatedMessageCount() const {
    return controller_.conversationWindowTruncatedMessageCount();
}

int DesktopShellViewModel::conversationWindowOmittedMessageCount() const {
    return controller_.conversationWindowOmittedMessageCount();
}

QString DesktopShellViewModel::conversationSummaryStatus() const {
    return controller_.conversationSummaryStatus();
}

QString DesktopShellViewModel::conversationSummaryText() const {
    return controller_.conversationSummaryText();
}

QString DesktopShellViewModel::conversationSummaryBudgetSummary() const {
    return controller_.conversationSummaryBudgetSummary();
}

int DesktopShellViewModel::conversationSummaryBudgetCharacters() const {
    return controller_.conversationSummaryBudgetCharacters();
}

int DesktopShellViewModel::conversationSummaryBlockCount() const {
    return controller_.conversationSummaryBlockCount();
}

int DesktopShellViewModel::conversationSummaryMessageCount() const {
    return controller_.conversationSummaryMessageCount();
}

int DesktopShellViewModel::conversationSummaryOmittedMessageCount() const {
    return controller_.conversationSummaryOmittedMessageCount();
}

int DesktopShellViewModel::conversationSummaryTruncatedBlockCount() const {
    return controller_.conversationSummaryTruncatedBlockCount();
}

QStringList DesktopShellViewModel::conversationSummaryBlockSummaries() const {
    return controller_.conversationSummaryBlockSummaries();
}

QString DesktopShellViewModel::retrievalPlanningStatus() const {
    return controller_.retrievalPlanningStatus();
}

QString DesktopShellViewModel::retrievalPlanningSummary() const {
    return controller_.retrievalPlanningSummary();
}

QString DesktopShellViewModel::retrievalPlanningReadiness() const {
    return controller_.retrievalPlanningReadiness();
}

QString DesktopShellViewModel::retrievalPlanningBudgetSummary() const {
    return controller_.retrievalPlanningBudgetSummary();
}

QString DesktopShellViewModel::retrievalPlanningSourceSummary() const {
    return controller_.retrievalPlanningSourceSummary();
}

int DesktopShellViewModel::retrievalPlanningSelectedSourceCount() const {
    return controller_.retrievalPlanningSelectedSourceCount();
}

int DesktopShellViewModel::retrievalPlanningExcludedSourceCount() const {
    return controller_.retrievalPlanningExcludedSourceCount();
}

int DesktopShellViewModel::retrievalPlanningSelectedCandidateCount() const {
    return controller_.retrievalPlanningSelectedCandidateCount();
}

int DesktopShellViewModel::retrievalPlanningExcludedCandidateCount() const {
    return controller_.retrievalPlanningExcludedCandidateCount();
}

int DesktopShellViewModel::retrievalPlanningTruncatedCandidateCount() const {
    return controller_.retrievalPlanningTruncatedCandidateCount();
}

QStringList DesktopShellViewModel::retrievalPlanningSourceSummaries() const {
    return controller_.retrievalPlanningSourceSummaries();
}

QString DesktopShellViewModel::memoryRelevanceSummaryText() const {
    return controller_.memoryRelevanceSummaryText();
}

QString DesktopShellViewModel::memoryRelevanceBudgetSummary() const {
    return controller_.memoryRelevanceBudgetSummary();
}

int DesktopShellViewModel::memoryRelevanceIncludedCount() const {
    return controller_.memoryRelevanceIncludedCount();
}

int DesktopShellViewModel::memoryRelevanceExcludedCount() const {
    return controller_.memoryRelevanceExcludedCount();
}

QStringList DesktopShellViewModel::memoryRelevanceTraceSummaries() const {
    return controller_.memoryRelevanceTraceSummaries();
}

QStringList DesktopShellViewModel::memoryRelevanceExclusionSummaries() const {
    return controller_.memoryRelevanceExclusionSummaries();
}

QString DesktopShellViewModel::conversationSalienceSummaryText() const {
    return controller_.conversationSalienceSummaryText();
}

QString DesktopShellViewModel::conversationSalienceBudgetSummary() const {
    return controller_.conversationSalienceBudgetSummary();
}

QString DesktopShellViewModel::conversationSalienceAllocationSummary() const {
    return controller_.conversationSalienceAllocationSummary();
}

int DesktopShellViewModel::conversationSalienceIncludedCount() const {
    return controller_.conversationSalienceIncludedCount();
}

int DesktopShellViewModel::conversationSalienceExcludedCount() const {
    return controller_.conversationSalienceExcludedCount();
}

int DesktopShellViewModel::conversationSalienceTruncatedCount() const {
    return controller_.conversationSalienceTruncatedCount();
}

QStringList DesktopShellViewModel::conversationSalienceTraceSummaries() const {
    return controller_.conversationSalienceTraceSummaries();
}

QStringList DesktopShellViewModel::conversationSalienceExclusionSummaries() const {
    return controller_.conversationSalienceExclusionSummaries();
}

QString DesktopShellViewModel::conversationCompressionStatus() const {
    return controller_.conversationCompressionStatus();
}

QString DesktopShellViewModel::conversationCompressionReadinessSummary() const {
    return controller_.conversationCompressionReadinessSummary();
}

QString DesktopShellViewModel::conversationCompressionPressureSummary() const {
    return controller_.conversationCompressionPressureSummary();
}

int DesktopShellViewModel::conversationCompressionCandidateCount() const {
    return controller_.conversationCompressionCandidateCount();
}

int DesktopShellViewModel::conversationCompressionSelectedCandidateCount() const {
    return controller_.conversationCompressionSelectedCandidateCount();
}

QString DesktopShellViewModel::conversationCompressionFallbackReason() const {
    return controller_.conversationCompressionFallbackReason();
}

QString DesktopShellViewModel::conversationCompressionTraceSummary() const {
    return controller_.conversationCompressionTraceSummary();
}

QString DesktopShellViewModel::conversationCompressionBudgetSummary() const {
    return controller_.conversationCompressionBudgetSummary();
}

QStringList DesktopShellViewModel::conversationCompressionCandidateSummaries() const {
    return controller_.conversationCompressionCandidateSummaries();
}

QStringList DesktopShellViewModel::conversationCompressionTraceSummaries() const {
    return controller_.conversationCompressionTraceSummaries();
}

bool DesktopShellViewModel::conversationSummaryAvailable() const {
    return controller_.conversationSummaryAvailable();
}

QString DesktopShellViewModel::conversationSummaryGenerationStatus() const {
    return controller_.conversationSummaryGenerationStatus();
}

QString DesktopShellViewModel::conversationSummaryReadinessSummary() const {
    return controller_.conversationSummaryReadinessSummary();
}

QString DesktopShellViewModel::conversationSummaryBlockedReason() const {
    return controller_.conversationSummaryBlockedReason();
}

QString DesktopShellViewModel::conversationSummaryEstimatedCompressionGain() const {
    return controller_.conversationSummaryEstimatedCompressionGain();
}

QString DesktopShellViewModel::conversationSummaryPreviewSummary() const {
    return controller_.conversationSummaryPreviewSummary();
}

QString DesktopShellViewModel::conversationSummaryPersistenceSummary() const {
    return controller_.conversationSummaryPersistenceSummary();
}

QString DesktopShellViewModel::conversationSummaryInjectionSummary() const {
    return controller_.conversationSummaryInjectionSummary();
}

QString DesktopShellViewModel::summaryContinuityStatus() const {
    return controller_.summaryContinuityStatus();
}

QString DesktopShellViewModel::summaryContinuityFreshnessSummary() const {
    return controller_.summaryContinuityFreshnessSummary();
}

QString DesktopShellViewModel::summaryContinuityCoverageSummary() const {
    return controller_.summaryContinuityCoverageSummary();
}

QString DesktopShellViewModel::summaryContinuityContributionSummary() const {
    return controller_.summaryContinuityContributionSummary();
}

QString DesktopShellViewModel::summaryContinuityFallbackSummary() const {
    return controller_.summaryContinuityFallbackSummary();
}

QString DesktopShellViewModel::summaryContinuityOrderingSummary() const {
    return controller_.summaryContinuityOrderingSummary();
}

QString DesktopShellViewModel::summaryContinuityBudgetTrace() const {
    return controller_.summaryContinuityBudgetTrace();
}

bool DesktopShellViewModel::contextExplainabilityEnabled() const {
    return settings_.contextExplainabilityVisible();
}

bool DesktopShellViewModel::contextExplainabilityVisible() const {
    return settings_.contextExplainabilityVisible();
}

void DesktopShellViewModel::setContextExplainabilityVisible(bool visible) {
    settings_.setContextExplainabilityVisible(visible);
}

QString DesktopShellViewModel::contextReasoningSummary() const {
    return controller_.contextReasoningSummary();
}

QString DesktopShellViewModel::contextReasoningBudgetSummary() const {
    return controller_.contextReasoningBudgetSummary();
}

QString DesktopShellViewModel::contextReasoningOrderingSummary() const {
    return controller_.contextReasoningOrderingSummary();
}

QString DesktopShellViewModel::contextReasoningFallbackSummary() const {
    return controller_.contextReasoningFallbackSummary();
}

QStringList DesktopShellViewModel::contextReasoningContributionSummaries() const {
    return controller_.contextReasoningContributionSummaries();
}

QStringList DesktopShellViewModel::contextReasoningInclusionHints() const {
    return controller_.contextReasoningInclusionHints();
}

QStringList DesktopShellViewModel::contextReasoningExclusionHints() const {
    return controller_.contextReasoningExclusionHints();
}

QStringList DesktopShellViewModel::contextReasoningDeveloperTraces() const {
    return controller_.contextReasoningDeveloperTraces();
}

QStringList DesktopShellViewModel::conversationSummaryCandidateSegments() const {
    return controller_.conversationSummaryCandidateSegments();
}

QStringList DesktopShellViewModel::conversationSummaryGenerationTraceSummaries() const {
    return controller_.conversationSummaryGenerationTraceSummaries();
}

core::SemanticRetrievalPolicy DesktopShellViewModel::semanticRetrievalPolicy() const {
    return controller_.semanticRetrievalPolicy();
}

bool DesktopShellViewModel::semanticRetrievalEnabled() const {
    return controller_.semanticRetrievalEnabled();
}

QString DesktopShellViewModel::semanticRetrievalStatus() const {
    return controller_.semanticRetrievalStatus();
}

QString DesktopShellViewModel::semanticRetrievalSummary() const {
    return controller_.semanticRetrievalSummary();
}

QString DesktopShellViewModel::semanticReadiness() const {
    return controller_.semanticReadiness();
}

QString DesktopShellViewModel::embeddingProviderReadiness() const {
    return controller_.embeddingProviderReadiness();
}

QString DesktopShellViewModel::embeddingProviderSummary() const {
    return controller_.embeddingProviderSummary();
}

QString DesktopShellViewModel::vectorIndexReadiness() const {
    return controller_.vectorIndexReadiness();
}

QString DesktopShellViewModel::vectorIndexSummary() const {
    return controller_.vectorIndexSummary();
}

int DesktopShellViewModel::vectorIndexedItemCount() const {
    return controller_.vectorIndexedItemCount();
}

QString DesktopShellViewModel::semanticProviderMode() const {
    return controller_.semanticProviderMode();
}

QString DesktopShellViewModel::selectedSemanticProviderName() const {
    return controller_.selectedSemanticProviderName();
}

QString DesktopShellViewModel::semanticProviderReadiness() const {
    return controller_.semanticProviderReadiness();
}

QString DesktopShellViewModel::semanticProviderHealth() const {
    return controller_.semanticProviderHealth();
}

QString DesktopShellViewModel::semanticProviderStatusSummary() const {
    return controller_.semanticProviderStatusSummary();
}

QString DesktopShellViewModel::semanticActivationReadiness() const {
    return controller_.semanticActivationReadiness();
}

QString DesktopShellViewModel::semanticActivationSummary() const {
    return controller_.semanticActivationSummary();
}

QStringList DesktopShellViewModel::semanticProviderCapabilitySummaries() const {
    return controller_.semanticProviderCapabilitySummaries();
}

QStringList DesktopShellViewModel::semanticActivationRequiredSteps() const {
    return controller_.semanticActivationRequiredSteps();
}

QStringList DesktopShellViewModel::semanticRetrievalReadinessChecks() const {
    return controller_.semanticRetrievalReadinessChecks();
}

QString DesktopShellViewModel::semanticCandidateStatus() const {
    return controller_.semanticCandidateStatus();
}

QString DesktopShellViewModel::semanticCandidateSummary() const {
    return controller_.semanticCandidateSummary();
}

QString DesktopShellViewModel::semanticCandidateBudgetSummary() const {
    return controller_.semanticCandidateBudgetSummary();
}

QString DesktopShellViewModel::semanticCandidateArbitrationSummary() const {
    return controller_.semanticCandidateArbitrationSummary();
}

int DesktopShellViewModel::semanticCandidateCount() const {
    return controller_.semanticCandidateCount();
}

int DesktopShellViewModel::semanticCandidateSelectedCount() const {
    return controller_.semanticCandidateSelectedCount();
}

int DesktopShellViewModel::semanticCandidateExcludedCount() const {
    return controller_.semanticCandidateExcludedCount();
}

int DesktopShellViewModel::semanticCandidateTruncatedCount() const {
    return controller_.semanticCandidateTruncatedCount();
}

QStringList DesktopShellViewModel::semanticCandidateParticipationSummaries() const {
    return controller_.semanticCandidateParticipationSummaries();
}

QString DesktopShellViewModel::hybridRetrievalStatus() const {
    return controller_.hybridRetrievalStatus();
}

QString DesktopShellViewModel::hybridRetrievalReadiness() const {
    return controller_.hybridRetrievalReadiness();
}

QString DesktopShellViewModel::hybridRetrievalSummary() const {
    return controller_.hybridRetrievalSummary();
}

QStringList DesktopShellViewModel::hybridRetrievalReadinessChecks() const {
    return controller_.hybridRetrievalReadinessChecks();
}

QString DesktopShellViewModel::semanticArbitrationStatus() const {
    return controller_.semanticArbitrationStatus();
}

QString DesktopShellViewModel::semanticArbitrationReadiness() const {
    return controller_.semanticArbitrationReadiness();
}

QString DesktopShellViewModel::semanticArbitrationSummary() const {
    return controller_.semanticArbitrationSummary();
}

QString DesktopShellViewModel::semanticArbitrationBudgetSummary() const {
    return controller_.semanticArbitrationBudgetSummary();
}

QStringList DesktopShellViewModel::semanticArbitrationSelectionSummaries() const {
    return controller_.semanticArbitrationSelectionSummaries();
}

QStringList DesktopShellViewModel::semanticArbitrationChecks() const {
    return controller_.semanticArbitrationChecks();
}

QString DesktopShellViewModel::embeddingRuntimeReadiness() const {
    return controller_.embeddingRuntimeReadiness();
}

QString DesktopShellViewModel::embeddingRuntimeSummary() const {
    return controller_.embeddingRuntimeSummary();
}

QString DesktopShellViewModel::embeddingRuntimeBudgetSummary() const {
    return controller_.embeddingRuntimeBudgetSummary();
}

QStringList DesktopShellViewModel::embeddingRuntimeRequirementSummaries() const {
    return controller_.embeddingRuntimeRequirementSummaries();
}

QStringList DesktopShellViewModel::embeddingRuntimeConstraintSummaries() const {
    return controller_.embeddingRuntimeConstraintSummaries();
}

QString DesktopShellViewModel::isolatedEmbeddingRuntimeStatus() const {
    return controller_.isolatedEmbeddingRuntimeStatus();
}

QString DesktopShellViewModel::isolatedEmbeddingRuntimeHealth() const {
    return controller_.isolatedEmbeddingRuntimeHealth();
}

QString DesktopShellViewModel::isolatedEmbeddingRuntimeReadiness() const {
    return controller_.isolatedEmbeddingRuntimeReadiness();
}

QString DesktopShellViewModel::isolatedEmbeddingRuntimeSummary() const {
    return controller_.isolatedEmbeddingRuntimeSummary();
}

QString DesktopShellViewModel::isolatedEmbeddingRuntimeBoundedState() const {
    return controller_.isolatedEmbeddingRuntimeBoundedState();
}

QStringList DesktopShellViewModel::isolatedEmbeddingRuntimeChecks() const {
    return controller_.isolatedEmbeddingRuntimeChecks();
}

QString DesktopShellViewModel::vectorPersistenceStatus() const {
    return controller_.vectorPersistenceStatus();
}

QString DesktopShellViewModel::vectorPersistenceHealth() const {
    return controller_.vectorPersistenceHealth();
}

QString DesktopShellViewModel::vectorPersistenceReadiness() const {
    return controller_.vectorPersistenceReadiness();
}

QString DesktopShellViewModel::vectorPersistenceSummary() const {
    return controller_.vectorPersistenceSummary();
}

QString DesktopShellViewModel::vectorPersistenceBoundedState() const {
    return controller_.vectorPersistenceBoundedState();
}

int DesktopShellViewModel::vectorPersistenceIndexedItemCount() const {
    return controller_.vectorPersistenceIndexedItemCount();
}

QStringList DesktopShellViewModel::vectorPersistenceChecks() const {
    return controller_.vectorPersistenceChecks();
}

QString DesktopShellViewModel::semanticSearchStatus() const {
    return controller_.semanticSearchStatus();
}

QString DesktopShellViewModel::semanticSearchReadiness() const {
    return controller_.semanticSearchReadiness();
}

QString DesktopShellViewModel::semanticSearchSummary() const {
    return controller_.semanticSearchSummary();
}

QString DesktopShellViewModel::semanticSearchBudgetSummary() const {
    return controller_.semanticSearchBudgetSummary();
}

QString DesktopShellViewModel::semanticSearchRuntimeState() const {
    return controller_.semanticSearchRuntimeState();
}

int DesktopShellViewModel::semanticSearchCandidateCount() const {
    return controller_.semanticSearchCandidateCount();
}

QString DesktopShellViewModel::semanticSearchArbitrationSummary() const {
    return controller_.semanticSearchArbitrationSummary();
}

QStringList DesktopShellViewModel::semanticSearchCandidateSummaries() const {
    return controller_.semanticSearchCandidateSummaries();
}

QStringList DesktopShellViewModel::semanticSearchChecks() const {
    return controller_.semanticSearchChecks();
}

QString DesktopShellViewModel::hybridBridgeStatus() const {
    return controller_.hybridBridgeStatus();
}

QString DesktopShellViewModel::hybridBridgeReadiness() const {
    return controller_.hybridBridgeReadiness();
}

QString DesktopShellViewModel::hybridBridgeSummary() const {
    return controller_.hybridBridgeSummary();
}

QString DesktopShellViewModel::hybridBridgeBudgetSummary() const {
    return controller_.hybridBridgeBudgetSummary();
}

QString DesktopShellViewModel::hybridBridgeSourceSummary() const {
    return controller_.hybridBridgeSourceSummary();
}

QString DesktopShellViewModel::hybridBridgeArbitrationSummary() const {
    return controller_.hybridBridgeArbitrationSummary();
}

QString DesktopShellViewModel::hybridBridgeFallbackSummary() const {
    return controller_.hybridBridgeFallbackSummary();
}

int DesktopShellViewModel::hybridBridgeCandidateCount() const {
    return controller_.hybridBridgeCandidateCount();
}

int DesktopShellViewModel::hybridBridgeSemanticFillCount() const {
    return controller_.hybridBridgeSemanticFillCount();
}

QStringList DesktopShellViewModel::hybridBridgeCandidateSummaries() const {
    return controller_.hybridBridgeCandidateSummaries();
}

QStringList DesktopShellViewModel::hybridBridgeChecks() const {
    return controller_.hybridBridgeChecks();
}

QString DesktopShellViewModel::semanticAcceptanceStatus() const {
    return controller_.semanticAcceptanceStatus();
}

QString DesktopShellViewModel::semanticAcceptanceReadiness() const {
    return controller_.semanticAcceptanceReadiness();
}

QString DesktopShellViewModel::semanticAcceptanceSummary() const {
    return controller_.semanticAcceptanceSummary();
}

QString DesktopShellViewModel::semanticAcceptanceBudgetSummary() const {
    return controller_.semanticAcceptanceBudgetSummary();
}

QString DesktopShellViewModel::semanticAcceptanceSourceSummary() const {
    return controller_.semanticAcceptanceSourceSummary();
}

QString DesktopShellViewModel::semanticAcceptanceArbitrationSummary() const {
    return controller_.semanticAcceptanceArbitrationSummary();
}

QString DesktopShellViewModel::semanticAcceptanceFallbackSummary() const {
    return controller_.semanticAcceptanceFallbackSummary();
}

int DesktopShellViewModel::semanticAcceptanceAcceptedCount() const {
    return controller_.semanticAcceptanceAcceptedCount();
}

int DesktopShellViewModel::semanticAcceptanceBudgetCharacters() const {
    return controller_.semanticAcceptanceBudgetCharacters();
}

QStringList DesktopShellViewModel::semanticAcceptanceCandidateSummaries() const {
    return controller_.semanticAcceptanceCandidateSummaries();
}

QStringList DesktopShellViewModel::semanticAcceptanceChecks() const {
    return controller_.semanticAcceptanceChecks();
}

QString DesktopShellViewModel::semanticSupplementAssemblyStatus() const {
    return controller_.semanticSupplementAssemblyStatus();
}

QString DesktopShellViewModel::semanticSupplementAssemblyReadiness() const {
    return controller_.semanticSupplementAssemblyReadiness();
}

QString DesktopShellViewModel::semanticSupplementAssemblySummary() const {
    return controller_.semanticSupplementAssemblySummary();
}

QString DesktopShellViewModel::semanticSupplementAssemblyBudgetSummary() const {
    return controller_.semanticSupplementAssemblyBudgetSummary();
}

QString DesktopShellViewModel::semanticSupplementAssemblySafetySummary() const {
    return controller_.semanticSupplementAssemblySafetySummary();
}

int DesktopShellViewModel::semanticSupplementAssemblyBlockCount() const {
    return controller_.semanticSupplementAssemblyBlockCount();
}

int DesktopShellViewModel::semanticSupplementAssemblyBudgetCharacters() const {
    return controller_.semanticSupplementAssemblyBudgetCharacters();
}

QStringList DesktopShellViewModel::semanticSupplementAssemblyChecks() const {
    return controller_.semanticSupplementAssemblyChecks();
}

QString DesktopShellViewModel::semanticPromptAuthorityStatus() const {
    return controller_.semanticPromptAuthorityStatus();
}

QString DesktopShellViewModel::semanticPromptAuthorityDecisionSummary() const {
    return controller_.semanticPromptAuthorityDecisionSummary();
}

QString DesktopShellViewModel::semanticPromptAuthoritySafetySummary() const {
    return controller_.semanticPromptAuthoritySafetySummary();
}

QString DesktopShellViewModel::semanticPromptAuthorityReadinessSummary() const {
    return controller_.semanticPromptAuthorityReadinessSummary();
}

QString DesktopShellViewModel::semanticPromptAuthorityFallbackSummary() const {
    return controller_.semanticPromptAuthorityFallbackSummary();
}

QString DesktopShellViewModel::semanticPromptAuthorityAuditSummary() const {
    return controller_.semanticPromptAuthorityAuditSummary();
}

int DesktopShellViewModel::semanticPromptAuthorityWouldIncludeBlockCount() const {
    return controller_.semanticPromptAuthorityWouldIncludeBlockCount();
}

QStringList DesktopShellViewModel::semanticPromptAuthorityChecks() const {
    return controller_.semanticPromptAuthorityChecks();
}

bool DesktopShellViewModel::semanticPromptInclusionEnabled() const {
    return controller_.semanticPromptInclusionEnabled();
}

void DesktopShellViewModel::setSemanticPromptInclusionEnabled(bool enabled) {
    settings_.setSemanticPromptInclusionEnabled(enabled);
    if (controller_.semanticPromptInclusionEnabled() !=
        settings_.semanticPromptInclusionEnabled()) {
        controller_.setSemanticPromptInclusionEnabled(settings_.semanticPromptInclusionEnabled());
    }
}

QString DesktopShellViewModel::semanticPromptInclusionStatus() const {
    return controller_.semanticPromptInclusionStatus();
}

QString DesktopShellViewModel::semanticPromptInclusionSummary() const {
    return controller_.semanticPromptInclusionSummary();
}

int DesktopShellViewModel::semanticPromptInclusionIncludedCount() const {
    return controller_.semanticPromptInclusionIncludedCount();
}

QString DesktopShellViewModel::semanticPromptInclusionBudgetSummary() const {
    return controller_.semanticPromptInclusionBudgetSummary();
}

QString DesktopShellViewModel::semanticPromptInclusionFallbackSummary() const {
    return controller_.semanticPromptInclusionFallbackSummary();
}

QString DesktopShellViewModel::semanticPromptInclusionAuditSummary() const {
    return controller_.semanticPromptInclusionAuditSummary();
}

bool DesktopShellViewModel::semanticPromptInclusionDeterministicAuthorityPreserved() const {
    return controller_.semanticPromptInclusionDeterministicAuthorityPreserved();
}

QStringList DesktopShellViewModel::semanticPromptInclusionChecks() const {
    return controller_.semanticPromptInclusionChecks();
}

bool DesktopShellViewModel::localInferenceStreamingEnabled() const {
    return controller_.localInferenceStreamingEnabled();
}

void DesktopShellViewModel::setLocalInferenceStreamingEnabled(bool enabled) {
    settings_.setLocalInferenceStreamingEnabled(enabled);
    if (controller_.localInferenceStreamingEnabled() !=
        settings_.localInferenceStreamingEnabled()) {
        controller_.setLocalInferenceStreamingEnabled(settings_.localInferenceStreamingEnabled());
    }
}

int DesktopShellViewModel::localInferenceTimeoutMs() const {
    return controller_.localInferenceTimeoutMs();
}

void DesktopShellViewModel::setLocalInferenceTimeoutMs(int timeoutMs) {
    settings_.setLocalInferenceTimeoutMs(timeoutMs);
    if (controller_.localInferenceTimeoutMs() != settings_.localInferenceTimeoutMs()) {
        controller_.setLocalInferenceTimeoutMs(settings_.localInferenceTimeoutMs());
    }
}

bool DesktopShellViewModel::localInferenceBusy() const {
    return controller_.localInferenceBusy();
}

QString DesktopShellViewModel::localInferenceRuntimeState() const {
    return controller_.localInferenceRuntimeState();
}

QString DesktopShellViewModel::localInferenceStatus() const {
    return controller_.localInferenceStatus();
}

QString DesktopShellViewModel::localInferenceSummary() const {
    return controller_.localInferenceSummary();
}

QString DesktopShellViewModel::localInferenceLastResponseSummary() const {
    return controller_.localInferenceLastResponseSummary();
}

QString DesktopShellViewModel::localInferenceLatencySummary() const {
    return controller_.localInferenceLatencySummary();
}

QStringList DesktopShellViewModel::localInferenceTraceSummaries() const {
    return controller_.localInferenceTraceSummaries();
}

bool DesktopShellViewModel::localInferenceStreamingAvailable() const {
    return controller_.localInferenceStreamingAvailable();
}

QString DesktopShellViewModel::localInferenceStreamStatus() const {
    return controller_.localInferenceStreamStatus();
}

QString DesktopShellViewModel::localInferenceStreamSummary() const {
    return controller_.localInferenceStreamSummary();
}

QString DesktopShellViewModel::localInferenceStreamingText() const {
    return controller_.localInferenceStreamingText();
}

int DesktopShellViewModel::availableToolCount() const {
    return controller_.availableToolCount();
}

QStringList DesktopShellViewModel::availableToolIds() const {
    return controller_.availableToolIds();
}

QString DesktopShellViewModel::memoryStatus() const {
    return controller_.memoryStatus();
}

QString DesktopShellViewModel::chatHistoryStatus() const {
    return controller_.chatHistoryStatus();
}

QString DesktopShellViewModel::conversationStoreStatus() const {
    return controller_.conversationStoreStatus();
}

int DesktopShellViewModel::conversationStoreConversationCount() const {
    return controller_.conversationStoreConversationCount();
}

QString DesktopShellViewModel::activeConversationSummary() const {
    return controller_.activeConversationSummary();
}

QStringList DesktopShellViewModel::conversationStoreSummaries() const {
    return controller_.conversationStoreSummaries();
}

QString DesktopShellViewModel::activeConversationId() const {
    return controller_.activeConversationId();
}

bool DesktopShellViewModel::activeConversationArchived() const {
    return controller_.activeConversationArchived();
}

QString DesktopShellViewModel::activeConversationStateSummary() const {
    return controller_.activeConversationStateSummary();
}

QStringList DesktopShellViewModel::conversationIds() const {
    return controller_.conversationIds();
}

QStringList DesktopShellViewModel::conversationTitles() const {
    return controller_.conversationTitles();
}

QStringList DesktopShellViewModel::conversationActiveSummaries() const {
    return controller_.conversationActiveSummaries();
}

QStringList DesktopShellViewModel::conversationLastUpdatedSummaries() const {
    return controller_.conversationLastUpdatedSummaries();
}

QStringList DesktopShellViewModel::conversationMessageCountSummaries() const {
    return controller_.conversationMessageCountSummaries();
}

QStringList DesktopShellViewModel::conversationArchivedSummaries() const {
    return controller_.conversationArchivedSummaries();
}

QStringList DesktopShellViewModel::conversationPinnedSummaries() const {
    return controller_.conversationPinnedSummaries();
}

int DesktopShellViewModel::activeConversationCount() const {
    return controller_.activeConversationCount();
}

int DesktopShellViewModel::archivedConversationCount() const {
    return controller_.archivedConversationCount();
}

int DesktopShellViewModel::userCreatedConversationCount() const {
    return controller_.userCreatedConversationCount();
}

bool DesktopShellViewModel::conversationBrowserEmptyStateVisible() const {
    return controller_.conversationBrowserEmptyStateVisible();
}

QString DesktopShellViewModel::conversationBrowserEmptyStateSummary() const {
    return controller_.conversationBrowserEmptyStateSummary();
}

QString DesktopShellViewModel::conversationHistorySummaryText() const {
    return controller_.conversationHistorySummaryText();
}

QStringList DesktopShellViewModel::conversationHistorySummaryLines() const {
    return controller_.conversationHistorySummaryLines();
}

int DesktopShellViewModel::conversationHistoryMessageCount() const {
    return controller_.conversationHistoryMessageCount();
}

QString DesktopShellViewModel::conversationPersistenceStatus() const {
    return controller_.conversationPersistenceStatus();
}

QString DesktopShellViewModel::conversationLastSavedStatus() const {
    return controller_.conversationLastSavedStatus();
}

QString DesktopShellViewModel::conversationLastRestoredStatus() const {
    return controller_.conversationLastRestoredStatus();
}

QString DesktopShellViewModel::conversationBrowserStatus() const {
    return controller_.conversationBrowserStatus();
}

QString DesktopShellViewModel::conversationBrowserSummaryText() const {
    return controller_.conversationBrowserSummaryText();
}

int DesktopShellViewModel::conversationListEntryCount() const {
    return controller_.conversationListEntryCount();
}

QString DesktopShellViewModel::conversationListCurrentTitle() const {
    return controller_.conversationListCurrentTitle();
}

int DesktopShellViewModel::conversationListCurrentMessageCount() const {
    return controller_.conversationListCurrentMessageCount();
}

QString DesktopShellViewModel::conversationListCurrentPersistenceStatus() const {
    return controller_.conversationListCurrentPersistenceStatus();
}

QString DesktopShellViewModel::conversationListCurrentLastUpdatedSummary() const {
    return controller_.conversationListCurrentLastUpdatedSummary();
}

QString DesktopShellViewModel::conversationListCurrentSearchAvailabilitySummary() const {
    return controller_.conversationListCurrentSearchAvailabilitySummary();
}

QString DesktopShellViewModel::conversationListCurrentExportAvailabilitySummary() const {
    return controller_.conversationListCurrentExportAvailabilitySummary();
}

QString DesktopShellViewModel::conversationListCurrentSummary() const {
    return controller_.conversationListCurrentSummary();
}

QString DesktopShellViewModel::conversationCurrentStorageMode() const {
    return controller_.conversationCurrentStorageMode();
}

QString DesktopShellViewModel::conversationFutureStorageMode() const {
    return controller_.conversationFutureStorageMode();
}

QString DesktopShellViewModel::conversationMigrationReadiness() const {
    return controller_.conversationMigrationReadiness();
}

QString DesktopShellViewModel::conversationMigrationStatusSummary() const {
    return controller_.conversationMigrationStatusSummary();
}

QString DesktopShellViewModel::conversationSchemaStatusSummary() const {
    return controller_.conversationSchemaStatusSummary();
}

QString DesktopShellViewModel::conversationSearchQueryText() const {
    return controller_.conversationSearchQueryText();
}

QString DesktopShellViewModel::conversationSearchStatus() const {
    return controller_.conversationSearchStatus();
}

QString DesktopShellViewModel::conversationSearchSummaryText() const {
    return controller_.conversationSearchSummaryText();
}

int DesktopShellViewModel::conversationSearchResultCount() const {
    return controller_.conversationSearchResultCount();
}

QStringList DesktopShellViewModel::conversationSearchResultSummaries() const {
    return controller_.conversationSearchResultSummaries();
}

bool DesktopShellViewModel::conversationExportAvailable() const {
    return controller_.conversationExportAvailable();
}

QString DesktopShellViewModel::conversationExportReadinessStatus() const {
    return controller_.conversationExportReadinessStatus();
}

QString DesktopShellViewModel::conversationExportReadinessSummary() const {
    return controller_.conversationExportReadinessSummary();
}

QStringList DesktopShellViewModel::conversationExportReadinessChecks() const {
    return controller_.conversationExportReadinessChecks();
}

QString DesktopShellViewModel::conversationExportLastResultSummary() const {
    return controller_.conversationExportLastResultSummary();
}

QString DesktopShellViewModel::conversationExportLastStatus() const {
    return controller_.conversationExportLastStatus();
}

QString DesktopShellViewModel::conversationExportLastFileName() const {
    return controller_.conversationExportLastFileName();
}

int DesktopShellViewModel::conversationExportLastMessageCount() const {
    return controller_.conversationExportLastMessageCount();
}

QString DesktopShellViewModel::conversationExportLastTimestamp() const {
    return controller_.conversationExportLastTimestamp();
}

QString DesktopShellViewModel::conversationDuplicateLastStatus() const {
    return controller_.conversationDuplicateLastStatus();
}

QString DesktopShellViewModel::conversationDuplicateLastResultSummary() const {
    return controller_.conversationDuplicateLastResultSummary();
}

bool DesktopShellViewModel::conversationDeleteAvailable() const {
    return controller_.conversationDeleteAvailable();
}

QString DesktopShellViewModel::conversationDeletePolicyStatus() const {
    return controller_.conversationDeletePolicyStatus();
}

QString DesktopShellViewModel::conversationDeletePolicySummary() const {
    return controller_.conversationDeletePolicySummary();
}

QStringList DesktopShellViewModel::conversationDeletePolicyRequirements() const {
    return controller_.conversationDeletePolicyRequirements();
}

QString DesktopShellViewModel::conversationDeleteReadinessStatus() const {
    return controller_.conversationDeleteReadinessStatus();
}

QString DesktopShellViewModel::conversationDeleteReadinessSummary() const {
    return controller_.conversationDeleteReadinessSummary();
}

QStringList DesktopShellViewModel::conversationDeleteReadinessChecks() const {
    return controller_.conversationDeleteReadinessChecks();
}

QString DesktopShellViewModel::conversationDeleteLastStatus() const {
    return controller_.conversationDeleteLastStatus();
}

QString DesktopShellViewModel::conversationDeleteLastResultSummary() const {
    return controller_.conversationDeleteLastResultSummary();
}

int DesktopShellViewModel::memoryCandidateCount() const {
    return controller_.memoryCandidateCount();
}

int DesktopShellViewModel::pendingMemoryCandidateCount() const {
    return controller_.pendingMemoryCandidateCount();
}

int DesktopShellViewModel::approvedMemoryCandidateCount() const {
    return controller_.approvedMemoryCandidateCount();
}

int DesktopShellViewModel::rejectedMemoryCandidateCount() const {
    return controller_.rejectedMemoryCandidateCount();
}

int DesktopShellViewModel::archivedMemoryCandidateCount() const {
    return controller_.archivedMemoryCandidateCount();
}

int DesktopShellViewModel::committedMemoryCandidateCount() const {
    return controller_.committedMemoryCandidateCount();
}

QStringList DesktopShellViewModel::memoryCandidateIds() const {
    return controller_.memoryCandidateIds();
}

QStringList DesktopShellViewModel::memoryCandidateReviewStates() const {
    return controller_.memoryCandidateReviewStates();
}

QStringList DesktopShellViewModel::memoryCandidateCommitStatuses() const {
    return controller_.memoryCandidateCommitStatuses();
}

QStringList DesktopShellViewModel::memoryCandidateSummaries() const {
    return controller_.memoryCandidateSummaries();
}

QStringList DesktopShellViewModel::pendingMemoryCandidateSummaries() const {
    return controller_.pendingMemoryCandidateSummaries();
}

QStringList DesktopShellViewModel::approvedMemoryCandidateSummaries() const {
    return controller_.approvedMemoryCandidateSummaries();
}

QStringList DesktopShellViewModel::rejectedMemoryCandidateSummaries() const {
    return controller_.rejectedMemoryCandidateSummaries();
}

QStringList DesktopShellViewModel::archivedMemoryCandidateSummaries() const {
    return controller_.archivedMemoryCandidateSummaries();
}

QString DesktopShellViewModel::lastMemoryCandidateReviewStatus() const {
    return controller_.lastMemoryCandidateReviewStatus();
}

QString DesktopShellViewModel::lastMemoryCandidateReviewSummary() const {
    return controller_.lastMemoryCandidateReviewSummary();
}

QString DesktopShellViewModel::memoryCommitReadinessStatus() const {
    return controller_.memoryCommitReadinessStatus();
}

QString DesktopShellViewModel::memoryCommitReadinessSummary() const {
    return controller_.memoryCommitReadinessSummary();
}

QStringList DesktopShellViewModel::memoryCommitReadinessChecks() const {
    return controller_.memoryCommitReadinessChecks();
}

int DesktopShellViewModel::memoryCommitPlanCount() const {
    return controller_.memoryCommitPlanCount();
}

QString DesktopShellViewModel::memoryCommitTargetSummary() const {
    return controller_.memoryCommitTargetSummary();
}

QStringList DesktopShellViewModel::memoryCommitCandidateSummaries() const {
    return controller_.memoryCommitCandidateSummaries();
}

QString DesktopShellViewModel::lastMemoryCommitStatus() const {
    return controller_.lastMemoryCommitStatus();
}

QString DesktopShellViewModel::lastMemoryCommitResultSummary() const {
    return controller_.lastMemoryCommitResultSummary();
}

QString DesktopShellViewModel::memoryRecallPolicyStatus() const {
    return controller_.memoryRecallPolicyStatus();
}

QString DesktopShellViewModel::memoryRecallPolicySummary() const {
    return controller_.memoryRecallPolicySummary();
}

QString DesktopShellViewModel::memoryRecallQueryText() const {
    return controller_.memoryRecallQueryText();
}

QString DesktopShellViewModel::memoryRecallStatus() const {
    return controller_.memoryRecallStatus();
}

QString DesktopShellViewModel::memoryRecallSummaryText() const {
    return controller_.memoryRecallSummaryText();
}

int DesktopShellViewModel::memoryRecallResultCount() const {
    return controller_.memoryRecallResultCount();
}

QStringList DesktopShellViewModel::memoryRecallResultSummaries() const {
    return controller_.memoryRecallResultSummaries();
}

QString DesktopShellViewModel::contextAssemblyPolicyStatus() const {
    return controller_.contextAssemblyPolicyStatus();
}

QString DesktopShellViewModel::contextAssemblyPolicySummary() const {
    return controller_.contextAssemblyPolicySummary();
}

QString DesktopShellViewModel::contextAssemblyStatus() const {
    return controller_.contextAssemblyStatus();
}

QString DesktopShellViewModel::contextAssemblySummaryText() const {
    return controller_.contextAssemblySummaryText();
}

int DesktopShellViewModel::contextAssemblySourceCount() const {
    return controller_.contextAssemblySourceCount();
}

int DesktopShellViewModel::contextAssemblyAvailableSourceCount() const {
    return controller_.contextAssemblyAvailableSourceCount();
}

int DesktopShellViewModel::contextAssemblyCandidateBlockCount() const {
    return controller_.contextAssemblyCandidateBlockCount();
}

int DesktopShellViewModel::contextAssemblyEstimatedSize() const {
    return controller_.contextAssemblyEstimatedSize();
}

QString DesktopShellViewModel::conversationContextAvailability() const {
    return controller_.conversationContextAvailability();
}

QString DesktopShellViewModel::committedMemoryContextAvailability() const {
    return controller_.committedMemoryContextAvailability();
}

QString DesktopShellViewModel::runtimeMetadataContextAvailability() const {
    return controller_.runtimeMetadataContextAvailability();
}

QString DesktopShellViewModel::orchestrationContextAvailability() const {
    return controller_.orchestrationContextAvailability();
}

QStringList DesktopShellViewModel::contextAssemblySourceSummaries() const {
    return controller_.contextAssemblySourceSummaries();
}

QStringList DesktopShellViewModel::contextAssemblyReadinessChecks() const {
    return controller_.contextAssemblyReadinessChecks();
}

int DesktopShellViewModel::memoryEntryCount() const {
    return controller_.memoryEntryCount();
}

QString DesktopShellViewModel::memoryMaintenanceStatus() const {
    return controller_.memoryMaintenanceStatus();
}

QString DesktopShellViewModel::chatMaintenanceStatus() const {
    return controller_.chatMaintenanceStatus();
}

QString DesktopShellViewModel::currentModeName() const {
    return modeManager_.currentModeName();
}

void DesktopShellViewModel::setCurrentModeName(const QString& modeName) {
    modeManager_.setModeByName(modeName);
}

QStringList DesktopShellViewModel::availableModes() const {
    return modeManager_.availableModes();
}

QString DesktopShellViewModel::currentPage() const {
    return currentPage_;
}

void DesktopShellViewModel::setCurrentPage(const QString& page) {
    const auto normalized = normalizedPageOrDefault(page);
    if (normalized == currentPage_) {
        return;
    }

    currentPage_ = normalized;
    emit currentPageChanged();
}

QStringList DesktopShellViewModel::availablePages() const {
    return {
        QStringLiteral("Memory"),
        QStringLiteral("Dashboard"),
        QStringLiteral("Agents"),
    };
}

ChatMessageListModel* DesktopShellViewModel::chatMessages() {
    return &chatMessages_;
}

QStringList DesktopShellViewModel::memoryEntries() const {
    return controller_.memoryEntries();
}

QString DesktopShellViewModel::themeName() const {
    return settings_.themeName();
}

void DesktopShellViewModel::setThemeName(const QString& themeName) {
    settings_.setThemeName(themeName);
}

QString DesktopShellViewModel::configurationProfile() const {
    return settings_.configurationProfile();
}

void DesktopShellViewModel::setConfigurationProfile(const QString& configurationProfile) {
    settings_.setConfigurationProfile(configurationProfile);
}

QString DesktopShellViewModel::appLanguage() const {
    return settings_.appLanguage();
}

void DesktopShellViewModel::setAppLanguage(const QString& language) {
    settings_.setAppLanguage(language);
}

QStringList DesktopShellViewModel::availableLanguages() const {
    return settings_.availableLanguages();
}

QString DesktopShellViewModel::languageDisplayName(const QString& language) const {
    return settings_.languageDisplayName(language);
}

bool DesktopShellViewModel::companionEnabled() const {
    return settings_.companionEnabled();
}

void DesktopShellViewModel::setCompanionEnabled(bool enabled) {
    settings_.setCompanionEnabled(enabled);
}

bool DesktopShellViewModel::companionAvailable() const {
    return companionService_.summary(settings_.companionEnabled(), companionNativeAvailable_,
                                     companionPaused_)
        .available;
}

bool DesktopShellViewModel::companionPaused() const {
    return companionPaused_;
}

void DesktopShellViewModel::setCompanionNativeAvailable(bool available) {
    if (available == companionNativeAvailable_) {
        return;
    }

    companionNativeAvailable_ = available;
    if (!available && companionPaused_) {
        companionPaused_ = false;
    }
    emit companionChanged();
}

void DesktopShellViewModel::setCompanionPaused(bool paused) {
    if (paused == companionPaused_) {
        return;
    }

    companionPaused_ = paused;
    emit companionChanged();
}

QString DesktopShellViewModel::companionStatus() const {
    return companionService_.summary(settings_.companionEnabled(), companionNativeAvailable_,
                                     companionPaused_)
        .status;
}

QString DesktopShellViewModel::companionAvailability() const {
    return companionService_.summary(settings_.companionEnabled(), companionNativeAvailable_,
                                     companionPaused_)
        .availability;
}

QString DesktopShellViewModel::companionPlatformCapability() const {
    return companionService_.summary(settings_.companionEnabled(), companionNativeAvailable_,
                                     companionPaused_)
        .platformCapability;
}

QString DesktopShellViewModel::companionPermissionPosture() const {
    return companionService_.summary(settings_.companionEnabled(), companionNativeAvailable_,
                                     companionPaused_)
        .permissionPostureSummary;
}

QString DesktopShellViewModel::companionSafetyBoundary() const {
    return companionService_.summary(settings_.companionEnabled(), companionNativeAvailable_,
                                     companionPaused_)
        .safetyBoundarySummary;
}

QString DesktopShellViewModel::companionQuickCaptureSummary() const {
    return companionService_.summary(settings_.companionEnabled(), companionNativeAvailable_,
                                     companionPaused_)
        .quickCaptureSummary;
}

QStringList DesktopShellViewModel::companionActionSummaries() const {
    return companionService_.summary(settings_.companionEnabled(), companionNativeAvailable_,
                                     companionPaused_)
        .actionSummaries;
}

QStringList DesktopShellViewModel::companionPlatformSummaries() const {
    return companionService_.summary(settings_.companionEnabled(), companionNativeAvailable_,
                                     companionPaused_)
        .platformSummaries;
}

QStringList DesktopShellViewModel::companionTraceSummaries() const {
    return companionService_.summary(settings_.companionEnabled(), companionNativeAvailable_,
                                     companionPaused_)
        .traceSummaries;
}

bool DesktopShellViewModel::developerModeEnabled() const {
    return settings_.developerModeEnabled();
}

void DesktopShellViewModel::setDeveloperModeEnabled(bool enabled) {
    settings_.setDeveloperModeEnabled(enabled);
}

QString DesktopShellViewModel::updateCheckPolicy() const {
    return settings_.updateCheckPolicy();
}

void DesktopShellViewModel::setUpdateCheckPolicy(const QString& policy) {
    settings_.setUpdateCheckPolicy(policy);
}

QString DesktopShellViewModel::notificationPolicy() const {
    return settings_.notificationPolicy();
}

void DesktopShellViewModel::setNotificationPolicy(const QString& policy) {
    settings_.setNotificationPolicy(policy);
}

bool DesktopShellViewModel::onboardingComplete() const {
    return settings_.onboardingComplete();
}

void DesktopShellViewModel::setOnboardingComplete(bool complete) {
    settings_.setOnboardingComplete(complete);
}

QString DesktopShellViewModel::onboardingUseCase() const {
    return settings_.onboardingUseCase();
}

void DesktopShellViewModel::setOnboardingUseCase(const QString& useCase) {
    settings_.setOnboardingUseCase(useCase);
}

QString DesktopShellViewModel::onboardingAiProvider() const {
    return settings_.onboardingAiProvider();
}

void DesktopShellViewModel::setOnboardingAiProvider(const QString& provider) {
    settings_.setOnboardingAiProvider(provider);
}

QString DesktopShellViewModel::recoveryDraftText() const {
    return settings_.recoveryDraftText();
}

void DesktopShellViewModel::setRecoveryDraftText(const QString& text) {
    settings_.setRecoveryDraftText(text);
}

bool DesktopShellViewModel::reducedMotionEnabled() const {
    return settings_.reducedMotionEnabled();
}

void DesktopShellViewModel::setReducedMotionEnabled(bool enabled) {
    settings_.setReducedMotionEnabled(enabled);
}

bool DesktopShellViewModel::highContrastEnabled() const {
    return settings_.highContrastEnabled();
}

void DesktopShellViewModel::setHighContrastEnabled(bool enabled) {
    settings_.setHighContrastEnabled(enabled);
}

QString DesktopShellViewModel::uiDensity() const {
    return settings_.uiDensity();
}

void DesktopShellViewModel::setUiDensity(const QString& density) {
    settings_.setUiDensity(density);
}

QStringList DesktopShellViewModel::activityTimelineSummaries() const {
    QStringList lines{
        QStringLiteral("Today - Chat Created - %1").arg(conversationListCurrentTitle()),
        QStringLiteral("Today - Export Completed - %1").arg(conversationExportLastStatus()),
        QStringLiteral("Today - Brain Memory Saved - %1 entries").arg(memoryEntryCount()),
        QStringLiteral("Yesterday - Theme Changed - %1").arg(themeName()),
        QStringLiteral("Yesterday - Settings Modified - local preferences only"),
    };
    return lines;
}

QStringList DesktopShellViewModel::notificationCenterSummaries() const {
    return notificationFilteredSummaries();
}

QStringList DesktopShellViewModel::notificationCategories() const {
    return {QStringLiteral("All"),      QStringLiteral("Tasks"),   QStringLiteral("Models"),
            QStringLiteral("Updates"),  QStringLiteral("Brain"),   QStringLiteral("Workspace"),
            QStringLiteral("Security")};
}

QStringList DesktopShellViewModel::notificationLifecycleSummaries() const {
    QStringList summaries;
    int pinned = 0;
    int unread = 0;
    int archived = 0;
    const auto notifications = notificationsFromJson(settings_.notificationCenterJson());
    for (const auto& value : notifications) {
        const auto item = value.toObject();
        pinned += item.value(QStringLiteral("pinned")).toBool() ? 1 : 0;
        unread += item.value(QStringLiteral("read")).toBool() ? 0 : 1;
        archived += item.value(QStringLiteral("archived")).toBool() ? 1 : 0;
    }
    summaries << QStringLiteral("Pinned: %1").arg(pinned)
              << QStringLiteral("Unread: %1").arg(unread)
              << QStringLiteral("Archived: %1").arg(archived)
              << QStringLiteral("Policy: %1").arg(settings_.notificationPolicy())
              << QStringLiteral("Persistence: local settings JSON");
    return summaries;
}

QStringList DesktopShellViewModel::notificationFilteredSummaries() const {
    QStringList summaries;
    const auto query = notificationSearchQuery_.trimmed().toLower();
    const auto filter = notificationCategoryFilter_;
    const auto notifications = notificationsFromJson(settings_.notificationCenterJson());
    for (const auto& value : notifications) {
        const auto item = value.toObject();
        const auto category = item.value(QStringLiteral("category")).toString();
        const auto line = notificationSummary(item);
        if (filter != QStringLiteral("All") && category != filter) {
            continue;
        }
        if (!query.isEmpty() && !line.toLower().contains(query) &&
            !item.value(QStringLiteral("body")).toString().toLower().contains(query)) {
            continue;
        }
        summaries.append(line);
    }
    return summaries.isEmpty() ? QStringList{QStringLiteral("No notifications match the filter.")}
                               : summaries;
}

QString DesktopShellViewModel::notificationSearchQuery() const {
    return notificationSearchQuery_;
}

void DesktopShellViewModel::setNotificationSearchQuery(const QString& query) {
    if (query == notificationSearchQuery_) {
        return;
    }
    notificationSearchQuery_ = query;
    emit nativeExperienceChanged();
}

QString DesktopShellViewModel::notificationCategoryFilter() const {
    return notificationCategoryFilter_;
}

void DesktopShellViewModel::setNotificationCategoryFilter(const QString& category) {
    const auto selected = notificationCategories().contains(category) ? category : QStringLiteral("All");
    if (selected == notificationCategoryFilter_) {
        return;
    }
    notificationCategoryFilter_ = selected;
    emit nativeExperienceChanged();
}

QString DesktopShellViewModel::updateWorkflowState() const {
    return settings_.updateWorkflowState();
}

QStringList DesktopShellViewModel::releaseNotesSummaries() const {
    return {
        QStringLiteral("Sentinel %1 - Phase 52 packaging and 1.0 RC readiness")
            .arg(core::AppMetadata::version()),
        QStringLiteral("Packaging metadata, release presets, QA plan, and release checklist are prepared without adding runtime authority."),
        QStringLiteral("Manual update check is explicit; no background polling, silent download, or automatic install exists."),
    };
}

QStringList DesktopShellViewModel::aboutSentinelSummaries() const {
    auto summaries = core::AppMetadata::safeBuildSummaries();
    summaries.append(QStringLiteral("Qt: %1").arg(QString::fromLatin1(qVersion())));
    summaries.append(QStringLiteral("Copyright: %1").arg(core::AppMetadata::copyright()));
    summaries.append(QStringLiteral("License: project license and third-party notices in local docs"));
    summaries.append(QStringLiteral("Docs: docs/PRIVACY.md, docs/SECURITY.md, docs/UPDATES.md, docs/DIAGNOSTICS.md"));
    return summaries;
}

QStringList DesktopShellViewModel::accessibilitySummaries() const {
    return {
        QStringLiteral("Keyboard navigation: enabled across shell, settings, command palette, and dialogs"),
        QStringLiteral("Focus indicators: tokenized focus borders on interactive controls"),
        QStringLiteral("Reduced motion: %1").arg(settings_.reducedMotionEnabled() ? QStringLiteral("Enabled") : QStringLiteral("Disabled")),
        QStringLiteral("High contrast: %1").arg(settings_.highContrastEnabled() ? QStringLiteral("Enabled") : QStringLiteral("Disabled")),
        QStringLiteral("UI density: %1").arg(settings_.uiDensity()),
        QStringLiteral("Screen reader labels: primary controls expose descriptive labels in QML"),
    };
}

QStringList DesktopShellViewModel::diagnosticsCenterSummaries() const {
    auto summaries = core::AppMetadata::safeBuildSummaries();
    summaries.append({
        QStringLiteral("Active provider: %1").arg(controller_.activeRuntimeProviderLabel()),
        QStringLiteral("Active model: %1").arg(controller_.activeRuntimeModelLabel()),
        QStringLiteral("Workspace count: %1").arg(workspaceIds().size()),
        QStringLiteral("Brain entries: %1").arg(memoryEntryCount()),
        QStringLiteral("Task statistics: %1").arg(controlledTaskDiagnosticsSummary()),
        QStringLiteral("Notification statistics: %1").arg(notificationLifecycleSummaries().join(QStringLiteral(" / "))),
        QStringLiteral("Logs: no automatic log collection or upload"),
    });
    return summaries;
}

QStringList DesktopShellViewModel::exportPreviewSummaries() const {
    return {
        QStringLiteral("Source: %1").arg(exportPreviewSource_),
        QStringLiteral("Format: %1").arg(exportPreviewFormat_),
        QStringLiteral("Preview: conversations, Brain entries, task reports, and workspace summaries support Markdown, TXT, JSON, and PDF."),
        QStringLiteral("Output: App-controlled export directory"),
        QStringLiteral("Privacy: export is foreground-only and user initiated"),
    };
}

QStringList DesktopShellViewModel::brainInsightSummaries() const {
    return {
        QStringLiteral("Recent activity: %1").arg(activityTimelineSummaries().value(0)),
        QStringLiteral("Workspace activity: %1").arg(brainWorkspaceSummaries().value(0)),
        QStringLiteral("Task completion trends: %1").arg(controlledTaskDiagnosticsSummary()),
        QStringLiteral("Model usage trends: active %1 / %2").arg(controller_.activeRuntimeProviderLabel(), controller_.activeRuntimeModelLabel()),
        QStringLiteral("Visualization only: no analytics upload"),
    };
}

QStringList DesktopShellViewModel::recoveryReliabilitySummaries() const {
    return {
        QStringLiteral("Crash recovery draft: %1").arg(settings_.recoveryDraftText().isEmpty() ? QStringLiteral("No draft saved") : QStringLiteral("Draft available")),
        QStringLiteral("Recovery notifications: local in-app notices only"),
        QStringLiteral("Unsaved work detection: composer drafts can be restored or discarded explicitly"),
        QStringLiteral("Safe shutdown summary: chats, Brain, settings, tasks, notifications, and Local RAG remain separate local stores"),
    };
}

QStringList DesktopShellViewModel::productPolishSummaries() const {
    return {
        QStringLiteral("Home remains chat-first"),
        QStringLiteral("Conversation sidebar remains collapsible"),
        QStringLiteral("Smooth transitions respect reduced-motion preference"),
        QStringLiteral("Consistent spacing, responsive layouts, compact mode, large mode, empty, loading, and error states are surfaced through shared QML components"),
    };
}

QString DesktopShellViewModel::selectedSkillProfile() const {
    return skillProfileService_.normalizedProfileId(settings_.selectedSkillProfile());
}

void DesktopShellViewModel::setSelectedSkillProfile(const QString& profileId) {
    settings_.setSelectedSkillProfile(skillProfileService_.normalizedProfileId(profileId));
}

QString DesktopShellViewModel::selectedSkillProfileName() const {
    return skillProfileService_.selectedProfile(settings_.selectedSkillProfile()).name;
}

QString DesktopShellViewModel::selectedSkillProfileSummary() const {
    return skillProfileService_.selectedProfile(settings_.selectedSkillProfile()).summary;
}

QString DesktopShellViewModel::selectedSkillProfileDescription() const {
    return skillProfileService_.selectedProfile(settings_.selectedSkillProfile()).description;
}

QString DesktopShellViewModel::selectedSkillProfileReadiness() const {
    return skillProfileService_.readiness(settings_.selectedSkillProfile()).status;
}

QString DesktopShellViewModel::selectedSkillProfilePolicyPosture() const {
    return skillProfileService_.selectedProfile(settings_.selectedSkillProfile()).policyPosture;
}

QStringList DesktopShellViewModel::skillProfileIds() const {
    QStringList ids;
    for (const auto& profile : skillProfileService_.availableProfiles()) {
        ids.append(profile.id);
    }
    return ids;
}

QStringList DesktopShellViewModel::skillProfileNames() const {
    QStringList names;
    for (const auto& profile : skillProfileService_.availableProfiles()) {
        names.append(profile.name);
    }
    return names;
}

QStringList DesktopShellViewModel::skillProfileSummaries() const {
    return skillProfileService_.profileSummaries();
}

QStringList DesktopShellViewModel::skillProfileCapabilitySummaries() const {
    return skillProfileService_.selectedProfile(settings_.selectedSkillProfile()).capabilitySummaries;
}

QStringList DesktopShellViewModel::skillProfileReadinessChecks() const {
    return skillProfileService_.readiness(settings_.selectedSkillProfile()).checks;
}

QStringList DesktopShellViewModel::skillProfileDeveloperDiagnostics() const {
    return skillProfileService_.readiness(settings_.selectedSkillProfile()).developerDiagnostics;
}

QString DesktopShellViewModel::selectedWorkspaceId() const {
    return workspaceService_.normalizedWorkspaceId(settings_.selectedWorkspaceId(),
                                                   settings_.workspaceCatalogJson());
}

void DesktopShellViewModel::setSelectedWorkspaceId(const QString& workspaceId) {
    settings_.setSelectedWorkspaceId(
        workspaceService_.normalizedWorkspaceId(workspaceId, settings_.workspaceCatalogJson()));
}

QString DesktopShellViewModel::selectedWorkspaceName() const {
    return workspaceService_
        .selectedWorkspace(settings_.selectedWorkspaceId(), settings_.workspaceCatalogJson())
        .name;
}

QString DesktopShellViewModel::selectedWorkspaceAccessState() const {
    return workspaceService_
        .selectedWorkspace(settings_.selectedWorkspaceId(), settings_.workspaceCatalogJson())
        .accessState;
}

QString DesktopShellViewModel::workspacePermissionPosture() const {
    return workspaceService_
        .selectedWorkspace(settings_.selectedWorkspaceId(), settings_.workspaceCatalogJson())
        .permissionPosture;
}

QString DesktopShellViewModel::selectedWorkspaceRootSummary() const {
    return workspaceService_
        .selectedWorkspace(settings_.selectedWorkspaceId(), settings_.workspaceCatalogJson())
        .rootSummary;
}

QString DesktopShellViewModel::workspaceReadinessStatus() const {
    return workspaceService_.readiness(settings_.selectedWorkspaceId(), settings_.workspaceCatalogJson()).status;
}

QString DesktopShellViewModel::workspaceReadinessSummary() const {
    return workspaceService_.readiness(settings_.selectedWorkspaceId(), settings_.workspaceCatalogJson()).summary;
}

QString DesktopShellViewModel::workspacePermissionSummary() const {
    return workspaceService_
        .selectedWorkspace(settings_.selectedWorkspaceId(), settings_.workspaceCatalogJson())
        .permissionSummary;
}

QStringList DesktopShellViewModel::workspaceIds() const {
    QStringList ids;
    for (const auto& workspace : workspaceService_.availableWorkspaces(settings_.workspaceCatalogJson())) {
        ids.append(workspace.id);
    }
    return ids;
}

QStringList DesktopShellViewModel::workspaceNames() const {
    QStringList names;
    for (const auto& workspace : workspaceService_.availableWorkspaces(settings_.workspaceCatalogJson())) {
        names.append(workspace.name);
    }
    return names;
}

QStringList DesktopShellViewModel::workspaceSummaries() const {
    return workspaceService_.workspaceSummaries(settings_.workspaceCatalogJson());
}

QStringList DesktopShellViewModel::workspacePermissionPostures() const {
    return workspaceService_.permissionPostures();
}

QStringList DesktopShellViewModel::workspaceActionPlaceholders() const {
    return workspaceService_.actionPlaceholders();
}

QStringList DesktopShellViewModel::workspaceReadinessChecks() const {
    return workspaceService_.readiness(settings_.selectedWorkspaceId(), settings_.workspaceCatalogJson()).checks;
}

QStringList DesktopShellViewModel::workspaceBoundaryDiagnostics() const {
    return workspaceService_.readiness(settings_.selectedWorkspaceId(), settings_.workspaceCatalogJson()).boundaryDiagnostics;
}

QStringList DesktopShellViewModel::workspaceTemplateNames() const {
    return workspaceService_.builtInTemplateNames();
}

QString DesktopShellViewModel::workspaceLastActionStatus() const {
    return workspaceLastActionStatus_;
}

QString DesktopShellViewModel::workspaceLastActionSummary() const {
    return workspaceLastActionSummary_;
}

QStringList DesktopShellViewModel::attachmentSummaries() const {
    QStringList summaries;
    for (const auto& attachment : attachments_) {
        summaries.append(core::ragDocumentSummary(attachment));
    }
    return summaries;
}

QString DesktopShellViewModel::attachmentStatus() const {
    return attachments_.isEmpty() ? QStringLiteral("No Attachments")
                                  : QStringLiteral("%1 attachment(s)").arg(attachments_.size());
}

QString DesktopShellViewModel::attachmentPreviewSummary() const {
    if (attachments_.isEmpty()) {
        return QStringLiteral("No file attached. Drag, browse, or paste explicitly to attach.");
    }
    return core::ragDocumentSummary(attachments_.last());
}

QStringList DesktopShellViewModel::fileChatActionSummaries() const {
    return {
        QStringLiteral("Summarize attached file"),
        QStringLiteral("Ask questions about attached file"),
        QStringLiteral("Extract key points"),
        QStringLiteral("Translate selected attachment"),
        QStringLiteral("Export file-chat results"),
    };
}

bool DesktopShellViewModel::localKnowledgeBaseEnabled() const {
    return settings_.localKnowledgeBaseEnabled();
}

void DesktopShellViewModel::setLocalKnowledgeBaseEnabled(bool enabled) {
    settings_.setLocalKnowledgeBaseEnabled(enabled);
}

QString DesktopShellViewModel::localKnowledgeBaseStatus() const {
    if (!settings_.localKnowledgeBaseEnabled()) {
        return QStringLiteral("Disabled");
    }
    return localRagStore_ && localRagStore_->isAvailable() ? QStringLiteral("Enabled / Manual Only")
                                                           : QStringLiteral("Unavailable");
}

QStringList DesktopShellViewModel::knowledgeBaseDocumentSummaries() const {
    if (!localRagStore_) {
        return {};
    }
    QStringList summaries;
    for (const auto& document : localRagStore_->documents(selectedWorkspaceId())) {
        summaries.append(core::ragDocumentSummary(document));
    }
    return summaries;
}

QStringList DesktopShellViewModel::recentRetrievalSummaries() const {
    if (!localRagStore_) {
        return {};
    }
    QStringList summaries;
    for (const auto& retrieval : localRagStore_->recentRetrievals(selectedWorkspaceId())) {
        summaries.append(core::ragRetrievalSummary(retrieval));
    }
    return summaries;
}

QStringList DesktopShellViewModel::retrievalExplainabilitySummaries() const {
    if (!settings_.retrievalExplainabilityEnabled()) {
        return {QStringLiteral("Retrieval explainability disabled by user setting.")};
    }
    return {
        QStringLiteral("Source document: shown for each retrieval result"),
        QStringLiteral("Section/chunk reference: shown as manual chunk metadata"),
        QStringLiteral("Relevance metadata: shown as bounded local score metadata"),
    };
}

QStringList DesktopShellViewModel::brainWorkspaceSummaries() const {
    return {
        QStringLiteral("Workspace Timeline - %1 / %2").arg(selectedWorkspaceName(),
                                                           workspaceLastActionSummary_),
        QStringLiteral("Attached Documents - %1").arg(attachmentStatus()),
        QStringLiteral("Knowledge Base Summary - %1 / %2 document(s)")
            .arg(localKnowledgeBaseStatus())
            .arg(knowledgeBaseDocumentSummaries().size()),
        QStringLiteral("Recent Retrievals - %1 record(s)").arg(recentRetrievalSummaries().size()),
        QStringLiteral("Planned Tasks - %1").arg(
            controlledAgentTaskService_
                .timelineSummaries(controlledAgentTaskService_.tasksFromJson(
                                       settings_.controlledAgentTasksJson()),
                                   selectedWorkspaceId(), QStringLiteral("pending approval"))
                .size()),
        QStringLiteral("Completed Tasks - %1").arg(
            controlledAgentTaskService_
                .timelineSummaries(controlledAgentTaskService_.tasksFromJson(
                                       settings_.controlledAgentTasksJson()),
                                   selectedWorkspaceId(), QStringLiteral("completed"))
                .size()),
        QStringLiteral("Failed Tasks - %1").arg(
            controlledAgentTaskService_
                .timelineSummaries(controlledAgentTaskService_.tasksFromJson(
                                       settings_.controlledAgentTasksJson()),
                                   selectedWorkspaceId(), QStringLiteral("failed"))
                .size()),
        QStringLiteral("Cancelled Tasks - %1").arg(
            controlledAgentTaskService_
                .timelineSummaries(controlledAgentTaskService_.tasksFromJson(
                                       settings_.controlledAgentTasksJson()),
                                   selectedWorkspaceId(), QStringLiteral("cancelled"))
                .size()),
    };
}

QStringList DesktopShellViewModel::exportCenterSummaries() const {
    auto summaries = QStringList{
        QStringLiteral("Chats - Markdown, PDF, TXT, DOCX, JSON"),
        QStringLiteral("Workspace summaries - %1 default").arg(settings_.exportDefaultFormat()),
        QStringLiteral("Document summaries - citations %1")
            .arg(settings_.exportIncludeCitations() ? QStringLiteral("included")
                                                    : QStringLiteral("excluded")),
        QStringLiteral("Retrieval reports - timestamps %1 / anonymize names %2 / model metadata %3")
            .arg(settings_.exportIncludeTimestamps() ? QStringLiteral("included")
                                                     : QStringLiteral("excluded"),
                 settings_.exportAnonymizeNames() ? QStringLiteral("on") : QStringLiteral("off"),
                 settings_.exportIncludeModelMetadata() ? QStringLiteral("included")
                                                        : QStringLiteral("excluded")),
    };
    summaries.append(controlledAgentTaskService_.exportCenterSummaries());
    return summaries;
}

QStringList DesktopShellViewModel::privacyCenterSummaries() const {
    return {
        QStringLiteral("Knowledge Base: %1")
            .arg(settings_.localKnowledgeBaseEnabled() ? QStringLiteral("Enabled")
                                                       : QStringLiteral("Disabled")),
        QStringLiteral("Indexing: Manual Only"),
        QStringLiteral("Document Scope: Workspace Only"),
        QStringLiteral("Cloud Retrieval: Disabled"),
        QStringLiteral("Telemetry: Disabled"),
        QStringLiteral("Hidden filesystem scanning: Disabled"),
        QStringLiteral("Autonomous agents: Disabled"),
        QStringLiteral("Hidden task execution: Disabled"),
        QStringLiteral("Cloud activation from tasks: Disabled"),
    };
}

QString DesktopShellViewModel::createWorkspace(const QString& name, const QString& templateName) {
    const auto result =
        workspaceService_.createWorkspace(settings_.workspaceCatalogJson(), name, templateName);
    workspaceLastActionStatus_ = result.status;
    workspaceLastActionSummary_ = result.summary;
    if (result.success) {
        settings_.setWorkspaceCatalogJson(result.catalogJson);
        settings_.setSelectedWorkspaceId(result.selectedWorkspaceId);
    }
    emit workspaceChanged();
    return result.success ? result.selectedWorkspaceId : QString();
}

bool DesktopShellViewModel::renameWorkspace(const QString& workspaceId, const QString& name) {
    const auto result =
        workspaceService_.renameWorkspace(settings_.workspaceCatalogJson(), workspaceId, name);
    workspaceLastActionStatus_ = result.status;
    workspaceLastActionSummary_ = result.summary;
    if (result.success) {
        settings_.setWorkspaceCatalogJson(result.catalogJson);
    }
    emit workspaceChanged();
    return result.success;
}

bool DesktopShellViewModel::archiveWorkspace(const QString& workspaceId) {
    const auto result =
        workspaceService_.archiveWorkspace(settings_.workspaceCatalogJson(), workspaceId);
    workspaceLastActionStatus_ = result.status;
    workspaceLastActionSummary_ = result.summary;
    if (result.success) {
        settings_.setWorkspaceCatalogJson(result.catalogJson);
    }
    emit workspaceChanged();
    return result.success;
}

bool DesktopShellViewModel::deleteWorkspace(const QString& workspaceId) {
    const auto result = workspaceService_.deleteWorkspace(
        settings_.workspaceCatalogJson(), workspaceId, settings_.selectedWorkspaceId());
    workspaceLastActionStatus_ = result.status;
    workspaceLastActionSummary_ = result.summary;
    if (result.success) {
        settings_.setWorkspaceCatalogJson(result.catalogJson);
        settings_.setSelectedWorkspaceId(result.selectedWorkspaceId);
    }
    emit workspaceChanged();
    return result.success;
}

QString DesktopShellViewModel::duplicateWorkspace(const QString& workspaceId) {
    const auto result =
        workspaceService_.duplicateWorkspace(settings_.workspaceCatalogJson(), workspaceId);
    workspaceLastActionStatus_ = result.status;
    workspaceLastActionSummary_ = result.summary;
    if (result.success) {
        settings_.setWorkspaceCatalogJson(result.catalogJson);
        settings_.setSelectedWorkspaceId(result.selectedWorkspaceId);
    }
    emit workspaceChanged();
    return result.success ? result.selectedWorkspaceId : QString();
}

bool DesktopShellViewModel::attachFileToChat(const QString& filePath) {
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        workspaceLastActionStatus_ = QStringLiteral("Refused");
        workspaceLastActionSummary_ = QStringLiteral("Attachment requires one explicit file.");
        emit workspaceChanged();
        return false;
    }
    const auto type = fileTypeForPath(filePath);
    if (!supportedAttachmentType(type)) {
        workspaceLastActionStatus_ = QStringLiteral("Refused");
        workspaceLastActionSummary_ = QStringLiteral("Unsupported attachment type.");
        emit workspaceChanged();
        return false;
    }
    attachments_.append({attachmentId(selectedWorkspaceId(), info.fileName(), info.size()),
                         selectedWorkspaceId(), info.fileName(), type, info.size(),
                         QStringLiteral("Attached Explicitly"),
                         QStringLiteral("Explicit user attachment; no folder import.")});
    workspaceLastActionStatus_ = QStringLiteral("Document Attached");
    workspaceLastActionSummary_ = QStringLiteral("Attached %1.").arg(info.fileName());
    emit attachmentChanged();
    emit workspaceChanged();
    return true;
}

bool DesktopShellViewModel::pasteAttachment(const QString& name, const QString& text) {
    const auto fileName = name.trimmed().isEmpty() ? QStringLiteral("pasted-attachment.txt")
                                                   : name.trimmed();
    const auto type = fileTypeForPath(fileName);
    if (!supportedAttachmentType(type) || text.trimmed().isEmpty()) {
        workspaceLastActionStatus_ = QStringLiteral("Refused");
        workspaceLastActionSummary_ = QStringLiteral("Paste attachment requires supported name and text.");
        emit workspaceChanged();
        return false;
    }
    attachments_.append({attachmentId(selectedWorkspaceId(), fileName, text.size()),
                         selectedWorkspaceId(), fileName, type, text.toUtf8().size(),
                         QStringLiteral("Pasted Explicitly"),
                         QStringLiteral("Explicit paste attachment; no background processing.")});
    workspaceLastActionStatus_ = QStringLiteral("Document Attached");
    workspaceLastActionSummary_ = QStringLiteral("Pasted %1.").arg(fileName);
    emit attachmentChanged();
    emit workspaceChanged();
    return true;
}

bool DesktopShellViewModel::removeAttachment(const QString& attachmentIdValue) {
    for (qsizetype index = 0; index < attachments_.size(); ++index) {
        if (attachments_.at(index).id == attachmentIdValue.trimmed()) {
            attachments_.removeAt(index);
            workspaceLastActionStatus_ = QStringLiteral("Attachment Removed");
            workspaceLastActionSummary_ = QStringLiteral("Removed attachment.");
            emit attachmentChanged();
            emit workspaceChanged();
            return true;
        }
    }
    return false;
}

bool DesktopShellViewModel::replaceAttachment(const QString& attachmentIdValue,
                                              const QString& filePath) {
    if (!removeAttachment(attachmentIdValue)) {
        return false;
    }
    return attachFileToChat(filePath);
}

bool DesktopShellViewModel::addKnowledgeBaseDocument(const QString& filePath) {
    if (!settings_.localKnowledgeBaseEnabled() || !localRagStore_) {
        workspaceLastActionStatus_ = QStringLiteral("Refused");
        workspaceLastActionSummary_ = QStringLiteral("Enable the workspace knowledge base first.");
        emit workspaceChanged();
        return false;
    }
    const QFileInfo info(filePath);
    const auto type = fileTypeForPath(filePath);
    if (!info.exists() || !info.isFile() || !supportedAttachmentType(type)) {
        workspaceLastActionStatus_ = QStringLiteral("Indexing Failed");
        workspaceLastActionSummary_ = QStringLiteral("Knowledge base accepts explicit supported files only.");
        emit workspaceChanged();
        return false;
    }
    const auto result = localRagStore_->addDocument(
        {attachmentId(selectedWorkspaceId(), info.fileName(), info.size()), selectedWorkspaceId(),
         info.fileName(), type, info.size(), QStringLiteral("Added / Not Indexed"),
         QStringLiteral("Workspace-only explicit document.")});
    workspaceLastActionStatus_ = result.status;
    workspaceLastActionSummary_ = result.summary;
    emit workspaceChanged();
    return result.success;
}

bool DesktopShellViewModel::removeKnowledgeBaseDocument(const QString& documentId) {
    if (!localRagStore_) {
        return false;
    }
    const auto result = localRagStore_->removeDocument(selectedWorkspaceId(), documentId);
    workspaceLastActionStatus_ = result.status;
    workspaceLastActionSummary_ = result.summary;
    emit workspaceChanged();
    return result.success;
}

bool DesktopShellViewModel::reindexKnowledgeBase() {
    if (!settings_.localKnowledgeBaseEnabled() || !localRagStore_) {
        workspaceLastActionStatus_ = QStringLiteral("Refused");
        workspaceLastActionSummary_ = QStringLiteral("Manual re-index requires an enabled knowledge base.");
        emit workspaceChanged();
        return false;
    }
    const auto result = localRagStore_->markReindexed(selectedWorkspaceId());
    workspaceLastActionStatus_ = result.status;
    workspaceLastActionSummary_ = result.summary;
    emit workspaceChanged();
    return result.success;
}

bool DesktopShellViewModel::clearKnowledgeBase() {
    if (!localRagStore_) {
        return false;
    }
    const auto result = localRagStore_->clearWorkspace(selectedWorkspaceId());
    workspaceLastActionStatus_ = result.status;
    workspaceLastActionSummary_ = result.summary;
    emit workspaceChanged();
    return result.success;
}

QString DesktopShellViewModel::defaultPermissionPolicyState() const {
    return permissionPolicyService_.normalizedState(settings_.defaultPermissionPolicyState());
}

void DesktopShellViewModel::setDefaultPermissionPolicyState(const QString& state) {
    settings_.setDefaultPermissionPolicyState(permissionPolicyService_.normalizedState(state));
}

QString DesktopShellViewModel::permissionPolicyStatus() const {
    return permissionPolicyService_.registrySummary(settings_.defaultPermissionPolicyState()).status;
}

QString DesktopShellViewModel::permissionPolicySummary() const {
    return permissionPolicyService_.registrySummary(settings_.defaultPermissionPolicyState()).summary;
}

QStringList DesktopShellViewModel::permissionPolicyStateLabels() const {
    return permissionPolicyService_.permissionStateLabels();
}

QStringList DesktopShellViewModel::permissionPolicyDomainIds() const {
    return permissionPolicyService_.permissionDomainIds();
}

QStringList DesktopShellViewModel::permissionPolicyDomainNames() const {
    return permissionPolicyService_.permissionDomainNames();
}

QStringList DesktopShellViewModel::permissionPolicyDomainSummaries() const {
    return permissionPolicyService_.registrySummary(settings_.defaultPermissionPolicyState())
        .domainSummaries;
}

QStringList DesktopShellViewModel::permissionPolicyDeveloperDiagnostics() const {
    return permissionPolicyService_.registrySummary(settings_.defaultPermissionPolicyState())
        .developerDiagnostics;
}

QString DesktopShellViewModel::toolGatewayStatus() const {
    return toolExecutionGateway_
        .registrySummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_)
        .status;
}

QString DesktopShellViewModel::toolGatewaySummary() const {
    return toolExecutionGateway_
        .registrySummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_)
        .summary;
}

QString DesktopShellViewModel::toolGatewayPermissionPosture() const {
    return toolExecutionGateway_
        .registrySummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_)
        .permissionPosture;
}

int DesktopShellViewModel::toolGatewayToolCount() const {
    return toolExecutionGateway_
        .registrySummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_)
        .toolCount;
}

int DesktopShellViewModel::toolGatewayMetadataSafeCount() const {
    return toolExecutionGateway_
        .registrySummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_)
        .metadataSafeCount;
}

int DesktopShellViewModel::toolGatewayUnavailableCount() const {
    return toolExecutionGateway_
        .registrySummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_)
        .unavailableCount;
}

int DesktopShellViewModel::toolGatewayRefusedCount() const {
    return toolExecutionGateway_
        .registrySummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_)
        .refusedCount;
}

QStringList DesktopShellViewModel::toolGatewayToolSummaries() const {
    return toolExecutionGateway_
        .registrySummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_)
        .toolSummaries;
}

QStringList DesktopShellViewModel::toolGatewayDeveloperDiagnostics() const {
    return toolExecutionGateway_
        .registrySummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_)
        .developerDiagnostics;
}

QString DesktopShellViewModel::agentRuntimeStatus() const {
    return agentRuntimeService_
        .runtimeSummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_,
                        toolExecutionGateway_, skillProfileService_,
                        settings_.selectedSkillProfile(), workspaceService_,
                        settings_.selectedWorkspaceId())
        .status;
}

QString DesktopShellViewModel::agentRuntimeSummary() const {
    return agentRuntimeService_
        .runtimeSummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_,
                        toolExecutionGateway_, skillProfileService_,
                        settings_.selectedSkillProfile(), workspaceService_,
                        settings_.selectedWorkspaceId())
        .summary;
}

QString DesktopShellViewModel::agentRuntimeApprovalPosture() const {
    return agentRuntimeService_
        .runtimeSummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_,
                        toolExecutionGateway_, skillProfileService_,
                        settings_.selectedSkillProfile(), workspaceService_,
                        settings_.selectedWorkspaceId())
        .approvalPosture;
}

int DesktopShellViewModel::agentRuntimeAgentCount() const {
    return agentRuntimeService_
        .runtimeSummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_,
                        toolExecutionGateway_, skillProfileService_,
                        settings_.selectedSkillProfile(), workspaceService_,
                        settings_.selectedWorkspaceId())
        .agentCount;
}

int DesktopShellViewModel::agentRuntimeReadyAgentCount() const {
    return agentRuntimeService_
        .runtimeSummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_,
                        toolExecutionGateway_, skillProfileService_,
                        settings_.selectedSkillProfile(), workspaceService_,
                        settings_.selectedWorkspaceId())
        .readyAgentCount;
}

int DesktopShellViewModel::agentRuntimeRefusedAgentCount() const {
    return agentRuntimeService_
        .runtimeSummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_,
                        toolExecutionGateway_, skillProfileService_,
                        settings_.selectedSkillProfile(), workspaceService_,
                        settings_.selectedWorkspaceId())
        .refusedAgentCount;
}

QStringList DesktopShellViewModel::agentRuntimeAgentSummaries() const {
    return agentRuntimeService_
        .runtimeSummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_,
                        toolExecutionGateway_, skillProfileService_,
                        settings_.selectedSkillProfile(), workspaceService_,
                        settings_.selectedWorkspaceId())
        .agentSummaries;
}

QStringList DesktopShellViewModel::agentRuntimeReadinessSummaries() const {
    return agentRuntimeService_
        .runtimeSummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_,
                        toolExecutionGateway_, skillProfileService_,
                        settings_.selectedSkillProfile(), workspaceService_,
                        settings_.selectedWorkspaceId())
        .readinessSummaries;
}

QStringList DesktopShellViewModel::agentRuntimeDeveloperDiagnostics() const {
    return agentRuntimeService_
        .runtimeSummary(settings_.defaultPermissionPolicyState(), permissionPolicyService_,
                        toolExecutionGateway_, skillProfileService_,
                        settings_.selectedSkillProfile(), workspaceService_,
                        settings_.selectedWorkspaceId())
        .developerDiagnostics;
}

QString DesktopShellViewModel::agentPlanId() const {
    return agentRuntimeService_
        .previewPlan(QStringLiteral("Review the current goal and propose safe next steps."),
                     QStringLiteral("general-assistant"), settings_.defaultPermissionPolicyState(),
                     permissionPolicyService_, toolExecutionGateway_, skillProfileService_,
                     settings_.selectedSkillProfile(), workspaceService_,
                     settings_.selectedWorkspaceId())
        .planId;
}

QString DesktopShellViewModel::agentPlanGoalSummary() const {
    return agentRuntimeService_
        .previewPlan(QStringLiteral("Review the current goal and propose safe next steps."),
                     QStringLiteral("general-assistant"), settings_.defaultPermissionPolicyState(),
                     permissionPolicyService_, toolExecutionGateway_, skillProfileService_,
                     settings_.selectedSkillProfile(), workspaceService_,
                     settings_.selectedWorkspaceId())
        .goalSummary;
}

QStringList DesktopShellViewModel::agentPlanSteps() const {
    return agentRuntimeService_
        .previewPlan(QStringLiteral("Review the current goal and propose safe next steps."),
                     QStringLiteral("general-assistant"), settings_.defaultPermissionPolicyState(),
                     permissionPolicyService_, toolExecutionGateway_, skillProfileService_,
                     settings_.selectedSkillProfile(), workspaceService_,
                     settings_.selectedWorkspaceId())
        .orderedPlanSteps;
}

QStringList DesktopShellViewModel::agentPlanRequiredTools() const {
    return agentRuntimeService_
        .previewPlan(QStringLiteral("Review the current goal and propose safe next steps."),
                     QStringLiteral("general-assistant"), settings_.defaultPermissionPolicyState(),
                     permissionPolicyService_, toolExecutionGateway_, skillProfileService_,
                     settings_.selectedSkillProfile(), workspaceService_,
                     settings_.selectedWorkspaceId())
        .requiredTools;
}

QStringList DesktopShellViewModel::agentPlanRequiredPermissions() const {
    return agentRuntimeService_
        .previewPlan(QStringLiteral("Review the current goal and propose safe next steps."),
                     QStringLiteral("general-assistant"), settings_.defaultPermissionPolicyState(),
                     permissionPolicyService_, toolExecutionGateway_, skillProfileService_,
                     settings_.selectedSkillProfile(), workspaceService_,
                     settings_.selectedWorkspaceId())
        .requiredPermissions;
}

QString DesktopShellViewModel::agentPlanEstimatedRisk() const {
    return agentRuntimeService_
        .previewPlan(QStringLiteral("Review the current goal and propose safe next steps."),
                     QStringLiteral("general-assistant"), settings_.defaultPermissionPolicyState(),
                     permissionPolicyService_, toolExecutionGateway_, skillProfileService_,
                     settings_.selectedSkillProfile(), workspaceService_,
                     settings_.selectedWorkspaceId())
        .estimatedRisk;
}

QString DesktopShellViewModel::agentPlanApprovalState() const {
    return agentRuntimeService_
        .previewPlan(QStringLiteral("Review the current goal and propose safe next steps."),
                     QStringLiteral("general-assistant"), settings_.defaultPermissionPolicyState(),
                     permissionPolicyService_, toolExecutionGateway_, skillProfileService_,
                     settings_.selectedSkillProfile(), workspaceService_,
                     settings_.selectedWorkspaceId())
        .approvalState;
}

QString DesktopShellViewModel::agentPlanRefusalReason() const {
    return agentRuntimeService_
        .previewPlan(QStringLiteral("Review the current goal and propose safe next steps."),
                     QStringLiteral("general-assistant"), settings_.defaultPermissionPolicyState(),
                     permissionPolicyService_, toolExecutionGateway_, skillProfileService_,
                     settings_.selectedSkillProfile(), workspaceService_,
                     settings_.selectedWorkspaceId())
        .refusalReason;
}

QStringList DesktopShellViewModel::agentPlanDiagnostics() const {
    return agentRuntimeService_
        .previewPlan(QStringLiteral("Review the current goal and propose safe next steps."),
                     QStringLiteral("general-assistant"), settings_.defaultPermissionPolicyState(),
                     permissionPolicyService_, toolExecutionGateway_, skillProfileService_,
                     settings_.selectedSkillProfile(), workspaceService_,
                     settings_.selectedWorkspaceId())
        .diagnostics;
}

QString DesktopShellViewModel::controlledTaskActiveSummary() const {
    const auto tasks =
        controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    for (const auto& task : tasks) {
        if (task.state == core::ControlledTaskState::Running) {
            return core::controlledAgentTaskSummary(task);
        }
    }
    for (const auto& task : tasks) {
        if (task.workspaceId == selectedWorkspaceId() &&
            task.state == core::ControlledTaskState::PendingApproval) {
            return core::controlledAgentTaskSummary(task);
        }
    }
    return QStringLiteral("No controlled agent task is active.");
}

QString DesktopShellViewModel::controlledTaskCurrentStep() const {
    const auto tasks =
        controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    for (const auto& task : tasks) {
        if (task.state == core::ControlledTaskState::Running && task.currentStepIndex >= 0 &&
            task.currentStepIndex < task.steps.size()) {
            return core::controlledAgentStepSummary(task.steps.at(task.currentStepIndex));
        }
    }
    return QStringLiteral("No visible step is running.");
}

QString DesktopShellViewModel::controlledTaskProgressSummary() const {
    const auto tasks =
        controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    for (const auto& task : tasks) {
        if (task.state != core::ControlledTaskState::Running) {
            continue;
        }
        int completed = 0;
        for (const auto& step : task.steps) {
            if (step.state == core::ControlledTaskState::Completed ||
                step.state == core::ControlledTaskState::Cancelled) {
                ++completed;
            }
        }
        return QStringLiteral("%1 of %2 step(s) resolved. Remaining: %3.")
            .arg(completed)
            .arg(task.steps.size())
            .arg(std::max(0, static_cast<int>(task.steps.size()) - completed));
    }
    return QStringLiteral("No running task progress.");
}

QStringList DesktopShellViewModel::controlledTaskPlanSteps() const {
    const auto tasks =
        controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    for (auto it = tasks.crbegin(); it != tasks.crend(); ++it) {
        if (it->workspaceId != selectedWorkspaceId()) {
            continue;
        }
        QStringList steps;
        for (const auto& step : it->steps) {
            steps.append(core::controlledAgentStepSummary(step));
        }
        return steps;
    }
    return {};
}

QStringList DesktopShellViewModel::controlledTaskQueueSummaries() const {
    return controlledAgentTaskService_.queueSummaries(
        controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson()),
        selectedWorkspaceId());
}

QStringList DesktopShellViewModel::controlledTaskTimelineSummaries() const {
    return controlledAgentTaskService_.timelineSummaries(
        controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson()),
        selectedWorkspaceId(), QStringLiteral("all"));
}

QStringList DesktopShellViewModel::controlledTaskExplainabilitySummaries() const {
    const auto tasks =
        controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    for (auto it = tasks.crbegin(); it != tasks.crend(); ++it) {
        if (it->workspaceId == selectedWorkspaceId()) {
            return controlledAgentTaskService_.explainabilitySummaries(*it);
        }
    }
    return {QStringLiteral("No controlled task explainability record yet.")};
}

QStringList DesktopShellViewModel::controlledTaskPermissionSummaries() const {
    return controlledAgentTaskService_.permissionSummaries(
        controlledAgentTaskService_.permissionsFromJson(
            settings_.controlledAgentPermissionsJson()),
        selectedWorkspaceId());
}

QStringList DesktopShellViewModel::controlledTaskNotificationCategories() const {
    return controlledAgentTaskService_.notificationCategories();
}

QStringList DesktopShellViewModel::controlledTaskExportSummaries() const {
    return controlledAgentTaskService_.exportCenterSummaries();
}

QString DesktopShellViewModel::controlledTaskDiagnosticsSummary() const {
    const auto diagnostics = controlledAgentTaskService_.diagnostics(
        controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson()));
    return QStringLiteral("%1 / Last completed: %2 / Approvals: %3 / Failures: %4")
        .arg(diagnostics.activeTask, diagnostics.lastCompletedTask,
             diagnostics.approvalStatistics, diagnostics.failureStatistics);
}

QStringList DesktopShellViewModel::controlledTaskSafetyGuarantees() const {
    return controlledAgentTaskService_.safetyGuarantees();
}

QString DesktopShellViewModel::planControlledAgentTask(const QString& goal) {
    auto tasks = controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    QStringList resources;
    for (const auto& attachment : attachments_) {
        if (attachment.workspaceId == selectedWorkspaceId()) {
            resources.append(attachment.fileName);
        }
    }
    const auto task = controlledAgentTaskService_.createPlan(
        goal, selectedWorkspaceId(), controller_.activeRuntimeProviderLabel(),
        controller_.activeRuntimeModelLabel(), resources, tasks);
    tasks = controlledAgentTaskService_.upsertTask(tasks, task);
    settings_.setControlledAgentTasksJson(controlledAgentTaskService_.tasksToJson(tasks));
    emit controlledAgentTasksChanged();
    return task.id;
}

bool DesktopShellViewModel::modifyControlledAgentPlan(const QString& taskId,
                                                      const QStringList& steps) {
    auto tasks = controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    auto task = controlledAgentTaskService_.taskById(tasks, taskId);
    if (task.id.isEmpty()) {
        return false;
    }
    task = controlledAgentTaskService_.setSteps(task, steps);
    tasks = controlledAgentTaskService_.upsertTask(tasks, task);
    settings_.setControlledAgentTasksJson(controlledAgentTaskService_.tasksToJson(tasks));
    emit controlledAgentTasksChanged();
    return true;
}

bool DesktopShellViewModel::approveControlledAgentTask(const QString& taskId,
                                                       const QString& choice) {
    auto tasks = controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    auto task = controlledAgentTaskService_.taskById(tasks, taskId);
    if (task.id.isEmpty()) {
        return false;
    }
    task = controlledAgentTaskService_.approve(task, choice);
    tasks = controlledAgentTaskService_.upsertTask(tasks, task);
    settings_.setControlledAgentTasksJson(controlledAgentTaskService_.tasksToJson(tasks));
    emit controlledAgentTasksChanged();
    return true;
}

bool DesktopShellViewModel::denyControlledAgentTask(const QString& taskId) {
    auto tasks = controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    auto task = controlledAgentTaskService_.taskById(tasks, taskId);
    if (task.id.isEmpty()) {
        return false;
    }
    task = controlledAgentTaskService_.deny(task);
    tasks = controlledAgentTaskService_.upsertTask(tasks, task);
    settings_.setControlledAgentTasksJson(controlledAgentTaskService_.tasksToJson(tasks));
    emit controlledAgentTasksChanged();
    return true;
}

bool DesktopShellViewModel::startControlledAgentTask(const QString& taskId) {
    auto tasks = controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    auto task = controlledAgentTaskService_.taskById(tasks, taskId);
    if (task.id.isEmpty()) {
        return false;
    }
    task = controlledAgentTaskService_.start(task, tasks);
    tasks = controlledAgentTaskService_.upsertTask(tasks, task);
    settings_.setControlledAgentTasksJson(controlledAgentTaskService_.tasksToJson(tasks));
    emit controlledAgentTasksChanged();
    return task.state == core::ControlledTaskState::Running;
}

bool DesktopShellViewModel::executeControlledAgentStep(const QString& taskId) {
    auto tasks = controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    auto task = controlledAgentTaskService_.taskById(tasks, taskId);
    if (task.id.isEmpty()) {
        return false;
    }
    task = controlledAgentTaskService_.executeCurrentStep(task);
    tasks = controlledAgentTaskService_.upsertTask(tasks, task);
    settings_.setControlledAgentTasksJson(controlledAgentTaskService_.tasksToJson(tasks));
    emit controlledAgentTasksChanged();
    return true;
}

bool DesktopShellViewModel::skipControlledAgentStep(const QString& taskId) {
    auto tasks = controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    auto task = controlledAgentTaskService_.taskById(tasks, taskId);
    if (task.id.isEmpty()) {
        return false;
    }
    task = controlledAgentTaskService_.skipCurrentStep(task);
    tasks = controlledAgentTaskService_.upsertTask(tasks, task);
    settings_.setControlledAgentTasksJson(controlledAgentTaskService_.tasksToJson(tasks));
    emit controlledAgentTasksChanged();
    return true;
}

bool DesktopShellViewModel::retryControlledAgentStep(const QString& taskId) {
    auto tasks = controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    auto task = controlledAgentTaskService_.taskById(tasks, taskId);
    if (task.id.isEmpty()) {
        return false;
    }
    task = controlledAgentTaskService_.retryCurrentStep(task);
    tasks = controlledAgentTaskService_.upsertTask(tasks, task);
    settings_.setControlledAgentTasksJson(controlledAgentTaskService_.tasksToJson(tasks));
    emit controlledAgentTasksChanged();
    return true;
}

bool DesktopShellViewModel::cancelControlledAgentTask(const QString& taskId) {
    auto tasks = controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    auto task = controlledAgentTaskService_.taskById(tasks, taskId);
    if (task.id.isEmpty()) {
        return false;
    }
    task = controlledAgentTaskService_.cancel(task);
    tasks = controlledAgentTaskService_.upsertTask(tasks, task);
    settings_.setControlledAgentTasksJson(controlledAgentTaskService_.tasksToJson(tasks));
    emit controlledAgentTasksChanged();
    return true;
}

bool DesktopShellViewModel::reorderControlledAgentTask(const QString& taskId, int newIndex) {
    auto tasks = controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    tasks = controlledAgentTaskService_.reorderQueue(tasks, taskId, newIndex);
    settings_.setControlledAgentTasksJson(controlledAgentTaskService_.tasksToJson(tasks));
    emit controlledAgentTasksChanged();
    return true;
}

bool DesktopShellViewModel::setControlledToolPermission(const QString& category,
                                                        const QString& choice) {
    auto permissions = controlledAgentTaskService_.permissionsFromJson(
        settings_.controlledAgentPermissionsJson());
    permissions = controlledAgentTaskService_.grantPermission(permissions, selectedWorkspaceId(),
                                                              category.trimmed(), choice.trimmed());
    settings_.setControlledAgentPermissionsJson(
        controlledAgentTaskService_.permissionsToJson(permissions));
    emit controlledAgentTasksChanged();
    return true;
}

bool DesktopShellViewModel::exportControlledAgentTask(const QString& taskId,
                                                      const QString& format) {
    const auto tasks =
        controlledAgentTaskService_.tasksFromJson(settings_.controlledAgentTasksJson());
    const auto task = controlledAgentTaskService_.taskById(tasks, taskId);
    if (task.id.isEmpty()) {
        return false;
    }
    QDir directory(appDataPath() + QStringLiteral("/exports"));
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        return false;
    }
    const auto normalized = format.trimmed().toLower();
    const auto extension = normalized == QStringLiteral("json")
                               ? QStringLiteral("json")
                               : (normalized == QStringLiteral("txt")
                                      ? QStringLiteral("txt")
                                      : (normalized == QStringLiteral("pdf") ? QStringLiteral("pdf")
                                                                             : QStringLiteral("md")));
    const auto path =
        directory.filePath(QStringLiteral("%1-controlled-task.%2").arg(task.id, extension));
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    file.write(controlledAgentTaskService_.exportTaskReport(task, format));
    const auto committed = file.commit();
    emit controlledAgentTasksChanged();
    return committed;
}

bool DesktopShellViewModel::sendMessage(const QString& message) {
    return controller_.sendMessage(message);
}

bool DesktopShellViewModel::cancelLocalInference() {
    return controller_.cancelLocalInference();
}

bool DesktopShellViewModel::runLocalInference(const QString& prompt, const QString& model) {
    return controller_.runLocalInference(prompt, model);
}

bool DesktopShellViewModel::requestConversationSummaryGeneration() {
    return controller_.requestConversationSummaryGeneration();
}

bool DesktopShellViewModel::searchConversation(const QString& query) {
    return controller_.searchConversation(query);
}

void DesktopShellViewModel::clearConversationSearch() {
    controller_.clearConversationSearch();
}

bool DesktopShellViewModel::exportTranscript(const QString& format) {
    return controller_.exportTranscript(format);
}

bool DesktopShellViewModel::checkForUpdates() {
    settings_.setUpdateWorkflowState(QStringLiteral("Checked manually - no automatic update check was started"));
    QString updatedJson;
    if (updateNotification(settings_.notificationCenterJson(), QStringLiteral("updates-manual"),
                           [](QJsonObject& item) {
                               item.insert(QStringLiteral("read"), false);
                               item.insert(QStringLiteral("archived"), false);
                               item.insert(QStringLiteral("body"),
                                           QStringLiteral("Manual check completed locally. Release notes are available; download requires explicit confirmation."));
                           },
                           &updatedJson)) {
        settings_.setNotificationCenterJson(updatedJson);
    }
    emit nativeExperienceChanged();
    return false;
}

bool DesktopShellViewModel::confirmUpdateDownload() {
    settings_.setUpdateWorkflowState(
        QStringLiteral("Download confirmation shown - downloader is not implemented in this build"));
    emit nativeExperienceChanged();
    return false;
}

void DesktopShellViewModel::replayOnboarding() {
    settings_.setOnboardingComplete(false);
}

void DesktopShellViewModel::seedRecoveryDraft(const QString& text) {
    settings_.setRecoveryDraftText(text);
}

bool DesktopShellViewModel::pinNotification(const QString& notificationId) {
    QString updatedJson;
    if (!updateNotification(settings_.notificationCenterJson(), notificationId,
                            [](QJsonObject& item) {
                                item.insert(QStringLiteral("pinned"),
                                            !item.value(QStringLiteral("pinned")).toBool());
                            },
                            &updatedJson)) {
        return false;
    }
    settings_.setNotificationCenterJson(updatedJson);
    return true;
}

bool DesktopShellViewModel::archiveNotification(const QString& notificationId) {
    QString updatedJson;
    if (!updateNotification(settings_.notificationCenterJson(), notificationId,
                            [](QJsonObject& item) {
                                item.insert(QStringLiteral("archived"), true);
                                item.insert(QStringLiteral("read"), true);
                            },
                            &updatedJson)) {
        return false;
    }
    settings_.setNotificationCenterJson(updatedJson);
    return true;
}

bool DesktopShellViewModel::markNotificationRead(const QString& notificationId) {
    QString updatedJson;
    if (!updateNotification(settings_.notificationCenterJson(), notificationId,
                            [](QJsonObject& item) {
                                item.insert(QStringLiteral("read"), true);
                            },
                            &updatedJson)) {
        return false;
    }
    settings_.setNotificationCenterJson(updatedJson);
    return true;
}

bool DesktopShellViewModel::clearArchivedNotifications() {
    const auto notifications = notificationsFromJson(settings_.notificationCenterJson());
    QJsonArray kept;
    for (const auto& value : notifications) {
        const auto item = value.toObject();
        if (!item.value(QStringLiteral("archived")).toBool()) {
            kept.append(item);
        }
    }
    settings_.setNotificationCenterJson(notificationsToJson(kept));
    return true;
}

bool DesktopShellViewModel::prepareExportPreview(const QString& source, const QString& format) {
    const auto normalizedSource = source.trimmed().isEmpty() ? QStringLiteral("conversations")
                                                            : source.trimmed();
    const auto normalizedFormat = format.trimmed().isEmpty() ? QStringLiteral("Markdown")
                                                            : format.trimmed();
    if (normalizedSource == exportPreviewSource_ && normalizedFormat == exportPreviewFormat_) {
        return true;
    }
    exportPreviewSource_ = normalizedSource;
    exportPreviewFormat_ = normalizedFormat;
    emit nativeExperienceChanged();
    return true;
}

bool DesktopShellViewModel::exportDiagnostics(const QString& format) {
    const auto extension = format.trimmed().toLower() == QStringLiteral("json") ? QStringLiteral("json")
                                                                                : QStringLiteral("txt");
    QDir directory(appDataPath() + QStringLiteral("/exports"));
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        return false;
    }
    const auto outputPath = directory.filePath(QStringLiteral("sentinel-diagnostics.%1").arg(extension));
    QSaveFile file(outputPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    QByteArray payload;
    if (extension == QStringLiteral("json")) {
        QJsonObject root;
        root.insert(QStringLiteral("version"), core::AppMetadata::version());
        root.insert(QStringLiteral("projectVersion"), core::AppMetadata::projectVersion());
        root.insert(QStringLiteral("buildNumber"), core::AppMetadata::buildNumber());
        root.insert(QStringLiteral("gitCommit"), core::AppMetadata::gitCommit());
        root.insert(QStringLiteral("buildType"), core::AppMetadata::buildType());
        root.insert(QStringLiteral("platform"), core::AppMetadata::platform());
        root.insert(QStringLiteral("architecture"), core::AppMetadata::architecture());
        root.insert(QStringLiteral("qtVersion"), QString::fromLatin1(qVersion()));
        root.insert(QStringLiteral("activeProvider"), controller_.activeRuntimeProviderLabel());
        root.insert(QStringLiteral("activeModel"), controller_.activeRuntimeModelLabel());
        root.insert(QStringLiteral("workspaceCount"), workspaceIds().size());
        root.insert(QStringLiteral("brainEntryCount"), memoryEntryCount());
        root.insert(QStringLiteral("taskStatistics"), controlledTaskDiagnosticsSummary());
        root.insert(QStringLiteral("notificationStatistics"),
                    notificationLifecycleSummaries().join(QStringLiteral(" / ")));
        payload = QJsonDocument(root).toJson(QJsonDocument::Indented);
    } else {
        payload = diagnosticsCenterSummaries().join(QStringLiteral("\n")).toUtf8();
    }
    if (file.write(payload) != payload.size()) {
        return false;
    }
    return file.commit();
}

bool DesktopShellViewModel::requestConversationExport(const QString& format) {
    return exportTranscript(format);
}

QString DesktopShellViewModel::createConversation(const QString& title) {
    const auto conversationId = controller_.createConversation(title);
    if (!conversationId.isEmpty()) {
        settings_.setActiveConversationId(conversationId);
    }
    return conversationId;
}

bool DesktopShellViewModel::switchConversation(const QString& conversationId) {
    const auto switched = controller_.switchConversation(conversationId);
    if (switched) {
        settings_.setActiveConversationId(controller_.activeConversationId());
    }
    return switched;
}

bool DesktopShellViewModel::renameConversation(const QString& conversationId,
                                               const QString& title) {
    return controller_.renameConversation(conversationId, title);
}

bool DesktopShellViewModel::pinConversation(const QString& conversationId) {
    return controller_.pinConversation(conversationId);
}

bool DesktopShellViewModel::unpinConversation(const QString& conversationId) {
    return controller_.unpinConversation(conversationId);
}

QString DesktopShellViewModel::duplicateConversation(const QString& conversationId) {
    return controller_.duplicateConversation(conversationId);
}

bool DesktopShellViewModel::archiveConversation(const QString& conversationId) {
    const auto archived = controller_.archiveConversation(conversationId);
    if (archived) {
        settings_.setActiveConversationId(controller_.activeConversationId());
    }
    return archived;
}

bool DesktopShellViewModel::unarchiveConversation(const QString& conversationId) {
    const auto unarchived = controller_.unarchiveConversation(conversationId);
    if (unarchived && conversationId == controller_.activeConversationId()) {
        settings_.setActiveConversationId(controller_.activeConversationId());
    }
    return unarchived;
}

bool DesktopShellViewModel::requestPermanentDeleteConversation(const QString& conversationId) {
    return controller_.requestPermanentDeleteConversation(conversationId);
}

QString DesktopShellViewModel::createMemoryCandidateFromConversationText(const QString& text) {
    return controller_.createMemoryCandidateFromConversationText(text);
}

bool DesktopShellViewModel::approveMemoryCandidate(const QString& candidateId) {
    return controller_.approveMemoryCandidate(candidateId);
}

bool DesktopShellViewModel::rejectMemoryCandidate(const QString& candidateId) {
    return controller_.rejectMemoryCandidate(candidateId);
}

bool DesktopShellViewModel::resetMemoryCandidate(const QString& candidateId) {
    return controller_.resetMemoryCandidate(candidateId);
}

bool DesktopShellViewModel::archiveMemoryCandidate(const QString& candidateId) {
    return controller_.archiveMemoryCandidate(candidateId);
}

bool DesktopShellViewModel::requestMemoryCandidateCommit(const QString& candidateId) {
    return controller_.requestMemoryCandidateCommit(candidateId);
}

bool DesktopShellViewModel::recallLocalMemory(const QString& query) {
    return controller_.recallLocalMemory(query);
}

void DesktopShellViewModel::clearLocalMemoryRecall() {
    controller_.clearLocalMemoryRecall();
}

bool DesktopShellViewModel::runAgentRequest(const QString& request) {
    return controller_.runAgentRequest(request);
}

bool DesktopShellViewModel::clearMemory() {
    return controller_.clearMemory();
}

bool DesktopShellViewModel::clearChat() {
    return controller_.clearChat();
}

void DesktopShellViewModel::setModeByName(const QString& modeName) {
    modeManager_.setModeByName(modeName);
}

void DesktopShellViewModel::remember(const QString& key, const QString& value) {
    controller_.remember(key, value);
}

QString DesktopShellViewModel::normalizedPageOrDefault(const QString& page) {
    const auto trimmed = page.trimmed();
    const QStringList pages{
        QStringLiteral("Memory"),
        QStringLiteral("Dashboard"),
        QStringLiteral("Agents"),
        QStringLiteral("Settings"),
    };
    return pages.contains(trimmed) ? trimmed : QStringLiteral("Dashboard");
}

} // namespace sentinel::desktop
