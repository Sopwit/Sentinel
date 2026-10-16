# P5-01 — Daemon and typed IPC lifecycle baseline

Date: 2026-10-16 (Europe/Istanbul). Dedicated managed worktree
`audit/p5-01-daemon-ipc-baseline`; branch `audit/p5-01-daemon-ipc-baseline`.
Verified entry and observed HEAD `70e9fa072af9b4139d86f518c6d89f9c849c29b2` (== `origin/main` == `main`,
zero divergence). Working tree clean at entry and exit.

**Audit and documentation only.** This task made no source, test or registration edit, ran no product
build, launched no model, made no cloud request, terminated no process, changed no acceptance/decision
status, and did not commit, push, merge, dispatch a workflow or open a PR. Every status below is a
source/contract classification or a citation of previously recorded evidence, not a new acceptance
execution. No measurement is invented and no universal performance limit is asserted.

## 1. Current daemon architecture

`sentinel-daemon` is a single C++20 `QCoreApplication` composition root; it owns Chat, AgentRuntime,
ModelService, the registry/gateway, permission services and the SQLite stores. There is exactly one
daemon writer per endpoint and no second `AgentRuntime`, IPC protocol or controller. Desktop and the
Rust CLI/TUI are clients of the same generated protocol; the legacy C++ CLI remains a separate
in-process compatibility path outside this contract.

| Concern | Authoritative location | Observed baseline behavior |
| --- | --- | --- |
| Composition root | [`DaemonService::initialize`](../../apps/sentinel-daemon/service/DaemonService.cpp) (lines 47–214) | Claims the endpoint before opening stores, then builds `ApplicationControllerBuilder::withStandardDefaults` (66–67). |
| Startup ordering fence | Same, lines 50–54 and 204–206 | Endpoint lock is acquired first so a losing second daemon cannot mutate the live daemon's persistence; session projection store opens last. |
| Typed validation/routing | [`DaemonIpcServer.cpp`](../../apps/sentinel-daemon/service/DaemonIpcServer.cpp) | One request envelope parser and one command dispatcher; no dynamic method invocation. |
| Canonical schema | [`protocol/ipc-v1.json`](../../protocol/ipc-v1.json) | `major=1`, `minor=1`; generates the daemon C++ contract, the shared Qt client contract and the Rust payload enums (`tools/ipc/generate.py`, `generate_desktop.py`). |
| Client transport (Desktop) | [`DaemonClient.cpp`](../../apps/sentinel-desktop/src/DaemonClient.cpp) | `QLocalSocket` framing, connection epoch, pending-ID map, bounded reconnect. |
| Client projection (Desktop) | [`DesktopRuntimeClient.cpp`](../../apps/sentinel-desktop/src/DesktopRuntimeClient.cpp) | Snapshot/event projection fenced by `server_generation` and monotonic sequence; disconnect clears state. |
| Client transport (Rust) | [`lib.rs`](../../cli/crates/sentinel-ipc/src/lib.rs) | `UnixStream` on Unix; Windows `Client::connect` returns `Unavailable` (593–611). |
| Ownership contract | [`DAEMON.md`](../development/DAEMON.md), [`IPC.md`](../interfaces/IPC.md) | Cooperative drain; external/owned process distinction; no forced thread destruction. |

The daemon exposes one foreground active run to multiple connected clients, mirroring the existing
controller. Session IDs are conversation IDs; a separate owner-only `ipc-sessions.sqlite3` stores the
latest acknowledged run identity/kind/frozen binding/terminal state but never transcripts or grants
([`DaemonIpcServer.cpp`](../../apps/sentinel-daemon/service/DaemonIpcServer.cpp) lines 108–152).

## 2. IPC version and platform matrix

| Dimension | Unix (macOS/Linux) | Windows |
| --- | --- | --- |
| Protocol | 1.1 (canonical `ipc-v1.json`), major-gated | Same definition; unused in production |
| Transport | Qt `QLocalServer`/`QLocalSocket` Unix-domain socket; Rust `UnixStream` | Rust transport unimplemented: `Error::Unavailable("Windows named-pipe transport is not implemented yet")` |
| Peer identity | macOS `getpeereid`, Linux `SO_PEERCRED` same-UID check | No peer-UID check implemented; daemon-side `Q_OS_WIN` refuses an existing filesystem endpoint (404–409) |
| Socket ACL | `QLocalServer::UserAccessOption`; runtime dir owner-only `0700`, owner + non-symlink checks | Not established |
| Default endpoint | `~/.sentinel/run/daemon.sock` | N/A |
| Exclusive claim | `QLockFile` with `setStaleLockTime(0)`, claimed before stores | Lock exists; pipe ownership semantics unverified |

Fixed protocol bounds are contract, not calibration: frame 262144 bytes, per-client pending output
1048576 bytes, 32 connected clients, request string fields 65536 chars, identifiers 128 chars, attach
output 65536 chars, Rust event backlog 256, voice PCM bound 1920000 bytes. These are enforced in
[`DaemonIpcServer.cpp`](../../apps/sentinel-daemon/service/DaemonIpcServer.cpp) (486–536, 656–677,
1703–1704) and documented in [`IPC.md`](../interfaces/IPC.md) §Security and limits.

Platform reach summary: Unix transport is implemented on both macOS and Linux but not natively
accepted; Windows transport is **missing**, so every Windows cell is blocked by implementation, not by
testing.

## 3. Reconnect, session and event reliability matrix

Classification vocabulary: **Implemented/source-confirmed** (code path exists and is readable),
**Deterministically tested** (a registered fixture exercises it), **Verified native platform** (a
native macOS/Linux/Windows acceptance run exists), **Missing/blocked** (no implementation or no safe
evidence path). A passing fixture is not native-platform acceptance. No row below is Verified native
platform, because no native acceptance run was performed in this baseline.

| ID | Scenario | Classification | Evidence / limit |
| --- | --- | --- | --- |
| R-01 | Single daemon writer; endpoint lock ownership | Deterministically tested | `collisionDoesNotUnlinkOwner`, `ownedSocketRemovedOnStop` ([`test_daemon_ipc.cpp`](../../tests/core/test_daemon_ipc.cpp) 486–494, 751–759); lock acquired before stores (DaemonService 50–54). |
| R-02 | Foreign-UID client rejection | Implemented/source-confirmed | `getpeereid`/`SO_PEERCRED` abort path (DaemonIpcServer 517–536). No cross-UID fixture exists; native multi-user acceptance missing. |
| R-03 | Stale-socket recovery after unclean owner exit | Deterministically tested | `staleSocketRecovered` (test_daemon_ipc 731–750); probe + remove only after refused/not-found (DaemonIpcServer 418–432). |
| R-04 | Unsafe endpoint refusal (regular file / foreign owner / non-private dir) | Deterministically tested | `refusesRegularFileAtEndpoint` (test_daemon_ipc 721–730); owner/`0700`/non-symlink guards (DaemonIpcServer 370–417). |
| R-05 | Protocol major-version mismatch stops reconnect | Deterministically tested | daemon `incompatibleMajor` (test_daemon_ipc 448–459), desktop `incompatibleMajor` ([`test_desktop_ipc.cpp`](../../tests/desktop/test_desktop_ipc.cpp) 868–876); `VersionMismatch` halts reconnect (DaemonClient 245–247, 296). |
| R-06 | Handshake required before commands | Deterministically tested | `handshakeRequired` (test_daemon_ipc 460–468). |
| R-07 | Disconnect during active run does **not** cancel | Deterministically tested | `disconnectDoesNotCancelChat` (test_daemon_ipc 636–655), `cor17_disconnectReattachDuringActiveRun` ([`test_agent_reliability_ipc.cpp`](../../tests/desktop/test_agent_reliability_ipc.cpp) 313). |
| R-08 | Reconnect + attach restores snapshot without replaying a mutation | Deterministically tested | `multipleClientsAndReconnect` (test_daemon_ipc 469–485), `reconnectInvalidatesPendingIds` (test_desktop_ipc 966–987), COR-17 reattach. |
| R-09 | Multiple concurrent clients on one foreground run | Deterministically tested | `multipleClientsAndReconnect`, `realDaemonServiceChatAndMultiClient` (test_desktop_ipc 2148); 32-client cap (DaemonIpcServer 512). |
| R-10 | Request/response correlation + duplicate/expired-ID suppression | Deterministically tested | `requestCorrelationAndDuplicateSuppression` (test_desktop_ipc 877–901), `disconnectSettlesPendingRequestsOnceAndRejectsLateReplies` (775–817); `!m_pending.contains(id)` drop (DaemonClient 283–297). |
| R-11 | Timeout closes socket; late reply not consumed by next command | Deterministically tested | `timeoutDoesNotReplayMutation` (test_desktop_ipc 948–965); Rust absolute-deadline `request_until` (lib.rs 263–373). |
| R-12 | Event generation fence after restart/reconnect | Deterministically tested | Generation reset on change (DesktopRuntimeClient 181–202); COR-18 old-generation cases. |
| R-13 | Event sequence dedup + first-terminal immutability | Deterministically tested | `cor18_staleDuplicateForeignEventsIgnored` (test_agent_reliability_ipc 314–315); sequence stamped per event (DaemonIpcServer 1604). |
| R-14 | Foreign session/run/old-generation event filtering | Deterministically tested | COR-18; daemon publishes only to clients subscribed to the session (DaemonIpcServer 1606–1612). |
| R-15 | Interrupted-run recovery on daemon restart | Implemented/source-confirmed (partial test) | `running`/`approval` → `failed` + `daemon-restarted`, output cleared, never replayed (DaemonIpcServer 115–124). Completed-run identity survives restart is deterministic (test_desktop_ipc 1724–1730); the interrupted conversion itself is not asserted by a fixture. |
| R-16 | Slow/oversized client backpressure disconnect without cancelling the run | Implemented/source-confirmed | Write-side pending-bytes abort (DaemonIpcServer 499–503) and read-side `message-too-large` (558, tested at test_daemon_ipc 523–531). The slow-client pending-bytes path itself is untested. |

Run/approval identity routing (supporting R-07/R-08): `run.cancel` requires a matching `m_runId` and a
running/approval state (DaemonIpcServer 1473–1486); `approval.respond` requires the active run plus the
one-use approval ID and otherwise returns `invalid-approval` (1487–1502). Deterministic coverage exists
via `agentApprovalRoundTrip`, `agentCancellationDuringApproval`, `rejectsUnknownSessionAndApprovalReplay`.

## 4. Shutdown, drain and ownership limitations

- **Cooperative drain only.** `requestShutdown()` (DaemonIpcServer 445–473) is idempotent, closes the
  listener, requests Chat/Agent/title cancellation, then polls nonblocking `tryShutdownRuntime()` on a
  25 ms timer until the runtime and title worker are drained before emitting `shutdownDrained` and
  permitting `QCoreApplication::quit()`.
- **No forced termination.** No worker is detached or forcibly destroyed. A non-cooperative in-process
  provider keeps the runtime in `Stopping`; the daemon stays responsive, reports no successful exit and
  retains all ownership ([`DAEMON.md`](../development/DAEMON.md) §Shutdown drain and provider lifetime).
  A bounded hard-stop guarantee requires an externally supervised isolated provider and is **not
  implemented** (PD-D10 Pending).
- **Destructor ownership fence.** `DaemonService::~DaemonService` (32–45) stops the health timer, stops
  Chat, synchronously shuts down AgentRuntime, detaches the controller from the IPC server, then resets
  `LocalModelRuntime`, controller and settings before `stopServer()`. It may wait on non-cooperative
  code and must not run after an incomplete `tryShutdown` result.
- **Owned vs external model processes.** `LocalModelRuntime` terminates only processes it started and
  never terminates an existing external server ([`LocalModelRuntime.h`](../../apps/sentinel-daemon/service/LocalModelRuntime.h) line 13;
  `.cpp` 254–256). This is a lifetime boundary, not permission to terminate externally owned processes.
- **Legacy authority exception.** The legacy C++ CLI still constructs its own in-process
  `ApplicationController`; its disposition is PD-D05 Pending ([`ACCEPTANCE_DECISIONS.md`](../roadmaps/ACCEPTANCE_DECISIONS.md)).
  It is not a competing daemon but it remains a second task authority path.

## 5. Existing test evidence

Registered deterministic targets exist in [`tests/CMakeLists.txt`](../../tests/CMakeLists.txt)
(262–273 daemon, 343–349 desktop IPC, 351–358 agent reliability IPC, 339 generated contract). This
baseline cites those registrations and the previously recorded runs; it did not rebuild or rerun product
tests.

| Target | What it covers | Recorded evidence |
| --- | --- | --- |
| `test_daemon_ipc` | Shutdown ack/drain, multi-client/reconnect, disconnect-does-not-cancel, approval round trip, cancellation during approval, collision/stale/regular-file endpoint, oversized frame, chunked framing, major mismatch, malformed payload, event mapping | 29 substantive slots; cited from source, last full run in P0.1/P2/P3 audits |
| `test_desktop_ipc` | Pending settle on disconnect, late-reply rejection, reconnect ID invalidation, timeout non-replay, correlation/duplicate suppression, generation-less older server, real daemon service multi-client, restart identity | Cited from source; `realDaemonServiceChatAndMultiClient`, `presentationAdapterChatSessionsCancelRecover` |
| `test_agent_reliability_ipc` | COR-17 disconnect/reattach during active run; COR-18 stale/duplicate/foreign/old-generation events | 2 slots, offscreen, 60 s timeout |
| `test_ipc_generated_contract` | Schema/contract drift | Generator `--check` gate |
| Rust `sentinel-ipc` | 26 tests (12 `tests` + 2 `transport` + 1 `generation` + 11 `close_regression`) | Recorded in P1-04 reports; **no current run in this task** |

Historical full-suite figures (127 CTest targets, 130 Rust tests in P0.1; 132/132 CTest in P3-03) are
inherited baseline evidence only, at different candidate SHAs. They are not current-candidate results
and must not be promoted as such.

**P1-04 reconciliation.** [`P1_04_4_HOSTED_IPC_STRESS_INTEGRATION.md`](P1_04_4_HOSTED_IPC_STRESS_INTEGRATION.md)
and `P1_04_3_GITHUB_CI_READINESS.md` described the manual hosted stress job as existing only on branch
`ci/p1-04-ipc-stress`. That is now superseded: current `origin/main` **does** contain the manual
workflow via commit `d4230063` (ancestor of `70e9fa07`). In [`.github/workflows/ci-rust.yml`](../../.github/workflows/ci-rust.yml)
the job `ipc-stress` is gated solely by `if: github.event_name == 'workflow_dispatch'` (no label or
schedule trigger), matrix `ubuntu-latest` + `macos-14`, running 100 sequential
`cargo +1.94.0 test --locked -p sentinel-ipc` with per-iteration logs/timeouts. This task did **not**
dispatch it. No hosted 100-repeat evidence exists for the current candidate; AC-P1-04 stays Ready for
Validation / partial. Historical local macOS (86/14 pre-fix, then 100/100) and Fedora 100/100 remain
local fixture evidence, not hosted acceptance.

## 6. Required missing tests

| Gap | Why it matters | Suggested owner boundary |
| --- | --- | --- |
| Cross-UID client rejection (R-02) | Unix trust boundary is source-confirmed but never exercised | Unix fixture with a second UID or a documented native checklist |
| Slow-client pending-bytes backpressure (R-16) | A stuck reader can silently disconnect without touching the run | Deterministic synthetic socket that stops reading |
| Interrupted-run `daemon-restarted` conversion (R-15) | Recovery currently asserted only for completed runs | Persisted `running`/`approval` row re-opened by a fresh server instance |
| Native Unix multi-client lifecycle | Fixtures are loopback-only, not native acceptance | ENV-LINUX / ENV-MAC native harness per acceptance matrix |
| Windows named-pipe transport/ACL/identity | Entire Windows surface is blocked on implementation | PD-D07 decision before any test |
| Non-cooperative provider hard-stop | Drain cannot bound it in-process | Requires PD-D10 supervised-isolation decision |
| Hosted 100-repeat stress | AC-P1-04 closure requirement | Maintainer-authorized `workflow_dispatch` run; not this task |
| Deadline/event-replay under load | Rust backlog (256) and TUI channel (256) bounded but untested under saturation | Extend existing Rust fixtures; no new protocol |

No missing test above authorizes terminating external processes, adding a second runtime, or relaxing a
fixed protocol/security bound.

## 7. Platform acceptance gaps

| Platform cell | Required environment | Current state |
| --- | --- | --- |
| ENV-HOST (Unix fixtures) | Disposable sockets/profiles, no models | Deterministic fixtures exist; not a native acceptance result |
| ENV-LINUX | Fedora/KDE/Wayland x86_64 | No native daemon/IPC run; unavailable in this task |
| ENV-MAC | Apple Silicon native Qt | No native daemon/IPC run; host presence is not acceptance |
| ENV-WIN / ENV-WIN-TERM | Windows x86_64 native pipe + terminal | Blocked: transport unimplemented, no ACL/identity policy |
| ENV-UNIX-CI | Ubuntu + macOS hosted | Manual 100-repeat job is present in main but has not been run |
| ENV-HEADLESS | Isolated running QCoreApplication daemon | Code exists; isolated service not provisioned here |

A passing loopback fixture is explicitly not native-platform acceptance for any cell above
([`ACCEPTANCE_MATRIX.md`](../roadmaps/ACCEPTANCE_MATRIX.md) §Status, scope and evidence contract).

## 8. Performance instrumentation plan

No budget is measured or asserted here; all thresholds remain Pending Calibration (PD-D04). Existing
instrumentation is limited to an uptime counter (`m_uptime`/`uptime_ms`, DaemonIpcServer 441, 720) and a
Desktop handshake log emitted by `DesktopRuntimeClient` ("Desktop handshake to authoritative
projection/session (ms)", 827–828). The exact instrumentation still required:

| Metric | Instrumentation to add | Notes |
| --- | --- | --- |
| Idle daemon CPU | Out-of-process sampling (delta user+system CPU seconds / elapsed) per BUD-02 | Do not sample in-process; P4 owns the sampler |
| Daemon RSS/PSS | PID-tree-freeze + OS-specific counter per BUD-01 | Linux PSS vs macOS footprint vs Windows private bytes kept separate |
| Startup-to-ready latency | Monotonic t0 at process start plus stamps at listen, stores-open, controller-built, first `hello` | Only the post-listen uptime exists today |
| Reconnection latency | Stamp disconnect → connected → `ready()` in `DaemonClient`/`DesktopRuntimeClient` | No metric exists today |
| Shutdown/drain duration | Stamp `requestShutdown` → listener closed → `tryShutdown` true → `shutdownDrained` → quit | 25 ms poll is not a measurement |
| IPC round-trip latency | Monotonic send/receive stamps keyed by request ID (per command name) | Deadline math exists in both clients; no histogram |
| Event-replay correctness under load | Count emitted vs accepted events and sequence gaps near the 32-client/256-backlog bounds | Correctness metric, not a latency budget |

All seven metrics feed BUD-01/02/10/11/12 and must coordinate with the P4 resource-monitor design; P5
must not build a parallel resource instrument (AC-P4-01). Interactive latency uses monotonic event
stamps, never the 1 Hz sampler.

## 9. Dependencies on P3/P4 and pending decisions

- **P3 (model lifecycle ownership).** [`P3_02_LIFECYCLE_OWNERSHIP_CONTRACT_RESULT.md`](P3_02_LIFECYCLE_OWNERSHIP_CONTRACT_RESULT.md)
  and [`P3_03_LIFECYCLE_READINESS_REMEDIATION_RESULT.md`](P3_03_LIFECYCLE_READINESS_REMEDIATION_RESULT.md)
  established owned-vs-external process boundaries and readiness fencing. AC-P3-02 remains Blocked.
  P5 shutdown inherits the owned/external distinction but gains no authority to terminate external
  processes.
- **P4 (resources/telemetry).** AC-P4-01/02/03 own CPU/RSS/GPU sampling, adaptive policy and
  security-invariance. P5 depends on P4 for BUD-01/02/03/04 and must not duplicate the monitor.
- **PD-D10 (provider ownership/hard-stop, Pending).** Blocks AC-P5-04's non-cooperative branch. No
  hard-stop claim is permitted without the supervised-isolation decision.
- **PD-D05 (legacy C++ CLI, Pending).** Government of the second task authority path that AC-P5-04
  must reconcile.
- **PD-D07 (Windows trust/isolation, Pending).** Blocks AC-P5-02; fail-closed until adopted.
- **PD-D03/PD-D04 (environment + calibrated budgets, Pending).** Block native/perf scoring.
- **PD-D01/PD-D11 (candidate scope; named humans, Pending).** Gate every future acceptance instance.

## 10. Recommended P5-02

Recommend **P5-02 — Deterministic daemon IPC reliability closure (Unix), audit-scoped hardening**:

1. Add the four executable missing fixtures that need no native hardware or decision: cross-UID peer
   rejection harness (or a documented native-only fallback), slow-client pending-bytes disconnect,
   interrupted-run `daemon-restarted` conversion, and generation/sequence/backpressure saturation.
2. Wire the startup-to-ready, reconnection and shutdown/drain monotonic stamps behind the P4-approved
   instrumentation seam (no new sampler, no thresholds).
3. Defer Windows transport until PD-D07, and defer the non-cooperative hard-stop branch until PD-D10.

Scope guard: P5-02 must not introduce a second AgentRuntime, IPC protocol or daemon controller, must
not terminate externally owned processes, and must not change the acceptance matrix or fixed protocol
bounds. Request maintainer authorization before any hosted `workflow_dispatch` stress run for AC-P1-04.

## 11. Relevant acceptance criteria and current status

| Acceptance ID | Requirement (abbrev.) | Current status | P5-01 finding |
| --- | --- | --- | --- |
| AC-P5-01 | Endpoint ownership/security | Blocked | Unix owner checks + lock implemented and deterministically tested; native acceptance and cross-UID test missing |
| AC-P5-02 | Windows named pipe | Blocked | Rust Windows transport unimplemented; PD-D07 Pending |
| AC-P5-03 | Reconnect/restart/history | Unverified | Reconnect + snapshot + no-replay behavior deterministic; no historical replay/page guarantee; restart conversion untested |
| AC-P5-04 | Shutdown/legacy authority | Blocked | Cooperative drain deterministic; non-cooperative branch and legacy CLI unclosed (PD-D05, PD-D10, PD-D04) |
| AC-P5-05 | Contract/deadlines | Unverified | Generated 1.1 contract + per-request deadlines exist; no current-candidate malformed/version/event/timeout run recorded here |
| AC-P1-04 | Unix IPC handshake | Ready for Validation / partial | Manual 100-repeat job is in main but unrun; hosted evidence outstanding (F02) |
| AC-P4-01..03 | Resource telemetry/adaptive | Blocked | P5 instrumentation depends on P4; do not duplicate |

Traceability: F02 (Rust handshake) → AC-P1-04/AC-P5-05; F04 (Windows pipes) → AC-P5-02; F03 (legacy
CLI) → AC-P5-04; F09 (large compositions) → AC-P5-04. No criterion status is changed by this report.

## Validation and scope notes

- **Links:** every relative link in this report resolves within the repository.
- **Formatting:** Markdown tables/fences verified; no trailing whitespace introduced.
- **Generated contract:** `protocol/ipc-v1.json` is unmodified; `tools/ipc/generate.py --check` remains
  the drift gate and was not required to rerun because no generated input changed.
- **Privacy:** no personal machine paths, credentials, tokens or raw private data are embedded; the
  documented `~/.sentinel/run/daemon.sock` form is used.
- **Git:** audit-only; no commit, push, merge or workflow dispatch. `git diff --check` clean; the only
  added artifact is this report.
- **Evidence boundary:** all classifications are source/contract reads or citations of earlier reports
  at recorded candidate SHAs, not current-candidate executions. No native platform is Verified.

**P5 is not complete.** Unix IPC ownership, reconnect and event-fencing behavior is substantially
implemented and deterministically tested, but native platform acceptance, Windows transport,
interrupted-run recovery assertion, non-cooperative shutdown, cross-UID rejection and hosted 100-repeat
stress all remain open.
