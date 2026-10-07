# Sentinel — Phase 3 Desktop IPC migration report

Historical foundation status (2026-10-04): **PARTIAL**. The original findings below are preserved. The 2026-10-05 final-closure appendix records the subsequent migration and current verdict. No commit or push.

## Pre-implementation migration matrix

Audited `ApplicationBootstrapper`, `DesktopShellViewModel`, `ChatViewModel`,
`ApplicationControllerBuilder`, `DaemonIpcServer`, canonical IPC v1.0, and native companion.
Personal-Brain is unavailable on PATH; repository was supplied explicitly by the user.

| Capability | Current Desktop owner/dependency | IPC equivalent | Strategy / risk | Test owner |
|---|---|---|---|---|
| Provider/model list | Shell → controller → ModelService | model.list/current | Project daemon catalog; readiness/capabilities need contract; high | Desktop IPC/model |
| Selected provider/model | Shell + AppSettings → ModelService | current read only | Add explicit validated selection; preserve frozen binding; high | Model + daemon |
| Chat send/stream/cancel | Shell → controller → ChatModeService | chat.send/output.delta/run terminal/run.cancel | Session projection; replace terminal text, invalidate late events; high | Chat + Desktop IPC |
| Retry/regenerate | Shell → controller/history | missing | Extend typed message/history operations before replacing; high | Chat recovery |
| Conversation selection | Shell → conversation store | session.create/list/attach | Attach snapshots plus persisted history contract; high | Conversations + IPC |
| Agent start/events/cancel | Shell → controller → AgentRuntime | agent.start/tool events/run.cancel | Project authoritative events only; high | Agent + IPC |
| Approvals | Shell → controller/PermissionService | approval.requested/respond | One-use IDs; reconnect snapshot; current IPC allow/deny only; high | Permissions + IPC |
| Tool activity/terminal state | Shell → controller/runtime | tool.requested/result/run terminal | Project session-scoped events; high | Agent + IPC |
| Workspace | Shell → WorkspaceService/settings | missing (binding snapshot only) | Typed canonical root mutation/snapshot; freeze active root; high | Workspace + IPC |
| Network mode | Shell → settings/controller/provider policy | missing | Daemon-owned typed configuration; high | Settings + security |
| Settings | Bootstrap/Shell → AppSettings | missing | Separate visual/window local preferences from runtime authority; high | Settings |
| Memory/context | Shell → controller/stores/ContextEngine | missing | Typed read/mutation commands; no Desktop store writer; high | Memory/context |
| Controlled tasks | Shell → ControlledTaskService | missing | Typed task commands/events; preserve AgentRuntime; high | Controlled tasks |
| Plugin/MCP/skills | Shell → controller/settings/SkillProfileService | missing | Daemon-owned configuration and projections; high | Extensions |
| Notifications | Shell → NotificationService | events available, routing absent | Foreground suppression; completion/approval/disconnect; medium | Notifications |
| Voice | Shell → audio session/transcription/TTS/settings | missing | Separate native capture/playback from daemon policy/config; high | Voice |
| Tray/Quick Panel | NativeCompanionAdapter → Shell | native UI stays local | Existing QSystemTrayIcon; replace hardcoded readiness with connection projection; medium | Desktop/native |

## Architecture and migration decision

Before: Desktop builds `ApplicationControllerBuilder.withStandardDefaults()` and owns
runtime/services and persistence independently of the daemon. Rust CLI/TUI use typed IPC.
Target: Desktop presentation → reusable Qt IPC client/session adapters → daemon → existing core.
Retain ApplicationController as daemon coordinator; avoid an unrelated controller rewrite.
The old Desktop `DaemonClient` sends an obsolete unversioned status command to the wrong
endpoint and is not a functioning v1 client. Replace that transport first.

Do not switch production Chat/Agent to an incomplete adapter that would lose settings,
workspace, conversation history, model selection or approval semantics. No hidden fallback
or alternate Desktop runtime is introduced by the transport work.

## 1. Architecture

Before: Qt/QML → DesktopShellViewModel → locally built ApplicationController/core;
Rust CLI/TUI → IPC → daemon/core.
After this change: the existing Desktop daemon-status client uses the real typed v1.0
transport in a reusable `sentinel_desktop_ipc` library linked only to Qt Core/Network.
Desktop Chat/Agent and other runtime flows still use the original local controller.
Target architecture has **not** been reached. No second Chat/Agent backend, hidden
fallback, generic remote method invocation, or replacement runtime was added.
Desktop's eventual role is presentation, state projection and native integration;
daemon's role remains runtime, persistence and security authority.

## 2. Desktop IPC client

`DaemonClient` replaces the obsolete unversioned status-only implementation.
It provides generated command enums/names, canonical response/event field validation,
asynchronous QLocalSocket lifecycle, request UUID correlation, bounded framing and
outgoing buffers, 256 pending-request limit, monotonic request deadlines and typed errors.
`daemonReachable` becomes true only after a successful compatible handshake.
Connection states are Disconnected, Connecting, Connected, Reconnecting, VersionMismatch,
Unavailable and ShuttingDown. These are QObject properties, available to QML.
Raw runtime request submission is a C++ API, not a QML invokable.

Reconnect uses a nonblocking single-shot timer; pending requests fail and are never
replayed. Duplicate/expired response IDs are ignored. Known malformed events disconnect;
unknown events are ignored only for a compatible future minor. VersionMismatch halts
automatic reconnect. Explicit disconnect/connect permits retry after repair.
Errors cover unavailable daemon, protocol mismatch, malformed response, invalid local
request, timeout, cancellation, missing session/run, provider/model unavailable,
permission denial, stale approval, runtime busy and internal daemon errors.
Some detailed runtime rejection distinctions are not yet supplied by v1.0's server.

Protocol stays **1.0**. Canonical `optional_responses.hello.server_generation` is additive.
Daemon assigns a fresh UUID on each successful listen. Qt exposes this separately from
its connection epoch; older daemons without the field remain compatible. Rust's generated
Hello variant uses an optional field. Both clients have compatibility coverage.
The generator now avoids rewriting unchanged outputs.

## 3. Model / provider

Transport supports model.list/current. Production Desktop selectors still use local
ModelService/AppSettings; model/provider migration is **not complete**. The protocol has
no model mutation command or complete readiness/capability projection. No local routing
or fallback policy was added. Existing core frozen-binding behavior is unchanged;
Desktop selector/binding-freeze coverage over IPC remains required.

## 4. Chat migration

Qt client → actual daemon service with deterministic provider validates send, streaming,
terminal text, cancel, suppression of the cancelled provider's late completion, and
post-cancel multi-turn recovery. Session-scoped events remain isolated between clients.
These are service/client integration tests, **not real Desktop UI E2E**.
Production Shell send/retry/regenerate/history still calls ApplicationController.
Persistent full-history projection, conversation switching and restart UI semantics
remain unimplemented. Transport does not claim to implement a Chat presentation state machine.

## 5. Agent migration

Qt client → actual daemon/core fixtures validate start, tool/approval events, pending
approval reattachment, allow/deny, cancellation, terminal response and stale approval
rejection. A deterministic planner/executor supplies fixture final text; this is not
proof of real-model grounded filesystem execution through Desktop.
Production Agent actions remain on the original controller. No Agent state machine,
permission logic, tool execution or grounding logic was copied into the client.

## 6. Sessions

Transport supports create/list/attach. Integration tests create real conversation-backed
sessions and reattach a fresh client to a daemon-held pending approval. Daemon restart
changes server generation; reconnect uses fresh request IDs. A Desktop session projection,
persisted history selection, automatic reattach and cross-generation interrupted-run UX
remain required. Disconnect alone does not cancel a daemon run.

## 7. Desktop lifecycle

Client startup is asynchronous; absent daemon produces Unavailable, and loss of an
established connection produces Reconnecting. Protocol mismatch is distinct. Manual
client disconnect stops retries and cancels pending client requests without daemon shutdown.
Real daemon service restart and reconnect pass in integration tests.
Existing ApplicationBootstrapper daemon discovery/owned-QProcess launch policy remains
unchanged and needs migration: packaged binary discovery, connect-before-launch, detached
persistent ownership, startup races and production shutdown have not been certified.

## 8. Multi-client

Two Qt clients sharing the actual daemon service pass generation equality, event isolation,
and continued connection when the other disconnects. Existing daemon lock/authentication
and one-foreground-run policy are unchanged. Same-user attached clients can respond to a
one-use approval; later responses are rejected. Desktop + actual CLI, Desktop + actual TUI,
and three-client runtime coexistence were **not run**. Rust CLI/TUI regression is passing.

## 9. Quick Panel foundation

Connection/error/generation QObject properties are available for the existing Quick Panel
and shell to consume. No Quick Panel redesign or alternate runtime input path was added.
Provider/model/workspace summaries, active-run projection and pending-approval counts over
IPC remain required. No UI completion claim.

## 10. Tray / native integration

Existing NativeCompanionAdapter/QSystemTrayIcon implementation is retained for macOS,
Windows and Linux. Existing show/activate and companion entry points remain.
Hardcoded readiness and direct-runtime actions still need replacement by daemon state.
No platform certification, new global shortcut registration or final shortcuts were added.

## 11. Notifications

Existing NotificationService boundary remains. Foreground-aware daemon completion,
failure, approval and disconnect notification routing remains unimplemented.
Transport emits validated events and does not fabricate notifications or authorization.

## 12. Security

PermissionService, ToolExecutionGateway, authorized paths/workspaces, sandbox, network
policy and secret storage were not changed. The new transport does not own these services.
The existing server continues to validate untrusted requests and enforce owner-only local
IPC. Oversized/malformed incoming frames fail safely; outgoing invalid payloads fail locally.
No credentials were inspected, added, removed or logged by this migration work. Synthetic
pre-existing UpgradeTest fixture keys are not credentials. Existing Desktop runtime
ownership still prevents a final single-authority migration verdict.

## 13. Legacy direct paths

Removed: obsolete Desktop unversioned `command: status` transport and obsolete endpoint.
Retained/still required: ApplicationControllerBuilder bootstrap, Shell direct Chat/Agent,
settings/workspace/model writes, stores, extensions, controlled tasks and voice.
Safe to remove now: **none of those runtime paths**. No old behavior tests were deleted.
ApplicationController 127-case and DesktopShell 74-case coverage is retained; equivalent
IPC-backed presentation coverage must precede their eventual ownership transfer.

## 14. Real Desktop E2E

Provider/model: no real provider/model run attempted in the native Desktop.
Real Desktop Chat, streaming, approval modal, filesystem Agent, cancellation, Desktop
restart and daemon-restart degraded UX: **not validated**.
Only the actual daemon service with deterministic provider/planner/executor fixtures was
exercised by the new Qt client. No loading/empty/error/focus/responsive user-flow closure
is claimed; no QML flow was changed.

## 15. Performance

Startup-to-connected, status round trip, first visible chunk, event projection and native
reconnect latency: **not measured**. The final QtTest fixture suite takes 4.34 seconds;
that test duration is not a production performance measurement.

## 16. Tests

New Desktop IPC suite: **20 PASS / 0 FAIL / 0 SKIP**, including setup/cleanup.
Coverage: handshake/status, older v1 hello without generation, unavailable, incompatible
major, local invalid request, out-of-order correlation, duplicate-response suppression,
fragmentation, malformed JSON/event/oversized frame, request timeout without mutation
replay, reconnect ID invalidation, compatible future events, real service Chat streaming,
Chat cancellation/recovery, real service Agent allow/deny/cancel with pending-approval
reattach and stale-response rejection, two-client isolation, daemon restart and socket cleanup.
Rust adds optional-generation backward compatibility coverage; total **17 PASS**.

## 17. Regression and static checks

| Check | Result |
|---|---|
| Configure/build tests preset | PASS with CCACHE_DISABLE=1 |
| C++ full suite | 109 PASS / 1 FAIL / 110 executed; 397.91 seconds |
| Failing test | test_upgrade: settingsForwardCompatible timed out at 300 seconds, QtTest aborted |
| ApplicationController independent rerun | 127/0/0 |
| DesktopShell independent rerun | 74/0/0 |
| Final new Desktop IPC suite | 20/0/0 |
| Rust fmt/clippy (all targets/features, warnings denied) | PASS |
| Rust workspace tests | 17 PASS / 0 FAIL |
| Canonical generator --check | PASS |
| Changed C++ clang-format | PASS |
| Scoped clang-tidy: client and daemon server | PASS; warnings suppressed in existing dependency headers |
| QML lint target | Completed with existing warnings; no QML edits |
| git diff --check | PASS |

Socket-binding tests initially failed inside the filesystem/network sandbox; approved
unsandboxed test runs passed. Initial compilation failed because ccache targeted an
unwritable external volume; disabling ccache resolved it without source changes.
Homebrew clang-tidy required the macOS SDK explicitly and a temporary compile database
with AppleClang PCH flags removed; no project compile flags were altered.
The upgrade failure matches the case previously associated with native Keychain approval,
but this run did not capture a native dialog or stack proving its exact wait source.
No timeout was extended, test skipped or secret-store policy weakened to produce PASS.

## 18. Hygiene

All owned test processes finished; final process inspection found no owned test, daemon, plugin-host or model-server orphan.
The new IPC fixtures clean their sockets and private temporary directories.
The single aborted UpgradeTest directory was identified by this run's timestamp and
verified to contain only Sentinel Light theme/schema settings, then removed narrowly.
No Keychain item or user profile was removed. No daemon, plugin host or model server was
launched outside test fixtures by this work. Diagnostic logs remain outside the repository
in `/tmp/sentinel-phase3-*`; generated/build outputs remain ignored.

## 19. Defects found

- Existing Desktop status client used an obsolete endpoint/envelope; fixed.
- IPC lacked server generation identity; added compatibly.
- Qt test temporary paths exceeded macOS Unix socket limits when using the full test
  application name; fixed with short private temporary-directory templates.
- Native encryption regression could not complete unattended; unresolved.
- v1.0 lacks Desktop mutation/history/settings/workspace/extensions/voice contracts;
  recorded gaps, not papered over with generic remote invocation or a hidden fallback.

## 20. Remaining work

1. Add canonical typed contracts for model selection/readiness, history/message recovery,
   workspace and daemon settings, controlled tasks, extensions, context/memory and voice.
2. Implement Desktop session/model/Chat/Agent projections, notifications and Quick Panel
   summary over those contracts, with real user-flow and state coverage.
3. Remove production Desktop runtime composition only after equivalent behavior coverage
   and profile/settings migration are complete; retain core controller in daemon.
4. Implement/certify Desktop daemon discovery/start/reconnect/shutdown ownership policy.
5. Run the required real-provider native Desktop E2E and CLI/TUI coexistence matrix;
   measure performance and inspect resource hygiene afterwards.
6. Resolve native UpgradeTest authorization and obtain a clean full baseline.

## 21. Commit readiness and final verdict

Changes are source, generated contract, tests and documentation only. Working tree started
clean. No commit/push. Final git status/stat/diff-check and process inspection completed; diff-check PASS.
The status lists only the intended 14 source/contract/test/document files; no runtime artifacts. This is reviewable IPC foundation work, **not a commit-ready completed Phase 3**.

**FINAL VERDICT: PARTIAL.** Production Desktop Chat/Agent/model authority remains local;
real Desktop E2E/coexistence and single-authority closure criteria are unmet. The full
regression is also not clean. Do not label service-level client fixtures as Phase 3 PASS.

Evidence: `/tmp/sentinel-phase3-{build,ctest,desktop-ipc,controller,shell,rust,tidy,qmllint}.log`.

---

# 2026-10-05 — Phase 3 final closure

This appendix supersedes the historical migration verdict above while preserving its
PARTIAL findings and original validation. Scope: Desktop Chat/Agent/model/session
single-authority migration and native integration foundation; no commit/push, visual
redesign, brand integration or additional runtime backend.

## 1. Migration matrix

| Production path | Classification | Current authority / limit |
|---|---|---|
| Bootstrap ApplicationControllerBuilder/runtime composition | MIGRATED | Only daemon builds the runtime; Desktop builds IPC and presentation objects |
| Chat send/retry/regenerate/edit/stream/cancel | MIGRATED | DesktopRuntimeClient → typed commands → daemon ChatModeService |
| Agent start/activity/tools/final/cancel | MIGRATED | DesktopRuntimeClient → daemon AgentRuntime and its existing AgentLoop |
| Provider/model/readiness/capabilities/selectors | MIGRATED | Daemon ModelService; active run binding is frozen |
| Session create/list/attach/history/resume | MIGRATED | Daemon conversation store plus daemon-owned IPC run metadata |
| Approval/grant decisions | MIGRATED | Daemon approval IDs and PermissionService/ToolExecutionGateway |
| Runtime settings, credentials, model helpers, maintenance | MIGRATED | Daemon AppSettings/settings service and acquisition helpers |
| Memory/context primitive queries and exposed mutations | MIGRATED | Finite daemon action/read allowlist; no Desktop store composition |
| Workspace settings/profile editing | PRESENTATION-ONLY | UI transforms daemon-projected configuration; daemon freezes and authorizes execution |
| Permission/tool/skill/Agent preview catalogs | PRESENTATION-ONLY | Existing metadata-only descriptions, no executor or runtime grant writer |
| Mode, theme, notifications, native window/tray | PRESENTATION-ONLY | Separate `.desktop` preference store and native presentation |
| Inspector, controlled-task service pointers, advanced voice service paths | LEGACY-COMPATIBILITY | Remote bridge returns unavailable; no alternate local execution path |
| Local Shell/controller constructor and deterministic provider/executor | TEST-ONLY | Existing regression fixtures; production bootstrap never selects them |
| Other generated non-primary diagnostics | LEGACY-COMPATIBILITY | Reserved allowlist readers return unavailable/default, not a live runtime |

**STILL DIRECT runtime authority: none in production Chat, Agent, model, session or
approval execution.** The Desktop core link remains for presentation types/catalogs
and the explicitly retained legacy unit-test constructor. A disconnected daemon does
not cause any runtime to be constructed or any task to execute locally.

## 2. Desktop IPC

Qt QLocalSocket transport has a versioned handshake, UUID correlation, bounded framing,
request deadlines, typed errors and distinct connection states. Canonical generation
and sequence are validated independently of the local connection epoch. Pending
requests fail on disconnect and mutations are never replayed. Foreign session/run,
old generation, duplicate sequence and closed-run events cannot alter the active UI.
Primary Desktop projection fields are paged; actions and settings are generated finite
allowlists with strict argument types, scope and runtime-busy restrictions.

## 3. Sessions

Create/list/attach and canonical live/persisted messages are usable from actual Desktop.
The preferred session is a UI preference, not a second conversation store. Daemon-owned,
owner-only Qt SQL `ipc-sessions.sqlite3` stores latest IPC run identity/kind/binding/state
separately from transcripts and grants. It retains terminal state across restart;
interrupted projections become failed (`daemon-restarted`) and are never resumed.
The ordinary conversation/history stores remain the only transcript retention owners.
A deterministic adapter test reopens this metadata store and restores the same completed
run. Native Desktop restart and daemon restart restore the existing conversation.

## 4. Model/provider authority

Runtime provider IDs/labels, selected provider/model, readiness/capabilities and installed
model metadata originate from daemon ModelService. Unknown provider/model selectors
are rejected explicitly. A Desktop selector change never changes an active binding;
the adapter regression changes the future provider while verifying the active provider
remains Ollama. Static download/catalog descriptions remain UI metadata. Model download
helpers are daemon-owned proxies; no new model was downloaded for certification.

## 5. Chat

Production flow is QML → Shell/Chat presentation → bridge → DesktopRuntimeClient →
DaemonClient → daemon → core ChatModeService. Native visible stream, completion,
second turn, Stop cancellation, Cancelled terminal and post-cancel recovery passed.
Canonical history is preserved while awaiting a new run, and late message responses
are checked against both session and run identity. Terminal text is authoritative;
stream output cannot overwrite an earlier assistant message or duplicate a final.

## 6. Agent

Production Agent uses the same IPC path into daemon AgentRuntime and ToolExecutionGateway.
Actual file reading in the controlled workspace produced accepted grounded Completed
answers. Agent activity/tool/approval events are projected without a Desktop planner or
Agent state machine. Native Stop produced Cancelled, no late Completed, and a following
actual directory-observation run completed. Failure and repeated-tool guards remained
truthful; no natural-language shell shortcut or grounding relaxation was introduced.

## 7. Approval

The modal projects the daemon approval/run/session IDs, tool, risk, resource and reason.
Deny has initial keyboard focus. Allow-once and Deny use typed responses; duplicate
submission is suppressed while pending and the server rejects stale/reused approval IDs.
Reconnect preserves pending approval in the same daemon generation. Cancellation removes
pending approval. There is no QML-created persistent grant or grant replay after restart.

Native approval `0729c413-210f-4178-ba86-61d76126396c`, run
`52b8ecd9-3197-4851-8a69-9d7bd581c499`, session
`a81dcf4d-f63f-4dd8-a9d0-d3410cca9ed3` survived Desktop restart. Allow executed one
web-fetch of the owned local model health endpoint, then one actual read-file, followed
by AgentLoop accepted Completed. Deny approval
`827278bf-5cba-4b9d-ad1a-89edaca66354` produced Failed with
`User denied the required action`, without executing the requested tool.

## 8. Native real Desktop E2E

Model: the already installed NVIDIA Nemotron 3 Nano 4B Q4_K_M GGUF, served by the existing
llama-server binary as `llama-cpp-server/sentinel-nemotron` at owned loopback port 8081.
No synthetic planner/executor supplied the native UI results. Temporary workspace
contained `project(SentinelClosureFixture LANGUAGES CXX)`.

| Case | Real evidence |
|---|---|
| Chat first + second turn | Both visibly returned PHASE3_CLOSURE_OK; second run `2893bf4a-1595-4873-b2af-9ea69c55b717` |
| Chat stream/cancel | Visible counting tokens; run `50f9f957-0588-45b0-b8ff-52c29e4a2b2c` Cancelled, no later Completed |
| Chat recovery | `c72da289-b960-4e2c-88ed-5e732d9055a7` Completed with RECOVERED |
| Grounded Agent | `2d83230c-1e49-4399-b216-e70c54543171` actual read-file and accepted Completed |
| Agent after cancellation | `006f1b67-0310-4b9a-b260-115aa9b3894e` actual list-directory, accepted Completed |
| Approved continuation | `52b8ecd9-3197-4851-8a69-9d7bd581c499` one web-fetch + one read-file + accepted Completed |
| Restart Chat | Request `e6a04766-0cce-4b26-bb3b-8bf0cb13f40f`, run `a30104c8-00d4-4532-9a46-f4344f0bddbf`, generation `0cfecc7c-e3af-45c3-bb04-7b532383be52`, visible RESTART_RECOVERED |

Original generation: `3bd3e133-777c-4741-9453-909a501e174f`; final daemon generation:
`101ee95b-e62f-4cb6-abc6-1a2dd4792415`. Metadata-only Qt logging captures request
UUID/command/generation when verbose logging is enabled, excluding task/secret payloads.
AX text, screenshots, daemon events and CLI results are retained under ignored
`build/certification/phase3-closure-oct5`.

Model outcome limits were preserved, not hidden: standalone health-JSON Agent run
`63971a67-b02f-4846-afff-549b57a78430` executed its approved tool but failed grounding;
composite/write attempts were cancelled when the model kept generating; final
three-client run `ae7e465b-58b4-42c3-a4be-b8de3b2ed888` executed web-fetch once and
read-file once, then Failed on the existing repeated-tool guard. These were not counted
as grounded success and did not invalidate the separately observed accepted runs.

## 9. Connection/reconnect

Already-running daemon, native Desktop restart with daemon alive, pending-approval
restoration, daemon crash (visible Reconnecting), new-generation restart and usable
post-restart Chat passed. A second restart retained the same completed IPC run identity.
Slow startup showed actual `Daemon unavailable` followed by `Daemon connected` when the
real isolated daemon started. Actual Desktop against a controlled major-2 handshake
showed `Daemon protocol is incompatible` and did not execute work.

Cold startup used actual Desktop plus an actual packaged sibling daemon in a disposable
portable profile. Qt client logs show handshake, status and session attach against a
new generation; CLI status remained healthy after Desktop exit. The copied app's AX
window capture timed out in Computer Use, so cold-start connection evidence is the
real Qt/daemon logs rather than a claimed screenshot. Empty session, pending/streaming,
unavailable/error, approval keyboard focus and window zoom states were captured. No
visual redesign or broad responsive-layout completion is claimed.

## 10. Multi-client

Desktop + actual Rust CLI + actual Rust TUI coexisted. CLI created its own conversation
`daec808e-8940-4050-91b0-6527f3ac8f24` and completed CLI_ISOLATION_OK; TUI attached it
and displayed that output while Desktop streamed on its separate session. CLI status
returned active_runs=1 with all three clients connected. TUI received no Desktop chunks
or approval prompt. Exiting the idle TUI did not cancel Desktop; Desktop Stop cancelled
its own run. A later three-client Desktop approval executed its tool once and remained
isolated from the TUI's session. Server and adapter integration also cover request IDs,
wrong-run events, independent subscription/cancellation and duplicate/stale approvals.

## 11. UpgradeTest

Standalone recheck passed 5/0/0 before migration fixes. Final three standalone reruns
passed 5/0/0 in 0.157, 0.142 and 0.138 seconds. No Keychain prompt was observed in these
runs. The previous timeout's root cause is **not inferred** from Phase 2. No timeout
increase, skip or test removal was made. Separately, a deterministic IPC fixture stall
was actually sampled in macOS SecItemCopyMatching; that fixture now injects its existing
in-memory credential store. Production Keychain behavior was not bypassed or altered.

## 12. C++ regression

Final CMake tests configure/build passed. Full CTest: **111/111 PASS**, 78.71 seconds,
zero timeout/failure/new skip. Six existing host-conditional QTest cases remain skipped: four strict macOS child-process shell fixtures and the missing-Docker/missing-npx branches when those tools are installed. These are unchanged, explicitly visible in LastTest.log, and are not included in the 26/0/0 IPC count. ApplicationController: **127/0/0**; DesktopShell:
**74/0/0**; Desktop IPC: **26/0/0**; ChatModeService: **10/0/0**. Both canonical generator
checks are CTest gates. Previous meaningful regression coverage remains intact.

## 13. Rust

`cargo fmt --all --check`, workspace/all-targets/all-features clippy with `-D warnings`,
and workspace tests passed: **17 tests**, zero failure/ignored. Local socket integration
was run with the required host socket access; sandbox PermissionDenied was not treated
as a source defect or a passing test. Old hello responses without generation remain
compatible; generated optional Desktop snapshot fields preserve v1 clients.

## 14. Static

Build and generated-contract checks passed. clang-format dry-run/Werror passed for all
28 changed/new C++ header/source/include files. Configured scoped clang-tidy returned
zero for 13 compiled changed sources using the actual tests compilation database with
Apple PCH removed for LLVM inspection; final daemon source was rechecked after hardening.
qmllint returned zero for all 69 QML files. `git diff --check` passed.

This is **not warning-free**: configured tidy style/readability warnings remain;
existing QML context-property/winId lint warnings remain; native web-fetch emitted the
existing QNetworkAccessManager cross-thread-parent warning while producing its actual
successful result. None was reclassified as a clean warning-free build.

## 15. Security

Descriptor registry, ToolExecutionGateway, PermissionService, AuthorizedPath/path guard,
sandbox, network mode, credential boundary, grounding, frozen workspace and immutable
ModelBinding remain daemon/core boundaries. Desktop owns no runtime store, provider
router, executor, fallback backend or permission-grant writer. Explicit unknown/wrong-type
commands/actions/settings and unavailable provider/model IDs are rejected; finite
numeric settings are validated. IPC is same-user/owner-only, with locked private endpoints
and bounded transport. Run metadata is owner-only and contains no transcript or grant.

## 16. Hygiene

Final audit: no Sentinel daemon, Desktop, plugin-host or llama-server process remains from this work. All observers/TUIs exited; the native supervisors reported CLEANED. Owned current and earlier native workspace/profile roots and their socket/lock/database/credential fixtures were removed, including the interrupted prior-run root. Evidence/logs remain ignored build artifacts. No user process or unrelated socket was selected for cleanup.

## 17. Defects fixed

Removed production local runtime composition and the remaining settings/model-helper
network owners. Added typed session/history/action/settings projections and immutable
run binding snapshots. Fixed cancelled provider workers outliving ChatModeService during
replacement runs: the service now tracks/joins every live worker, with a meaningful
shutdown regression. Fixed late closed-run events poisoning the next run's sequence,
stale generation/snapshot projection, session field leakage, history clearing/assistant
stream overwrite, duplicate approval submission, malformed optional metadata handling,
readable approval resources and an undefined onboarding bool binding. Added durable
terminal run metadata without duplicating transcript retention or persistent grants.

## 18. Remaining work outside closure

Non-primary diagnostics, richer Inspector/controlled-task/advanced voice adapters,
transcript pagination/historical event replay, grant-scope UI beyond v1 Allow/Deny,
Windows Rust transport, Quick Panel visual completion and brand assets are not claimed
as delivered. Unsupported service-pointer paths stay unavailable instead of acquiring
local authority. LLM task reliability and the existing web-fetch thread warning remain
separate core/provider concerns. Linux/Windows native certification was not performed
on this macOS host.

## 19. Commit readiness

No commit/push. Final status/stat/diff checks include generated source contracts and
new adapter/service/test/report files. Build products, screenshots, sockets, model data,
logs, temporary profiles and credential fixtures are not staged or tracked. Changes
are ready for review within this phase's scope; preserved historical findings are
explicitly distinguished from current evidence.

**FINAL VERDICT: FIXED + PASS** — the production Chat/Agent/model/session single-authority migration and the required real native closure criteria passed. Scope limits and unsuccessful LLM attempts above remain explicit. No commit or push.
