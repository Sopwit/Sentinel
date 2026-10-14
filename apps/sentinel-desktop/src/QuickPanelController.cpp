// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/desktop/QuickPanelController.h"
#include "sentinel/desktop/DesktopRuntimeClient.h"
#include <QMetaEnum>
#include <QRegularExpression>
#include <QUrl>
namespace sentinel::desktop {
QuickPanelController::QuickPanelController(DesktopRuntimeClient& runtime, QObject* parent)
    : QObject(parent), runtime_(runtime) {
    connect(&runtime_, &DesktopRuntimeClient::changed, this, &QuickPanelController::project);
    connect(&runtime_.transport(), &DaemonClient::connectionStateChanged, this,
            &QuickPanelController::changed);
    connect(&runtime_, &DesktopRuntimeClient::operationFailed, this, [this](const QString& code) {
        error_ = code;
        emit changed();
    });
}
QVariantMap QuickPanelController::state() const {
    auto result = runtime_.quickPanelSnapshot();
    const auto connection = runtime_.transport().connectionState();
    result["connection"] =
        QString::fromLatin1(QMetaEnum::fromType<DaemonClient::ConnectionState>().valueToKey(
            static_cast<int>(connection)));
    result["ready"] = runtime_.ready() && !runtime_.sessionId().isEmpty();
    result["workspace"] = runtime_.ready() ? runtime_.value("currentWorkspaceName").toString()
                                           : QStringLiteral("Unavailable");
    result["model"] = runtime_.ready() ? runtime_.value("activeRuntimeProviderLabel").toString() +
                                             QStringLiteral(" / ") +
                                             runtime_.value("activeRuntimeModelLabel").toString()
                                       : QStringLiteral("Unavailable");
    if (runtime_.ready() && result.value("busy").toBool() &&
        !runtime_.value("activeChatModelId").toString().isEmpty()) {
        result["model"] = runtime_.value("activeChatProviderId").toString() +
                          QStringLiteral(" / ") + runtime_.value("activeChatModelId").toString();
    }
    result["network"] = runtime_.ready() ? runtime_.settingsValue("networkMode", "Unavailable")
                                         : QStringLiteral("Unavailable");
    if (!runtime_.ready()) {
        result["approvalCount"] = 0;
        result["approval"] = QVariantMap{};
    }
    const auto voice = runtime_.voiceState();
    result["voiceAvailable"] = runtime_.ready() && voice.value("available").toBool() &&
                               voice.value("state").toString() != "Transcribing";
    result["voiceState"] = voice.value("state").toString().toLower();
    result["voiceLevel"] = voice.value("level", 0.0);
    result["voiceTranscript"] = voice.value("transcript", QString{});
    result["voice"] =
        !runtime_.ready()                               ? tr("Loading voice state…")
        : voice.value("requesting_permission").toBool() ? tr("Requesting microphone permission")
        : voice.value("state").toString() == "StartingCapture" ? tr("Preparing microphone…")
        : voice.value("state").toString() == "Transcribing"    ? tr("Processing speech…")
        : voice.value("state").toString() == "Failed"
            ? tr("Voice error: %1").arg(voice.value("failure").toString())
        : !voice.value("available").toBool() && voice.value("state").toString() != "Listening"
            ? tr("Unavailable — STT runtime/model is not ready or voice is busy")
            : voice.value("state").toString();
    return result;
}
bool QuickPanelController::submit(const QString& text, bool agentMode) {
    error_.clear();
    const bool accepted =
        runtime_.dispatch(agentMode ? "runAgentRequest" : "sendMessage", {text.trimmed()}).toBool();
    if (!accepted) {
        error_ = QStringLiteral("Request unavailable: check connection, session and active run.");
    }
    emit changed();
    return accepted;
}
bool QuickPanelController::ask(const QString& text) {
    return submit(text, false);
}
bool QuickPanelController::agent(const QString& text) {
    return submit(text, true);
}
bool QuickPanelController::runtimeVoiceAction(const QString& action) {
    return runtime_.voiceAction(action);
}
bool QuickPanelController::cancel() {
    return runtime_.cancelRun();
}
bool QuickPanelController::approve(bool allow) {
    const bool accepted = runtime_.respondToQuickApproval(allow);
    emit changed();
    return accepted;
}
void QuickPanelController::continueConversation() {
    emit openRequested("Dashboard", runtime_.sessionId());
}
void QuickPanelController::openApproval() {
    const auto session = runtime_.quickPanelSnapshot().value("approvalSession").toString();
    if (!session.isEmpty()) {
        openLink("sentinel://session/" + session);
    }
}
void QuickPanelController::project() {
    if (runtime_.ready() && error_ == "daemon-unavailable") {
        error_.clear();
    }
    const auto snapshot = runtime_.quickPanelSnapshot();
    const auto generation = runtime_.transport().serverGeneration();
    const auto session = runtime_.sessionId();
    const bool generationChanged = !generation.isEmpty() && generation != lastGeneration_;
    if (generationChanged) {
        lastApproval_.clear(); // Approval identities are daemon-wide, not selected-session-wide.
    }
    if (generationChanged || session != lastSession_) {
        lastRun_.clear();
        lastState_.clear();
        pendingTerminalStates_.clear();
        lastGeneration_ = generation;
        lastSession_ = session;
    }
    const auto run = snapshot.value("runId").toString();
    const auto status = snapshot.value("state").toString();
    if (!run.isEmpty() && run == lastRun_ && status != lastState_ &&
        (lastState_ == "running" || lastState_ == "approval") &&
        (status == "completed" || status == "failed" || status == "cancelled") &&
        snapshot.value("kind") == "agent") {
        // Observe terminal transitions even while reattachment gates delivery.
        // Keep their order within this session/generation until ready again.
        pendingTerminalStates_.append(status);
    }
    lastRun_ = run;
    lastState_ = status;
    const auto sameScope = [&] {
        return runtime_.sessionId() == session &&
               runtime_.transport().serverGeneration() == generation;
    };
    while (runtime_.ready() && sameScope() && !pendingTerminalStates_.isEmpty()) {
        // Consume one identity before emitting a potentially reentrant signal.
        // If a slot reattaches, remaining deliveries stay queued until ready;
        // if it changes session/generation, project() discards the old queue.
        const auto terminal = pendingTerminalStates_.takeFirst();
        emit notificationRequested(
            tr("Sentinel Agent: %1").arg(terminal), tr("Open Sentinel to review this run."),
            session, QStringLiteral("Agent"),
            terminal == "failed" ? QStringLiteral("High") : QStringLiteral("Normal"));
    }
    if (runtime_.ready() && sameScope()) {
        const auto current = runtime_.quickPanelSnapshot();
        const auto pending = current.value("approval").toMap();
        const auto approval = pending.value("approval_id").toString();
        if (!approval.isEmpty() && approval != lastApproval_) {
            lastApproval_ = approval;
            emit notificationRequested(tr("Approval needed"),
                                       pending.value("detail").toString().left(240),
                                       current.value("approvalSession").toString(),
                                       QStringLiteral("Security"), QStringLiteral("High"));
        }
    }
    emit changed();
}
QVariantMap QuickPanelController::parseLink(const QString& text) {
    if (text.size() > 512) {
        return {};
    }
    const QUrl url(text, QUrl::StrictMode);
    if (!url.isValid() || url.scheme() != "sentinel" || !url.userInfo().isEmpty() ||
        url.port() != -1 || url.hasQuery() || url.hasFragment()) {
        return {};
    }
    if (url.host() == "settings" && (url.path().isEmpty() || url.path() == "/")) {
        return {{"page", "Settings"}};
    }
    if (url.host() != "session") {
        return {};
    }
    const QString id = url.path().mid(1);
    static const QRegularExpression boundedId(QStringLiteral("^[A-Za-z0-9_-]{1,128}$"));
    if (!boundedId.match(id).hasMatch()) {
        return {};
    }
    return {{"page", "Dashboard"}, {"sessionId", id}};
}
bool QuickPanelController::openLink(const QString& url) {
    const auto target = parseLink(url);
    if (target.isEmpty()) {
        return false;
    }
    const auto session = target.value("sessionId").toString();
    if (!session.isEmpty()) {
        if (!runtime_.ready()) {
            return false;
        }
        runtime_.dispatch("switchConversation", {session});
    }
    emit openRequested(target.value("page").toString(), session);
    return true;
}
} // namespace sentinel::desktop
