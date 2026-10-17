# CI-04 — Integrated Main Stabilization Audit and Candidate Validation

**Audit Date**: 2026-10-17 (Europe/Istanbul)  
**Repository**: `https://github.com/sentinel-projects/sentinel`  
**Priority**: Critical — Candidate Consolidation and Main Stabilization  
**Worktree**: `<managed-worktrees>/ci-04-main-stabilization/Sentinel-sanitized`  
**Branch**: `integration/ci-04-main-stabilization`  
**Base Commit (`origin/main`)**: `d9fbcd463620ab4e9aa543034dcd436580c332a0`  
**Host Environment**: macOS Darwin 27.0.1 (arm64), AppleClang 21.0.0, Qt 6.12.0, Rust 1.98.1  
**Target Architecture**: C++20, Qt 6/QML Modular Monolith; Primary target: Fedora KDE Plasma; Cross-platform: Linux, macOS, Windows  

> [!IMPORTANT]
> In strict accordance with audit constraints, no `git commit`, `git push`, `git merge`, pull request creation, or GitHub Actions workflow dispatch was executed. All changes reside as verified uncommitted modifications in the independent `integration/ci-04-main-stabilization` worktree, ready for maintainer-authorized integration.

---

## 1. Actual Base SHA & Ancestry Verification

### 1.1 Base Commit Identity
The active candidate branch was created as an independent git worktree based directly on `origin/main`:

```text
commit d9fbcd463620ab4e9aa543034dcd436580c332a0 (HEAD -> integration/ci-04-main-stabilization, origin/main, origin/HEAD)
Merge: 44d6313b 5ccd7eda
Author: Emir <emir@users.noreply.github.com>
Date:   Sat Oct 17 00:46:17 2026 +0300

    Merge branch 'test/p5-02-daemon-ipc-reliability'
```

### 1.2 Upstream Integration History
Verification of first-parent commit ancestry confirms the status of the four previously reported scopes already contained in `origin/main` (`d9fbcd46`):

1. **CI-01 Linux Descriptor Correction**:
   - Commit `865c7093` (`fix/ci-01-linux-agent-descriptors`), merged in commit `3c6dd911`.
   - Verified present: `LlmAgentRuntime::availableTools()` projection and platform matrix in `tests/core/test_llm_agent_runtime.cpp`.
2. **CI-02 macOS Sandbox Remediation**:
   - Commit `dcaa9cb6` (`fix/ci-02-macos-sandbox-format`), merged in commit `015a8f10`.
   - Verified present: Mach-O load command parser and Cellar authorization in `core/src/runtime/ProcessSandbox.cpp`, preflight stderr capture in `core/src/runtime/RealToolExecutor.cpp`, and skip guards in `tests/core/test_real_tool_executor_tools.cpp`.
3. **P1-05 Legacy CLI Exit-Code Implementation**:
   - Branch `review/p1-05-current-main`, merged in commit `44d6313b`.
   - Verified present: Exit code taxonomy (0=success, 1=usage, 2=failure, 3=timeout), `sentinel-cli` subcommands, and initial fixture tests.
4. **P5-02 Daemon IPC Reliability Implementation**:
   - Commits `9dbbbcdb` and `5ccd7eda` (`test/p5-02-daemon-ipc-reliability`), merged in commit `d9fbcd46`.
   - Verified present: Peer authorization baseline, backpressure disconnect handling, and basic daemon restart reconciliation.

---

## 2. Imported Changes & Source Hashes

Three follow-up scopes required reconciliation against `origin/main`:
1. **P1-05.1 Portable Legacy CLI Test Isolation**:
   - Source worktree (read-only): `<managed-worktrees>/p1-05-current-main/Sentinel-sanitized`
   - Reconciled file: `tests/integration/test_legacy_cli_exit_codes.cpp`
   - Reference audit: `docs/audits/P1_05_1_PORTABLE_CLI_TEST_ISOLATION_RESULT.md`
2. **P5-02.1 Daemon IPC Security & SQLite Reconciliation Hardening**:
   - Source worktree (read-only): `<managed-worktrees>/review/p5-02-ipc-security/Sentinel-sanitized`
   - Reconciled files:
     - `apps/sentinel-daemon/service/DaemonIpcServer.h`
     - `apps/sentinel-daemon/service/DaemonIpcServer.cpp`
     - `tests/core/test_daemon_ipc.cpp`
     - `docs/audits/P5_02_DAEMON_IPC_RELIABILITY_RESULT.md` (appended Section 13)
   - Reference audit: `docs/audits/P5_02_1_IPC_SECURITY_PERSISTENCE_REVIEW.md`
3. **CI-Pinned LLVM 18 Formatting Violations**:
   - Narrow formatting alignment per CI-pinned `clang-format 18.1.8` (`build/p2-tools/bin/clang-format`):
     - `core/src/runtime/RealToolExecutor.cpp` (lines 797–800, 818–823, 968–971)
     - `apps/sentinel-daemon/service/DaemonIpcServer.cpp` (lines 389, 403, 429, 538)
     - `tests/core/test_daemon_ipc.cpp` (lines 1241–1271)

### 2.1 Cryptographic SHA-256 Digest Matrix

| Repository-Relative Path | Baseline SHA-256 (`d9fbcd46`) | Candidate SHA-256 (`ci-04`) | Status / Modification Rationale |
| --- | --- | --- | --- |
| `apps/sentinel-daemon/service/DaemonIpcServer.cpp` | `01ae925d063913fb6d0cc9222f2c162cabe85c98424928a883ff530d5334a39f` | `725582ed2b899fee73a5dec6621e747b38930b26861a912d0094005e56e71a4c` | P5-02.1 transactional SQLite reconciliation, stopServer unsubscription, send guards, descriptor checks, clang-format 18 alignment |
| `apps/sentinel-daemon/service/DaemonIpcServer.h` | `5ecae1d16465526f74d89f8650811f22a5baf3733a2b792b969b43e5ca8512d1` | `f38fdfe8317cc3523a71702c803370fb3ca3c9e46d29cc41171c89134ca8ea25` | P5-02.1 test seam immutability guard while listening |
| `core/src/runtime/RealToolExecutor.cpp` | `0875c19526d07e1aa368f0ffba9f3d80f46c85c14b8ebc5399cb6ddc84bf87f2` | `082f8529fc2232cd0b8039bcf5d5d0a92931b3b856c5741a14974e006d662268` | Clang-format 18.1.8 alignment on preflight conditions and ternary operator |
| `tests/core/test_daemon_ipc.cpp` | `f057bba2e3cff19040a5fe9bf2b3cfb2e372165c716ad2e9c56a350a52da3e6b` | `e49551dd8f81a5cab34caa98e2ff1c425e859ce0f8a7899b63b4b8e2522f212a` | P5-02.1 multi-run atomic restart test, test-seam immutability test, controller cleanup, clang-format 18 formatting |
| `tests/integration/test_legacy_cli_exit_codes.cpp` | `cab36aa414c461c52e523b6f22031025f712b86129d055aed2228f4fb4ba4b0c` | `325ea4941e4d76f54c7cfdee2c652842b92de562a89d86a002fb711323ef27bc` | P1-05.1 symmetric child-scoped test environment, cross-platform relative settings derivation, Windows profile compatibility |
| `docs/audits/P5_02_DAEMON_IPC_RELIABILITY_RESULT.md` | `0cdb48a9be312c6e1924ea929369ed604d9f42366f144c1a521c5edfd7626bfc` | `e82b24bf4f3585704759b9c9a410e0e8f7613337bf47a35df62825adb740cb8b` | Appended Section 13 dated acceptance boundaries |
| `docs/audits/P1_05_1_PORTABLE_CLI_TEST_ISOLATION_RESULT.md` | *(absent)* | `716f53183c4505b03aad0be1afa293a66b6cb40f83335338da63e61de008715d` | Imported P1-05.1 audit report |
| `docs/audits/P5_02_1_IPC_SECURITY_PERSISTENCE_REVIEW.md` | *(absent)* | `dafa99c4271970652439fb3c6236f8c466e44ceafe636f06af338114eb119ebc` | Imported P5-02.1 audit report |
| `docs/audits/CI_04_INTEGRATED_MAIN_STABILIZATION_RESULT.md` | *(absent)* | *(new file)* | This deliverable audit result |

---

## 3. Reconciliation of Already Merged Code

All previous subsystem contracts remain strictly preserved:
1. **Tool Execution Gateway**: `ToolDescriptor` registry contracts, `IFileSystemService`, `ProcessExecutor`, and `ProcessSandbox` boundaries remain unchanged.
2. **Bounded Streaming & Providers**: `IChatProvider`, `OllamaRuntime`, `LocalInferenceRuntime`, and bounded chunk limits are preserved without regressions.
3. **Agent Loop & Runtime**: `AgentRuntime` lifecycle, `AgentLoop` state graph, and final answer acceptance boundaries remain intact.
4. **Rust CLI & TUI Workspace**: IPC framing protocol (`sentinel-ipc`), terminal rendering (`sentinel-tui`, `sentinel-mascot`), and CLI dispatch (`sentinel-cli`) remain locked and unchanged.

---

## 4. Legacy CLI Isolation Results (Part A)

### 4.1 Root Cause & Remediation
Prior to P1-05.1, `tests/integration/test_legacy_cli_exit_codes.cpp` failed on macOS during fixture initialization (`initTestCase()`) because the test asserted that `settingsFile` started with `XDG_CONFIG_HOME`. Under macOS, Qt standard paths route preferences to `~/Library/Preferences/sentinel-cli/settings.json`, causing the test assertion to fail before executing any of the 20 exit-code test cases.

The remediation:
1. Added helper `applyTestEnvironment(QProcessEnvironment& env, const QString& root)` configuring `HOME`, `CFFIXED_USER_HOME`, `XDG_*`, and Windows `%USERPROFILE%`/`%APPDATA%`/`%LOCALAPPDATA%` exclusively within child process environments.
2. The global process environment (`environ`) is never modified (`::setenv` and `qputenv` are never called).
3. Derived `settingsSuffix_` relative to the temporary probe root using `QDir::fromNativeSeparators`, asserting that the settings file is rooted inside the disposable directory regardless of platform OS conventions.
4. Updated `chatRefusedWithoutConversationStoreSetsFailure()` to block Windows `USERPROFILE/Documents` alongside `HOME/Documents` and `XDG_DOCUMENTS_HOME`.

### 4.2 Test Verification
The Legacy CLI test suite executed without any external environment wrappers or overrides (`CFFIXED_USER_HOME` was not passed to `ctest`):

```text
********* Start testing of LegacyCliExitCodesTest *********
Config: Using QtTest library 6.12.0, Qt 6.12.0 (arm64-little_endian-lp64 shared (dynamic) release build; by Apple LLVM 21.0.0 (clang-2100.3.34.2)), macos 27.0.1
PASS   : LegacyCliExitCodesTest::initTestCase()
PASS   : LegacyCliExitCodesTest::usageWithoutArgsReturnsSuccess()
PASS   : LegacyCliExitCodesTest::unknownCommandReturnsUsageError()
PASS   : LegacyCliExitCodesTest::chatWithoutPromptReturnsUsageError()
PASS   : LegacyCliExitCodesTest::configUnknownKeyReturnsUsageError()
PASS   : LegacyCliExitCodesTest::modelPullBlankNameReturnsUsageError()
PASS   : LegacyCliExitCodesTest::statusAndConfigSucceedWithoutConfiguration()
PASS   : LegacyCliExitCodesTest::modelListSucceedsAgainstFixture()
PASS   : LegacyCliExitCodesTest::modelListFailsWhenEndpointIsRefused()
PASS   : LegacyCliExitCodesTest::modelPullSucceedsAgainstFixture()
PASS   : LegacyCliExitCodesTest::modelPullFailureReturnsNonzero()
PASS   : LegacyCliExitCodesTest::modelPullHttpErrorReturnsNonzero()
PASS   : LegacyCliExitCodesTest::modelPullStallReturnsTimeout()
PASS   : LegacyCliExitCodesTest::chatWithoutConfigurationReturnsNonzero()
PASS   : LegacyCliExitCodesTest::chatProviderFailureReturnsNonzero()
PASS   : LegacyCliExitCodesTest::chatSucceedsAgainstFixture()
PASS   : LegacyCliExitCodesTest::chatStallReturnsTimeout()
PASS   : LegacyCliExitCodesTest::chatRefusedWithoutConversationStoreSetsFailure()
PASS   : LegacyCliExitCodesTest::interruptedPullExitsAbnormally()
PASS   : LegacyCliExitCodesTest::cleanupTestCase()
Totals: 20 passed, 0 failed, 0 skipped, 0 blacklisted, 10935ms
********* Finished testing of LegacyCliExitCodesTest *********
```

**Result**: **20 passed, 0 failed, 0 skipped** across all exit-code assertion categories (Success: 0, Usage: 1, Failure: 2, Timeout: 3).

---

## 5. SQLite & IPC Security Results (Part B)

### 5.1 Hardening Summary
1. **Fail-Closed Peer Authorization**:
   - `querySocketPeerUid(descriptor)` immediately returns `std::nullopt` if `descriptor < 0`.
   - On Linux, enforced `size >= sizeof(peer)` to reject truncated credential structures.
   - Sockets failing credential validation are aborted and deleted before parsing or buffering frames; no errors or session metadata are leaked to unauthorized callers.
2. **Listener Test Seam Immutability**:
   - `setPeerUidResolverForTesting` refuses any modification once `m_server.isListening()`. Verified via `testSeamCannotBeModifiedWhileListening`.
3. **Positional Binding Correction**:
   - Replaced flawed `addBindValue()` loop appending with explicit index-based parameter binding (`bindValue(0, ...)`, `bindValue(1, sessionId)`), ensuring that all interrupted sessions in multi-session environments are updated on disk.
4. **Transactional Consistency & Atomicity**:
   - Wrapped SQLite reconciliation in `db.transaction()` and `db.commit()`. If any update query fails or the commit fails, the transaction rolls back cleanly, returning `false` and preventing disk/memory divergence. Verified via `multipleInterruptedRunsReconciledAtomicallyOnRestart`.
5. **Clean Unsubscription on Stop**:
   - `stopServer()` now explicitly unsubscribes `m_agentSubscription` from `AgentRuntime`, preventing ghost events from reaching dead or restarted listeners.
6. **Socket State Guards on Outbound Writes**:
   - `send()` verifies `socket->state() != QLocalSocket::UnconnectedState && socket->state() != QLocalSocket::ClosingState` before checking pending byte limits or issuing writes.

### 5.2 Test Verification
The enhanced daemon IPC test suite executed cleanly:

```text
Totals: 42 passed, 0 failed, 0 skipped, 0 blacklisted, 3205ms
********* Finished testing of DaemonIpcServerTest *********
1/1 Test #115: test_daemon_ipc ..................   Passed    4.41 sec
```

- New test `multipleInterruptedRunsReconciledAtomicallyOnRestart`: Verified that 3 sessions across 2 interrupted states (`running`, `approval`) and 1 `completed` state are reconciled transactionally, directly agree with SQLite on disk, and remain idempotent across consecutive restarts.
- New test `testSeamCannotBeModifiedWhileListening`: Verified that attempting to overwrite the peer UID resolver while listening is rejected and native authentication continues functioning securely.
- Desktop Agent Reliability IPC (`test_agent_reliability_ipc`): Verified COR-17 through COR-20 on real Unix transport (7/7 passed).

---

## 6. Formatting Gate Results (Part C)

### 6.1 Repository CI Pinned Clang-Format 18.1.8
The formatting gate was executed using the repository's pinned `clang-format 18.1.8` binary located at `build/p2-tools/bin/clang-format`.

### 6.2 Remediation of Known Violations
1. **`core/src/runtime/RealToolExecutor.cpp`**:
   - Lines 797–800: Aligned multi-line `if` condition formatting.
   - Lines 818–823: Aligned nested ternary operator indentation.
   - Lines 968–971: Aligned boolean condition formatting for `missingDocker`.
2. **`apps/sentinel-daemon/service/DaemonIpcServer.cpp`**:
   - Lines 389, 403, 429, 538: Formatted empty struct brace initializers (`struct stat info {};`, `struct ucred peer {};`).
3. **`tests/core/test_daemon_ipc.cpp`**:
   - Lines 1241–1271: Formatted multi-line JSON builder expressions.

### 6.3 Gate Execution Against Integration Diff
Running the exact CI quality gate command from `.github/workflows/ci-linux.yml`:

```bash
git diff -U0 origin/main -- apps core integrations plugins tests | \
  build/p2-tools/bin/clang-format-diff.py -p1 \
  -binary build/p2-tools/bin/clang-format
```

**Output**: Empty (0 diff lines).  
**Exit code**: **0 (Clean)**.  
**Whitespace verification (`git diff --check`)**: **0 errors (Clean)**.

---

## 7. Full C++ & Rust Test Totals (Part D)

### 7.1 C++ Test Suite (CTest)
Total tests discovered in `build/tests`: **139** (matching the baseline target count of 139).

| Test Execution Mode | Command | Discovered | Passed | Failed | Skipped | Total Time | Status |
| --- | --- | --- | --- | --- | --- | --- | --- |
| **Sequential CTest** | `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | 139 | **139** | 0 | 0 | 157.59 s | **100% PASS** |
| **Parallel CTest (j=8)** | `QT_QPA_PLATFORM=offscreen ctest --preset tests -j8 --output-on-failure` | 139 | **139** | 0 | 0 | 52.04 s | **100% PASS** |

#### Key Subsystem Results
- `test_legacy_cli_exit_codes`: 20/20 passed (10.94 s).
- `test_daemon_ipc`: 42/42 passed (3.21 s).
- `test_agent_reliability_ipc` (COR-17..COR-20): 7/7 passed (3.06 s).
- `test_agent_reliability_harness`: 102 passed, 0 failed, 1 intentional skip (COR-11 assembly) (15.49 s).
- `test_llm_agent_runtime`: 53/53 passed (0.58 s).
- `test_real_tool_executor_tools`: 51 passed, 1 skip (npx unavailable) (2.01 s).
- Provider contracts (`test_provider_contracts`, `test_native_provider_contract`, `test_bounded_streaming_contract`, `test_local_inference`): 100% passed.

### 7.2 Rust Workspace Matrix (`cli/`)

| Target / Crate | Test Command | Passed | Failed | Ignored | Status |
| --- | --- | --- | --- | --- | --- |
| `sentinel-cli` | `cargo test -p sentinel-cli` | 15 | 0 | 0 | **PASS** |
| `sentinel-ipc` | `cargo test -p sentinel-ipc` | 26 | 0 | 0 | **PASS** |
| `sentinel-mascot` | `cargo test -p sentinel-mascot` | 22 | 0 | 0 | **PASS** |
| `sentinel-tui` | `cargo test -p sentinel-tui` | 78 | 0 | 0 | **PASS** |
| **Complete Workspace** | `cargo test --locked` | **141** | **0** | **0** | **100% PASS** |

- `cargo fmt --all --check`: Clean (exit code 0).
- `cargo clippy --locked --all-targets -- -D warnings`: Clean (0 warnings, exit code 0).

### 7.3 Integrity & Security Scans
- `python3 tools/ipc/generate.py --check`: Clean (0 diff, code 0).
- `python3 tools/ipc/generate_desktop.py --check`: Clean (0 diff, code 0).
- `python3 tools/analysis/check_personal_paths.py`: **0 file(s) flagged (Clean, code 0)**.
- Credential token scan (`sk-`, `ghp_`, passwords): **0 matches**.

---

## 8. Verification Classifications & Matrix Status (Part E)

To prevent conflating local host evidence with hosted CI results:

| Verification Category | Platform / Runner | Current Candidate Status | Verification Class | Authoritative Notes |
| --- | --- | --- | --- | --- |
| **Local macOS Validation** | macOS Darwin 27.0.1 (arm64) | **GREEN (100%)** | Verified on Candidate | 139/139 CTest passed; 141/141 Rust passed; 20/20 Legacy CLI passed; 42/42 Daemon IPC passed. |
| **Hosted macOS CI** | macOS 14 arm64 (`ci-macos.yml`) | **Pending Integration** | Unverified for Candidate | Historical run `38982561276` failed on #118 (Legacy CLI path) and #115 (IPC restart). Both are remediated locally in this candidate. |
| **Hosted Linux CI** | Ubuntu 22.04 x86_64 (`ci-linux.yml`) | **Pending Integration** | Unverified for Candidate | Historical run `38982561228` failed on format (RealToolExecutor, DaemonIpcServer) and #115 (Daemon IPC). Both are remediated locally. |
| **Hosted Windows CI** | Windows Server 2022 (`ci-windows.yml`) | **Blocked (Pre-existing)** | Unverified for Candidate | Blocked by pre-existing Windows sandbox support and missing Windows named-pipe authentication (PD-D07). |
| **Rust Matrix CI** | Ubuntu & macOS-14 (`ci-rust.yml`) | **GREEN** | Locally & Matrix Clean | Rust workspace passes 141 tests, fmt, and clippy without code changes. |
| **CodeQL Analysis** | Ubuntu 22.04 x86_64 (`codeql-analysis.yml`) | **Pending Dispatch** | Unverified for Candidate | Clean historical baseline; candidate changes maintain type safety and bound limits. |
| **Formatting & Quality** | Pinned Clang-Format 18.1.8 | **GREEN (100%)** | Verified on Candidate | 0 formatting discrepancies across all touched C++ files. |
| **AddressSanitizer (ASan)** | C++ ASan preset | **Not Executed** | Pending Dispatch | Requires explicit maintainer CI execution. |
| **Packaging & Release** | macOS / Linux packages | **Not Executed** | Release Stage | Outside candidate stabilization scope. |

---

## 9. Remaining Blockers & Outstanding Product Decisions

1. **AC-P1-04 (Hosted 100-Repeat Unix Stress)**:
   - Remains **Unverified / Blocked**.
   - Requires two independent hosted 100-repeat Unix runs on `macos-14` and `ubuntu-22.04`. In accordance with audit constraints, hosted workflows were not dispatched.
2. **PD-D05 (Legacy CLI Disposition)**:
   - Remains **Pending Decision**.
   - Whether the in-process Legacy C++ CLI (`apps/sentinel-cli`) should be retained alongside or retired in favor of the canonical daemon-backed Rust CLI (`cli/`) remains undecided by maintainers. This stabilization ensures that as long as the Legacy CLI is retained, its tests pass portably across platforms.
3. **PD-D07 (Windows Named-Pipe Trust and Security)**:
   - Remains **Pending Decision**.
   - Windows named-pipe peer authorization using `GetNamedPipeHandleStateW` and SID impersonation is not implemented. Windows CLI transport explicitly returns `Error::Unavailable`.
4. **PD-D10 (Provider Ownership & Hard-Stop Policy)**:
   - Remains **Pending Decision**.
   - Cooperative drain versus hard-stop process termination policies remain pending maintainer determination.

---

## 10. Recommended Maintainer-Authorized Integration Procedure

To integrate this candidate into `main`:

1. **Working Tree Review**:
   Review the exact git diff in worktree `<managed-worktrees>/ci-04-main-stabilization/Sentinel-sanitized`:
   ```bash
   git diff
   ```
2. **Commit Candidate Changes**:
   Stage and commit the 6 modified files and 2 imported audit reports on branch `integration/ci-04-main-stabilization`:
   ```bash
   git add apps/sentinel-daemon/service/DaemonIpcServer.cpp \
           apps/sentinel-daemon/service/DaemonIpcServer.h \
           core/src/runtime/RealToolExecutor.cpp \
           tests/core/test_daemon_ipc.cpp \
           tests/integration/test_legacy_cli_exit_codes.cpp \
           docs/audits/P5_02_DAEMON_IPC_RELIABILITY_RESULT.md \
           docs/audits/P1_05_1_PORTABLE_CLI_TEST_ISOLATION_RESULT.md \
           docs/audits/P5_02_1_IPC_SECURITY_PERSISTENCE_REVIEW.md \
           docs/audits/CI_04_INTEGRATED_MAIN_STABILIZATION_RESULT.md
   git commit -m "fix(ci): consolidate main stabilization corrections (CI-04)"
   ```
3. **Pull Request Submission**:
   Push branch `integration/ci-04-main-stabilization` to `origin` and open a PR targeting `main`.
4. **Automated CI Validation**:
   - `Linux CI`: Confirms format gate passes (18.1.8) and `test_daemon_ipc` passes.
   - `macOS CI`: Confirms `test_legacy_cli_exit_codes` and `test_daemon_ipc` pass.
   - `ci-rust`: Confirms Rust workspace matrix.
5. **Authorized Stress Dispatch**:
   Following green CI, maintainers may trigger the manual `ipc-stress` workflow in `ci-rust.yml` to close AC-P1-04.
