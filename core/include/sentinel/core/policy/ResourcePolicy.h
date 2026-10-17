// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// P4-03 adaptive resource policy foundation.
//
// This is a *policy evaluation* contract. It is deliberately separate from:
//   * telemetry collection (P4-02 `HostResourceSnapshot` / `ResourceMonitor`),
//   * authorization (`ResourceAuthorizationSnapshot`, `ToolExecutionGateway`),
//   * execution admission (`AgentExecutionScheduler`),
//   * model/process ownership (`ProcessExecutor`, `LocalModelRuntime`).
//
// Truth and safety rules enforced by these types:
//   * An unavailable or stale measurement is never read as "healthy" and never
//     as zero. A missing measurement can raise or hold pressure, it can never
//     clear it.
//   * Every numeric threshold is configuration, not a constant. Until PD-D03 /
//     PD-D04 they are unset, so the engine reports `Unconfigured` and proposes
//     nothing: today's fixed limits stay in force.
//   * A profile (Eco/Balanced/Performance) can only ever change *admission
//     amounts*. It carries no permission, approval, sandbox or ownership
//     authority, and the action vocabulary contains no destructive verb.
//   * The engine is a pure evaluator: it takes no lock, starts no thread, sends
//     no request, unloads no model and terminates no process.

#include <QList>
#include <QString>
#include <QStringList>

#include <optional>

namespace sentinel::core {

// ---------------------------------------------------------------------------
// Vocabulary
// ---------------------------------------------------------------------------

// Named resource profile. This is NOT `RoutingMode`: `RoutingMode::Balanced`
// already means a user-facing provider-routing preference and must not be
// silently reused. A profile is a named multiplier over per-device budgets.
enum class ResourcePolicyProfile {
    Eco,
    Balanced,
    Performance,
};

QString resourcePolicyProfileName(ResourcePolicyProfile profile);

// Ordered, monotonic pressure tiers. `Unknown` is a real verdict meaning "the
// policy holds no verified pressure state", never "no pressure".
enum class ResourcePressureState {
    Unknown = 0,
    Normal = 1,
    Elevated = 2,
    High = 3,
    Critical = 4,
};

QString resourcePressureStateName(ResourcePressureState state);

// How much of the watched telemetry the evaluation could actually use.
enum class ResourceTelemetryState {
    // Every watched signal must be usable and at least one is `Measured`.
    Fresh,
    // Some watched signals are usable; at least one is `Unknown`/`Stale`.
    Degraded,
    // No watched signal is usable at all (monitoring absent or interrupted).
    Interrupted,
};

QString resourceTelemetryStateName(ResourceTelemetryState state);

// Availability of one policy input. Mirrors the P4-02 telemetry vocabulary so a
// later adapter is a direct mapping, but is declared here so policy evaluation
// does not depend on the telemetry transport implementation.
//
//   * `Measured`    — a value was sampled and is current.
//   * `Stale`       — a value exists but is past its freshness bound.
//   * `Unknown`     — the signal should be available but is not right now.
//   * `Unavailable` — supported, but absent on this host (for example no
//                     battery). Not a degradation: the signal does not apply.
//   * `Unsupported` — the platform cannot ever provide it. Not a degradation.
enum class ResourcePolicyInputState {
    Unknown,
    Measured,
    Stale,
    Unavailable,
    Unsupported,
};

QString resourcePolicyInputStateName(ResourcePolicyInputState state);

template <typename T> struct ResourcePolicyInput {
    ResourcePolicyInputState state = ResourcePolicyInputState::Unknown;
    std::optional<T> value;

    // True when a value exists and may be consumed (fresh or stale).
    bool usable() const {
        return value.has_value() && (state == ResourcePolicyInputState::Measured ||
                                     state == ResourcePolicyInputState::Stale);
    }
    // True only for a current measurement.
    bool fresh() const {
        return value.has_value() && state == ResourcePolicyInputState::Measured;
    }
    // True when the signal applies to this host at all.
    bool applicable() const {
        return state != ResourcePolicyInputState::Unavailable &&
               state != ResourcePolicyInputState::Unsupported;
    }
};

template <typename T> ResourcePolicyInput<T> measuredPolicyInput(T value) {
    ResourcePolicyInput<T> input;
    input.state = ResourcePolicyInputState::Measured;
    input.value = std::move(value);
    return input;
}

template <typename T> ResourcePolicyInput<T> stalePolicyInput(T value) {
    ResourcePolicyInput<T> input;
    input.state = ResourcePolicyInputState::Stale;
    input.value = std::move(value);
    return input;
}

template <typename T> ResourcePolicyInput<T> unknownPolicyInput() {
    ResourcePolicyInput<T> input;
    input.state = ResourcePolicyInputState::Unknown;
    return input;
}

template <typename T>
ResourcePolicyInput<T> inapplicablePolicyInput(ResourcePolicyInputState state) {
    ResourcePolicyInput<T> input;
    input.state = (state == ResourcePolicyInputState::Unavailable ||
                   state == ResourcePolicyInputState::Unsupported)
                      ? state
                      : ResourcePolicyInputState::Unsupported;
    return input;
}

// Physical power source. `Unknown` is a real state, not "AC" by default.
enum class ResourcePolicyPowerSource {
    Unknown,
    Ac,
    Battery,
};

// Ownership of the model residency the activity depends on. P4-03 only ever
// reads this; it never changes it.
enum class ResourceModelOwnership {
    // No model residency is involved in the current activity.
    None,
    // The process Sentinel started itself (`LocalModelRuntime` llama-server).
    SentinelManaged,
    // A server Sentinel does not own (Ollama, LM Studio, externally started
    // llama.cpp). Its residency is never Sentinel's to release.
    SharedExternal,
};

QString resourceModelOwnershipName(ResourceModelOwnership ownership);

// Reasons a decision was reached. Every decision carries at least one; the
// caller never has to infer behaviour from a bare state.
enum class ResourcePolicyReason {
    None,
    // The policy has no configured escalation rule yet (PD-D03/PD-D04), or the
    // configuration watches no signal this host provides. Either way the engine
    // is inert and reports `ThresholdsNotCalibrated`/`NoWatchedSignals` instead
    // of a verdict.
    ThresholdsNotCalibrated,
    NoWatchedSignals,
    TelemetryFresh,
    TelemetryDegraded,
    TelemetryInterrupted,
    MonitoringRecovered,
    MonitoringInterruptedFallback,
    EscalationRequested,
    EscalationSustained,
    EscalationSustainPending,
    DeescalationRequested,
    DeescalationBlockedByMinimumResidence,
    DeescalationBlockedByCooldown,
    CooldownActive,
    CriticalPressure,
    ForegroundRunActive,
    BackgroundRunActive,
    ForegroundAdmissionPreserved,
    BackgroundAdmissionRestricted,
    BackgroundAdmissionAllowed,
    ProfileSelectedEco,
    ProfileSelectedBalanced,
    ProfileSelectedPerformance,
    ConfiguredBudgetApplied,
    ConfiguredBudgetUncalibrated,
    SharedModelResidencyPreserved,
    ManagedModelServingActiveRunPreserved,
    ConfigurationInvalid,
};

QString resourcePolicyReasonName(ResourcePolicyReason reason);

// Actions a decision may propose. The vocabulary is closed on purpose: there is
// no "unload", "terminate", "kill" or "grant" action, so a policy decision
// cannot express a destructive or authority-granting outcome at all.
enum class ResourcePolicyAction {
    None,
    // Refuse *new* admissions. Never drops an accepted binding or active run.
    RefuseNewAdmission,
    // Restrict new *background* starts. Running work is untouched.
    RestrictBackgroundAdmission,
    // Clamp the shared tool-concurrency budget to a configured value.
    LimitToolConcurrency,
};

QString resourcePolicyActionName(ResourcePolicyAction action);

// The only ownership posture P4-03 can emit. PD-D10 (provider ownership and
// hard-stop policy) is pending, so model unload and process termination are
// structurally unexpressible rather than merely disabled by a flag.
enum class ResourceOwnershipPosture {
    PreserveAllResidency,
};

QString resourceOwnershipPostureName(ResourceOwnershipPosture posture);

// ---------------------------------------------------------------------------
// Configuration (requirements pending PD-D03 / PD-D04)
// ---------------------------------------------------------------------------

// Escalation thresholds. Every field is optional: an unset threshold means the
// signal cannot escalate, and the engine reports `ThresholdsNotCalibrated`.
// No numeric value here is an adopted product threshold — PD-D03/PD-D04 must
// calibrate them on declared hardware before any consumer is wired.
struct ResourcePolicyThresholds {
    // All fractions are 0..1. CPU is host-wide busy fraction.
    std::optional<double> cpuBusyEscalateAt;   // at-or-above
    std::optional<double> cpuBusyDeescalateAt; // at-or-below
    // Native memory-pressure level normalized to 0..1.
    std::optional<double> memoryPressureEscalateAt;
    std::optional<double> memoryPressureDeescalateAt;
    // Host available (reclaimable) bytes: low is pressure, so the comparison is
    // inverted relative to the fractions above.
    std::optional<qint64> availableMemoryEscalateAtBytes;   // at-or-below
    std::optional<qint64> availableMemoryDeescalateAtBytes; // at-or-above
    // Sentinel's own resident bytes.
    std::optional<qint64> processResidentEscalateAtBytes;   // at-or-above
    std::optional<qint64> processResidentDeescalateAtBytes; // at-or-below
    // Native thermal level strings that count as a warning (for example
    // "serious", "critical"). Empty means thermal cannot escalate.
    QStringList thermalWarningLevels;
    // Escalate while the host runs on battery. Off by default: it is an
    // escalation policy decision, not a measurement.
    bool escalateOnBattery = false;
    std::optional<double> batteryChargeEscalateAt;   // at-or-below
    std::optional<double> batteryChargeDeescalateAt; // at-or-above
    // GPU absence escalation is a configuration requirement, off by default.
    bool escalateWhenGpuUnavailable = false;
    std::optional<double> gpuUtilizationEscalateAt;   // at-or-above
    std::optional<double> gpuUtilizationDeescalateAt; // at-or-below

    // Tier contributed by each signal while crossed. Tier mapping is policy
    // semantics (not a numeric threshold) and stays `Elevated` until calibrated
    // profiles ask for more.
    ResourcePressureState cpuEscalatedTier = ResourcePressureState::Elevated;
    ResourcePressureState memoryPressureEscalatedTier = ResourcePressureState::Elevated;
    ResourcePressureState availableMemoryEscalatedTier = ResourcePressureState::Elevated;
    ResourcePressureState processResidentEscalatedTier = ResourcePressureState::Elevated;
    ResourcePressureState thermalEscalatedTier = ResourcePressureState::Elevated;
    ResourcePressureState powerEscalatedTier = ResourcePressureState::Elevated;
    ResourcePressureState gpuEscalatedTier = ResourcePressureState::Elevated;

    // True when at least one escalation rule exists. When false the engine is
    // inert and reports `ThresholdsNotCalibrated` instead of a verdict.
    bool anyEscalationRuleConfigured() const;
    // True when at least one de-escalation rule exists. Without one, a raised
    // signal can never be cleared by measurement (only by interruption).
    bool anyDeescalationRuleConfigured() const;
};

// Hysteresis timing. All values are durations in monotonic milliseconds. Zero
// means "no requirement configured"; it is not a calibrated default.
struct ResourcePolicyTiming {
    // A raised candidate tier must be observed continuously for this long
    // before it takes effect (spike rejection).
    qint64 escalationSustainMs = 0;
    // Minimum time to remain in a tier before leaving it.
    qint64 minimumResidenceMs = 0;
    // Quiet period after any transition during which no further transition may
    // occur (oscillation damping).
    qint64 cooldownMs = 0;
    // How long a fully interrupted monitoring state may hold the last verified
    // tier. After this, the engine falls back to the documented conservative
    // fallback (today's fixed limits) so a restriction cannot outlive the
    // telemetry that justified it. Zero disables the automatic fallback.
    qint64 telemetryGraceMs = 0;
};

// Numeric budget for one profile. Optional fields are unset until PD-D04; an
// uncalibrated budget proposes no numeric limit at all.
struct ResourceProfileBudget {
    bool calibrated = false;
    std::optional<int> maxConcurrentToolStarts;
    std::optional<int> maxConcurrentBackgroundStarts;
    // Multiplies configured escalation thresholds; below 1.0 restricts earlier
    // and above 1.0 later. Unset means no profile scaling.
    std::optional<double> escalationThresholdScale;
    std::optional<int> samplingIntervalMs;
    // Background starts are only admitted at or below this tier.
    ResourcePressureState backgroundAdmissionCeiling = ResourcePressureState::Critical;
};

struct ResourceProfileBudgetSet {
    ResourceProfileBudget eco;
    ResourceProfileBudget balanced;
    ResourceProfileBudget performance;

    const ResourceProfileBudget& forProfile(ResourcePolicyProfile profile) const;
    ResourceProfileBudget& forProfile(ResourcePolicyProfile profile);
};

struct ResourcePolicyConfig {
    ResourcePolicyProfile profile = ResourcePolicyProfile::Balanced;
    ResourcePolicyThresholds thresholds;
    ResourcePolicyTiming timing;
    ResourceProfileBudgetSet profiles;
};

// Returns human-readable configuration problems (empty when consistent). Used
// by tests and by a future settings surface; the engine never repairs a
// configuration silently.
QStringList validateResourcePolicyConfig(const ResourcePolicyConfig& config);

// ---------------------------------------------------------------------------
// Inputs
// ---------------------------------------------------------------------------

// Everything the engine may read for one evaluation. Telemetry-scoped inputs are
// optional-with-availability; activity and ownership are facts supplied by the
// runtime that already owns them (AgentRuntime, ModelService/LocalModelRuntime)
// and are not measurements.
struct ResourcePolicyInputs {
    // Host-wide telemetry (P4-02 `HostResourceSnapshot` mapping target).
    // Host-wide CPU busy fraction. The P4-02 snapshot exposes only
    // Sentinel-process CPU, so a host-wide sampler (or a documented
    // process-scoped substitute) is a P4-04 requirement and this input stays
    // `Unknown` until one exists.
    ResourcePolicyInput<double> cpuBusyFraction;     // 0..1 of total capacity
    ResourcePolicyInput<qint64> hostAvailableBytes;  // reclaimable bytes
    ResourcePolicyInput<double> memoryPressureLevel; // native indicator 0..1
    ResourcePolicyInput<QString> thermalLevel;       // native level string
    ResourcePolicyInput<ResourcePolicyPowerSource> powerSource;
    ResourcePolicyInput<bool> batteryPresent;
    ResourcePolicyInput<double> batteryChargeFraction; // 0..1
    ResourcePolicyInput<bool> gpuAvailable;
    ResourcePolicyInput<double> gpuUtilizationFraction; // 0..1, per-device max
    // Sentinel-process telemetry.
    ResourcePolicyInput<qint64> processResidentBytes;
    ResourcePolicyInput<double> processCpuFractionOfCore;

    // Activity facts (not telemetry).
    int activeForegroundRuns = 0;
    int activeBackgroundRuns = 0;

    // Ownership facts (not telemetry). Read-only for this contract.
    ResourceModelOwnership modelOwnership = ResourceModelOwnership::None;
    bool modelServingActiveRun = false;
};

// ---------------------------------------------------------------------------
// Decision
// ---------------------------------------------------------------------------

struct ResourcePolicyDecision {
    ResourcePolicyProfile profile = ResourcePolicyProfile::Balanced;
    ResourcePressureState state = ResourcePressureState::Unknown;
    ResourceTelemetryState telemetry = ResourceTelemetryState::Interrupted;
    ResourcePolicyReason primaryReason = ResourcePolicyReason::None;
    QList<ResourcePolicyReason> reasons;
    QList<ResourcePolicyAction> actions;

    // Only ever `PreserveAllResidency` in P4-03.
    ResourceOwnershipPosture ownershipPosture = ResourceOwnershipPosture::PreserveAllResidency;

    // Admission outcome. Foreground admission is never withdrawn by pressure:
    // an accepted run keeps its binding, and cancellation keeps absolute
    // priority over any admission change.
    bool refuseNewAdmission = false;
    bool restrictBackgroundAdmission = false;
    bool foregroundAdmissionAllowed = true;

    // Advisory numeric limits. Empty means "no configured value": the consumer
    // must keep the existing fixed limits.
    std::optional<int> toolConcurrencyLimit;
    std::optional<int> backgroundConcurrencyLimit;

    bool degraded = false;             // some watched signal unusable
    bool thresholdsCalibrated = false; // at least one escalation rule configured

    // Activity/ownership observability, echoed for the caller's diagnostics.
    bool foregroundRunsActive = false;
    bool backgroundRunsActive = false;
    ResourceModelOwnership modelOwnership = ResourceModelOwnership::None;
    bool modelServingActiveRun = false;

    qint64 evaluatedAtMonotonicMs = 0;
    // True when this evaluation changed the effective tier.
    bool isTransition = false;
};

// Safe, deterministic, non-localized summary used by tests and diagnostics.
QString resourcePolicyDecisionSummary(const ResourcePolicyDecision& decision);

// Structural security invariant. It is a compile-time constant because the
// decision type cannot express an authority change: it has no permission,
// approval, sandbox, credential or process field, and its action vocabulary has
// no destructive verb. Pinned by test so the claim cannot silently rot.
[[nodiscard]] constexpr bool resourcePolicyDecisionPreservesSecurity() {
    return true;
}

// ---------------------------------------------------------------------------
// Engine
// ---------------------------------------------------------------------------

// Deterministic, side-effect-free evaluator. It holds only hysteresis state
// (current tier and timing anchors); it performs no IO, owns no thread, starts
// no timer and never touches a process, permission or provider.
//
// Time is an explicit argument so behaviour is fully reproducible with an
// injected clock in tests; the engine itself only ever reads the caller's
// monotonic milliseconds.
class ResourcePolicyEngine final {
public:
    explicit ResourcePolicyEngine(ResourcePolicyConfig config = {});

    const ResourcePolicyConfig& config() const {
        return config_;
    }
    // Replaces the configuration and resets hysteresis state (a profile change
    // is not a pressure transition).
    void setConfig(ResourcePolicyConfig config);

    // Configuration problems reported by `validateResourcePolicyConfig`.
    QStringList configurationProblems() const {
        return configurationProblems_;
    }

    // Resets hysteresis state to the initial `Unknown` tier.
    void reset();

    ResourcePressureState pressureState() const {
        return state_;
    }
    qint64 stateEnteredAtMonotonicMs() const {
        return stateEnteredAtMs_;
    }

    // Evaluates one snapshot of inputs. Pure with respect to everything outside
    // this engine.
    ResourcePolicyDecision evaluate(const ResourcePolicyInputs& inputs, qint64 nowMonotonicMs);

private:
    ResourceProfileBudget selectedBudget() const;

    ResourcePolicyConfig config_;
    QStringList configurationProblems_;

    ResourcePressureState state_ = ResourcePressureState::Unknown;
    qint64 stateEnteredAtMs_ = 0;
    qint64 cooldownUntilMs_ = 0;
    // Pending escalation candidate and when it was first observed.
    ResourcePressureState pendingTier_ = ResourcePressureState::Unknown;
    qint64 pendingSinceMs_ = 0;
    // Continuous monitoring-interruption anchor (0 when telemetry is usable).
    qint64 interruptedSinceMs_ = 0;
    // Last tier verified while telemetry was usable.
    ResourcePressureState lastVerifiedTier_ = ResourcePressureState::Unknown;
};

} // namespace sentinel::core
