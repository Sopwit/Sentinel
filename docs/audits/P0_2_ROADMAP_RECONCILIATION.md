# P0.2 — Roadmap Reconciliation & Acceptance Criteria

Date: 2026-10-16. Requested/local baseline: **`b152a960c11fe6e13b086666752ea5954005797b`**. Branch: `feat/p4-desktop-live-integration`. Scope: documentation and planning only. [Platform roadmap](../roadmaps/SENTINEL_PLATFORM_ROADMAP.md) defines phase acceptance; [GitHub backlog](../roadmaps/GITHUB_ISSUE_BACKLOG.md) prepares issues without creating them. [P0.1 audit](P0_1_CURRENT_STATE_AUDIT.md) is preserved.

## Baseline and evidence boundary

`git rev-parse HEAD` matches the requested SHA. At entry, `git diff --name-only <baseline>` and `git diff --cached --name-only` were empty. Initial `git status --short` showed only the prior untracked `docs/audits/` containing P0.1. **During documentation preparation, five source files became modified in the shared working tree.** These edits were not made by this documentation task and were preserved. Therefore HEAD matches but the final working tree is not identical to the audit baseline. No remote fetch or current GitHub settings/CI query was performed. The P0.1 report remains uncommitted and must be retained alongside these new documents in any later approved commit.

P0.1 SHA-256 before this task: `97ac34d20409a2b180faf02c5a31f8351a19092bcbb1e9186cb2085d7c14b274`. The final preservation check must match. `brain` remains unavailable on PATH; no Personal-Brain brief/context was retrieved. The explicit supplied checkout was verified, not guessed.

Inputs read: AGENTS.md, README.md, docs/ROADMAP.md, P0.1 in bounded sections, architecture, Agent Runtime, building/testing, daemon, IPC, security model/permissions/sandbox/secrets, release process, consolidated SEC-P1 blocker register, CODEOWNERS/PR template and tools/agent/README.md. Selected source anchors were rechecked for Desktop daemon launch, Rust approval/Windows transport/handshake fixture, Linux shortcut, legacy exit behavior, fixed resource settings and stub services. Other subsystem statements inherit P0.1 source evidence at the recorded SHA; they are not independent new live verification. The affected Desktop voice behavior is explicitly qualified by the working-tree delta below.

P0.1's 127/127 CTest targets and 130/130 Rust rerun tests remain historical observations, with an earlier Rust handshake failure retained. No build, model, microphone, native UI or platform acceptance was run in P0.2. “Implemented” means source exists, not whole-feature acceptance. Proposed changes/targets below are future work; no finding is automatically closed by planning.

## Working-tree source delta requiring revalidation

Read-only diff snapshots during final checks first showed **31 insertions / 3 deletions**, then **34 insertions / 3 deletions across the same five files**, with HEAD still at the baseline and no staged changes. This confirms active working-tree drift during this documentation task:

- `apps/sentinel-desktop/include/sentinel/desktop/DesktopShellViewModel.h`: adds local STT readiness helper and native fallback state.
- `apps/sentinel-desktop/include/sentinel/desktop/SystemDictationService.h`: adds native backend supported() capability query.
- `apps/sentinel-desktop/src/DesktopShellViewModel.cpp`: local STT availability checks, conditional native dictation fallback when start is unavailable, and stop routing to the chosen backend.
- `apps/sentinel-desktop/src/MacSystemDictationService.mm`: supported() returns true.
- `apps/sentinel-desktop/src/SystemDictationService.cpp`: supported() returns false for the non-macOS implementation.

This is observed source intent only: no compile, native microphone/permissions, cancellation, fallback privacy or model acceptance was verified for these changes. P0.1 describes committed behavior, not this modified voice path. P6/P10 and Security and Privacy must revalidate backend selection, explicit user/OS consent, start/stop/cancel ownership and late-result handling against the eventual adopted candidate. The diff does not establish a duplicate AgentRuntime/router/gateway or change the roadmap invariants. P0-01 must record the actual candidate SHA plus any accepted dirty diff before implementation/validation. Counts describe this inspection snapshot and are not a claim that concurrent writers cannot make later changes.

## Confirmed decisions and architectural reconciliation

The user confirms independent sentinel-projects open-source ownership; optional Ro-ASD bundling without automatic model downloads; C++20/Qt6/QML; Desktop/Quick Panel/tray/Rust terminal interfaces; Ollama/llama.cpp/LM Studio/extensible providers; Passive/Assisted before Proactive; hardware-adaptive resource policy; System Assistant/Developer Agent/Personal Assistant use cases; reuse of existing services; strict permissions/isolation/truthful completion.

The execution contract preserves one AgentRuntime/AgentLoop, IModelRouter/ModelBinding and ToolExecutionGateway. Daemon owns production execution for Desktop/Rust clients; QML stays presentation-only. Legacy C++ CLI remains a documented in-process exception awaiting a compatibility/disposition decision. LocalModelRuntime is a provider process helper, not a new router. StaticTaskPlanner, AgentTaskRuntime metadata and WebUI/ACP stubs must not be marketed as execution capability. Settings, memory, conversations/history, runs and grants remain separate owners through existing interfaces/Qt SQL.

P0–P12 names are now explicitly supplied target roadmap IDs. P0.1 priority tags were severity/order suggestions, not phase assignments. Historical Phase 2/3/P4.1/TUI/SEC-P1 names and records remain untouched. The platform roadmap is the proposed acceptance authority after maintainer adoption; docs/ROADMAP.md remains the older broad directional overview until a narrow follow-up links/reconciles it.

## Subsystem reconciliation

Source paths below are repository-relative; the P0.1 sections supply fuller symbol/test evidence. “Missing” means absent or unaccepted within that audit scope, not proof no related helper exists anywhere. Dependencies refer to the new roadmap contracts.

| Subsystem | Current verified source / partial state | Missing functionality / known defects | Existing files/interfaces to reuse | Dependencies | Recommended work / owner phase |
| --- | --- | --- | --- | --- | --- |
| Shared Core | C++20 Qt modular monolith; shared static core; metadata services mixed with live services | Large controller/composition classes and overlapping catalogs; no acceptance from class counts | core/src/app/ApplicationController.cpp; ApplicationControllerBuilder; core/CMakeLists.txt | P0 authority map; regression harness | Bound ownership/debt and refactor only after evidence; P0/P2/P5, maintainer/runtime |
| Agent Runtime | Multi-step/final/evidence/approval/cancel/repetition paths implemented; fixture coverage | Real-model completion unverified; sync/async parity and cooperative cancellation limits | core/src/agent/AgentRuntime.cpp; AgentLoop.cpp; LlmAgentRuntime.cpp; ObservationPolicy.cpp | P0 corpus; usable provider/sandbox | P1 reliable Passive/Assisted acceptance, runtime lead |
| Tool Registry/Gateway | 38 built-ins, one disabled internal; dynamic MCP/plugin inventories; policy/argument/resource gates | Uneven wrapper/receipt tests; available definitions differ from usable tools | core/src/runtime/BuiltInToolProvider.cpp; ToolExecutionGateway.cpp; IToolRegistry; IFileSystemService; ProcessExecutor | P1 terminal/final rules; SP | P1 negative/receipt tests, P6 platform availability, P11 extensions |
| Providers | Local and compatible/cloud adapters registered; immutable binding infrastructure | Capability/readiness parity unverified; static catalog duplication/provider wording | core/src/model/ModelService.cpp; IChatProvider; IModelRouter; ModelBinding | P1 failure semantics; credentials/network policy | P2 backend matrix, providers lead |
| Model lifecycle | Acquisition/import/registration, sidecars/journal and selected llama process helper | Uniform load/unload/ownership/retry incomplete; health not model identity; consent acceptance missing | core/src/model/ModelOperationService.cpp; ModelStorageManager.cpp; apps/sentinel-daemon/service/LocalModelRuntime.cpp | P2 capability; P5 ownership; P4 admission later | P3 explicit lifecycle/consent, models lead |
| Resources | Hardware facts/disk checks/model fit and some provider VRAM telemetry | No accepted continuous pressure policy; fixed scheduler 1–4 and llama ctx 8192/parallel 1 | HardwareCapabilityService.cpp; ModelLibrary.cpp; AgentExecutionScheduler.cpp; LocalModelRuntime.cpp | P2/P3 contracts; QP profiles | P4 adaptive sampler/policy, resources lead |
| Daemon/IPC | Owner-checked Unix IPC, generated contracts, bounded snapshots, recovery projections and drain | Rust Windows pipes missing; transcript/history limits; non-cooperative drain; handshake flake | apps/sentinel-daemon/service/DaemonIpcServer.cpp; DaemonService.cpp; protocol/ipc-v1.json; sentinel-ipc | P1 identity/cancel; SP; P0 profiles | P1 scoped flake, P5 platform/lifecycle hardening |
| Desktop/Quick Panel | IPC client and shared runtime projection; Qt/QML views and QuickPanelController | Linux global shortcut unavailable; readiness labels/native keyboard states not fully accepted | ApplicationBootstrapper.cpp; DesktopRuntimeClient.cpp; NativeCompanionAdapter.cpp; ui/qml/components/dialogs/TrayCompanionWindow.qml | P1/P2/P5 | P6 truthful activation/actions/focus; Desktop lead |
| System Tray | QSystemTrayIcon, native adapter and background lifecycle code | Actual delivery/activation/tray absence/platform behavior unverified | apps/sentinel-desktop/src/NativeCompanionAdapter.cpp | P5 startup ownership; P6 focus | P6 native lifecycle; P7 distro validation |
| Rust CLI/TUI | Daemon-only Unix client, explicit Chat/Agent, one-use approvals/cancel/reconnect; Core Warden presentation | Windows transport; unsupported plan/compact/undo/redo/skills/init/grill/external editor; flake | cli/crates/sentinel-cli/src/main.rs; sentinel-ipc/src/lib.rs; sentinel-tui/src/lib.rs/commands.rs | P1/P5; backend authoritative contracts | P1 transport/terminal reliability; P6 parity; P8/P11 for supported additions |
| Legacy C++ CLI | In-process chat/status/config/model list/pull diagnostic surface | Timeout/pull failure can exit 0; no daemon authority equivalence; disposition undecided | apps/sentinel-cli/main.cpp; commands/ChatCommand.cpp; ModelCommand.cpp | P0 compatibility decision; P1 failure contracts | P1 correct failure exits; P5 implement adopted migration/deprecation/retention plan |
| Memory/context | Separate Qt SQL stores/migrations; literal recall and opt-in semantic paths | Approximate budget can exceed model window; volatile candidates/todos; long-session/retrieval acceptance incomplete | core/src/agent/ContextEngine.cpp; app/ContextAssembly.cpp; memory/SemanticRetrieval.cpp; SQLite stores | P2 model context; P4 budgets; P5 history | P8 privacy/context/migration acceptance |
| Voice/multimodal | Baseline configured Whisper/Piper, PCM leases, macOS dictation, text/PDF/DOCX/images; working-tree dictation fallback edits observed but unvalidated | Real audio/vision/native acceptance; OCR absent; optional dependencies; ImageProcessor compress no-op | core/src/voice/UnifiedAudioService.cpp; WhisperTranscription.cpp; PiperTts.cpp; chat/ChatAttachmentLoader.cpp | P2/P3/P4/P5/P6; OS consent | P10 declared-scope acceptance, audio lead |
| Plugins/MCP/Skills | Experimental host/broker/schema/lifecycle/skill services | Native-code trust/cancel/compatibility; WebUiService/ACP stubs; configuration-only Copilot helper | PluginManager.cpp; PluginHostSession.cpp; McpToolProvider.cpp; SkillService.cpp; WebUiService.cpp; AcpService.cpp | P1/P2/P5/SP | P11 supported API/trust matrix; keep stubs explicit |
| Linux/KDE/Ro-ASD | Primary target; Linux systemd/DBus/Flatpak assets; service launches daemon | KDE shortcut absent; app-launch Unsupported; headless clipboard; Ro-ASD-specific acceptance absent | packaging/linux; NativeCompanionAdapter; BuiltInToolProvider | P3 consent; P5/P6; RE/SP | P6 platform action policy; P7 independent distro bundle, packaging lead |
| macOS/Windows | macOS native adapters/Keychain; Windows platform services/credential/process code | Native validation unclaimed; Windows IPC absent and sandbox partial; ARM64 experimental | core/src/platform; core/src/runtime/ProcessSandbox.cpp; desktop native adapters; sentinel-ipc | P5 transport/SP; P6/P10 | P5/P6/P10 native matrix; P12 target acceptance |
| Security/sandbox | Resource authorization/grants/credentials/gateway; Linux/macOS conditional sandbox; Windows partial | Not a universal isolation boundary; same-user IPC trust; no immutable security ledger certified | core/src/security; ProcessSandbox.cpp; SecureFileMutationBackend.cpp; SQLitePermissionGrantStore | Every capability phase | SP foundational and per-change review; P5 isolation guarantees |
| Testing/packaging/release | CMake/Cargo CI and package/signing/draft workflow sources | SEC-P1 B01–B14 unresolved; minimum-Rust/native/package/current-remote evidence incomplete | tests; .github/workflows; cmake/SentinelCPack.cmake; docs/release/RELEASE_PROCESS.md | QP/RE/SP + shipped phase acceptance | P12 exact-candidate gate; packaging-specific work in sentinel-packaging |

## Open finding ownership ledger

All findings below remain **OPEN**. Roles are proposed accountable responsibilities; @Sopwit is the current sentinel CODEOWNERS maintainer, not evidence that separate teams exist. Maintainer assigns actual humans during P0. Closure requires the listed phase evidence and a disposition linked to the original finding. Backlog keys are local planning identifiers, not real GitHub numbers.

| ID / retained finding | Primary phase / accountable role | Initial backlog route | Closure evidence / dependencies |
| --- | --- | --- | --- |
| F01 Real-model Agent acceptance gap | P1 / runtime | P1-01, P1-02, P1-03 | Adopted corpus, all attempts/independent verifiers, correct final/fail/deny/cancel states; P0 matrix |
| F02 Intermittent Rust IPC handshake Disconnected | P1 / terminal-IPC | P1-04 | Reproduction/root-cause or bounded residual-risk record, deterministic regression and stress on Unix; passing rerun alone insufficient |
| F03 Legacy in-process CLI + exit semantics | P0 decision, P1 exits, P5 authority / maintainer + CLI | P0-03, P1-05, EP-P5 | Consumer inventory and chosen disposition; nonzero failure/timeout tests; no second task runtime |
| F04 Windows Rust named pipes missing | P5 / IPC | EP-P5 | Native Windows contract/security/reconnect/task/approval/cancel tests |
| F05 Linux/KDE shortcut missing | P6 / Desktop-platform | EP-P6, EP-P7 | Actual KDE/Wayland keyboard activation/focus and denial/unsupported flow |
| F06 Headless daemon clipboard unavailable | P6 + SP / Desktop-security | EP-P6, WS-SP | Explicit broker or retained truthful unavailable decision; authorization/denial/disconnect tests; no GUI fallback execution |
| F07 Conditional/unsupported Linux application launching | P6 + SP / Desktop-platform | EP-P6, WS-SP | Enforceable supported launch policy and native receipt; no success from opener acknowledgement |
| F08 Provider lifecycle/resource policy partial | P3/P4 / models/resources | EP-P3, EP-P4 | Owned/external lifecycle transitions and profile-driven pressure admission tests |
| F09 Large controller/IPC compositions | P0 map, P2/P5 bounded cleanup / architecture | P0-03, EP-P2, EP-P5 | Ownership map and narrowly justified changes with behavior/contract parity; no mandatory rewrite |
| F10 WebUiService/AcpService stubs | P11 / extensions | EP-P11 | Explicit non-product disposition or separately accepted implementation; no class-based closure |
| F11 Unsupported Rust TUI commands | P6 with P8/P11 dependencies / terminal | EP-P6 | Per-command backend contract/tests/native behavior or explicit continued unavailable state; full phase scope cannot claim unsupported commands work |
| F12 Model readiness/ownership semantics | P2/P3 / providers/models | EP-P2, EP-P3 | Selected/installed/reachable/authorized/loaded distinctions, active binding invariance, health identity and owned-process tests |
| F13 Release security/dependency/assets/license/privacy blockers | P12 + SP/RE / release maintainer, security, rights owner/admin | P0-05, WS-SP, WS-RE, EP-P12 | Exact-candidate evidence for every applicable B01–B14 entry; no automatic closure |

### SEC-P1 blocker continuity

The [existing consolidated register](../development/SEC_P1_4_GITHUB_HARDENING_REPORT.md#5-consolidated-blocker-register) is retained without changing severity or claiming current remote/advisory revalidation. Assign remediation and decisions as follows:

| Existing IDs | Accountable decision/remediation roles | Repository and roadmap route |
| --- | --- | --- |
| B01/B02 lru memory/panic safety; B03 paste maintenance | Dependency maintainer + security reviewer; compatibility/physical terminal reviewer | sentinel dependency/code work; WS-SP/WS-QP, EP-P12 |
| B04 macOS notices/SDK; B05 Qt/source delivery; B14 third-party/toolchain provenance | Rights/source-delivery owner and release maintainer, legal advice when needed | sentinel source/dependency inventory; sentinel-packaging exact distribution notices/source mechanism; WS-RE, EP-P12 |
| B06 Core Warden rights; B07 branding provenance; B08 sound ownership | Actual rights holder/contributor and maintainer; no fabricated license attribution | sentinel asset provenance; sentinel-packaging redistribution inventory; WS-SP/RE, EP-P12 |
| B09 privacy/raw evidence; B10 tracked outputs/history exposure | Privacy/data owner + maintainer | sentinel bounded remediation; history rewrite/untracking requires separately authorized scope; WS-SP, EP-P12 |
| B11 native/binary-asset acceptance | Validation and release leads | sentinel runtime/terminal; sentinel-packaging candidate payload/native deployment; WS-QP/RE, EP-P12 |
| B12 protections/governance; B13 security intake/automation | Organization/repository administrator | sentinel governance decisions (not a packaging workaround); WS-SP/RE, EP-P12 |

A security issue body uses public-safe symptoms and restricted evidence pointers, not raw vulnerabilities, credentials or personal data. Distribution blockers apply to Ro-ASD bundling too. Documentation does not grant publication or destructive historical-remediation authority.

## Stale documentation versus current behavior

These are candidates for narrow follow-up P0-04; existing documents/history were not edited here.

| Document | Reconciliation |
| --- | --- |
| docs/ROADMAP.md | Describes CLI as a small dispatcher and daemon-facing interfaces as exploratory; distinguish retained C++ CLI from implemented Rust/daemon surfaces and link the new target contract after adoption. |
| docs/development/DAEMON.md | Stage A/B/C migration narrative and blanket no-client-auto-spawn claim are stale for production Desktop; preserve historical context, add current scope. Desktop ensureBackgroundDaemon launches detached daemon; Rust clients do not. |
| docs/development/ARCHITECTURE.md | Current prose correctly says no **in-process runtime fallback**; this does not conflict with detached daemon launch. Its diagram still draws Desktop directly into application services and should show client IPC. P0.1's broad wording suggesting this prose prohibits process launch is narrowed here without overwriting the audit. |
| docs/security/PERMISSIONS.md | Blanket CLI no-interactive-approval claim needs surface/version qualification: Rust TUI y/n sends approval.respond; that is one-use approval, not equivalent permanent-grant management. |
| docs/concepts/AGENT_RUNTIME.md | “Implemented” describes source; retain README's incomplete real-model acceptance statement alongside it. No blanket status promotion. |
| .github/pull_request_template.md / packaging metadata | Template release URL and some service documentation reference older project identities; inventory links/identity during P0-04/P7, verify canonical ownership before replacing. No repository transfer inferred. |
| Historical PHASE*, P4.1, SEC-P1, TUI V3/V4 reports | Keep their original dates/SHAs/outcomes. Link as evidence; never rewrite their pass/fail results into current roadmap completion. |

## Adoption and unresolved decisions

All ten supplied product decisions are incorporated. Remaining implementation decisions are listed in the roadmap's Outstanding adoption decisions: exact target/release scope and native/model/hardware matrix, real human owners, Ro-ASD image/packaging support and sentinel-packaging availability, C++ CLI disposition, profile budgets, mode mapping, KDE/clipboard/Linux launch policy, Windows pipe/sandbox trust scope, model process ownership/hard-stop guarantees, optional command/OCR/WebUI/ACP scope and release blocker/source/signing dispositions.

P0.2 delivers the proposed execution contract; it does **not** complete P0 adoption, P1 reliability, release clearance or any implementation phase. A decision that a capability stays unavailable is an honest scope decision, not a claim its missing implementation is finished. Parent epics remain open until their adopted scope criteria pass; release scope exclusions must be explicit.

## Documentation validation receipt

- Baseline HEAD/branch and initial clean source: verified. Final inspection found the five-file Desktop dictation delta above; staged diff remained empty. Affected voice claims require revalidation.
- P0.1 preservation: SHA-256 checked before/after against the recorded value.
- This task wrote only three new documents. P0.1 and existing tracked documentation remain unchanged; separately appearing source changes were preserved. No task-authored source/config edits, commits/pushes/GitHub objects or runtime setting changes.
- Structural check: P0–P12 exactly once as phase definitions, all twelve requested contract fields each; 13 retained findings, B01–B14 coverage, detailed P0/P1 candidates and later parent entries.
- Local Markdown link/heading targets, local backlog keys/dependencies and initial issue dependency graph checked; proposed external Project/repository availability not verified.
- Whitespace and focused personal-path scan checked; neither is a full privacy/security certification.
- No CMake/Cargo/native/live-model validation rerun for documentation-only work. P0.1 results are clearly labeled prior evidence.

Recommended next task: **P0-02 — adopt the isolated Agent acceptance matrix and profile budgets**, then **P1-04 — reproduce/investigate Rust IPC handshake flake** using temporary fixtures. Actual model/native runs wait for the adopted matrix and available user-selected resources; no model download is implied.
