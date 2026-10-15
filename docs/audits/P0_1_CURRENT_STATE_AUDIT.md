# P0.1 — Current Architecture & Implementation Audit

Audit date: 2026-10-16 (Europe/Istanbul). Local source baseline: **`b152a960c11fe6e13b086666752ea5954005797b`**, branch **`feat/p4-desktop-live-integration`**. Repository requested: `sentinel-projects/sentinel`. This report audits the supplied sanitized checkout; it does not certify remote main, another branch, or a released binary. Initial `git status --short` was empty. No fetch, installation, model download, runtime configuration change, commit, push, issue, or publication was performed.

## 1. Executive Summary

Sentinel has a substantial C++20/Qt 6 implementation, a QML Desktop, a headless daemon, a daemon-backed Rust CLI/TUI, and a retained in-process C++ CLI. Production Desktop bootstrap uses IPC; the daemon composes the existing shared core. The authoritative AgentRuntime/AgentLoop, model binding, registry and execution gateway exist. This is an implementation baseline, **not acceptance of reliable autonomous task completion or public release readiness**.

The independently counted built-in inventory is **38 descriptors and 38 handler mappings**, of which `local-plan-summary` is disabled/internal. Thirteen are marked ProductionReady by source metadata and 24 Experimental; those labels are developer declarations, not this audit's live certification. Dynamic MCP/plugin tools make the configured total variable. Clipboard tools are unavailable in the production headless daemon; Linux app launch is explicitly unsupported. Merely counting definitions exaggerates usable capability.

Current verification: CMake configure/build passed; **127/127 CTest targets passed** offscreen; Rust full rerun **130/130 passed** after one earlier intermittent IPC fixture failure. Rust fmt, Clippy and generated IPC/Desktop contracts passed. Live model and native acceptance remain unverified.

Most important findings:

- Production Desktop shares daemon authority, but the C++ CLI still constructs its own core controller. Retained static/metadata services and large composition classes create competing vocabulary and maintenance debt, rather than evidence of a necessary second runtime.
- Desktop now auto-launches a detached daemon. Architecture/daemon documentation still contains earlier migration and no-auto-spawn claims. The roadmap also describes daemon/interfaces and CLI growth as future work despite existing Rust surfaces.
- Agent final acceptance has evidence/claim gates, iteration bounds, repetition detection and cancellation. Real provider task reliability remains unverified here; existing reports explicitly retain acceptance gaps.
- Resource capability facts and model fit estimates exist; a live CPU/RAM/GPU pressure-driven scheduler was not found in the inspected paths.
- Linux/Fedora/KDE is the declared primary target, yet the native Quick Panel global shortcut implementation only handles macOS and Windows. Rust Windows IPC explicitly returns unavailable.
- WebUiService and AcpService contain state-only stubs. ModelLifecycleService is a static catalog, not process loading/unloading. These must not be advertised as working integrations.
- Public distribution remains blocked by documented dependency, provenance/license, privacy, native acceptance and governance gates. The historical register is evidence of unresolved project decisions, not a newly run vulnerability scan or live GitHub settings check.

### Classification and evidence rules

**Implemented** = substantive source path exists, with test evidence where stated; it does not imply every platform/provider works. **Partial** = meaningful subset exists with an identified gap. **Experimental** = conditional or explicitly experimental implementation. **Planned** = documented direction or required target with no verified complete implementation. **Broken** = a concrete source/test failure within a stated boundary. **Unverified** = insufficient current execution evidence. Findings below distinguish source inspection, current commands, historical reports and inference. All paths are repository-relative; symbols identify the inspected implementation.

## 2. Repository and Module Inventory

| Area | Source/build evidence | Responsibility and status |
| --- | --- | --- |
| Root build | `CMakeLists.txt`, `CMakePresets.json` | C++20 required; Qt >=6.5 Core/Gui/Quick/Qml/Sql/Test/Network/Widgets/LinguistTools/Multimedia/Concurrent; optional Pdf. Implemented. Presets require CMake 3.27, while direct root minimum is 3.24. |
| Shared core | `core/CMakeLists.txt`, `core/include/sentinel/core`, `core/src` | Static `sentinel_core`; app, agent, chat, runtime, security, model, memory, plugins/MCP, voice and numerous auxiliary services. Implemented with mixed maturity. |
| Desktop | `apps/sentinel-desktop/CMakeLists.txt`, `bootstrap/ApplicationBootstrapper.cpp`, `ui/qml/Main.qml` | Qt/QML shell, IPC clients, view models, native companion, audio capture. Implemented; native acceptance incomplete. |
| Daemon | `apps/sentinel-daemon/main.cpp`, `service/DaemonService.cpp`, `DaemonIpcServer.cpp`, `LocalModelRuntime.cpp` | Headless composition authority, local IPC, shutdown drain, local model process helper. Implemented/Partial recovery. |
| Legacy CLI | `apps/sentinel-cli/main.cpp`, `commands/*.cpp` | In-process chat, status, config, Ollama list/pull. Partial. |
| Plugin host/SDK | `apps/sentinel-plugin-host/main.cpp`, `plugins/sdk`, `plugins/samples` | Separate native host protocol and samples. Experimental. |
| Rust workspace | `cli/Cargo.toml`, `cli/crates/{sentinel-cli,sentinel-ipc,sentinel-tui,sentinel-mascot}` | Edition 2024, declared Rust 1.94, version 1.0.0-rc.8; Clap CLI, Unix IPC, Ratatui/Crossterm TUI and Core Warden assets. Implemented on Unix; Windows transport missing. |
| IPC contract | `protocol/ipc-v1.json`, `tools/ipc/generate.py`, `generate_desktop.py` | Generated C++/Rust payload and finite Desktop operation contracts; protocol 1.1 independent of app version. Implemented. |
| Validation | `tests/CMakeLists.txt`, `tests/core`, `tests/desktop`, `tests/gui`, `tests/integration`, Rust tests | QtTest/CTest plus Rust unit/mock-daemon tests; fixtures are not model acceptance. |
| Distribution | `cmake/SentinelCPack.cmake`, `cmake/SentinelSigning.cmake`, `packaging`, `.github/workflows` | Multi-platform packaging/signing workflows; public release blocked. |
| Documentation/evidence | `docs/development`, `docs/security`, `docs/reviews`, `docs/ROADMAP.md` | Current guidance mixed with phase/historical records. Reports need date/commit qualification. |

Personal-Brain: `command -v brain` returned no executable. No brief or bounded Brain context could be retrieved. The explicitly supplied working directory, root instructions and Git identity were used; no alternative workspace was guessed. Root `AGENTS.md`, `README.md`, architecture, Agent Runtime, building, testing and security docs were read before report creation. No additional AGENTS.md was found by the repository file search.

## 3. Current Architecture

The actual production flow is:

```text
QML -> DesktopShellViewModel / DesktopControllerBridge
    -> DesktopRuntimeClient / DaemonClient -> typed local IPC
Rust CLI/TUI -> sentinel-ipc ---------------------------> DaemonIpcServer
    -> ApplicationControllerBuilder / ApplicationController
    -> ChatModeService or AgentRuntime -> AgentLoop
    -> ModelService -> IModelRouter -> frozen ModelBinding -> IChatProvider
    -> IToolRegistry / ToolDescriptor -> ToolExecutionGateway -> handler
    -> IFileSystemService / ProcessExecutor / extension boundary
    -> separate Qt SQL stores and settings/credential owners
```

Source: `ApplicationBootstrapper::setupQmlEngine` in `apps/sentinel-desktop/bootstrap/ApplicationBootstrapper.cpp`; `DaemonService` construction/start in `apps/sentinel-daemon/service/DaemonService.cpp`; `ApplicationControllerBuilder::withStandardDefaults/build`; `ApplicationController` constructor; `AgentRuntime` constructor/start; `ModelService` binding/provider resolution in `core/src/model/ModelService.cpp`.

**Implemented:** core does not require the QML shell for execution. Production bootstrap constructs remote adapters and local presentation preferences, not an ApplicationController fallback. `ApplicationController` wraps the supplied planner/runtime seams in AgentRuntime, injects ModelService's router, and owns run/grant stores. `ControlledTaskService` projects core events rather than creating a separate executor.

**Architecture debt:** `core/src/app/ApplicationController.cpp` is 10,566 lines; `DaemonIpcServer.cpp` 1,867 and `DesktopRuntimeClient.cpp` 1,053 at this baseline. These are inspection counts, not complexity scores. The controller spans provider settings, model acquisition, memory, chat, tasks, audio and diagnostics. Generated action allowlists help constrain IPC but do not remove composition coupling.

**Retained overlaps:** `StaticProviderCatalog`, `StaticModelRouter`, `ModelRegistry`, `ModelLifecycleService`, `StaticTaskPlanner`, Ollama runtime/inference adapters, `NullAgentRuntime` and `NullToolExecutor` coexist with live ModelService/AgentRuntime. Null/test and metadata roles are legitimate, but UI wording and architecture docs must identify them. `StaticProviderCatalog::defaultEntries` even creates two `ollama-local` entries; the default StaticModelRouter filters unavailable placeholders, so this is metadata duplication, not proof of two running Ollama providers. `StaticModelRouter::resolveSelection` can construct a routed descriptor for a configured selection absent from its static lists; live ModelService checks therefore matter. Do not treat this static route as health certification.

**Unwired/stub examples:** repository searches found no production app/agent use of WebUiService, AcpService or ModelLifecycleService. `WebUiService::start` only stores config/sets a Boolean; no listener is opened. `AcpService::connect/sendMessage/receiveMessage` toggles state, ignores messages and returns an empty message. ModelLifecycleService hardcodes model/pricing records. Their existence in `core/CMakeLists.txt` is not functional integration. `IdeIntegrationService` does environment/lock detection and invokes an extension installation command; extension compatibility/end-to-end integration is unverified.

## 4. Desktop/Daemon/IPC Assessment

| Capability | Classification | Evidence and limits |
| --- | --- | --- |
| Main Desktop | Implemented | `ui/qml/Main.qml`, pages and `DesktopShellViewModel`; remote bridge/runtime client present loading/disconnected/ready/session/run state. Dashboard embeds `HomeChatSurface`, not a separate resource/task metrics dashboard. |
| Chat/history/Inspector | Implemented/Partial | `DesktopRuntimeClient::onEvent`, `RemoteAgentInspectorService`, `viewmodels/AgentInspectorViewModel.cpp`; generation/run/sequence filtering, remote persisted history, delayed detail selection and reconnect handling. Native real model acceptance unverified. |
| Settings | Implemented | `DesktopRuntimeClient` settings projection and `DesktopSettingsStore`, daemon `DaemonDesktopSettings.cpp`, generated finite setting/service operations; visual preferences remain local and secrets are excluded from projection. Submitted mutations stay pending until reply. |
| Quick Panel | Implemented/Partial | `QuickPanelController.cpp`, `ui/qml/components/dialogs/TrayCompanionWindow.qml`, NativeCompanionAdapter; shares DesktopRuntimeClient; input review precedes execution. Global shortcut gap below. |
| Tray/background | Implemented/Partial | `NativeCompanionAdapter::{registerShortcut,unregisterShortcut}` and QSystemTrayIcon setup; tray availability and platform activation conditional. Linux shortcut branch reports unavailable. |
| Daemon startup | Implemented | `DaemonService`, `DaemonIpcServer::start`; socket claim before stores, shared builder, persisted configuration. `ApplicationBootstrapper::ensureBackgroundDaemon` discovers binary and calls QProcess::startDetached once; safe mode/`--no-daemon-start` suppress it. Rust clients do not auto-spawn. |
| Shutdown | Implemented with limit | IPC server closes listener, rejects new work, cancels tasks/title work and checks `AgentRuntime::tryShutdown(0)` on a 25 ms drain timer. Non-cooperative in-process providers can keep Stopping indefinitely; there is no safe forced thread kill. |
| Crash/reconnect | Partial | `DaemonIpcServer` persists run projection, invalidates interrupted work on restart, offers current snapshot/attachment. No automatic replay/resumption or historical event stream; bounded transcript responses can fail when oversized. |
| IPC trust | Implemented, platform-limited | QLocalServer owner-only private directory/socket, lock and stale-socket checks; Linux SO_PEERCRED/macOS getpeereid. Same UID clients may shut down/cancel; no remote-user isolation claim. Windows Rust transport absent. |

`protocol/ipc-v1.json` is the single source for payloads; Desktop methods/projections also use generated allowlists. `DaemonIpcServer::{handleRequest,publishEvent,session}` enforces correlated identifiers, bounded frames/output/client counts and session subscriptions. `DaemonClient` tracks connection epochs; mutations are not blindly replayed. Rust's request deadline and event identity guards are in `cli/crates/sentinel-ipc/src/lib.rs` and CLI/TUI event handlers. Source constants/docs describe 262,144-byte frames, 32 clients and 1,048,576-byte queued server output per client; slow clients can be disconnected without cancelling the run.

Documentation discrepancies: `docs/development/DAEMON.md` still says Stage B is future and Stage A Desktop is in-process, and says clients implement no detached auto-spawn. `docs/development/ARCHITECTURE.md` correctly describes daemon-backed Desktop but also says neither surface starts the runtime, contradicted by the new Desktop launch helper if read as process startup. Distinguish **no in-process fallback** from **background daemon process launch**. `docs/interfaces/IPC.md` is closer to the current implementation. P4.1 historical evidence (`docs/development/P41_DESKTOP_LIVE_INTEGRATION_RESULT.md`) explicitly leaves native GUI and inference incomplete.

UI acceptance in this audit is bounded to source and automated fixtures. No personal GUI session was manipulated. Native tray activation, keyboard focus, loading/empty/error and responsive flows on physical Fedora/KDE, Windows and macOS require further acceptance; automated QML/TUI results must not be substituted for that.

## 5. Rust TUI and C++ CLI Assessment

**Rust CLI — Implemented on Unix:** `Commands`, `ModelCommand`, `WorkspaceCommand`, `ProviderCommand`, `Task`, `run` in `cli/crates/sentinel-cli/src/main.rs` define status, models, model current/select, sessions, workspace list/create/select/root, changes, provider list/refresh/select/endpoint, chat, agent/run, attach, tui, doctor, completions and shutdown. Text, JSON and stream-JSON output exist. Explicit task args precede stdin; Ctrl+C requests cancellation during task execution. Session/run identity filtering prevents queued older terminals from completing the new command. Errors map distinct usage/unavailable/incompatible/cancelled/failure exits. Commands/tests use a mock daemon, not a Rust copy of AgentRuntime.

**TUI — Implemented/Partial:** `cli/crates/sentinel-tui/src/{lib,commands,editor,picker,transcript_cache,termination}.rs` implement composer editing, scrolling/search, fuzzy pickers, model/provider/workspace selection, explicit Chat/Agent, sessions, reconnect, inspection, authorized file references and Applied diffs. Enter sends; Shift+Enter, Alt+Enter or Ctrl+O provide newline variants depending terminal support. Ctrl+P/Tab opens commands; Ctrl+L models, Ctrl+W workspaces, Ctrl+G mode, Ctrl+R/F search, F1 help. Ctrl+C cancels active work and preserves an idle draft; Ctrl+D exits only idle/empty. Approval y/n sends one-use allow/deny; Esc leaves approval pending. Busy/disconnected availability is explicit in `Command::disabled`.

**Explicit missing TUI operations:** command catalog marks Plan, Compact, Undo, Redo, Skills, Init, Grill and ExternalEditor unsupported. Read-only planning requires daemon policy; revert needs an authoritative restore contract. Existing core compaction/worktree/skill classes do not make these terminal commands implemented. Applied changes are a bounded in-memory run baseline, not durable attribution or undo.

**Core Warden — Implemented/Experimental presentation:** `sentinel-mascot/src/{assets,capability,playback,state,terminal_input}.rs` validates assets, probes Kitty/Sixel/cell dimensions and supports fallback; `sentinel-tui/src/mascot*.rs` integrates it without becoming inference state authority. `apps/sentinel-desktop/src/CoreWardenItem.cpp` uses bounded image loading, canonical fallback and reduced motion. Terminal protocol/physical rendering acceptance and artwork rights remain open release gates; an asset hash proves identity, not license clearance.

**Windows — Broken/missing capability:** the Windows `Client::connect` implementation in `sentinel-ipc/src/lib.rs` returns `Unavailable("Windows named-pipe transport is not implemented yet")`. This is deliberate truthful rejection, not a working Windows TUI backend. Rust CI only runs Ubuntu/macOS.

**Legacy C++ CLI — Partial:** `apps/sentinel-cli/main.cpp` only dispatches chat, model list/pull, status and config get/set. Chat/model/status compose local ApplicationController or local helpers; they do not use daemon IPC. No TUI, task/Agent command, approval interface or equivalent reconnect/cancellation contract is present in this dispatcher. `executeChatCommand` times out after 30 seconds but returns 0; `executeModelCommand` pull prints failure yet also returns 0 after the event loop. These are **Broken automation exit semantics**, verified from source, not intentionally induced model/network failures. Config exposes three keys (ollamaEndpoint, routingMode, appLanguage). Retirement must wait for explicitly defined compatibility needs; do not extend it into another execution authority.

## 6. Agent Runtime Assessment

**Implemented:** `core/src/agent/AgentRuntime.cpp` owns sessions/runs, registry/gateway wiring, permission grants, subagents, event subscriptions and shutdown; `LlmAgentRuntime.cpp` supplies LLM step decisions; `AgentLoop.cpp` executes iterative planning, tool observation, approval pauses/resumption and final gating. `ControlledTaskService` projects that authority. `AgentRuntimeService::{agents,runtimeSummary,previewPlan}` supplies descriptive catalog/readiness/preview metadata, not another execution loop. `AgentTaskRuntime.cpp` is a retained deterministic metadata task state machine with `CompletedMetadata`, not evidence of autonomous execution; its naming should be distinguished from AgentRuntime in UI/docs. A successful tool result alone cannot complete the Agent run.

`AgentLoopPhase` declares Idle, Running, Cancelling, AwaitingApproval, Completed, Cancelled, Failed and Stuck; `AgentTerminalReason` distinguishes approval denial, provider/tool/security/capability failures, iteration limit and inability to complete (`core/include/sentinel/core/agent/AgentLoopState.h`). `AgentEvent.h` projects typed run/model/tool/approval/terminal events.

`AgentLoop` synchronous and async paths both enforce cancellation, max iterations, recorded actions and doom detection. Final handling evaluates observation requirements/evidence, grounding and claims (`ClaimGroundingResolver.cpp`, `ObservationPolicy.cpp`); rejected finals can request bounded verification recovery instead of fabricated success. `ContextEngine::build` and `ToolOutputTruncator` constrain observations. `AgentExecutionScheduler` caps parallel tools at 1–4, checks dependency failures and authorization-resource conflicts, and shares a concurrency budget. This is tool scheduling, not pressure-aware resource allocation.

**Partial/risks:** sync/async loop branches repeat orchestration logic and require parity tests. `DoomLoopDetector::isStuck/recordAction` implements repetition history, while `detect(const QJsonObject&)` is a stub returning a default indicator; do not infer the broader detector API works from AgentLoop's use of isStuck. A rejected final/Indeterminate classification can still prevent otherwise useful tasks from completing; existing model acceptance reports record that boundary. Cancellation is cooperative; synchronous network tool requests cannot necessarily abort immediately. Shutdown ownership fences can wait indefinitely on hostile/non-returning in-process code.

Tests inspected: `test_agent_loop.cpp` cases for accepted finals, classified observation requirements, rejected finals, cancellation during classification, repeated read/mutation behavior, iteration limit, approval/denial and doom loops; `test_agent_runtime.cpp`, `test_llm_agent_runtime.cpp`, `test_controlled_agent_tasks.cpp`, `test_agent_task_runtime.cpp`, gateway/process tests. Fixture success proves deterministic contracts only. No live local/cloud model task was run in this audit.

`docs/concepts/AGENT_RUNTIME.md` labels the area Implemented, while README explicitly says acceptance incomplete. These are compatible only when implementation and reliability acceptance are separately stated.

## 7. Complete Tool Inventory

### Counting, registration and visibility

Recounted directly from `BuiltInToolProvider::descriptors()` and `kMethods` in `core/src/runtime/BuiltInToolProvider.cpp`: **38 unique built-ins, all mapped**. `registerTools` rejects count/method mismatch and atomically replaces the builtin provider in `InMemoryToolRegistry`. `AgentRuntime` registers these for RealToolExecutor; compatibility/null seams exist for fixtures. Schemas forbid additional properties, and `ToolArgumentValidator` validates before dispatch. The inventory below is complete for static built-ins at this SHA, not a snapshot of a user's configured daemon.

All enabled built-ins are **eligible**, not guaranteed model-visible: `ToolExecutionGateway::toolCompleteness/toolMetadata`, platform support, handler `unavailableReason`, registry enablement and AgentRuntime exposure constrain them. `local-plan-summary` is disabled and never exposed. Source labels 13 ProductionReady, 24 Experimental and one Internal. Audit classifications distinguish implemented handler logic from experimental integration.

Table shorthand: `R` = `core/src/runtime/RealToolExecutor.cpp`; all `execute…` symbols below are members of RealToolExecutor. `T` = `tests/core/test_real_tool_executor_tools.cpp`; “reference” means a test mentions the tool, not exhaustive behavior coverage. Authorization domains/access are the descriptor's explicit requirements; approval/sandbox is additionally gateway-controlled. All rows have registration evidence in BuiltInToolProvider. No native side effects were triggered to certify a row.

| Tool | Handler / actual behavior and boundary | Risk; authorization | Audit status; test evidence |
| --- | --- | --- | --- |
| git-status | executeGitInspection via executeAsync; fixed Git arguments, no optional locks/fsmonitor; Git + process sandbox required | Medium; filesystem read + process execute | Experimental; T gitInspectionUsesFixedSandboxedArguments |
| git-diff | same Git handler; staged switch; external diff/textconv disabled, bounded output | Medium; filesystem read + process execute | Experimental; T; test_agent_loop |
| stat-path | executeStatPath; authorized type/size, typed missing-path failure | Low; filesystem read | Implemented; T statPathUsesAuthorizedFilesystem |
| local-plan-summary | executeLocalPlanSummary; metadata only, disabled/internal, never model-visible | Low; agent invoke | Partial/internal; test_null_agent_runtime, test_application_controller |
| list-directory | executeListDirectory; root entries only, 500 cap, hidden opt-in and completeness metadata | Low; filesystem read | Implemented; T readDirectoryListsEntries |
| read-file | executeReadFile; authorized UTF-8 text, 64 KiB read cap, numbered paging, binary/directory rejection | Low; filesystem read | Implemented; T readFileReturnsNumberedLines/readFilePageEvidenceMatchesReturnedContent |
| write-file | executeWriteFile; IFileSystemService secure full-file write and mutation receipt | High; filesystem write | Implemented; test_agent_loop, test_daemon_ipc references; broader FS tests |
| edit-file | executeEditFile; fuzzy replacement/replaceAll/create; secure filesystem mutation | High; filesystem read + write | Implemented; T editFileAppliesReplacement/editFileCreatesNewFileWithEmptyOldString |
| grep | executeGrep; bounded regex file-content search; scope/hidden/traversal metadata | Low; filesystem read | Implemented; T grepFindsMatchesUnderDirectory |
| glob | executeGlob; bounded recursive paths/depth, no symlink target traversal; truncation metadata | Low; filesystem read | Implemented; T globListsMatchingFiles |
| delete-file | executeDeleteFile; authorized file deletion, refuses directories | High; filesystem delete | Implemented; T deleteFileRemovesFileAndRefusesDirectories |
| move-file | executeMoveFile; authorized source/destination, refuses existing destination | High; filesystem delete + write | Implemented; T moveFileRenamesWithinWorkspace/moveFileRefusesExistingDestination |
| apply-patch | executeApplyPatch -> filesystem patch service; contextual multi-file mutation; safe cancellation boundary | High; filesystem write; per-target gateway resolution | Implemented; T applyPatchUpdatesExistingFile/add-delete/context-mismatch cases |
| list-code-definitions | executeListCodeDefinitions; bounded source symbol extraction, not a complete semantic compiler index | Low; filesystem read | Implemented; T listCodeDefinitionsExtractsSymbols |
| run-command | executeAsync sandboxed ProcessExecutor; synchronous executeRunCommand explicitly blocks | High; process execute (argument digest) | Experimental; T frozen workspace/Docker fixtures; test_tool_execution_gateway |
| app-launch | executeAsync platform opener; Linux descriptor Unsupported; no proof app opened | High; application execute | Experimental/Partial; T appLaunchRedirectsDomainNamesToOpenUrl |
| app-quit | executeAsync platform process quit/termination; Linux partial matching, Windows taskkill semantics | High; application control | Experimental; no direct literal tool test found |
| open-url | executeAsync sandboxed opener, HTTP(S) checks; acknowledgement not navigation proof | Medium; browser invoke / host | Experimental; T openUrlRejectsNonHttpScheme |
| system-notify | executeAsync osascript/PowerShell/notify-send; delivery unobservable | Medium; system control | Experimental; no direct literal tool test found |
| clipboard-read | executeClipboardRead; QGuiApplication required, daemon headless unavailable | Medium; clipboard read | Partial; T clipboardToolsFailGracefullyWithoutGuiSession |
| clipboard-write | executeClipboardWrite; GUI session required, daemon unavailable | Medium; clipboard write | Partial; T same headless rejection case |
| system-info | executeSystemInfo; platform/system facts, not utilization telemetry | Low; system read | Implemented; T systemInfoReportsPlatform |
| process-list | executeAsync platform listing; bounded text, lacks structured completeness | Medium; system read; process sandbox path | Experimental; T processListReportsProcessLines |
| current-time | executeCurrentTime; UTC and epoch | Low; system read | Implemented; T currentTimeReportsUtcAndEpoch |
| set-alarm | executeSetAlarm -> AlarmStore; parsed time and persistence; delivery receipt absent | Medium; application control | Experimental; T setAlarmAndListAlarmsRoundTrip/setAlarmRejectsUnparseableTime |
| list-alarms | executeListAlarms; AlarmStore entries; no structured completeness receipt | Low; application read | Experimental; T alarm round-trip |
| cancel-alarm | executeCancelAlarm; remove scheduled record; limited receipt | Low; application control | Experimental; T cancelAlarmRemovesScheduledAlarm |
| todo-write | executeTodoWrite; session-isolated <=100 items, <=256 sessions; volatile, mutex protected | Low; conversation write | Implemented/Experimental maturity; T todoToolsAreSessionIsolatedAndBounded |
| todo-read | executeTodoRead; session checklist with persistent:false | Low; conversation read | Implemented/Experimental maturity; T same checklist case |
| memory-search | executeMemorySearch; bounded memory snapshot/store search, absence not authoritative | Low; memory read | Experimental; T memorySearchFindsSnapshotEntries/without snapshot |
| history-search | executeHistorySearch; bounded history snapshot/store search | Low; conversation read | Experimental; T historySearchFindsSnapshotEntries/without snapshot |
| spawn-agent | executeSpawnAgent; injected SubagentService runner, explicit purpose and bounded tools/workspace | Medium; agent invoke | Experimental; T injected runner/no runner/purpose/task; test_agent_runtime |
| browser-screenshot | executeAsync external Node/Playwright browser operation, private artifact directory; dependencies not bundled proof | Medium; browser invoke / host | Experimental; T missing Node/URL tests only |
| browser-pdf | same async browser path with PDF output; private artifact and dependency limits | Medium; browser invoke / host | Experimental; T missing Node/URL tests only |
| web-fetch | executeWebFetch -> WebFetchTool; network policy, HTTP result, format conversion and truncation; synchronous request | High; network read / host | Experimental; no direct literal wrapper test found; backend/path coverage incomplete |
| web-search | executeWebSearch -> WebSearchTool; configured service or DuckDuckGo fallback, policy-gated; no search completeness guarantee | High; network invoke | Experimental; test_web_search_tool backend; test_llm_agent_runtime reference |
| voice-transcribe | executeAsync configured STT runtime; authorized audio file + cancellation; no model bundled | High; audio read + filesystem read | Experimental; test_unified_audio_lifecycle and test_voice backend tests; no literal wrapper test found |
| voice-speak | executeAsync configured TTS; returns generated audio artifact, not proof of playback | High; audio invoke | Experimental; test_voice/test_unified_audio_lifecycle backend tests; planner reference |

The 13 ProductionReady labels are stat-path, list-directory, read-file, write-file, edit-file, delete-file, move-file, grep, glob, list-code-definitions, current-time, apply-patch and system-info. Risk/domain labels are not sufficient approval by themselves. Process tools use the async authorized path; legacy synchronous helper methods that return Blocked or simplistic text are not the registered process execution path. Avoid treating those helpers as evidence of a live gateway bypass without proving reachability.

Legacy/coarse permission domains are also assigned by descriptor category: filesystem Low-risk tools use `workspace-access`, filesystem mutations use `filesystem-write`; git-status/git-diff/run-command/app-launch/app-quit/system-info/process-list/current-time use `subprocess-execution`; web-fetch/web-search use `cloud-provider-access`; voice uses `voice-capture` or `voice-playback`. Remaining Agent-category tools (including browser/clipboard/alarms/todos) use `context-injection` as the coarse domain, while their explicit authorization requirements in the table express the actual resources. This mismatch in vocabulary is technical debt; coarse domain labels must not replace resource authorization.

`read-file` applies offset/limit after a bounded 64 KiB read, so line paging does not establish access to an arbitrary tail beyond that byte cap. `edit-file` refuses an existing empty file and files above its 16 MiB complete-read bound, while allowing creation of a missing file with empty oldString. These are source-visible capability limitations, not newly induced failures.

Test debt: wrapper tests are uneven; no exact tool-ID literal was found in tests for app-quit, system-notify, web-fetch or voice-transcribe. This is a bounded text-reference check, not proof of zero indirect coverage. Most browser tests verify missing prerequisites, not successful browser rendering. Network/voice backend tests do not establish end-to-end tool delivery. `experimentalReasons` retains an executor-wide checklist explanation, but the later assignment and actual handlers now use session-scoped state; its effective maturityReason is corrected. Remove obsolete map text in a future separately authorized cleanup.

### Dynamic tools and additional definitions

- **MCP — Experimental:** `McpToolCatalog::mcpToolToDescriptor` generates `mcp.<escaped-server>.<escaped-tool>`, Medium risk by default, `tool-execution`, ExternalService/Invoke provider authorization; optional declared filesystem semantic evidence. `McpToolProvider::refresh/disconnectServer` validates schema/names/collisions and atomically replaces/unregisters inventory. Actual tool names/count depend on configured servers; no personal daemon inventory was queried. Tests: `test_mcp_tool_provider.cpp`, `test_mcp_integration.cpp`.
- **Plugins — Experimental:** `PluginManager::registerRemoteTools` generates `plugin.<escaped-plugin>.<escaped-local-id>`, requires declared tool-execution permission, strict schema, host capability subset, explicit risk/requirements and a live host handler. The SDK sample `plugins/samples/custom_agent_tool/CustomToolPlugin.cpp` defines **echo** and **delayed_echo**; a second sample module reuses that source. These are not enabled built-ins. Tests: `test_plugin_tool_integration.cpp`, `test_plugin_manager.cpp`, broker fixtures.
- **Non-registry helpers:** `core/src/output/StructuredOutputTool.cpp` (`validateOutput` only parses JSON and does not enforce its supplied schema) and `core/src/codemode/CodeModeInterpreter.cpp` are not entries in BuiltInToolProvider's registration map. Their existence must not inflate the built-in inventory or imply an additional execution system. Null/compatibility fixture descriptors likewise are not additional production built-ins.

A complete configured runtime inventory remains **Unverified** because doing so would inspect personal extension configuration or start a new configured runtime. The finite static inventory and registration mechanisms above are verified from current source.

## 8. AI Backend and Model Management Assessment

| Backend/operation | Status | Implementation evidence and limits |
| --- | --- | --- |
| Ollama | Implemented; live unverified | `OllamaChatProvider.cpp`, `OllamaRuntime.cpp`, `LocalInference.cpp`, ModelService registration/discovery; stream/chat/tool handling and list/pull APIs. Endpoint/model readiness required. |
| llama.cpp server | Implemented/Partial | `ModelService.cpp` selected endpoint provider; daemon `LocalModelRuntime::ensureRunning` starts fixed llama-server command for registered GGUF on allowed loopback HTTP, health probes and process ownership. External server recognized rather than replaced. |
| LM Studio / local compatible | Implemented; live unverified | `ModelService.cpp` registers `lm-studio`, `openai-compatible-local`, compatible request/discovery paths; ModelOperationService uses LM Studio inventory. No model download/load acceptance here. |
| OpenAI-compatible / cloud | Implemented adapters; unverified credentials/endpoints | ModelService IDs include cloud-api, openai, openai-compatible, claude, gemini, deepseek, groq, mistral; `SelectedEndpointChatProvider`, `HealthTrackedProvider`, native Claude/Gemini translation and provider configuration resolution. Network and user credentials gate execution. |
| GitHub Copilot helper | Partial/unwired | `core/src/copilot/CopilotProvider.cpp` only stores configuration, reports nonempty key and constructs an endpoint; no request implementation or production registration found. Not a verified cloud backend. |
| Catalog/discovery | Implemented | `ModelLibrary.cpp`, `ModelRegistry.cpp`, `HuggingFaceModelSource.cpp`, cached capability/fit and provider-owned inventory. StaticProviderCatalog placeholders are not discovered runnable models. |
| Download/import/register | Implemented/Partial | `ModelOperationService::{beginDownload,issueDownloadRequest,loadRecoveryJournal}`, QSaveFile streaming, size/header/hash checks when metadata supplied, sidecar ownership and interrupted records; GGUF registration. Full remote lifecycle unverified. |
| Loading/unloading | Partial | Selected GGUF daemon-owned process starts/stops on model change while idle; shutdown owns its process. Ollama/LM Studio server lifecycles remain provider-owned. No uniform verified load/unload contract across every backend found. |
| Frozen run binding | Implemented | ModelService resolves through IModelRouter; AgentRuntime/loop retain ModelBinding. Selection changes target future runs, not an active run. Tests: test_model_registry, test_runtime_provider, test_static_model_router, provider certification fixtures. |

`LocalModelRuntime` is a runtime process helper, not a second model router. It hardcodes context size 8192 and parallel 1. On no selection it may choose the first existing GGUF; an HTTP 200 `/health` is treated as ready before discovery. It does not prove the responding external server loaded the requested alias at that point. Its launch explicitly sets `unconfinedPermitted=true` for fixed provider activation, separate from model-generated tools. Review selection/ownership/health identity and crash retry behavior before treating this helper as general automatic recovery: attemptedKey suppresses repeated launch for the same key, and provider switching cleanup is not a general resource manager.

Downloads of safetensors/bin/ONNX are artifact acquisition only; they do not assemble a full repository or make it runnable by llama.cpp. `ModelStorageManager` limits deletion/cache cleanup to owned artifacts/sidecars. Runtime installer in `DaemonModelHelpers.cpp` can use explicit Homebrew/source setup, but was not invoked. No dependency/model installation was performed.

Readiness presentation remains partial: `ApplicationController::localChatSendAvailable` primarily checks selection/idle/archive rather than proving live provider health. P4.1 report and inspected ModelService binding failures agree that selected/installed/connected must not imply execution-ready. Some local summaries remain Ollama-oriented even when another backend is selected. Static lifecycle prices and capability placeholders are not current provider facts.

## 9. Resource Management Assessment

**Implemented facts:** `HardwareCapabilityService::refresh` collects OS/architecture/logical CPU count; RAM via sysctl/sysconf/GlobalMemoryStatusEx; macOS physical CPU count; Linux DRM vendor and optional VRAM total. Windows/macOS native GPU inventory is not populated by that adapter. `ModelLibrary` compares artifact size and reported RAM and propagates provider active VRAM telemetry. `ModelOperationService::refreshOllamaTelemetry` queries `/api/version` and `/api/ps`. `ModelStorageManager::canStore` checks disk capacity/reserve.

**Partial:** facts and fit estimates support recommendations, not memory admission guarantees. `HealthService::report` aggregates registered Boolean checks; it is not an OS sampler. `system-info` reports facts rather than load.

**Planned/unverified gap:** no continuous cross-platform CPU utilization, available-memory pressure, GPU utilization/temperature sampler or adaptive eviction/backpressure policy was found in the inspected hardware, model, scheduler and health implementations. `AgentExecutionScheduler` enforces fixed concurrency/conflicts; it never reads hardware pressure. LocalModelRuntime fixed context/parallel flags are not adaptive policies. A future resource policy should feed the existing ModelService/runtime/scheduler, preserving one route and execution authority.

## 10. Security and Permission Assessment

**Implemented boundaries:** `ToolExecutionGateway::{validatePlan,execute,executeAsync}` validates strict descriptors/arguments, snapshot consistency, handler/platform availability, approvals, authorization and sandbox decisions. `AuthorizationResolver`, `ResourceAuthorizationResolver`, `ExternalDirectoryGate`, `PermissionPolicyService`, `PolicyEvaluator`, `SQLitePermissionGrantStore` bind resources and access modes. Secure file mutation backends and `QtFileSystemService` revalidate resources; tests cover workspace escape/replaced resources. Approval responses are run/one-use-ID checked by the daemon and cannot silently become permanent grants.

**Sandbox — Partial cross-platform:** `core/src/runtime/ProcessSandbox.cpp` uses Linux Bubblewrap and macOS sandbox-exec when available and refuses required unavailable/failed enforcement. Windows restricted token/Job Object reports PartiallyEnforced because filesystem confinement/network denial are incomplete. macOS forbid-detached plans prohibit forks, not equivalent to kill-on-parent-exit. `ProcessExecutor` owns tracked processes/output/cancellation. Native helper/provider activation has separate explicitly authorized unconfined paths; this is not universal tool sandbox coverage.

**Credentials — Implemented, availability conditional:** `CredentialStore.cpp` selects Keychain, Windows Credential Manager or Linux Secret Service; unavailable backends report disabled/unavailable rather than plaintext fallback. `ProviderCredentials.cpp`, AppSettings and daemon settings projections separate values from presence. Legacy migration requires successful secure write/readback. OS backend operation was not exercised against personal keychains; test fixture success is narrower.

**Network policy — Implemented application gate, not a firewall:** `core/include/sentinel/core/network/NetworkPolicyService.h` keeps a process-wide atomic Online/Offline/LocalOnly policy. Both restrictive modes allow localhost/literal loopback; external transport callers must apply the check and still enforce tool authorization. This source does not establish OS-wide network confinement or DNS-rebinding resistance.

**IPC — same-user trust:** Unix owner checks/private directories/peer UID prevent another UID using the endpoint under the stated threat model. Same-user malicious software remains trusted enough to send cancel/shutdown/configuration commands. Local IPC is not an authentication boundary against a compromised login. No remote TCP daemon listener is claimed.

**Extensions — Experimental:** PluginManager uses separate PluginHostSession/native host and declared capability brokerage. The host runs native code and cannot be made safe solely by JSON permission declarations. McpService/McpOAuthService include process/network/OAuth behavior and schema enforcement; remote server declarations/results remain untrusted evidence. Skills are instructions/resources, not an isolation boundary (`core/src/skill/SkillService.cpp`, extension service, tests). This audit did not enable any extensions.

**Audit/history — Partial:** AgentActivityLog and SQLiteAgentRunStore retain execution/evidence/authorization detail; FileLogger records application diagnostics. `DurableEventLog` also exists as an auxiliary service. No tamper-evident independent security audit sink was established by this inspection; do not equate chat history or diagnostic logs with a complete immutable security ledger. Retention/redaction and sensitive tool output need explicit policy.

No penetration test, full secret scan, dependency advisory refresh or live GitHub governance query was performed. Existing SEC-P1 reports remain historical evidence; unresolved findings are not silently waived.

## 11. Memory, Voice, Multimodal and Platform Assessment

### Persistence and context

**Implemented:** settings (`JsonSettingsStore`/platform wrapper), memory (`SQLiteMemoryStore`), chat history (`SQLiteChatHistoryStore`), conversations (`SQLiteConversationStore`), runs (`SQLiteAgentRunStore`) and permissions (`SQLitePermissionGrantStore`) are separate owners. Qt SQL/QSQLITE is used. Daemon IPC additionally persists run projections without duplicating transcripts/grants. Source has schema metadata/transactional migrations/column upgrades and tests: `test_storage_migration`, SQLite store tests, `test_persistence_startup_certification`, background-job persistence. StorageMigration moves legacy profile data and repoints stored paths; it is distinct from database schema migration.

**Implemented/Partial retrieval:** `SemanticRetrieval.cpp` contains embedding/provider/vector retrieval contracts; `LocalRagStore.cpp` stores documents/retrievals; memory recall/context assembly integrate selected entries. Backend/model-dependent embedding quality and latency are unverified. `MemoryRecall` explicitly uses literal key/value matching; base ContextAssembly reports semantic/vector search disabled, while ApplicationController separately enables Ollama semantic supplements only through explicit embedding/context-inclusion policies. Do not conflate these paths. `InMemoryMemoryCandidateStore` is volatile; todos also disappear on daemon restart. A persistent conversation is not automatically a persistent approved semantic memory item.

`ContextEngine::build` estimates token cost approximately by characters/4 + overhead, reserves at least 1024 or one fifth of window/output budget, caps assembled budget at 24,000, and keeps critical goal text even if it exceeds remaining budget. Consequently the assembly limit is an estimate, not a strict tokenizer/context guarantee. `CompactionService`/structured summaries exist but terminal `/compact` is unsupported. Tests for context assembly/estimation/semantic retrieval are fixtures, not proof of arbitrary long-session retention.

### Voice and multimodal

**Implemented/Experimental:** `WhisperTranscription.cpp`, `PiperTts.cpp`, `UnifiedAudioService.cpp`, `AudioFileSession.cpp` and daemon voice IPC implement configured local STT/TTS, cancellation/readiness/failure and bounded PCM ownership. Desktop captures microphone audio; daemon performs STT. `MacSystemDictationService.mm` supplies conditional native on-device dictation; non-macOS service reports unavailability. Default legacy providers are disabled Null clients until configured. No microphone, speaker, model, or OS permission prompt was exercised here.

**Implemented/Partial attachments:** `ChatAttachmentLoader` accepts <=4 attachments, <=4 MiB each; text extraction capped at 24,000 characters. Images are bounded/decoded and scaled (1536 maximum side), then require a vision-capable provider. Optional Qt Pdf extracts text with a page bound; scanned PDFs have no OCR. DOCX invokes external unzip on a snapshot and parses XML. Video/general OCR/document-layout understanding is not established. `ImageProcessor` helpers do not independently prove end-to-end model vision capability; its `compress` method currently returns the input unchanged.

### Platforms

- **Linux/Fedora/KDE — Partial acceptance:** portable Qt shell, Bubblewrap, Secret Service and Linux packaging exist; `packaging/linux/systemd/sentinel-desktop.service` actually starts `/usr/bin/sentinel-daemon` and restarts on failure. DBus activation and Flatpak manifests also exist. Native KDE/Wayland flow was not run. NativeCompanionAdapter lacks Linux global shortcut registration, so primary-target parity is incomplete. Multiple legacy/current Flatpak filenames require packaging inventory reconciliation.
- **macOS — Implemented paths, acceptance incomplete:** Objective-C++ dictation, Carbon hotkeys, Keychain, sandbox-exec and bundle/signing hooks. Current commands ran on macOS; unsigned/ad-hoc warning is not a successful signing/notarization result.
- **Windows — Partial:** CMake/MSVC packaging/platform crash/protocol/taskbar/DPAPI/credential/restricted process support exists. Rust named-pipe transport missing and sandbox restrictions incomplete. Windows ARM64 preset is experimental. Neither Windows native execution nor cross-compiled output was validated here.

## 12. Test and Release Readiness

Existing tests cover core contracts, real temporary IPC transports with deterministic providers, filesystem/process security, models, persistence, QML presentation and Rust terminal rendering. CI files define Linux/macOS/Windows C++ builds/tests; Linux quality includes formatting/QML lint/metadata checks and ASan. Rust CI declares 1.94 and Ubuntu/macOS fmt/clippy/test. CodeQL and release workflows exist. Their presence is source evidence only; this audit did not query current hosted CI runs.

`CMakeLists.txt` owns release identity 1.0.0-rc.8 and Rust IPC build.rs derives application version from it. Cargo package metadata also contains that version; future version edits must preserve synchronization. CPack/signing hooks and draft release workflow do not imply legal/native release acceptance. `README.md` and `SEC_P1_4_GITHUB_HARDENING_REPORT.md` explicitly retain public distribution blockers: lru advisories/paste maintenance, notices/Qt corresponding source, Core Warden/reference/branding/sound rights, privacy/history artifacts, binary/asset matching, native acceptance, signing and governance. Advisory status and remote protection settings were **not refreshed** here; treat the register as pending verification/remediation, not new exploit evidence.

Current validation results are recorded in section 16. No release package, signing operation, update installation or release workflow was triggered.

## 13. Current vs Target Gap Matrix

The supplied P0–P12 roadmap is not fully specified in tracked `docs/ROADMAP.md`. Target states below are **audit recommendations inferred from the user's objective and repository invariants**, not invented phase commitments. P0/P1/P2 priorities mean blocking baseline, next correctness/security work, then expansion; they do not assign numbered roadmap ownership.

| Area | Current state | Target state | Required work | Priority | Dependencies | Evidence |
| --- | --- | --- | --- | --- | --- | --- |
| Authority | Desktop/Rust daemon-backed; legacy CLI in-process | Clear single production runtime ownership | Define compatibility and retirement/migration gate for C++ CLI; document bootstrap launch | P0 | IPC parity/profile ownership | ApplicationBootstrapper, legacy ChatCommand, DaemonService |
| Agent acceptance | Evidence gates and fixtures, live reliability unverified | Reproducible grounded task success/failure/cancel | Run approved isolated model/task matrix; retain failure taxonomy | P0 | model availability, sandbox, existing runtime | AgentLoop, README, agent tests |
| Release | Explicit blocked register | Exact candidate meets all gates | Revalidate/remediate dependency/provenance/privacy/native/governance findings | P0 | maintainer/legal/admin decisions | SEC-P1.2/1.3/1.4, release process |
| Docs | Current IPC mixed with Stage A/no-spawn/future CLI claims | One accurate current-state narrative | Update stale docs separately after accepting baseline | P0 | architecture decision | DAEMON.md, ARCHITECTURE.md, ROADMAP.md |
| IPC regression | One handshake failure then complete rerun pass | Deterministic transport fixture | Examine mock close vs client timeout restoration; repeat scoped test under controlled conditions | P1 | existing Rust client/tests | sentinel-ipc lib.rs transport_tests |
| Readiness | selection/connection can look ready before binding succeeds | Health/capability-specific truthful UI | Align send summaries and model/provider readiness projections | P1 | ModelService health/binding | localChatSendAvailable, P4.1 report |
| Tools | 38 definitions; several conditional and sparse wrapper tests | Configured usable inventory + tested outcomes | Surface descriptor/handler/platform reasons; add missing bounded wrapper/receipt coverage | P1 | gateway/registry, fixtures | BuiltInToolProvider, ToolCompleteness, T |
| KDE Quick Panel | Linux global shortcut unavailable | Native primary-target activation | Add supported KDE/portal/platform backend through existing adapter boundary and native tests | P1 | platform APIs, Wayland session | NativeCompanionAdapter::registerShortcut |
| Windows terminal | no named-pipe transport | Same daemon contract on Windows | Implement shared-client Windows transport and native CI fixtures | P1 | canonical IPC, Windows test host | sentinel-ipc Windows Client |
| Sandbox | Linux/macOS conditional, Windows partial | Honest enforced policy per platform | Certify backend limits; isolate non-cooperative provider/extension processes where needed | P1 | platform backend/test hosts | ProcessSandbox, shutdown drain |
| Model lifecycle | downloads/register/process startup; provider ownership varies | Explicit load/unload/ownership/health identity | Define lifecycle contract and retries using ModelService and existing process helper | P1 | provider APIs, ProcessExecutor | LocalModelRuntime, ModelOperationService |
| Resources | capability facts/fit; fixed concurrency | Pressure-aware bounded scheduling | Add sampler and policy inputs to existing scheduler/model authority | P2 | platform telemetry, validated admission policy | HardwareCapabilityService, AgentExecutionScheduler |
| Context/memory | approximate budget, partial semantic integration | Predictable bounded context and persistence semantics | Tokenizer/oversize policy, retrieval acceptance and documented memory retention | P1 | model capabilities, stores | ContextEngine, SemanticRetrieval, SQLite stores |
| TUI unsupported actions | explicit unavailable plan/compact/undo/etc. | Supported operations only after backend authority exists | Add contracts and tests incrementally; keep truthful unavailable states | P2 | daemon policy/restore/compaction contracts | commands.rs Unsupported |
| Voice/multimodal | configured STT/TTS/images/text docs; OCR absent | Verified selected feature scope | Native audio/vision flow acceptance; decide OCR separately | P2 | binaries/models/permissions, optional Pdf/unzip | UnifiedAudioService, ChatAttachmentLoader |
| Stub services | WebUi/ACP state-only | Clear non-product status or completed bounded integration | Mark unused/stub scope; remove or implement only in approved roadmap work | P2 | product scope/security contract | WebUiService, AcpService |
| Composition | very large controller/IPC classes, legacy metadata overlap | Maintainable existing modular monolith | Map ownership, reduce duplication within current boundaries when authorized | P2 | passing regression suite, architecture review | ApplicationController, StaticProviderCatalog |

## 14. Critical Risks

1. **P0 — False readiness/completion expectations.** Transport connected, selected model, installed file, tool acknowledgement and accepted grounded final are different facts. Existing UI readiness labels and generic experimental tool receipts can blur these. Preserve the final-answer gate and expose accurate reasons.
2. **P0 — Distribution blocked.** Prior dependency/legal/privacy/governance findings cannot be closed by a green build. Revalidate exact candidate and remote settings before any release decision.
3. **P1 — Primary-platform/Windows gaps.** KDE Quick Panel global shortcut is missing; Rust Windows IPC cannot connect; Windows process sandbox is partial. Cross-platform CI compilation is insufficient native certification.
4. **P1 — Shutdown and cancellation limits.** Cooperative C++ providers can prevent full drain; synchronous network tools can defer cancellation. An external supervisor/process boundary is required for a hard termination guarantee.
5. **P1 — Divergent legacy semantics.** C++ CLI timeout/pull failure can exit successfully and uses a separate in-process composition. This can mislead scripts and complicate profile/run authority.
6. **P1 — Test nondeterminism.** Rust handshake fixture failed once with Disconnected and passed on rerun. Treat as an unresolved potential race, not proven production transport failure or clean first-pass acceptance.
7. **P2 — Architecture narrative debt.** Static catalogs, state-only integration stubs and large controller classes make implementation maturity hard to infer. Additions must reuse AgentRuntime/IModelRouter/ToolExecutionGateway rather than create replacements.

## 15. Recommended Development Priorities

1. Accept this baseline and define the actual P0–P12 target/acceptance matrix; reconcile the current branch with the intended release branch without assuming remote main matches.
2. Close P0 documentation/readiness/release-gate ambiguity. Keep distribution blocked until exact findings receive verified dispositions.
3. Investigate the Rust handshake regression fixture and retain deterministic IPC/agent/cancellation/security suites as hard baselines.
4. Run an explicitly authorized isolated real-model task matrix through the existing runtime, including failure, denial, cancellation, repetition and restart cases. Do not tune final gates merely to make acceptance look green.
5. Complete primary-platform KDE/Wayland and Windows IPC/sandbox acceptance before claiming platform parity.
6. Strengthen model lifecycle/ownership, resource monitoring and context guarantees through the existing services; only then expand unsupported terminal/extension/voice capabilities.

These are recommendations only. No implementation or configuration change is authorized by this audit report.

## 16. Evidence, Commands and Test Results

Commands were run in the supplied checkout using existing dependencies. CMake reported 4.4.4 and configured Homebrew Qt 6.12.0; rustc reported 1.98.1 (so this run does not certify the declared Rust 1.94 minimum). Build outputs are ignored/generated artifacts, not permitted source changes. Current-run results below are separate from all historical reports.

| Command/check | Result and interpretation |
| --- | --- |
| `command -v brain` | Unavailable on PATH; project brief not run. |
| `git branch --show-current`, `git rev-parse HEAD`, `git status --short` | Branch/SHA above; initial clean tree. |
| Read instructions/source/docs with `cat`, `sed`, `rg --files`, `rg -n` | Actual source inspected across each requested area; file/symbol citations inline. No external repository content substituted for local source. |
| Python read-only descriptor/map/test-reference enumeration | 38 descriptors and handler map inspected; table generated from source evidence and reviewed behavior. Literal test references do not imply full coverage. |
| `python3 tools/ipc/generate.py --check` | PASS, exit 0. |
| `python3 tools/ipc/generate_desktop.py --check` | PASS, exit 0. |
| `python3 tools/analysis/check_personal_paths.py` | PASS, 0 files flagged before and after report creation; not universal privacy certification. |
| `cargo fmt --all --check --manifest-path cli/Cargo.toml` | PASS, exit 0. |
| `cargo clippy --locked --offline --manifest-path cli/Cargo.toml --all-targets -- -D warnings` | PASS, exit 0; dependencies already cached. |
| `cargo test --locked --offline --manifest-path cli/Cargo.toml` in sandbox | FAIL due to environment: CLI 6 passed/9 failed at UnixListener::bind, Operation not permitted. Not a product failure diagnosis. |
| Same Rust command with local-socket sandbox restriction lifted | CLI 15/15 PASS; IPC 14/15, handshake_correlation_and_event_queue FAILED with Disconnected; workspace stopped at failing target. |
| `cargo test --workspace --no-fail-fast --locked --offline --manifest-path cli/Cargo.toml` with local sockets allowed | PASS exit 0: CLI 15, IPC 15, mascot 22, TUI 78 = **130 tests**; doc-test targets zero tests. Previous intermittent failure remains recorded. |
| `cmake --preset tests` | PASS exit 0; macOS unsigned/ad-hoc signing warning; no dependency install. |
| `cmake --build --preset tests` | PASS, exit 0; Desktop, daemon, legacy CLI, plugin host and tests built on macOS. No signing/notarization acceptance. |
| CTest | PASS, exit 0: `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure`, all **127/127 CTest targets**, 157.61 seconds. Includes registered benchmarks and offscreen UI/IPC/security fixtures; target count is not individual Qt assertion count. |
| Native GUI / physical terminal / KDE/Wayland / Windows / live local or cloud model / actual audio/vision | NOT RUN; require isolated native environments and configured runtime resources. No success inferred from fixtures. |
| Package/sign/notarize/release/dependency scanner/live GitHub governance | NOT RUN; outside safe audit validation scope. |

Potential Rust race evidence: `cli/crates/sentinel-ipc/src/lib.rs`, `transport_tests::handshake_correlation_and_event_queue`, failure at request/response path around line 613. The mock closes after two replies; the client restores socket timeout state on successful requests. That timing is a hypothesis for investigation, not a proven root cause. The full repeat passed; no code was changed.

Historical evidence consulted, with no claim of rerunning it: `docs/development/P41_DESKTOP_LIVE_INTEGRATION_RESULT.md`, `PHASE3_DESKTOP_IPC_MIGRATION_REPORT.md` guidance through current architecture docs, SEC-P1 consolidated blocker register and linked dependency/distribution assessments, README release warnings. Historical native/model outcomes apply only to their recorded source/binaries.

## 17. Unknowns Requiring Further Verification

- The authoritative full P0–P12 phase definitions, acceptance owners and intended baseline branch; docs/ROADMAP.md is broad direction, not that contract.
- Remote main/PR state, current hosted CI and live protection/security settings; none queried or fetched here.
- Exact configured dynamic tool/provider inventories, OS secret-store readiness and user permission grants; personal runtime state was not inspected.
- Reliable real-model completion for representative tasks, provider-specific tool calling/streaming and cancellation latency; no model run/download.
- Native Desktop/Quick Panel/tray/audio/dictation/Wayland focus and responsive flows; automated fixture evidence is narrower.
- Hard termination and crash recovery guarantees for non-cooperative providers/plugins, local model health identity and retry/backoff behavior.
- Full successful browser/network/voice tool wrappers, GUI clipboard availability through a headless daemon, and all platform mutation receipts.
- Exact release payload, advisory status, notices/corresponding-source delivery, artwork/sound provenance, privacy history and signing acceptance.
- Rust handshake failure reproducibility/root cause and whether it affects only the fixture or a production close timing path.
- Cross-platform resource pressure thresholds, tokenizer accuracy, persistent semantic memory quality and migration durability under real power loss.

Only this new report is an intended tracked repository modification. No source/build configuration/dependency/existing documentation edits, commits or pushes form part of P0.1.
