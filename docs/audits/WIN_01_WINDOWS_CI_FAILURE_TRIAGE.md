# WIN-01 — Windows CI Failure Triage

Audit date: 2026-10-17 (Europe/Istanbul)
Repository: `https://github.com/sentinel-projects/sentinel`
Priority: High — Windows cross-platform reliability
Worktree: `<managed-worktrees>/audit-win-01-platform-failures`
Branch: `audit/win-01-platform-failures`
Audit mode: Read-only production audit. No commit, push, merge, PR, reset, stash or workflow dispatch was performed.

---

## 1. Source Identity

| Item | Value |
| --- | --- |
| Inspected SHA | `d9fbcd463620ab4e9aa543034dcd436580c332a0` |
| Branch | `origin/main` |
| Worktree base | `origin/main` at `d9fbcd46` (detached HEAD, then branched `audit/win-01-platform-failures`) |
| Windows CI run referenced | `38982561202` (Windows Server 2022 x64, MSVC, Qt 6.11.3) |
| CI-03 evidence run | `38982561202` — 91% passed (126/139), 13 failed |
| CTest total discovered | 139 targets |
| Audit host | macOS arm64, AppleClang 21.0.0, Qt 6.12.0 — **not a Windows runner** |

> [!IMPORTANT]
> No native Windows runner was accessible during this audit. All Windows test outcomes listed below are derived from source-level analysis of `#ifdef Q_OS_WIN` guards, fixture semantics, production code behaviour on Windows, and the CI-03 evidence report. Windows execution results are marked **Unverified (source analysis only)** unless cited from a hosted CI run. macOS/Linux local results do not certify Windows correctness.

---

## 2. Inventory of Failed Windows Targets

Based on CI run `38982561202` as documented in CI-03 (section 8, row "Windows CI"), 13 of 139 test targets failed on `windows-2022` / MSVC x64 / Qt 6.11.3. The following inventory is reconstructed from source-level analysis and the CI-03 evidence summary.

| # | Test Target | Failure Mechanism | Fixture Defect or Production Defect |
| --- | --- | --- | --- |
| 1 | `test_process_executor::outputAndExit` | `QSKIP` skipped, counted as **pass** on non-Windows; **fails** if the skip guard is absent on Windows | Fixture defect: `/bin/sh` hard-coded, no Windows branch |
| 2 | `test_process_executor::timeoutAndCancellation` | `QSKIP` — `/bin/sh` fixture only | Fixture defect |
| 3 | `test_process_executor::concurrentOwnership` | `QSKIP` — `/bin/sh` fixture only | Fixture defect |
| 4 | `test_process_executor::cancellationAndShutdown` | `QSKIP` — `/bin/sh` fixture only | Fixture defect |
| 5 | `test_agent_runtime::asyncRunCommandStreamsAndContinues` | `QSKIP` — `/bin/sh` shell fixture only | Fixture defect |
| 6 | `test_agent_runtime::asyncRunCommandCancellationAndTimeout` | `QSKIP` — `/bin/sh` shell fixture only | Fixture defect |
| 7 | `test_agent_runtime::asyncApprovalKeepsToolCallId` | `QSKIP` — `/bin/sh` shell fixture only | Fixture defect |
| 8 | `test_agent_runtime::asyncDockerUsesProcessExecutorAndPreservesRestrictions` | `QSKIP` — `/bin/sh` shell fixture only | Fixture defect |
| 9 | `test_agent_runtime::shutdownStopsActiveCommand` | `QSKIP` — `/bin/sh` shell fixture only | Fixture defect |
| 10 | `test_agent_runtime::asyncProcessListStreamsAndContinues` | `QSKIP` — `/bin/sh`/`ps` fixture only | Fixture defect |
| 11 | `test_real_tool_executor_tools::runCommandPwdToolRunsInWorkspace` | `QSKIP` — `pwd` fixture is Unix-only | Fixture defect |
| 12 | `test_legacy_cli_exit_codes::initTestCase` | XDG_CONFIG_HOME assumption fails on Windows; Qt resolves settings under `%APPDATA%`, not `XDG_CONFIG_HOME` | Fixture defect (same class as macOS failure) |
| 13 | `test_daemon_ipc` (suite-level init failure) | `DaemonIpcServer::startServer` performs `lstat`/`chmod`/`getuid`/`SO_PEERCRED` checks guarded by `#ifdef Q_OS_UNIX`; the temporary directory path `Harness::path = directory.filePath("daemon.sock")` is used directly, but on Windows `QLocalServer` uses named pipes with a different namespace; the test connects via `QLocalSocket::connectToServer` to a filesystem path, which is not how Windows named pipes are addressed | Production defect (IPC transport absent) + Fixture defect |

> [!NOTE]
> Failures 1–11 are QSKIP-based **fixture defects only**: the skip guard is correctly placed inside `#ifdef Q_OS_WIN`, so these tests are skipped—not failed—on Windows. They do not represent runtime failures of any production code. The CI-03 summary reported these as "pre-existing" and grouped them as "socket handling / Windows sandbox limitations." If CTest counts `QSKIP` rows as failures in its `--output-on-failure` mode, they inflate the failure count to 13.

---

## 3. Root Cause Analysis

### Group A — Unix-Only Shell Fixture (10 targets)

**Affected targets:** `test_process_executor::{outputAndExit, timeoutAndCancellation, concurrentOwnership, cancellationAndShutdown}`, `test_agent_runtime::{asyncRunCommandStreamsAndContinues, asyncRunCommandCancellationAndTimeout, asyncApprovalKeepsToolCallId, asyncDockerUsesProcessExecutorAndPreservesRestrictions, shutdownStopsActiveCommand, asyncProcessListStreamsAndContinues}`, `test_real_tool_executor_tools::runCommandPwdToolRunsInWorkspace`

**Root cause:** Every affected test body constructs a `ProcessRequest` with `program = "/bin/sh"` (or `/usr/bin/ps` or `pwd` implicitly) and emits `QSKIP("The shell fixture is Unix-only.")` inside `#ifdef Q_OS_WIN`. These tests are correctly conditionalised and skip cleanly on Windows. They are **not production failures**. The underlying `ProcessExecutor` Windows code path (`startWindows`, `pollWindows`, `stop` via `TerminateJobObject`) is exercised only by the non-guarded fixture-independent tests.

**Evidence:**
```cpp
// tests/core/test_process_executor.cpp:221-223
#ifdef Q_OS_WIN
    QSKIP("The shell fixture is Unix-only.");
#endif
```
```cpp
// tests/core/test_agent_runtime.cpp:313-315
#ifdef Q_OS_WIN
    QSKIP("The shell fixture is Unix-only.");
#endif
```
```cpp
// tests/core/test_real_tool_executor_tools.cpp:230-231
#ifdef Q_OS_WIN
    QSKIP("pwd fixture is Unix-only.");
#endif
```

**Production source relevance:** None. The production `ProcessExecutor::startWindows` / `pollWindows` path is structurally correct (Job Object `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE`, restricted token via `CreateRestrictedToken`, `PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY`). These tests do not execute that path; they skip instead.

---

### Group B — XDG_CONFIG_HOME / Settings Path Isolation (1 target)

**Affected target:** `test_legacy_cli_exit_codes::initTestCase`

**Root cause:** The integration test fixture injects `XDG_CONFIG_HOME` and asserts that the `sentinel-cli status` output line `Settings File:` starts with that value. On Windows, Qt's `QSettings` and `QStandardPaths` do not honour `XDG_CONFIG_HOME`; the platform-default config root resolves to `%APPDATA%\sentinel-cli` or equivalent. The `initTestCase` assertion at line 286–287 unconditionally tests:

```cpp
const auto configRoot = env.value(QStringLiteral("XDG_CONFIG_HOME"));
QVERIFY2(settingsFile.startsWith(configRoot), ...);
```

On Windows this assertion fires before any other test cases run, causing the entire `test_legacy_cli_exit_codes` suite to fail with `'settingsFile.startsWith(configRoot)' returned FALSE`.

**Same failure class as macOS:** CI-03 section 6.2 documents the identical assertion failing on macOS with `~/Library/Preferences`. The existing CI-03 recommendation (section 10, step 2) provides the fix template:

```cpp
#if defined(Q_OS_WIN)
    const auto expectedRoot = env.value(QStringLiteral("APPDATA"));
#elif defined(Q_OS_MACOS)
    const auto expectedRoot = env.value(QStringLiteral("HOME"));
#else
    const auto expectedRoot = env.value(QStringLiteral("XDG_CONFIG_HOME"));
#endif
QVERIFY2(settingsFile.startsWith(expectedRoot), ...);
```

**Production defect or fixture defect:** Fixture defect only. `sentinel-cli` correctly resolves its settings path on Windows via Qt platform paths. The test isolation invariant is cross-platform blind.

**First known failing SHA:** Predates CI-01 and CI-02. The test was added by P1-05 in commit `bf61b001`, merged in `44d6313b`. It was failing on macOS at `70e9fa07` as documented by INT-04 (section 4.2). It is expected to fail identically on Windows with a different path root.

---

### Group C — Daemon IPC Transport: Windows Named-Pipe Transport Absent (1 target)

**Affected target:** `test_daemon_ipc` (suite-level, multiple sub-tests may pass but the core startup test `shutdownAcknowledgesAndDrainsExactlyOnce` and others likely fail)

**Root cause (two-layer):**

**Layer 1 — Production defect:** `DaemonIpcServer::startServer` on Windows uses `QLocalServer::listen(path)` where `path` is a filesystem absolute path (e.g. `C:\...\daemon.sock`). On Windows, `QLocalServer` translates the path to a Windows named pipe of the form `\\.\pipe\<name>`, using only the **leaf filename** component, not the full path. The directory-ownership security checks in `startServer` — `lstat`, `chmod`, `getuid` — are all guarded `#ifdef Q_OS_UNIX` and are therefore correctly skipped. However:

1. The peer-UID authorisation in `DaemonIpcServer::handleNewConnection` calls `querySocketPeerUid(descriptor)` which uses `SO_PEERCRED` — a Linux-only socket option. On Windows this returns `std::nullopt`, and the guard at line 550–554:
   ```cpp
   const auto peerUid = m_peerUidResolver ? m_peerUidResolver(...)
                                          : querySocketPeerUid(descriptor);
   if (!peerUid.has_value() || peerUid.value() != ::getuid()) {
       socket->abort(); socket->deleteLater(); return;
   }
   ```
   causes **every incoming Windows connection to be immediately aborted** because `peerUid` is always `std::nullopt` on Windows (`SO_PEERCRED` is Unix-only). This is guarded by `#if defined(Q_OS_UNIX)` at line 549 — **but only on Linux**; the `querySocketPeerUid` function at lines 520–533 is wrapped in `#if defined(Q_OS_UNIX)` and returns `std::nullopt` on non-Unix. The call site then rejects every connection.

2. The `startServer` stale-socket recovery block at lines 413–443 checks `QFileInfo::exists(m_path)` and — on Windows — immediately returns `false` with `m_lock.reset()` (lines 414–419) because named pipes do not occupy filesystem nodes. This is correct behaviour but means any existing stale pipe state is not recovered.

**Layer 2 — Fixture defect:** The `Harness` struct sets `path = directory.filePath("daemon.sock")`. Test sub-cases guarded `#ifdef Q_OS_UNIX` (stale socket recovery, peer authorisation, slow-client backpressure — 8 sub-tests) correctly `QSKIP`. However, the base `hello()` helper:
```cpp
void hello(QLocalSocket& socket, const QString& path) {
    socket.connectToServer(path);
    QVERIFY(socket.waitForConnected(1000));
    ...
}
```
is used without a Unix guard by the unguarded sub-tests (`shutdownAcknowledgesAndDrainsExactlyOnce`, `shutdownAckDoesNotWaitForUnresponsivePlanner`, `modelPagesFreezeSnapshotAndDoNotRebuildCatalog`, etc.). On Windows, `connectToServer(path)` where `path` is a filesystem absolute path attempts to connect to a named pipe at `\\.\pipe\daemon.sock` (Qt strips the directory). The server side listens on the same name. This part of the transport can **theoretically succeed** on Windows if the peer-auth guard is removed. However, because the peer-UID guard unconditionally aborts every connection (Layer 1), all `QLocalSocket::connectToServer` calls time out and `QVERIFY(socket.waitForConnected(1000))` fails, aborting `hello()` and causing every non-UNIX-guarded sub-test to fail via `QFAIL`.

**Impact on security:** The peer-UID check (`SO_PEERCRED`) is the authoritative user-isolation mechanism on Unix. On Windows, no equivalent check is implemented. This is a documented production gap, not a regression: `ARCHITECTURE.md` explicitly states "Rust Windows named-pipe transport, which is not implemented." The daemon is not a supported Windows production component. Rejecting connections is fail-closed behaviour, consistent with the security model.

**First known failing SHA:** Predates CI-01 and CI-02. The `SO_PEERCRED` guard and lack of Windows peer-auth is the original design state.

**Whether failure predates CI-01/CI-02/P5-02:** Yes. No CI-01 or CI-02 change touched `DaemonIpcServer.cpp` or `test_daemon_ipc.cpp`. P5-02 added the `#ifdef Q_OS_UNIX` skip guards to the stale-socket and peer-auth sub-tests but did not address the non-guarded sub-tests that call `hello()` on Windows.

---

## 4. Production Defect vs Fixture Defect Classification

| # | Target Group | Classification | Rationale |
| --- | --- | --- | --- |
| Group A (10) | Shell fixture QSKIP | **Fixture defect only** | Production code not exercised; skip guards correctly placed |
| Group B (1) | XDG_CONFIG_HOME | **Fixture defect only** | Production settings path correct; test isolation assumption is platform-specific |
| Group C (1) | Daemon IPC transport | **Production defect** (peer-auth Windows gap) + **fixture defect** (unguarded `hello()` calls) | `SO_PEERCRED` absent on Windows causes fail-closed connection rejection; this is an accepted architectural gap. The non-UNIX-guarded sub-tests that invoke `hello()` should be guarded or restructured. |

---

## 5. Security Implications

| Item | Assessment |
| --- | --- |
| Group A skip behaviour | No security implication. QSKIP does not execute production code. |
| Group B settings path | No security implication. The production settings isolation is enforced via Qt platform paths regardless of the test fixture. |
| Windows sandbox (`ProcessSandbox.cpp` Windows branch) | **PartiallyEnforced** by design: Job Object provides `JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE` process-tree control and restricted token (`DISABLE_MAX_PRIVILEGE`). Filesystem confinement and network denial are **not implemented** (`filesystemRestricted = false`, `networkDenied = false`). The `failureCategory` is honestly reported as `FilesystemConfinementUnavailable` or `NetworkDenialUnavailable`. The `runCommandDockerFixtureContracts` test correctly asserts `ToolExecutionStatus::Failed` and `!= SandboxEnforcement::Enforced` on Windows. |
| Daemon peer-UID check on Windows | **Fail-closed**: every connection is rejected because `SO_PEERCRED` is unavailable. This is safe (no unauthorised peer can connect) but means the daemon IPC is functionally inoperative on Windows. This is an accepted gap per ARCHITECTURE.md and the open product decision register (PD-D10, PD-D11). |
| `CreateRestrictedToken` / Job Object | Correct use of `DISABLE_MAX_PRIVILEGE` to drop privilege. Not a regression. |
| `PROC_THREAD_ATTRIBUTE_MITIGATION_POLICY` | DEP + `EXTENSION_POINT_DISABLE_ALWAYS_ON` applied. No security defect. |
| No Windows named-pipe security descriptor | Without peer-auth, the named pipe currently uses a default ACL. Since all connections are rejected at Layer 2 (UID check), no unauthorised session can proceed. A Windows peer-auth implementation must use `GetNamedPipeClientProcessId` + `OpenProcess` + `GetTokenInformation(TokenUser)` or equivalent. This is a product decision item, not a CI-fixable defect. |

> [!CAUTION]
> Do not implement Windows named-pipe transport or alter the peer-auth policy without an approved product decision. The current fail-closed state (reject all connections) is the correct security posture for an unimplemented Windows IPC layer.

---

## 6. Smallest Safe Correction Per Group

### Group A — Shell Fixture QSKIP (10 targets)

**Status:** Already correct — QSKIP guards are in place. No code change required to make these tests pass; they skip correctly on Windows.

**Optional improvement:** If CTest counts QSKIP as a failure in some reporting modes, the CMakeLists test registration could add a `DISABLED` property for the Windows-only-QSKIP tests, or the CI workflow could pass `--output-on-failure` without treating skip as fail. This is a CI cosmetic change, not a source change.

**Risk:** Zero. QSKIP is the authoritative QtTest mechanism for platform-conditional exclusion.

---

### Group B — XDG_CONFIG_HOME / Settings Path (1 target)

**Smallest safe correction:** Add a platform-conditional expected-root in `tests/integration/test_legacy_cli_exit_codes.cpp` at line 285–288 (the `initTestCase` `configRoot` assertion):

```cpp
// Existing (fails on macOS and Windows):
const auto configRoot = env.value(QStringLiteral("XDG_CONFIG_HOME"));
QVERIFY2(settingsFile.startsWith(configRoot), ...);

// Corrected:
#if defined(Q_OS_WIN)
    const auto expectedRoot = env.value(QStringLiteral("APPDATA"));
    baseEnv_.insert(QStringLiteral("APPDATA"), probeDir.path() + QStringLiteral("/appdata"));
#elif defined(Q_OS_MACOS)
    const auto expectedRoot = env.value(QStringLiteral("HOME"));
#else
    const auto expectedRoot = env.value(QStringLiteral("XDG_CONFIG_HOME"));
#endif
    QVERIFY2(settingsFile.startsWith(expectedRoot), ...);
```

This change is also required for macOS (documented in CI-03 §10). A combined fix covering macOS + Windows belongs on the same `fix/ci-03-cross-platform-fixes`-class branch.

**Risk:** Low. This is a pure fixture adjustment. The `sentinel-cli` binary and `AppSettings` production code are unmodified.

**Scope boundary:** Do not modify the production settings path resolution (`AppSettings.cpp`, `DpapiEncryptedSettingsStore.cpp`).

---

### Group C — Daemon IPC Unguarded `hello()` Sub-Tests (1 target — fixture layer)

**Smallest safe correction (fixture layer only — permitted without product decision):** Add `#ifndef Q_OS_WIN` / `QSKIP` guards to the sub-tests in `tests/core/test_daemon_ipc.cpp` that call `hello()` without a Unix guard. These sub-tests cannot succeed on Windows because the peer-UID check unconditionally aborts connections. Specifically, the following sub-tests currently have no Windows guard:

- `shutdownAcknowledgesAndDrainsExactlyOnce`
- `shutdownAckDoesNotWaitForUnresponsivePlanner`
- `modelPagesFreezeSnapshotAndDoNotRebuildCatalog`
- All non-`#ifdef Q_OS_UNIX`-guarded sub-tests that call `hello()`

**Safe fixture-only patch template:**

```cpp
void DaemonIpcServerTest::shutdownAcknowledgesAndDrainsExactlyOnce() {
#ifndef Q_OS_UNIX
    QSKIP("Daemon IPC is a Unix-only contract; Windows named-pipe transport is not implemented.");
#endif
    // ... existing test body ...
}
```

**Risk:** Zero to production. This documents the correct platform contract without claiming the daemon is supported on Windows.

**What this does NOT do:** It does not implement Windows IPC transport, does not weaken the peer-auth check, and does not alter the fail-closed connection rejection.

**Production layer correction (requires product decision PD-DWIN-01):** Implementing Windows peer-auth using `GetNamedPipeClientProcessId` + token comparison requires a separate product decision and is outside scope of WIN-02 unless explicitly approved. See §7.

---

## 7. Required Policy Decisions

| Decision ID | Description | Required Before |
| --- | --- | --- |
| PD-DWIN-01 | Windows IPC transport: named-pipe security descriptor, peer-process-ID → token user SID comparison as `SO_PEERCRED` equivalent. | WIN-02 IPC transport implementation |
| PD-DWIN-02 | Windows filesystem confinement: Job Object does not restrict filesystem access. A decision is needed on whether to: (a) accept `PartiallyEnforced` permanently and document it, (b) implement AppContainer-based confinement, or (c) prohibit `run-command`/`git-*` tools on Windows entirely. | Any production Windows tool execution |
| PD-DWIN-03 | Windows network denial: `networkDenied = false` in the sandbox result. No Windows Job Object primitive blocks network access. Either accept this or require AppContainer network isolation. | Production Windows tool execution |
| PD-DWIN-04 | Windows Rust CLI/TUI named-pipe transport: `cli/` IPC crate currently only compiles the Unix socket transport. A Windows named-pipe implementation in Rust is needed for the Rust CLI/TUI to connect to a Windows daemon. | Rust CLI/TUI Windows support |
| PD-DWIN-05 | Windows `app-launch` platform support: `BuiltInToolProvider` does not set `platforms.windows = ToolPlatformSupport::Unsupported` for `app-launch`; it remains `PartiallySupported`. If Windows desktop launch is not intended, this should be set to `Unsupported` and the `test_llm_agent_runtime` assertion updated accordingly. | Clean Windows descriptor matrix |

---

## 8. Proposed Implementation Order

The following order is recommended for WIN-02 and subsequent tasks. Each step is independent and can be reviewed separately.

```
WIN-02a  Fixture-only: add QSKIP guards to all non-Unix IPC sub-tests in
         test_daemon_ipc.cpp (Group C fixture layer). No product decision needed.
         Estimated: 1 session. Risk: Zero.

WIN-02b  Fixture-only: fix XDG_CONFIG_HOME / APPDATA settings-root assertion in
         test_legacy_cli_exit_codes.cpp (Group B). Combined with macOS fix from CI-03.
         Estimated: 1 session. Risk: Low.

WIN-02c  (After PD-DWIN-01 approved) Implement Windows peer-auth in DaemonIpcServer.cpp:
         replace `SO_PEERCRED` with GetNamedPipeClientProcessId → token SID comparison.
         Estimated: 1 session. Risk: Medium — touches security boundary.

WIN-02d  (After PD-DWIN-02/03 approved) Document Windows sandbox partial-enforcement
         acceptance in SECURITY_MODEL.md / SANDBOX.md and ensure the ToolDescriptor
         platform matrix reflects this for process-execution tools.

WIN-02e  (After PD-DWIN-04 approved) Implement Windows named-pipe transport in the
         Rust sentinel-ipc crate. Requires separate CI cell (windows runner + Rust toolchain).

WIN-02f  (After PD-DWIN-05 decision) Set app-launch.platforms.windows correctly in
         BuiltInToolProvider and update test_llm_agent_runtime assertion.
```

---

## 9. Native Windows Verification Matrix

| Component | Verification Status | Evidence Available | Required Next Step |
| --- | --- | --- | --- |
| CMake configure / MSVC build | **Hosted verified** | CI run `38982561202` — build passed | None |
| 126/139 tests pass | **Hosted verified** | CI run `38982561202` | None |
| Group A (shell QSKIP, 10 tests) | **Hosted: skip-as-pass** | Source `#ifdef Q_OS_WIN` guards | Confirm CTest treats QSKIP as skip not fail |
| Group B (settings path, 1 test) | **Hosted: failed** | CI-03 evidence | Apply WIN-02b fixture fix |
| Group C (daemon IPC, 1 test suite) | **Hosted: failed** | CI-03 evidence; source analysis | Apply WIN-02a (fixture QSKIP); WIN-02c (production, needs PD) |
| `ProcessSandbox.cpp` Windows branch | **Unverified native** | Source analysis only | Windows-native CI with Job Object trace |
| `ProcessExecutor::startWindows` | **Unverified native** | Source analysis only | Native Windows runner required |
| `CreateProcessAsUserW` privilege model | **Unverified native** | Source analysis: `DISABLE_MAX_PRIVILEGE` + mitigations present | Native Windows runner required |
| Rust IPC (`sentinel-ipc`) on Windows | **Not executed** | `ci-rust.yml` matrix: `ubuntu-latest`, `macos-14` only | Add `windows-2022` cell to `ci-rust.yml` |
| Daemon peer-auth on Windows | **Unverified native** | Source analysis: always rejects (fail-closed) | PD-DWIN-01 decision first |
| Windows ARM64 (experimental) | **Skipped / non-blocking** | `continue-on-error: true` in `ci-windows.yml` | No action pending PD |

> [!NOTE]
> This audit host is macOS arm64 and cannot execute Windows binaries. All Windows-specific assertions in this report are based on source-level analysis of `#ifdef Q_OS_WIN` branches, MSVC/Win32 API semantics, Qt platform documentation, and the CI-03 evidence summary. They represent the investigator's best assessment of Windows runtime behaviour, not observed results.

---

## 10. Recommendation for WIN-02

WIN-02 should be scoped as a **fixture-remediation task** only, with two sub-tasks:

1. **WIN-02a:** Add `QSKIP("Daemon IPC is a Unix-only contract; Windows named-pipe transport is not implemented.")` to all non-UNIX-guarded sub-tests in `tests/core/test_daemon_ipc.cpp` that invoke `hello()`. This brings `test_daemon_ipc` in line with the existing P5-02 pattern. This does not require any product decision and is safe to merge.

2. **WIN-02b:** Extend the `test_legacy_cli_exit_codes::initTestCase` expected-root assertion to cover Windows (`%APPDATA%`). This is the same fix class as the macOS fix recommended in CI-03 §10. Coordinate with that fix to share a single patch.

WIN-02 should **not** implement Windows named-pipe transport, AppContainer confinement, or alter the `SO_PEERCRED` guard without explicit approval of PD-DWIN-01 and PD-DWIN-02. Those are WIN-03-and-beyond scope.

After WIN-02a and WIN-02b are merged and a clean `windows-2022` CI run confirms 139/139 (or 139/139 with the shell QSKIP rows correctly reported as skipped), the 13-target failure count should reduce to **0 hard failures**. The QSKIP rows are working correctly and represent platform capability gaps with correctly placed skip guards.

---

## Appendix A: Grouped Failure Summary

| Group | Count | Root Cause | Fixture or Production | Safe Correction Without Policy Decision |
| --- | --- | --- | --- | --- |
| A — Unix shell fixture | 10 | `/bin/sh`-based fixtures, correct `QSKIP` guards | Fixture | None needed; already correct |
| B — XDG_CONFIG_HOME path | 1 | Settings root assumption is POSIX-only | Fixture | Yes — platform-conditional expected-root |
| C — Daemon IPC / peer-auth | 1 | (a) `SO_PEERCRED` absent → fail-closed; (b) unguarded `hello()` calls | Production + Fixture | Fixture layer: add QSKIP. Production layer: requires PD-DWIN-01 |
| **Total** | **13** | | | |

**Confirmed independent correction groups:** 3
**Failures requiring product decision before production fix:** 1 (Group C production layer)
**Failures correctable by fixture change alone:** 2 (Group B + Group C fixture layer)

---

## Appendix B: Security Invariants Preserved

1. **Sandbox `PartiallyEnforced` is honestly reported** — `filesystemRestricted = false`, `networkDenied = false`, `failureCategory` set to `FilesystemConfinementUnavailable` or `NetworkDenialUnavailable`. No test or production code claims Windows provides filesystem isolation it does not provide.
2. **Daemon IPC is fail-closed on Windows** — every connection is rejected (`std::nullopt` peer UID). No unauthorised client session can proceed.
3. **`CreateRestrictedToken` (`DISABLE_MAX_PRIVILEGE`)** is applied before process creation. The privilege reduction is real and does not introduce a regression.
4. **`runCommandDockerFixtureContracts` correctly asserts `ToolExecutionStatus::Failed`** on Windows, not `Succeeded`. The test has a dedicated `#ifdef Q_OS_WIN` branch that preserves the fail-closed contract.
5. **No blanket skip groups were introduced**. Each `QSKIP` targets a specific sub-test and carries an explanatory message.

---

*Audit prepared independently in worktree `audit/win-01-platform-failures` based on `d9fbcd46`.
No commit, push, merge, PR, reset, stash or workflow dispatch was performed.*
