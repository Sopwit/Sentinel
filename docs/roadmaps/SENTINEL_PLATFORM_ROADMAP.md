# Sentinel Platform Roadmap — P0–P12

Planning baseline: `b152a960c11fe6e13b086666752ea5954005797b`; branch `feat/p4-desktop-live-integration`. Date: 2026-10-16. Status: **proposed execution contract; no phase declared complete by this document**. HEAD matches the baseline, but five Desktop dictation files changed in the shared working tree during preparation; see the reconciliation source-delta section. Their behavior is unvalidated and must be rebaselined for P6/P10 acceptance. Product decisions supplied by the maintainer are confirmed; implementation choices, staffing and numerical acceptance parameters below remain proposals until adoption. [Reconciliation](../audits/P0_2_ROADMAP_RECONCILIATION.md) retains findings; [backlog](GITHUB_ISSUE_BACKLOG.md) maps work. [P0.1](../audits/P0_1_CURRENT_STATE_AUDIT.md) remains unchanged.

## Product and architecture contract

Sentinel remains an independent cross-platform open-source project under `sentinel-projects`. Ro-ASD may bundle it, but models download only after user choice. Preserve C++20/Qt 6/QML, Desktop, Quick Panel, System Tray, Rust CLI/TUI and supported platform interfaces. Support Ollama, llama.cpp, LM Studio and extensible compatible providers. Keep System Assistant, Developer Agent and Personal Assistant in long-term scope; these are product use cases, not three runtimes.

AgentRuntime/AgentLoop remains execution authority; only accepted finals complete Agent runs. IModelRouter resolves one frozen ModelBinding and provider behavior stays behind IChatProvider. ToolDescriptor/registry and ToolExecutionGateway remain authoritative; IFileSystemService and ProcessExecutor own system boundaries. QML presents QML-safe state. Settings, memory, history, conversations, runs and grants remain separate, SQLite through Qt SQL. Permissions, sandbox, credentials and truthful receipts apply to every surface.

**Proposed execution terminology for P0 adoption:** Passive means user-requested conversation/inspection with no unsolicited action and no implicit mutation authority; Assisted means user-initiated work constrained by explicit policy/grants/approvals; Proactive means opt-in trigger-initiated work with bounded, revocable authority, introduced only in P9. These are product semantics to map to existing modes/policies, not assertions that identically named enums already exist. Passive and Assisted take precedence. Profile-based resource policies may adapt scheduling/context/model choice; fixed security/protocol bounds remain valid and cannot be relaxed by adaptation.

## Phase map and sequencing

| Phase | Outcome | Primary accountable role | Primary repository |
| --- | --- | --- | --- |
| P0 — Baseline, architecture and acceptance reconciliation | Establish an agreed execution contract without declaring unfinished features complete. | Maintainer/architecture | sentinel |
| P1 — Agent Reliability | Make Passive/Assisted tasks terminate truthfully and repeatably through the existing AgentRuntime. | Runtime | sentinel |
| P2 — Multi-Backend Architecture | Provide one capability-aware provider contract across local and extensible compatible endpoints. | Providers | sentinel |
| P3 — Model Lifecycle Management | Make acquisition, registration, ownership, loading and unloading explicit and recoverable. | Models | sentinel |
| P4 — Adaptive Resource Management | Adapt model and execution budgets to measured hardware and pressure. | Resources | sentinel |
| P5 — Daemon and IPC Hardening | Make one daemon authority durable, bounded and consistent across clients/platforms. | Daemon/IPC | sentinel |
| P6 — Fast Actions and Desktop UX | Make fast actions, Quick Panel, tray and terminal UX truthful, keyboard-accessible and responsive. | Desktop/terminal | sentinel |
| P7 — Ro-ASD Native Integration | Bundle Sentinel as an independent optional-model application with native Ro-ASD lifecycle integration. | Packaging/distro | sentinel-packaging + sentinel |
| P8 — Memory and Context | Provide predictable bounded context and explicit, separately owned personal memory. | Memory/context | sentinel |
| P9 — Automation | Add opt-in automation after reliable Passive/Assisted execution is accepted. | Runtime/automation | sentinel |
| P10 — Voice and Multimodal | Deliver consented, capability-aware audio and attachment workflows. | Audio/multimodal | sentinel |
| P11 — Extensions and Ecosystem | Stabilize extensions through existing permission, provider and tool contracts. | Extensions | sentinel |
| P12 — Release Acceptance | Approve an exact releasable candidate only after applicable correctness, security and distribution gates close. | Release maintainer | sentinel-packaging + sentinel |

P0–P12 are new roadmap IDs, **not** replacements for historical Phase 2/3, P4.1, SEC-P1 or TUI V3/V4 names. Work need not be serial. P0 supplies contracts; P1 establishes reliability; P2/P3/P5 may develop in parallel only against those contracts. P4 consumes lifecycle/capability data; P6 consumes reliable IPC/readiness; P7 requires platform/package gates; P8 feeds later automation; P9 remains later than reliable Passive/Assisted. P10/P11 can validate existing subsets earlier without declaring their whole phases complete. P12 closes only a declared release scope; excluded phases remain open, never relabeled accepted.

A dependency on a phase contract is different from requiring all phase implementation first. Before implementation, each actionable issue identifies the exact prerequisite artifact/issue; the backlog provides a noncyclic initial sequence. Scope changes require a decision record and regression impact review. No calendar dates or fabricated owners are assigned.

## Common acceptance and evidence rules

Every phase below inherits this Definition of Done: acceptance cases are agreed before execution; required automated and native/manual checks pass for the declared scope; failing/skipped/unavailable cases are recorded; no known finding closes without specific evidence; source/contract/docs agree; permissions and separate owners are preserved. A class, interface, fixture or document alone cannot close a phase. One passing rerun does not erase a flake. No new live/native results are claimed in this task.

P0 must adopt a bounded matrix of model/backend versions, hardware/OS/desktop/terminal profiles, repetitions, task classes, context sizes and per-profile latency/memory/cancellation deadlines. Numeric values not yet agreed are **TBD and block the affected acceptance**, not implicitly passed. Proposed starting live corpus: read-only repository inspection, single-file repair, multi-file dependency task, missing-capability failure, provider disconnect, denial, repeated-action recovery and cancellation during each execution stage; three attempts per successful task/backend/profile initially, with every attempt recorded. Maintainer may refine this before runs, never after results merely to make them pass. Functional/security cases require zero false-success, unauthorized side effects or late-event resurrection. Hard termination guarantees require an actual supervised process boundary.

Evidence records contain candidate SHA/dirty diff, app/protocol/runtime/model/asset identities, hardware/platform/profile, case and expected outcome, all repetitions including failures, measured budgets, independent artifact verifier, policy/grants, cancellation/approval trace, automated logs, native checklist and reviewer disposition. Use synthetic/redacted data; keep sensitive raw evidence private with retention/hash references. Legacy P0.1's 127 CTest targets and 130 Rust tests are prior observations only, not P0.2 reruns or release certification.

## P0 — Baseline, architecture and acceptance reconciliation

### Goal

Establish an agreed execution contract without declaring unfinished features complete.

### Current baseline

P0.1 records implementation and fixture evidence at the unchanged baseline; historical roadmap is broad direction, not numbered acceptance.

### Technical scope

Baseline identity, authority/ownership map, mode definitions, native/model test matrix, finding ledger and bounded backlog.

### Existing components to reuse

AGENTS.md; P0.1; protocol/ipc-v1.json; architecture/security/release docs; existing test and live-agent helpers.

### Required changes

Adopt these documents; resolve decision owners; prepare narrow stale-document corrections; define C++ CLI disposition and validation parameters.

### Dependencies

None for drafting; maintainer adoption and target/version/hardware decisions for closure.

### Functional acceptance criteria

1. Every P0.1 finding has a phase, accountable role and closure evidence; none disappears through renumbering.
2. Each phase contains all twelve contract fields and each backlog item a repository and blocking relationship.
3. Maintainer records target release scope, reference hardware/providers, Passive/Assisted meaning, and C++ CLI disposition, or explicitly leaves P0 open.

### Automated test requirements

Check local links/anchors, phase and finding coverage, dependency cycles, SHA/source diff and historical-document hashes; no runtime suite required for this documentation change.

### Native/manual verification requirements

Maintainer reviews the plan and assigns real owners; no platform or live-model validation is claimed.

### Risks

Unassigned roles, unknown Ro-ASD packaging contract, or arbitrary thresholds can turn planning into false acceptance.

### Completion evidence

Adoption record, decision log, complete ledger and documentation-check receipt, tied to baseline SHA.

### Out-of-scope items

Feature implementation, source refactors, GitHub creation, changing historical reports or existing gates.

## P1 — Agent Reliability

### Goal

Make Passive/Assisted tasks terminate truthfully and repeatably through the existing AgentRuntime.

### Current baseline

AgentLoop has evidence/final gates, cancellation, iteration/repetition checks and fixture coverage; real-model reliability remains unverified; Rust IPC handshake failed once before a passing rerun.

### Technical scope

Classification, bounded planning/recovery, tool observations, grounded finals, approval/denial, cancellation, provider failure, sync/async parity and client terminal identity.

### Existing components to reuse

AgentRuntime, AgentLoop, LlmAgentRuntime, ObservationPolicy, ClaimGroundingResolver, AgentExecutionScheduler, ToolExecutionGateway, tools/agent/live_coding.py.

### Required changes

Reproduce handshake failure; build a bounded acceptance corpus; fix observed reliability defects; correct legacy CLI exit semantics; keep approval and final gates intact.

### Dependencies

P0 adopted matrix/authority decisions; existing usable model and sandbox for live tests. P2/P3 behavior changes require rerunning affected cases, not delaying all P1 work.

### Functional acceptance criteria

1. For every agreed case/repetition, an independent verifier and expected terminal state agree; no tool result, refusal, timeout or cancellation is labeled completed.
2. Successful task cases require accepted grounded finals plus independently verified artifacts; expected failure/denial/cancel cases require their specified non-success terminal states.
3. Cancel during classification/provider/tool/approval closes the run once, suppresses late events, revokes pending approvals and permits a subsequent clean task.
4. Malformed arguments, repeated ineffective actions and unavailable capabilities terminate or recover within the predeclared iteration/deadline budgets.
5. Rust handshake stress regression has zero unexplained failures across 100 repetitions per supported Unix CI OS; investigation records whether fixture-only or production behavior is affected.

### Automated test requirements

Agent loop/runtime/gateway/controlled-task tests; deterministic delayed/stale/denied/failure fixtures; scoped handshake stress; legacy CLI subprocess exit tests with no real downloads; full CMake/CTest and Rust fmt/clippy/test for shared changes.

### Native/manual verification requirements

Run the adopted synthetic corpus through actual local models, including one coding and one read-only case per selected backend/hardware profile. Record all attempts. Exercise Assisted approvals/cancel in Desktop and TUI; do not use personal profiles.

### Risks

Overfitting one model, loosening grounding, flaky timing, or testing only successful tasks; non-cooperative in-process code has no hard kill guarantee.

### Completion evidence

Exact candidate/model identifier and digest when available, provider/runtime versions, hardware, mode, policy, case inputs, all attempt outcomes, timings, redacted traces and independent verifier results.

### Out-of-scope items

Proactive automation, new planner/router, broad controller refactor, universal model support or release clearance.

## P2 — Multi-Backend Architecture

### Goal

Provide one capability-aware provider contract across local and extensible compatible endpoints.

### Current baseline

ModelService registers Ollama, llama.cpp/LM Studio/compatible and cloud adapters; static metadata/catalogs coexist; live adapter parity is unverified.

### Technical scope

Binding, capabilities, discovery/readiness, streaming, tool calls, failures, credentials and network policy.

### Existing components to reuse

IChatProvider, IModelRouter, ModelBinding, ModelService, ProviderRequestRuntime, ProviderMessageNormalizer, ProviderErrorClassifier.

### Required changes

Define backend capability/error matrix; remove ambiguous provider-specific presentation; distinguish selected/installed/reachable/authorized/ready; document metadata-only helpers.

### Dependencies

P0 authority/capability contract; P1 terminal semantics; Security and Privacy credential/network gates.

### Functional acceptance criteria

1. Selecting backend B affects future requests only; an active request remains bound to A until terminal state.
2. Missing credentials, unhealthy endpoint, absent model or unsupported tools/vision produce distinct truthful readiness/failure; no silent cloud or local fallback.
3. Ollama, llama.cpp and LM Studio each pass declared discovery, chat, streaming/cancel and supported Agent/tool cases; compatible/cloud adapters pass their advertised subset.
4. A credential-presence/readiness query never returns raw secrets or triggers unrequested billable inference.

### Automated test requirements

Binding immutability, request normalization and tool schema tests per adapter; mock HTTP stream/timeout/auth errors; generated protocol checks; shared full regression suite.

### Native/manual verification requirements

Execute the declared subset against each supported local server; credentialed cloud acceptance only in an authorized environment; unavailable environments remain unverified.

### Risks

Provider capability guesses, model-name collisions, incompatible tool-call formats and metadata mistaken for live health.

### Completion evidence

Versioned backend matrix with supported/unsupported cases, wire fixtures and redacted real endpoint results.

### Out-of-scope items

Provider-specific second routers, universal feature parity, Copilot support inferred from configuration helper, new commercial accounts.

## P3 — Model Lifecycle Management

### Goal

Make acquisition, registration, ownership, loading and unloading explicit and recoverable.

### Current baseline

ModelOperationService/StorageManager implement acquisition and ownership records; LocalModelRuntime starts selected GGUF; external provider lifecycle differs and retries/health identity are incomplete.

### Technical scope

Consent, discovery, disk/capability preflight, download integrity, interrupted operations, register/load/unload/remove and external runtime ownership.

### Existing components to reuse

ModelService, ModelLibraryService, ModelOperationService, ModelStorageManager, HuggingFaceModelSource, LocalModelRuntime, ProcessExecutor.

### Required changes

Define lifecycle state/ownership transitions and idempotent requests; bind health to model identity; bounded retries/backoff; safe cancellation and removal; provider-specific capability disclosure.

### Dependencies

P2 provider contract; P1 active binding/cancel; P5 process/IPC ownership for restart acceptance; P4 can later supply adaptive admission.

### Functional acceptance criteria

1. Fresh installation or Ro-ASD bundle downloads no model until an explicit user choice identifies artifact, source, size/unknown size and target runtime.
2. Cancel/crash/restart of acquisition leaves truthful interrupted state; corrupt/partial artifacts never become installed-ready.
3. Selection, registration, file existence, server health and loaded model identity are separately observable.
4. Unloading/deleting a model in use refuses or follows a documented drain path; external provider processes/files are never killed/deleted as daemon-owned.
5. Unsupported artifact formats remain acquired-only and cannot be represented as runnable GGUF.

### Automated test requirements

Download interruption/hash/header/disk fixtures, ownership and sidecar deletion tests, model switch/busy state and health-alias mismatch, recovery journal fault injection.

### Native/manual verification requirements

Explicitly authorized download/load/unload tests on each provider with disposable artifacts and user-selected models; no such tests run in this planning task.

### Risks

Data loss, orphaned processes, health endpoint identity mismatch, gated license/download rights and misleading fit estimates.

### Completion evidence

Transition matrix, ownership record, before/after artifact/process inventory and interruption/cancel/restart results.

### Out-of-scope items

Bundled model weights by default, arbitrary external model deletion, full multi-file inference engines without a compatible provider.

## P4 — Adaptive Resource Management

### Goal

Adapt model and execution budgets to measured hardware and pressure.

### Current baseline

Hardware facts, disk preflight and some Ollama VRAM telemetry exist; scheduler concurrency and llama.cpp context/parallel parameters are fixed.

### Technical scope

CPU/RAM/GPU telemetry capability, freshness, admission, scheduling, context/concurrency adjustment, cooldown and user override.

### Existing components to reuse

HardwareCapabilityService, ModelLibrary fit, ModelStorageManager, AgentExecutionScheduler, ModelService and LocalModelRuntime.

### Required changes

Add platform sampler adapters and policy inputs to existing services; expose missing/stale measurements; derive profile-specific budgets and safe fallback; preserve cancellation priority.

### Dependencies

P2 capability and P3 lifecycle contracts; P1 bounded execution; Quality and Performance reference profiles.

### Functional acceptance criteria

1. Low-memory, high-memory and missing-GPU profiles produce distinct justified budgets from measured facts rather than one universal RAM/CPU cutoff.
2. Injected pressure reduces admissions/concurrency or proposes model/context adjustment without dropping an active binding or bypassing permissions.
3. Unavailable/stale telemetry is labeled unknown and uses a documented conservative fallback; zero is not fabricated.
4. Recovery uses hysteresis/cooldown so fluctuating pressure does not repeatedly load/unload; user overrides remain bounded and visible.
5. Per-profile memory, responsiveness and cancel budgets agreed before runs pass under normal and induced pressure; existing security input/frame bounds remain enforced.

### Automated test requirements

Deterministic fake samplers across profiles, missing metrics, pressure/recovery/hysteresis, override limits and scheduler resource-conflict regression tests.

### Native/manual verification requirements

Measure native low/high-memory and integrated/discrete/absent-GPU profiles where available; record sampling overhead, responsiveness and peaks; unavailable profiles stay open.

### Risks

Incorrect available-memory estimates, oscillation, expensive sampling and confusing policy thresholds with security limits.

### Completion evidence

Hardware/profile definitions, sampled series, policy decisions and per-profile budget measurements.

### Out-of-scope items

Universal rigid performance caps, a second scheduler/runtime, overclocking or OS process-priority changes without policy.

## P5 — Daemon and IPC Hardening

### Goal

Make one daemon authority durable, bounded and consistent across clients/platforms.

### Current baseline

Unix owner-checked IPC, generated contracts, snapshots and drain exist; Rust Windows transport missing; history/replay limited; shutdown remains cooperative.

### Technical scope

Windows named pipes, trust/identity, startup/profile ownership, reconnect/deadlines, backpressure, interrupted-run recovery and supervisor semantics.

### Existing components to reuse

DaemonService, DaemonIpcServer, DaemonClient, DesktopRuntimeClient, sentinel-ipc, protocol/ipc-v1.json and existing stores.

### Required changes

Implement Windows transport against the same contract; specify launch ownership and crash semantics; fix measured transport defects; define bounded history retrieval and non-cooperative provider strategy.

### Dependencies

P0 authority and CLI disposition; P1 terminal identity; P2/P3 provider/process lifetime; Security and Privacy platform trust model.

### Functional acceptance criteria

1. Windows CLI/TUI handshakes, attaches, runs, cancels and handles approval through the same daemon contract; foreign-user clients are rejected under the adopted pipe trust policy.
2. Desktop auto-launch and service-manager launch race to one endpoint owner; no profile has two runtime writers.
3. Disconnect/reconnect never replays a sent mutation; restart marks interrupted runs failed and invalidates approvals rather than resuming them silently.
4. Malformed/oversized frames and slow clients stay bounded and cannot change another run terminal state.
5. Shutdown rejects new work and keeps dependencies alive through drain; a non-cooperative provider reports Stopping until an explicitly supervised termination path exists.

### Automated test requirements

Unix and Windows native transport fixtures, lock/stale-endpoint races, identity/security tests, duplicate/late/generation events, absolute deadlines, output bounds, restart and drain faults.

### Native/manual verification requirements

Native Linux/macOS/Windows multi-client lifecycle and service startup/restart; Windows named-pipe permissions and slow/non-cooperative process cases.

### Risks

OS trust differences, lock/profile mismatch, replay, truncated history mistaken for complete, unsafe thread destruction.

### Completion evidence

Protocol compatibility matrix, native lifecycle traces, ownership inventories and security-negative results.

### Out-of-scope items

Remote unauthenticated daemon, implicit historical replay, unsafe thread kill or alternate client-side execution.

## P6 — Fast Actions and Desktop UX

### Goal

Make fast actions, Quick Panel, tray and terminal UX truthful, keyboard-accessible and responsive.

### Current baseline

Daemon-backed QML/Desktop/Quick Panel/tray and rich Rust TUI exist; Linux global shortcut and daemon clipboard unavailable; Linux app-launch descriptor unsupported; several TUI commands unsupported.

### Technical scope

Action availability/receipts, readiness, navigation, focus, tray lifecycle, KDE activation, clipboard service boundary and terminal command parity.

### Existing components to reuse

DesktopShellViewModel, DesktopRuntimeClient, QuickPanelController, NativeCompanionAdapter, QML views, ToolDescriptor/gateway and TUI command catalog.

### Required changes

Implement KDE/portal activation behind platform boundary; choose explicit authorized GUI clipboard brokerage or keep unavailable; design safe platform app launch; expose actual capability reasons; prioritize supported commands over decoration.

### Dependencies

P1 completion/approval, P2 readiness, P5 authoritative IPC; P7 validates Ro-ASD-specific integration; Security and Privacy approves any GUI brokerage.

### Functional acceptance criteria

1. Keyboard opens/dismisses Quick Panel on KDE/Wayland where supported; focus returns predictably; denied/unsupported registration shows a truthful reason.
2. Loading, empty, disconnected, provider-error, approval and terminal states are visible in main UI, Quick Panel and tray; reconnect does not enable stale actions.
3. Clipboard remains unavailable headless unless an explicitly consented GUI broker exists; broker loss/denial cannot report read/write success.
4. Linux app launch remains unavailable until supported policy enforcement exists; opener acknowledgement is never represented as verified application state.
5. Unsupported Plan/Compact/Undo/Redo/Skills/Init/Grill/ExternalEditor commands remain explicit; any enabled command has a daemon-authoritative contract and tests.
6. Agreed compact/wide layouts, keyboard-only focus and accessible names pass; dictation never auto-submits.

### Automated test requirements

QML loading/empty/error/focus/responsive tests, IPC stale projection, platform-action rejection/receipt tests and TUI command/terminal-size matrices.

### Native/manual verification requirements

Real keyboard/tray/shortcut/focus flows on Fedora KDE Wayland, macOS and Windows; physical Kitty/Sixel/text-fallback checks for advertised TUI support.

### Risks

GUI broker authority creep, unsupported platform claims, layout regressions and conflating connected with ready.

### Completion evidence

Per-platform flow checklist, recordings/redacted screenshots, keyboard/focus trace and authoritative action outcomes.

### Out-of-scope items

QML business logic, clipboard polling by default, a second runtime, unsupported command simulations or broad visual redesign.

## P7 — Ro-ASD Native Integration

### Goal

Bundle Sentinel as an independent optional-model application with native Ro-ASD lifecycle integration.

### Current baseline

Linux metadata, systemd/DBus/Flatpak assets exist; no Ro-ASD-specific native acceptance established. User confirms independent sentinel-projects identity and optional model acquisition.

### Technical scope

Packaging contracts, service identity, install/upgrade/uninstall, desktop activation, model-consent onboarding and distro support documentation.

### Existing components to reuse

Existing Linux packaging/systemd/DBus metadata, NativeCompanionAdapter, daemon/profile paths, ModelOperationService; no forked runtime.

### Required changes

Agree Ro-ASD ownership/version/support policy; reconcile packaging variants; package app separately from optional weights; validate service/policy defaults and cross-project dependencies.

### Dependencies

P3 no-silent-model-download, P5 service ownership, P6 KDE flows; Release Engineering and Security and Privacy gates apply before redistribution.

### Functional acceptance criteria

1. Ro-ASD may install Sentinel by default without downloading/loading a model or creating cloud credentials; user can decline all model acquisition.
2. The bundled application keeps sentinel-projects source/license/version identity and works independently of Ro-ASD.
3. Install/login/restart/upgrade yields one daemon/profile owner; uninstall does not silently erase user-owned conversations/models.
4. Native menu/tray/Quick Panel, permissions and offline behavior pass on the declared Ro-ASD/KDE image.
5. Packaging repository and core repository record a matching version/contract dependency.

### Automated test requirements

Package metadata/service-path validation, clean-image install/upgrade/uninstall scripts with synthetic profiles; model-download/network-negative checks.

### Native/manual verification requirements

Boot/login/upgrade/uninstall on an actual approved Ro-ASD image; record distro/desktop/Wayland version and clean-network observations.

### Risks

Unknown distro policy/maintainer, automatic downloads, repository identity drift and packaging rights blockers.

### Completion evidence

Image/package manifest, version/source correspondence, consent/no-download trace and native lifecycle record.

### Out-of-scope items

Project absorption into Ro-ASD, required bundled weights, distro-specific core logic or unapproved package publication.

## P8 — Memory and Context

### Goal

Provide predictable bounded context and explicit, separately owned personal memory.

### Current baseline

Separate Qt SQL stores and migrations exist; literal recall and opt-in semantic supplements coexist; token estimates approximate; candidate/todo state volatile.

### Technical scope

Token/oversize handling, retrieval relevance, consent/commit/delete/export, retention, schema migration and cross-session context.

### Existing components to reuse

ContextEngine, ContextAssembly, SemanticRetrieval, LocalRagStore, IMemoryStore, SQLite stores, CompactionService, PermissionPolicyService.

### Required changes

Define persistent versus volatile state; model-aware budget/oversize policy; evaluate retrieval; implement authoritative compaction exposure only when contract ready; establish retention/redaction controls.

### Dependencies

P2 capabilities, P3 embedding model lifecycle, P4 dynamic budget input, P5 bounded transcript contracts; privacy consent rules.

### Functional acceptance criteria

1. Oversized goals/attachments/history are rejected or explicitly reduced before provider overflow; no silent loss of critical instruction/permission state.
2. Memory commit requires explicit policy/consent; deleting/forgetting an item removes retrieval eligibility and follows documented backup retention.
3. Literal and semantic retrieval are separately labeled; semantic network/model calls occur only when enabled and authorized.
4. Migration handles older supported schemas and interruption without falsely reporting completion; settings/history/runs/grants remain separate.
5. Long-session and synthetic retrieval corpus meet predeclared per-profile relevance/budget criteria, including empty/no-match/error results.

### Automated test requirements

Oversize/token budget, long-session compaction, retrieval golden corpus, delete/retention, schema upgrade/rollback and interrupted-store fixtures.

### Native/manual verification requirements

Review long real-model conversations with synthetic data, memory opt-in/forget/restart and native export/delete flows.

### Risks

Privacy leakage, stale embeddings, approximate token counts, data loss and permissions summarized away.

### Completion evidence

Corpus metrics, budget traces, migration fixtures and consent/delete/retention records.

### Out-of-scope items

Unified database replacing separate owners, automatic private-memory ingestion or pretending volatile todos are durable.

## P9 — Automation

### Goal

Add opt-in automation after reliable Passive/Assisted execution is accepted.

### Current baseline

Background/reminder/alarm/controlled-task foundations exist; they do not establish a complete safe Proactive automation product.

### Technical scope

Schedule/trigger lifecycle, explicit bounded authority, disable/pause, deduplication, missed-run policy, notifications and audit.

### Existing components to reuse

AgentRuntime, ControlledTaskService, BackgroundJobService, ReminderService, AlarmStore, permission/grant stores and daemon.

### Required changes

Define automation records and permission expiry/revocation; route triggered tasks through existing runtime; make restart/missed-trigger and user-attention behavior explicit.

### Dependencies

P1 reliable terminal/deny/cancel, P4 admission, P5 durable lifecycle, P8 retention, Security and Privacy authority policy; P6 controls.

### Functional acceptance criteria

1. Passive/Assisted remains the default priority; no automation is enabled by installing, bundling, importing a skill or selecting a model.
2. Each automation displays trigger, scope, network/tool authority, expiry and disable control; revoked authority blocks the next action.
3. Duplicate/restarted triggers follow an explicit at-most-once side-effect or deduplication policy; missed runs do not replay uncontrolled work.
4. Pause/disable/cancel prevents subsequent dispatch; active work follows core cancellation semantics and has a truthful terminal record.
5. Notifications follow user preferences and redact private data.

### Automated test requirements

Fake-clock trigger/dedup/restart/expiry tests, grant revoke races, resource denial and exactly-once terminal projection; no real destructive actions.

### Native/manual verification requirements

Synthetic scheduled task, suspend/resume/reboot/timezone change and user disable/revoke workflows on supported native systems.

### Risks

Background authority escalation, repeated side effects, notification spam, clock changes and hidden cloud cost.

### Completion evidence

Trigger/authority schema, fake-clock traces, native restart/disable record and privacy review.

### Out-of-scope items

Always-on unrestricted Proactive agent, second execution engine or implicit permanent permissions.

## P10 — Voice and Multimodal

### Goal

Deliver consented, capability-aware audio and attachment workflows.

### Current baseline

Committed baseline has configured Whisper/Piper, PCM voice IPC, macOS dictation and text/PDF/DOCX/image attachments; working-tree native fallback/capability edits were observed but not tested. Real audio/vision/native acceptance incomplete; OCR absent.

### Technical scope

Capture/playback/transcription, model readiness, cancellation/leases, document/image limits and accurate feature availability.

### Existing components to reuse

UnifiedAudioService, WhisperTranscription, PiperTts, AudioFileSession, desktop audio/dictation, ChatAttachmentLoader, ModelService capabilities.

### Required changes

Complete native audio/vision acceptance; align generated-audio versus played-audio receipts; expose optional dependency/format limitations; evaluate OCR separately with consent and ownership.

### Dependencies

P2 capabilities, P3 optional speech/vision assets, P4 budgets, P5 bounded transfer, P6 review UX; privacy/OS permissions.

### Functional acceptance criteria

1. Microphone capture begins only after explicit user action/OS permission; cancel/disconnect/revision change releases the lease and rejects late transcripts.
2. Transcribed text is reviewed before sending; generated TTS audio and confirmed playback are separate outcomes.
3. Missing binaries/models/Pdf/unzip, scanned PDFs, unsupported vision or oversized files show actionable failure without fabricated extraction.
4. Declared STT/TTS/vision/document cases meet predeclared quality/latency budgets on selected profiles; model acquisition stays user-controlled.

### Automated test requirements

PCM bounds and lease ownership, late completion/cancel, malformed document/image, attachment limits, optional dependency and capability mismatch fixtures.

### Native/manual verification requirements

Actual microphone/speaker/OS denial on Linux/macOS/Windows; selected-model vision/text-document cases; no such validation claimed here.

### Risks

Private audio retention, OS permission differences, external parsers, inaccessible controls and file/decode resource exhaustion.

### Completion evidence

Redacted consent/lease/audio/attachment matrix, exact assets/providers and native quality/latency results.

### Out-of-scope items

Automatic listening/submission, bundled speech weights, general video understanding or OCR claimed from text extraction.

## P11 — Extensions and Ecosystem

### Goal

Stabilize extensions through existing permission, provider and tool contracts.

### Current baseline

Plugin host/SDK, MCP inventory/schema validation and Skills exist experimentally; WebUiService/ACP stubs are not product integrations.

### Technical scope

Compatibility/versioning, trust/permissions, host lifecycle, tool inventory, skill authority limits and bounded ecosystem documentation.

### Existing components to reuse

PluginManager, PluginHostSession, plugin SDK, McpService/McpToolProvider, SkillService, ExtensionService and existing registry/gateway.

### Required changes

Define supported extension contract; test capability brokerage/revocation/crash; document sample provenance; explicitly disposition WebUI/ACP and config-only Copilot helpers.

### Dependencies

P1 tool/final/cancel semantics, P2 provider extension contract, P5 host lifetime, P8 privacy and Security and Privacy review.

### Functional acceptance criteria

1. Malformed/colliding/unsupported schemas fail registration atomically; uninstall/disconnect removes model-visible tools and invalidates pending authority.
2. Extension requests cannot bypass gateway, grant scopes, filesystem/process boundaries or frozen model binding.
3. A crashing/hanging host has truthful failure/isolation behavior; credentials and tool results stay scoped to their owner.
4. Skills never become authorization grants; unsupported WebUI/ACP remain labeled stubs until an independently accepted bounded integration exists.
5. Versioned sample plugin and MCP fixture pass lifecycle/permission tests; real third-party compatibility is claimed only for tested versions.

### Automated test requirements

Plugin/MCP/skill schema, capability, collision, revoke, crash, cancellation and teardown tests; adversarial fixtures and API-version compatibility.

### Native/manual verification requirements

Install/enable/deny/revoke/disconnect/uninstall reviewed disposable extensions on each advertised platform; native isolation limitations recorded.

### Risks

Native-code trust, remote prompt injection, schema mismatch, dependency rights and accidental remote exposure.

### Completion evidence

Versioned compatibility/trust matrix, test plugin hashes, negative capability results and explicit stub disposition.

### Out-of-scope items

Unrestricted marketplace, treating MCP declarations as truth, hidden service listeners or new tool execution systems.

## P12 — Release Acceptance

### Goal

Approve an exact releasable candidate only after applicable correctness, security and distribution gates close.

### Current baseline

CI/build/package/signing/draft workflows exist; P0.1 fixture results are historical; SEC-P1 B01–B14 remain open pending revalidation/disposition.

### Technical scope

Candidate scope, native platform matrix, dependencies/licenses/source/art/privacy, package lifecycle, signatures/checksums and maintainer publication approval.

### Existing components to reuse

Existing CMake/Cargo/CI, CPack/signing/release QA, release process and SEC-P1 register; sentinel-packaging distribution work.

### Required changes

Create exact candidate evidence manifest; remediate applicable blockers; verify corresponding source and assets; complete native lifecycle/terminal tests; separate signing from publication.

### Dependencies

All in-scope phase acceptance and cross-cutting signoffs; P5 Windows transport before Windows terminal claim; P7 acceptance before Ro-ASD distribution claim.

### Functional acceptance criteria

1. Every shipped capability has accepted evidence for the exact candidate/platform; deferred phases stay open and are absent/disabled with explicit release notes.
2. Applicable B01–B14 findings have verified remediation or an existing-gate-compatible maintainer disposition; no undocumented waiver or green-CI closure.
3. Package/version/binary/asset/source/notice manifests agree; checksum verification and corresponding-source retrieval succeed.
4. Install/upgrade/uninstall, native GUI/terminal/signals, OS permissions and model-no-download defaults pass per target.
5. Signing/notarization status is independently verified or follows the existing explicit maintainer-approved target policy; ad-hoc signing is never mislabeled.
6. Maintainer separately authorizes publication; draft/tag/build success alone is insufficient.

### Automated test requirements

Exact-candidate CMake/Cargo checks, packaging metadata/QA, checksums, binary/resource identity, privacy and dependency scans, source-delivery and clean-install tests.

### Native/manual verification requirements

Physical target installers and terminals; Fedora/KDE/Wayland, macOS and Windows as advertised; Ro-ASD image if bundled; native signatures/notarization.

### Risks

Unresolved rights/privacy/advisories, historical evidence reuse, CI credential exposure and source/binary mismatch.

### Completion evidence

Signed-off candidate manifest, per-target acceptance, B01–B14 closure evidence, source delivery receipt and separate publication decision.

### Out-of-scope items

Automatic publishing, unapproved tag/protection changes, historical rewrite or lowering release gates to ship unfinished work.

## Cross-cutting workstreams

| Workstream | Owner role and repositories | Required scope | Acceptance and evidence | Blocking relationship |
| --- | --- | --- | --- | --- |
| Security and Privacy (SP) | Maintainer appoints security/privacy lead; sentinel, with distribution evidence in sentinel-packaging | Descriptor/resource authorization, platform sandbox limits, credentials, IPC user trust, extension isolation, consent/retention and SEC-P1 B01–B14 triage | Negative tests reject unauthorized operations; no raw secrets in public projections/evidence; scoped revocation/cancel works; dependency/privacy/native isolation receipts reviewed. Legal/admin decisions remain separately assigned. | Initial policy review precedes capability expansion; phase changes require scoped security review; final release signoff blocks P12. |
| Quality and Performance (QP) | Maintainer appoints validation lead; sentinel, native package measurements coordinated with sentinel-packaging | Deterministic regression, flake investigation, sync/async parity, accessibility, reference profiles, startup/render/resource/cancel budgets | Reproducible automated suites plus native matrix; per-profile budgets agreed before measurement; flaky/failing/skipped outcomes retained; declared Rust minimum tested in CI | Test matrix enables P1; resource/UX/native gates block their respective phase closure and P12. |
| Release Engineering (RE) | Release maintainer + packaging lead; sentinel-packaging for distribution, sentinel for core build/CI/dependency fixes | Exact version/artifact/source identity, reproducible package inputs, notices/licenses/rights, signing, clean lifecycle, admin protections and human publication | Exact candidate manifests/checksums, tested source retrieval, native installer/terminal evidence, unresolved-blocker register and separate maintainer publication approval | Planning starts at P0; actual redistribution waits for applicable B01–B14 closures and P12. |

Owner roles are proposed responsibilities, not existing teams or automatic GitHub assignees. Current sentinel CODEOWNERS names @Sopwit; maintainer must confirm capacity and real assignees. No legal conclusion or current remote security state is inferred from historical reports.

## Outstanding adoption decisions

Record decisions with owner, deadline/milestone, alternatives, affected acceptance IDs and evidence; until resolved, dependent criteria stay blocked:

- Exact release branch/scope, platform/architecture tiers, Ro-ASD image/version/support owner and whether sentinel-packaging exists/is writable under the organization.
- Named human owners/reviewers; one-maintainer review/recovery policy without inventing another reviewer.
- C++ CLI compatibility consumers and migration versus bounded diagnostic retention/deprecation path; no second task runtime either way.
- Reference hardware/model/backend/terminal matrix and per-profile acceptance budgets, plus meaning/mapping of Passive/Assisted modes.
- KDE activation backend, GUI clipboard broker trust/lifetime, Linux application-launch policy, Windows IPC user trust and sandbox-supported scope.
- Provider/model process ownership and hard-stop guarantees; resource telemetry fallback and user override policy.
- Public dependency/source/asset/privacy/admin blocker dispositions and exact corresponding-source/signing mechanism.
- Which unsupported TUI operations and optional OCR/WebUI/ACP/extension capabilities enter a release; excluded functionality remains explicit and open.
