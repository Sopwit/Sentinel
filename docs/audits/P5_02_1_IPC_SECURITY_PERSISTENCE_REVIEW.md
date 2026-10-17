# P5-02.1 — IPC Security, Persistence & Acceptance Hardening Review

**Audit Date**: 2026-10-17 (Europe/Istanbul)  
**Phase**: P5 — Daemon & IPC Reliability  
**Target Candidate**: P5-02 Deterministic Daemon IPC Reliability  
**Review Isolation Worktree**: `review/p5-02-ipc-security` (`<managed-worktree-root>/review/p5-02-ipc-security/Sentinel-sanitized`)  
**Source Worktree (Read-Only)**: `p5-02-daemon-ipc-reliability/Sentinel-sanitized`  
**Host Platform**: macOS Darwin 27.0.1 (arm64, AppleClang 21.0.0, Qt 6.12.0)  

---

## 1. Source and HEAD Reconciliation

### 1.1 Baseline and Reported State
The user request reported the P5-02 branch as `test/p5-02-daemon-ipc-reliability` with reported HEAD `9dbbbcdbb034d880b562ed63f275cc2fd597ebc5`.

Independent read-only git inspection revealed:
1. Branch `test/p5-02-daemon-ipc-reliability` had progressed by one commit beyond `9dbbbcdb` to `5ccd7eda18d9206c72e482ebd99079ebf08b1945` (`docs(audit): record P5-02 daemon IPC reliability closure`).
2. Both commits (`9dbbbcdb` and `5ccd7eda`) were merged into `main` at `d9fbcd463620ab4e9aa543034dcd436580c332a0`.
3. In the existing source worktree (`p5-02-daemon-ipc-reliability/Sentinel-sanitized`), the working tree was completely clean with zero uncommitted changes and only `build/` and `cli/target/` ignored.
4. The files modified in P5-02 at `5ccd7eda` were byte-for-byte identical to the merged state at `main` (`d9fbcd46`).

### 1.2 Isolation Guarantee
A dedicated, isolated review worktree was created:
- **Worktree path**: `<managed-worktree-root>/review/p5-02-ipc-security/Sentinel-sanitized`
- **Branch**: `review/p5-02-ipc-security`
- **Base commit**: `d9fbcd463620ab4e9aa543034dcd436580c332a0`
- **Source worktree status**: `p5-02-daemon-ipc-reliability/Sentinel-sanitized` was inspected strictly read-only; no commits, reset, stash, merge, push, workflow dispatch, or PRs were performed.

### 1.3 Exact SHA-256 Digest Matrix

| Repository-Relative Path | SHA-256 at `9dbbbcdb` | SHA-256 at `5ccd7eda` | SHA-256 in Review Candidate (`review/p5-02-ipc-security`) | Disposition |
| --- | --- | --- | --- | --- |
| `apps/sentinel-daemon/service/DaemonIpcServer.cpp` | `d4c1b1bfaf4be94e6d796d8a099ee3f4da92bc4c6e825cf3557143f1447a6717` | `01ae925d063913fb6d0cc9222f2c162cabe85c98424928a883ff530d5334a39f` | `8044f7853dbc412f71a315182711c89da8299a4fe2fecb2d89ff86b9661cbb5d` | Hardened |
| `apps/sentinel-daemon/service/DaemonIpcServer.h` | `5ecae1d16465526f74d89f8650811f22a5baf3733a2b792b969b43e5ca8512d1` | `5ecae1d16465526f74d89f8650811f22a5baf3733a2b792b969b43e5ca8512d1` | `f38fdfe8317cc3523a71702c803370fb3ca3c9e46d29cc41171c89134ca8ea25` | Hardened |
| `cli/crates/sentinel-ipc/src/lib.rs` | `ec145b9d21172d27dc38f2c98bacce23700c4f3b7bbc2e810c9b40c9fcb593b0` | `ec145b9d21172d27dc38f2c98bacce23700c4f3b7bbc2e810c9b40c9fcb593b0` | `ec145b9d21172d27dc38f2c98bacce23700c4f3b7bbc2e810c9b40c9fcb593b0` | Unchanged |
| `docs/audits/P5_02_DAEMON_IPC_RELIABILITY_RESULT.md` | *(absent)* | `0cdb48a9be312c6e1924ea929369ed604d9f42366f144c1a521c5edfd7626bfc` | `bf5a0e5af8ef67350740323fa53880b9f93bd94867d749763da3d67978b012ec` | Clarified §13 |
| `tests/core/test_daemon_ipc.cpp` | `ee0069c834415d1a92f2ee4d99de07c64a3782dc97d49a730f020a852b5d10a7` | `f057bba2e3cff19040a5fe9bf2b3cfb2e372165c716ad2e9c56a350a52da3e6b` | `fe663f84d68301c4ae5716641960fb1abb504b91e9e9d6c3edaa6e7937730806` | Hardened |
| `tests/desktop/test_agent_reliability_ipc.cpp` | `05fba7279149683db81abd5fa4b4ce52a9bdf3ff950617b457a694edd5915565` | `e9898ca467f80aa14bed3327b42ce8f822d82c24bdacab789c3c9cecc488ae75` | `e9898ca467f80aa14bed3327b42ce8f822d82c24bdacab789c3c9cecc488ae75` | Unchanged |
| `tests/core/test_agent_reliability_harness.cpp` | `48693e5682d452814374730c9a09f4a30038944f9f3bdf94f77b8583181ce4d6` | `ec62ce1d4baf2a739457f38a7b991417d8fecfd8490c6ce4ca1e722c68584e3a` | `ec62ce1d4baf2a739457f38a7b991417d8fecfd8490c6ce4ca1e722c68584e3a` | Unchanged |

---

## 2. Peer Authorization Security Findings (Part A)

### 2.1 Analysis of Peer Authorization Implementation
The audit examined `querySocketPeerUid`, `handleNewConnection`, `setPeerUidResolverForTesting`, and `PeerUidResolver`.

1. **Kernel Credential Query**:
   - macOS uses `::getpeereid(descriptor, &peerUid, &peerGid)` which queries trusted kernel credentials on Unix domain stream sockets.
   - Linux uses `::getsockopt(descriptor, SOL_SOCKET, SO_PEERCRED, &peer, &size)` to obtain `struct ucred`.
   - **Hardening Applied**: `querySocketPeerUid` now validates `descriptor >= 0` before issuing syscalls, and on Linux enforces `size >= sizeof(peer)` to reject partial or truncated credential structures.
2. **Fail-Closed Default**:
   - If peer credentials cannot be resolved (`std::nullopt`), or if `peerUid.value() != ::getuid()`, the connection is immediately aborted (`socket->abort(); socket->deleteLater(); continue;`).
3. **Execution Ordering**:
   - Credential validation occurs strictly inside `handleNewConnection()` before attaching `readyRead` or `disconnected` signals and before parsing or buffering any frame.
   - Rejected sockets are never added to `m_clients`.
4. **Information Leakage Prevention**:
   - No protocol frames, errors, or session data are sent to rejected peers; read on the client returns EOF (`read() == 0`).
   - Logging (`qInfo("IPC client connected")`) is deferred until after credential verification succeeds.
5. **Root vs. Non-Root Semantics**:
   - The authorization rule is strictly symmetric: `peerUid.value() == ::getuid()`.
   - A daemon running as an unprivileged user (UID 1000) rejects connection attempts from UID 0 (root).
   - A daemon running as root accepts only UID 0.
6. **Test Seam Safety & Exposure**:
   - `setPeerUidResolverForTesting` is a public member on `DaemonIpcServer`.
   - In production composition (`DaemonService.cpp`), `DaemonIpcServer` is instantiated internally and neither CLI commands, IPC requests, settings, plugins, nor untrusted input can reach or invoke `setPeerUidResolverForTesting`.
   - **Hardening Applied**: `setPeerUidResolverForTesting` was fortified with an active listener check:
     ```cpp
     void setPeerUidResolverForTesting(PeerUidResolver resolver) {
         if (m_server.isListening()) {
             return;
         }
         m_peerUidResolver = std::move(resolver);
     }
     ```
     Once the server is listening, attempts to modify or hijack the test resolver are silently ignored, preventing any accidental or dynamic alteration during operation. Tested by new test `testSeamCannotBeModifiedWhileListening`.

---

## 3. Backpressure & Memory Safety Boundaries (Part B)

### 3.1 Accounting Model & Ceilings
The application-level memory bounds are strictly separated from kernel socket buffers and total process RSS:

1. **Per-Client Application Queue Boundary**:
   - Frame inbound accumulator: `input.size() <= protocol::max_frame_bytes` (256 KB = 262,144 bytes). Exceeding this boundary triggers `message-too-large` error envelope and immediate disconnection.
   - Socket outbound queue: `socket->bytesToWrite() + bytes.size() <= protocol::max_pending_bytes` (1 MB = 1,048,576 bytes). If a slow client fails to read and unwritten Qt socket bytes exceed 1 MB, the server executes `socket->abort()` and drops the connection.
2. **Maximum Concurrent Clients**:
   - Strictly capped at `32` clients. Excess incoming sockets are aborted and scheduled for deletion (`deleteLater`) before any state allocation.
3. **Aggregate Application Pending Memory Ceiling**:
   - Maximum simultaneous application-level buffering across all 32 connected clients:
     $$\text{Max Pending Write} = 32 \times 1\text{ MB} = 32\text{ MB}$$
     $$\text{Max Inbound Frame Accumulator} = 32 \times 256\text{ KB} = 8\text{ MB}$$
     $$\text{Total Application IPC Framing Ceiling} = 40\text{ MB}$$
   - *Kernel distinction*: Kernel socket send/receive buffers (`SO_SNDBUF`/`SO_RCVBUF`, typically 8 KB–256 KB per descriptor) are managed by the kernel network stack and are independent of application heap memory.
   - *Process RSS distinction*: Daemon total RSS includes Qt shared libraries, Core memory allocations, SQLite memory caches, and in-process model execution buffers. The 40 MB ceiling strictly bounds application IPC pending write/read buffers, not global daemon RSS.
4. **Slow vs. Fast Client Fairness**:
   - In `publishEvent`, the loop iterates over all client sockets subscribed to the session.
   - A slow client hitting the 1 MB threshold is aborted in isolation inside `send()`, without pausing or stalling delivery to fast clients.
   - Verified in `cor20_multiClientBackpressureAndBufferSaturation` and `slowClientPendingBytesBackpressure`: fast clients receive the authoritative `run.completed` event without dropped frames while slow clients are severed.
5. **No Run Cancellation & Single Terminal Delivery**:
   - Dropping a client due to backpressure or unexpected disconnection does not abort or cancel active runs (`disconnectDoesNotCancelChat`, COR-17).
   - Only exactly one terminal event (`run.completed`, `run.failed`, `run.cancelled`) is produced per run.
6. **No Unbounded Event History & Memory Safety on Disconnect**:
   - `DaemonIpcServer` does not store historical event logs; events are broadcast transiently to active sockets, and only the latest run state per session is held in `m_lastRuns`.
   - On disconnect, `m_clients.remove(socket)` and `socket->deleteLater()` cleanly release per-client resources without use-after-free.
   - **Hardening Applied**: `DaemonIpcServer::send` now guards against writing to disconnected or closing sockets:
     ```cpp
     if (!socket || socket->state() == QLocalSocket::UnconnectedState ||
         socket->state() == QLocalSocket::ClosingState) {
         return;
     }
     ```

---

## 4. SQLite Restart Reconciliation & Idempotency (Part C)

### 4.1 Defects Discovered and Corrected

During the review of `DaemonIpcServer::openSessionProjectionStore()`, two critical defects were uncovered:

1. **Positional Parameter Accumulation Bug in Loop**:
   - `updateQuery.prepare("UPDATE session_projection SET payload = ? WHERE session_id = ?")` was prepared once before the `while (query.next())` loop.
   - Inside the loop, `updateQuery.addBindValue(...)` was called.
   - In Qt SQL, `addBindValue()` appends positional parameters to an internal vector. On the first iteration, positions 0 and 1 were bound. On the second iteration, `addBindValue()` appended to positions 2 and 3, but `exec()` only bound the first two positions (positions 0 and 1, belonging to the *first* session).
   - *Impact*: In any scenario with **multiple interrupted runs**, only the first row was updated on disk; subsequent rows were never updated in SQLite and remained indefinitely in `running` or `approval` state on disk!
   - *Correction*: Replaced `addBindValue` with explicit indexed binding:
     ```cpp
     updateQuery.bindValue(0, QString::fromUtf8(QJsonDocument(value).toJson(QJsonDocument::Compact)));
     updateQuery.bindValue(1, sessionId);
     ```
2. **Missing Transactional Consistency and Unchecked Query Execution**:
   - Previously, `updateQuery.exec()` return value was completely unchecked. If SQLite reported disk full, locked database, or I/O failure, the loop silently inserted `failed` into the in-memory `m_lastRuns` while the disk retained `running` or `approval`, causing disk and memory projections to diverge.
   - Furthermore, updates were committed individually. A crash during reconciliation would leave the database in an inconsistent state.
   - *Correction*: Wrapped the entire reconciliation loop in `db.transaction()` and `db.commit()`, checking `if (!updateQuery.exec())` and executing `db.rollback(); return false;` on failure.
3. **Unsubscription on Server Stop**:
   - `DaemonIpcServer::stopServer()` failed to unsubscribe `m_agentSubscription`. If a harness or component called `stopServer()`, background agent loop events continued to reach `onAgentEvent()` and invoke `rememberRun()`, corrupting restarted projections.
   - *Correction*: Added unsubscription to `stopServer()`:
     ```cpp
     if (m_controller && !m_agentSubscription.isEmpty()) {
         m_controller->agentRuntime()->unsubscribe(m_agentSubscription);
         m_agentSubscription.clear();
     }
     ```

### 4.2 Verified Reconciliation Rules
1. Interrupted `running` and `approval` runs become truthfully `failed` with detail `daemon-restarted` and empty output.
2. Completed runs survive restarts intact with their original run ID and output.
3. Disk storage (`session_projection`) and in-memory cache (`m_lastRuns`) strictly agree.
4. Reconciliation is idempotent across repeated restarts (tested across 3 sessions over 2 consecutive restarts).
5. Late approval requests on interrupted runs are rejected with `invalid-approval`.

---

## 5. COR-17 Through COR-20 Regression Results (Part D)

| Scenario | Suite | Focus | Result |
| --- | --- | --- | --- |
| **COR-17** | `test_agent_reliability_ipc` | Actor disconnect and observer re-attach during active running agent run. Mutation occurs exactly once; exactly one terminal event emitted. | **PASS** |
| **COR-18** | `test_agent_reliability_ipc` | Duplicate terminal, stale generation, and foreign session events injected over wire are ignored by client adapter. Stale approval replay rejected. | **PASS** |
| **COR-19** | `test_agent_reliability_ipc` | Monotonic `event_sequence` ordering verified across all wire frames; active `server_generation` enforced; exactly 1 terminal event delivered. | **PASS** |
| **COR-20** | `test_agent_reliability_ipc` | Multi-client backpressure on real Unix domain transport: slow client with 8 KB receive buffer dropped after exceeding 1 MB write queue; fast client completes run cleanly. | **PASS** |
| **Backlog Saturation** | `sentinel-ipc` (Rust) | Client receiving 257 events before response triggers `Error::Protocol("event backlog exceeded")`, enforcing the 256-event buffer bound. | **PASS** |

---

## 6. Summary of Production and Test Corrections (Part F)

1. **`apps/sentinel-daemon/service/DaemonIpcServer.h`**:
   - Fortified `setPeerUidResolverForTesting` to reject alterations while `m_server.isListening()`.
2. **`apps/sentinel-daemon/service/DaemonIpcServer.cpp`**:
   - Fortified `openSessionProjectionStore`: fixed multi-row positional binding with `bindValue(0, ...)` / `bindValue(1, ...)`, added atomic `db.transaction()` / `db.commit()`, and verified `updateQuery.exec()`.
   - Fortified `stopServer`: unsubscribes `m_agentSubscription` to prevent ghost event leakage into stopped or recovered servers.
   - Fortified `send`: added check for `UnconnectedState` and `ClosingState`.
   - Fortified `querySocketPeerUid`: added `descriptor < 0` guard and Linux `size >= sizeof(peer)` validation.
3. **`tests/core/test_daemon_ipc.cpp`**:
   - Added `multipleInterruptedRunsReconciledAtomicallyOnRestart` (verifying multi-session reconciliation, disk agreement, and repeated restart idempotency).
   - Added `testSeamCannotBeModifiedWhileListening` (verifying test seam immutability while listening).
   - Corrected lifecycle isolation in `interruptedRunningRunReconciledOnRestart`, `interruptedApprovalRunReconciledOnRestart`, and `completedRunSurvivesDaemonRestart`.
4. **`docs/audits/P5_02_DAEMON_IPC_RELIABILITY_RESULT.md`**:
   - Appended Section 13 dated acceptance clarification formally recording platform limitations, unverified hosted CI gates, and pending policy decisions.

---

## 7. Full Verification Results

### 7.1 Focused Suites
- **Core Daemon IPC (`test_daemon_ipc`)**:
  `42 passed, 0 failed, 0 skipped, 0 blacklisted` (3.04s) — includes 2 new hardening tests.
- **Desktop Agent Reliability IPC (`test_agent_reliability_ipc`)**:
  `7 passed, 0 failed, 0 skipped, 0 blacklisted` (5.99s) — COR-17..COR-20 verified on real Unix transport.
- **Agent Reliability Harness (`test_agent_reliability_harness`)**:
  `99 passed, 0 failed, 1 skipped, 0 blacklisted` (24.63s).
  - *Skipped scenario*: `cor11_cancelAfterFirstStreamedDelta(native-tool-assembly)` (existing explicit skip: `IChatProvider` exposes no typed incremental tool-argument assembly contract).
- **Rust Workspace (`cargo test --locked --manifest-path cli/Cargo.toml`)**:
  All 78 unit tests (`sentinel-tui`), 22 unit tests (`sentinel-mascot`), 27 unit tests (`sentinel-ipc`), and 3 doc-test suites passed with 0 failures.
- **Rust Clippy & Fmt**:
  `cargo clippy --locked --manifest-path cli/Cargo.toml --all-targets -- -D warnings` — Clean (0 warnings).
  `cargo fmt --manifest-path cli/Cargo.toml --all --check` — Clean.

### 7.2 Full CTest Suite (`ctest --preset tests --output-on-failure`)
- **Total Tests Run**: 139 tests.
- **Passed**: 138 tests (99.28% pass rate).
- **Failed**: 1 test (`test_legacy_cli_exit_codes`).
  - *Failure analysis*: `test_legacy_cli_exit_codes` is an existing integration suite from P1-05 (`review/p1-05-current-main`). On macOS Darwin arm64, `LegacyCliExitCodesTest::initTestCase()` asserts `settingsFile.startsWith(configRoot)`. Under macOS, Qt `QStandardPaths::writableLocation(AppConfigLocation)` returns `~/Library/Preferences` or `~/Library/Application Support` rather than `XDG_CONFIG_HOME`, causing the isolation preflight check in this legacy CLI test to fail. This is independent of P5-02 IPC daemon functionality and belongs to P1-05 backlog tracking.
- **All IPC, Agent Runtime, Provider, Resource, Security, and Storage tests passed.**

### 7.3 Scoped Static Analysis & Integrity
- **IPC Generator Checks (`tools/ipc/generate.py --check`, `generate_desktop.py --check`)**: Clean (0 contract drift).
- **Personal Path & Privacy Scanner (`tools/analysis/check_personal_paths.py`)**: Clean (0 files flagged).
- **Git Diff Check (`git diff --check`)**: Clean (0 whitespace errors).

---

## 8. Platform-Specific Limitations

1. **macOS Darwin**:
   - Peer credential check uses `::getpeereid()`. Verified natively on the audit host.
2. **Linux**:
   - Peer credential check uses `SO_PEERCRED` via `::getsockopt()`. Validated deterministically on Unix abstractions; real multi-user deployment requires a privileged multi-UID Linux test fixture.
3. **Windows**:
   - Completely absent. The Sentinel daemon uses `QLocalServer`/`QLocalSocket` Unix domain sockets. On Windows, `QLocalServer` maps to named pipes, but Windows pipe security (impersonation, SID comparisons, ACLs) is not implemented. Windows CLI transport in `sentinel-ipc` explicitly returns `Error::Unavailable("Windows named-pipe transport is not implemented yet")`.

---

## 9. Corrected Acceptance Matrix Status

| Matrix Row | Phase | Component | P5-02 Historical Claim | Corrected P5-02.1 Status | Authoritative Reason |
| --- | --- | --- | --- | --- | --- |
| **AC-P1-04** | P1 | Hosted Unix Stress | "Related acceptance" | **Unverified / Blocked** | Requires two independent hosted 100-repeat Unix runs (`macos-14`, `ubuntu-22.04`). Local passes do not clear hosted CI gates. |
| **AC-P5-01** | P5 | Endpoint Ownership / Auth | "Unix peer authorization is fully verified" | **Blocked** | Deterministically verified on Unix; blocked on Windows (no pipe auth) and native multi-user Linux. |
| **AC-P5-02** | P5 | Windows Named Pipe | "Unverified" | **Blocked** | Blocked by implementation and pending decision **PD-D07**. |
| **AC-P5-03** | P5 | Reconnect / Restart / History | "Deterministic Unix backpressure boundary is verified" | **Unverified** | Unix application-level backpressure boundary verified; platform-wide native acceptance remains open. |
| **AC-P5-04** | P5 | Shutdown / Legacy Authority | "Cooperative drain verified" | **Blocked** | Blocked by maintainer policy decisions **PD-D05** and **PD-D10**. |
| **AC-P5-05** | P5 | Contract / Deadlines / Recovery | "SQLite persistence and memory cache reconciliation are verified" | **Unverified** | SQLite restart recovery verified deterministically; systemd/launchd supervisor integration unverified. |

### Policy Decisions Status
- **PD-D07 (Windows trust and isolation)**: **Pending Decision**.
- **PD-D10 (Provider ownership and hard-stop policy)**: **Pending Decision**.
Neither decision has been adopted or altered.

---

## 10. Recommendation for P5-03

1. **Windows Named Pipe Security Provider**:
   - Design and implement a dedicated Windows named-pipe authorization module utilizing `GetNamedPipeHandleStateW` and token SID comparisons to achieve parity with Unix peer credentials, resolving **PD-D07**.
2. **Containerized Linux Multi-UID CI Workflow**:
   - Implement an unprivileged containerized test runner with two pre-provisioned system users to validate actual `SO_PEERCRED` cross-user rejection natively on Linux.
3. **Structured Telemetry & Health Monitoring**:
   - Extend `daemon.status` with live client counts, aggregate buffer metrics, and backpressure drop counters once P4 resource telemetry is adopted.
4. **Integration Readiness**:
   - With the transactional SQLite reconciliation fix, test-seam immutability guard, and corrected acceptance bounds in place, the hardened candidate on branch `review/p5-02-ipc-security` is **recommended for maintainer integration review**.
