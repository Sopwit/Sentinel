#pragma once

#include <QList>
#include <QString>
#include <QStringList>

namespace sentinel::core {

enum class CompanionStatus {
    Disabled,
    ReadinessOnly,
    Active,
    Paused,
};

enum class CompanionAvailability {
    Unavailable,
    ReadinessOnly,
    NativeAvailable,
};

enum class CompanionPermissionMode {
    Disabled,
    AskEveryTime,
    Trusted,
    Enabled,
};

enum class CompanionActionKind {
    OpenSentinel,
    NewConversation,
    QuickNote,
    PauseCompanion,
    Settings,
    Quit,
};

struct CompanionAction {
    CompanionActionKind kind = CompanionActionKind::OpenSentinel;
    QString id;
    QString label;
    QString summary;
    CompanionPermissionMode permissionMode = CompanionPermissionMode::Disabled;
    bool available = false;
    bool executionEnabled = false;
};

struct CompanionTrace {
    QString category;
    QString status;
    QString summary;
};

struct CompanionSummary {
    bool available = false;
    bool enabledPreference = false;
    QString status;
    QString availability;
    QString platformCapability;
    QString permissionPostureSummary;
    QString safetyBoundarySummary;
    QString quickCaptureSummary;
    QStringList actionSummaries;
    QStringList platformSummaries;
    QStringList traceSummaries;
};

QString companionStatusName(CompanionStatus status);
QString companionAvailabilityName(CompanionAvailability availability);
QString companionPermissionModeName(CompanionPermissionMode mode);
QString companionActionKindName(CompanionActionKind kind);
QString companionActionSummary(const CompanionAction& action);
QString companionTraceSummary(const CompanionTrace& trace);

class CompanionService final {
public:
    CompanionSummary summary(bool enabledPreference, bool nativeAvailable = false,
                             bool paused = false) const;
    QList<CompanionAction> actions(bool nativeAvailable = false, bool paused = false) const;
    QList<CompanionTrace> traces(bool enabledPreference, bool nativeAvailable = false,
                                 bool paused = false) const;
    QStringList platformSummaries() const;
    QString currentPlatformCapability(bool nativeAvailable = false) const;
};

CompanionService defaultCompanionService();

} // namespace sentinel::core
