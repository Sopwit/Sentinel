// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/policy/ResourcePolicy.h"

#include <algorithm>

namespace sentinel::core {

namespace {

void addReason(ResourcePolicyDecision& decision, ResourcePolicyReason reason) {
    if (!decision.reasons.contains(reason)) {
        decision.reasons.append(reason);
    }
}

// One watched signal's contribution to an evaluation.
struct SignalOutcome {
    bool watched = false;
    // The signal exists on this host (not Unavailable/Unsupported). An
    // inapplicable signal is excluded from evidence rather than treated as an
    // unknown measurement.
    bool applicable = true;
    // Any value exists for the signal, fresh or stale. Used for monitoring
    // coverage: a stale snapshot is degraded, not "no monitoring at all".
    bool present = false;
    // A fresh (Measured) value exists, so the signal may raise or clear.
    // The signal applies but its current value is Unknown/Stale: it may neither
    // raise nor clear pressure.
    bool unverified = false;
    bool raised = false;
    ResourcePressureState tier = ResourcePressureState::Elevated;
};

SignalOutcome baseOutcome(bool watched, ResourcePressureState tier) {
    SignalOutcome outcome;
    outcome.watched = watched;
    outcome.tier = tier;
    return outcome;
}

bool signalIsUnverified(const ResourcePolicyInputState state) {
    return state == ResourcePolicyInputState::Unknown || state == ResourcePolicyInputState::Stale;
}

// Fractions and byte counts where a *higher* value means more pressure.
SignalOutcome ascendingSignal(const ResourcePolicyInput<double>& input, bool watched,
                              const std::optional<double>& escalateAt,
                              const std::optional<double>& deescalateAt, ResourcePressureState tier,
                              double scale) {
    SignalOutcome outcome = baseOutcome(watched, tier);
    if (!watched) {
        return outcome;
    }
    if (!input.applicable()) {
        outcome.applicable = false;
        return outcome;
    }
    outcome.present = input.usable();
    const auto rawValue = input.value;
    if (signalIsUnverified(input.state) || !input.fresh() || !rawValue.has_value()) {
        outcome.unverified = true;
        return outcome;
    }
    const double value = *rawValue;
    if (escalateAt.has_value()) {
        outcome.raised = value >= (*escalateAt * scale);
        if (outcome.raised) {
            return outcome;
        }
        // With no separate clear threshold, falling back below the escalation
        // threshold is a measured clear.
        if (!deescalateAt.has_value()) {
            return outcome;
        }
    }
    outcome.raised = deescalateAt.has_value() && value > (*deescalateAt * scale);
    return outcome;
}

// Byte counts where a *lower* value means more pressure (available memory).
SignalOutcome descendingSignal(const ResourcePolicyInput<qint64>& input, bool watched,
                               const std::optional<qint64>& escalateAt,
                               const std::optional<qint64>& deescalateAt,
                               ResourcePressureState tier) {
    SignalOutcome outcome = baseOutcome(watched, tier);
    if (!watched) {
        return outcome;
    }
    if (!input.applicable()) {
        outcome.applicable = false;
        return outcome;
    }
    outcome.present = input.usable();
    const auto rawValue = input.value;
    if (signalIsUnverified(input.state) || !input.fresh() || !rawValue.has_value()) {
        outcome.unverified = true;
        return outcome;
    }
    const qint64 value = *rawValue;
    if (escalateAt.has_value()) {
        outcome.raised = value <= *escalateAt;
        if (outcome.raised) {
            return outcome;
        }
        if (!deescalateAt.has_value()) {
            return outcome;
        }
    }
    outcome.raised = deescalateAt.has_value() && value < *deescalateAt;
    return outcome;
}

// Artificial scale is applied to fractional thresholds only: scaling a byte
// threshold by a dimensionless factor has no meaning.
double fractionScale(const ResourceProfileBudget& budget) {
    const auto scale = budget.escalationThresholdScale;
    return scale.has_value() && *scale > 0.0 ? *scale : 1.0;
}

int tierIndex(ResourcePressureState state) {
    return static_cast<int>(state);
}

ResourcePolicyReason highestPriorityReason(const ResourcePolicyDecision& decision) {
    static constexpr ResourcePolicyReason priority[] = {
        ResourcePolicyReason::ConfigurationInvalid,
        ResourcePolicyReason::ThresholdsNotCalibrated,
        ResourcePolicyReason::NoWatchedSignals,
        ResourcePolicyReason::MonitoringInterruptedFallback,
        ResourcePolicyReason::TelemetryInterrupted,
        ResourcePolicyReason::MonitoringRecovered,
        ResourcePolicyReason::CriticalPressure,
        ResourcePolicyReason::EscalationSustained,
        ResourcePolicyReason::EscalationRequested,
        ResourcePolicyReason::EscalationSustainPending,
        ResourcePolicyReason::DeescalationBlockedByMinimumResidence,
        ResourcePolicyReason::DeescalationBlockedByCooldown,
        ResourcePolicyReason::DeescalationRequested,
        ResourcePolicyReason::BackgroundAdmissionRestricted,
        ResourcePolicyReason::TelemetryDegraded,
        ResourcePolicyReason::ForegroundRunActive,
        ResourcePolicyReason::BackgroundRunActive,
        ResourcePolicyReason::CooldownActive,
        ResourcePolicyReason::TelemetryFresh,
        ResourcePolicyReason::None,
    };
    for (const auto reason : priority) {
        if (decision.reasons.contains(reason)) {
            return reason;
        }
    }
    return decision.reasons.isEmpty() ? ResourcePolicyReason::None : decision.reasons.first();
}

} // namespace

// ---------------------------------------------------------------------------
// Names
// ---------------------------------------------------------------------------

QString resourcePolicyProfileName(ResourcePolicyProfile profile) {
    switch (profile) {
    case ResourcePolicyProfile::Eco:
        return QStringLiteral("Eco");
    case ResourcePolicyProfile::Balanced:
        return QStringLiteral("Balanced");
    case ResourcePolicyProfile::Performance:
        return QStringLiteral("Performance");
    }
    return QStringLiteral("Balanced");
}

QString resourcePressureStateName(ResourcePressureState state) {
    switch (state) {
    case ResourcePressureState::Unknown:
        return QStringLiteral("Unknown");
    case ResourcePressureState::Normal:
        return QStringLiteral("Normal");
    case ResourcePressureState::Elevated:
        return QStringLiteral("Elevated");
    case ResourcePressureState::High:
        return QStringLiteral("High");
    case ResourcePressureState::Critical:
        return QStringLiteral("Critical");
    }
    return QStringLiteral("Unknown");
}

QString resourceTelemetryStateName(ResourceTelemetryState state) {
    switch (state) {
    case ResourceTelemetryState::Fresh:
        return QStringLiteral("Fresh");
    case ResourceTelemetryState::Degraded:
        return QStringLiteral("Degraded");
    case ResourceTelemetryState::Interrupted:
        return QStringLiteral("Interrupted");
    }
    return QStringLiteral("Interrupted");
}

QString resourcePolicyInputStateName(ResourcePolicyInputState state) {
    switch (state) {
    case ResourcePolicyInputState::Unknown:
        return QStringLiteral("Unknown");
    case ResourcePolicyInputState::Measured:
        return QStringLiteral("Measured");
    case ResourcePolicyInputState::Stale:
        return QStringLiteral("Stale");
    case ResourcePolicyInputState::Unavailable:
        return QStringLiteral("Unavailable");
    case ResourcePolicyInputState::Unsupported:
        return QStringLiteral("Unsupported");
    }
    return QStringLiteral("Unknown");
}

QString resourceModelOwnershipName(ResourceModelOwnership ownership) {
    switch (ownership) {
    case ResourceModelOwnership::None:
        return QStringLiteral("None");
    case ResourceModelOwnership::SentinelManaged:
        return QStringLiteral("SentinelManaged");
    case ResourceModelOwnership::SharedExternal:
        return QStringLiteral("SharedExternal");
    }
    return QStringLiteral("None");
}

QString resourcePolicyReasonName(ResourcePolicyReason reason) {
    switch (reason) {
    case ResourcePolicyReason::None:
        return QStringLiteral("None");
    case ResourcePolicyReason::ThresholdsNotCalibrated:
        return QStringLiteral("ThresholdsNotCalibrated");
    case ResourcePolicyReason::NoWatchedSignals:
        return QStringLiteral("NoWatchedSignals");
    case ResourcePolicyReason::TelemetryFresh:
        return QStringLiteral("TelemetryFresh");
    case ResourcePolicyReason::TelemetryDegraded:
        return QStringLiteral("TelemetryDegraded");
    case ResourcePolicyReason::TelemetryInterrupted:
        return QStringLiteral("TelemetryInterrupted");
    case ResourcePolicyReason::MonitoringRecovered:
        return QStringLiteral("MonitoringRecovered");
    case ResourcePolicyReason::MonitoringInterruptedFallback:
        return QStringLiteral("MonitoringInterruptedFallback");
    case ResourcePolicyReason::EscalationRequested:
        return QStringLiteral("EscalationRequested");
    case ResourcePolicyReason::EscalationSustained:
        return QStringLiteral("EscalationSustained");
    case ResourcePolicyReason::EscalationSustainPending:
        return QStringLiteral("EscalationSustainPending");
    case ResourcePolicyReason::DeescalationRequested:
        return QStringLiteral("DeescalationRequested");
    case ResourcePolicyReason::DeescalationBlockedByMinimumResidence:
        return QStringLiteral("DeescalationBlockedByMinimumResidence");
    case ResourcePolicyReason::DeescalationBlockedByCooldown:
        return QStringLiteral("DeescalationBlockedByCooldown");
    case ResourcePolicyReason::CooldownActive:
        return QStringLiteral("CooldownActive");
    case ResourcePolicyReason::CriticalPressure:
        return QStringLiteral("CriticalPressure");
    case ResourcePolicyReason::ForegroundRunActive:
        return QStringLiteral("ForegroundRunActive");
    case ResourcePolicyReason::BackgroundRunActive:
        return QStringLiteral("BackgroundRunActive");
    case ResourcePolicyReason::ForegroundAdmissionPreserved:
        return QStringLiteral("ForegroundAdmissionPreserved");
    case ResourcePolicyReason::BackgroundAdmissionRestricted:
        return QStringLiteral("BackgroundAdmissionRestricted");
    case ResourcePolicyReason::BackgroundAdmissionAllowed:
        return QStringLiteral("BackgroundAdmissionAllowed");
    case ResourcePolicyReason::ProfileSelectedEco:
        return QStringLiteral("ProfileSelectedEco");
    case ResourcePolicyReason::ProfileSelectedBalanced:
        return QStringLiteral("ProfileSelectedBalanced");
    case ResourcePolicyReason::ProfileSelectedPerformance:
        return QStringLiteral("ProfileSelectedPerformance");
    case ResourcePolicyReason::ConfiguredBudgetApplied:
        return QStringLiteral("ConfiguredBudgetApplied");
    case ResourcePolicyReason::ConfiguredBudgetUncalibrated:
        return QStringLiteral("ConfiguredBudgetUncalibrated");
    case ResourcePolicyReason::SharedModelResidencyPreserved:
        return QStringLiteral("SharedModelResidencyPreserved");
    case ResourcePolicyReason::ManagedModelServingActiveRunPreserved:
        return QStringLiteral("ManagedModelServingActiveRunPreserved");
    case ResourcePolicyReason::ConfigurationInvalid:
        return QStringLiteral("ConfigurationInvalid");
    }
    return QStringLiteral("None");
}

QString resourcePolicyActionName(ResourcePolicyAction action) {
    switch (action) {
    case ResourcePolicyAction::None:
        return QStringLiteral("None");
    case ResourcePolicyAction::RefuseNewAdmission:
        return QStringLiteral("RefuseNewAdmission");
    case ResourcePolicyAction::RestrictBackgroundAdmission:
        return QStringLiteral("RestrictBackgroundAdmission");
    case ResourcePolicyAction::LimitToolConcurrency:
        return QStringLiteral("LimitToolConcurrency");
    }
    return QStringLiteral("None");
}

QString resourceOwnershipPostureName(ResourceOwnershipPosture posture) {
    switch (posture) {
    case ResourceOwnershipPosture::PreserveAllResidency:
        return QStringLiteral("PreserveAllResidency");
    }
    return QStringLiteral("PreserveAllResidency");
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

bool ResourcePolicyThresholds::anyEscalationRuleConfigured() const {
    return cpuBusyEscalateAt.has_value() || memoryPressureEscalateAt.has_value() ||
           availableMemoryEscalateAtBytes.has_value() ||
           processResidentEscalateAtBytes.has_value() || !thermalWarningLevels.isEmpty() ||
           escalateOnBattery || batteryChargeEscalateAt.has_value() || escalateWhenGpuUnavailable ||
           gpuUtilizationEscalateAt.has_value();
}

bool ResourcePolicyThresholds::anyDeescalationRuleConfigured() const {
    return cpuBusyDeescalateAt.has_value() || memoryPressureDeescalateAt.has_value() ||
           availableMemoryDeescalateAtBytes.has_value() ||
           processResidentDeescalateAtBytes.has_value() || batteryChargeDeescalateAt.has_value() ||
           gpuUtilizationDeescalateAt.has_value();
}

const ResourceProfileBudget& ResourceProfileBudgetSet::forProfile(ResourcePolicyProfile p) const {
    switch (p) {
    case ResourcePolicyProfile::Eco:
        return eco;
    case ResourcePolicyProfile::Balanced:
        return balanced;
    case ResourcePolicyProfile::Performance:
        return performance;
    }
    return balanced;
}

ResourceProfileBudget& ResourceProfileBudgetSet::forProfile(ResourcePolicyProfile p) {
    switch (p) {
    case ResourcePolicyProfile::Eco:
        return eco;
    case ResourcePolicyProfile::Balanced:
        return balanced;
    case ResourcePolicyProfile::Performance:
        return performance;
    }
    return balanced;
}

namespace {

bool validFraction(const std::optional<double>& value) {
    return !value.has_value() || (*value >= 0.0 && *value <= 1.0);
}

void requireAscending(QStringList& problems, const QString& name, const std::optional<double>& low,
                      const std::optional<double>& high) {
    if (low.has_value() && high.has_value() && *low >= *high) {
        problems.append(QStringLiteral("%1: de-escalation threshold must be lower than the "
                                       "escalation threshold")
                            .arg(name));
    }
}

} // namespace

QStringList validateResourcePolicyConfig(const ResourcePolicyConfig& config) {
    QStringList problems;
    const auto& thresholds = config.thresholds;

    const auto checkFraction = [&problems](const QString& name,
                                           const std::optional<double>& value) {
        if (!validFraction(value)) {
            problems.append(QStringLiteral("%1: threshold must be within 0..1").arg(name));
        }
    };
    checkFraction(QStringLiteral("cpuBusy"), thresholds.cpuBusyEscalateAt);
    checkFraction(QStringLiteral("cpuBusy"), thresholds.cpuBusyDeescalateAt);
    checkFraction(QStringLiteral("memoryPressure"), thresholds.memoryPressureEscalateAt);
    checkFraction(QStringLiteral("memoryPressure"), thresholds.memoryPressureDeescalateAt);
    checkFraction(QStringLiteral("batteryCharge"), thresholds.batteryChargeEscalateAt);
    checkFraction(QStringLiteral("batteryCharge"), thresholds.batteryChargeDeescalateAt);
    checkFraction(QStringLiteral("gpuUtilization"), thresholds.gpuUtilizationEscalateAt);
    checkFraction(QStringLiteral("gpuUtilization"), thresholds.gpuUtilizationDeescalateAt);

    requireAscending(problems, QStringLiteral("cpuBusy"), thresholds.cpuBusyDeescalateAt,
                     thresholds.cpuBusyEscalateAt);
    requireAscending(problems, QStringLiteral("memoryPressure"),
                     thresholds.memoryPressureDeescalateAt, thresholds.memoryPressureEscalateAt);
    requireAscending(problems, QStringLiteral("batteryCharge"), thresholds.batteryChargeEscalateAt,
                     thresholds.batteryChargeDeescalateAt);

    if (thresholds.availableMemoryDeescalateAtBytes.has_value() &&
        thresholds.availableMemoryEscalateAtBytes.has_value() &&
        *thresholds.availableMemoryDeescalateAtBytes <=
            *thresholds.availableMemoryEscalateAtBytes) {
        problems.append(QStringLiteral("availableMemory: the recovery threshold must be above the "
                                       "escalation threshold"));
    }
    if (thresholds.processResidentDeescalateAtBytes.has_value() &&
        thresholds.processResidentEscalateAtBytes.has_value() &&
        *thresholds.processResidentDeescalateAtBytes >=
            *thresholds.processResidentEscalateAtBytes) {
        problems.append(QStringLiteral("processResident: the recovery threshold must be below the "
                                       "escalation threshold"));
    }

    for (const auto tier :
         {thresholds.cpuEscalatedTier, thresholds.memoryPressureEscalatedTier,
          thresholds.availableMemoryEscalatedTier, thresholds.processResidentEscalatedTier,
          thresholds.thermalEscalatedTier, thresholds.powerEscalatedTier,
          thresholds.gpuEscalatedTier}) {
        if (tier == ResourcePressureState::Unknown || tier == ResourcePressureState::Normal) {
            problems.append(QStringLiteral("escalated tier must be Elevated or above"));
            break;
        }
    }

    if (config.timing.escalationSustainMs < 0 || config.timing.minimumResidenceMs < 0 ||
        config.timing.cooldownMs < 0 || config.timing.telemetryGraceMs < 0) {
        problems.append(QStringLiteral("timing durations must not be negative"));
    }

    const auto checkBudget = [&problems](const QString& name, const ResourceProfileBudget& budget) {
        if (budget.maxConcurrentToolStarts.has_value() && *budget.maxConcurrentToolStarts < 1) {
            problems.append(
                QStringLiteral("%1: maxConcurrentToolStarts must be at least 1").arg(name));
        }
        if (budget.maxConcurrentBackgroundStarts.has_value() &&
            *budget.maxConcurrentBackgroundStarts < 0) {
            problems.append(
                QStringLiteral("%1: maxConcurrentBackgroundStarts must not be negative").arg(name));
        }
        if (budget.escalationThresholdScale.has_value() &&
            *budget.escalationThresholdScale <= 0.0) {
            problems.append(
                QStringLiteral("%1: escalationThresholdScale must be positive").arg(name));
        }
        if (budget.samplingIntervalMs.has_value() && *budget.samplingIntervalMs < 1) {
            problems.append(QStringLiteral("%1: samplingIntervalMs must be positive").arg(name));
        }
        if (!budget.calibrated && (budget.maxConcurrentToolStarts.has_value() ||
                                   budget.maxConcurrentBackgroundStarts.has_value() ||
                                   budget.escalationThresholdScale.has_value() ||
                                   budget.samplingIntervalMs.has_value())) {
            problems.append(QStringLiteral("%1: a budget with configured values must be marked "
                                           "calibrated")
                                .arg(name));
        }
    };
    checkBudget(QStringLiteral("eco"), config.profiles.eco);
    checkBudget(QStringLiteral("balanced"), config.profiles.balanced);
    checkBudget(QStringLiteral("performance"), config.profiles.performance);

    if (tierIndex(config.profiles.eco.backgroundAdmissionCeiling) >
        tierIndex(config.profiles.balanced.backgroundAdmissionCeiling)) {
        problems.append(QStringLiteral("eco backgroundAdmissionCeiling must not be looser than "
                                       "balanced"));
    }
    if (tierIndex(config.profiles.balanced.backgroundAdmissionCeiling) >
        tierIndex(config.profiles.performance.backgroundAdmissionCeiling)) {
        problems.append(QStringLiteral("performance backgroundAdmissionCeiling must not be tighter "
                                       "than balanced"));
    }

    const auto scale = [](const ResourceProfileBudget& budget) {
        return budget.escalationThresholdScale.value_or(1.0);
    };
    if (scale(config.profiles.eco) > scale(config.profiles.balanced) ||
        scale(config.profiles.balanced) > scale(config.profiles.performance)) {
        problems.append(QStringLiteral("profile escalationThresholdScale must be ordered "
                                       "eco <= balanced <= performance"));
    }

    const auto toolLimit = [](const ResourceProfileBudget& budget) -> std::optional<int> {
        return budget.maxConcurrentToolStarts;
    };
    const auto ecoTool = toolLimit(config.profiles.eco);
    const auto balancedTool = toolLimit(config.profiles.balanced);
    const auto performanceTool = toolLimit(config.profiles.performance);
    if (ecoTool.has_value() && balancedTool.has_value() && *ecoTool > *balancedTool) {
        problems.append(QStringLiteral("eco maxConcurrentToolStarts must not exceed balanced"));
    }
    if (balancedTool.has_value() && performanceTool.has_value() &&
        *balancedTool > *performanceTool) {
        problems.append(QStringLiteral("balanced maxConcurrentToolStarts must not exceed "
                                       "performance"));
    }

    return problems;
}

// ---------------------------------------------------------------------------
// Summary
// ---------------------------------------------------------------------------

QString resourcePolicyDecisionSummary(const ResourcePolicyDecision& decision) {
    QStringList reasons;
    reasons.reserve(decision.reasons.size());
    for (const auto reason : decision.reasons) {
        reasons.append(resourcePolicyReasonName(reason));
    }
    QStringList actions;
    actions.reserve(decision.actions.size());
    for (const auto action : decision.actions) {
        actions.append(resourcePolicyActionName(action));
    }
    return QStringLiteral("profile=%1 state=%2 telemetry=%3 actions=[%4] reasons=[%5] limits=%6/%7 "
                          "degraded=%8")
        .arg(resourcePolicyProfileName(decision.profile), resourcePressureStateName(decision.state),
             resourceTelemetryStateName(decision.telemetry), actions.join(QLatin1Char(',')),
             reasons.join(QLatin1Char(',')),
             decision.toolConcurrencyLimit.has_value()
                 ? QString::number(*decision.toolConcurrencyLimit)
                 : QStringLiteral("unset"),
             decision.backgroundConcurrencyLimit.has_value()
                 ? QString::number(*decision.backgroundConcurrencyLimit)
                 : QStringLiteral("unset"),
             decision.degraded ? QStringLiteral("yes") : QStringLiteral("no"));
}

// ---------------------------------------------------------------------------
// Engine
// ---------------------------------------------------------------------------

ResourcePolicyEngine::ResourcePolicyEngine(ResourcePolicyConfig config) {
    setConfig(std::move(config));
}

void ResourcePolicyEngine::setConfig(ResourcePolicyConfig config) {
    config_ = std::move(config);
    configurationProblems_ = validateResourcePolicyConfig(config_);
    reset();
}

void ResourcePolicyEngine::reset() {
    state_ = ResourcePressureState::Unknown;
    stateEnteredAtMs_ = 0;
    cooldownUntilMs_ = 0;
    pendingTier_ = ResourcePressureState::Unknown;
    pendingSinceMs_ = 0;
    interruptedSinceMs_ = 0;
    lastVerifiedTier_ = ResourcePressureState::Unknown;
}

ResourceProfileBudget ResourcePolicyEngine::selectedBudget() const {
    return config_.profiles.forProfile(config_.profile);
}

ResourcePolicyDecision ResourcePolicyEngine::evaluate(const ResourcePolicyInputs& inputs,
                                                      qint64 nowMonotonicMs) {
    ResourcePolicyDecision decision;
    decision.profile = config_.profile;
    decision.evaluatedAtMonotonicMs = nowMonotonicMs;
    decision.thresholdsCalibrated = config_.thresholds.anyEscalationRuleConfigured();
    decision.foregroundRunsActive = inputs.activeForegroundRuns > 0;
    decision.backgroundRunsActive = inputs.activeBackgroundRuns > 0;
    decision.modelOwnership = inputs.modelOwnership;
    decision.modelServingActiveRun = inputs.modelServingActiveRun;
    decision.ownershipPosture = ResourceOwnershipPosture::PreserveAllResidency;

    if (!configurationProblems_.isEmpty()) {
        addReason(decision, ResourcePolicyReason::ConfigurationInvalid);
    }

    switch (config_.profile) {
    case ResourcePolicyProfile::Eco:
        addReason(decision, ResourcePolicyReason::ProfileSelectedEco);
        break;
    case ResourcePolicyProfile::Balanced:
        addReason(decision, ResourcePolicyReason::ProfileSelectedBalanced);
        break;
    case ResourcePolicyProfile::Performance:
        addReason(decision, ResourcePolicyReason::ProfileSelectedPerformance);
        break;
    }

    const auto& thresholds = config_.thresholds;
    const auto budget = selectedBudget();
    const double scale = fractionScale(budget);

    // --- Classify watched signals -----------------------------------------
    const bool cpuWatched =
        thresholds.cpuBusyEscalateAt.has_value() || thresholds.cpuBusyDeescalateAt.has_value();
    const bool memoryPressureWatched = thresholds.memoryPressureEscalateAt.has_value() ||
                                       thresholds.memoryPressureDeescalateAt.has_value();
    const bool availableMemoryWatched = thresholds.availableMemoryEscalateAtBytes.has_value() ||
                                        thresholds.availableMemoryDeescalateAtBytes.has_value();
    const bool processResidentWatched = thresholds.processResidentEscalateAtBytes.has_value() ||
                                        thresholds.processResidentDeescalateAtBytes.has_value();
    const bool thermalWatched = !thresholds.thermalWarningLevels.isEmpty();
    const bool powerWatched = thresholds.escalateOnBattery ||
                              thresholds.batteryChargeEscalateAt.has_value() ||
                              thresholds.batteryChargeDeescalateAt.has_value();
    const bool gpuWatched = thresholds.escalateWhenGpuUnavailable ||
                            thresholds.gpuUtilizationEscalateAt.has_value() ||
                            thresholds.gpuUtilizationDeescalateAt.has_value();

    QList<SignalOutcome> outcomes;
    outcomes.append(ascendingSignal(inputs.cpuBusyFraction, cpuWatched,
                                    thresholds.cpuBusyEscalateAt, thresholds.cpuBusyDeescalateAt,
                                    thresholds.cpuEscalatedTier, scale));
    outcomes.append(ascendingSignal(
        inputs.memoryPressureLevel, memoryPressureWatched, thresholds.memoryPressureEscalateAt,
        thresholds.memoryPressureDeescalateAt, thresholds.memoryPressureEscalatedTier, scale));
    outcomes.append(descendingSignal(inputs.hostAvailableBytes, availableMemoryWatched,
                                     thresholds.availableMemoryEscalateAtBytes,
                                     thresholds.availableMemoryDeescalateAtBytes,
                                     thresholds.availableMemoryEscalatedTier));
    outcomes.append(descendingSignal(inputs.processResidentBytes, processResidentWatched,
                                     thresholds.processResidentEscalateAtBytes,
                                     thresholds.processResidentDeescalateAtBytes,
                                     thresholds.processResidentEscalatedTier));

    // Thermal: a native level string, never a fabricated numeric score.
    {
        SignalOutcome outcome = baseOutcome(thermalWatched, thresholds.thermalEscalatedTier);
        if (thermalWatched && !inputs.thermalLevel.applicable()) {
            outcome.applicable = false;
        } else if (thermalWatched) {
            outcome.present = inputs.thermalLevel.usable();
            if (signalIsUnverified(inputs.thermalLevel.state) || !inputs.thermalLevel.fresh()) {
                outcome.unverified = true;
            } else {
                outcome.raised =
                    thresholds.thermalWarningLevels.contains(*inputs.thermalLevel.value);
            }
        }
        outcomes.append(outcome);
    }

    // Power: battery operation is an explicit escalation policy, off by default.
    {
        SignalOutcome outcome = baseOutcome(powerWatched, thresholds.powerEscalatedTier);
        if (powerWatched) {
            const bool sourceApplicable = inputs.powerSource.applicable();
            if (!sourceApplicable) {
                // A host without power telemetry contributes nothing.
                outcome.applicable = false;
            } else if (signalIsUnverified(inputs.powerSource.state) ||
                       !inputs.powerSource.fresh()) {
                outcome.present = inputs.powerSource.usable();
                outcome.unverified = true;
            } else {
                const bool onBattery =
                    *inputs.powerSource.value == ResourcePolicyPowerSource::Battery;
                if (onBattery && thresholds.escalateOnBattery) {
                    outcome.raised = true;
                } else if (onBattery && thresholds.batteryChargeEscalateAt.has_value() &&
                           inputs.batteryChargeFraction.applicable() &&
                           inputs.batteryChargeFraction.fresh()) {
                    outcome.raised = *inputs.batteryChargeFraction.value <=
                                     (*thresholds.batteryChargeEscalateAt * scale);
                }
            }
        }
        outcomes.append(outcome);
    }

    // GPU: absence escalation is an explicit configuration requirement.
    {
        SignalOutcome outcome = baseOutcome(gpuWatched, thresholds.gpuEscalatedTier);
        if (gpuWatched) {
            const bool anyApplicable =
                inputs.gpuAvailable.applicable() || inputs.gpuUtilizationFraction.applicable();
            if (!anyApplicable) {
                outcome.applicable = false;
            }
            bool contributed = false;
            if (thresholds.escalateWhenGpuUnavailable && inputs.gpuAvailable.applicable()) {
                outcome.present = inputs.gpuAvailable.usable();
                if (signalIsUnverified(inputs.gpuAvailable.state) || !inputs.gpuAvailable.fresh()) {
                    outcome.unverified = true;
                    contributed = true;
                } else {
                    outcome.raised = !*inputs.gpuAvailable.value;
                    contributed = true;
                }
            }
            if (!contributed && thresholds.gpuUtilizationEscalateAt.has_value()) {
                if (inputs.gpuUtilizationFraction.applicable()) {
                    outcome.present = inputs.gpuUtilizationFraction.usable();
                    if (signalIsUnverified(inputs.gpuUtilizationFraction.state) ||
                        !inputs.gpuUtilizationFraction.fresh()) {
                        outcome.unverified = true;
                    } else {
                        outcome.raised = *inputs.gpuUtilizationFraction.value >=
                                         (*thresholds.gpuUtilizationEscalateAt * scale);
                    }
                }
            }
        }
        outcomes.append(outcome);
    }

    int watchedCount = 0;
    int applicableWatchedCount = 0;
    int presentCount = 0;
    int unverifiedCount = 0;
    bool anyRaised = false;
    ResourcePressureState raisedTier = ResourcePressureState::Unknown;
    for (const auto& outcome : outcomes) {
        if (!outcome.watched) {
            continue;
        }
        ++watchedCount;
        if (outcome.applicable) {
            ++applicableWatchedCount;
        }
        if (outcome.present) {
            ++presentCount;
        }
        if (outcome.unverified) {
            ++unverifiedCount;
        }
        if (outcome.raised) {
            anyRaised = true;
            raisedTier =
                tierIndex(outcome.tier) > tierIndex(raisedTier) ? outcome.tier : raisedTier;
        }
    }

    if (watchedCount == 0 || applicableWatchedCount == 0) {
        addReason(decision, ResourcePolicyReason::NoWatchedSignals);
    }
    if (!decision.thresholdsCalibrated) {
        addReason(decision, ResourcePolicyReason::ThresholdsNotCalibrated);
    }

    const ResourceTelemetryState telemetry = presentCount == 0 ? ResourceTelemetryState::Interrupted
                                             : unverifiedCount > 0
                                                 ? ResourceTelemetryState::Degraded
                                                 : ResourceTelemetryState::Fresh;
    decision.telemetry = telemetry;
    decision.degraded = telemetry != ResourceTelemetryState::Fresh;

    if (telemetry == ResourceTelemetryState::Fresh) {
        addReason(decision, ResourcePolicyReason::TelemetryFresh);
    } else if (telemetry == ResourceTelemetryState::Degraded) {
        addReason(decision, ResourcePolicyReason::TelemetryDegraded);
    } else {
        addReason(decision, ResourcePolicyReason::TelemetryInterrupted);
    }

    const bool wasInterrupted = interruptedSinceMs_ != 0;

    // --- Inert policy ------------------------------------------------------
    if (!decision.thresholdsCalibrated || applicableWatchedCount == 0) {
        // No verified verdict is possible. The conservative fallback is today's
        // fixed limits: propose nothing, restrict nothing, and never read the
        // absence of a rule as "Normal".
        state_ = ResourcePressureState::Unknown;
        lastVerifiedTier_ = ResourcePressureState::Unknown;
        pendingTier_ = ResourcePressureState::Unknown;
        interruptedSinceMs_ = 0;
        decision.state = state_;
        decision.reasons.removeAll(ResourcePolicyReason::TelemetryFresh);
        decision.reasons.removeAll(ResourcePolicyReason::TelemetryDegraded);
        decision.reasons.removeAll(ResourcePolicyReason::TelemetryInterrupted);
        decision.telemetry = ResourceTelemetryState::Interrupted;
        decision.degraded = true;
        decision.foregroundAdmissionAllowed = true;
        decision.actions = {ResourcePolicyAction::None};
        addReason(decision, ResourcePolicyReason::ConfiguredBudgetUncalibrated);
        decision.primaryReason = highestPriorityReason(decision);
        return decision;
    }

    // --- Monitoring interruption ------------------------------------------
    if (telemetry == ResourceTelemetryState::Interrupted) {
        if (interruptedSinceMs_ == 0) {
            interruptedSinceMs_ = nowMonotonicMs;
            lastVerifiedTier_ = state_;
        }
        const bool graceElapsed =
            config_.timing.telemetryGraceMs > 0 &&
            nowMonotonicMs - interruptedSinceMs_ >= config_.timing.telemetryGraceMs;
        if (graceElapsed && state_ != ResourcePressureState::Normal &&
            state_ != ResourcePressureState::Unknown) {
            // A restriction must not outlive the telemetry that justified it.
            state_ = ResourcePressureState::Normal;
            stateEnteredAtMs_ = nowMonotonicMs;
            cooldownUntilMs_ = nowMonotonicMs + config_.timing.cooldownMs;
            pendingTier_ = ResourcePressureState::Unknown;
            decision.isTransition = true;
            addReason(decision, ResourcePolicyReason::MonitoringInterruptedFallback);
        }
        decision.state = state_;
        decision.foregroundAdmissionAllowed = true;
    } else {
        // --- Verified evaluation ------------------------------------------
        if (wasInterrupted) {
            addReason(decision, ResourcePolicyReason::MonitoringRecovered);
        }
        interruptedSinceMs_ = 0;

        ResourcePressureState target = anyRaised ? raisedTier : ResourcePressureState::Normal;
        // Missing evidence can escalate or hold, never clear.
        if (decision.degraded && tierIndex(target) < tierIndex(state_)) {
            target = state_;
        }

        if (tierIndex(target) > tierIndex(state_)) {
            const qint64 sustain = config_.timing.escalationSustainMs;
            bool transitionAllowed = true;
            // Spike rejection applies to real pressure tiers; establishing the
            // fallback `Normal` baseline is immediate.
            if (sustain > 0 && target != ResourcePressureState::Normal) {
                if (pendingTier_ != target) {
                    pendingTier_ = target;
                    pendingSinceMs_ = nowMonotonicMs;
                }
                if (nowMonotonicMs - pendingSinceMs_ < sustain) {
                    transitionAllowed = false;
                    addReason(decision, ResourcePolicyReason::EscalationSustainPending);
                } else {
                    addReason(decision, ResourcePolicyReason::EscalationSustained);
                }
            } else {
                addReason(decision, ResourcePolicyReason::EscalationRequested);
            }
            if (transitionAllowed) {
                state_ = target;
                stateEnteredAtMs_ = nowMonotonicMs;
                cooldownUntilMs_ = nowMonotonicMs + config_.timing.cooldownMs;
                pendingTier_ = ResourcePressureState::Unknown;
                decision.isTransition = true;
            }
        } else if (tierIndex(target) < tierIndex(state_)) {
            const bool residenceOk =
                nowMonotonicMs - stateEnteredAtMs_ >= config_.timing.minimumResidenceMs;
            const bool cooldownOk = nowMonotonicMs >= cooldownUntilMs_;
            if (!residenceOk) {
                addReason(decision, ResourcePolicyReason::DeescalationBlockedByMinimumResidence);
            } else if (!cooldownOk) {
                addReason(decision, ResourcePolicyReason::DeescalationBlockedByCooldown);
            } else {
                // Ordered, monotonic de-escalation: one tier at a time.
                const int next =
                    std::max(tierIndex(ResourcePressureState::Normal), tierIndex(state_) - 1);
                state_ = static_cast<ResourcePressureState>(next);
                stateEnteredAtMs_ = nowMonotonicMs;
                cooldownUntilMs_ = nowMonotonicMs + config_.timing.cooldownMs;
                pendingTier_ = ResourcePressureState::Unknown;
                decision.isTransition = true;
                addReason(decision, ResourcePolicyReason::DeescalationRequested);
            }
        } else {
            pendingTier_ = ResourcePressureState::Unknown;
        }
        decision.state = state_;
        decision.foregroundAdmissionAllowed = true;

        if (nowMonotonicMs < cooldownUntilMs_ &&
            tierIndex(state_) > tierIndex(ResourcePressureState::Normal)) {
            addReason(decision, ResourcePolicyReason::CooldownActive);
        }
    }

    // --- Admission and ownership outcome ----------------------------------
    decision.refuseNewAdmission = state_ == ResourcePressureState::Critical;
    if (decision.refuseNewAdmission) {
        addReason(decision, ResourcePolicyReason::CriticalPressure);
    }
    decision.restrictBackgroundAdmission =
        decision.refuseNewAdmission ||
        (state_ != ResourcePressureState::Unknown &&
         tierIndex(state_) > tierIndex(budget.backgroundAdmissionCeiling));

    if (decision.restrictBackgroundAdmission) {
        addReason(decision, ResourcePolicyReason::BackgroundAdmissionRestricted);
    } else {
        addReason(decision, ResourcePolicyReason::BackgroundAdmissionAllowed);
    }
    addReason(decision, ResourcePolicyReason::ForegroundAdmissionPreserved);
    if (decision.foregroundRunsActive) {
        addReason(decision, ResourcePolicyReason::ForegroundRunActive);
    }
    if (decision.backgroundRunsActive) {
        addReason(decision, ResourcePolicyReason::BackgroundRunActive);
        // Running background work is never preempted; only new starts are held.
    }

    // Ownership is read-only in P4-03 (PD-D10 pending).
    if (decision.modelOwnership == ResourceModelOwnership::SharedExternal) {
        addReason(decision, ResourcePolicyReason::SharedModelResidencyPreserved);
    } else if (decision.modelOwnership == ResourceModelOwnership::SentinelManaged &&
               decision.modelServingActiveRun) {
        addReason(decision, ResourcePolicyReason::ManagedModelServingActiveRunPreserved);
    }

    if (budget.calibrated) {
        decision.toolConcurrencyLimit = budget.maxConcurrentToolStarts;
        if (decision.restrictBackgroundAdmission) {
            decision.backgroundConcurrencyLimit = budget.maxConcurrentBackgroundStarts;
        }
        addReason(decision, ResourcePolicyReason::ConfiguredBudgetApplied);
    } else {
        addReason(decision, ResourcePolicyReason::ConfiguredBudgetUncalibrated);
    }

    if (decision.refuseNewAdmission) {
        decision.actions.append(ResourcePolicyAction::RefuseNewAdmission);
    }
    if (decision.restrictBackgroundAdmission) {
        decision.actions.append(ResourcePolicyAction::RestrictBackgroundAdmission);
    }
    if (decision.toolConcurrencyLimit.has_value() ||
        decision.backgroundConcurrencyLimit.has_value()) {
        decision.actions.append(ResourcePolicyAction::LimitToolConcurrency);
    }
    if (decision.actions.isEmpty()) {
        decision.actions.append(ResourcePolicyAction::None);
    }

    decision.primaryReason = highestPriorityReason(decision);
    return decision;
}

} // namespace sentinel::core
