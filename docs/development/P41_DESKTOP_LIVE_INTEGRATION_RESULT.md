# SENTINEL P4.1 DESKTOP LIVE INTEGRATION RESULT

Date: 2026-10-15. Repository: sanitized checkout, `<repo>`.
Branch: `feat/p4-desktop-live-integration`. Base/unchanged HEAD: `0f68c8e94bdbc09f2e31f037877e7c095839699d`.

## Delivery status and baseline

Two bounded corrections implemented locally: authoritative GGUF installation projection and Inspector selection/reconnection consistency. No commit, push, PR creation, merge, history rewrite, packaging change, or Windows investigation performed. The initial working tree was clean. GitHub read-only PR metadata confirms #18 (`feat/p3-agent-runtime-reliability`) targets #17's `fix/p2-cross-platform-ci-stabilization`; #17 targets `main`. Neither PR was modified.

Personal-Brain's `brain` executable was unavailable on PATH. The explicitly supplied sanitized working directory was verified instead. Architecture, Agent Runtime, security model, building, testing and local AGENTS instructions were read before editing. The historical audit and feature matrix were used as investigation references, not current proof. Literal SYS-01/SYS-02/SYS-04/SYS-07 identifiers were not found in tracked documentation; the available audit uses A01/A02/A04/A07. No equivalence between those numbering systems is asserted.

## Verified Desktop architecture

Production bootstrap constructs `DaemonClient`, `DesktopRuntimeClient`, `DesktopControllerBridge`, `DesktopShellViewModel`, remote model helpers and `RemoteAgentInspectorService`. QML presents their projections; it does not execute an alternate AgentLoop. The bridge's remote mode and generated Desktop method/projection contracts route actions over typed local IPC. `DaemonIpcServer` validates requests and composes the daemon-owned `ApplicationController` and Core services.

- Chat: `HomeChatSurface` → shell/bridge → runtime client → `chat.send` / session messages / run events → daemon session controller → Core Chat/provider binding → persisted messages and terminal state → Desktop projection.
- Models: ModelsPage → `DesktopModelHelper` → `model.helper_state` / action → `DaemonModelHelpers` → `ModelLibraryService`, ModelService discovery, model operations/storage → paginated JSON rows → cards.
- Inspector: AgentInspectorPage → AgentInspectorViewModel → RemoteAgentInspectorService → `agent.history` → daemon agent run store → history codec → selected run, steps, tools, authorizations, evidence and claims.
- Dashboard: DashboardPage hosts the shared HomeChatSurface. It is not a separate metrics/task-outcome dashboard. Connection and provider presentation also lives in navigation/status components.
- Settings: daemon settings/actions remain separate from local presentation preferences. Memory, chat history, conversations, agent-run records and permission grants retain their separate owners. Qt SQL remains the SQLite boundary.

## Chat integration assessment

Implemented before P4.1 and preserved: conversation creation/attachment/listing; daemon-backed send and streamed output; cancellation; history loading; run/session identity; generation and sequence rejection; terminal run guards; rejected-submission persistence across polling; no local inference fallback.

`DesktopRuntimeClient::onEvent` requires matching session/run, rejects older generation/sequence and ignores events after terminal state. `session.messages` additionally checks the request's captured run identity. `ready()` gates projected settings/session/history readiness rather than merely a connected socket. Disconnect clears messages/projections and disables send. Reconnect refreshes authoritative state; a new daemon generation clears the run cursor. Failed submission is retained separately because it created no authoritative run.

Deterministic provider and real local IPC tests cover successful streaming, execution failure, rejected sends, cancellation/recovery, session switching, delayed initial history, reattachment, duplicate/late responses and approval outcomes. They are not real model inference evidence.

Verified remaining gap: Core `localChatSendAvailable()` checks idle state, archive state and `selectedModel().isValid()`. It does not prove provider health or capability readiness. Its summary can say “Ready to send” before execution resolves a binding and rejects an unavailable model/provider. This was not changed in this bounded package; rejection remains truthful and visible. A daemon connection or valid selection must not be interpreted as inference readiness.

## Dashboard state assessment

Current Dashboard reuses Chat state and actions. Its dynamic Chat controls are daemon-backed, with explicit loading/disconnection gates in the runtime client. No separate active-task/recent-outcome metrics were invented or added. Navigation connection status is distinct from provider/model readiness. A full daemon/provider/model/activity status composition remains partially integrated; this package does not certify the existing “Ready” labels as universal model readiness.

## Inspector state assessment and correction

Existing durable history integration exposes session/run IDs, lifecycle, provider/model, steps/tools, approval authorizations, cancellation/failure/completion, evidence/claims and historical runs. Tool rows are decoded stored records; no speculative success entries were added. Detail and recent history are polled, not guaranteed instantaneous event-stream views.

Confirmed defects:

1. Selecting a run absent from the current recent-run cache replaced the view model's requested selection with the empty detail's run ID. That could lose the selection before asynchronous detail arrived.
2. Changing selection while an earlier read was pending did not promptly request the newest detail after that read settled; it waited for polling. Older detail was decoded without checking the current requested selection.
3. Connection changes cleared remote caches but retained the pagination flag, and the connected branch did not immediately publish cache invalidation.

Corrections: preserve the requested ID while showing an empty detail until authoritative data arrives; capture the run requested by each pending read; publish detail only for the current selection; immediately request the newest selection after the older response or failure; clear history/detail/pagination and publish invalidation on connection changes. Reconnect reloads the selected run. Empty returned detail clears cached detail. Existing request correlation and connection epoch protections remain authoritative for late wire responses.

## Model Library state assessment and correction

Core already separates catalog state, installation, availability, selection, capabilities, runtime reachability, compatibility fit and model operations. A registered GGUF is Installed only when Core's file check succeeds. Ollama discovery has current/stale/unverified catalog distinctions; a selected model is not evidence that a downloaded GGUF exists or can execute.

Confirmed defect: `DaemonModelHelpers::rebuildModelRows` replaced Core installation state with `!entry.localFile.isEmpty()` for GGUF cards and included retained paths in its installed count. Removing a registered file left its path present, so Desktop falsely marked it installed and counted it. This contradicted Core's Unknown installation state.

Correction: card `installed` and installed count now require `ModelLibraryInstalledState::Installed`. File paths remain metadata, never installation evidence. The existing `gguf` QML branch consumes that daemon boolean without adding filesystem checks or model-status heuristics. Installed does not imply runtime readiness. No IPC schema change was needed.

Historical wrong-size Ollama family badges were not reintroduced as a new finding: current card selection checks exact model identity, allowing only the explicit `:latest` alias. The older broad-family claim is not established by this audit.

Remaining model gaps:

- Cards expose an installation boolean rather than a full catalog/discovered/selected/provider/runtime/ready state vocabulary. Unknown and NotInstalled collapse in this particular card field.
- The GGUF helper caches generated rows for up to ten seconds; unchanged polling does not immediately recompute every backend model change.
- Runtime/capability fit information exists in Core but is not comprehensively displayed by cards.
- Non-Ollama selected-model summaries still contain Ollama-oriented wording in `ApplicationController::selectedLocalModelSummary()`; those labels need a provider-neutral follow-up.
- Downloading/failed operation state exists separately from installation. Full download/install/remove lifecycle was not exercised against remote sources.

## Changed files

- `apps/sentinel-daemon/service/DaemonModelHelpers.cpp`: installation boolean/count use Core state.
- `apps/sentinel-desktop/include/sentinel/desktop/RemoteAgentInspectorService.h`: pending run identity.
- `apps/sentinel-desktop/src/RemoteAgentInspectorService.cpp`: selected-detail correlation, immediate follow-up, connection cache invalidation.
- `apps/sentinel-desktop/src/viewmodels/AgentInspectorViewModel.cpp`: preserve requested selection through asynchronous loading.
- `tests/desktop/test_desktop_ipc.cpp`: deterministic selection/failure/reconnect, missing-file installation and provider-execution failure regressions.
- This report and `p41-evidence/`: scoped validation evidence.

No QML source, Rust source, generated contract, Core runtime logic or permissions policy changed.

## New deterministic regression tests

- `inspectorSelectionDuringPendingRead`: response and failed-read rows. Select two uncached runs during a pending read; preserve newest ID; keep detail empty until confirmed; request newest run after settlement; display confirmed answer; clear cache/pagination/detail on disconnect; reload the same selection on reconnect.
- `modelRowsUseAuthoritativeInstallation`: existing registered GGUF with unverified runtime and deleted registered GGUF rows. Assert card installation and aggregate count agree with Core inventory rather than retained path. Temporary GGUF header is a registration fixture, not a valid inference model.
- `providerExecutionFailureNeverProjectsSuccess`: real daemon server/transport and Desktop adapter with deterministic failing provider; assert failed lifecycle/message, no active generation, then successful recovery on the next turn.

## Verification matrix and evidence boundaries

| Scenario | Evidence | Boundary |
| --- | --- | --- |
| Daemon unavailable at startup | `unavailable`, `presentationAdapterNoFallback`, readiness tests | Deterministic Desktop/socket tests |
| Daemon becomes ready | `connectedDoesNotMeanReadyUntilSettingsAndSessionArrive`, delayed history tests | Fixture protocol barriers |
| Provider unavailable | rejected-send regression; production empty-model send rejection | Fixture provider, real production rejection |
| Catalog-only versus installed | model catalog fixtures and new authoritative installation test | Fixture metadata; no downloads |
| Installed but runtime unverified | new registered-file test | File/header fixture; no inference |
| Chat completes | `realDaemonServiceChatAndMultiClient`, adapter Chat/session tests | Actual local server/socket, deterministic provider |
| Chat execution fails | new provider failure/recovery regression | Actual local server/socket, deterministic provider |
| Chat cancellation | `realChatCancelAndRecover`, adapter cancellation tests | Actual local server/socket, deterministic provider |
| Late/stale IPC | request correlation, reconnect invalidation, terminal reattach tests | Deterministic wire peer |
| Inspector transition/history | durable history test plus new selection/failure/reconnect rows | Actual store/server and wire fixtures |
| Dashboard update | shared Chat tests, `test_chat_surface`, UI phase/status tests | Offscreen QML fixtures; not standalone metrics acceptance |
| Restart/reconnect | production smoke and new Inspector reconnect test | Real executable restart plus deterministic Desktop peer |

Production daemon smoke used a copied built executable in an owner-only temporary directory, `--portable`, a disposable profile identity and a dedicated socket. Verified running status, Desktop projection, model-helper/agent-history queries, session create/attach, absent-model send rejection, daemon generation change on restart, restored session and reattachment. The selected production profile had `llama-cpp-server` with no model selected. Read-only probes found no model service on loopback ports 11434, 8080 or 1234. No downloads, new credentials, personal sessions or user model files were used. Summary: [production daemon evidence](p41-evidence/production-daemon-smoke.json).

Real inference: UNVERIFIED. Native GUI acceptance: INCOMPLETE. A native launch with isolated preference/socket arguments was attempted, but app automation resolved an existing personal Sentinel window; no actions were performed on that window and it is excluded from acceptance evidence. Offscreen QML flow/focus/compact-layout checks pass only within their existing fixture scopes. Native Fedora KDE, Windows and complete macOS user-flow acceptance remain required.

## Local validation results

| Check | Result |
| --- | --- |
| `cmake --preset tests` | PASS |
| `CCACHE_DISABLE=1 cmake --build --preset tests -j6` | PASS |
| `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | **127/127 PASS**, final completed build, 127.97 seconds |
| Desktop IPC QtTest inside CTest | 66 passed including setup/cleanup, zero failed/skipped; new response/failure/reconnect and provider-failure rows included |
| Rust `cargo +1.94.0 test --locked` | **130/130 PASS**; initial sandbox socket failures resolved on host rerun |
| Rust formatting | PASS, `cargo +1.94.0 fmt --all --check` |
| Rust Clippy | PASS, `cargo +1.94.0 clippy --locked --all-targets -- -D warnings` |
| QML lint | PASS, all 72 QML files, exit 0 |
| IPC generators | PASS, `tools/ipc/generate.py --check` and `generate_desktop.py --check` |
| Scoped clang-tidy | Completed, zero errors; 107 warnings: 106 braces/readability and one existing runtime-setup move suggestion |
| `git diff --check` | PASS |
| Hosted CI | Not used |

Full CTest includes model/Chat/IPC, agent lifecycle/Inspector, Desktop shell, QML UI surfaces, secure mutation, plugin sandbox/host and daemon lifecycle regressions. The registered CTest total remains 127 because new rows were added inside the existing Desktop IPC executable. CTest success does not mean every platform-dependent QtTest case ran.

Seven existing QtTest subcases skipped on this host:

- `CredentialStoreTest::missingLinuxHelperRefusesPersistence`: Linux-only helper contract.
- `AgentRuntimeTest::asyncRunCommandStreamsAndContinues`.
- `AgentRuntimeTest::asyncRunCommandCancellationAndTimeout`.
- `AgentRuntimeTest::asyncDockerUsesProcessExecutorAndPreservesRestrictions`.
- `AgentRuntimeTest::shutdownStopsActiveCommand`: these four shell fixtures require child creation prohibited by the strict macOS sandbox plan; native plugin-host tests remain separate evidence.
- `ObservationPolicyTest::optInLiveClassification`: production-provider opt-in not enabled.
- `RealToolExecutorToolsTest::browserToolsReportMissingNodeGracefully`: npx is installed, so its missing-node branch cannot run.

Whole-repository clang-tidy, Linux execution, ASan/coverage, remote/cloud-provider inference, full model downloads/removals and full native GUI acceptance were not performed. Native Windows stabilization remains explicitly deferred. The static result is scoped and warning-bearing, not a repository-wide clean certification. [Machine-readable validation summary](p41-evidence/validation-summary.json) records totals and skipped-case reasons.

## Security and state ownership

No tool permission, approval policy, sandbox restriction, process/network/filesystem grant, credential storage rule or memory ownership rule was changed. Tests use dedicated temporary sockets and stores; production smoke uses disposable portable state. Host-level test access was used to permit local Unix socket binding and isolated Qt test storage, not to change product permissions. Initial sandbox-bound Rust socket failures were rerun successfully with host test access. The ccache external-volume restriction was resolved by `CCACHE_DISABLE=1`, without weakening product security.

Typed IPC generators remain unchanged and checked. Inspector still uses the existing bounded history codec and stored diagnostics; no new raw provider/storage exceptions or secret fields were introduced. Existing credential-nonreading projection, secure mutation, plugin sandbox/host, approval and cancellation tests remain in full CTest. This is scoped preservation evidence, not a new whole-product privacy certification. P3's permanently unresponsive in-process provider limitation remains unchanged.

## P4.2 readiness

Ready for review of these local corrections; not full Desktop acceptance or release readiness. Next coherent package: a provider-neutral readiness projection shared by Chat, navigation/Dashboard and Model Library. Derive catalog, installation, discovery, selection, runtime/provider availability and capability compatibility from ModelService/ModelLibrary; keep operation progress separate. Make “Ready to send” match binding readiness, correct provider-specific wording, and add provider-loss/recovery tests without silent provider switching. Then validate native Fedora KDE and macOS flows for loading/empty/error, keyboard focus, compact layouts and reconnect using an existing safe local model. Controlled Tasks, richer Dashboard metrics, Voice overhaul and Windows stabilization remain outside this package.
