// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// P4-03 adaptive resource policy foundation: deterministic fixtures only.
//
// Every numeric value in this file is a FIXTURE VALUE chosen to make the state
// machine observable. None of them is an adopted product threshold: PD-D03
// (hardware/environment) and PD-D04 (calibrated budgets) are pending. The
// production defaults stay uncalibrated and inert, which the first cases pin.
//
// No test reads the host's real battery, temperature, load or process list, and
// no test starts, stops or inspects any process.

#include "sentinel/core/policy/ResourcePolicy.h"

#include <QtTest>

using sentinel::core::ResourceModelOwnership;
using sentinel::core::ResourceOwnershipPosture;
using sentinel::core::ResourcePolicyAction;
using sentinel::core::ResourcePolicyConfig;
using sentinel::core::ResourcePolicyDecision;
using sentinel::core::ResourcePolicyEngine;
using sentinel::core::ResourcePolicyInputs;
using sentinel::core::ResourcePolicyPowerSource;
using sentinel::core::ResourcePolicyProfile;
using sentinel::core::ResourcePolicyReason;
using sentinel::core::ResourcePressureState;
using sentinel::core::ResourceTelemetryState;

namespace {

// Injected monotonic clock. The origin is arbitrary and non-zero so the engine's
// "not yet interrupted" sentinel is never confused with a real timestamp.
class FakeClock {
public:
    qint64 nowMs() const {
        return nowMs_;
    }
    void advance(qint64 deltaMs) {
        nowMs_ += deltaMs;
    }

private:
    qint64 nowMs_ = 1000;
};

// ---------------------------------------------------------------------------
// FIXTURE VALUES (not product thresholds; pending PD-D03/PD-D04)
// ---------------------------------------------------------------------------
constexpr double kFixtureCpuEscalate = 0.80;
constexpr double kFixtureCpuDeescalate = 0.50;
constexpr double kFixtureCpuIdle = 0.10;
constexpr qint64 kFixtureSustainMs = 2000;
constexpr qint64 kFixtureResidenceMs = 1000;
constexpr qint64 kFixtureCooldownMs = 1000;
constexpr qint64 kFixtureGraceMs = 3000;
constexpr int kFixtureEcoToolLimit = 1;
constexpr int kFixtureEcoBackgroundLimit = 0;
constexpr int kFixtureBalancedToolLimit = 2;
constexpr int kFixtureBalancedBackgroundLimit = 1;

// Calibrated-by-fixture configuration. The profile-specific numeric budgets are
// fixture values used only to prove ordering and gating behaviour.
ResourcePolicyConfig
fixtureConfig(ResourcePolicyProfile profile = ResourcePolicyProfile::Balanced) {
    ResourcePolicyConfig config;
    config.profile = profile;
    config.thresholds.cpuBusyEscalateAt = kFixtureCpuEscalate;
    config.thresholds.cpuBusyDeescalateAt = kFixtureCpuDeescalate;
    config.timing.escalationSustainMs = kFixtureSustainMs;
    config.timing.minimumResidenceMs = kFixtureResidenceMs;
    config.timing.cooldownMs = kFixtureCooldownMs;
    config.timing.telemetryGraceMs = kFixtureGraceMs;

    config.profiles.eco.calibrated = true;
    config.profiles.eco.maxConcurrentToolStarts = kFixtureEcoToolLimit;
    config.profiles.eco.maxConcurrentBackgroundStarts = kFixtureEcoBackgroundLimit;
    config.profiles.eco.backgroundAdmissionCeiling = ResourcePressureState::Normal;
    config.profiles.balanced.calibrated = true;
    config.profiles.balanced.maxConcurrentToolStarts = kFixtureBalancedToolLimit;
    config.profiles.balanced.maxConcurrentBackgroundStarts = kFixtureBalancedBackgroundLimit;
    config.profiles.balanced.backgroundAdmissionCeiling = ResourcePressureState::Elevated;
    config.profiles.performance.calibrated = true;
    config.profiles.performance.maxConcurrentToolStarts = kFixtureBalancedToolLimit * 2;
    config.profiles.performance.maxConcurrentBackgroundStarts = kFixtureBalancedBackgroundLimit * 2;
    config.profiles.performance.backgroundAdmissionCeiling = ResourcePressureState::High;
    return config;
}

// Same fixture configuration with no hysteresis timing, used where a test wants
// a single evaluation to land on a tier directly.
ResourcePolicyConfig
immediateConfig(ResourcePolicyProfile profile = ResourcePolicyProfile::Balanced,
                ResourcePressureState cpuTier = ResourcePressureState::Elevated) {
    ResourcePolicyConfig config = fixtureConfig(profile);
    config.thresholds.cpuEscalatedTier = cpuTier;
    config.timing.escalationSustainMs = 0;
    config.timing.minimumResidenceMs = 0;
    config.timing.cooldownMs = 0;
    return config;
}

ResourcePolicyInputs idleInputs() {
    ResourcePolicyInputs inputs;
    inputs.cpuBusyFraction = sentinel::core::measuredPolicyInput(kFixtureCpuIdle);
    return inputs;
}

ResourcePolicyInputs pressuredInputs(double cpu = kFixtureCpuEscalate + 0.10) {
    ResourcePolicyInputs inputs;
    inputs.cpuBusyFraction = sentinel::core::measuredPolicyInput(cpu);
    return inputs;
}

void expectState(const ResourcePolicyDecision& decision, ResourcePressureState expected,
                 const char* what) {
    QVERIFY2(decision.state == expected,
             qPrintable(QStringLiteral("%1: expected state %2 but got %3 (%4)")
                            .arg(QString::fromUtf8(what),
                                 sentinel::core::resourcePressureStateName(expected),
                                 sentinel::core::resourcePressureStateName(decision.state),
                                 sentinel::core::resourcePolicyDecisionSummary(decision))));
}

void expectTelemetry(const ResourcePolicyDecision& decision, ResourceTelemetryState expected,
                     const char* what) {
    QVERIFY2(decision.telemetry == expected,
             qPrintable(QStringLiteral("%1: expected telemetry %2 but got %3 (%4)")
                            .arg(QString::fromUtf8(what),
                                 sentinel::core::resourceTelemetryStateName(expected),
                                 sentinel::core::resourceTelemetryStateName(decision.telemetry),
                                 sentinel::core::resourcePolicyDecisionSummary(decision))));
}

void expectReason(const ResourcePolicyDecision& decision, ResourcePolicyReason expected,
                  const char* what) {
    QVERIFY2(decision.reasons.contains(expected),
             qPrintable(QStringLiteral("%1: expected reason %2 in %3")
                            .arg(QString::fromUtf8(what),
                                 sentinel::core::resourcePolicyReasonName(expected),
                                 sentinel::core::resourcePolicyDecisionSummary(decision))));
}

} // namespace

class ResourcePolicyTest final : public QObject {
    Q_OBJECT
private slots:
    void productionDefaultsAreInertAndNeverHealthy();
    void uncalibratedNumericsAreNeverProposed();
    void normalTelemetryYieldsNormalWithoutRestriction();
    void sustainedPressureEscalatesAfterSustainWindow();
    void shortLivedSpikeDoesNotEscalate();
    void coolingDownRespectsResidenceAndCooldown();
    void staleSnapshotNeverClearsAndNeverRaises();
    void unknownInputIsNeverHealthyAndFabricatesNoZero();
    void unavailableAndUnsupportedAreNotDegradations();
    void unknownBatteryIsNotAcAndBatteryEscalatesOnlyWhenConfigured();
    void thermalWarningEscalatesAndClearsOnMeasuredLevel();
    void foregroundAgentRunIsNeverWithdrawn();
    void competingBackgroundTasksAreBoundedNotPreempted();
    void sharedProviderOwnershipIsPreserved();
    void managedModelServingRunIsPreserved();
    void cancelledTaskLeavesNoResidualPolicyState();
    void permissionInvarianceAcrossProfilesAndTiers();
    void rapidStateTransitionsAreDamped();
    void monitorShutdownFallsBackAfterGraceAndRecovers();
    void monitoringInterruptionWithoutGraceHoldsRestriction();
    void profileSemanticsOrderWithoutProductThresholds();
    void configurationValidationRejectsInconsistentThresholds();
    void namesAreStableAndNonLocalized();
};

// A policy with no configured escalation rule must never report "healthy": it
// reports Unknown with an explicit reason and proposes nothing.
void ResourcePolicyTest::productionDefaultsAreInertAndNeverHealthy() {
    ResourcePolicyEngine engine{}; // production default configuration
    QVERIFY(engine.configurationProblems().isEmpty());

    const auto decision = engine.evaluate(idleInputs(), 5000);
    expectState(decision, ResourcePressureState::Unknown, "unconfigured policy");
    expectReason(decision, ResourcePolicyReason::ThresholdsNotCalibrated, "unconfigured policy");
    expectReason(decision, ResourcePolicyReason::NoWatchedSignals, "unconfigured policy");
    QVERIFY(decision.degraded);
    QVERIFY(!decision.thresholdsCalibrated);
    QVERIFY(!decision.refuseNewAdmission);
    QVERIFY(!decision.restrictBackgroundAdmission);
    QVERIFY(decision.foregroundAdmissionAllowed);
    QCOMPARE(decision.toolConcurrencyLimit.has_value(), false);
    QCOMPARE(decision.actions.size(), 1);
    QCOMPARE(decision.actions.first(), ResourcePolicyAction::None);
    QCOMPARE(decision.ownershipPosture, ResourceOwnershipPosture::PreserveAllResidency);
    QCOMPARE(decision.primaryReason, ResourcePolicyReason::ThresholdsNotCalibrated);
}

void ResourcePolicyTest::uncalibratedNumericsAreNeverProposed() {
    for (const auto tier : {ResourcePressureState::Elevated, ResourcePressureState::High,
                            ResourcePressureState::Critical}) {
        ResourcePolicyConfig config = immediateConfig(ResourcePolicyProfile::Eco, tier);
        config.profiles.eco.calibrated = false;
        config.profiles.eco.maxConcurrentToolStarts.reset();
        config.profiles.eco.maxConcurrentBackgroundStarts.reset();
        ResourcePolicyEngine engine(config);
        const auto decision = engine.evaluate(pressuredInputs(), 5000);
        expectState(decision, tier, "uncalibrated numeric budget");
        QCOMPARE(decision.toolConcurrencyLimit.has_value(), false);
        QCOMPARE(decision.backgroundConcurrencyLimit.has_value(), false);
        expectReason(decision, ResourcePolicyReason::ConfiguredBudgetUncalibrated,
                     "uncalibrated numeric budget");
    }
}

void ResourcePolicyTest::normalTelemetryYieldsNormalWithoutRestriction() {
    FakeClock clock;
    ResourcePolicyEngine engine(fixtureConfig());

    const auto decision = engine.evaluate(idleInputs(), clock.nowMs());
    expectState(decision, ResourcePressureState::Normal, "idle host");
    expectTelemetry(decision, ResourceTelemetryState::Fresh, "idle host");
    expectReason(decision, ResourcePolicyReason::TelemetryFresh, "idle host");
    QVERIFY(!decision.degraded);
    QVERIFY(decision.thresholdsCalibrated);
    QVERIFY(!decision.refuseNewAdmission);
    QVERIFY(!decision.restrictBackgroundAdmission);
    expectReason(decision, ResourcePolicyReason::BackgroundAdmissionAllowed, "idle host");
    expectReason(decision, ResourcePolicyReason::ForegroundAdmissionPreserved, "idle host");
    QCOMPARE(decision.toolConcurrencyLimit.value_or(-1), kFixtureBalancedToolLimit);
    expectReason(decision, ResourcePolicyReason::ProfileSelectedBalanced, "idle host");
    expectReason(decision, ResourcePolicyReason::ConfiguredBudgetApplied, "idle host");
}

void ResourcePolicyTest::sustainedPressureEscalatesAfterSustainWindow() {
    FakeClock clock;
    ResourcePolicyEngine engine(fixtureConfig());

    expectState(engine.evaluate(idleInputs(), clock.nowMs()), ResourcePressureState::Normal,
                "baseline");

    clock.advance(500);
    auto decision = engine.evaluate(pressuredInputs(), clock.nowMs());
    expectState(decision, ResourcePressureState::Normal, "pressure inside sustain window");
    expectReason(decision, ResourcePolicyReason::EscalationSustainPending, "sustain window");
    QVERIFY(!decision.isTransition);

    clock.advance(kFixtureSustainMs);
    decision = engine.evaluate(pressuredInputs(), clock.nowMs());
    expectState(decision, ResourcePressureState::Elevated, "sustained pressure");
    expectReason(decision, ResourcePolicyReason::EscalationSustained, "sustained pressure");
    // The Balanced fixture ceiling is Elevated, so background starts stay open at
    // this tier; Eco/Performance differences are pinned separately.
    expectReason(decision, ResourcePolicyReason::BackgroundAdmissionAllowed,
                 "balanced ceiling at Elevated");
    QCOMPARE(decision.toolConcurrencyLimit.value_or(-1), kFixtureBalancedToolLimit);
    QVERIFY(decision.isTransition);
}

void ResourcePolicyTest::shortLivedSpikeDoesNotEscalate() {
    FakeClock clock;
    ResourcePolicyEngine engine(fixtureConfig());

    expectState(engine.evaluate(idleInputs(), clock.nowMs()), ResourcePressureState::Normal,
                "spike baseline");

    clock.advance(500);
    expectState(engine.evaluate(pressuredInputs(), clock.nowMs()), ResourcePressureState::Normal,
                "spike start");
    clock.advance(500);
    auto decision = engine.evaluate(pressuredInputs(), clock.nowMs());
    expectState(decision, ResourcePressureState::Normal, "spike middle");
    QVERIFY(!decision.reasons.contains(ResourcePolicyReason::EscalationSustained));

    clock.advance(500);
    decision = engine.evaluate(idleInputs(), clock.nowMs());
    expectState(decision, ResourcePressureState::Normal, "spike ended");
    QVERIFY(!decision.isTransition);
    QVERIFY(!decision.reasons.contains(ResourcePolicyReason::EscalationSustained));
}

void ResourcePolicyTest::coolingDownRespectsResidenceAndCooldown() {
    FakeClock clock;
    ResourcePolicyConfig config = fixtureConfig();
    // FIXTURE VALUES: cooldown deliberately longer than the residence time so
    // both ordering rules are observable in one sequence.
    config.timing.minimumResidenceMs = kFixtureResidenceMs;
    config.timing.cooldownMs = 2500;
    ResourcePolicyEngine engine(config);

    expectState(engine.evaluate(idleInputs(), clock.nowMs()), ResourcePressureState::Normal,
                "cooling baseline");
    clock.advance(500);
    engine.evaluate(pressuredInputs(), clock.nowMs());
    clock.advance(kFixtureSustainMs);
    expectState(engine.evaluate(pressuredInputs(), clock.nowMs()), ResourcePressureState::Elevated,
                "cooling escalation");

    // Pressure clears immediately, but the state must not leave before the
    // configured residence time.
    clock.advance(100);
    auto decision = engine.evaluate(idleInputs(), clock.nowMs());
    expectState(decision, ResourcePressureState::Elevated, "minimum residence");
    expectReason(decision, ResourcePolicyReason::DeescalationBlockedByMinimumResidence,
                 "minimum residence");

    // Once residence has elapsed, the cooldown still holds the tier.
    clock.advance(kFixtureResidenceMs);
    decision = engine.evaluate(idleInputs(), clock.nowMs());
    expectState(decision, ResourcePressureState::Elevated, "cooldown block");
    expectReason(decision, ResourcePolicyReason::DeescalationBlockedByCooldown, "cooldown block");

    clock.advance(2500);
    decision = engine.evaluate(idleInputs(), clock.nowMs());
    expectState(decision, ResourcePressureState::Normal, "cooled down");
    expectReason(decision, ResourcePolicyReason::DeescalationRequested, "cooled down");
    QVERIFY(decision.isTransition);
}

void ResourcePolicyTest::staleSnapshotNeverClearsAndNeverRaises() {
    FakeClock clock;
    ResourcePolicyEngine engine(fixtureConfig());

    expectState(engine.evaluate(idleInputs(), clock.nowMs()), ResourcePressureState::Normal,
                "stale baseline");
    clock.advance(500);
    engine.evaluate(pressuredInputs(), clock.nowMs());
    clock.advance(kFixtureSustainMs);
    expectState(engine.evaluate(pressuredInputs(), clock.nowMs()), ResourcePressureState::Elevated,
                "stale escalation");

    // A stale value above the escalation threshold must not raise further, and a
    // stale value below the clear threshold must not clear: stale is neither
    // healthy nor actionable.
    ResourcePolicyInputs staleHigh;
    staleHigh.cpuBusyFraction = sentinel::core::stalePolicyInput(0.99);
    clock.advance(10);
    auto decision = engine.evaluate(staleHigh, clock.nowMs());
    expectState(decision, ResourcePressureState::Elevated, "stale high");
    expectTelemetry(decision, ResourceTelemetryState::Degraded, "stale high");
    expectReason(decision, ResourcePolicyReason::TelemetryDegraded, "stale high");
    QVERIFY(decision.degraded);
    QVERIFY(!decision.isTransition);

    ResourcePolicyInputs staleLow;
    staleLow.cpuBusyFraction = sentinel::core::stalePolicyInput(0.01);
    clock.advance(kFixtureResidenceMs * 2);
    decision = engine.evaluate(staleLow, clock.nowMs());
    expectState(decision, ResourcePressureState::Elevated, "stale low");
    QVERIFY(!decision.reasons.contains(ResourcePolicyReason::DeescalationRequested));

    // A fresh, truthful clear still works afterwards.
    clock.advance(kFixtureResidenceMs * 2);
    decision = engine.evaluate(idleInputs(), clock.nowMs());
    expectState(decision, ResourcePressureState::Normal, "recovered clear");
}

void ResourcePolicyTest::unknownInputIsNeverHealthyAndFabricatesNoZero() {
    const auto unknownCpu = sentinel::core::unknownPolicyInput<double>();
    QCOMPARE(unknownCpu.state, sentinel::core::ResourcePolicyInputState::Unknown);
    QCOMPARE(unknownCpu.value.has_value(), false);

    FakeClock clock;
    ResourcePolicyEngine engine(fixtureConfig());

    expectState(engine.evaluate(idleInputs(), clock.nowMs()), ResourcePressureState::Normal,
                "unknown baseline");
    clock.advance(500);
    engine.evaluate(pressuredInputs(), clock.nowMs());
    clock.advance(kFixtureSustainMs);
    expectState(engine.evaluate(pressuredInputs(), clock.nowMs()), ResourcePressureState::Elevated,
                "unknown escalation");

    // Telemetry disappears. The verified tier is held: a missing measurement is
    // never read as "healthy" and never clears a restriction.
    const ResourcePolicyInputs missing;
    clock.advance(10);
    auto decision = engine.evaluate(missing, clock.nowMs());
    expectState(decision, ResourcePressureState::Elevated, "missing telemetry");
    expectTelemetry(decision, ResourceTelemetryState::Interrupted, "missing telemetry");
    expectReason(decision, ResourcePolicyReason::TelemetryInterrupted, "missing telemetry");
    QVERIFY(!decision.reasons.contains(ResourcePolicyReason::DeescalationRequested));
    QVERIFY(decision.degraded);
}

void ResourcePolicyTest::unavailableAndUnsupportedAreNotDegradations() {
    ResourcePolicyConfig config = fixtureConfig();
    config.thresholds.thermalWarningLevels = {QStringLiteral("critical")};
    ResourcePolicyEngine engine(config);

    // The host provides CPU telemetry but no thermal level at all (macOS/Windows
    // report thermal as Unsupported). An inapplicable signal is excluded from
    // evidence: it is neither a degradation nor a fabricated "cool" reading.
    ResourcePolicyInputs inputs = idleInputs();
    inputs.thermalLevel = sentinel::core::inapplicablePolicyInput<QString>(
        sentinel::core::ResourcePolicyInputState::Unsupported);

    const auto decision = engine.evaluate(inputs, 5000);
    expectState(decision, ResourcePressureState::Normal,
                "unsupported thermal is not a degradation");
    expectTelemetry(decision, ResourceTelemetryState::Fresh, "unsupported thermal");
    QVERIFY(!decision.degraded);

    // A supported-but-absent signal (a desktop without a battery) is equally not
    // a degradation when the platform reports it that way.
    ResourcePolicyConfig powerConfig = fixtureConfig();
    powerConfig.thresholds.escalateOnBattery = true;
    ResourcePolicyEngine powerEngine(powerConfig);
    ResourcePolicyInputs absent = idleInputs();
    absent.batteryPresent = sentinel::core::inapplicablePolicyInput<bool>(
        sentinel::core::ResourcePolicyInputState::Unavailable);
    absent.powerSource = sentinel::core::inapplicablePolicyInput<ResourcePolicyPowerSource>(
        sentinel::core::ResourcePolicyInputState::Unavailable);
    const auto absentDecision = powerEngine.evaluate(absent, 5000);
    expectState(absentDecision, ResourcePressureState::Normal, "absent battery");
    QVERIFY(!absentDecision.degraded);

    // A host that provides none of the watched signals leaves the policy inert
    // rather than "healthy".
    ResourcePolicyConfig thermalOnly = fixtureConfig();
    thermalOnly.thresholds.cpuBusyEscalateAt.reset();
    thermalOnly.thresholds.cpuBusyDeescalateAt.reset();
    ResourcePolicyEngine inertEngine(thermalOnly);
    const auto inertDecision = inertEngine.evaluate(inputs, 5000);
    expectState(inertDecision, ResourcePressureState::Unknown, "no applicable signal");
    expectReason(inertDecision, ResourcePolicyReason::NoWatchedSignals, "no applicable signal");
}

void ResourcePolicyTest::unknownBatteryIsNotAcAndBatteryEscalatesOnlyWhenConfigured() {
    ResourcePolicyConfig config = immediateConfig();
    config.thresholds.escalateOnBattery = true;
    ResourcePolicyEngine engine(config);

    ResourcePolicyInputs unknownPower = idleInputs();
    unknownPower.powerSource = sentinel::core::unknownPolicyInput<ResourcePolicyPowerSource>();
    auto decision = engine.evaluate(unknownPower, 5000);
    expectState(decision, ResourcePressureState::Normal, "unknown power source");
    expectTelemetry(decision, ResourceTelemetryState::Degraded, "unknown power source");
    QVERIFY(decision.degraded);

    ResourcePolicyInputs onAc = idleInputs();
    onAc.powerSource = sentinel::core::measuredPolicyInput(ResourcePolicyPowerSource::Ac);
    decision = engine.evaluate(onAc, 5000);
    expectState(decision, ResourcePressureState::Normal, "AC power");
    QVERIFY(!decision.degraded);

    ResourcePolicyInputs onBattery = idleInputs();
    onBattery.powerSource = sentinel::core::measuredPolicyInput(ResourcePolicyPowerSource::Battery);
    decision = engine.evaluate(onBattery, 5000);
    expectState(decision, ResourcePressureState::Elevated, "battery power");
    expectReason(decision, ResourcePolicyReason::EscalationRequested, "battery power");

    // Without the explicit escalation policy, battery operation is not pressure.
    ResourcePolicyConfig passive = immediateConfig();
    passive.thresholds.escalateOnBattery = false;
    ResourcePolicyEngine passiveEngine(passive);
    expectState(passiveEngine.evaluate(onBattery, 5000), ResourcePressureState::Normal,
                "battery without escalation policy");
}

void ResourcePolicyTest::thermalWarningEscalatesAndClearsOnMeasuredLevel() {
    ResourcePolicyConfig config = immediateConfig();
    config.thresholds.thermalWarningLevels = {QStringLiteral("serious"),
                                              QStringLiteral("critical")};
    ResourcePolicyEngine engine(config);

    ResourcePolicyInputs nominal = idleInputs();
    nominal.thermalLevel = sentinel::core::measuredPolicyInput(QStringLiteral("nominal"));
    expectState(engine.evaluate(nominal, 1000), ResourcePressureState::Normal, "nominal thermal");

    ResourcePolicyInputs serious = idleInputs();
    serious.thermalLevel = sentinel::core::measuredPolicyInput(QStringLiteral("serious"));
    auto decision = engine.evaluate(serious, 2000);
    expectState(decision, ResourcePressureState::Elevated, "thermal warning");
    expectReason(decision, ResourcePolicyReason::EscalationRequested, "thermal warning");

    // The thermal level is a native string, never a fabricated numeric score.
    const auto celsius = sentinel::core::unknownPolicyInput<double>();
    QCOMPARE(celsius.value.has_value(), false);

    ResourcePolicyInputs recovered = idleInputs();
    recovered.thermalLevel = sentinel::core::measuredPolicyInput(QStringLiteral("nominal"));
    decision = engine.evaluate(recovered, 4000);
    expectState(decision, ResourcePressureState::Normal, "thermal cleared");
    expectReason(decision, ResourcePolicyReason::DeescalationRequested, "thermal cleared");
}

void ResourcePolicyTest::foregroundAgentRunIsNeverWithdrawn() {
    ResourcePolicyEngine engine(
        immediateConfig(ResourcePolicyProfile::Balanced, ResourcePressureState::Critical));

    ResourcePolicyInputs inputs = pressuredInputs();
    inputs.activeForegroundRuns = 1;
    inputs.activeBackgroundRuns = 2;

    const auto decision = engine.evaluate(inputs, 5000);
    expectState(decision, ResourcePressureState::Critical, "critical pressure");
    expectReason(decision, ResourcePolicyReason::CriticalPressure, "critical pressure");
    QVERIFY(decision.refuseNewAdmission);
    QVERIFY(decision.restrictBackgroundAdmission);
    // Active foreground work keeps its binding; only new admissions are refused.
    QVERIFY(decision.foregroundAdmissionAllowed);
    expectReason(decision, ResourcePolicyReason::ForegroundRunActive, "foreground run");
    expectReason(decision, ResourcePolicyReason::BackgroundRunActive, "background runs");
    expectReason(decision, ResourcePolicyReason::ForegroundAdmissionPreserved, "foreground run");
    QVERIFY(decision.actions.contains(ResourcePolicyAction::RefuseNewAdmission));
    QVERIFY(decision.actions.contains(ResourcePolicyAction::RestrictBackgroundAdmission));
}

void ResourcePolicyTest::competingBackgroundTasksAreBoundedNotPreempted() {
    ResourcePolicyEngine engine(immediateConfig(ResourcePolicyProfile::Eco));

    ResourcePolicyInputs inputs = pressuredInputs();
    inputs.activeBackgroundRuns = 3;

    const auto decision = engine.evaluate(inputs, 5000);
    expectState(decision, ResourcePressureState::Elevated, "eco background pressure");
    QVERIFY(decision.restrictBackgroundAdmission);
    expectReason(decision, ResourcePolicyReason::BackgroundAdmissionRestricted,
                 "eco background pressure");
    // Running background work is not preempted; only new starts are held, and the
    // limit is the configured fixture budget.
    QCOMPARE(decision.backgroundConcurrencyLimit.value_or(-1), kFixtureEcoBackgroundLimit);
    QCOMPARE(decision.toolConcurrencyLimit.value_or(-1), kFixtureEcoToolLimit);
    QVERIFY(decision.actions.contains(ResourcePolicyAction::LimitToolConcurrency));
}

void ResourcePolicyTest::sharedProviderOwnershipIsPreserved() {
    ResourcePolicyEngine engine(
        immediateConfig(ResourcePolicyProfile::Performance, ResourcePressureState::Critical));

    ResourcePolicyInputs inputs = pressuredInputs();
    inputs.modelOwnership = ResourceModelOwnership::SharedExternal;
    inputs.modelServingActiveRun = true;
    inputs.activeForegroundRuns = 1;

    const auto decision = engine.evaluate(inputs, 5000);
    QCOMPARE(decision.ownershipPosture, ResourceOwnershipPosture::PreserveAllResidency);
    expectReason(decision, ResourcePolicyReason::SharedModelResidencyPreserved,
                 "shared external runtime");
    QVERIFY(
        !decision.reasons.contains(ResourcePolicyReason::ManagedModelServingActiveRunPreserved));
}

void ResourcePolicyTest::managedModelServingRunIsPreserved() {
    ResourcePolicyEngine engine(
        immediateConfig(ResourcePolicyProfile::Balanced, ResourcePressureState::High));

    ResourcePolicyInputs inputs = pressuredInputs();
    inputs.modelOwnership = ResourceModelOwnership::SentinelManaged;
    inputs.modelServingActiveRun = true;
    inputs.activeForegroundRuns = 1;

    const auto decision = engine.evaluate(inputs, 5000);
    QCOMPARE(decision.ownershipPosture, ResourceOwnershipPosture::PreserveAllResidency);
    expectReason(decision, ResourcePolicyReason::ManagedModelServingActiveRunPreserved,
                 "managed model serving an active run");

    // The same policy proposes no lifecycle change even when the managed model is
    // idle: unloading is a PD-D10 decision, not a pressure consequence.
    inputs.modelServingActiveRun = false;
    const auto idleDecision = engine.evaluate(inputs, 6000);
    QCOMPARE(idleDecision.ownershipPosture, ResourceOwnershipPosture::PreserveAllResidency);
}

void ResourcePolicyTest::cancelledTaskLeavesNoResidualPolicyState() {
    FakeClock clock;
    ResourcePolicyEngine engine(immediateConfig());

    ResourcePolicyInputs running = pressuredInputs();
    running.activeForegroundRuns = 1;
    auto decision = engine.evaluate(running, clock.nowMs());
    expectState(decision, ResourcePressureState::Elevated, "running task");
    expectReason(decision, ResourcePolicyReason::ForegroundRunActive, "running task");

    // Cancellation is a runtime concern that keeps absolute priority; it is not a
    // pressure input. The policy must not invent a state change for it, must not
    // retain a run-scoped reason, and must expose no cancellation action.
    ResourcePolicyInputs cancelled = pressuredInputs();
    clock.advance(10);
    decision = engine.evaluate(cancelled, clock.nowMs());
    expectState(decision, ResourcePressureState::Elevated, "cancelled task");
    QVERIFY(!decision.reasons.contains(ResourcePolicyReason::ForegroundRunActive));
    QVERIFY(!decision.reasons.contains(ResourcePolicyReason::BackgroundRunActive));
    for (const auto& action : decision.actions) {
        QVERIFY(action == ResourcePolicyAction::None ||
                action == ResourcePolicyAction::RefuseNewAdmission ||
                action == ResourcePolicyAction::RestrictBackgroundAdmission ||
                action == ResourcePolicyAction::LimitToolConcurrency);
    }
}

void ResourcePolicyTest::permissionInvarianceAcrossProfilesAndTiers() {
    const QList<ResourcePolicyProfile> profiles = {ResourcePolicyProfile::Eco,
                                                   ResourcePolicyProfile::Balanced,
                                                   ResourcePolicyProfile::Performance};

    const auto assertSafeDecision = [](const ResourcePolicyDecision& decision) {
        QVERIFY(sentinel::core::resourcePolicyDecisionPreservesSecurity());
        QCOMPARE(decision.ownershipPosture, ResourceOwnershipPosture::PreserveAllResidency);
        QVERIFY(decision.foregroundAdmissionAllowed);
        for (const auto& action : decision.actions) {
            QVERIFY(action == ResourcePolicyAction::None ||
                    action == ResourcePolicyAction::RefuseNewAdmission ||
                    action == ResourcePolicyAction::RestrictBackgroundAdmission ||
                    action == ResourcePolicyAction::LimitToolConcurrency);
        }
        for (const auto& reason : decision.reasons) {
            const QString name = sentinel::core::resourcePolicyReasonName(reason);
            QVERIFY(!name.contains(QStringLiteral("Permission")));
            QVERIFY(!name.contains(QStringLiteral("Approval")));
            QVERIFY(!name.contains(QStringLiteral("Sandbox")));
            QVERIFY(!name.contains(QStringLiteral("Grant")));
            QVERIFY(!name.contains(QStringLiteral("Delegat")));
        }
    };

    for (const auto profile : profiles) {
        // Normal tier: measured, unpressured telemetry.
        ResourcePolicyEngine idleEngine(immediateConfig(profile));
        const auto idleDecision = idleEngine.evaluate(idleInputs(), 5000);
        expectState(idleDecision, ResourcePressureState::Normal, "profile/tier matrix");
        assertSafeDecision(idleDecision);

        for (const auto tier : {ResourcePressureState::Elevated, ResourcePressureState::High,
                                ResourcePressureState::Critical}) {
            ResourcePolicyEngine engine(immediateConfig(profile, tier));
            const auto decision = engine.evaluate(pressuredInputs(), 5000);
            expectState(decision, tier, "profile/tier matrix");
            assertSafeDecision(decision);
        }
    }
}

void ResourcePolicyTest::rapidStateTransitionsAreDamped() {
    FakeClock clock;
    ResourcePolicyEngine engine(fixtureConfig());

    expectState(engine.evaluate(idleInputs(), clock.nowMs()), ResourcePressureState::Normal,
                "damping baseline");

    int transitions = 0;
    int elevatedObservations = 0;
    for (int i = 0; i < 12; ++i) {
        // Alternate just above the escalation threshold and just below the clear
        // threshold every 250 ms, i.e. far faster than the configured timing.
        const auto inputs = (i % 2 == 0) ? pressuredInputs() : idleInputs();
        clock.advance(250);
        const auto decision = engine.evaluate(inputs, clock.nowMs());
        if (decision.isTransition) {
            ++transitions;
        }
        if (decision.state == ResourcePressureState::Elevated ||
            decision.state == ResourcePressureState::High) {
            ++elevatedObservations;
        }
    }

    // Bounded oscillation: the state machine may settle but never chatters.
    QVERIFY2(
        transitions <= 2,
        qPrintable(QStringLiteral("expected bounded transitions, observed %1").arg(transitions)));
    QVERIFY2(elevatedObservations <= 2,
             qPrintable(QStringLiteral("expected bounded elevated states, observed %1")
                            .arg(elevatedObservations)));
}

void ResourcePolicyTest::monitorShutdownFallsBackAfterGraceAndRecovers() {
    FakeClock clock;
    ResourcePolicyEngine engine(fixtureConfig());

    expectState(engine.evaluate(idleInputs(), clock.nowMs()), ResourcePressureState::Normal,
                "shutdown baseline");
    clock.advance(500);
    engine.evaluate(pressuredInputs(), clock.nowMs());
    clock.advance(kFixtureSustainMs);
    expectState(engine.evaluate(pressuredInputs(), clock.nowMs()), ResourcePressureState::Elevated,
                "shutdown escalation");

    // The monitor stops: no snapshot at all.
    const ResourcePolicyInputs stopped;
    clock.advance(1000);
    auto decision = engine.evaluate(stopped, clock.nowMs());
    expectState(decision, ResourcePressureState::Elevated, "monitor stopped inside grace");
    expectTelemetry(decision, ResourceTelemetryState::Interrupted, "monitor stopped");
    QVERIFY(!decision.reasons.contains(ResourcePolicyReason::MonitoringInterruptedFallback));

    // Grace elapses: the restriction must not outlive the telemetry that
    // justified it.
    clock.advance(kFixtureGraceMs);
    decision = engine.evaluate(stopped, clock.nowMs());
    expectState(decision, ResourcePressureState::Normal, "monitor shutdown fallback");
    expectReason(decision, ResourcePolicyReason::MonitoringInterruptedFallback,
                 "monitor shutdown fallback");
    QVERIFY(!decision.restrictBackgroundAdmission);

    // Monitoring returns: recovery is observable and requires fresh evidence.
    clock.advance(100);
    decision = engine.evaluate(idleInputs(), clock.nowMs());
    expectState(decision, ResourcePressureState::Normal, "monitor recovery");
    expectReason(decision, ResourcePolicyReason::MonitoringRecovered, "monitor recovery");
    expectTelemetry(decision, ResourceTelemetryState::Fresh, "monitor recovery");
    QVERIFY(!decision.degraded);
}

void ResourcePolicyTest::monitoringInterruptionWithoutGraceHoldsRestriction() {
    ResourcePolicyConfig config = fixtureConfig(ResourcePolicyProfile::Eco);
    config.timing.telemetryGraceMs = 0; // no automatic fallback configured
    FakeClock clock;
    ResourcePolicyEngine engine(config);

    expectState(engine.evaluate(idleInputs(), clock.nowMs()), ResourcePressureState::Normal,
                "no-grace baseline");
    clock.advance(500);
    engine.evaluate(pressuredInputs(), clock.nowMs());
    clock.advance(kFixtureSustainMs);
    expectState(engine.evaluate(pressuredInputs(), clock.nowMs()), ResourcePressureState::Elevated,
                "no-grace escalation");

    const ResourcePolicyInputs stopped;
    clock.advance(60LL * 60 * 1000);
    const auto decision = engine.evaluate(stopped, clock.nowMs());
    // Documented behaviour: without a configured grace the restriction persists
    // until telemetry returns. PD-D04 must adopt a bounded grace.
    expectState(decision, ResourcePressureState::Elevated, "no-grace interruption");
    QVERIFY(decision.restrictBackgroundAdmission);
}

void ResourcePolicyTest::profileSemanticsOrderWithoutProductThresholds() {
    FakeClock clock;
    ResourcePolicyEngine eco(immediateConfig(ResourcePolicyProfile::Eco));
    ResourcePolicyEngine balanced(immediateConfig(ResourcePolicyProfile::Balanced));
    ResourcePolicyEngine performance(immediateConfig(ResourcePolicyProfile::Performance));

    const auto ecoDecision = eco.evaluate(pressuredInputs(), clock.nowMs());
    const auto balancedDecision = balanced.evaluate(pressuredInputs(), clock.nowMs());
    const auto performanceDecision = performance.evaluate(pressuredInputs(), clock.nowMs());

    // Eco restricts background work first, Balanced next, Performance last. The
    // ordering is fixture-configured here; production values are PD-D04.
    QVERIFY(ecoDecision.restrictBackgroundAdmission);
    QVERIFY(!balancedDecision.restrictBackgroundAdmission);
    QVERIFY(!performanceDecision.restrictBackgroundAdmission);

    QCOMPARE(ecoDecision.toolConcurrencyLimit.value_or(-1), kFixtureEcoToolLimit);
    QCOMPARE(balancedDecision.toolConcurrencyLimit.value_or(-1), kFixtureBalancedToolLimit);
    QVERIFY(performanceDecision.toolConcurrencyLimit.value_or(-1) >
            balancedDecision.toolConcurrencyLimit.value_or(-1));

    // No profile may change security posture.
    QCOMPARE(ecoDecision.ownershipPosture, performanceDecision.ownershipPosture);
    QVERIFY(ecoDecision.foregroundAdmissionAllowed);
    QVERIFY(performanceDecision.foregroundAdmissionAllowed);
}

void ResourcePolicyTest::configurationValidationRejectsInconsistentThresholds() {
    ResourcePolicyConfig config = fixtureConfig();
    config.thresholds.cpuBusyDeescalateAt = config.thresholds.cpuBusyEscalateAt;
    config.thresholds.batteryChargeEscalateAt = 1.5; // outside 0..1
    config.profiles.eco.maxConcurrentToolStarts = 3; // looser than balanced
    config.profiles.eco.calibrated = false;          // but numeric values set

    const QStringList problems = sentinel::core::validateResourcePolicyConfig(config);
    QVERIFY(!problems.isEmpty());

    ResourcePolicyEngine engine(config);
    QCOMPARE(engine.configurationProblems().size(), problems.size());
    const auto decision = engine.evaluate(idleInputs(), 5000);
    expectReason(decision, ResourcePolicyReason::ConfigurationInvalid, "invalid configuration");
    QCOMPARE(decision.primaryReason, ResourcePolicyReason::ConfigurationInvalid);

    // A configuration that only differs by removing an escalation rule without a
    // clear rule is accepted but stays conservative: the raised tier is latched
    // until a clear rule exists.
    ResourcePolicyConfig latched = fixtureConfig();
    latched.thresholds.cpuBusyDeescalateAt.reset();
    QVERIFY(sentinel::core::validateResourcePolicyConfig(latched).isEmpty());
}

void ResourcePolicyTest::namesAreStableAndNonLocalized() {
    QCOMPARE(sentinel::core::resourcePolicyProfileName(ResourcePolicyProfile::Eco),
             QStringLiteral("Eco"));
    QCOMPARE(sentinel::core::resourcePressureStateName(ResourcePressureState::Critical),
             QStringLiteral("Critical"));
    QCOMPARE(sentinel::core::resourceTelemetryStateName(ResourceTelemetryState::Interrupted),
             QStringLiteral("Interrupted"));
    QCOMPARE(sentinel::core::resourcePolicyInputStateName(
                 sentinel::core::ResourcePolicyInputState::Unavailable),
             QStringLiteral("Unavailable"));
    QCOMPARE(sentinel::core::resourcePolicyActionName(ResourcePolicyAction::RefuseNewAdmission),
             QStringLiteral("RefuseNewAdmission"));
    QCOMPARE(sentinel::core::resourceOwnershipPostureName(
                 ResourceOwnershipPosture::PreserveAllResidency),
             QStringLiteral("PreserveAllResidency"));
    QCOMPARE(sentinel::core::resourceModelOwnershipName(ResourceModelOwnership::SharedExternal),
             QStringLiteral("SharedExternal"));

    ResourcePolicyEngine engine(fixtureConfig());
    const auto decision = engine.evaluate(idleInputs(), 5000);
    const QString summary = sentinel::core::resourcePolicyDecisionSummary(decision);
    QVERIFY(summary.contains(QStringLiteral("profile=Balanced")));
    QVERIFY(summary.contains(QStringLiteral("state=Normal")));
    QVERIFY(summary.contains(QStringLiteral("telemetry=Fresh")));
    QVERIFY(summary.contains(QStringLiteral("degraded=no")));
}

QTEST_MAIN(ResourcePolicyTest)

#include "test_resource_policy.moc"
