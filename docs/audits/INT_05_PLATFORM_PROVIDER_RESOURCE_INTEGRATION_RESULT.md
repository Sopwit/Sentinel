# INT-05 — Controlled Integration of P2-M02, P3-04, P4-03.1 and CI-05

Audit date: 2026-10-17 (Europe/Istanbul).
Repository: `https://github.com/sentinel-projects/sentinel`.
Result type: **isolation + integration candidate + independent regression validation**.
Integration worktree: `<managed-worktrees>/int-05-platform-provider-resource`.
Integration branch: `integration/int-05-platform-provider-resource`.
Host: macOS Darwin (arm64), AppleClang 21.0.0.21000334, Qt 6.12.0, CMake 4.4.4, Ninja 1.13.2,
ccache 4.14.1, Rust 1.98.1, LLVM/clang-tidy 23.1.3, pip `clang-format==18.1.8`.

> **Constraints observed.** No `git commit`, `push`, `merge`, `reset`, `stash`, `rebase`,
> pull-request or GitHub Actions workflow dispatch was executed. `git fetch origin` (read-only)
> was used once to confirm the current main SHA. All four source worktrees were inspected
> **strictly read-only** and remain byte-for-byte preserved.

---

## 1. Isolation and verified base identity

| Item | Value |
| --- | --- |
| Reported main | `6035bf74cbb3f2556e29fe1bf31163a56eb5b591` |
| `git rev-parse origin/main` (after `git fetch origin`) | `6035bf74cbb3f2556e29fe1bf31163a56eb5b591` — **unchanged, matches the report** |
| **Integration base used** | **`origin/main` = `6035bf74`** |
| Local `main` HEAD at task start | `8e8d6b3f5a398f387f3d94ce795d636ea8a00dbf` (2 commits ahead of `origin/main`) |
| Integration worktree HEAD after creation | `6035bf74` (`git worktree add -b integration/int-05-platform-provider-resource <wt> 6035bf74`) |

### 1.1 Local `main` vs `origin/main` discrepancy (documented, not assumed)

Local `main` carried **two commits that are not on `origin/main`**:

| Commit | Subject | Relationship to INT-05 |
| --- | --- | --- |
| `fc4c39ca` | `fix(daemon): set local model runtime readiness to unavailable on process failure` | **Content-identical to the P3-04 reviewed correction** (Part D). Imported here from the P3-04 source worktree. |
| `8e8d6b3f` | `docs(architecture): record P2-M01 provider modularization architecture and migration plan` | P2-M01 documentation, **outside the four INT-05 scopes**; intentionally not imported (see §3, §10). |

Because the task names the **latest verified `origin/main`**, the candidate was based on
`6035bf74`. This is *not* a loss of reviewed content: the P3-04 correction is re-imported
byte-for-byte from its source worktree and is identical to `fc4c39ca`. The only practical delta
between an `origin/main`-based and a local-`main`-based candidate is the out-of-scope P2-M01
architecture document.

### 1.2 Source worktrees inspected (read-only)

| Scope | Branch | Worktree | Source HEAD | Entry state |
| --- | --- | --- | --- | --- |
| CI-05 | `integration/ci-05-windows-stabilization` | `<managed-worktrees>/ci-05-windows-stabilization` | `3a98bab5f040b5e46f429ba423f1d1c1a023d3e1` | 1 modified tracked, 2 untracked docs, no staged |
| P2-M02 | `refactor/p2-m02-openai-compatible-protocol` | `<opencode-tmp>/p2-m02-openai-compatible-protocol` | `6035bf74cbb3f2556e29fe1bf31163a56eb5b591` | 4 modified tracked, 4 untracked paths, no staged |
| P3-04 | `test/p3-04-lifecycle-reliability` | `<managed-worktrees>/p3-04-lifecycle-reliability/Sentinel-sanitized` | `3a98bab5f040b5e46f429ba423f1d1c1a023d3e1` | 3 modified tracked, 1 untracked doc, no staged |
| P4-03.1 | `review/p4-03-resource-policy` | `<managed-worktrees>/review-p4-03-resource-policy/Sentinel-sanitized` | `6035bf74cbb3f2556e29fe1bf31163a56eb5b591` | 2 modified tracked, 6 untracked paths, no staged |

Committed, staged, unstaged and untracked content were inspected separately. **No source used
HEAD-only assumptions**: every source change is uncommitted, and the untracked new files were the
primary deliverables.

---

## 2. Part A — Reconciliation against current main

For each source change: (1) presence in `origin/main`, (2) file hash, (3) overlaps,
(4) import decision, (5) newer-main preservation.

### 2.1 Presence / hash matrix (SHA-256, abbreviated to 16 hex)

| Scope | File | `origin/main` blob | Imported candidate | Outcome |
| --- | --- | --- | --- | --- |
| CI-05 | `tests/core/test_daemon_ipc.cpp` | `d7c6f9e47edf671e` | *(same)* | **Already in main** — no import |
| P2-M02 | `core/src/runtime/LocalInference.cpp` | `9b3eb80acb3fd624` | `8abeed031efa5c6b` | imported |
| P2-M02 | `core/src/runtime/OllamaRuntime.cpp` | `4c761147992db62b` | `7e83dd1f797c3d8b` | imported |
| P2-M02 | `core/include/.../openai_compatible/OpenAiCompatibleProtocol.h` | absent | `45388c45fd7f5726` | new, imported |
| P2-M02 | `core/src/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.cpp` | absent | `cf5aadbc845d7442` | new, imported |
| P2-M02 | `tests/core/test_openai_compatible_protocol.cpp` | absent | `99f6127fec7b1a0f` | new, imported |
| P2-M02 | `core/CMakeLists.txt` / `tests/CMakeLists.txt` | `ef7c1a56…` / `cc9450a5…` | merged (see §2.3) | registrations merged |
| P3-04 | `apps/sentinel-daemon/service/LocalModelRuntime.cpp` | `9490a57d4c242ad1` | `7e2d118c4fae4623` | imported (== `fc4c39ca`) |
| P3-04 | `tests/core/test_model_lifecycle_ownership.cpp` | `cb3e016845b43b04` | `4b15b2e8d8ed2793` | imported (== `fc4c39ca`) |
| P3-04 | `tests/desktop/test_local_model_runtime.cpp` | `bf1b716a84bde498` | `8ebe127d512f6503` | imported (== `fc4c39ca`) |
| P4-03.1 | `core/include/sentinel/core/policy/ResourcePolicy.h` | absent | `2fa97a7e859241ab` | new, imported |
| P4-03.1 | `core/src/policy/ResourcePolicy.cpp` | absent | `33d26adc081d4859` | new, imported (**corrected**, not `7a38c336`) |
| P4-03.1 | `tests/core/test_resource_policy.cpp` | absent | `46d3974b716ddc97` | new, imported (25-case suite) |
| P4-03.1 | `tests/core/test_resource_policy_review.cpp` | absent | `803d7806a8c3f931` | new, imported (adversarial suite) |
| P4-03.1 | `core/CMakeLists.txt` / `tests/CMakeLists.txt` | `ef7c1a56…` / `cc9450a5…` | merged (see §2.3) | registrations merged |

Full SHA-256 values are reproduced in §12.

### 2.2 Overlapping modifications

Only two files are touched by more than one reviewed scope:

* `core/CMakeLists.txt` — **P2-M02** (+2 lines) and **P4-03.1** (+2 lines).
* `tests/CMakeLists.txt` — **P2-M02** (+1 line) and **P4-03.1** (+9 lines incl. comments).

The two source edits occupy **disjoint anchors** (P4-03.1 near `StaticAgentRegistry.h` /
`SubagentService.cpp` / `test_vendor_cloud_contract`; P2-M02 near `ProviderErrorClassifier` /
`test_native_provider_contract`). No three-way conflict exists, and neither source file was
copied wholesale — the registrations were re-applied by hand so pre-existing P4-02 and P4-01
registrations are untouched (§4).

No other file overlaps. P3-04 and CI-05 do not modify any CMake file.

### 2.3 Merged CMake registrations (exact union)

`core/CMakeLists.txt` — 4 additions:

```
+    include/sentinel/core/policy/ResourcePolicy.h                                  (P4-03.1)
+    include/sentinel/core/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.h  (P2-M02)
+    src/policy/ResourcePolicy.cpp                                                  (P4-03.1)
+    src/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.cpp          (P2-M02)
```

`tests/CMakeLists.txt` — 9 additions:

```
+add_sentinel_core_test(test_openai_compatible_protocol core/test_openai_compatible_protocol.cpp)  (P2-M02)
+# P4-03: adaptive resource policy foundation ...
+add_sentinel_core_test(test_resource_policy core/test_resource_policy.cpp)                        (P4-03.1)
+# P4-03.1: independent adversarial fixtures ...
+add_sentinel_core_test(test_resource_policy_review core/test_resource_policy_review.cpp)          (P4-03.1)
```

### 2.4 Preserved newer-main corrections

* **P4-02 native resource telemetry** (`ResourceTelemetry.h`, `IResourceMonitorAdapter.h`,
  `ResourceMonitor.h`, `ResourceTelemetry.cpp`, `ResourceMonitor.cpp`,
  `NativeResourceMonitorAdapter.cpp`) is present and its CMake registrations are unchanged.
  The imported policy module does **not** reference any P4-02 type.
* **P4-01** execution-budget and telemetry-contract test registrations are unchanged.
* **Prior SEC-01 privacy fix** and the CI-04/CI-05 daemon-IPC hardening are unchanged.
* No whole worktree directory and no build registration was overwritten.

---

## 3. Part B — Windows CI compatibility (CI-05)

**CI-05 is already fully integrated in `origin/main`** (commit `eaa5c26d`,
`test(ipc): skip Unix-only peer credential checks on Windows and record WIN-02/CI-05 results`).
The candidate's `tests/core/test_daemon_ipc.cpp` is byte-identical to `origin/main`
(`d7c6f9e47edf671e7b99f4a7dacfa33fb2cf2d6405cc6be253c5922fa90163d7`) and to the CI-05 source
worktree; no re-import was performed.

Verified boundary properties (source-level):

| Property | Value |
| --- | --- |
| `#ifndef Q_OS_UNIX` guard blocks | **30** |
| `QSKIP` occurrences | **39** (30 Windows-skips + 9 pre-existing `#ifdef Q_OS_UNIX … #else QSKIP`) |
| Platform-independent tests kept active | `initTestCase`, `cleanupTestCase`, `refusesRegularFileAtEndpoint` (3) |
| Guard message | references `SO_PEERCRED`, Windows named-pipe transport, `PD-DWIN-01` |
| Unix security-negative tests preserved | six `peerAuthorization*` + `slowClientPendingBytesBackpressure` + `testSeamCannotBeModifiedWhileListening` (all execute natively, §6) |

* No Windows named-pipe transport or authentication was implemented or simulated.
* No production file was changed by CI-05; `DaemonIpcServer` remains fail-closed.
* **Windows IPC acceptance (`AC-P5-01` / `AC-P5-02`) remains open.**

---

## 4. Part C — Provider protocol extraction (P2-M02)

Imported exactly: `OpenAiCompatibleProtocol.{h,cpp}` (60 / 139 lines), the 424-line
characterization suite, the two runtime call-site edits, and the report. `LocalInference.cpp`
(−133/+37 combined with `OllamaRuntime.cpp`) and `OllamaRuntime.cpp` route the OpenAI-compatible
chat path, request-body construction, response-text extraction and model catalog parsing through
the new module.

Preserved invariants (verified by the imported characterization suite, §6.2):

* OpenAI-compatible **request bytes** (7 golden byte-equivalence rows, including production-path
  capture through `LMStudioLocalInferenceClient::infer`).
* **URL construction** for `/v1`-aware endpoints and the Groq `/openai/v1` base path.
* **Model catalog validation** (P2-DEF-01 malformed-catalog rejection; valid-empty accepted).
* **Provider configuration and IDs** unchanged; no `LMStudioConfig` rename.
* **Tool / structured-output** bodies pass through verbatim.
* **Streaming and cancellation** call sites unchanged.
* **Network / security enforcement** unchanged (host-mismatch `501` and endpoint policy intact).
* **Credential redaction** unchanged (§8).

Native **Claude** and **Gemini** behaviour was **not** migrated (explicitly out of scope).

---

## 5. Part D — Model lifecycle reliability (P3-04)

Imported the verified failed-startup correction and its deterministic tests:

* `apps/sentinel-daemon/service/LocalModelRuntime.cpp` (+4): on `ProcessState::Failed` from
  `dependencies_.start()`, immediately set `readiness_ = Unavailable` with
  `"llama.cpp process failed to start."`, clear `processId_`, and rely on `attemptedKey_` to
  suppress restart loops.
* `tests/core/test_model_lifecycle_ownership.cpp` (+100): `activeAgentRunRetainsModelBindingAcrossSelectionChange`,
  `modelDeletionDoesNotInvalidateActiveRunBinding`, `repeatedShutdownAndProcessOwnershipInvariants`.
* `tests/desktop/test_local_model_runtime.cpp` (+36): `synchronousFailedStartSetsUnavailable`,
  `repeatedShutdownIsIdempotent`, `duplicateEnsureRunningWhileProbingDropsExtraRequests`.

Preserved exactly:

* **D4** witness `providerSwitchRetainsOwnedChild` — preserved `XFAIL`.
* **D5b** witness `endpointSwitchDoesNotReconcileOwnedChild` — preserved `XFAIL`.
* **D6** witness `removalDuringActualUseRemainsBlocked` — preserved `QSKIP`.
* Active-run **binding stability** unchanged (immutable `ModelBinding`, refcounted provider).

No resource leases, no automatic termination of shared processes and no unapproved deletion
prevention policy were introduced. Result: `test_local_model_runtime` 52 passed / 1 skipped;
`test_model_lifecycle_ownership` 16 passed. **`AC-P3-02` ownership/delete safety remains open
(`PD-D10`).**

---

## 6. Part E — Adaptive policy foundation (P4-03.1, corrected)

Imported the **independently reviewed (corrected)** implementation, not the original uncorrected
source. The corrected `core/src/policy/ResourcePolicy.cpp` (`33d26adc…`) was imported; the
uncorrected copy present in the host `main` worktree (`7a38c336…`) was **not** used.

All five confirmed corrections verified present in the imported source:

| # | Correction | Evidence (imported `ResourcePolicy.cpp`) |
| --- | --- | --- |
| 1 | **RSS pressure comparison direction** | `ascendingBytesSignal(...)` introduced and routed to `processResidentBytes`; `hostAvailableBytes` stays on `descendingBytesSignal` |
| 2 | **Power-source presence handling** | `outcome.present = inputs.powerSource.usable();` in the fresh branch |
| 3 | **Reject invalid measured fractions** | `std::isfinite(value) \|\| value < 0.0 \|\| value > 1.0` guard in `ascendingSignal`; `value < 0` guard on byte helpers; battery-charge finiteness guard |
| 4 | **Fail-closed invalid configuration** | inert fallback condition `!configurationProblems_.isEmpty() \|\| !thresholdsCalibrated \|\| applicableWatchedCount == 0` |
| 5 | **Prevent signed cooldown overflow** | `saturatingDeadlineMs(base, delta)` used for all three `cooldownUntilMs_` assignments |

Preserved:

* the **25 original fixtures** (`test_resource_policy.cpp`, byte-identical `46d3974b…`),
* the **13 independent adversarial tests** (`test_resource_policy_review.cpp`, `803d7806…`),
* the **default inert / uncalibrated** policy (production defaults propose nothing),
* the **P4-02 telemetry API and CMake registrations**.

Not done (by scope): no live policy actions wired, no autonomous resource adjustment, no
Eco/Balanced/Performance activation. `AC-P4-02` adaptive optimization remains open
(`PD-D03`/`PD-D04`).

---

## 7. Part F — Cross-subsystem regression (local, this candidate)

All commands executed inside the integration worktree at base `6035bf74`.

| # | Gate | Command | Result |
| --- | --- | --- | --- |
| 1 | Clean configure | `cmake --preset tests` (fresh `build/tests`) | ✅ exit 0 |
| 2 | Full build | `cmake --build --preset tests -j8` | ✅ **1554/1554** targets, exit 0 |
| 3 | Full sequential CTest | `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | ✅ **145/145 passed** (227.87 s) |
| 4 | Full parallel CTest | `… ctest --preset tests -j8` | ✅ **145/145 passed** (34.64 s) |
| 5 | Provider protocol | `test_openai_compatible_protocol` | ✅ **56 / 0 / 0** |
| 6 | Policy reviewed | `test_resource_policy` | ✅ **25 / 0 / 0** |
| 7 | Policy adversarial | `test_resource_policy_review` | ✅ **13 / 0 / 0** |
| 8 | Lifecycle ownership | `test_model_lifecycle_ownership` | ✅ **16 / 0 / 0** |
| 9 | Local model runtime | `test_local_model_runtime` | ✅ **52 / 0 / 1 skip** (D4/D5b XFAIL + D6 skip preserved) |
| 10 | Daemon IPC | `test_daemon_ipc` | ✅ **42 / 0 / 0** |
| 11 | Agent reliability IPC | `test_agent_reliability_ipc` | ✅ **7 / 0 / 0** |
| 12 | Legacy CLI (normal env) | `test_legacy_cli_exit_codes` (no env overrides) | ✅ **20 / 0 / 0** (11.5 s) |
| 13 | Rust format | `cargo fmt --all --check` | ✅ clean |
| 14 | Rust Clippy | `cargo clippy --locked --all-targets -- -D warnings` | ✅ 0 warnings |
| 15 | Rust tests | `cargo test --locked` | ✅ **142 passed / 0 failed / 0 ignored** (15+27+22+78) |
| 16 | Pinned clang-format 18.1.8 | `git diff -U0 origin/main -- apps core integrations plugins tests \| clang-format-diff.py -p1` | ✅ 0 diff lines |
| 17 | Scoped clang-tidy | analysis DB + `clang-tidy` on changed/new sources | ✅ exit 0 (warnings only) |
| 18 | Generated IPC contracts | `generate.py --check`; `generate_desktop.py --check` | ✅ both exit 0 |
| 19 | Privacy | `check_personal_paths.py` | ✅ 0 files flagged |
| 20 | Credentials | token/PEM regex over changed files | ✅ 0 matches |
| 21 | Whitespace | `git diff --check origin/main` | ✅ clean |

### 7.1 Actual discovered target count

`ctest --preset tests -N` → **Total Tests: 145**.

Reconciliation: `origin/main` = 142 targets; **+1** (P2-M02 `test_openai_compatible_protocol`) and
**+2** (P4-03.1 `test_resource_policy`, `test_resource_policy_review`) = **145**. The previously
reported CI-05 figure of 139 predates the P4-02 telemetry targets and is superseded.

### 7.2 clang-format nuance (not a regression)

The pinned diff-hunk gate is clean. Whole-file formatting flags two pre-existing
`readability`/wrapping violations in `core/src/runtime/OllamaRuntime.cpp` (lines ~391/425) that
exist **identically in `origin/main`** and lie outside the imported hunks; the CI gate operates on
diff hunks and passes.

### 7.3 clang-tidy nuance (not a regression)

`clang-tidy` (LLVM 23) exit 0; `WarningsAsErrors: ''`. New-file diagnostics:
`OpenAiCompatibleProtocol.cpp` 10 × `readability-braces-around-statements` (matches P2-M02 report);
`ResourcePolicy.cpp` 5 × `bugprone-unchecked-optional-access` (the inherited D-13 finding, all
guarded by the custom `.fresh()` predicate); `test_resource_policy_review.cpp` and
`LocalModelRuntime.cpp` 0. No new warning **class** was introduced.

### 7.4 Rust totals

`sentinel-cli` 15 + `sentinel-ipc` 27 + `sentinel-mascot` 22 + `sentinel-tui` 78 = **142 passed**,
0 failed, 0 ignored. C++-only change; matches baseline.

---

## 8. Security invariants

1. **Fail-closed daemon IPC** — peer-credential rejection unchanged; no production IPC file
   modified by this integration (§3).
2. **No Windows authentication bypass** — guarded tests `QSKIP`; nothing simulates a passing
   Windows session.
3. **Per-test granular skips only** — platform-independent IPC tests remain active.
4. **Unix security-negative tests active** — six `peerAuthorization*` cases execute and pass.
5. **Policy cannot grant authority** — `ResourcePolicyDecision` has no permission/approval/
   sandbox/credential field; the action vocabulary is closed; `ResourceOwnershipPosture`
   expresses only `PreserveAllResidency`; no unload/termination verb exists.
6. **Policy side-effect-free / inert** — no IO, lock, timer, process or provider access; the
   uncalibrated default proposes no tier and no limit.
7. **Invalid policy configuration fails closed** — correction D-4 enforces the inert fallback.
8. **Tool-execution / sandbox boundaries untouched** — `ToolDescriptor`/registry,
   `ToolExecutionGateway`, `IFileSystemService`, `ProcessExecutor` unchanged.
9. **Credential redaction preserved** — no request-body, header or credential behaviour changed;
   credential scan clean.
10. **No new dependencies, no platform-only core logic** — C++/Rust dependency graphs unchanged.

---

## 9. Part G — Acceptance and hosted CI

### 9.1 Local evidence (this candidate, macOS arm64) vs hosted evidence

| Category | Status | Class |
| --- | --- | --- |
| Local macOS arm64 | ✅ 145/145 CTest (seq + par), 142/142 Rust, 20/20 Legacy CLI, 42/42 daemon IPC, 56/56 P2-M02, 25/25 + 13/13 P4-03.1, 16/16 + 52 lives | **Verified locally** |
| Hosted Linux x86_64 (`ubuntu-22.04`) | ⏳ Not dispatched | **Unverified** |
| Hosted macOS (`macos-14`) | ⏳ Not dispatched | **Unverified** |
| Hosted Windows x64/MSVC (`windows-2022`) | ⏳ Not dispatched | **Unverified** |
| Rust matrix (`ci-rust.yml`) | ⏳ Not dispatched | **Unverified** |
| CodeQL (`codeql-analysis.yml`) | ⏳ Not dispatched | **Unverified** |
| Linux quality + ASan (`ci-linux.yml`) | ⏳ Not dispatched | **Unverified** |

**No hosted workflow was dispatched.** The candidate is *prepared for* the six workflow targets
above; no hosted result is claimed.

### 9.2 Outstanding decisions and blockers (all preserved open)

| Item | Status | What remains |
| --- | --- | --- |
| **PD-D03** hardware/environment availability | OPEN | Device-class declaration for calibration. |
| **PD-D04** calibrated performance budgets | OPEN | Numeric budgets + grace window; also gates policy activation. |
| **PD-D10** provider ownership / hard-stop | OPEN | Lease/drain policy for D4/D5b/D6. |
| **PD-D05** legacy CLI disposition | OPEN | Retention/retirement decision. |
| **PD-D07** Windows trust and isolation | OPEN | Named-pipe security design. |
| **AC-P1-04** hosted IPC stress | OPEN | 100-repeat stress on supported Unix CI OS. |
| **AC-P2-01 / AC-P2-03** | Unverified / Blocked | Real-provider acceptance; live parity. |
| **AC-P3-02** ownership/delete safety | Blocked | Resource lease + refusal/drain contract. |
| **AC-P4-01 / AC-P4-02 / AC-P4-03** | Blocked / Not Started | Adaptive optimization acceptance + security invariance across tiers. |
| **AC-P5-01 / AC-P5-02** Windows IPC | Blocked / OPEN | Transport + authentication unimplemented. |

No acceptance criterion was modified, closed or weakened.

---

## 10. Known limitations

* **Base-lineage note** — candidate is based on `origin/main` `6035bf74`; local `main` additionally
  holds `fc4c39ca` (content-identical to the imported P3-04 correction) and `8e8d6b3f` (P2-M01
  architecture doc, out of scope). If a maintainer prefers local-`main` lineage, add only
  `8e8d6b3f`; the four reviewed change sets are otherwise identical.
* **P2-M01 P2-M02 forward dependency** — provider modularization is a multi-step plan; this task
  delivers only the OpenAI-compatible extraction. Native Claude/Gemini and the `ModelService`
  primary body remain inline.
* **Policy is inert** — `ResourcePolicyEngine` has no consumer; `cpuBusyFraction` has no approved
  source; `batteryPresent` / `processCpuFractionOfCore` remain unread (D-9).
* **Inherited clang-tidy D-13** — the reviewed policy emits 5 `bugprone-unchecked-optional-access`
  warnings (pre-existing, behaviourally safe).
* **Windows IPC** is a skip-and-fail-closed capability only, not support.

---

## 11. Recommended maintainer integration procedure

1. Branch from `origin/main` (`6035bf74`) or from local `main` (`8e8d6b3f`) — see §10.
2. Apply the change sets in this order, verifying the SHA-256 values in §12:
   1. **CI-05** — no-op (already in main); confirm `test_daemon_ipc.cpp` = `d7c6f9e4…`.
   2. **P2-M02** — add the 3 new files, apply the 2 runtime edits, add 2 core + 1 tests CMake line.
   3. **P3-04** — apply the 3 file deltas (+139/−1).
   4. **P4-03.1** — add the 4 policy/test files (**corrected** `ResourcePolicy.cpp`), add 2 core +
      9 tests CMake lines.
3. Merge the two CMake files by re-applying the individual registrations (do **not** overwrite
   them) so P4-01/P4-02 registrations survive.
4. Run: `cmake --preset tests` → `cmake --build --preset tests` → `ctest --preset tests` (expect
   **145** targets) → Rust fmt/clippy/test → pinned `clang-format==18.1.8` diff gate →
   scoped `clang-tidy` → IPC/privacy/credential scans → `git diff --check`.
5. Preserve the D4/D5b `XFAIL`, D6 `QSKIP` and the CI-05 Windows skips; do not "fix" them.
6. Hand off to maintainer-authorized hosted CI for Linux/macOS/Windows/Rust/CodeQL/ASan.

---

## 12. Exact source identities and hashes (SHA-256, full)

Imported / verified artifacts:

```
45388c45fd7f57268b1119bd36586a1305fca71b51dd1612f9e8e2087321bf76  core/include/sentinel/core/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.h
cf5aadbc845d7442f46af019d1204372e32e8414c879d1c98135c177a7b29513  core/src/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.cpp
99f6127fec7b1a0f568a8091e0c07ee1db2dc1984fb6d826735254658e513263  tests/core/test_openai_compatible_protocol.cpp
8abeed031efa5c6b8f44c78cbd27a9ebc06f2c61fee8dd68157373e96c8ae24d  core/src/runtime/LocalInference.cpp
7e83dd1f797c3d8ba75f61ed88e1d34d5303948982a355b2111a6756b0112c09  core/src/runtime/OllamaRuntime.cpp
7e2d118c4fae4623cf460acfb6345bcc302a9d70fb0c6e32504cc3c9a1fdd577  apps/sentinel-daemon/service/LocalModelRuntime.cpp
4b15b2e8d8ed2793679167bd782c9b738d76eb6f05999918027165cccdced76e  tests/core/test_model_lifecycle_ownership.cpp
8ebe127d512f65030209abf71dfc284f4d94d7eb36aa21c7b74b8b94fe229375  tests/desktop/test_local_model_runtime.cpp
2fa97a7e859241ab2091d331ca3bc239319df0bfa08ea40ba6befc640241009e  core/include/sentinel/core/policy/ResourcePolicy.h
33d26adc081d4859297069c4da3699015e874493dd6a385a615152c7e1d89d1c  core/src/policy/ResourcePolicy.cpp   (corrected)
46d3974b716ddc97d2a3bb9055eeee29ffb89552fecc164d81ea4a68240aebd2  tests/core/test_resource_policy.cpp
803d7806a8c3f931ac153af3ceae38aad42304b8ac04347fbeef0f92c43e153a  tests/core/test_resource_policy_review.cpp
d7c6f9e47edf671e7b99f4a7dacfa33fb2cf2d6405cc6be253c5922fa90163d7  tests/core/test_daemon_ipc.cpp   (already in main)
```

Superseded/uncorrected artifact **not** used: `7a38c336f62a59745524c2aa6b3bb2fec06e3a90271e62ae93ad05c1d3811d5c`
(`core/src/policy/ResourcePolicy.cpp`, host `main` worktree, original P4-03).

Changed-file inventory (working tree vs `origin/main`):

```
 M apps/sentinel-daemon/service/LocalModelRuntime.cpp        (+4)
 M core/CMakeLists.txt                                       (+4)
 M core/src/runtime/LocalInference.cpp                       (-133/+37 combined)
 M core/src/runtime/OllamaRuntime.cpp
 M tests/CMakeLists.txt                                      (+9)
 M tests/core/test_model_lifecycle_ownership.cpp             (+100)
 M tests/desktop/test_local_model_runtime.cpp                (+36)
?? core/include/sentinel/core/policy/ResourcePolicy.h        (484)
?? core/include/sentinel/core/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.h (60)
?? core/src/policy/ResourcePolicy.cpp                        (1001)
?? core/src/provider/protocols/openai_compatible/OpenAiCompatibleProtocol.cpp (139)
?? tests/core/test_openai_compatible_protocol.cpp            (424)
?? tests/core/test_resource_policy.cpp                       (808)
?? tests/core/test_resource_policy_review.cpp                (362)
?? docs/audits/P2_M02_OPENAI_COMPATIBLE_PROTOCOL_EXTRACTION_RESULT.md
?? docs/audits/P3_04_MODEL_LIFECYCLE_RELIABILITY_RESULT.md
?? docs/audits/P4_03_1_ADAPTIVE_POLICY_INDEPENDENT_REVIEW.md
?? docs/audits/P4_03_ADAPTIVE_RESOURCE_POLICY_DESIGN.md
```

Tracked diff vs `origin/main`: **7 files, +189 / −134**.

### 12.1 Controlled privacy sanitization

The imported `docs/audits/P3_04_MODEL_LIFECYCLE_RELIABILITY_RESULT.md` contained one absolute host
path (`/Users/<user>/…`). It was rewritten to the repo convention
`<managed-worktrees>/p3-04-lifecycle-reliability/Sentinel-sanitized` (matching the CI-05/WIN-01
precedent) so the privacy scanner reports **0 files flagged**. No technical content was altered.

---

## 13. Final assessment

The candidate is a **narrow, additive integration**: one already-present test file (CI-05), one
new pure protocol module (P2-M02), one lifecycle reliability correction (P3-04) and one corrected,
inert policy foundation (P4-03.1). No production provider security path, sandbox boundary or
authorization contract was weakened. All local gates pass (**145/145 CTest** sequential and
parallel, **142/142 Rust**, pinned format clean, scoped tidy exit 0, IPC/privacy/credential/diff
checks clean).

The candidate **is ready for maintainer-authorized hosted CI**, subject to the open
`PD-D03`/`PD-D04`/`PD-D05`/`PD-D07`/`PD-D10` decisions and the native Windows IPC, hosted stress,
real-provider and adaptive-optimization acceptance gates listed in §9.2. It does **not** claim any
product acceptance.

*Prepared in worktree `integration/int-05-platform-provider-resource` at base `6035bf74`. No commit,
push, merge, reset, stash, rebase, PR or workflow dispatch was performed. All four source
worktrees were inspected read-only and remain byte-for-byte preserved.*
