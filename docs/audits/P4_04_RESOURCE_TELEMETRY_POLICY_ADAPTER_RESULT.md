# P4-04 Resource Telemetry → Adaptive Policy Adapter — Result

Phase: P4 — Resource Management.
Priority: High.
Branch: `feature/p4-04-resource-policy-adapter`.
Result type: a deterministic, **read-only** translation unit between the P4-02 native
resource telemetry contract (`HostResourceSnapshot`) and the corrected P4-03.1 policy-input
contract (`ResourcePolicyInputs`), plus a diagnostics side-channel.

> **Non-claims.** This change does **not** activate automatic optimization, does **not**
> calibrate or adopt any numeric threshold, does **not** wire the policy into any production
> path, and does **not** enable model unload, process termination, permission mutation,
> provider-ownership change, admission enforcement or profile switching. The policy engine
> stays inert. No production threshold is invented here.

---

## 1. Base identity

| Item | Value |
| --- | --- |
| Repository | `https://github.com/sentinel-projects/sentinel` |
| Verified `origin/main` | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| Branch point (base SHA) | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| `main` vs `origin/main` | `0 ahead / 0 behind` (reconciled before branching) |
| Working tree at branch creation | clean |
| P4-02 dependency | integrated (`ResourceTelemetry.h`, `IResourceMonitorAdapter.h`, `ResourceMonitor.h`, `NativeResourceMonitorAdapter.cpp`, P4-02 suites) |
| P4-03.1 dependency | integrated (`ee8b1547 fix(policy): harden adaptive resource policy signals, arithmetic, and review tests`) |
| Both policy suites present | `tests/core/test_resource_policy.cpp`, `tests/core/test_resource_policy_review.cpp` |

### 1.1 P4-03.1 five corrections verified present in `core/src/policy/ResourcePolicy.cpp`

| # | Correction | Evidence |
| --- | --- | --- |
| 1 | Correct RSS comparison direction | `ascendingBytesSignal(...)` is used for `processResidentBytes`; `descendingBytesSignal(...)` for `hostAvailableBytes` |
| 2 | Correct power-source presence | `outcome.present = inputs.powerSource.usable();` in the power block |
| 3 | Invalid / non-finite fraction rejection | `!std::isfinite(value) \|\| value < 0.0 \|\| value > 1.0` → `unverified`; `value < 0` guards on byte signals |
| 4 | Invalid-configuration fail-closed | inert branch `if (!configurationProblems_.isEmpty() \|\| !thresholdsCalibrated \|\| applicableWatchedCount == 0)` |
| 5 | Saturating cooldown arithmetic | `saturatingDeadlineMs(base, delta)` used for all three `cooldownUntilMs_` assignments |

No adaption uses the original defective policy implementation.

### 1.2 Workspace integrity note (material)

This repository is being worked by multiple agents concurrently. During the P4-04 run the
checked-out branch was switched several times by other agents (reflog), and their
uncommitted work (P2-M03 provider protocols, P6-03 accessibility, P7-02a Fedora packaging)
is present in the same working tree. Consequences, stated honestly:

* P4-04's own artifacts (§2) are self-contained and do not overlap those change sets.
* `tests/CMakeLists.txt` transiently held a P6-03 block; the P4-04 registration is the
  trailing three-line hunk and is independent of it.
* Build/CTest numbers in §7 were produced on this **shared** tree and therefore include the
  other agents' in-progress files. P4-04 scoped results (§7.1) are unaffected.
* This branch was restored (`git switch feature/p4-04-resource-policy-adapter`) without
  discarding any other agent's work — no commit, push, merge, reset, stash, PR or dispatch
  was performed by P4-04.

---

## 2. Exact changed files

| File | Change | Lines | SHA-256 |
| --- | --- | --- | --- |
| `core/include/sentinel/core/policy/ResourcePolicyTelemetryAdapter.h` | new | 156 | `eacd0d344df42bd34dab3d0e624a8f2fc8a6965c8094ffef73065e3b5e233ce9` |
| `core/src/policy/ResourcePolicyTelemetryAdapter.cpp` | new | 426 | `604fcc8b9c89c8fe49f3582dc464565fd43c97360fbf9e41db5de332f4b101cb` |
| `tests/core/test_resource_policy_adapter.cpp` | new | 631 | `9c551590c17b9ad3d53517fc748b0834a02248373d9fef4e084e0e5ee67790fc` |
| `core/CMakeLists.txt` | +2 (header + source in `sentinel_core`) | — | — |
| `tests/CMakeLists.txt` | +3 (`test_resource_policy_adapter` registration) | — | — |
| `docs/audits/P4_04_RESOURCE_TELEMETRY_POLICY_ADAPTER_RESULT.md` | new (this document) | — | — |

No other file was intentionally modified by P4-04. All changes are left **uncommitted** for
maintainer review.

---

## 3. Objective and design

The adapter is a **pure translation**: one `HostResourceSnapshot` plus an explicit monotonic
clock in; one immutable `ResourcePolicyTelemetryMapping` out, containing:

* `inputs` — a `ResourcePolicyInputs` bundle a caller may hand to `ResourcePolicyEngine::evaluate`;
* `diagnostics` — a read-only side-channel preserving the provenance the policy input drops.

The translator holds **no** engine, clock, sampler or thread. It never calls `evaluate`. The
engine stays inert until PD-D03/PD-D04 calibrate it; consuming the mapping is an explicit
caller decision.

Public surface (`ResourcePolicyTelemetryAdapter.h`):

* `ResourcePolicyTelemetryAdapterConfig` — `metricMaxAgeMs` (defaults to the P4-02 monitor
  bound `kResourceMonitorDefaultMetricMaxAgeMs`) and `applyFreshness`.
* `ResourcePolicyInputProvenance` — mapped state, source availability, value presence,
  freshness, applicability, age, origin, reliability, scope, source id, detail, capture time.
* `ResourcePolicyTelemetryDiagnostics` — adapter id, batch capture time, clock used, max age,
  host logical CPU count, GPU device count, one provenance record per policy input, and
  stable translation notes.
* `ResourcePolicyTelemetryAdapter::adapt(snapshot, now)` and
  `adapt(snapshot, capabilities, now)`.

---

## 4. Part A — Input mapping

Units and scopes are preserved exactly; host and process metrics are never conflated.

| P4-02 source | Policy input | Unit / semantics preserved |
| --- | --- | --- |
| *(none — no host-wide CPU source in P4-02)* | `cpuBusyFraction` | always `Unknown`; `process.cpuFractionOfHost` is Sentinel-process share, never host CPU |
| `process.residentBytes` | `processResidentBytes` | bytes; scope `SentinelProcess` |
| `process.cpuFractionOfCore` | `processCpuFractionOfCore` | fraction of one core (1.0 == one full core) |
| `hostMemory.availableBytes` | `hostAvailableBytes` | reclaimable bytes; scope `Host` |
| `hostMemory.pressureLevel` | `memoryPressureLevel` | native indicator 0..1 |
| `thermal.level` | `thermalLevel` | native level string (never a fabricated score) |
| `power.powerSource` | `powerSource` | `ResourcePowerSource` → `ResourcePolicyPowerSource` (Unknown/Ac/Battery) |
| `power.batteryPresent` | `batteryPresent` | boolean presence |
| `power.batteryChargeFraction` | `batteryChargeFraction` | 0..1 |
| `gpus[].utilizationFraction` | `gpuUtilizationFraction` | **per-device maximum**, never a sum |
| GPU device presence + capabilities | `gpuAvailable` | boolean availability classification |
| GPU `vramTotalBytes` / `vramUsedBytes` | *(not mapped)* | device-local VRAM is never folded into host RAM or process RSS |

Deliberate non-mappings documented in diagnostics: host total memory, process virtual
memory, process thread count, GPU VRAM. Activity counts and model ownership are **not**
invented — they are left at their neutral defaults because they are runtime facts owned
elsewhere.

Concrete refusals to conflate:

* Process RSS ≠ available system memory (distinct sources; distinct inputs).
* GPU VRAM ≠ physical RAM (VRAM never reaches `hostAvailableBytes`/`processResidentBytes`).
* Process CPU ≠ host-wide CPU (`cpuBusyFraction` stays `Unknown`).
* Unsupported telemetry ≠ zero consumption (no value, `applicable() == false`).

---

## 5. Part B — Read-only guarantees

| Requirement | Status | Mechanism |
| --- | --- | --- |
| No new background threads | satisfied | no thread/timer; `adapt` is a synchronous pure function |
| No additional resource samplers | satisfied | no adapter/`ResourceMonitor` ownership |
| No model unload | satisfied | no model/runtime reference at all |
| No process termination | satisfied | no process reference |
| No admission enforcement | satisfied | no scheduler; `refuseNewAdmission` never produced here |
| No permission mutations | satisfied | no permission/approval/sandbox type used |
| No provider ownership changes | satisfied | ownership left at default; not read from providers |
| No automatic profile switching | satisfied | no `ResourcePolicyProfile` selection; no engine held |
| Engine stays inert without calibration | satisfied | adapter never calls `evaluate`; test proves default engine is inert |

`adapt` copies the caller's snapshot before applying freshness, so it never mutates the
source snapshot (pinned by test 16).

---

## 6. Part C — Telemetry quality, freshness and provenance

### 6.1 Availability mapping

`Measured → Measured`, `Stale → Stale`, `Unavailable → Unavailable`,
`Unsupported → Unsupported`, `Unknown → Unknown`. `Measured`/`Stale` **without** a value
degrades to `Unknown` (a producer contract violation is never read as a value).

### 6.2 Freshness

* When `applyFreshness` is true (default), `applySnapshotFreshness(snapshot, now, maxAgeMs)`
  is applied to the private copy **before** mapping.
* A `Measured` metric older than `maxAgeMs` becomes `Stale` with its value preserved (the
  policy’s "stale holds, never clears" rule then applies downstream).
* A `Measured` metric with no captured monotonic timestamp becomes `Unknown` with the value
  dropped (cannot be proven fresh).
* Age is recorded per input; it is `-1` when it cannot be derived.
* When `applyFreshness` is false, the snapshot's own labels are trusted verbatim.

### 6.3 Provenance side-channel

The policy input keeps only `state` + `value`. Everything else is preserved in
`ResourcePolicyTelemetryDiagnostics`: `ResourceMeasurementOrigin`, `ResourceReliability`,
`ResourceScope`, `sourceId`, `detail`, per-metric `ResourceSampleTimestamp` (monotonic +
wall clock), value presence, fresh/applicable flags and age. No synthetic "healthy" value is
ever produced for a missing measurement.

### 6.4 GPU classification

* Device present → `gpuAvailable = Measured(true)`; utilization is the per-device maximum
  over usable values (fresh if any contributor is fresh, otherwise stale).
* No device entry → classification uses the static capability declaration
  (`GpuUtilization`/`GpuMemory`, combined conservatively: Measured > Unavailable >
  Unsupported > Unknown) when supplied; otherwise `Unsupported`. A supported-but-absent GPU
  maps to `Measured(false)`; `Unsupported`/`Unavailable` map to inapplicable inputs.

---

## 7. Part D — Deterministic tests

`tests/core/test_resource_policy_adapter.cpp` (18 slots, registered as
`test_resource_policy_adapter`). Every fixture is explicit; no host telemetry, thread,
sampler, model or process is touched. No existing threshold semantics were changed.

| Required scenario | Case |
| --- | --- |
| 1 Fresh measurements | `freshMeasurementsMapToFreshInputs` |
| 2 Stale measurements | `staleMeasurementsMapToStaleAndAreNotCleared` (+ `trustedLabelsAreUsedWhenFreshnessDisabled`) |
| 3 Unsupported signals | `unsupportedSignalsAreNotZeroConsumption` |
| 4 Missing data | `missingDataNeverFabricatesHealthyValues` |
| 5 Invalid values | `invalidValuesArePreservedForThePolicyToReject` |
| 6 Battery telemetry | `batteryTelemetryKeepsUnitsAndPresence` |
| 7 Thermal telemetry | `thermalTelemetryMapsNativeLevelString` |
| 8 Memory-pressure telemetry | `memoryPressureKeepsOriginAndReliability` |
| 9 Process RSS | `processRssIsBytesAndNeverHostOrGpuMemory` |
| 10 Mixed availability | `mixedAvailabilityIsPreservedPerSignal` |
| 11 Timestamp boundaries | `timestampBoundariesFollowFreshnessRule` |
| 12 Provenance preservation | `provenanceIsPreservedInDiagnostics` |
| 13 Repeated identical inputs | `repeatedIdenticalInputsAreDeterministic` |
| 14 No accidental policy activation | `mappingDoesNotActivateAnUncalibratedPolicy` |
| (extra) GPU per-device maximum | `gpuUtilizationIsPerDeviceMaximumNeverASum` |
| (extra) GPU capability classification | `gpuAvailabilityUsesCapabilityDeclaration` |
| (extra) Unavailable is not a degradation | `unavailableSignalsAreNotADegradation` |

Notable pins: NaN pressure is preserved and drives the policy to `Degraded` (never `Fresh`,
never escalating); `age == maxAge` stays fresh and `age == maxAge + 1` becomes stale; the
default (uncalibrated) engine yields `state=Unknown`, `primaryReason=ThresholdsNotCalibrated`,
`actions=[None]`, no limits, `PreserveAllResidency`.

---

## 8. Full validation evidence

Environment: Apple clang 21.0.0 / CMake 4.x / Ninja / Qt 6.12.0 (Homebrew), macOS arm64;
existing `build/tests` reused; build parallelism limited to `-j 4`.

| Check | Command | Result |
| --- | --- | --- |
| Configure | `cmake --preset tests` | Passed (exit 0) |
| Full build | `cmake --build --preset tests -j 4` | Passed (exit 0) |
| Focused resource + policy suites | `ctest -R '…resource…\|execution_budget\|resource_policy(_adapter\|_review)?'` | **7/7 passed** |
| Full CTest | `ctest --preset tests --output-on-failure` | **100% passed out of 150; 0 failed; 177.49 s** |
| Formatting | `clang-format --dry-run --Werror` on the new header/source/test | Passed (0 violations) |
| Scoped static analysis | `clang-tidy -p <PCH-free analysis DB>` on the new source and test | exit 0; **0 user-code warnings** on both |
| IPC generated contracts | `python3 tools/ipc/generate.py --check` | exit 0 (contracts unchanged) |
| Privacy scan | `python3 tools/analysis/check_personal_paths.py` | 1 pre-existing unrelated file flagged (`docs/audits/P7_01_…`); **0 P4-04 files** |
| Git whitespace | `git diff --check` + per-file CRLF/trailing scan of the new files | Clean |
| Git hygiene | `git status`; no commit/push/merge/reset/stash/PR/dispatch | Verified |

### 8.1 Scoped totals

* `test_resource_policy_adapter` — 18 cases + init/cleanup, all passed.
* `test_resource_policy` (#148) — 25 cases, passed (reviewed suite unchanged).
* `test_resource_policy_review` (#149) — 13, passed.
* P4-01/P4-02 resource suites (`test_resource_telemetry_contract` #144,
  `test_execution_budget_policy` #145, `test_native_resource_telemetry` #146,
  `test_resource_monitor_sampling` #147) — all passed.

Per §1.2, the full-suite totals are on a shared tree that also contains other agents'
uncommitted work; the scoped P4-04 results above are unaffected by that.

---

## 9. Runtime policy enforcement confirmation

Adaptive/resource policy enforcement remains **disabled**:

* The adapter never constructs, calls or stores a `ResourcePolicyEngine`.
* With production defaults the engine reports `ThresholdsNotCalibrated` and proposes
  `actions=[None]` (pinned by `mappingDoesNotActivateAnUncalibratedPolicy`).
* The decision vocabulary still cannot express unload, termination, permission, approval,
  sandbox or ownership change; `ownershipPosture` is `PreserveAllResidency`.
* No numeric threshold, budget, sampling interval, sustain/residence/cooldown duration or
  grace window is added or changed by P4-04.

No adaptive optimization is claimed or accepted.

---

## 10. Remaining blockers

| Decision | What it gates | Effect today |
| --- | --- | --- |
| **PD-D03** — hardware/environment availability | Declared device classes so `cpuBusyFraction` can have an approved host-wide source and thresholds can be justified. P4-04 deliberately keeps `cpuBusyFraction = Unknown`. | Engine tolerates the input as `Unknown`; no escalation relies on it. |
| **PD-D04** — calibrated performance budgets | Adopted numeric thresholds, per-profile budgets, sustain/residence/cooldown, sampling interval and telemetry grace; and the open semantics D-6/D-7/D-8 (de-escalation use, scale direction for descending thresholds, escalation fallback). | Engine is inert; no limit proposed. |
| **PD-D10** — provider ownership / hard-stop policy | Owned-vs-external identity and any safe drain/unload strategy. | Only `PreserveAllResidency` is expressible; no unload/eviction policy may be added. |

Also outstanding (not decisions): a host-wide CPU sampler or an approved documented
process-scoped substitute; an explicit foreground/background predicate and observable
active-run count; the `AgentExecutionScheduler` release + priority contract; and a settings
surface to load a calibrated `ResourcePolicyConfig`.

---

## 11. Proposed P4-05 scope

1. **Decide the host-wide CPU source (PD-D03)** and either add a host-scoped sampler or
   adopt a documented process-scoped substitute, wiring exactly one of `cpuBusyFraction` /
   `processCpuFractionOfCore`. Keep the mapping unit free of platform-specific logic.
2. **Calibrate (PD-D04)** on declared hardware; land a validated `ResourcePolicyConfig`
   loader. Do not add numeric defaults to the adapter or engine.
3. **Admission wiring** via the existing `AgentExecutionScheduler` (release notification +
   explicit foreground priority), with a single consumer applying
   `toolConcurrencyLimit`/`backgroundConcurrencyLimit`; never a second scheduler; never
   preempt running work.
4. **Consolidate the evaluator's per-signal classification** into one uniform signal-outcome
   contract so D-1/D-2/D-3 cannot recur; document the engine as single-thread/non-reentrant
   (D-11); consider sharing the P4-02 enums (D-12).
5. **Re-prove AC-P4-03** by replaying the gateway denial/approval/sandbox/permission fixtures
   at every pressure tier and profile, keeping `resourcePolicyDecisionPreservesSecurity()`
   pinned, and extending the P4-04 adapter tests with a live-capability integration fixture.
6. **Ownership actions only after PD-D10**, only for the owned `llama-server` child via the
   existing owned-handle path; external runtimes remain read-only.

---

## 12. Changed files in this worktree

New (P4-04 artifacts):

* `core/include/sentinel/core/policy/ResourcePolicyTelemetryAdapter.h`
* `core/src/policy/ResourcePolicyTelemetryAdapter.cpp`
* `tests/core/test_resource_policy_adapter.cpp`
* `docs/audits/P4_04_RESOURCE_TELEMETRY_POLICY_ADAPTER_RESULT.md`

Modified (registration only):

* `core/CMakeLists.txt` (+2)
* `tests/CMakeLists.txt` (+3 for `test_resource_policy_adapter`)

No commit, push, merge, reset, stash, PR or hosted workflow dispatch was performed. No other
agent's changes were discarded or overwritten. All P4-04 changes remain uncommitted for
maintainer review.
