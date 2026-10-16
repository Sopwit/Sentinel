# P5-02 Deterministic Daemon IPC Reliability Closure

Audit date: 2026-10-17.
Phase: P5 — Daemon & IPC Reliability
Related acceptance: AC-P5-01, AC-P5-03, AC-P5-05, AC-P1-04.

## 1. Exact Tested Source Identity

- **Repository**: `https://github.com/sentinel-projects/sentinel`
- **Tracked origin/main HEAD**: `70e9fa072af9b4139d86f518c6d89f9c849c29b2`
- **Branch**: `test/p5-02-daemon-ipc-reliability`
- **Worktree path**: `<codex-worktree-root>/p5-02-daemon-ipc-reliability/Sentinel-sanitized`
- **Working tree commit baseline**: `9dbbbcdbb034d880b562ed63f275cc2fd597ebc5`
- **Platform**: macOS Darwin 27.0.1 (arm64, Apple LLVM 21.0.0, Qt 6.12.0)
- **Presets**: CMake `tests` preset; Cargo locked manifests.

## 2. Existing Daemon Invariants Preserved

The Sentinel daemon IPC layer operates under the following authoritative contracts:

1. **Single-Writer Endpoint Ownership**:
   The daemon socket endpoint (`~/.sentinel/run/daemon.sock`) is exclusively owned by a single running daemon process. Competing processes fail closed on startup.
2. **Unix Same-UID Peer Authorization**:
   Connections on the Unix domain socket are checked via OS-level peer credentials (`::getpeereid` on macOS; `getsockopt(..., SOL_SOCKET, SO_PEERCRED, ...)` on Linux). Only clients whose effective UID matches the daemon's UID (`::getuid()`) are accepted. Non-matching or unidentifiable peers are aborted immediately without reading or writing frames.
3. **Typed IPC Protocol 1.1**:
   JSON-delimited streaming framing with strict schema validation. Version negotiation requires matching major version (`1`) and backward-compatible minor version.
4. **Maximum Frame Size**:
   `protocol::max_frame_bytes = 262144` (256 KB). Frames exceeding this limit trigger a `message-too-large` error envelope and immediate disconnection.
5. **Maximum Pending Buffer Capacity (Slow-Client Backpressure Boundary)**:
   `protocol::max_pending_bytes = 1048576` (1 MB). If an attached client stops reading and pending unwritten bytes exceed 1 MB, the daemon disconnects the slow socket to prevent memory exhaustion.
6. **Maximum Concurrent Clients**:
   `32` clients. Excess connections are immediately closed.
7. **Event Fencing and Monotonicity**:
   All events emitted by the daemon carry a strictly monotonic `event_sequence` and the unique `server_generation` UUID of the daemon instance. Reconnects and re-attaches use snapshots and never cause event replay or duplicate terminal events.
8. **Independent Session Lifetime (No Automatic Run Cancellation)**:
   A client disconnecting or being dropped due to backpressure never aborts or cancels an ongoing agent run.
9. **Cooperative Shutdown**:
   `daemon.shutdown` requests cooperative draining. The server acknowledges the request and emits `shutdownDrained` once the agent runtime has completed or stopped cleanly. No forced process termination or unapproved hard-stops are performed.

## 3. New Deterministic Test Matrix

| Part | Test Case | Target Binary | Description | Status |
| --- | --- | --- | --- | --- |
| **A** | `peerAuthorizationNativeSameUid` | `test_daemon_ipc` | Native same-UID client connection succeeds and authenticates | **PASS** |
| **A** | `peerAuthorizationDirectDescriptorValidation` | `test_daemon_ipc` | Direct socket descriptor query retrieves peer UID matching `::getuid()` | **PASS** |
| **A** | `peerAuthorizationCrossUidRejectionSimulated` | `test_daemon_ipc` | Foreign peer UID (`uid + 999`) is rejected and aborted without response | **PASS** |
| **A** | `peerAuthorizationMissingCredentialsRejectionSimulated` | `test_daemon_ipc` | Unresolvable peer credentials (`nullopt`) are rejected and aborted | **PASS** |
| **A** | `peerAuthorizationRootUidRejectedWhenNonRootDaemon` | `test_daemon_ipc` | Peer UID 0 is rejected when daemon runs as unprivileged user | **PASS** |
| **A** | `peerAuthorizationNoFallbackToUnauthenticated` | `test_daemon_ipc` | Rejected peer cannot read responses, event streams, or access sessions | **PASS** |
| **B** | `slowClientPendingBytesBackpressure` | `test_daemon_ipc` | Slow client socket exceeding 1 MB pending bytes is aborted; fast client completes run | **PASS** |
| **C** | `interruptedRunningRunReconciledOnRestart` | `test_daemon_ipc` | Active running run is reconciled to `failed` (`daemon-restarted`) on daemon restart | **PASS** |
| **C** | `interruptedApprovalRunReconciledOnRestart` | `test_daemon_ipc` | Pending approval run is reconciled to `failed`; stale approval response rejected | **PASS** |
| **C** | `completedRunSurvivesDaemonRestart` | `test_daemon_ipc` | Completed run retains `completed` state and run ID across daemon restart | **PASS** |
| **D** | `cor19_eventSequenceMonotonicAndGenerationFencing` | `test_agent_reliability_ipc` | Monotonic `event_sequence`, generation checks, exactly 1 terminal, no replay of side-effects | **PASS** |
| **D** | `cor20_multiClientBackpressureAndBufferSaturation` | `test_agent_reliability_ipc` | Multi-client backpressure on real transport: slow client dropped, fast client completes cleanly | **PASS** |
| **D** | `event_backlog_saturation_triggers_protocol_error` | `sentinel-ipc` (Rust) | Inbound event queue exceeding 256 events before reply triggers ProtocolError | **PASS** |

## 4. Cross-UID Verification Boundaries

- **Production Implementation**:
  `DaemonIpcServer::querySocketPeerUid(qintptr descriptor)` queries native OS peer credentials via `::getpeereid()` on macOS and `::getsockopt(..., SOL_SOCKET, SO_PEERCRED, ...)` on Linux.
- **Simulated Peer Identity Hook**:
  To test rejection paths deterministically without requiring `sudo`, multi-user provisioning, or operating system privilege modification, `DaemonIpcServer::setPeerUidResolverForTesting` was introduced. It defaults to the native `querySocketPeerUid` implementation.
- **Distinction**:
  - *Real OS Coverage*: `peerAuthorizationNativeSameUid` and `peerAuthorizationDirectDescriptorValidation` directly invoke the native OS kernel credential query on the active socket descriptor.
  - *Simulated Coverage*: `peerAuthorizationCrossUidRejectionSimulated`, `peerAuthorizationMissingCredentialsRejectionSimulated`, and `peerAuthorizationRootUidRejectedWhenNonRootDaemon` inject simulated foreign peer UIDs to deterministically verify that rejection occurs before any IPC handshake or session frame can be received.
  - Rejection is complete: the socket is aborted, no reply frame is sent, and `read()` on the client fd returns `0` (EOF) or error with 0 bytes available.

## 5. Slow-Client Backpressure Results

- **Mechanism**:
  In `DaemonIpcServer::send()`, before writing serialized JSON to a client socket:
  ```cpp
  if (socket->bytesToWrite() + bytes.size() > protocol::max_pending_bytes) {
      qWarning("IPC slow client disconnected");
      socket->abort();
      return;
  }
  ```
- **Verification**:
  - A slow client connects, completes handshake, and attaches to a session.
  - The client stops reading (`SO_RCVBUF` minimized to 8192 bytes, nonblocking mode enabled, no `read()` calls).
  - The agent run produces 7 tool invocations with 160 KB arguments each (~1.12 MB total), exceeding the 1,048,576 byte threshold.
  - The daemon aborts the slow client's socket. Subsequent client `read()` returns `<= 0`.
  - The fast client attached to the same session receives all events without dropped frames or stream corruption.
  - The agent run continues uninterrupted and finishes with an authoritative `run.completed` event.
  - A fresh client attaches to the session after completion and receives the authoritative snapshot with `state: completed`.
  - Total tool invocations remained exactly 1 batch; no duplicate execution receipts occurred.

## 6. Daemon Restart & Interrupted Run Results

- **Mechanism**:
  On daemon startup, `DaemonIpcServer::openSessionProjectionStore()` inspects SQLite `session_projection` records:
  ```cpp
  if (value.value("state") == "running" || value.value("state") == "approval") {
      value.insert("state", "failed");
      value.insert("output", QString{});
      value.insert("detail", "daemon-restarted");
      updateQuery.addBindValue(QString::fromUtf8(QJsonDocument(value).toJson(QJsonDocument::Compact)));
      updateQuery.addBindValue(sessionId);
      updateQuery.exec();
  }
  ```
- **Verification**:
  - A running run interrupted mid-execution by daemon restart is reconciled to `state: failed`, `detail: daemon-restarted`. No false `completed` state is ever reported.
  - A pending approval run interrupted by daemon restart is reconciled to `state: failed`. An approval response sent after restart is rejected with `invalid-approval`.
  - Completed runs survive daemon restart intact with their original output and run ID.
  - Both SQLite persistent storage on disk and in-memory projection cache (`m_lastRuns`) are synchronized.

## 7. Event Fencing and Saturation Results

- **Event Sequence Monotonicity**:
  Verified in both `test_daemon_ipc` and `test_agent_reliability_ipc` (`cor19`): all emitted event frames have strictly increasing `event_sequence` numbers (`seq > lastSeq`).
- **Generation Fencing**:
  All event frames match the active `server_generation` UUID. Injected frames with mismatched or stale generations are discarded by `DesktopRuntimeClient`.
- **Authoritative Terminal Delivery**:
  Exactly one terminal event (`run.completed`, `run.failed`, or `run.cancelled`) is delivered per run across all attached clients. Duplicate or late terminal events injected on the wire do not alter the client state.
- **No Side-Effect Replay**:
  Re-attaching to a completed session returns the terminal snapshot without re-executing tools or altering workspace files (`executor->calls.load() == 1`, file hash unchanged).
- **Rust IPC Backlog Boundary**:
  `sentinel-ipc` client buffers up to 256 events while waiting for command responses. A stream of 257 events triggers `Error::Protocol("event backlog exceeded")`, preventing unbounded client memory growth.

## 8. Actual Test Counts

- **Core Daemon IPC Suite (`tests/test_daemon_ipc`)**:
  `40 passed, 0 failed, 0 skipped, 0 blacklisted` (3.02s).
- **Desktop Agent Reliability IPC Suite (`tests/test_agent_reliability_ipc`)**:
  `7 passed, 0 failed, 0 skipped, 0 blacklisted` (6.17s).
- **Agent Reliability Harness (`tests/test_agent_reliability_harness`)**:
  `4 passed, 0 failed, 0 skipped, 0 blacklisted` (COR-19 and COR-20 verified).
- **Full CTest Suite (`ctest --preset tests`)**:
  `136 passed out of 136 tests (100% pass rate)` (212.94s).
- **Rust Workspace Tests (`cargo test --locked --manifest-path cli/Cargo.toml`)**:
  All 78 unit tests (`sentinel-tui`), 22 unit tests (`sentinel-mascot`), 27 unit tests (`sentinel-ipc`), and doc tests passed with 0 failures.
- **Rust Clippy (`cargo clippy --all-targets -- -D warnings`)**:
  Clean (0 warnings).
- **Rust Format (`cargo fmt --all --check`)**:
  Clean.
- **Generated Contracts (`tools/ipc/generate.py --check`, `generate_desktop.py --check`)**:
  Clean.
- **Personal Path & Privacy Scanner (`tools/analysis/check_personal_paths.py`)**:
  0 files flagged.
- **Git Diff Check (`git diff --check`)**:
  Clean.

## 9. Production Defects Discovered and Fixed

1. **Unpersisted Interrupted-Run SQLite State**:
   `DaemonIpcServer::openSessionProjectionStore` previously reconciled interrupted runs in its in-memory map `m_lastRuns`, but did not issue an `UPDATE` to the SQLite `session_projection` table on disk. If the daemon restarted a second time without starting a new run, the old `running` or `approval` state on disk would re-emerge.
   *Fix*: Added an UPDATE query to write the reconciled `failed` state back to disk during initialization.
2. **Missing `rememberRun()` Trigger on `approval.requested`**:
   `publishEvent` only invoked `rememberRun()` on `run.started`, `run.completed`, `run.cancelled`, and `run.failed`. When an agent entered the approval state, `m_lastRuns` still held the initial state.
   *Fix*: Added `approval.requested` to the `rememberRun()` trigger set so that session snapshots and re-attaches immediately reflect `state: approval`.
3. **Race Condition in `cor18_staleDuplicateForeignEventsIgnored`**:
   `QTRY_VERIFY(adapter.ready())` evaluated to true before the `respondToAgentApproval` command had fully drained its subsequent `session.attach` response. Releasing the gate immediately caused the resulting `run.completed` event to arrive before the attach response, causing the attach response to be captured in Relay's `heldSnapshots` and leaving the client in `approval` state.
   *Fix*: Changed wait condition to `QTRY_VERIFY(adapter.ready() && adapter.value("conversationState") == "running")`.
4. **AgentLoop Tool Repetition Cycle Guard**:
   In `AgentLoop`, repeating the exact same tool call with identical arguments triggers cycle detection failure (`"Agent repeated the same tool call without a changed argument or intervening observation"`). In backpressure testing, varying the tool argument across iterations prevents the cycle detector from aborting the run prematurely.

## 10. Missing Windows / Native Evidence

- **Windows Named Pipe Transport**:
  The daemon currently uses `QLocalServer`/`QLocalSocket` Unix domain sockets on macOS/Linux. On Windows, `QLocalServer` uses named pipes, but Windows peer authentication (`GetNamedPipeHandleState`, token impersonation, SID comparisons) is not implemented. Windows peer rejection tests are skipped via `Q_OS_UNIX` / `QSKIP`.
- **Privileged Native Multi-UID Verification**:
  Real multi-user cross-UID rejection requires operating system accounts with different UIDs. This cannot and should not be run in unprivileged local environments or standard CI runners. The simulated peer UID resolver provides deterministic unit coverage, but real multi-user Linux deployment remains an integration verification requirement.

## 11. Remaining Acceptance Blockers

- **AC-P5-01 (Single-Writer Daemon & Peer Authorization)**:
  Unix peer authorization is fully verified deterministically. Windows named pipe peer authorization remains unverified.
- **AC-P5-03 (Slow-Client Backpressure)**:
  Deterministic Unix backpressure boundary is verified. Native Windows pipe buffer saturation behavior is unverified.
- **AC-P5-05 (Daemon Interruption and Lifecycle Reconciliation)**:
  SQLite persistence and memory cache reconciliation are verified. Linux systemd service lifecycle integration requires target platform test runner.

## 12. Recommended P5-03 Implementation

1. **Windows IPC Security Provider**:
   Implement a Windows-specific peer authorization module using `GetNamedPipeHandleStateW` and token SID checks to provide parity with the Unix `SO_PEERCRED` / `getpeereid` implementation.
2. **Containerized Linux Multi-UID CI Fixture**:
   Create a dedicated GitHub Actions workflow using a Docker container with two pre-created users (`sentinel-daemon` and `sentinel-client`) to test real un-simulated `SO_PEERCRED` cross-UID rejection.
3. **Structured Telemetry & Health Endpoint**:
   Once P4 baseline resource instrumentation is finalized, expose daemon health metrics (active client count, pending bytes per client, dropped client counter) via `daemon.status`.
