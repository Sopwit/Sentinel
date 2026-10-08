# SENTINEL — SYSTEM-WIDE FEATURE & INTEGRITY AUDIT REPORT

Date: 2026-10-08. Scope: current macOS dirty working tree and isolated production binaries built from it. **Verdict: FIXES REQUIRED.** Discovery only. Personal-Brain `brain` command unavailable; repository path explicitly supplied by user was used. Architecture, Agent Runtime, security, building and testing instructions read before audit.

This is a repository-wide discovery report with bounded evidence, not certification of every line or every platform. Lexical inventories are complete for the captured file set; heuristic classifications and runtime gaps are explicitly distinguished. Prior reports were not reused as current PASS evidence.

## 1. Executive Summary

Total features: 64. PASS: 12; PARTIAL: 21; BROKEN: 8; UNREACHABLE: 5; STALE: 2; MOCK_ONLY: 1; DEFERRED: 0; BLOCKED: 13; UNKNOWN: 2.

P0: 0; P1: 5; P2: 8; P3: 5; v1 blockers: 11 confirmed issue records. Counts refer to feature rows and issue records respectively, not tests or files.

Main failures: dangling Agent descriptor, real Agent planning, disconnected Inspector/Controlled Tasks/Knowledge Base, missing Desktop fields, incorrect installed-model badges and legacy duplicate authority. [Canonical feature matrix](SYSTEM_WIDE_FEATURE_MATRIX.md) gives one status per bounded feature. [Evidence index](system-audit-evidence/README.md) defines evidence limits.

## 2. Repository Inventory

1129 baseline repository files scanned; ACTIVE_PRODUCTION 889, TEST 128, GENERATED 5, PLATFORM_SPECIFIC 91, HISTORICAL 16. 358 C++, 353 headers, 72 QML, 8 Rust, 78 Markdown, 8 .cmake plus CMakeLists and packaging/resources/assets/configs are included. Classification is a mechanical navigation aid, not proof of liveness. No confirmed dead file was deleted. WindowsSecureFileMutationBackend candidate resolves ACTIVE through CMake registration and securefs platform declarations. Lexical scan contains 1060 marker occurrences, heavily including SPDX “or later”, unsupported rejection branches and UI placeholderText; these are not 1060 defects. Exact source/line inventories preserve reviewable candidates. Per-marker categories are heuristic; unreviewed ambiguous candidates remain evidence limitations, not certified harmless findings.

## 3. Core Architecture

PARTIAL. Desktop main → ApplicationBootstrapper → shell/bridge → DesktopRuntimeClient → typed daemon. DaemonService composes runtime, settings, workspace, model/provider and store authority; AgentRuntime owns AgentLoop execution. Rust CLI/TUI are thin clients. Exception A09: independently authoritative legacy C++ Chat CLI remains built/installed. A01 violates descriptor lifetime; A04/A06 strand local-service consumers after remote migration.

## 4. Desktop

PARTIAL. Main exposes Home/Dashboard, Models, Inspector and Settings, with QuickPanel composition. 259 lexical UI handlers are inventoried in ui-actions.tsv, not claimed individually E2E-certified. Launch, seven onboarding steps, navigation, model cards, Inspector error, Memory blank states and restart reconnection observed. Loading/empty/error evidence exists in subsets; comprehensive keyboard-focus, responsive resizing and every control success/error projection remain UNKNOWN. Four registered legacy QML components lack instantiation in inspected current Main graph; registration alone is not reachability.

## 5. Daemon

PASS for isolated lifecycle and typed service transport. Fresh daemon starts with protected profile permissions, rejects unsafe profile mode, accepts terminal/Desktop clients and changes generation on restart. No production user profile used. Desktop gaps are client contracts, not proof daemon services are absent.

## 6. Chat

PASS for real Ollama streaming, multi-turn, cancellation, reattach and persisted resume. Existing qwen2.5:3b only; no download. Same session returned AUDIT_READY then remembered that token. Stdin transport worked (model did not reproduce the exact requested token). Context/provider failure paths have fixtures; complete Desktop flow and cloud/offline transitions remain PARTIAL.

## 7. Agent

BROKEN. Greeting and three bounded safe-read planning attempts fail “Agent could not determine a valid next action.” A qwen planner attempt emits action=read-file rather than validated action=tool. Explicit format prompt and Ask Every Time did not complete. Do not infer every provider fails. A01 definite dangling descriptor reference is independent of the unproven runtime root cause. No accepted final answer, successful safe edit or real allow/deny smoke. A08 raw planner JSON deltas exposed; no secret chain-of-thought observed. Tool completion remains distinct from accepted AgentLoop final completion in architecture.

## 8. Tools

PARTIAL. tools.tsv inventories 35 built-in descriptors, names, risk/mode, schema, implementation/test references. Audit doctor exposed 32; disabled local-plan-summary and todo read/write explain catalog difference. builtInSchema rejects additional properties; ToolArgumentValidator and ToolExecutionGateway are authority with descriptor resource/permission policies. Cancellation differs for async ProcessExecutor, synchronous metadata/FS, MCP and plugin transports; no blanket cancel-all claim. Result/evidence envelopes and complete-observation grounding have tests. Production Agent per-tool E2E is not completed. Registration, validators and permissions were traced; no demonstrated gateway bypass, but A01 is a descriptor integrity defect.

## 9. Filesystem

PASS for bounded enumeration, PARTIAL/BROKEN for Agent read/edit workflow. Real selected workspace scan returns audit.txt, excludes hidden fixture and reports truncated=false. Hidden-file negative claims must depend on scope/include-hidden/completeness metadata, not omission alone; existing grounding fixtures pass. Traversal, symlink, recursion, permissions, secure mutation and negative-claim tests pass. No safe edit or Agent-read success claimed; upstream planning fails. Immutable builtin workspace root update correctly rejects; custom root succeeds.

## 10. Models / Providers

Ollama real Chat PASS. Provider runtime/catalog contains local/server and cloud adapters (see matrix); remaining provider rows BLOCKED for configured live E2E. Discovery/capability/error/network logic has fixtures, not cloud certification. LocalEcho is MOCK_ONLY test/development evidence. Credentials were not enumerated or exposed. Provider-neutral immutable ModelBinding is architectural; model-specific Agent contract compatibility fails current smoke.

## 11. Model Library

BROKEN installed identity A07: live inventory has only qwen2.5:3b but UI marks family 14B/32B installed. Curated modelCatalog entries are static marketing/metadata, not installed inventory. Refresh/select reachable. Download/removal intentionally not invoked; associated tests do not replace product lifecycle evidence.

## 12. Memory

Core persistence PASS: production remember action accepted synthetic SAFE_MEMORY_VALUE; read-only isolated SQLite confirms row and terminal count1. Memory is active, not merely structural. Separate memory/history stores and context retrieval tests pass. UI BROKEN A05: Memory (), Continuity / and Chat History / blank summaries. UI delete/clear and corruption recovery are not real smoked.

## 13. Sessions / History

PASS for persisted original session restoration across daemon restart and terminal resume, with Desktop reconnect. SQLite conversation/history and daemon session projection separate from agent runs/grants. Titles/summaries/pin/archive/clear/retention tests and routes exist but complete production UI lifecycle is PARTIAL. No claim every duplicate-event/concurrency path real stress-tested.

## 14. Controlled Tasks

BROKEN Desktop A04. Service implementation/tests useful, daemon construction exists, but shell accesses controller_.controlledTasks() through remote pointer projection returning nullptr. Planning returns empty; approval/start/cancel false. No scheduling/recovery runtime proof. Backend structural presence is not reachable Desktop functionality.

## 15. Context Engine

PARTIAL. Agent ContextEngine assembles workspace, memory, file/resource and model constraints with privacy/evidence rules. Chat has intended separate chat context service; separate purposes do not alone imply duplicate authority. Unit/grounding/context tests pass; successful real Agent context/final grounding blocked upstream. Compaction and every attachment path not runtime-certified.

## 16. Workspaces

PASS bounded create/select/restart persistence. Custom workspace root created in isolated fixture and terminal selected it; file references reflect scope. Builtin coding root immutability enforced. Rename/removal/missing-root/full Desktop path fixtures are not all real smoked.

## 17. Settings

PARTIAL. settings.tsv includes 68 desktop schema keys with consumer references/default metadata. Remote product settings are SettingsService-owned; theme/shortcut also have presentation-local behavior, so daemon theme differing from local theme is not automatically a bug. LocalOnly, Ask Every Time, Dark and workspace persisted isolated restart. Unsupported key correctly rejected. References alone do not verify every setting consumer/load/error/restart; A05 projection and A10 safe-mode claim confirmed.

## 18. Secrets

BLOCKED live platform lifecycle. CredentialStore/ProviderCredentials and platform secure-store abstractions reviewed; Windows encrypted settings wrapper exists. No actual API keys accessed, printed or copied. Synthetic Keychain/DPAPI write/delete/migration and Linux secure-store acceptance remain platform work; test success is not secret-storage deployment certification.

## 19. Network / Offline

PARTIAL. LocalOnly accepted and persisted; local Chat uses loopback Ollama. Runtime/provider policy fixtures validate restrictions. No cloud credential/endpoint configured, so cloud suppression, network loss recovery and QuickPanel projection need real synthetic endpoint validation. No silent network fallback observed; no exhaustive absence claim.

## 20. Permissions / Approvals

PARTIAL. Policy, approval requests, resource authorization, allow-once, session and persistent grants remain separate concepts in gateway/services. Ask Every Time accepted. Real allow/deny/stale/duplicate approval/reconnect not reached because Agent fails before execution. Fixtures passing do not complete UI/TUI approval smoke. A12 forward schema guard risk.

## 21. Sandbox

macOS PARTIAL; Linux/Windows BLOCKED runtime. ProcessSandbox uses sandbox-exec, bubblewrap and restricted token/job branches. Strict macOS deny-forks explains four shell fixtures skipped; native executor policy tests remain useful. Fail-closed intent and fallback branches inspected, no observed silent unsandboxed execution. Cross-platform security certification requires VMs.

## 22. Scheduler

PARTIAL. AgentExecutionScheduler bounded concurrency/cancel/isolation/event ordering exercised through agent_loop and runtime fixtures; repeated agent_loop until-fail:3 passed. Three repeats are not comprehensive race/sanitizer stress. No real successful Agent/subagent concurrent workload because A02.

## 23. MCP

PARTIAL: IMPLEMENTED + EXPERIMENTAL; not REAL_E2E_VALIDATED externally. Server/config/discovery/tool schemas/routing/failure and stdio fixture tests inspected. Tools route via MCP provider and gateway. No configured server in isolated daemon; no new external setup installed. Desktop management unreachable A15; terminal status subset exists.

## 24. Plugins

PARTIAL/EXPERIMENTAL. Native plugin host/manifest discovery/permissions/lifecycle/isolation and fixture tests exist. Known test plugin lifecycle is integration evidence, not a configured production daemon E2E. Unload/reload/crash/version compatibility need installed-product path smoke. Desktop management gap A15.

## 25. Skills / Extensions

PARTIAL runtime registry and ExtensionService; Desktop management UNREACHABLE. Distinguish skills registry implementation from SkillProfileService metadata and future Custom (A16). No documentation-only concept counted as working runtime. Permissions/discovery/config fixtures exist; real skill invocation not completed.

## 26. Voice / Audio

BLOCKED physical capture/STT/TTS. AudioDeviceService/UnifiedAudioService/VoiceSessionService, RMS, device selection, IPC and cancellation fixtures pass. Runtime voice reports available=false/Idle with no ready STT; no microphone permission prompt or physical capture performed. STS readiness not established. Do not label environmental absence a product capture defect.

## 27. Quick Panel

UNKNOWN complete workflow. Controller/actions contain Ask/Agent/PTT/run/approval/workspace/model/network/Continue/Open/Settings/Quit and reconnect projections. Existing tests pass. Full visible native invocation and dismissal/performance/keyboard flow not completed; local shortcut disabled in fixture. Linux missing implementation is separately BROKEN A11. Notification visual acceptance BLOCKED.

## 28. Native Integration

BLOCKED comprehensive acceptance. macOS bundle Desktop launched visibly; NativeIntegrationTest 3 includes initialization/body/cleanup, not three physical feature certifications. Tray, shortcut, startup, notifications, deep-link/file-open and lifecycle OS acceptance incomplete. Windows/Linux static paths indexed, no VM PASS. A11 Linux shortcut unavailable branch confirmed.

## 29. CLI / TUI

PARTIAL. Built Rust CLI real help/status/doctor/models/sessions/chat/stdin/cancel/attach/resume/completion used. TUI 80x24 rendered current daemon/model and restored cancelled transcript; Ctrl-P palette and Ctrl-O sessions picker, Escape and clean Ctrl-C exit observed. Rust 36 tests cover CLI14/IPC11/TUI11 including sizes. Global install, full real file refs/edit/diff/approvals, all slash commands and resize not certified. Legacy CLI A09/A17 separate.

## 30. Storage / Databases

PARTIAL. storage-schema.tsv inventories Qt SQL CREATE TABLE/PRAGMA/version sites for memory, history, conversations, agent runs, grants, local RAG and projections; auxiliary alarm/config persistence distinct. WAL/migration/corruption/locking fixtures exist. Isolated durable settings/session/memory readback verified. Forward-version grant store risk A12; no unused table declared solely by grep. No destructive user-data tests.

## 31. Recovery

PASS bounded graceful daemon restart/reconnect/session/settings preservation; PARTIAL broad failure injection. Agent protocol failure terminates with explicit error; rejected profile permissions diagnostic truthful. Daemon crash/Desktop crash/provider death/DB corruption/config corruption/network-loss and shutdown races not all physically injected. Suite fixtures are coverage, not all-product runtime proof. A10 recovery flag overclaim.

## 32. Logging

PARTIAL. FileLogger rotates by date and retains days with owner-only permissions; macOS raw sink marks messages public. No central redaction or per-file size cap found A13. Doctor/run/session events useful for correlation. No actual secret leakage observed. Payload privacy and uncontrolled single-day growth require bounded synthetic checks.

## 33. Inspector

BROKEN A03. Reachable Main page, red “Agent history is unavailable.” observed. Actual service constructed nullptr while daemon AgentRunStore exists. Empty list is error, not a legitimate no-runs PASS. No remote Inspector route found.

## 34. Secondary UI Surfaces

PARTIAL. Dashboard/Home uses remote shell projections, current ready/model observed; every metric/card not individually certified. Memory/tasks broken as above. Model catalog curated/static vs live installation distinguished. Registered ChatPanel/ActiveAgentsPanel/CognitionStreamPanel/StatusBar lack current Main instantiation; UNREACHABLE scoped candidates, not deletion instructions. About/settings/onboarding reachable; wizard summary A14.

## 35. Resources / Branding

PASS lightweight integrity checker:1129 source files/69 hashes, authoritative resources/branding. Generated checks current. No full purge repeated. Native icon/tray notification visual acceptance still BLOCKED; source hash check does not certify appearance.

## 36. Documentation Truth

CURRENT: daemon typed protocol/Rust terminal runtime docs that agree with observed flow. STALE/UNDERCLAIM: README partial terminal migration statement and older architecture Desktop-local graph A18. OVERCLAIM: safe-mode option, skill-profile behavior wording, Installed model badge. HISTORICAL: prior phase/cleanup PASS reports, never used as fresh gate proof. No blanket documentation rewrite performed.

## 37. Test Coverage

Fresh 114 C++ targets and Rust36 green. Service/unit, gateway/security, IPC integration, MCP/plugin fixtures, audio mocks and QML shell/controller tests cover many contracts. Crucial gap: tests can construct local service graphs and therefore miss remote Desktop null pointers/projection omissions and real model planner failure. Native3 is minimal fixture coverage. ASan lifetime, full production Desktop composition, successful real Agent and cross-platform native/security flows needed. No flakiness conclusion from one suite and three targeted repeats.

## 38. Skipped Tests

test-skips.tsv has 34 lexical conditional/skip sites, with expanded reason/context ledger. Fresh suite six actual skips: four AgentRuntime shell child-creation fixtures due strict macOS deny-forks; two real executor “missing Docker”/“missing Node” branches because dependencies are present. Reasons valid for branch selection, not feature PASS. Windows/Unix/audio/native availability guards remain platform/environment-dependent. No test deleted or disabled by this audit; exhaustive dynamic skip behavior on other platforms unavailable.

## 39. Generated Contracts

PASS current checks: generate.py --check and generate_desktop.py --check. Protocol1.1 source schema drives generated outputs. Determinism/current output verified, not claim no schema design omissions. A05 is source-contract projection design gap with current generated files, not generated drift.

## 40. Build System

PASS current tests preset configure/build and Cargo workspace gates. CMake target/install/resources/platform conditions inspected; unconditional legacy CLI install authority exception A09. Optional Qt/audio/external tool/platform conditions require platform-specific build acceptance. No unused target declared from weak symbol counts. Whole-repository clang-tidy not executed.

## 41. Platform Conditional Paths

102 lexical occurrences indexed in platform-paths.tsv. macOS current-host build/tests executed; physical sandbox/audio/native gaps explicit. Windows secure mutation candidate active through CMake/platform declarations. Linux bubblewrap/native services and Windows restricted-token/DPAPI/registry paths static only; no cross-platform execution claim. Later Fedora/Windows VM phase must exercise each indexed boundary.

## 42. Dependencies

Hard: C++20/CMake/Qt6 including Qt SQL; Rust terminal crates from workspace lockfile. Runtime-discovered/optional: Ollama/LM Studio/llama.cpp servers, cloud endpoints, Docker, Node/browser helpers, bubblewrap, sandbox-exec, native APIs, audio/STT/TTS executables/config. Existing Ollama server started only for audit; existing qwen model, no new model/dependency install. Missing STT exposed unavailable state. No legal/license compliance audit.

## 43. Security Invariants

PARTIAL with HIGH issues A01/A09. AgentRuntime/AgentLoop final acceptance, immutable ModelBinding, schema validator/gateway, IFileSystemService, separate grants/stores, resource/sandbox boundaries and scheduler ownership traced and fixture-tested. No heuristic natural-language-to-shell planner introduced/found in audited composition; shell requires explicit tool gateway policy. Negative external/FS claims require evidence/completeness. Raw planner output A08 breaches output separation. Null ControlledTask/RAG consumers break authority reachability rather than justify duplicating local authority. Rust remains thin. No P0 demonstrated; broad absence of all vulnerabilities not claimed.

## 44. Real Smoke Matrix

See feature matrix and runtime ledger. PASS: Desktop launch, daemon, installed-model Chat/multi-turn, model selection, custom workspace, bounded FS scan, cancellation/attach, restart/history resume, memory write/readback, settings persistence, CLI/doctor and TUI navigation. FAIL: real Agent greeting/read; read/edit and allow/deny not completed. UNKNOWN: full QuickPanel and responsive UI. BLOCKED: configured cloud providers, microphone/STT/TTS, native visual acceptance and other-platform execution. MCP/plugin live product not configured; fixture tests only. No new downloads, no user production-data destructive tests.

## 45. P0/P1/P2/P3 Issue List

No P0 demonstrated. P1/P2 blockers are current product issues; static confirmed defects distinguished from runtime root-cause uncertainty.

### A01 — Agent descriptor lifetime

Severity: P1. V1_BLOCKER: YES. File/component: `core/src/agent/LlmAgentRuntime.cpp:549-566`.

Symptom: matched points into temporary availableTools() QList; dereferenced after loop destroys it.

Root cause/evidence: Definite C++ lifetime defect; no sanitizer reproduction performed. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Keep descriptor value or owning collection alive.

Test needed: ASan production planning test with real descriptors.

### A02 — Agent current-host E2E

Severity: P1. V1_BLOCKER: YES. File/component: `core/src/agent/LlmAgentRuntime.cpp`.

Symptom: Installed qwen2.5:3b fails greeting and safe file-read planning with invalid-next-action.

Root cause/evidence: Three file planning attempts and greeting failed; model protocol compatibility unresolved, not proof all providers fail. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Repair planner protocol handling after eliminating A01; preserve validation.

Test needed: Real installed-model greeting, read, edit, approvals and grounded final answer.

### A03 — Inspector

Severity: P1. V1_BLOCKER: YES. File/component: `apps/sentinel-desktop/bootstrap/ApplicationBootstrapper.cpp:209-210`.

Symptom: Visible Inspector reports Agent history is unavailable.

Root cause/evidence: AgentInspectorService constructed with nullptr; no remote inspector route found. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Expose daemon-owned run history and bind Inspector.

Test needed: Launch production Desktop against daemon and inspect completed run.

### A04 — Controlled Tasks

Severity: P1. V1_BLOCKER: YES. File/component: `apps/sentinel-desktop/src/DesktopShellViewModel.cpp:5894`.

Symptom: Desktop planning returns empty and approve/start/cancel cannot execute.

Root cause/evidence: Remote bridge returns nullptr for controlledTasks(); no matching remote action route. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Route controlled-task actions and projections through daemon authority.

Test needed: Production Desktop plan, approve, start, cancel and restart recovery.

### A05 — Desktop projection

Severity: P2. V1_BLOCKER: YES. File/component: `tools/ipc/generate_desktop.py:15; apps/sentinel-desktop/src/DesktopRuntimeClient.cpp:351-358`.

Symptom: Memory, continuity and chat-history summaries render blank.

Root cause/evidence: 619 non-eager fields omitted from generated snapshot; reader returns cache without lazy fetch. Some fields are synthesized separately; not all 619 are defects. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Define remote projection contract for every required field.

Test needed: Production composition test plus Memory page populated/empty/error states.

### A06 — Knowledge Base

Severity: P2. V1_BLOCKER: YES. File/component: `apps/sentinel-desktop/src/DesktopShellViewModel.cpp:243,5443`.

Symptom: Remote Desktop add/reindex/clear paths have no store.

Root cause/evidence: localRagStore_ only created for local source; production uses remote source. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Move operations to daemon and expose typed actions.

Test needed: Desktop ingest/query/reindex/clear against isolated daemon.

### A07 — Model Library

Severity: P2. V1_BLOCKER: YES. File/component: `ui/qml/pages/models/ModelsPage.qml:688-717`.

Symptom: Qwen2.5 14B and 32B show Installed with only 3B installed.

Root cause/evidence: Matching drops tag and matches model-family prefix; real UI and /api/tags disagree. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Use canonical exact installed model identity.

Test needed: Install-state test distinguishing tags and runtime UI check.

### A08 — Agent output projection

Severity: P2. V1_BLOCKER: YES. File/component: `core/src/agent/LlmAgentRuntime.cpp:244-253; apps/sentinel-daemon/service/DaemonIpcServer.cpp:1479-1483`.

Symptom: Rejected planner JSON is streamed as user-visible output deltas.

Root cause/evidence: Raw planner stream observer forwards protocol chunks. No secret chain-of-thought observed. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Separate planning protocol from accepted user output.

Test needed: Assert planner JSON never appears in transcript/output.delta.

### A09 — Duplicate legacy CLI authority

Severity: P1. V1_BLOCKER: YES. File/component: `apps/sentinel-cli/commands/ChatCommand.cpp:24-32; apps/sentinel-cli/CMakeLists.txt`.

Symptom: Installed legacy CLI constructs independent ApplicationController/stores.

Root cause/evidence: Documented legacy binary remains built/installed alongside daemon-backed Rust CLI. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Choose one shipping authority; migrate or exclude legacy client.

Test needed: Installed-binary contract test proves all clients share daemon sessions.

### A10 — Safe mode

Severity: P2. V1_BLOCKER: YES. File/component: `apps/sentinel-desktop/bootstrap/ApplicationBootstrapper.cpp:121-124`.

Symptom: Flag promises disabled extensions and factory defaults without enforcing either on existing daemon.

Root cause/evidence: Only daemon autostart suppression and diagnostic flag references found. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Implement explicit daemon safe-mode contract or narrow advertised semantics.

Test needed: Existing-daemon launch with flag validates defaults/extensions policy.

### A11 — Primary Linux shortcut

Severity: P2. V1_BLOCKER: YES. File/component: `apps/sentinel-desktop/src/NativeCompanionAdapter.cpp`.

Symptom: Non-macOS/non-Windows shortcut branch reports unavailable.

Root cause/evidence: Static implementation omission; Fedora runtime not exercised. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Implement supported Linux/KDE shortcut integration and report availability accurately.

Test needed: Fedora VM shortcut registration, conflict, dismissal and reconnect.

### A12 — Permission schema version

Severity: P2. V1_BLOCKER: NO. File/component: `core/src/security/SQLitePermissionGrantStore.cpp:53-82`.

Symptom: Initialization unconditionally sets user_version=2 without rejecting future schemas.

Root cause/evidence: Static upgrade-path risk; no user database mutated. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Reject newer incompatible schema before migration/write.

Test needed: Isolated newer-version database remains unchanged on rejection.

### A13 — Logging bounds/privacy

Severity: P3. V1_BLOCKER: NO. File/component: `core/src/app/FileLogger.cpp`.

Symptom: Daily rotation has no size cap or central redaction; raw messages can enter sinks.

Root cause/evidence: Owner permissions and day retention exist. No actual credential leak observed. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Define payload redaction and bounded growth policy.

Test needed: Synthetic-secret redaction and oversized-log retention tests.

### A14 — Onboarding summary

Severity: P3. V1_BLOCKER: NO. File/component: `ui/qml/onboarding/FinishStep.qml:82,89`.

Symptom: Completion summary said Set up later despite provider/model shown preselected.

Root cause/evidence: Observed UI mismatch; external model selection may not update wizard snapshot. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Make selection adoption and summary source explicit.

Test needed: Preselected model, Next and completion-summary user flow.

### A15 — Extension UI reachability

Severity: P2. V1_BLOCKER: NO. File/component: `ui/qml/Main.qml; core/src/app/ExtensionService.cpp`.

Symptom: Extension management service exists without current Desktop navigation/actions.

Root cause/evidence: No QML extension-settings action/state consumers found; backend/terminal subset exists. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Expose intended management surface or document scope.

Test needed: Production extension install/configure/disable flow without duplicate authority.

### A16 — Skill profile claim

Severity: P3. V1_BLOCKER: NO. File/component: `ui/qml/pages/settings/WorkspaceSettingsTab.qml:331; core/src/app/SkillProfileService.cpp`.

Symptom: UI describes agent behavior selection while service profiles are metadata/future Custom.

Root cause/evidence: Static source states limited integration. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Align wording and scope with actual behavior.

Test needed: Profile selection proves advertised runtime effect or accurately labels metadata.

### A17 — Legacy CLI error exit

Severity: P3. V1_BLOCKER: NO. File/component: `apps/sentinel-cli/commands/ChatCommand.cpp`.

Symptom: Failure/timeout path returns success exit status.

Root cause/evidence: Independent legacy command source inspection. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Return documented nonzero failure status or retire binary.

Test needed: Timeout/error CLI exit-code integration test.

### A18 — Documentation drift

Severity: P3. V1_BLOCKER: NO. File/component: `README.md; docs/development/ARCHITECTURE.md`.

Symptom: Older Desktop/local graph and partial terminal migration descriptions disagree with current Rust/IPC path.

Root cause/evidence: Current composition traced; earlier PASS reports are historical evidence only. Current source location above and matching runtime/evidence inventories support the claim.

Recommended fix: Update architecture, scope and feature claims after closure.

Test needed: Documentation-to-composition checklist.

## 46. Deferred Items

Only genuine environment/platform items: Fedora/Windows VM runtime/security/native acceptance; physical microphone and configured STT/TTS; configured cloud/MCP/plugin product smoke; native notifications/startup/deep-link visual acceptance. Optional future Custom skill-profile behavior may be deferred if wording truthful. A01–A11 current intended workflow defects are not deferred. Uninvestigated support-module composition stays UNKNOWN, not silently deferred.

## 47. Recommended Closure Batches

Batch A — P0/P1 Core Integrity: A01,A02,A09; establish safe descriptor lifetime, successful provider-neutral Agent contract and one shipping service authority; then real read/edit/allow/deny/cancel/final grounding. Batch B — P2 Feature Wiring / Reachability: A03–A08,A11,A15,A16; daemon-owned Inspector/Tasks/RAG/projections, exact model identity, private planner protocol and truthful extension/native UI. Batch C — P2/P3 Storage / Recovery / Diagnostics: A10,A12,A13,A17; safe-mode semantics, forward-schema guard, bounded/redacted logs and exit status. Batch D — Cleanup / Stale / Docs: A14,A18 and resolved reachability candidates; correct wizard/docs after actual behavior, preserve candidates until proven obsolete. No closure changes started.

## 48. Regression

| Gate | Fresh result |
| --- | --- |
| Configure/build | cmake tests preset PASS |
| C++ | 114/114 PASS, 68.90 seconds |
| Controller | 127 PASS |
| Shell | 74 PASS |
| Desktop IPC | 41 PASS |
| Native | 3 PASS (not visual acceptance) |
| Rust | 36 PASS: CLI14/IPC11/TUI11 |
| cargo fmt --check | PASS |
| cargo clippy --workspace --all-targets --all-features | PASS |
| cargo test --workspace | PASS |
| QML lint | exit0, 72 warnings |
| Generated contracts | both --check PASS |
| Branding | PASS,1129 files/69 hashes |
| clang-tidy | five-source scoped analysis exit0; no displayed diagnostics |
| Scheduler repeat | agent_loop until-fail:3 PASS |
| git diff --check | PASS exit0 on current working tree, including audit documents |

Initial sandbox-restricted suite/Cargo socket failures were environment PermissionDenied; accepted host reruns passed. Both attempts retained; not product regression failures. Tidy scope: UnifiedAudioService, DaemonIpcServer, DesktopRuntimeClient, QuickPanelController, ApplicationBootstrapper. Suppressed system-header diagnostics are not proof whole repo warning-free.

## 49. Commit Readiness

No commit/push. Audit artifacts only; existing extensive dirty working tree preserved. No product defects fixed, no code deletion. Verdict FIXES REQUIRED; green gates cannot override broken production composition and Agent workflow. Closure requires user instruction; stop after report.
