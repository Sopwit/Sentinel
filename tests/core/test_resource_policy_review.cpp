// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// P4-03.1 independent adversarial fixtures for the adaptive resource policy.
//
// These cases are deliberately separate from the implementation's own
// `test_resource_policy.cpp` so the reviewed 25-case suite stays byte-identical
// and provably unweakened. Every numeric value here is a FIXTURE VALUE used to
// make a state-machine or safety property observable; none is a product
// threshold (PD-D03/PD-D04 remain pending).
//
// The suite targets the review checklist: ascending vs descending pressure
// semantics, fresh vs interrupted telemetry coverage, invalid/non-finite
// metrics, invalid configuration fail-closed behaviour, extreme/overflowing
// timestamps, idempotence and permission invariance across transitions.
//
// No test reads host telemetry, starts a thread, loads a model or touches a
// process.

#include "sentinel/core/policy/ResourcePolicy.h"

#include <QtTest>

#include <limits>

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

constexpr qint64 kGiB = 1024LL * 1024 * 1024;

// CPU-only, hysteresis-free fixture. One immutable signal keeps every case
// readable; the point is the semantic, not the number.
ResourcePolicyConfig cpuOnlyConfig() {
    ResourcePolicyConfig config;
    config.thresholds.cpuBusyEscalateAt = 0.80;
    config.thresholds.cpuBusyDeescalateAt = 0.50;
    return config; // timing left at zero: a single evaluation lands directly
}

void expectState(const ResourcePolicyDecision& decision, ResourcePressureState expected,
                 const char* what) {
    QVERIFY2(decision.state == expected,
             qPrintable(QStringLiteral("%1: expected %2 got %3 (%4)")
                            .arg(QString::fromUtf8(what),
                                 sentinel::core::resourcePressureStateName(expected),
                                 sentinel::core::resourcePressureStateName(decision.state),
                                 sentinel::core::resourcePolicyDecisionSummary(decision))));
}

} // namespace

class ResourcePolicyReviewTest final : public QObject {
    Q_OBJECT
private slots:
    void processResidentPressureIsAscendingNotInverted();
    void freshPowerSourceIsPresentAndFreshTelemetryIsNotInterrupted();
    void invalidAndNonFiniteMetricsCannotClearPressure();
    void invalidConfigurationFailsClosed();
    void invalidNumericBudgetsFailClosed();
    void overflowingCooldownDeadlineFailsClosed();
    void multipleSimultaneousPressureConditionsUseHighestTier();
    void incompleteHardwareInformationIsDegradedNotHealthy();
    void repeatedIdenticalInputsAreIdempotent();
    void conflictingForegroundAndBackgroundPrioritiesAreBounded();
    void permissionInvarianceHoldsAcrossEveryTransition();
};

// DEFECT EXPOSURE: process resident memory is an ASCENDING signal (more RSS is
// more pressure) and the configuration contract documents
// `processResidentEscalateAtBytes` as "at-or-above". The evaluator routed it
// through the descending (available-memory) comparator, inverting the verdict:
// a high-RSS process read as healthy and a low-RSS process read as pressured.
void ResourcePolicyReviewTest::processResidentPressureIsAscendingNotInverted() {
    ResourcePolicyConfig config;
    config.thresholds.processResidentEscalateAtBytes = kGiB;       // at-or-above
    config.thresholds.processResidentDeescalateAtBytes = kGiB / 2; // at-or-below
    QVERIFY(sentinel::core::validateResourcePolicyConfig(config).isEmpty());

    ResourcePolicyEngine engine(config);

    ResourcePolicyInputs high;
    high.processResidentBytes = sentinel::core::measuredPolicyInput<qint64>(2 * kGiB);
    expectState(engine.evaluate(high, 1000), ResourcePressureState::Elevated, "high RSS escalates");

    ResourcePolicyInputs low;
    low.processResidentBytes = sentinel::core::measuredPolicyInput<qint64>(64LL * 1024 * 1024);
    expectState(engine.evaluate(low, 2000), ResourcePressureState::Normal, "low RSS clears");
}

// DEFECT EXPOSURE: a fresh, measured power source never set the "present" flag,
// so a power-only configuration reported the telemetry as Interrupted (or held
// a stale tier) even though a truthful, current reading existed.
void ResourcePolicyReviewTest::freshPowerSourceIsPresentAndFreshTelemetryIsNotInterrupted() {
    ResourcePolicyConfig config;
    config.thresholds.escalateOnBattery = true;
    QVERIFY(sentinel::core::validateResourcePolicyConfig(config).isEmpty());
    ResourcePolicyEngine engine(config);

    ResourcePolicyInputs ac;
    ac.powerSource = sentinel::core::measuredPolicyInput(ResourcePolicyPowerSource::Ac);
    const auto acDecision = engine.evaluate(ac, 1000);
    expectState(acDecision, ResourcePressureState::Normal, "fresh AC power");
    QCOMPARE(acDecision.telemetry, ResourceTelemetryState::Fresh);
    QVERIFY(!acDecision.degraded);

    ResourcePolicyInputs battery;
    battery.powerSource = sentinel::core::measuredPolicyInput(ResourcePolicyPowerSource::Battery);
    const auto batteryDecision = engine.evaluate(battery, 2000);
    expectState(batteryDecision, ResourcePressureState::Elevated, "fresh battery power");
    QCOMPARE(batteryDecision.telemetry, ResourceTelemetryState::Fresh);
}

// A `Measured` label on an impossible value (NaN, infinity, outside 0..1) must
// never be read as a truthful clear or raise. Such a value is unusable evidence:
// it degrades telemetry and may only hold the verified tier.
void ResourcePolicyReviewTest::invalidAndNonFiniteMetricsCannotClearPressure() {
    ResourcePolicyEngine engine(cpuOnlyConfig());

    ResourcePolicyInputs pressure;
    pressure.cpuBusyFraction = sentinel::core::measuredPolicyInput<double>(0.90);
    expectState(engine.evaluate(pressure, 1000), ResourcePressureState::Elevated, "escalated");

    ResourcePolicyInputs nan;
    nan.cpuBusyFraction =
        sentinel::core::measuredPolicyInput<double>(std::numeric_limits<double>::quiet_NaN());
    auto decision = engine.evaluate(nan, 2000);
    expectState(decision, ResourcePressureState::Elevated, "NaN must not clear");
    QVERIFY(decision.telemetry != ResourceTelemetryState::Fresh);
    QVERIFY(decision.degraded);

    ResourcePolicyInputs negative;
    negative.cpuBusyFraction = sentinel::core::measuredPolicyInput<double>(-0.5);
    decision = engine.evaluate(negative, 3000);
    expectState(decision, ResourcePressureState::Elevated, "negative fraction must not clear");
    QVERIFY(decision.degraded);

    ResourcePolicyInputs infinity;
    infinity.cpuBusyFraction =
        sentinel::core::measuredPolicyInput<double>(std::numeric_limits<double>::infinity());
    decision = engine.evaluate(infinity, 4000);
    expectState(decision, ResourcePressureState::Elevated, "infinite fraction must not be trusted");
}

// An invalid configuration must fail closed: report the problem, produce no
// verdict and propose no numeric limit. Today an invalid config still yields a
// live tier and budget.
void ResourcePolicyReviewTest::invalidConfigurationFailsClosed() {
    ResourcePolicyConfig config = cpuOnlyConfig();
    // A legitimate escalation rule coexists with an impossible recovery ordering.
    config.thresholds.availableMemoryEscalateAtBytes = 4 * kGiB;
    config.thresholds.availableMemoryDeescalateAtBytes = 4 * kGiB; // must be strictly above
    QVERIFY(!sentinel::core::validateResourcePolicyConfig(config).isEmpty());

    ResourcePolicyEngine engine(config);
    QVERIFY(!engine.configurationProblems().isEmpty());

    ResourcePolicyInputs pressure;
    pressure.cpuBusyFraction = sentinel::core::measuredPolicyInput<double>(0.99);
    const auto decision = engine.evaluate(pressure, 1000);
    QVERIFY(decision.reasons.contains(ResourcePolicyReason::ConfigurationInvalid));
    QCOMPARE(decision.primaryReason, ResourcePolicyReason::ConfigurationInvalid);
    expectState(decision, ResourcePressureState::Unknown, "invalid configuration is inert");
    QCOMPARE(decision.toolConcurrencyLimit.has_value(), false);
    QVERIFY(!decision.refuseNewAdmission);
}

// A calibrated budget with an impossible numeric value (here a zero tool
// concurrency, which the validator rejects) must not be applied as a live limit.
void ResourcePolicyReviewTest::invalidNumericBudgetsFailClosed() {
    ResourcePolicyConfig config = cpuOnlyConfig();
    config.profiles.balanced.calibrated = true;
    config.profiles.balanced.maxConcurrentToolStarts = 0; // must be at least 1
    QVERIFY(!sentinel::core::validateResourcePolicyConfig(config).isEmpty());

    ResourcePolicyEngine engine(config);
    ResourcePolicyInputs idle;
    idle.cpuBusyFraction = sentinel::core::measuredPolicyInput<double>(0.10);
    const auto decision = engine.evaluate(idle, 1000);
    QVERIFY(decision.reasons.contains(ResourcePolicyReason::ConfigurationInvalid));
    QCOMPARE(decision.toolConcurrencyLimit.has_value(), false);
    expectState(decision, ResourcePressureState::Unknown, "invalid budget is inert");
}

// Extreme monotonic timestamps must not overflow a cooldown deadline in a way
// that silently stops honouring the cooldown.
void ResourcePolicyReviewTest::overflowingCooldownDeadlineFailsClosed() {
    ResourcePolicyConfig config = cpuOnlyConfig();
    config.timing.minimumResidenceMs = 0;
    config.timing.cooldownMs = 1000;
    ResourcePolicyEngine engine(config);

    const qint64 huge = std::numeric_limits<qint64>::max() - 500;
    ResourcePolicyInputs pressure;
    pressure.cpuBusyFraction = sentinel::core::measuredPolicyInput<double>(0.99);
    expectState(engine.evaluate(pressure, huge), ResourcePressureState::Elevated,
                "escalate at max");

    ResourcePolicyInputs idle;
    idle.cpuBusyFraction = sentinel::core::measuredPolicyInput<double>(0.10);
    const auto decision = engine.evaluate(idle, huge);
    expectState(decision, ResourcePressureState::Elevated, "cooldown holds at max timestamp");
    QVERIFY(decision.reasons.contains(ResourcePolicyReason::DeescalationBlockedByCooldown));

    // A monotonic discontinuity (time moving backwards) must neither clear nor crash.
    const auto backwards = engine.evaluate(idle, -5);
    expectState(backwards, ResourcePressureState::Elevated, "backwards clock holds");
    QVERIFY(!backwards.isTransition);
}

// Positive control: when several signals are simultaneously crossed the highest
// contributed tier wins and coverage stays fresh.
void ResourcePolicyReviewTest::multipleSimultaneousPressureConditionsUseHighestTier() {
    ResourcePolicyConfig config;
    config.thresholds.cpuBusyEscalateAt = 0.80;
    config.thresholds.cpuBusyDeescalateAt = 0.50;
    config.thresholds.cpuEscalatedTier = ResourcePressureState::Elevated;
    config.thresholds.memoryPressureEscalateAt = 0.70;
    config.thresholds.memoryPressureDeescalateAt = 0.30;
    config.thresholds.memoryPressureEscalatedTier = ResourcePressureState::High;
    QVERIFY(sentinel::core::validateResourcePolicyConfig(config).isEmpty());

    ResourcePolicyEngine engine(config);
    ResourcePolicyInputs inputs;
    inputs.cpuBusyFraction = sentinel::core::measuredPolicyInput<double>(0.95);
    inputs.memoryPressureLevel = sentinel::core::measuredPolicyInput<double>(0.90);
    const auto decision = engine.evaluate(inputs, 1000);
    expectState(decision, ResourcePressureState::High, "highest raised tier wins");
    QCOMPARE(decision.telemetry, ResourceTelemetryState::Fresh);
}

// A watched-but-unknown signal degrades coverage but must not be read as a
// healthy zero, and must not by itself clear or escalate.
void ResourcePolicyReviewTest::incompleteHardwareInformationIsDegradedNotHealthy() {
    ResourcePolicyConfig config;
    config.thresholds.cpuBusyEscalateAt = 0.80;
    config.thresholds.cpuBusyDeescalateAt = 0.50;
    config.thresholds.thermalWarningLevels = {QStringLiteral("critical")};
    config.thresholds.escalateOnBattery = true;
    ResourcePolicyEngine engine(config);

    ResourcePolicyInputs inputs;
    inputs.cpuBusyFraction = sentinel::core::measuredPolicyInput<double>(0.10);
    inputs.thermalLevel = sentinel::core::unknownPolicyInput<QString>();
    inputs.powerSource = sentinel::core::unknownPolicyInput<ResourcePolicyPowerSource>();
    const auto decision = engine.evaluate(inputs, 1000);
    QCOMPARE(decision.telemetry, ResourceTelemetryState::Degraded);
    QVERIFY(decision.degraded);
    expectState(decision, ResourcePressureState::Normal, "no pressure from missing data");
}

// Repeating an identical input at the same logical time must be idempotent and
// must not report a second transition.
void ResourcePolicyReviewTest::repeatedIdenticalInputsAreIdempotent() {
    ResourcePolicyConfig config = cpuOnlyConfig();
    config.timing.cooldownMs = 5000;
    ResourcePolicyEngine engine(config);

    ResourcePolicyInputs pressure;
    pressure.cpuBusyFraction = sentinel::core::measuredPolicyInput<double>(0.90);
    const auto first = engine.evaluate(pressure, 10000);
    QVERIFY(first.isTransition);
    const auto second = engine.evaluate(pressure, 10000);
    QVERIFY(!second.isTransition);
    QCOMPARE(first.state, second.state);
    QVERIFY(!second.reasons.contains(ResourcePolicyReason::EscalationRequested));
}

// Positive control: at Critical with both foreground and background work running,
// running work is never withdrawn; only new admissions are refused and new
// background starts are held, with the configured fixture budgets.
void ResourcePolicyReviewTest::conflictingForegroundAndBackgroundPrioritiesAreBounded() {
    ResourcePolicyConfig config;
    config.thresholds.cpuBusyEscalateAt = 0.80;
    config.thresholds.cpuBusyDeescalateAt = 0.50;
    config.thresholds.cpuEscalatedTier = ResourcePressureState::Critical;
    config.profiles.eco.calibrated = true;
    config.profiles.eco.maxConcurrentToolStarts = 1;
    config.profiles.eco.maxConcurrentBackgroundStarts = 0;
    config.profiles.eco.backgroundAdmissionCeiling = ResourcePressureState::Normal;
    config.profiles.balanced.calibrated = true;
    config.profiles.balanced.maxConcurrentToolStarts = 2;
    config.profiles.balanced.maxConcurrentBackgroundStarts = 0; // zero is a valid "none"
    config.profiles.balanced.backgroundAdmissionCeiling = ResourcePressureState::Normal;
    config.profiles.performance.calibrated = true;
    config.profiles.performance.maxConcurrentToolStarts = 4;
    config.profiles.performance.maxConcurrentBackgroundStarts = 2;
    config.profiles.performance.backgroundAdmissionCeiling = ResourcePressureState::High;
    QVERIFY(sentinel::core::validateResourcePolicyConfig(config).isEmpty());

    ResourcePolicyEngine engine(config);
    ResourcePolicyInputs inputs;
    inputs.cpuBusyFraction = sentinel::core::measuredPolicyInput<double>(0.99);
    inputs.activeForegroundRuns = 2;
    inputs.activeBackgroundRuns = 5;
    const auto decision = engine.evaluate(inputs, 1000);
    expectState(decision, ResourcePressureState::Critical, "critical");
    QVERIFY(decision.refuseNewAdmission);
    QVERIFY(decision.restrictBackgroundAdmission);
    QVERIFY(decision.foregroundAdmissionAllowed);
    QCOMPARE(decision.toolConcurrencyLimit.value_or(-1), 2);
    QCOMPARE(decision.backgroundConcurrencyLimit.value_or(-1), 0);
}

// Security posture must be invariant across every pressure transition and every
// ownership value: no profile, tier or transition may grant authority or
// express a destructive outcome.
void ResourcePolicyReviewTest::permissionInvarianceHoldsAcrossEveryTransition() {
    ResourcePolicyConfig config = cpuOnlyConfig();
    config.timing.minimumResidenceMs = 0;
    config.timing.cooldownMs = 0;
    ResourcePolicyEngine engine(config);

    const auto assertSafe = [](const ResourcePolicyDecision& decision) {
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
            QVERIFY(!name.contains(QStringLiteral("Grant")));
            QVERIFY(!name.contains(QStringLiteral("Terminat")));
            QVERIFY(!name.contains(QStringLiteral("Unload")));
        }
    };

    qint64 now = 0;
    for (int i = 0; i < 8; ++i) {
        ResourcePolicyInputs inputs;
        inputs.cpuBusyFraction =
            sentinel::core::measuredPolicyInput<double>((i % 2 == 0) ? 0.95 : 0.10);
        inputs.activeForegroundRuns = 1;
        inputs.activeBackgroundRuns = 2;
        inputs.modelOwnership = (i % 3 == 0) ? ResourceModelOwnership::SharedExternal
                                             : ResourceModelOwnership::SentinelManaged;
        inputs.modelServingActiveRun = (i % 2 == 0);
        assertSafe(engine.evaluate(inputs, now));
        now += 100;
    }
}

QTEST_MAIN(ResourcePolicyReviewTest)

#include "test_resource_policy_review.moc"
