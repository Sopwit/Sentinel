#include "sentinel/core/CompanionService.h"

#include <QtGlobal>

namespace sentinel::core {

QString companionStatusName(CompanionStatus status) {
    switch (status) {
    case CompanionStatus::Disabled:
        return QStringLiteral("Disabled");
    case CompanionStatus::ReadinessOnly:
        return QStringLiteral("Readiness Only");
    }
    return QStringLiteral("Unknown");
}

QString companionAvailabilityName(CompanionAvailability availability) {
    switch (availability) {
    case CompanionAvailability::Unavailable:
        return QStringLiteral("Unavailable");
    case CompanionAvailability::ReadinessOnly:
        return QStringLiteral("Readiness Only");
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

CompanionSummary CompanionService::summary(bool enabledPreference) const {
    CompanionSummary result;
    result.available = false;
    result.enabledPreference = enabledPreference;
    result.status = companionStatusName(enabledPreference ? CompanionStatus::ReadinessOnly
                                                          : CompanionStatus::Disabled);
    result.availability = companionAvailabilityName(CompanionAvailability::ReadinessOnly);
    result.platformCapability = currentPlatformCapability();
    result.permissionPostureSummary =
        QStringLiteral("Permission posture is Disabled by default; Ask Every Time, Trusted, and "
                       "Enabled are future policy states only.");
    result.safetyBoundarySummary =
        QStringLiteral("Companion is readiness-only: no background daemon, provider call, tool "
                       "execution, filesystem scan, microphone capture, playback, memory write, "
                       "or transcript mutation.");
    result.quickCaptureSummary =
        QStringLiteral("Quick Capture placeholder is metadata-only; no note, memory, transcript, "
                       "filesystem, or model action is performed.");
    result.platformSummaries = platformSummaries();
    for (const auto& action : actions()) {
        result.actionSummaries.append(companionActionSummary(action));
    }
    for (const auto& trace : traces(enabledPreference)) {
        result.traceSummaries.append(companionTraceSummary(trace));
    }
    return result;
}

QList<CompanionAction> CompanionService::actions() const {
    return {
        {CompanionActionKind::OpenSentinel,
         companionActionKindName(CompanionActionKind::OpenSentinel),
         QStringLiteral("Open Sentinel"),
         QStringLiteral("foreground navigation placeholder; no hidden execution"),
         CompanionPermissionMode::Disabled,
         false,
         false},
        {CompanionActionKind::NewConversation,
         companionActionKindName(CompanionActionKind::NewConversation),
         QStringLiteral("New conversation"),
         QStringLiteral("metadata-only placeholder; no transcript mutation"),
         CompanionPermissionMode::Disabled,
         false,
         false},
        {CompanionActionKind::QuickNote,
         companionActionKindName(CompanionActionKind::QuickNote),
         QStringLiteral("Quick note"),
         QStringLiteral("Quick Capture placeholder; no filesystem, memory, transcript, or model "
                        "write"),
         CompanionPermissionMode::Disabled,
         false,
         false},
        {CompanionActionKind::PauseCompanion,
         companionActionKindName(CompanionActionKind::PauseCompanion),
         QStringLiteral("Pause companion"),
         QStringLiteral("readiness placeholder; no background work exists to pause"),
         CompanionPermissionMode::Disabled,
         false,
         false},
        {CompanionActionKind::Settings,
         companionActionKindName(CompanionActionKind::Settings),
         QStringLiteral("Settings"),
         QStringLiteral("foreground navigation placeholder; no permission change is applied"),
         CompanionPermissionMode::Disabled,
         false,
         false},
        {CompanionActionKind::Quit,
         companionActionKindName(CompanionActionKind::Quit),
         QStringLiteral("Quit"),
         QStringLiteral("readiness-only placeholder; no process exit is invoked"),
         CompanionPermissionMode::Disabled,
         false,
         false},
    };
}

QList<CompanionTrace> CompanionService::traces(bool enabledPreference) const {
    return {
        {QStringLiteral("preference"),
         enabledPreference ? QStringLiteral("enabled preference")
                           : QStringLiteral("disabled preference"),
         QStringLiteral("stored setting changes visibility intent only; it does not create tray "
                        "runtime support")},
        {QStringLiteral("platform"), QStringLiteral("readiness-only"), currentPlatformCapability()},
        {QStringLiteral("permissions"),
         QStringLiteral("disabled"),
         QStringLiteral("future permission modes are represented but no companion action can run")},
        {QStringLiteral("quick-capture"),
         QStringLiteral("placeholder"),
         QStringLiteral("no filesystem write, memory write, transcript mutation, or model call")},
        {QStringLiteral("execution"),
         QStringLiteral("blocked"),
         QStringLiteral("no provider, tool, voice, plugin, background worker, or autonomous "
                        "authority is added")},
    };
}

QStringList CompanionService::platformSummaries() const {
    return {
        QStringLiteral("macOS menu bar: planned readiness only; no menu bar item is created."),
        QStringLiteral("Windows system tray: planned readiness only; no tray icon is created."),
        QStringLiteral("Linux StatusNotifier/AppIndicator/system tray: planned readiness only; "
                       "no tray item is created."),
    };
}

QString CompanionService::currentPlatformCapability() const {
#if defined(Q_OS_MACOS)
    return QStringLiteral("Current platform: macOS menu bar readiness only; native menu bar "
                          "integration is not implemented.");
#elif defined(Q_OS_WIN)
    return QStringLiteral("Current platform: Windows system tray readiness only; native tray "
                          "integration is not implemented.");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("Current platform: Linux StatusNotifier/AppIndicator/system tray "
                          "readiness only; native tray integration is not implemented.");
#else
    return QStringLiteral("Current platform: generic companion readiness only; native shell "
                          "integration is not implemented.");
#endif
}

CompanionService defaultCompanionService() {
    return CompanionService{};
}

} // namespace sentinel::core
