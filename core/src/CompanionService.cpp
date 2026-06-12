#include "sentinel/core/CompanionService.h"

#include <QtGlobal>

namespace sentinel::core {

QString companionStatusName(CompanionStatus status) {
    switch (status) {
    case CompanionStatus::Disabled:
        return QStringLiteral("Disabled");
    case CompanionStatus::ReadinessOnly:
        return QStringLiteral("Readiness Only");
    case CompanionStatus::Active:
        return QStringLiteral("Active");
    case CompanionStatus::Paused:
        return QStringLiteral("Paused");
    }
    return QStringLiteral("Unknown");
}

QString companionAvailabilityName(CompanionAvailability availability) {
    switch (availability) {
    case CompanionAvailability::Unavailable:
        return QStringLiteral("Unavailable");
    case CompanionAvailability::ReadinessOnly:
        return QStringLiteral("Readiness Only");
    case CompanionAvailability::NativeAvailable:
        return QStringLiteral("Native Available");
    }
    return QStringLiteral("Unknown");
}

QString companionPermissionModeName(CompanionPermissionMode mode) {
    switch (mode) {
    case CompanionPermissionMode::Disabled:
        return QStringLiteral("Disabled");
    case CompanionPermissionMode::AskEveryTime:
        return QStringLiteral("Ask Every Time");
    case CompanionPermissionMode::Trusted:
        return QStringLiteral("Trusted");
    case CompanionPermissionMode::Enabled:
        return QStringLiteral("Enabled");
    }
    return QStringLiteral("Unknown");
}

QString companionActionKindName(CompanionActionKind kind) {
    switch (kind) {
    case CompanionActionKind::OpenSentinel:
        return QStringLiteral("open-sentinel");
    case CompanionActionKind::NewConversation:
        return QStringLiteral("new-conversation");
    case CompanionActionKind::QuickNote:
        return QStringLiteral("quick-note");
    case CompanionActionKind::PauseCompanion:
        return QStringLiteral("pause-companion");
    case CompanionActionKind::Settings:
        return QStringLiteral("settings");
    case CompanionActionKind::Quit:
        return QStringLiteral("quit");
    }
    return QStringLiteral("unknown");
}

QString companionActionSummary(const CompanionAction& action) {
    return QStringLiteral("%1 [%2]: %3 permission=%4 available=%5 execution=%6")
        .arg(action.label,
             action.id,
             action.summary,
             companionPermissionModeName(action.permissionMode),
             action.available ? QStringLiteral("yes") : QStringLiteral("no"),
             action.executionEnabled ? QStringLiteral("enabled") : QStringLiteral("disabled"));
}

QString companionTraceSummary(const CompanionTrace& trace) {
    return QStringLiteral("%1: %2 - %3").arg(trace.category, trace.status, trace.summary);
}

CompanionSummary CompanionService::summary(bool enabledPreference, bool nativeAvailable,
                                           bool paused) const {
    CompanionSummary result;
    result.available = enabledPreference && nativeAvailable;
    result.enabledPreference = enabledPreference;
    if (!enabledPreference) {
        result.status = companionStatusName(CompanionStatus::Disabled);
    } else if (!nativeAvailable) {
        result.status = companionStatusName(CompanionStatus::ReadinessOnly);
    } else if (paused) {
        result.status = companionStatusName(CompanionStatus::Paused);
    } else {
        result.status = companionStatusName(CompanionStatus::Active);
    }
    result.availability = companionAvailabilityName(
        nativeAvailable ? CompanionAvailability::NativeAvailable
                        : CompanionAvailability::Unavailable);
    result.platformCapability = currentPlatformCapability(nativeAvailable);
    result.permissionPostureSummary =
        QStringLiteral("Companion actions are foreground-safe shell actions only; provider, model, "
                       "tool, voice, filesystem, and agent permissions remain disabled.");
    result.safetyBoundarySummary =
        QStringLiteral("Companion is native shell presentation only: no background daemon, provider "
                       "call, tool execution, filesystem scan, microphone capture, playback, "
                       "memory write, or hidden transcript mutation.");
    result.quickCaptureSummary =
        QStringLiteral("Quick Capture placeholder is metadata-only; no note, memory, transcript, "
                       "filesystem, or model action is performed.");
    result.platformSummaries = platformSummaries();
    for (const auto& action : actions(nativeAvailable, paused)) {
        result.actionSummaries.append(companionActionSummary(action));
    }
    for (const auto& trace : traces(enabledPreference, nativeAvailable, paused)) {
        result.traceSummaries.append(companionTraceSummary(trace));
    }
    return result;
}

QList<CompanionAction> CompanionService::actions(bool nativeAvailable, bool paused) const {
    const bool shellActionAvailable = nativeAvailable;
    return {
        {CompanionActionKind::OpenSentinel,
         companionActionKindName(CompanionActionKind::OpenSentinel),
         QStringLiteral("Open Sentinel"),
         QStringLiteral("foreground navigation; no hidden execution"),
         CompanionPermissionMode::Disabled,
         shellActionAvailable,
         shellActionAvailable},
        {CompanionActionKind::NewConversation,
         companionActionKindName(CompanionActionKind::NewConversation),
         QStringLiteral("New Conversation"),
         QStringLiteral("uses existing safe conversation creation path when available"),
         CompanionPermissionMode::Disabled,
         shellActionAvailable,
         shellActionAvailable},
        {CompanionActionKind::QuickNote,
         companionActionKindName(CompanionActionKind::QuickNote),
         QStringLiteral("Quick Note"),
         QStringLiteral("Quick Capture placeholder; no filesystem, memory, transcript, or model "
                        "write"),
         CompanionPermissionMode::Disabled,
         false,
         false},
        {CompanionActionKind::PauseCompanion,
         companionActionKindName(CompanionActionKind::PauseCompanion),
         paused ? QStringLiteral("Resume Companion") : QStringLiteral("Pause Companion"),
         QStringLiteral("presentation/readiness metadata only; no runtime or model behavior is "
                        "changed"),
         CompanionPermissionMode::Disabled,
         shellActionAvailable,
         shellActionAvailable},
        {CompanionActionKind::Settings,
         companionActionKindName(CompanionActionKind::Settings),
         QStringLiteral("Settings"),
         QStringLiteral("foreground navigation; no permission change is applied"),
         CompanionPermissionMode::Disabled,
         shellActionAvailable,
         shellActionAvailable},
        {CompanionActionKind::Quit,
         companionActionKindName(CompanionActionKind::Quit),
         QStringLiteral("Quit"),
         QStringLiteral("normal application quit; no background runtime is started"),
         CompanionPermissionMode::Disabled,
         shellActionAvailable,
         shellActionAvailable},
    };
}

QList<CompanionTrace> CompanionService::traces(bool enabledPreference, bool nativeAvailable,
                                               bool paused) const {
    return {
        {QStringLiteral("preference"),
         enabledPreference ? QStringLiteral("enabled preference")
                           : QStringLiteral("disabled preference"),
         QStringLiteral("stored setting controls native tray/menu visibility only; it does not "
                        "start runtime work")},
        {QStringLiteral("platform"),
         nativeAvailable ? QStringLiteral("native available") : QStringLiteral("unavailable"),
         currentPlatformCapability(nativeAvailable)},
        {QStringLiteral("permissions"),
         QStringLiteral("disabled"),
         QStringLiteral("shell actions do not grant provider, model, tool, voice, filesystem, or "
                        "agent authority")},
        {QStringLiteral("quick-capture"),
         QStringLiteral("placeholder"),
         QStringLiteral("no filesystem write, memory write, transcript mutation, or model call")},
        {QStringLiteral("pause"),
         paused ? QStringLiteral("paused") : QStringLiteral("ready"),
         QStringLiteral("pause changes companion presentation/readiness metadata only")},
        {QStringLiteral("execution"),
         QStringLiteral("blocked"),
         QStringLiteral("no provider, tool, voice, plugin, background worker, or autonomous "
                        "authority is added")},
    };
}

QStringList CompanionService::platformSummaries() const {
    return {
        QStringLiteral("macOS menu bar/status item: uses Qt tray integration first."),
        QStringLiteral("Windows system tray: uses Qt native system tray integration."),
        QStringLiteral("Linux StatusNotifier/AppIndicator/system tray: uses Qt tray/status "
                       "notifier integration when available and degrades gracefully."),
    };
}

QString CompanionService::currentPlatformCapability(bool nativeAvailable) const {
    const QString availability =
        nativeAvailable ? QStringLiteral("native integration available")
                        : QStringLiteral("native integration unavailable");
#if defined(Q_OS_MACOS)
    return QStringLiteral("Current platform: macOS menu bar/status item through Qt tray; %1.")
        .arg(availability);
#elif defined(Q_OS_WIN)
    return QStringLiteral("Current platform: Windows system tray through Qt tray; %1.")
        .arg(availability);
#elif defined(Q_OS_LINUX)
    return QStringLiteral("Current platform: Linux StatusNotifier/AppIndicator/system tray through "
                          "Qt tray when supported; %1.")
        .arg(availability);
#else
    return QStringLiteral("Current platform: generic Qt tray integration; %1.").arg(availability);
#endif
}

CompanionService defaultCompanionService() {
    return CompanionService{};
}

} // namespace sentinel::core
