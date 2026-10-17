# P4-03.1 Adaptive Resource Policy — Independent Review

Phase: P4 — Resource Management.
Priority: High.
Dependency: P4-03 Adaptive Resource Policy Foundation (`design/p4-03-adaptive-resource-policy`,
source HEAD `3a98bab5`).
Result type: independent review of an **unwired, uncalibrated** policy foundation, with a
small set of justified corrections and an independent adversarial suite.

> **Non-claims.** This review does **not** accept P4, does not claim adaptive optimization,
> does not adopt any numeric threshold, does not enable model unload or process termination,
> and does not wire the policy into any production path. It reviews an inert foundation.

---

## 1. Verified source identity

Everything below was verified from the repository, not assumed.

| Item | Value |
| --- | --- |
| Repository | `https://github.com/sentinel-projects/sentinel` |
| Review worktree | `<worktree-root>/review-p4-03-resource-policy/Sentinel-sanitized` |
| Review branch | `review/p4-03-resource-policy` (created with `git worktree add -b … 6035bf74`) |
| Verified `origin/main` (fetched) | `6035bf74cbb3f2556e29fe1bf31163a56eb5b591` |
| Previously reported main | `6035bf74` — **still current**; `git fetch origin` left `origin/main` at `6035bf74` |
| Source branch | `design/p4-03-adaptive-resource-policy` |
| Source worktree | `<worktree-root>/p4-03-adaptive-resource-policy/Sentinel-sanitized` |
| Source HEAD | `3a98bab5f040b5e46f429ba423f1d1c1a023d3e1` — matches the reported source HEAD |
| Source working tree | **uncommitted**: 2 modified tracked files, 4 untracked paths (see below) |
| Source merge-base with main | `d9fbcd463620ab4e9aa543034dcd436580c332a0` |

### 1.1 Source branch is divergent from main (identity finding)

`3a98bab5` is **not** an ancestor of the review base. `git rev-list --left-right --count
3a98bab5...origin/main` reports **1 ahead / 7 behind**, and `git merge-base 3a98bab5
origin/main` is `d9fbcd46`. The P4-03 design document states `Base = origin/main = 3a98bab5`;
that is inaccurate at review time — `3a98bab5` is a sibling CI-stabilization commit
(`fix(ci): consolidate main stabilization, portable CLI isolation, and IPC security
hardening`) that main reached through other commits. Critically, **`3a98bab5` does not
contain P4-02** (`core/.../resource/ResourceTelemetry.h` etc. are absent), whereas the review
base `6035bf74` does. This is material for Part D (§7).

### 1.2 Inspected change set (committed, staged, unstaged, untracked)

The source implementation is entirely uncommitted, as reported:

```
 M core/CMakeLists.txt      (+2: policy header + policy source in sentinel_core)
 M tests/CMakeLists.txt     (+4: test_resource_policy registration)
?? core/include/sentinel/core/policy/ResourcePolicy.h
?? core/src/policy/ResourcePolicy.cpp
?? tests/core/test_resource_policy.cpp
?? docs/audits/P4_03_ADAPTIVE_RESOURCE_POLICY_DESIGN.md
```

No staged content (`git diff --cached` empty). The **source worktree was preserved exactly**;
nothing was committed, staged, reset, stashed, merged, pushed or dispatched against it.

### 1.3 Reviewed artifact hashes (SHA-256)

| Artifact | Lines | SHA-256 |
| --- | --- | --- |
| `core/include/sentinel/core/policy/ResourcePolicy.h` | 484 | `2fa97a7e859241ab2091d331ca3bc239319df0bfa08ea40ba6befc640241009e` |
| `core/src/policy/ResourcePolicy.cpp` | 931 | `7a38c336f62a59745524c2aa6b3bb2fec06e3a90271e62ae93ad05c1d3811d5c` |
| `tests/core/test_resource_policy.cpp` | 808 | `46d3974b716ddc97d2a3bb9055eeee29ffb89552fecc164d81ea4a68240aebd2` |
| `docs/audits/P4_03_ADAPTIVE_RESOURCE_POLICY_DESIGN.md` | 578 | `c67b4530507cb668c3c2c5e3a5e960f7b5428737b02bb3796663acf36003e884` |

These four files were copied **verbatim** onto `origin/main` (`6035bf74`) in the review
worktree and the two CMake registrations re-applied at the same anchors. The reviewed 25-case
suite `tests/core/test_resource_policy.cpp` remains **byte-identical** after the review (see
§6.2); corrections are confined to `ResourcePolicy.cpp`.

### 1.4 Isolation rules observed

* Separate worktree `review/p4-03-resource-policy` on verified `origin/main`.
* Source worktree read-only; its uncommitted state preserved exactly.
* No commit, push, merge, reset, stash, PR or hosted workflow dispatch was performed.
* `git fetch origin` (read-only) was used once to verify the current main SHA.

---

## 2. Architecture and complexity assessment (Part A)

### 2.1 What the implementation is

A pure state-machine evaluator plus a plain-data configuration/decision contract:

* `ResourcePolicy.h` (484 lines): vocabulary enums, `ResourcePolicyInput<T>` (availability +
  optional value), `ResourcePolicyInputs`, `ResourcePolicyConfig`
  (thresholds/timing/per-profile budgets), `ResourcePolicyDecision`, `ResourcePolicyEngine`.
* `ResourcePolicy.cpp` (931 lines): name functions, config validator, summary formatter, and
  the evaluator (signal classification, telemetry coverage, hysteresis, admission outcome).
* `tests/core/test_resource_policy.cpp` (808 lines): 25 deterministic fixture cases.

### 2.2 Complexity judgement

The layered split (telemetry ≠ policy ≠ authorization ≠ admission ≠ ownership) is
appropriate and matches `AGENTS.md`. The header is large but mostly declarative; the honest
complexity problem is in the evaluator, not the header:

1. **Ad-hoc per-signal classification.** CPU/memory/available/process use two shared helpers
   (`ascendingSignal`, `descendingSignal`), but thermal, power and GPU are hand-rolled inline
   blocks with their own `present`/`unverified`/`raised` bookkeeping. This is exactly where
   defects D-1, D-2 and D-3 arose (§4). A single uniform "evaluate one signal → outcome"
   contract would make the class of bug unrepresentable. **Recommend consolidation in
   P4-04, not a redesign now.**
2. **Duplicated enums.** `ResourcePolicyInputState` mirrors P4-02's
   `ResourceMetricAvailability`, and `ResourcePolicyPowerSource` mirrors
   `ResourcePowerSource`. Defensible while P4-02 was unmerged; now that P4-02 is on main
   (D-12) this is a mapping obligation, not a contract problem.
3. **Dead surface.** `ResourcePolicyInputs::batteryPresent` and
   `processCpuFractionOfCore` are never read; `ResourceProfileBudget::samplingIntervalMs`
   and the de-escalation fields `batteryChargeDeescalateAt` / `gpuUtilizationDeescalateAt`
   (and `anyDeescalationRuleConfigured()` itself) are declared, validated and counted but
   never used by the evaluator (D-6, D-9). These are small but widen the review surface.

Verdict: **acceptable complexity; not a redesign candidate.** Five minimal corrections
(≈200 diff lines, one file) remove all confirmed defects. No speculative abstraction was
introduced.

---

## 3. Security invariants (Part B)

Checked against the contract and pinned by the deterministic fixtures. Result legend:
**Holds** / **Held after correction** / **Does not hold**.

| # | Invariant | Result | Evidence |
| --- | --- | --- | --- |
| 1 | Uncalibrated policies remain inactive | **Holds** | `productionDefaultsAreInertAndNeverHealthy`, `uncalibratedNumericsAreNeverProposed`; inert branch proposes `None` and no limit |
| 2 | Unknown telemetry is never interpreted as healthy | **Holds** | `unknownInputIsNeverHealthyAndFabricatesNoZero`; `Unknown` sets `unverified`, `degraded`, never `Fresh` |
| 3 | Stale/unavailable handled consistently | **Holds** | `staleSnapshotNeverClearsAndNeverRaises`; `unavailableAndUnsupportedAreNotDegradations` (stale holds; unavailable/unsupported excluded) |
| 4 | Profiles cannot grant tool permissions | **Holds** | `ResourcePolicyDecision` has no permission/approval/sandbox/credential/delegation field; action vocabulary is closed; `permissionInvarianceAcrossProfilesAndTiers` |
| 5 | Policy decisions cannot bypass `ToolExecutionGateway` | **Holds (structural)** | The engine is not in the gateway path and the decision type cannot express authorization; verified in §7.2 that no production code constructs it |
| 6 | No model unload or process termination action can be emitted | **Holds (structural)** | `ResourcePolicyAction` has no destructive verb; `ResourceOwnershipPosture` has the single value `PreserveAllResidency` |
| 7 | Shared Local / user-owned runtimes remain protected | **Holds** | `sharedProviderOwnershipIsPreserved`; `SharedExternal → PreserveAllResidency` |
| 8 | Active Agent tasks retain ownership and authorization | **Holds** | `foregroundAdmissionAllowed` is a constant `true`; no cancellation action; `foregroundAgentRunIsNeverWithdrawn`, `cancelledTaskLeavesNoResidualPolicyState` |
| 9 | Invalid configuration fails closed | **Did not hold → Held after correction** | Defect D-4: an invalid config still produced a live tier and could apply a validator-rejected budget. Corrected to fall into the inert fallback |
| 10 | Evaluator side-effect-free | **Holds with caveat** | No IO, no lock, no thread, no timer, no process/provider access; it *does* mutate internal hysteresis state and is **not thread-safe** (D-11) |

### 3.1 Telemetry restriction vs unavailable data vs admission refusal

These are three different axes and the contract keeps them apart:

* **Telemetry restriction** is `ResourceTelemetryState` — *coverage* of the watched signals:
  `Fresh` (all watched, applicable signals fresh), `Degraded` (some value exists, some
  watched value is `Unknown`/`Stale`), `Interrupted` (no watched signal has any value).
* **Unavailable data** is a per-input `ResourcePolicyInputState` of `Unavailable` (supported
  but absent, e.g. no battery) or `Unsupported` (platform cannot provide it). Both are
  **excluded from evidence**, count as neither present nor degraded, and cannot make a host
  look unhealthy (or healthy).
* **Admission refusal** is the `refuseNewAdmission` outcome, driven solely by
  `state == Critical`. It is *not* produced by `Degraded`/`Interrupted` telemetry directly.

The important composition: missing data can **hold** an existing restriction (including an
existing `Critical` refusal) but cannot **create** a new one. A tier is only ever raised from
fresh (measured) evidence; the only exception is the documented bounded interruption
fallback, which *relaxes* toward today's fixed limits after `telemetryGraceMs`. So an
interruption can sustain a refusal for at most the grace window and can never manufacture a
new refusal.

---

## 4. Discovered defects

Severity is stated per current reach (production defaults are uncalibrated and no consumer is
wired, so most defects are **latent** today but would activate at PD-D04).

### D-1 — Process resident memory pressure is inverted (High correctness, latent)
`processResidentEscalateAtBytes` is documented "at-or-above" and the validator enforces a
recovery threshold *below* it (an ascending contract). The evaluator routed
`processResidentBytes` through `descendingSignal`, the **available-memory** comparator
(lower value = pressure). Consequence: a high-RSS process reads healthy and a low-RSS process
reads pressured. No existing test configured the signal, so it was unexposed.

### D-2 — A fresh power source is never counted as present (Medium correctness, latent)
In the power block the fresh/measured branch never set `outcome.present`. A fresh, measured
power source (`Ac` or `Battery`) therefore reported `Interrupted` telemetry when power was the
only watched signal, and a fresh battery reading could be diverted into the interruption
branch instead of escalating. Existing power tests always co-watched CPU, masking it.

### D-3 — Non-finite / out-of-range `Measured` fractions are trusted (Medium robustness, latent)
A `Measured` fraction that is NaN, infinite, negative or >1 was evaluated by raw comparison.
`NaN` compares false against both the escalation and recovery thresholds, so a NaN "clearing"
signal **silently cleared a verified restriction and reported `Fresh`**. `+inf` escalated
inconsistently. There was no finiteness or range check on metric values.

### D-4 — Invalid configuration is reported but not enforced (Medium safety, latent)
`ConfigurationInvalid` was added as a reason, but evaluation continued: the engine still
emitted a tier and could apply a budget the validator had just rejected (e.g. a calibrated
`maxConcurrentToolStarts = 0` yielded `toolConcurrencyLimit = 0`). Invariant 9 did not hold.

### D-5 — Cooldown deadline can overflow (Low robustness, latent)
`cooldownUntilMs_ = nowMonotonicMs + config_.timing.cooldownMs` is signed 64-bit addition.
At an extreme monotonic reading this is undefined behaviour; the observed effect is a
wrapped (negative) deadline, i.e. the cooldown is silently skipped and de-escalation happens
earlier than configured.

### D-6 — De-escalation config is dead (Low, no current effect)
`batteryChargeDeescalateAt` and `gpuUtilizationDeescalateAt` are validated and counted by
`anyDeescalationRuleConfigured()`, but the evaluator never reads them; and
`anyDeescalationRuleConfigured()` is itself never called. Configuring only these fields has no
effect.

### D-7 — `escalationThresholdScale` is inverted for battery charge (Low, semantics gap)
The scale is documented "below 1.0 restricts earlier". For battery charge (a *low*-is-pressure
threshold) multiplying the threshold by a scale <1 makes escalation happen *later* — the
opposite of Eco/ascending semantics. The intended direction for descending fractions is
under-specified, so this needs PD-D04 rather than a guess.

### D-8 — Escalation fallback uses the recovery threshold (Low, undocumented)
When only a de-escalation threshold is configured (no escalation threshold),
`ascendingSignal` treats `value > deescalateAt` as raised. This "clear threshold doubles as an
escalation threshold" behaviour is not documented.

### D-9 — Dead inputs/fields (Low)
`batteryPresent` and `processCpuFractionOfCore` are never read; `samplingIntervalMs` is never
used by the evaluator. They suggest capability the engine does not exercise.

### D-10 — Interruption anchor uses a 0 sentinel (Low robustness)
`interruptedSinceMs_ != 0` means "monitoring is interrupted". If monitoring stops at a
monotonic reading of exactly `0`, the anchor is not recorded, `MonitoringRecovered` is
misreported and the grace window re-anchors on the next call. Impact is bounded (state must
be non-`Unknown`, i.e. the clock origin is effectively the stop instant), but the sentinel is
fragile; `std::optional<qint64>` would be correct.

### D-11 — "Pure evaluator" overstates thread-safety (Info)
`evaluate()` mutates hysteresis state and is non-`const`; `setConfig`/`reset` mutate too. There
is no lock. Concurrent use of one instance is a data race. The design intends single-thread
ownership (daemon), so this is a documentation gap: the header should state the engine is
**not reentrant/thread-safe** and must be externally serialized.

### D-12 — Duplicated telemetry enums (Info)
`ResourcePolicyInputState` and `ResourcePolicyPowerSource` duplicate P4-02's
`ResourceMetricAvailability` and `ResourcePowerSource`. Now that P4-02 is on main, the P4-04
adapter must map between them; consider a single shared mapping.

### D-13 — Source validation claim does not reproduce (Info, inherited)
The P4-03 design document (§12.1) states clang-tidy returned "zero user-code diagnostics" and
that six `bugprone-unchecked-optional-access` findings were fixed. Reproducing the same
method at review time still yields **5** `bugprone-unchecked-optional-access` warnings in
`ResourcePolicy.cpp` (all guarded by the custom `.fresh()` predicate, so behaviourally safe,
but not seen as guarded by the check). All 5 are pre-existing in the reviewed source.

---

## 5. Exact corrections applied (review worktree only)

Corrections are confined to `core/src/policy/ResourcePolicy.cpp`; the header and the
25-case suite are untouched. Each correction is minimal and behaviour-preserving for the
production default (uncalibrated → inert) configuration.

| Defect | Exact correction |
| --- | --- |
| D-1 | Renamed `descendingSignal` → `descendingBytesSignal`; added `ascendingBytesSignal` (mirror, `>=`/`>`); routed `processResidentBytes` through it. `hostAvailableBytes` stays on the descending helper. |
| D-2 | In the power block, set `outcome.present = inputs.powerSource.usable();` before the applicable/unverified/fresh split (removed the now-redundant duplicate assignment). |
| D-3 | `ascendingSignal` now rejects non-finite or out-of-`0..1` values as `unverified`; added a `value < 0` guard to both byte-signal helpers; battery charge is finiteness-guarded. |
| D-4 | Inert fallback condition is now `!configurationProblems_.isEmpty() || !thresholdsCalibrated || applicableWatchedCount == 0`, so an invalid configuration proposes no tier and no limit. |
| D-5 | Added `saturatingDeadlineMs(base, delta)` and used it for all three `cooldownUntilMs_` assignments. |

Not corrected (reported only, pending PD-D04): D-6, D-7, D-8, D-9, D-10, D-11, D-12, D-13 —
each needs either an adopted semantic (scale direction, de-escalation use), or is
documentation/cleanup, or is an inherited validation discrepancy rather than a new defect.

Correction diff: **13 hunks, 1 file** (`ResourcePolicy.cpp`); `ResourcePolicy.h` identical;
`tests/core/test_resource_policy.cpp` byte-identical.

---

## 6. Adversarial test results (Part C)

### 6.1 New independent suite

`tests/core/test_resource_policy_review.cpp` (registered as `test_resource_policy_review`) —
11 cases, deterministic fixtures only (injected clock, injected inputs, explicit fixture
configurations), no host telemetry, no threads, no processes. It is deliberately separate so
the reviewed 25-case file stays byte-identical.

| Required adversarial scenario | Case |
| --- | --- |
| Ascending vs descending pressure semantics | `processResidentPressureIsAscendingNotInverted` |
| Fresh vs unavailable telemetry coverage | `freshPowerSourceIsPresentAndFreshTelemetryIsNotInterrupted` |
| Invalid / non-finite metric values | `invalidAndNonFiniteMetricsCannotClearPressure` |
| Invalid configuration fails closed | `invalidConfigurationFailsClosed` |
| Zero / invalid tool-concurrency budgets | `invalidNumericBudgetsFailClosed` |
| Overlapping timestamps, monotonic discontinuity | `overflowingCooldownDeadlineFailsClosed` |
| Multiple simultaneous pressure conditions | `multipleSimultaneousPressureConditionsUseHighestTier` |
| Incomplete hardware information | `incompleteHardwareInformationIsDegradedNotHealthy` |
| Repeated identical inputs | `repeatedIdenticalInputsAreIdempotent` |
| Conflicting foreground/background priorities; zero background budget | `conflictingForegroundAndBackgroundPrioritiesAreBounded` |
| Permission invariance across policy transitions | `permissionInvarianceHoldsAcrossEveryTransition` |

Boundary, hysteresis-oscillation and cooldown-expiry transitions remain covered by the
reviewed suite (`coolingDownRespectsResidenceAndCooldown`, `rapidStateTransitionsAreDamped`,
`sustainedPressureEscalatesAfterSustainWindow`, `shortLivedSpikeDoesNotEscalate`) and were not
duplicated or weakened.

### 6.2 Before / after

Against the **unmodified reviewed source**, the independent suite exposed every confirmed
defect (failures are the expected, desired outcome — they prove the defects were real):

```
Totals: 6 passed, 7 failed
  FAIL processResidentPressureIsAscendingNotInverted   (high RSS -> Normal, low RSS -> Elevated)
  FAIL freshPowerSourceIsPresentAndFreshTelemetryIsNotInterrupted (fresh AC -> Interrupted)
  FAIL invalidAndNonFiniteMetricsCannotClearPressure   (NaN cleared; telemetry Fresh)
  FAIL invalidConfigurationFailsClosed                 (invalid config still Elevated)
  FAIL invalidNumericBudgetsFailClosed                 (limit 0 applied)
  FAIL overflowingCooldownDeadlineFailsClosed          (cooldown skipped)
```

Against the **corrected** source, both suites pass:

```
test_resource_policy         : Totals: 25 passed, 0 failed   (reviewed suite, byte-identical)
test_resource_policy_review  : Totals: 13 passed, 0 failed   (11 cases + init/cleanup)
```

No existing test was weakened, skipped, blacklisted or deleted. Fixture values are fixtures,
not product configuration.

---

## 7. P4-02 compatibility findings (Part D)

### 7.1 P4-02 status

P4-02 is **merged into the review base**. `origin/main` = `6035bf74` is
`feat(resources): add native resource telemetry monitor adapters and sampling tests`;
`core/include/sentinel/core/resource/{ResourceTelemetry.h,IResourceMonitorAdapter.h,ResourceMonitor.h}`,
the native adapter source, `P4_02_NATIVE_RESOURCE_TELEMETRY_RESULT.md` and the P4-02 tests are
all present. The P4-03 design's premise ("P4-02 is unmerged; 12 uncommitted entries") is stale
(§1.1): the P4-03 source branch was authored against `3a98bab5`, which lacks P4-02.

Consequence for integration: the policy contract is self-contained (it references no P4-02
type), so the reviewed files re-base cleanly onto main. The two CMake registrations were
re-applied at anchors P4-02 does not touch (`core/CMakeLists.txt`: after
`agent/StaticAgentRegistry.h` / `agent/SubagentService.cpp`; `tests/CMakeLists.txt`: after
`test_vendor_cloud_contract`), so **no merge conflict** is expected.

### 7.2 Field mapping and the six compatibility questions

The design's §8.1 mapping table was checked against the actual merged types:

| Question | Finding |
| --- | --- |
| Duplicate telemetry models? | **No** duplicate snapshot. The policy does not model telemetry; it consumes a handful of scalar inputs. The only duplication is the two enums (D-12). |
| Invalid unit conversions? | **None needed.** Bytes (`hostMemory.availableBytes`, `process.residentBytes`) are `qint64` bytes in both; fractions (`pressureLevel`, `cpuFractionOfHost`) are `0..1` in both. The one gap is a *missing* source (host-wide CPU), not a wrong conversion. |
| Incorrect freshness handling? | The adapter must call `applyMetricFreshness`/`applySnapshotFreshness` **before** mapping and then map `Measured→Measured`, `Stale→Stale`, `Unavailable→Unavailable`, `Unsupported→Unsupported`, `Unknown→Unknown`. The policy then enforces "stale holds, never clears". Correct as specified. |
| Loss of availability provenance? | **Yes, partially.** The policy input preserves `state` + `value` but drops `ResourceReliability`, `ResourceMeasurementOrigin`, `sourceId`, `detail` and the per-metric `capturedAt`. `Approximate` vs `Authoritative` is not distinguishable inside the engine. Recommend the P4-04 adapter keep those on a diagnostics side-channel and never present an `Approximate` mapping as authoritative. |
| GPU VRAM confused with system RAM? | **No.** `GpuResourceMetrics.vramTotalBytes/vramUsedBytes` are device-local and never folded into `hostAvailableBytes`/`processResidentBytes`; the policy has no VRAM field at all. P4-04 must aggregate `gpus[]` as a per-device maximum (not a sum) and treat `Unsupported` as inapplicable. |
| Platform-specific assumptions? | **None in the policy** (no `#if defined(Q_OS_*)`, no syscalls). Platform assumptions live in the P4-04 adapter. Note macOS/Windows report GPU as `Unsupported`, so `gpuAvailable`/`gpuUtilizationFraction` will be inapplicable there. |

Additional: `cpuBusyFraction` must remain `Unknown` until a host-wide sampler (or a documented
process-scoped substitute) is approved — P4-02 exposes `process.cpuFractionOfHost`, which is
Sentinel-process CPU as a fraction of host capacity, **not** host-wide busy fraction. The
policy's `processCpuFractionOfCore` input exists but is never read (D-9), so even the
process-scoped substitute is not wired. Per the task, **no P4-04 adapter was implemented**.

---

## 8. Full validation totals (Part E)

All commands run in the review worktree
(`<worktree-root>/review-p4-03-resource-policy/Sentinel-sanitized`),
Apple clang 21.0.0 / CMake 4.4.4 / Ninja / Homebrew Qt 6.12.0, ccache.

| Check | Command | Result |
| --- | --- | --- |
| Configure | `cmake --preset tests` | Passed (exit 0) |
| Full build | `cmake --build --preset tests -j 8` | Passed (exit 0), 1319 targets |
| Focused policy suites | `ctest -R 'test_resource_policy(_review)?'` | 25/25 and 13/13 passed |
| Full CTest | `ctest --preset tests --output-on-failure` | **100% tests passed out of 144; 0 failed; 222.37 s** |
| Formatting | `clang-format --dry-run --Werror` on the header, source, reviewed test, adversarial test | Passed (0 violations) |
| Scoped static analysis | `clang-tidy -p <analysis-db> core/src/policy/ResourcePolicy.cpp tests/core/test_resource_policy_review.cpp` | exit 0; 5 user-code `bugprone-unchecked-optional-access` in `ResourcePolicy.cpp` (all pre-existing, D-13); 0 in the adversarial test |
| IPC generated contracts | `python3 tools/ipc/generate.py --check` | exit 0 (contracts unchanged) |
| Privacy scan | `python3 tools/analysis/check_personal_paths.py` | 0 files flagged |
| Rust format | `cargo fmt --check` (in `cli/`) | exit 0 (no Rust source touched) |
| Rust lint/test | `cargo clippy` / `cargo test` | Not applicable — no Rust source in this change set |
| Git whitespace | `git diff --check` (tracked) + per-file trailing-whitespace/CRLF scan of all four untracked artifacts | Clean |
| Git hygiene | `git status --porcelain`; no commit/push/merge/reset/stash/PR/dispatch | Verified |

### 8.1 Inherited baseline vs newly introduced

* **Newly introduced defects:** none. On the corrected tree the full suite is
  **100% of 144** (`0 failed`), including `test_resource_policy` (#138) and
  `test_resource_policy_review` (#139) and the P4-01/P4-02 resource suites
  (`test_resource_telemetry_contract` #141, `test_execution_budget_policy` #142,
  `test_native_resource_telemetry` #143, `test_resource_monitor_sampling` #144).
* **Inherited:** D-13 (clang-tidy claim non-reproducible) is inherited from the P4-03 report.
  The P4-02 report recorded a 141/142 full run with one pre-existing failure; on the review
  base `6035bf74` that failure is absent (the P1-05.1 portable-CLI-isolation fix is on main),
  so there are **no inherited failing tests to distinguish**. The 5 clang-tidy
  `bugprone-unchecked-optional-access` warnings are the only inherited validation gap.

Full CTest output tail:

```
100% tests passed out of 144

Total Test time (real) = 222.37 sec
```

---

## 9. Remaining dependencies (PD-D03, PD-D04, PD-D10)

None of these is satisfied by P4-03 or by this review; all remain blocking gates.

| Decision | What it gates | Effect today |
| --- | --- | --- |
| **PD-D03 — hardware/environment availability** | Declared device classes (Fedora KDE Wayland x86_64, Apple Silicon, Windows x86_64) so thresholds and tier mappings can be justified. Without it `cpuBusyFraction` has no approved source and no per-device budget can be adopted. | Engine tolerates the input as `Unknown`; no escalation relies on it. |
| **PD-D04 — calibrated performance budgets** | Adopted numeric per-profile budgets, escalation/de-escalation thresholds, sustain/residence/cooldown durations, sampling interval and a telemetry grace window. | The engine is deliberately inert (`ThresholdsNotCalibrated`); no limit is proposed. **Also where D-7 (scale direction for descending thresholds) and D-6/D-8 (use/removal of de-escalation semantics) must be decided.** |
| **PD-D10 — provider ownership / hard-stop policy** | Owned-vs-external identity and any safe drain/unload strategy. | Only `PreserveAllResidency` is expressible; no unload/eviction policy may be added. |

P4-04 requirements that are not decisions: a host-wide CPU sampler (or an approved
process-scoped substitute), an explicit foreground/background predicate, an observable
active-run count, a bounded telemetry grace, and the `AgentExecutionScheduler` release +
priority contract.

---

## 10. Recommendation for P4-04

1. **Land the P4-03 foundation plus these five corrections first** (D-1…D-5). They are small,
   local and remove confirmed correctness/robustness defects while the engine is still
   unwired — the cheapest time to fix them.
2. **Keep the engine inert and unwired** until PD-D04 calibrates budgets and a grace window.
   Do not add numeric defaults.
3. **Then integrate the P4-02 adapter** (deferred, per the design §10 step 1): one small
   `HostResourceSnapshot → ResourcePolicyInputs` unit, applying freshness first, preserving
   availability, with tests per availability state, and no policy logic in it. Keep a
   diagnostics side-channel for origin/reliability/source/detail that the policy input drops
   (§7.2).
4. **Decide the host-wide CPU question** (PD-D03): add a sampler or explicitly adopt a
   documented process-scoped substitute, wired to `cpuBusyFraction` or
   `processCpuFractionOfCore` (currently unused).
5. **Admission wiring:** extend the existing `AgentExecutionScheduler` (release notification +
   explicit foreground priority), then let one consumer apply `toolConcurrencyLimit` /
   `backgroundConcurrencyLimit`. Never a second scheduler; never preempt running work.
6. **Re-prove AC-P4-03:** replay the gateway denial/approval/sandbox/permission fixtures at
   every pressure tier and profile and keep `resourcePolicyDecisionPreservesSecurity()` pinned.
7. **Resolve the open semantics with PD-D04:** scale direction for descending thresholds
   (D-7), whether de-escalation thresholds are used or removed (D-6), and the escalation
   fallback rule (D-8).
8. **Consolidate the evaluator's per-signal classification** into one uniform signal-outcome
   contract so D-1/D-2/D-3 cannot recur; document the engine as single-thread
   (non-reentrant) (D-11); consider sharing the P4-02 enums (D-12).
9. **Ownership actions only after PD-D10**, only for the owned llama-server child via the
   existing owned-handle path; external runtimes stay read-only forever.

### Final assessment

The reviewed foundation is **safe for controlled integration** as an inert, unwired contract:
production defaults are uncalibrated and propose nothing, the decision type structurally
cannot grant authority or express a destructive outcome, and the five confirmed defects are
latent and now corrected. It is **not** safe to wire adaptive behaviour until PD-D03, PD-D04
and PD-D10 land. This review does **not** accept P4 and does **not** claim any adaptive
optimization.

---

## 11. Changed files in the review worktree

New (review artifacts):

* `docs/audits/P4_03_1_ADAPTIVE_POLICY_INDEPENDENT_REVIEW.md` — this document.
* `tests/core/test_resource_policy_review.cpp` — independent adversarial suite.

Transferred from the reviewed source (unmodified):

* `core/include/sentinel/core/policy/ResourcePolicy.h`
* `tests/core/test_resource_policy.cpp`
* `docs/audits/P4_03_ADAPTIVE_RESOURCE_POLICY_DESIGN.md`

Transferred and corrected (5 corrections, D-1…D-5):

* `core/src/policy/ResourcePolicy.cpp`

Registration:

* `core/CMakeLists.txt` (+2, same anchors as the source)
* `tests/CMakeLists.txt` (+4 for `test_resource_policy`, +3 for
  `test_resource_policy_review`)

No commit, push, merge, reset, stash, PR or workflow dispatch was performed. The source
worktree `design/p4-03-adaptive-resource-policy` was preserved exactly.
