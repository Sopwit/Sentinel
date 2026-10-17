# CI-05 — Windows Compatibility and Main Stabilization Integration Result

**Audit Date**: 2026-10-17 (Europe/Istanbul)
**Repository**: `https://github.com/sentinel-projects/sentinel`
**Priority**: Critical — Controlled integration of CI-04 and WIN-02
**Worktree**: `<managed-worktrees>/ci-05-windows-stabilization`
**Branch**: `integration/ci-05-windows-stabilization`
**Base Commit (`origin/main`)**: `3a98bab5f040b5e46f429ba423f1d1c1a023d3e1`
**Previously Reported Baseline**: `d9fbcd463620ab4e9aa543034dcd436580c332a0` (now an ancestor of `origin/main`, one commit behind)
**Host Environment**: macOS Darwin 27.0.1 (arm64), AppleClang 21.0.0, Qt 6.12.0, Rust 1.98.1, CMake 4.4.4, Ninja 1.13.2, ccache 4.14.1
**Target Architecture**: C++20, Qt 6/QML modular monolith; Fedora KDE Plasma primary; Linux, macOS, Windows portable

> [!IMPORTANT]
> In strict accordance with the task constraints, **no** `git commit`, `git push`, `git merge`, `git rebase`, `git reset`, `git stash`, pull-request creation, or GitHub Actions workflow dispatch was executed.
> - The integration branch was created from `origin/main` and the changes reside as verified uncommitted modifications plus one untracked imported report.
> - Both source worktrees (`integration/ci-04-main-stabilization`, `fix/win-02-ipc-test-compatibility`) were inspected **strictly read-only** and remain byte-for-byte preserved.
> - No Windows named-pipe transport was implemented and no authentication policy was altered.

---

## 1. Actual Base SHA & Ancestry

### 1.1 Base Identity

The originally reported baseline `d9fbcd46` is **no longer** the tip of `origin/main`. The local remote-tracking reference `origin/main` has advanced by exactly one commit to:

```text
commit 3a98bab5f040b5e46f429ba423f1d1c1a023d3e1
Author: Sopwit <131982697+Sopwit@users.noreply.github.com>
Date:   Sat Oct 17 15:40:40 2026 +0300
    fix(ci): consolidate main stabilization, portable CLI isolation, and IPC security hardening
```

`d9fbcd46` is confirmed an ancestor of `origin/main` (`git merge-base --is-ancestor` = YES). The integration branch was therefore based on the **latest verified `origin/main` = `3a98bab5`**, not on the obsolete reported baseline.

> No `git fetch` was performed (network isolation is consistent with the no-dispatch constraint); "latest verified `origin/main`" is taken as the local remote-tracking ref `3a98bab5`.

### 1.2 Consequence for Part A (critical finding)

Commit `3a98bab5` **already contains the complete CI-04 change set**. Every CI-04 source file and audit document is byte-for-byte identical between the `integration/ci-04-main-stabilization` worktree and `origin/main`. Therefore **Part A has no outstanding changes to import** — re-importing CI-04 would be a no-op. This is documented in Section 3.

---

## 2. Read-Only Verification of Source Worktrees

### 2.1 `integration/ci-04-main-stabilization`

| Item | Value |
| --- | --- |
| Worktree | `<managed-worktrees>/ci-04-main-stabilization/Sentinel-sanitized` |
| HEAD | `d9fbcd463620ab4e9aa543034dcd436580c332a0` |
| Branch | `integration/ci-04-main-stabilization` |
| Tracked modifications (unstaged) | 6 files |
| Untracked reports | 3 files |
| Staged changes | none |
| Commits unique to this worktree | none (HEAD == `d9fbcd46`) |

Tracked modifications: `DaemonIpcServer.cpp`, `DaemonIpcServer.h`, `RealToolExecutor.cpp`, `P5_02_DAEMON_IPC_RELIABILITY_RESULT.md`, `test_daemon_ipc.cpp`, `test_legacy_cli_exit_codes.cpp`.
Untracked reports: `CI_04_INTEGRATED_MAIN_STABILIZATION_RESULT.md`, `P1_05_1_PORTABLE_CLI_TEST_ISOLATION_RESULT.md`, `P5_02_1_IPC_SECURITY_PERSISTENCE_REVIEW.md`.

### 2.2 `fix/win-02-ipc-test-compatibility`

| Item | Value |
| --- | --- |
| Worktree | `<managed-worktrees>/win-02-ipc-test-compatibility/Sentinel-sanitized` |
| HEAD | `d9fbcd463620ab4e9aa543034dcd436580c332a0` |
| Branch | `fix/win-02-ipc-test-compatibility` |
| Tracked modifications (unstaged) | `tests/core/test_daemon_ipc.cpp` |
| Untracked reports | `docs/audits/WIN_02_WINDOWS_IPC_TEST_COMPATIBILITY_RESULT.md` |
| Staged changes | none |
| Commits unique to this worktree | none (HEAD == `d9fbcd46`) |

### 2.3 Source Hash Matrix (SHA-256)

The CI-04 source worktree hashes are **identical to `origin/main`** (already integrated):

| Path | `d9fbcd46` baseline | CI-04 worktree | `origin/main` (`3a98bab5`) | Equality |
| --- | --- | --- | --- | --- |
| `apps/sentinel-daemon/service/DaemonIpcServer.cpp` | `01ae925d…a39f` | `725582ed…a4c` | `725582ed…a4c` | ✅ CI-04 == main |
| `apps/sentinel-daemon/service/DaemonIpcServer.h` | `5ecae1d1…2d1` | `f38fdfe8…ea25` | `f38fdfe8…ea25` | ✅ CI-04 == main |
| `core/src/runtime/RealToolExecutor.cpp` | `0875c195…7f2` | `082f8529…268` | `082f8529…268` | ✅ CI-04 == main |
| `tests/core/test_daemon_ipc.cpp` | `f057bba2…e6b` | `e49551dd…212a` | `e49551dd…212a` | ✅ CI-04 == main |
| `tests/integration/test_legacy_cli_exit_codes.cpp` | `cab36aa4…4b0c` | `325ea494…7bc` | `325ea494…7bc` | ✅ CI-04 == main |
| `docs/audits/P5_02_DAEMON_IPC_RELIABILITY_RESULT.md` | `0cdb48a9…6bfc` | `e82b24bf…cb8b` | `e82b24bf…cb8b` | ✅ CI-04 == main |
| `docs/audits/P1_05_1_PORTABLE_CLI_TEST_ISOLATION_RESULT.md` | absent | `716f5318…715d` | `716f5318…715d` | ✅ CI-04 == main |
| `docs/audits/P5_02_1_IPC_SECURITY_PERSISTENCE_REVIEW.md` | absent | `dafa99c4…9ebc` | `dafa99c4…9ebc` | ✅ CI-04 == main |
| `docs/audits/CI_04_INTEGRATED_MAIN_STABILIZATION_RESULT.md` | absent | `8f063059…9162` | `8f063059…9162` | ✅ CI-04 == main |
| `tests/core/test_daemon_ipc.cpp` (WIN-02) | `f057bba2…e6b` | WIN-02: `3d721df9…912c` | — | not yet in main |

**Untracked-report safety note**: the WIN-01 triage report already committed to `origin/main` (`7987b040…36c6`) differs from the untracked copy in the `audit-win-01-platform-failures` worktree (`8669b575…4062`) **only** by sanitization of the worktree path (`<managed-worktrees>/…` vs an absolute host path). Importing raw untracked copies verbatim can therefore reintroduce host paths; the WIN-02 report imported here was verified clean by the privacy scanner (Section 7.10).

---

## 3. Part A — CI-04 Import

**Result: nothing outstanding. CI-04 is already fully present in `origin/main` (`3a98bab5`).**

Preservation confirmed by digest equality (Section 2.3):

- ✅ **P1-05.1 portable Legacy CLI test isolation** — `tests/integration/test_legacy_cli_exit_codes.cpp` == main; the portable `applyTestEnvironment` child-scoped isolation is present and the 20-case suite passes locally without external environment overrides (Section 7.2).
- ✅ **P5-02.1 SQLite transaction & restart reconciliation** — `DaemonIpcServer.cpp`/`.h` == main; positional `bindValue(0/1)` fix, `db.transaction()`/`db.commit()`/`db.rollback()`, `stopServer()` unsubscription, and socket-state send guards are present.
- ✅ **Daemon peer authorization hardening** — `querySocketPeerUid` `descriptor < 0` guard, Linux `size >= sizeof(peer)` validation, and the listener-seam immutability guard are present.
- ✅ **LLVM 18.1.8 formatting corrections** — `RealToolExecutor.cpp` and `DaemonIpcServer.cpp` are at their CI-04 formatted hashes.
- ✅ **Relevant audit documentation** — `P1_05_1_…`, `P5_02_1_…`, `P5_02_DAEMON_IPC_RELIABILITY_RESULT.md`, `CI_04_…` are all committed in main.

**No newer main change was overwritten**, because no file was rewritten in Part A.

---

## 4. Part B — WIN-02 Integration

### 4.1 Method

WIN-02 was authored against `d9fbcd46` (i.e. **without** CI-04). Because `origin/main` already contains CI-04's rewrite of the same file, a controlled three-way merge was performed in the integration worktree:

```
git merge-file  current(=origin/main file)  base(=d9fbcd46 file)  other(=WIN-02 file)
```

Outcome: **exit code 0 — zero conflicts.** The merged result contains CI-04's restructuring plus WIN-02's guards. The only newly-introduced CI-04 test (`multipleInterruptedRunsReconciledAtomicallyOnRestart()`), which does not exist in WIN-02's base and is therefore absent from WIN-02's patch, received the same guard manually (this is exactly the follow-up WIN-02 §6.2 anticipated).

### 4.2 Exact Change

Single code file modified: `tests/core/test_daemon_ipc.cpp`.

- **29** platform guards imported from WIN-02:
  - 28 × `#ifndef Q_OS_UNIX` with `QSKIP("Daemon IPC peer authentication requires SO_PEERCRED (Unix-only); Windows named-pipe transport is not implemented (see PD-DWIN-01).")`.
  - 1 × `#ifndef Q_OS_UNIX` with `QSKIP("Socket filesystem node cleanup is a Unix platform contract.")` (`ownedSocketRemovedOnStop`).
- **1** guard added for the CI-04-introduced test `multipleInterruptedRunsReconciledAtomicallyOnRestart()` using the same peer-authentication message.
- **4** pinned-clang-format-18.1.8 normalization hunks carried by WIN-02 (nested braced-initializer spacing in the slow-client backpressure region). These lines were untouched by CI-04; including them makes the file **fully clean** under the pinned formatter (Section 7.8).

Totals: `#ifndef Q_OS_UNIX` blocks = **30**; `QSKIP` occurrences = **39** (30 Windows-skips + 9 pre-existing `#ifdef Q_OS_UNIX … #else QSKIP`); `git diff --stat origin/main` = `134 insertions(+), 8 deletions(-)`.

### 4.3 Candidate File Hashes

| Path | `origin/main` | CI-05 candidate | WIN-02 source |
| --- | --- | --- | --- |
| `tests/core/test_daemon_ipc.cpp` | `e49551dd8f81a5cab34caa98e2ff1c425e859ce0f8a7899b63b4b8e2522f212a` | `d7c6f9e47edf671e7b99f4a7dacfa33fb2cf2d6405cc6be253c5922fa90163d7` | `3d721df90fbd7ee38a16be9363d4f84277f5faa4c8f79f6f9b6db4ee2e75912c` |
| `docs/audits/WIN_02_WINDOWS_IPC_TEST_COMPATIBILITY_RESULT.md` (imported verbatim) | absent | `1d33a91ed996ec94a056a9efabe850e6a563d76fbc66ce12a49bea0b82c02d39` | `1d33a91e…d39` |

### 4.4 Integration Conflicts

**None.** The 3-way merge reported no conflict markers; CI-04's restructuring and WIN-02's guards occupy disjoint regions except where the guard insertion had to be re-anchored to CI-04's controller-cleanup edits in the three `interrupted*` tests (handled by the merge without divergence). No production file was changed.

---

## 5. Windows Test Coverage & Skip Boundaries

### 5.1 `test_daemon_ipc` (CTest target #115) — 42 test slots

| Class | Count | Behaviour on Windows |
| --- | --- | --- |
| Platform-independent (active) | **3** | Execute and pass: `initTestCase`, `cleanupTestCase`, `refusesRegularFileAtEndpoint` |
| Require authenticated Unix IPC (guarded) | **30** | Explicit `QSKIP` with platform-limitation message |
| Pre-existing P5-02 Unix guards | **9** | Explicit `QSKIP` (peer authorization / stale socket / socket-node cleanup / raw-descriptor backpressure / seam immutability) |

On Windows the target therefore reports **3 passed, 0 failed, 39 skipped** and **exits 0** (QtTest counts `QSKIP` as skipped, not failed). On Unix all **42 pass, 0 skipped** (verified natively, Section 7.3).

### 5.2 Guard properties (requirement compliance)

- ✅ **No blanket Windows skip** — `initTestCase`, `cleanupTestCase`, and `refusesRegularFileAtEndpoint` remain active; skips are per test case.
- ✅ **No production authentication bypass** — no production file changed; the fail-closed peer-UID rejection is untouched.
- ✅ **No fake IPC success** — no dummy/simulated responses; guarded tests `QSKIP` with an explicit message.
- ✅ **Unix authorization tests remain active on Unix** — six `peerAuthorization*` tests plus `slowClientPendingBytesBackpressure` and `testSeamCannotBeModifiedWhileListening` execute natively on Unix (all PASS, Section 7.3).
- ✅ **Windows unavailable transport remains fail-closed** — `DaemonIpcServer` still rejects every connection where peer credentials are unavailable.
- ✅ **Skipped tests explicitly explain the limitation** — every guard carries a message referencing `SO_PEERCRED`, Windows named-pipe transport, and `PD-DWIN-01`.

---

## 6. Part C — Failure Inventory Reconciliation (test cases vs CTest targets)

### 6.1 The distinction that must be preserved

`CTest` reports pass/fail **per target** (executable). `QSKIP` inside a QtTest target does **not** fail that target (exit code 0). The WIN-01/CI-03 "13" figure conflated **test-case rows** with **CTest targets**. The correct decomposition is:

| # | Class | Count | Level | Effect on Windows CTest |
| --- | --- | --- | --- | --- |
| 1 | Intentionally skipped Unix-specific test cases (`/bin/sh`, `ps`, `pwd` fixtures) | **11** | **test cases** inside **3 targets** | Targets pass (skip-as-pass); 0 hard target failures |
| 2 | Legacy CLI fixture failure | **1** | **CTest target** #118 `test_legacy_cli_exit_codes` | Hard fail → remediated by CI-04 (fix present in main) |
| 3 | Daemon IPC fixture failure | **1** | **CTest target** #115 `test_daemon_ipc` | Hard fail → remediated by WIN-02 guard (this candidate) |

### 6.2 Group A verification (source-level, exact)

The "11" is enumerated from the sources and is exactly 11 test cases across 3 targets:

- `tests/core/test_process_executor.cpp`: **4** (`#ifdef Q_OS_WIN … QSKIP("The shell fixture is Unix-only.")`) — `outputAndExit`, `timeoutAndCancellation`, `concurrentOwnership`, `cancellationAndShutdown`.
- `tests/core/test_agent_runtime.cpp`: **6** (`asyncRunCommandStreamsAndContinues`, `asyncRunCommandCancellationAndTimeout`, `asyncApprovalKeepsToolCallId`, `asyncDockerUsesProcessExecutorAndPreservesRestrictions`, `shutdownStopsActiveCommand`, `asyncProcessListStreamsAndContinues`).
- `tests/core/test_real_tool_executor_tools.cpp`: **1** (`runCommandPwdToolRunsInWorkspace`).

Total: 4 + 6 + 1 = **11** test cases, mapped to **3 CTest targets** — not 11 targets.

### 6.3 Exact counts against hosted logs

CTest numbering was confirmed identical to the hosted references in CI-03/WIN-01 on this build: **#115 = `test_daemon_ipc`**, **#118 = `test_legacy_cli_exit_codes`**, **#102 = `test_agent_reliability_harness`**, **139 targets total**.

> [!WARNING]
> The raw hosted run logs for `38982561202` (Windows), `38982561276` (macOS) and `38982561228` (Linux) are **not present in this environment** and were **not re-fetched** (no network dispatch permitted). The reconciliation above is cross-checked against the CI-03, WIN-01 and WIN-02 reports plus a source-level enumeration of the guards. Hosted re-verification of the exact row-level counts remains **required** (Section 8).

---

## 7. Part D — Regression Results (local, macOS arm64)

All results below were produced inside the integration worktree from base `3a98bab5`.

| # | Gate | Command | Result |
| --- | --- | --- | --- |
| 1 | Configure + build | `cmake --preset tests` / `cmake --build --preset tests` | ✅ Configure OK; build 1525/1525 OK; second build up-to-date |
| 2 | Legacy CLI 20-case suite (no env overrides) | `env -u CFFIXED_USER_HOME -u XDG_* test_legacy_cli_exit_codes` | ✅ **20 passed, 0 failed, 0 skipped** (11.2 s) |
| 3 | Daemon IPC suite | `test_daemon_ipc` | ✅ **42 passed, 0 failed, 0 skipped** (3.3 s) |
| 4 | Agent Reliability IPC COR-17..COR-20 | `test_agent_reliability_ipc` | ✅ **7 passed, 0 failed, 0 skipped** (6.5 s) |
| 5 | Full sequential CTest | `QT_QPA_PLATFORM=offscreen ctest --preset tests` | ✅ **139/139 passed** (209.8 s) |
| 6 | Full parallel CTest | `QT_QPA_PLATFORM=offscreen ctest --preset tests -j8` | ✅ **139/139 passed** (35.9 s) |
| 7 | Rust | `cargo fmt --all --check`; `cargo clippy --locked --all-targets -- -D warnings`; `cargo test --locked` | ✅ fmt clean; clippy 0 warnings; tests **142 passed, 0 failed, 0 ignored** |
| 8 | Pinned clang-format 18.1.8 | `git diff -U0 origin/main -- apps core integrations plugins tests \| clang-format-diff.py -p1 -binary clang-format(18.1.8)` | ✅ 0 diff lines (clean); whole file also clean under 18.1.8 |
| 9 | Generated IPC contracts | `python3 tools/ipc/generate.py --check`; `generate_desktop.py --check` | ✅ both exit 0 / clean |
| 10 | Privacy & credential scans | `check_personal_paths.py`; token regex (`ghp_`, `sk-`, `AKIA`, PEM keys) | ✅ 0 files flagged; 0 credential matches |
| 11 | Whitespace | `git diff --check origin/main` | ✅ clean |

### 7.1 CTest summary

| Mode | Discovered | Passed | Failed | Time |
| --- | --- | --- | --- | --- |
| Sequential | 139 | **139** | 0 | 209.8 s |
| Parallel `-j8` | 139 | **139** | 0 | 35.9 s |

> The prior report target of "139" is confirmed here as **139 CTest targets** (not assumed): `ctest -N` reports exactly 139, all pass in both modes.

### 7.2 Legacy CLI detail — 20/20

`usageWithoutArgsReturnsSuccess`, `unknownCommandReturnsUsageError`, `chatWithoutPromptReturnsUsageError`, `configUnknownKeyReturnsUsageError`, `modelPullBlankNameReturnsUsageError`, `statusAndConfigSucceedWithoutConfiguration`, `modelListSucceedsAgainstFixture`, `modelListFailsWhenEndpointIsRefused`, `modelPullSucceedsAgainstFixture`, `modelPullFailureReturnsNonzero`, `modelPullHttpErrorReturnsNonzero`, `modelPullStallReturnsTimeout`, `chatWithoutConfigurationReturnsNonzero`, `chatProviderFailureReturnsNonzero`, `chatSucceedsAgainstFixture`, `chatStallReturnsTimeout`, `chatRefusedWithoutConversationStoreSetsFailure`, `interruptedPullExitsAbnormally` (+ `initTestCase`/`cleanupTestCase`) — **20 passed**.

### 7.3 Daemon IPC detail — 42/42, security-negative tests intact

All 42 slots pass on Unix, including the six preserved security-negative peer-authorization cases: `peerAuthorizationNativeSameUid`, `peerAuthorizationDirectDescriptorValidation`, `peerAuthorizationCrossUidRejectionSimulated`, `peerAuthorizationMissingCredentialsRejectionSimulated`, `peerAuthorizationRootUidRejectedWhenNonRootDaemon`, `peerAuthorizationNoFallbackToUnauthenticated`; plus `slowClientPendingBytesBackpressure`, `testSeamCannotBeModifiedWhileListening`, and the CI-04 regression tests `multipleInterruptedRunsReconciledAtomicallyOnRestart`, `interruptedRunningRunReconciledOnRestart`, `interruptedApprovalRunReconciledOnRestart`, `completedRunSurvivesDaemonRestart`.

### 7.4 Focused suites

| Suite | Result |
| --- | --- |
| `test_agent_reliability_harness` | 99 passed, 0 failed, **1 intentional skip** (COR-11 assembly) |
| `test_real_tool_executor_tools` | 51 passed, 0 failed, **1 skip** (npx present) |
| `test_llm_agent_runtime` | 53 passed, 0 failed, 0 skipped |

### 7.5 Rust workspace breakdown

| Crate / target | Passed |
| --- | --- |
| `sentinel-cli` (`tests/commands.rs`) | 15 |
| `sentinel-ipc` | 27 |
| `sentinel-mascot` | 22 |
| `sentinel-tui` | 78 |
| **Total** | **142 passed, 0 failed, 0 ignored** |

`cargo fmt --all --check` → clean; `cargo clippy --locked --all-targets -- -D warnings` → 0 warnings.

### 7.6 Security-negative test cases preserved

- Six `peerAuthorization*` daemon IPC cases remain active on Unix (Section 7.3).
- `runCommandDockerFixtureContracts` retains its dedicated `#ifdef Q_OS_WIN` branch asserting `ToolExecutionStatus::Failed` and `sandbox.enforcement != SandboxEnforcement::Enforced` — the Windows sandbox fail-closed contract is unchanged.

---

## 8. Part E — Platform Verification

### 8.1 Workflow targets inspected

| Workflow | Runner | Toolchain | Scope |
| --- | --- | --- | --- |
| `ci-linux.yml` (`quality`) | `ubuntu-22.04` | g++, clang-tidy, **clang-format 18.1.8** (pip pinned), Qt 6.11.* | format gate, clang-tidy, QML lint, desktop metadata |
| `ci-linux.yml` (`build-test`) | `ubuntu-22.04` | g++, Qt 6.11.* | configure/build/ctest |
| `ci-linux.yml` (`asan-build-test`) | `ubuntu-22.04` | g++, ASan preset | ASan build/test |
| `ci-macos.yml` | `macos-14` | AppleClang, Qt 6.11.* | configure/build/ctest |
| `ci-windows.yml` (`build-test`) | `windows-2022` | **MSVC x64**, Qt 6.11.3, `QT_FORCE_STDERR_LOGGING=1` | configure/build/ctest |
| `ci-windows.yml` (`experimental-arm64`) | `windows-2022` | MSVC ARM64, Qt 6.8.*, `continue-on-error` | non-blocking |
| `ci-rust.yml` | `ubuntu-latest`, `macos-14` | Rust 1.94.0 | fmt/clippy/test; `ipc-stress` manual |
| `codeql-analysis.yml` | `ubuntu-22.04` | CodeQL C++ | static analysis |

### 8.2 Local vs hosted separation

| Category | Status | Class |
| --- | --- | --- |
| Local macOS arm64 | ✅ 139/139 CTest, 142/142 Rust, 20/20 Legacy CLI, 42/42 Daemon IPC | **Verified on this candidate** |
| Hosted Linux (Ubuntu 22.04) | ⏳ Pending | **Unverified** — CI-04 format + #115 remediations are present but not hosted-verified on this SHA |
| Hosted macOS (macos-14) | ⏳ Pending | **Unverified** — #118 + #115 remediations present |
| Hosted Windows (windows-2022 MSVC x64) | ⏳ Pending | **Unverified** — WIN-02 guards present; requires a native run |
| CodeQL | ⏳ Pending | **Not dispatched** |
| ASan | ⏳ Not executed | Requires maintainer dispatch |

> [!CAUTION]
> This candidate must **not** be described as "Windows support." Windows named-pipe IPC transport and peer authentication remain unimplemented (PD-D07 / PD-DWIN-01). On Windows the daemon IPC target now skips 39 authenticated-IPC test cases and still **fails closed** at runtime; that is honest capability limitation, not support.

### 8.3 Candidate prepared for native validation

The candidate is prepared for, but has **not** been executed on:
- **Ubuntu 22.04** (`ubuntu-22.04`) — format gate + `test_daemon_ipc` + `test_legacy_cli_exit_codes`.
- **macOS-14** — `test_legacy_cli_exit_codes` + `test_daemon_ipc`.
- **Windows-2022 MSVC x64** — confirm 3 passed / 39 skipped for #115 and 0 hard failures overall.

---

## 9. Part F — Acceptance Boundaries Kept Open (unchanged)

| Boundary | Status | Reason not closed by CI-05 |
| --- | --- | --- |
| **AC-P1-04** | OPEN | Hosted 100-repeat Unix IPC stress requires explicit maintainer dispatch (not performed). |
| **AC-P5-01** | OPEN | Windows IPC authentication/transport unimplemented. |
| **AC-P5-02** | OPEN | Windows named-pipe security (PD-D07) undecided. |
| **AC-P5-04** | OPEN | Ownership / shutdown policy (PD-D05, PD-D10) undecided. |
| **PD-D05** | OPEN | Legacy CLI disposition undecided. |
| **PD-D07** | OPEN | Windows IPC security design undecided. |
| **PD-D10** | OPEN | Provider ownership / hard-stop policy undecided. |

No approved product decision or acceptance criterion was modified.

---

## 10. Security Invariants Preserved

1. **Fail-closed daemon IPC** — `DaemonIpcServer` still rejects any peer whose kernel credentials cannot be resolved or do not match `::getuid()`. No production file was modified by CI-05.
2. **No authentication bypass** — Windows connections remain rejected; no resolver is injected, no fallback added.
3. **No faked IPC success** — guarded tests `QSKIP`; none simulate a passing Windows session.
4. **Per-test granular guards** — no suite-wide `QSKIP`; platform-independent tests (`initTestCase`, `cleanupTestCase`, `refusesRegularFileAtEndpoint`) remain active.
5. **Authorization tests active on Unix** — the six `peerAuthorization*` cases and seam-immutability test execute natively and pass.
6. **Windows sandbox fail-closed contract** — `runCommandDockerFixtureContracts` still asserts `Failed` / `!= Enforced` on Windows; `PartiallyEnforced` remains honestly reported.
7. **Tool execution & sandbox boundaries untouched** — `ToolDescriptor`/registry, `ToolExecutionGateway`, `IFileSystemService`, `ProcessExecutor`, `ProcessSandbox` unchanged.
8. **No new dependencies** — C++/Rust dependency graphs unchanged; no platform-only core logic introduced.

---

## 11. Recommendation for P4-02 Integration

The candidate is a **narrow, low-risk** change (one test file + one imported report). P4-02 (native resource telemetry) should be integrated **after** CI-05 to avoid reintroducing a stale CTest baseline:

1. **Land CI-05 first** so the daemon IPC target is portable before resource-telemetry test targets are added/rebalanced. Note that `origin/main` (`3a98bab5`) already carries `test_resource_telemetry_contract.cpp` and `test_execution_budget_policy.cpp` edits from the P4-01 line — P4-02 should be rebased onto the CI-05 result.
2. **Re-derive the CTest count** after P4-02 (currently 139 targets; P4-02 is expected to add/modify targets, so the "139" figure must be re-measured rather than assumed).
3. **Keep AC boundaries open** — P4-02 must not be presented as closing AC-P5-01/02/04 or the PD-D05/07/10 decisions.
4. **Re-run the pinned 18.1.8 gate and privacy scan** for any new resource-telemetry sources.

---

## 12. Recommendation for Maintainer-Authorized CI Review

The candidate **is ready for maintainer-authorized CI review**, subject to the boundaries below.

**Ready:**

- Base `3a98bab5` = latest verified `origin/main`; CI-04 fully present; WIN-02 integrated with zero conflicts and zero production-code change.
- Local gates: 139/139 CTest (sequential and parallel), 142/142 Rust, 20/20 Legacy CLI, 42/42 Daemon IPC, pinned 18.1.8 clean, IPC contracts clean, privacy/credential clean, `git diff --check` clean.

**Outstanding native-platform blockers (not resolvable locally):**

- Hosted Ubuntu 22.04 / macOS-14 / Windows-2022 verification of this exact SHA.
- Native Windows confirmation that #115 reports 3 passed / 39 skipped and that no CTest target hard-fails.
- AC-P1-04 hosted stress (maintainer dispatch).
- PD-D07 Windows IPC security design; PD-D05 / PD-D10 policy decisions.

**Not performed (by constraint):** commit, push, merge, rebase, PR, workflow dispatch.

---

*Report prepared independently in worktree `<managed-worktrees>/ci-05-windows-stabilization` on branch `integration/ci-05-windows-stabilization` based on `3a98bab5`. Both source worktrees were inspected read-only and preserved exactly. No commit, push, merge, rebase, reset, stash, PR, or workflow dispatch was performed.*
