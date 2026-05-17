# Architecture Decisions

## 1. Cross-platform Qt Desktop

Decision: Sentinel uses C++20, Qt 6, and QML for a cross-platform desktop application.

Reason: The product is desktop-first and should remain portable while allowing a Linux/Fedora KDE Plasma optimized experience.

Avoided:

- Electron.
- Browser-first desktop shell.
- Python backend for the app core.
- Linux-only core assumptions.

## 1.1 Platform Abstraction Direction

Decision: Keep platform-specific behavior behind explicit service interfaces.

Reason: Linux integrations may be richer, but the core architecture must remain portable to Windows and macOS.

Planned interfaces:

- `IPlatformService`
- `IPathProvider`
- `INotificationService`
- `ISystemIntegrationService`

Rule: platform services should not leak into storage contracts, provider contracts, controllers, or QML pages.

Phase 3.4 implementation status:

- Added `IPathProvider`.
- Added `IPlatformService`.
- Added `INotificationService` as a lightweight placeholder.
- Added `ISystemIntegrationService` as a lightweight placeholder.
- Kept these as boundaries only; no OS-specific integration/automation implementation yet.

## 2. Modular Monolith

Decision: Keep the repository as a modular monolith with clear internal boundaries.

Reason: The project is still in alpha. Interfaces give enough separation without the operational cost of separate services or packages.

## 3. C++ Core Owns Business Logic

Decision: Core behavior belongs in C++ classes and interfaces. QML owns presentation and user interaction.

Reason: This keeps behavior testable, deterministic, and independent from UI layout.

## 4. QML Boundary Through View Models

Decision: QML interacts with `DesktopShellViewModel` and QML-safe models.

Reason: Raw core objects should not leak into the UI layer.

## 5. Provider Isolation

Decision: Chat providers are hidden behind `IChatProvider`.

Reason: Real providers, local providers, status handling, and future configuration can evolve without changing UI code.

## 6. Memory Persistence Boundary

Decision: Memory storage is hidden behind `IMemoryStore`.

Reason: In-memory and SQLite implementations must share one small contract.

Storage decision:

- Desktop memory is stored at `QStandardPaths::AppDataLocation + "/memory.sqlite3"`.
- SQLite is accessed through Qt SQL and `QSQLITE`.
- Path ownership now routes through `IPathProvider` (`StandardPathProvider` by default).

Lifecycle decision:

- Memory maintenance includes a clear operation through the `IMemoryStore` boundary.
- UI receives generic maintenance status strings only.

## 7. Settings Persistence Boundary

Decision: Settings storage is hidden behind `ISettingsStore`.

Reason: Settings defaults, validation, and persistence can evolve without coupling QML to file IO.

Storage decision:

- Desktop settings are stored at `QStandardPaths::AppConfigLocation + "/settings.json"`.
- Path ownership now routes through `IPathProvider` (`StandardPathProvider` by default).

## 8. Settings And Memory Stay Separate

Decision: Settings and memory persistence use separate stores and separate files.

Reason: Settings are user/application configuration. Memory entries are runtime AI context data. Combining them would blur ownership and migration paths.

## 9. Chat History Persistence Boundary

Decision: Chat history persistence is hidden behind `IChatHistoryStore`.

Reason: Chat messages are ordered conversation records, not key-value memory. They need their own contract, schema, clear operation, and migration path.

Storage decision:

- Desktop chat history is stored at `QStandardPaths::AppDataLocation + "/chat_history.sqlite3"`.
- SQLite is accessed through Qt SQL and `QSQLITE`.
- Stored fields are `id`, `role`, `content`, `timestamp`, and `status`.
- Rows load in ascending `id` order.
- The schema metadata table stores `schema_version = 1`.

Runtime behavior:

- `ApplicationController` loads persisted messages when available.
- New runtime messages are appended to the chat history store when available.
- Clearing chat clears runtime and persistent chat history when available.
- If persistent chat storage is unavailable, runtime clear still succeeds with generic runtime-only status.
- Runtime chat continues if chat persistence is unavailable.
- QML receives only generic chat history status, such as `Available` or `Runtime Only`.
- QML receives only generic maintenance statuses, such as `Ready`, `Clear completed`, `Runtime Only`, or
  `Unavailable`.
- The desktop UI confirms before clearing local chat history.

Data ownership rule:

- Local data categories remain explicit and separate:
  - Settings (`ISettingsStore`)
  - Memory (`IMemoryStore`)
  - Chat history (`IChatHistoryStore`)
- Clear actions for memory/chat must not delete settings.

Current limitation:

- Chat history is a single local transcript.
- Multi-conversation/thread support is intentionally not implemented yet.
- Encryption, export, and pruning are intentionally not implemented yet.

## 10. AI Context Layer

Decision: Add repository-local AI instruction files in Phase 3.1.5.

Reason: Future AI coding sessions need a compact source of truth for phase status, architecture rules, and prompt guidance.

Expected effect:

- Smaller prompts.
- Less repeated context.
- Lower risk of architecture drift.
- Consistent constraints across Codex, Claude, ChatGPT, and similar agents.

## 11. UI/UX Roadmap Direction

Decision: Keep current UI as a foundation while planning a more mature Qt Quick experience.

Current state:

- Qt/QML shell.
- Chat, memory, settings, dashboard, bottom dock, header, and status bar.
- Minimal lifecycle UX around chat history.

Future direction:

- Smooth animations.
- Responsive layouts.
- Adaptive themes.
- Assistant-like chat interaction.
- Animated panels.
- Dashboard cards.
- Extensible component system.

Rule: future UI polish should stay behind QML/view-model boundaries and must not introduce business logic into QML.

## 12. Pre-agent Release Checkpoint

Decision: Add a stabilization checkpoint before Phase 4 implementation work.

Reason: The architecture needs an explicit audit gate so agent/tool runtime work does not begin on top of stale docs, inconsistent wording, or accidental boundary drift.

Checkpoint criteria:

- Core boundaries and platform boundaries remain explicit and interface-driven.
- QML exposure stays generic and does not leak SQLite/platform internals.
- Persistence separation remains strict across settings, memory, and chat history.
- Coverage exists for path ownership and maintenance status behavior.
- Release verification (`cmake --preset tests`, `cmake --build --preset tests`, `ctest --preset tests`) and formatting checks pass.

Out of scope during this checkpoint:

- Phase 4 agent/tool runtime implementation.
- Real provider/network integration and API keys.
- Plugin loading.
- Privileged automation.

## 13. Provider And Agent Runtime Separation

Decision: Keep provider and agent runtime boundaries separate.

Reason: Chat response generation and orchestration/tool planning are different concerns and should evolve independently without forcing cross-layer coupling.

Boundary rules:

- `IChatProvider` remains the chat generation contract.
- `IAgentRuntime` is the orchestration/runtime boundary for future action flows.
- Controllers and QML consume generic agent status/response surfaces, not runtime internals.
- Phase 4.0 runtime is local deterministic skeleton only (`NullAgentRuntime`) with no networking/tool execution.

## 14. Tool Descriptor And Registry Boundary

Decision: Separate tool metadata registration from tool execution.

Reason: The architecture needs a safe incremental path where tool identity, parameters, and risk metadata can be modeled and tested before any execution runtime is introduced.

Boundary rules:

- `ToolDescriptor` and related descriptor enums/structs model metadata only.
- `IToolRegistry` owns deterministic tool metadata registration and lookup.
- `InMemoryToolRegistry` is the default local deterministic registry implementation.
- Agent runtime may expose tool metadata from the registry, but must not execute tools in Phase 4.1.

## 15. Tool Invocation Planning Before Execution

Decision: Add a planning boundary before any tool execution runtime is introduced.

Reason: The app needs a safe intermediate layer where an agent can describe intended tool use in
structured data without gaining the ability to perform actions.

Boundary rules:

- Tool descriptors describe available tool metadata.
- Proposed invocation plans describe intent to use a tool.
- Invocation plans must remain value-only and non-operational.
- Agent runtimes may return proposed invocation plans, but neither controller nor QML may execute
  them.
- Controller and view-model exposure is limited to generic plan status and summary strings.
- Permission and sandbox runtime design must remain non-operational until a real execution phase is
  explicitly approved.

## 16. Approval Metadata Before Sandbox Or Execution

Decision: Model approval and permission state as metadata before introducing sandboxing or tool
execution.

Reason: Planned tool invocations need deterministic approval state and risk visibility without
granting the application any ability to perform actions.

Boundary rules:

- Approval policy evaluates `ToolInvocationPlan` values only.
- Approval decisions describe state such as not required, requires approval, approved, or denied.
- Permission descriptors are metadata labels, not runtime grants.
- Controller and view-model exposure is limited to generic approval status and summary strings.
- Approval does not execute tools, launch processes, mutate files, call networks, load plugins, or
  activate sandbox behavior.
- Real permission prompts, audit logs, sandboxing, and executors remain future work.

## 17. Sandbox Capability Metadata Before Runtime Enforcement

Decision: Model sandbox capability boundaries as deterministic metadata before introducing any
real sandbox runtime or tool executor.

Reason: The app needs a clear boundary between a plan, approval state, and the capabilities a
future runtime would require, without granting operating-system permissions or executing actions.

Boundary rules:

- Planning describes intended tool invocation metadata.
- Approval describes user or policy approval metadata.
- Sandbox policy describes whether planned capability metadata is within the allowed boundary.
- `ISandboxPolicy` evaluates `ToolInvocationPlan` and `ApprovalDecision` values only.
- `CapabilityDescriptor` values are labels for future runtime constraints, not real permission
  grants.
- Approval does not override sandbox capability denial.
- Controller and view-model exposure is limited to generic sandbox status and summary strings.
- Sandbox evaluation does not execute tools, launch processes, mutate files, call networks, load
  plugins, request privileges, or enforce an OS sandbox.

## 18. Placeholder Execution Boundary Before Real Executors

Decision: Add an execution ownership boundary that is explicitly placeholder-only before any real
tool execution implementation.

Reason: The application needs a stable interface for future execution ownership while preserving
the current no-action safety model.

Boundary rules:

- Planning chooses intended tool invocation metadata.
- Approval represents user or policy permission metadata.
- Sandbox policy represents capability-boundary metadata.
- `IToolExecutor` represents future execution ownership, but current implementations must remain
  non-operational.
- `NullToolExecutor` returns deterministic placeholder results only.
- Approved and sandbox-allowed plans may produce placeholder success, not real action.
- Denied, unapproved, sandbox-blocked, empty, or unknown-tool plans must be represented as blocked
  or safely rejected.
- Controller and view-model exposure is limited to generic execution status and summary strings.
- The execution boundary must not launch processes, spawn subprocesses, mutate files, call
  networks, load plugins, request privileges, invoke OS automation, or enforce a real sandbox.

## 19. Agent Pipeline Result Stabilization

Decision: Consolidate the controller-facing Phase 4 pipeline state into a value-based aggregate
result.

Reason: Planning, approval, sandbox, and placeholder execution statuses should move through one
deterministic result model so controller and view-model summaries do not duplicate string fallback
logic.

Boundary rules:

- The aggregate result represents metadata only.
- The full route remains:
  registry -> planning -> approval -> sandbox capability metadata -> placeholder execution boundary.
- QML may receive generic status and summary strings only.
- The aggregate result does not expose raw mutable runtime internals or execution controls.
- Stabilizing the result model does not authorize real execution, shell/process launch,
  filesystem mutation, networking, provider integrations, plugin loading, privileged automation, or
  real sandbox enforcement.

## 20. Runtime Context Is Metadata Ownership Only

Decision: Add a local runtime session/context owner that records the latest metadata-only agent
pipeline state.

Reason: Future agent orchestration needs a deterministic place to hold current runtime context
without confusing that context with execution, persistence, plugin loading, or sandbox enforcement.

Boundary rules:

- `RuntimeSession` owns an in-memory `AgentRuntimeContext`.
- `AgentRuntimeContext` may copy the latest pipeline result, active planned tool ids, approval
  metadata, sandbox metadata, and placeholder execution metadata.
- Session ids and revisions are deterministic local metadata.
- Runtime context is read-only at the QML boundary and exposes generic summaries only.
- Runtime context is not tool execution, planning, approval policy, persistence, plugin loading,
  sandbox enforcement, OS automation, networking, or privileged automation.
- Adding runtime context must not launch processes, spawn subprocesses, mutate files, call networks,
  read API keys, load plugins, or enforce a real sandbox.

## 21. Agent Activity Log Is In-Memory Metadata Only

Decision: Add an in-memory activity/audit trail skeleton for metadata-only agent pipeline events.

Reason: The agent pipeline needs deterministic observability before any future real execution or
security audit feature is introduced.

Boundary rules:

- `AgentActivityEntry` is value data with deterministic sequence id, activity type, status, and
  generic summary.
- `AgentActivityLog` is in-memory only in Phase 4.8.
- The controller may record request received, plan created, approval evaluated, sandbox evaluated,
  placeholder execution evaluated, and pipeline completed/blocked events.
- QML receives only aggregate read-only fields such as activity count and latest summary.
- Activity logging must not persist to files or SQLite, export data, record secrets, record raw
  system paths, launch processes, spawn subprocesses, mutate files, call networks, load plugins,
  enforce sandbox behavior, or perform tool execution.
- Future durable audit, export, pruning, redaction, and security review features must be explicit
  later phases.

## 22. Agent Pipeline UI Visibility Is Read-Only

Decision: Expose the current metadata-only agent pipeline state in the desktop UI as read-only
status text.

Reason: Users need visibility into the pipeline, runtime context, active planned tool ids, and
activity count before any future approval or execution UX exists.

Boundary rules:

- QML receives only simple view-model values such as strings, counts, and string lists.
- Dashboard visibility may show latest pipeline status/summary, runtime context status/summary,
  active planned tool ids, activity count, and latest activity summary.
- The UI must not expose raw pipeline results, runtime context objects, activity entries, mutable
  controller internals, paths, secrets, or platform details.
- Phase 4.9 does not add execution buttons, approval controls, provider integration, networking,
  plugin loading, activity persistence, shell/process launch, filesystem mutation, OS automation,
  privileged automation, or real sandbox enforcement.

## 23. Phase 4 Completion Checkpoint

Decision: Close Phase 4 with an architecture checkpoint before starting Phase 5 UI/UX work.

Reason: The agent/tool foundation now has multiple metadata-only boundaries. A checkpoint keeps
the no-execution model explicit before UI polish begins.

Boundary rules:

- Provider, agent runtime, tool registry, planning, approval, sandbox metadata, placeholder
  execution, runtime context, and activity logging remain separate responsibilities.
- Phase 5 may improve presentation, but must not turn metadata-only state into real execution,
  approval, sandbox enforcement, provider integration, plugin loading, networking, filesystem
  mutation, shell/process launch, subprocess launch, or privileged automation.
- QML remains a read-only consumer of agent/runtime state unless a later explicit phase changes
  that boundary.
- Checkpoint documentation should summarize completed scope, limitations, readiness criteria, and
  explicit out-of-scope work.

## 24. AI Orchestration Planning Is Separate From Runtime Execution

Decision: Plan future AI orchestration around a separate `ModelRouter` and `RoutingPolicy` concept
before Phase 5 UI work.

Reason: Model/provider selection needs its own architecture boundary so chat providers, agent
runtime metadata, tool execution, and model-management UI do not become coupled.

Boundary rules:

- `ModelRouter` and `RoutingPolicy` are future concepts only in Phase 4.11.
- Provider capability profiles, task classification, local/cloud fallback, device-aware selection,
  and user routing modes are metadata planning topics, not implemented runtime behavior.
- Sensitive data should prefer local-only routing, and cloud use must require explicit user
  permission in any future implementation.
- Future model-management UI should display metadata such as installed/downloadable models,
  recommendations, RAM/disk requirements, and local/cloud badges without owning routing logic.
- This planning does not add provider integrations, networking, API keys, model downloads, model
  execution, plugin loading, filesystem/system actions, real tool execution, approval UX, or
  sandbox runtime.

## 25. UI Design Tokens Before Advanced Motion

Decision: Start Phase 5 with a small QML design-token singleton and concise UI/UX plan.

Reason: The desktop shell needs shared palette, spacing, radius, and typography values before
larger UI polish, motion, or assistant visual work.

Boundary rules:

- `SentinelTheme.qml` owns reusable presentation constants only.
- QML may consume tokens for colors, spacing, radii, and text sizing, but must not gain business
  logic, provider behavior, model management, downloads, execution, or platform actions.
- Motion remains documented guidance only in Phase 5.0; no heavy animation or particle assistant
  visuals are implemented.
- UI changes must preserve current Dashboard, Chat, Memory, Settings, and runtime visibility
  behavior.

## 26. Lightweight Motion Before Assistant Visuals

Decision: Add only subtle interaction motion before any advanced assistant visual system.

Reason: The shell needs consistent hover, focus, and page/content transition behavior, but the app
must remain quiet and performant on Fedora KDE Plasma and macOS.

Boundary rules:

- Motion tokens are limited to standard durations and easing values in `SentinelTheme.qml`.
- Interaction polish may use cheap color, opacity, and border-color transitions.
- Avoid blur-heavy layers, particle systems, shader-heavy rendering, OpenGL/Vulkan custom renderers,
  and continuous idle animation.
- Motion must not trigger provider/model calls, tool execution, filesystem/system actions,
  networking, plugin loading, approval actions, or sandbox behavior.

## 27. Adaptive Layout Before Advanced Visual Systems

Decision: Add compact, normal, and wide layout behavior before building advanced assistant visuals.

Reason: The desktop shell should remain readable on narrower Linux/KDE and macOS windows while
preserving the current simple Qt/QML architecture.

Boundary rules:

- Responsive behavior is limited to QML presentation tokens, spacing, wrapping, dock sizing, and
  page/card column choices.
- Breakpoints should remain cheap width checks, not runtime services or device-specific platform
  probes.
- Narrow layouts should prefer wrapping and stable spacing over hiding core state.
- Adaptive layout work must not add provider/model execution, networking, plugin loading,
  filesystem/system actions, approval actions, sandbox behavior, particle systems, assistant-face
  rendering, or custom rendering systems.

## 28. Small Component Consistency Before Visual Expansion

Decision: Normalize repeated QML presentation patterns with small shared components before adding
larger visual systems.

Reason: Consistent inputs, status rows, button sizing, and card padding reduce UI drift while
preserving the existing view-model and runtime boundaries.

Boundary rules:

- Shared QML components may wrap presentation-only controls such as text fields and read-only info
  rows.
- These components must not own business logic, persistence, provider/model routing, approval,
  execution, filesystem/system actions, networking, plugin loading, or platform behavior.
- Visual QA guidance should cover current manual screen states and responsive widths.
- Automated UI driving, assistant-face rendering, particle systems, heavy animation, and custom
  rendering systems remain future work.

## 29. Workspace UX Reference Translation Stays Native

Decision: Treat the former `lovable-tasarim` reference as a historical design input whose useful
workspace ideas were translated into native Qt/QML components.

Reason: Sentinel must remain a native Qt/C++ modular monolith while borrowing only stable UI/UX
direction from the reference.

Boundary rules:

- Do not integrate React, Vite, Tailwind, Node, WebView, or web runtime dependencies.
- Preserve the cinematic AI operating environment direction: presence-first composition,
  translucent floating surfaces, soft glow hierarchy, thin-line geometry, and generous negative
  space.
- `SentinelTheme.qml` may own mode-aware visual helpers, but those helpers remain presentation-only.
- `Atmosphere`, `WorkspacePresence`, `ShellPanel`, dock, header, dashboard, and chat panel
  changes must bind to QML-safe view-model properties only.
- Workspace UI must not add provider/model execution, real tools, approval controls, sandbox
  runtime, plugin loading, networking, filesystem/system actions, voice, hardware integration,
  assistant-face rendering, advanced particles, heavy motion, or Qt Quick 3D.

## 30. Phase 5.4.5 Is A Stabilization Gate

Decision: Insert an architecture and UI risk audit checkpoint after Phase 5.4 and before Phase 5.5.

Reason: The workspace integration added more QML visual structure and mode-aware tokens; those
changes need a boundary/readability/tooling audit before additional UI work.

Boundary rules:

- Fix only small safe issues: stale docs, naming inconsistencies, duplicated simple styling, minor
  QML binding cleanup, component organization cleanup, label clarity, and checklist gaps.
- Do not redesign the UI or add product features.
- Verify with build, tests, formatting, available QML linting, and lightweight startup smoke checks.

## 31. Phase 5.5 Reconstructs Visual Identity Natively

Decision: Reconstruct the Phase 5.x shell around the translated Sentinel visual identity while
keeping Sentinel native Qt/QML and C++.

Reason: The technically correct Phase 5.4 workspace still read too much like a developer dashboard
or utility application. Sentinel needs a presence-first cinematic operating environment before
later assistant visual work.

Boundary rules:

- QML may add presentation primitives such as dock, orb, and telemetry surfaces.
- Visual state must bind only to QML-safe view-model properties.
- Do not add provider/model execution, real tools, networking, plugin loading, filesystem/system
  actions, web runtime integration, assistant-face rendering, particle engines, Qt Quick 3D, or
  custom rendering systems.

## 32. Model Routing Starts As Metadata Only

Decision: Add Phase 6.0 model/provider routing as value-based metadata and a deterministic static
router before any provider or model execution work.

Reason: Model/provider selection needs a testable architecture boundary that stays separate from
chat generation, agent orchestration, and tool execution.

Boundary rules:

- `IModelRouter` selects `ProviderDescriptor` and `ModelDescriptor` metadata only.
- `StaticModelRouter` is local-only and deterministic by default.
- `IChatProvider` remains the response-generation boundary.
- `IAgentRuntime` remains the agent/tool orchestration boundary.
- Controller and desktop view-model exposure must stay QML-safe strings/status values.
- No provider calls, networking, API keys, model downloads, model execution, real tool execution,
  plugin loading, filesystem/system actions, or broad UI work are part of Phase 6.0.

## 33. Routing Mode Preference Is Settings Metadata

Decision: Persist the user routing mode preference through `AppSettings` and `JsonSettingsStore`.

Reason: Routing mode affects future model/provider selection metadata and should survive app
restarts, but it must stay separate from credentials, provider setup, networking, and execution.

Boundary rules:

- Default routing mode is `Local Only`.
- Unknown routing mode values normalize back to `Local Only`.
- Settings persistence remains JSON-backed through `ISettingsStore`; SQLite is not used for this.
- QML may change the routing mode through `DesktopShellViewModel`, but routing logic remains in
  C++.
- Changing routing mode updates route metadata only; it does not call providers, use API keys,
  access the network, download or execute models, execute tools, load plugins, or perform
  filesystem/system actions.

## 34. Provider Catalog Is Metadata Only

Decision: Add a provider/model catalog boundary before provider integration or model-management
actions.

Reason: The desktop app needs deterministic metadata for future provider/model UI and routing
policy work without implying that providers are configured or executable.

Boundary rules:

- `IProviderCatalog` exposes read-only value metadata only.
- `StaticProviderCatalog` may describe future providers/models, including Ollama Local, OpenAI
  Cloud, and Anthropic Cloud placeholders.
- Catalog entries may include provider id/name, local/cloud kind, availability, supported task
  types, privacy level, and rough RAM/disk hints.
- Catalog entries must not include credentials, API keys, secrets, endpoints, transport clients, or
  executable provider objects.
- Cloud catalog entries remain not configured until a later explicit provider phase.
- `StaticModelRouter` may consume available catalog metadata but must not call providers, access the
  network, download models, execute models, load plugins, or execute tools.
- QML may show read-only text summaries only; no setup buttons, API key fields, download buttons,
  or execution controls are part of this phase.

## 35. Task Planner Is Metadata Only

Decision: Add a high-level task planner and capability graph boundary as value metadata before any
provider/model execution or tool execution work.

Reason: Sentinel needs deterministic planning visibility over task type, routing mode, privacy, and
provider/model capability metadata without granting runtime execution authority.

Boundary rules:

- `ITaskPlanner` creates `TaskPlan` metadata only.
- `StaticTaskPlanner` is deterministic and local-safe.
- Capability graph nodes and planned task steps are value data, not executable operations.
- Task planning may consider task type, routing mode, provider/model catalog availability,
  local/cloud suitability, privacy sensitivity, supported task metadata, and rough resource hints.
- Sensitive/private tasks must prefer or require local metadata.
- Unknown tasks must use a safe local metadata fallback when available.
- Unavailable providers/models must not be selected for executable routes.
- `IModelRouter` remains responsible for model/provider route metadata.
- `IAgentRuntime` remains responsible for future tool/action orchestration metadata.
- `IChatProvider` remains responsible for chat response generation.
- Task planning must not call providers, access networks, use API keys, download or execute models,
  execute tools, load plugins, mutate files, or perform system actions.
- QML may show read-only status, summary, and counts only; no execution controls are part of this
  phase.

## 36. Agent Registry Is Metadata Only

Decision: Add a static agent registry and agent descriptor model before any autonomous agent
runtime behavior.

Reason: Sentinel needs stable names, roles, capability summaries, and task affinities for future
orchestration UI and planning without implying that autonomous agents can run.

Boundary rules:

- `IAgentRegistry` exposes read-only value metadata only.
- `StaticAgentRegistry` returns deterministic static descriptors for Atlas, Orin, Vela, Kaze, Nyx,
  and Sol.
- Agent descriptors may include id, display name, role, capability summary, preferred task types,
  local/cloud affinity, privacy affinity, state, and priority.
- `StaticTaskPlanner` may select a preferred agent metadata label for a plan, but it must not
  execute the agent.
- `IAgentRuntime` remains the future execution/orchestration boundary.
- No autonomous loops, threads, background workers, provider/model calls, networking, API keys,
  downloads, tool execution, memory writes, plugin loading, filesystem/system actions, or dynamic
  discovery are part of this phase.
- QML may show read-only counts and summaries only; no execution controls or autonomous toggles are
  part of this phase.

## 37. Memory Taxonomy Is Metadata Only

Decision: Add a static memory taxonomy catalog before any semantic memory, vector memory, or
autonomous recall/write behavior.

Reason: Sentinel needs stable memory category names, retention/privacy labels, recall hints, and
planner affinity metadata without changing the existing key-value memory persistence boundary.

Boundary rules:

- `IMemoryCatalog` exposes read-only value metadata only.
- `StaticMemoryCatalog` returns deterministic descriptors for Episodic, Semantic, Procedural,
  Reflective, and Ambient memory categories.
- Memory descriptors may include type, shard status, retention policy, privacy level, recall hint,
  task affinities, tags, and association metadata.
- `IMemoryStore` remains the explicit key-value memory persistence contract.
- `SQLiteMemoryStore` remains simple key-value storage and is not replaced by the taxonomy catalog.
- `StaticTaskPlanner` may select a preferred memory affinity metadata label for a plan, but it must
  not recall, search, write, embed, vectorize, or mutate memory.
- No vector database, embeddings, semantic search, autonomous memory writes, provider/model calls,
  networking, API keys, downloads, tool execution, plugin loading, filesystem/system actions, or
  dynamic discovery are part of this phase.
- QML may show read-only counts and summaries only; no semantic search controls or memory graph
  execution controls are part of this phase.

## 38. Orchestration Snapshot Is A Read Model

Decision: Add a deterministic orchestration snapshot that aggregates existing metadata for
workspace visibility without adding orchestration execution.

Reason: The desktop shell needs one compact read model for routing, task planning, provider
catalog, agent, memory taxonomy, runtime context, and activity metadata before any future execution
or automation phase.

Boundary rules:

- `OrchestrationSnapshot` and `WorkspaceStateSummary` are value data only.
- The snapshot is built from existing metadata already owned by `ApplicationController`.
- The snapshot may include health status, summary text, and compact signal strings for QML.
- Snapshot updates are tied to existing metadata-only changes such as routing changes and runtime
  metadata updates.
- The snapshot must not execute plans, call providers, execute models, search memory, embed data,
  vectorize data, execute tools, load plugins, access networks, mutate files, perform system
  actions, or start autonomous loops.
- No background refresh, timers, threads, workers, downloads, API keys, credentials, or dynamic
  discovery are part of this phase.
- QML may show read-only snapshot status, summary, and signal strings only; no execution controls
  are part of this phase.

## 39. Orchestration Diagnostics Are Metadata Readiness Checks

Decision: Add deterministic orchestration diagnostics and readiness reports over existing metadata
without probing external systems or enabling execution.

Reason: Sentinel needs a compact readiness checklist for routing, catalogs, planning, privacy, and
execution-boundary posture before future provider/model work, but this phase must not imply hidden
provider setup or runtime capability.

Boundary rules:

- `OrchestrationDiagnostic`, `OrchestrationReadinessCheck`, and
  `OrchestrationReadinessReport` are value data only.
- `StaticOrchestrationDiagnostics` inspects existing `OrchestrationSnapshot` and provider catalog
  metadata only.
- Diagnostic ordering must remain deterministic for tests and UI.
- Checks may report metadata state for routing mode, selected route, provider catalog, agent
  registry, memory taxonomy, task planner, snapshot health, local-only privacy posture, cloud
  provider unavailability/not-configured status, and disabled execution capability.
- Diagnostics must not call providers, execute models, probe local model runtimes, read API keys,
  access networks, scan the filesystem, run external processes, execute tools, load plugins, build
  embeddings, run semantic/vector search, mutate memory, or start background workers.
- Controller and QML exposure stays read-only and QML-safe: status, summary, and diagnostic
  strings only.

## 40. Conversation Session Context Is Separate Metadata

Decision: Add a higher-level conversation/session context layer as deterministic metadata without
replacing chat history or Phase 4 runtime context.

Reason: The desktop shell needs a stable interaction context summary for future AI workspace
behavior, but the current alpha must keep chat messages, agent runtime metadata, and orchestration
metadata separate and non-operational.

Boundary rules:

- `ChatSession` remains the in-memory chat transcript owner and stays connected to
  `IChatHistoryStore` for persistence.
- `ConversationSession` owns interaction/session metadata only: session id/status, interaction
  mode, attention state, context scope, and context-window summaries.
- Phase 4 `RuntimeSession` remains the agent pipeline metadata owner and is not merged into
  `ConversationSession`.
- The conversation context window may copy routing mode, preferred agent summary, memory affinity
  summary, and latest orchestration snapshot summary.
- Controller and QML exposure stays read-only and QML-safe: strings only.
- This layer must not add multi-conversation persistence, provider/model calls, streaming, API key
  handling, networking, model downloads, model execution, tool execution, plugin loading,
  filesystem/system actions, embeddings, vector search, semantic search, autonomous workers,
  timers, threads, or external process calls.

## 41. Conversation State Graph Is Metadata Only

Decision: Add a deterministic conversation state graph for high-level interaction state metadata
without using state transitions as execution triggers.

Reason: The shell needs a readable current state and last-transition summary for future
conversation orchestration, while preserving the existing separation between chat transcripts,
conversation session context, and agent runtime metadata.

Boundary rules:

- `ConversationStateGraph` owns only the current conversation state and last transition result.
- `ConversationSession` remains the owner of session/context metadata.
- `ChatSession` remains the owner of chat message history.
- Phase 4 `RuntimeSession` remains the owner of agent pipeline runtime metadata.
- Valid transitions are deterministic metadata rules; invalid transitions are rejected with
  deterministic summaries.
- Controller and QML exposure stays read-only and QML-safe: current state, transition status, and
  transition summary strings only.
- Transitions must not call providers, execute models, stream tokens, execute tools, approve
  actions, load plugins, access networks, scan or mutate filesystems, perform system actions,
  build embeddings, run semantic/vector search, start background workers, or run external
  processes.

## 42. Phase 6 Checkpoint Before Runtime Work

Decision: Close Phase 6 with a pre-runtime architecture checkpoint before starting Phase 7.

Reason: Phase 6 introduced multiple metadata orchestration surfaces. A checkpoint keeps the
no-execution model explicit, records readiness criteria, and prevents Phase 7 from being treated
as implicit provider/model/tool execution.

Boundary rules:

- Phase 6.10 may update documentation and small safe consistency gaps only.
- The checkpoint must not add product features, provider integrations, networking, API key
  handling, downloads, streaming, model execution, real tool execution, plugin loading,
  filesystem/system actions, vector search, embeddings, semantic search, or autonomous workers.
- Phase 7.0 should begin with local runtime boundary planning and ownership mapping.
- Full provider/model execution, cloud routing, credentials, downloads, streaming, plugins, vector
  memory, and real tool execution require later explicit scopes.
- QML exposure remains through `DesktopShellViewModel` and QML-safe values only.
- `docs/PHASE_6_CHECKPOINT.md` is the durable checkpoint record for completed scope, known
  limitations, readiness criteria, and recommended Phase 7 breakdown.

## 43. Local Runtime Boundary Is Metadata Only

Decision: Add `ILocalRuntime` as the future local inference/runtime boundary with a
non-executable null implementation.

Reason: Sentinel needs an explicit owner for future local runtime metadata before any provider or
model execution is considered. The boundary should make local runtime readiness visible without
probing local services or implying executable capability.

Boundary rules:

- `ILocalRuntime` is separate from `IChatProvider`, `IModelRouter`, `IAgentRuntime`, and
  `IToolExecutor`.
- `LocalRuntimeDescriptor`, status, health, and capability values are metadata only.
- `NullLocalRuntime` reports deterministic metadata and refuses requests with a placeholder
  non-executable response.
- Controller and QML exposure stays read-only and QML-safe: status, health, summary, capability
  summaries, and refusal summary strings only.
- The local runtime boundary must not call Ollama/providers, execute models, download models,
  stream tokens, launch processes/subprocesses, scan or mutate filesystems, execute tools, load
  plugins, access networks, read API keys, or start background workers.

## 44. Local Runtime Sessions Are Ownership Metadata

Decision: Add local runtime session ownership/lifecycle metadata without allocating or executing
runtime resources.

Reason: Future local runtime work needs a stable place to describe session ownership, allocation,
and reservation state before any local provider/model execution can be considered.

Boundary rules:

- `LocalRuntimeSession` is not `ChatSession`.
- `LocalRuntimeSession` is not Phase 4 `RuntimeSession`.
- `LocalRuntimeSession` is not provider/model execution.
- `LocalRuntimeSession` is not tool execution or plugin ownership.
- `LocalRuntimeAllocation` and `LocalRuntimeReservation` are descriptive metadata only.
- `NullLocalRuntimeSessionManager` returns deterministic placeholder sessions only.
- Controller and QML exposure stays read-only and QML-safe: counts, status strings, summaries, and
  string lists.
- Session metadata must not allocate models, call providers, use API keys, access networks,
  download models, stream output, launch processes/subprocesses, scan or mutate filesystems,
  execute tools, load plugins, or start background workers.

## 45. Runtime Capability Negotiation Is Metadata Only

Decision: Add a runtime capability negotiation registry that describes what future runtime work may
support without activating or executing any capability.

Reason: Future runtime phases need a stable vocabulary for local inference, streaming, multimodal,
memory, tool/plugin bridge, filesystem/process, cloud relay, local-only, and privacy-safe
capabilities before any execution or permission policy is implemented.

Boundary rules:

- `IRuntimeCapabilityRegistry` exposes deterministic capability descriptors and negotiation
  metadata only.
- `StaticRuntimeCapabilityRegistry` enables only safety posture metadata: local-only enforcement
  and privacy-safe mode.
- Disabled and unavailable capabilities remain descriptive and non-executable.
- Runtime capability negotiation is separate from `ILocalRuntime`, local runtime sessions,
  `IChatProvider`, `IModelRouter`, `IAgentRuntime`, `IToolExecutor`, approval policy, sandbox
  policy, and plugin management.
- Controller and QML exposure stays read-only and QML-safe: counts, enabled/disabled summaries,
  negotiation profile summary, negotiation result summary, and local-only enforcement summary.
- Capability negotiation must not activate capabilities, call providers, use API keys, access
  networks, download models, execute models, stream output, launch processes/subprocesses, scan or
  mutate filesystems, execute tools, load plugins, approve permissions, or start background
  workers.

## 46. Runtime Permission Policy Is Metadata-Only Default-Deny

Decision: Add `IRuntimePermissionPolicy` with `StaticRuntimePermissionPolicy` as a deterministic
permission metadata boundary that denies execution-level runtime requests by default.

Reason: Runtime phases need explicit permission vocabulary and decision metadata before any future
execution authority is considered.

Boundary rules:

- `RuntimePermission`, `RuntimePermissionLevel`, `RuntimePermissionRequest`, and
  `RuntimePermissionDecision` are value metadata only.
- Default policy behavior denies execution-level requests in metadata-only mode.
- Permission metadata is separate from capability negotiation, runtime safety reporting, and runtime
  request pipeline status.
- Permission metadata must not execute models/providers/tools/plugins, launch processes, access
  filesystems, or access networks.

## 47. Runtime Request Pipeline Is Metadata Trace Only

Decision: Add `IRuntimePipeline` with `StaticRuntimePipeline` as a deterministic request-pipeline
metadata boundary.

Reason: Runtime readiness work needs ordered request/permission/safety/execution-boundary trace
visibility before any execution runtime is introduced.

Boundary rules:

- `RuntimePipelineRequest`, stage/status enums, traces, and results are value metadata only.
- The pipeline consumes existing permission/safety metadata and returns deterministic status/summary
  traces.
- Execution boundary stage remains blocked/no-execution in current phase.
- Pipeline metadata must not call providers/models, execute tools/plugins, launch processes, access
  filesystems, or access networks.

## 48. Runtime Safety Policy Reports Local-Only No-Execution Posture

Decision: Add `IRuntimeSafetyPolicy` with `StaticRuntimeSafetyPolicy` as deterministic runtime
safety posture metadata.

Reason: Runtime boundary stabilization needs explicit local-only/no-execution safety reporting
before sandbox/runtime execution work is approved.

Boundary rules:

- `RuntimeSafetyPolicy`, `RuntimeSafetyRule`, `RuntimeSafetyDecision`, and `RuntimeSafetyReport`
  are value metadata only.
- Safety reporting is deterministic and read-only for controller/view-model/QML visibility.
- Safety policy metadata is separate from sandbox runtime enforcement and execution ownership.
- Safety policy metadata must not activate runtime capabilities, call providers/models, execute
  tools/plugins, launch processes, access filesystems, or access networks.

## 49. Phase 7 Runtime Checkpoint Keeps Execution Out Of Scope

Decision: Close Phase 7 with an architecture checkpoint before Phase 8 planning.

Reason: The local runtime boundary now includes session, capability, permission, safety, and
pipeline metadata. A checkpoint keeps those responsibilities separate and prevents metadata
visibility from being treated as execution authority.

Boundary rules:

- Phase 7.6 may update documentation, tests, naming, ownership comments, and QML read-only exposure
  consistency only.
- `docs/PHASE_7_CHECKPOINT.md` is the durable checkpoint record for completed scope, limitations,
  guardrails, readiness criteria, and strict out-of-scope work.
- `ApplicationController` remains the C++ owner for runtime boundaries.
- `DesktopShellViewModel` remains the QML-safe read-only exposure layer for runtime metadata.
- The checkpoint must not add provider/model execution, provider calls, networking/API keys,
  downloads, streaming, subprocess/process launch, filesystem/system actions, real tools, plugin
  loading, sandbox runtime enforcement, embeddings, semantic search, autonomous workers, or
  execution/setup UI.

## 50. Execution Lifecycle Is Metadata-Only Coordination

Decision: Add `IExecutionLifecycle`, `StaticExecutionLifecycle`, `ExecutionCoordinator`, and
execution session metadata as a future execution lifecycle/session coordination layer.

Reason: Future provider/runtime integration needs a deterministic lifecycle vocabulary before any
execution path is enabled.

Boundary rules:

- Execution request, intent, priority, lifecycle state/status/result/trace, session ownership, and
  coordination snapshot values are metadata only.
- The lifecycle can describe requested, validating, permission-check, safety-check, coordination,
  ready-placeholder, and blocked states.
- The current lifecycle always ends blocked and non-executable.
- Invalid transitions are rejected safely and do not advance state.
- `ExecutionCoordinator` produces read-only snapshots only; it is not a scheduler or worker.
- `ApplicationController` owns lifecycle/coordinator interfaces.
- `DesktopShellViewModel` exposes only strings and string lists.
- Execution lifecycle remains separate from `ILocalRuntime`, `IChatProvider`, `IAgentRuntime`,
  `IToolExecutor`, provider adapters, tools, plugins, and platform services.
- This decision does not allow provider/model execution, Ollama launch, process/subprocess launch,
  filesystem/system actions, networking/API keys, downloads, streaming, tool/plugin execution,
  autonomous workers, timers/background loops, execution controls, or setup UI.

Future Phase 9 integration must add explicit provider/runtime interfaces, policy gates, tests, and
documentation before any executable path can exist.

## 51. Runtime Adapter And Provider Bridge Are Pre-integration Metadata

Decision: Add local runtime adapter, provider runtime bridge, and runtime integration readiness
contracts as metadata-only pre-integration boundaries.

Reason: Future Ollama/local runtime integration needs explicit adapter and bridge ownership before
any local provider implementation, endpoint configuration, model discovery, or execution path is
introduced.

Boundary rules:

- `ILocalRuntimeAdapter` describes adapter metadata only and does not connect to, call, launch, or
  probe a local runtime.
- `LocalRuntimeAdapterDescriptor` and capability summaries are descriptive placeholders.
- `IProviderRuntimeBridge` is not an `IChatProvider` implementation and does not generate responses.
- Provider bridge requests/responses are metadata only and report not connected/not executable.
- `StaticRuntimeIntegrationReadiness` produces ordered readiness checks from already-owned
  metadata only; it does not probe the system.
- Controller and QML exposure stays read-only and QML-safe: status strings, summaries, and string
  lists.
- Settings/Dashboard may show readiness metadata but must not add setup buttons, endpoint fields,
  model selection UI, or execution controls.
- This decision does not allow Ollama/OpenAI/Anthropic calls, API keys, networking, downloads,
  streaming, process/subprocess launch, filesystem/system actions, model discovery, model
  execution, real tools/plugins, embeddings/vector DB, or autonomous workers.

## 52. Ollama Health Boundary Is Local Read-Only

Decision: Add Ollama endpoint/config metadata and an `IOllamaRuntimeClient` boundary for local
health checks and installed-model metadata discovery only.

Reason: Phase 9 needs the first controlled real local-provider contact point, but chat inference,
model execution, downloads, process launch, and tooling remain too broad for this step.

Boundary rules:

- Default endpoint is `http://127.0.0.1:11434`.
- Endpoint normalization accepts only loopback HTTP endpoints and falls back to the default for
  invalid, cloud, non-loopback, query, fragment, or non-HTTP values.
- `AppSettings` may persist the normalized endpoint only; no API keys or credentials are stored.
- `NullOllamaRuntimeClient` is deterministic and unavailable.
- `OllamaHttpRuntimeClient` is injectable and may call only local Ollama read-only metadata APIs:
  `/api/version` for health and `/api/tags` for installed model summaries.
- The Ollama client is not `IChatProvider`, provider bridge execution, execution lifecycle, model
  router execution, agent runtime, tool executor, or plugin runtime.
- Controller and QML exposure stays read-only and QML-safe: endpoint, connection/health strings,
  summary, model count, and model summary strings.
- This decision does not allow prompt execution, model generation, streaming, model
  download/pull/delete/run, subprocess/process launch, cloud provider calls, API keys, tool/plugin
  execution, filesystem/system scans/actions, background workers, setup UI, model selection UI, or
  routing chat requests to Ollama.

Phase 9.3-9.5 adds that separately scoped, policy-gated, interface-owned,
injectable/testable inference boundary.

## 53. Local Inference Requires A Dedicated Boundary

Decision: Prompt execution for local models must go through `ILocalInferenceClient`, not
`IChatProvider`, `IProviderRuntimeBridge`, `IAgentRuntime`, or tool execution.

Reason: Phase 9.3-9.5 introduces the first executable local inference path, so prompt execution
needs a narrow owner that can be injected, tested without Ollama, and gated by existing
permission/safety metadata before any runtime call is made.

Boundary rules:

- Only local loopback HTTP Ollama endpoints are valid.
- No cloud endpoints, API keys, redirects, model pulls/downloads/deletes, subprocess launch,
  filesystem/system actions, tools/plugins, or autonomous loops are allowed.
- Streaming remains out of scope and is rejected as request metadata.
- Blank prompts and missing or unavailable models are rejected safely.
- `ApplicationController` records permission/safety and client traces before exposing summaries.
- QML receives strings and string lists only; raw requests, responses, clients, and traces are not
  exposed.

## 54. Selected Local Model Metadata Before Model Management

Decision: Persist a selected local model name as configuration metadata and validate it against
discovered local model metadata only when that metadata is already available.

Reason: Users need a stable local-model preference before chat routing is allowed to target local
inference, but selection must not imply model management or execution.

Boundary rules:

- Selected local model storage is a setting, not memory or chat history.
- No model download, pull, delete, install, or process launch is introduced.
- Known invalid selected models are rejected before local inference is invoked.
- Unknown discovery state is reported as metadata rather than treated as permission to manage
  models.
- QML receives summaries, strings, lists, and booleans only.

## 55. Streaming Boundary Before Streaming Execution

Decision: Add streaming value types and an interface while keeping the implementation disabled.

Reason: Streaming needs a separate contract from non-streaming inference so future token delivery
can be guarded and tested without destabilizing the existing `/api/generate` path.

Boundary rules:

- `LocalInferenceStreamChunk`, `LocalInferenceStreamStatus`, and stream result values are
  metadata/value objects.
- `ILocalInferenceStreamClient` is a boundary, not permission to stream.
- Current behavior is deterministic disabled status and no opened stream.
- No token streaming UI is required in this phase.
- Non-streaming inference remains permission/safety gated and stable.

## 56. Chat-To-Ollama Routing Is Explicit Opt-In

Decision: Chat may use local Ollama inference only when a persisted local chat inference setting is
explicitly enabled.

Reason: Phase 10.0-10.2 is the first chat path that can call local inference, so the default must
remain the deterministic local-safe provider path and every executable local request must stay
behind the existing boundary.

Boundary rules:

- The local chat inference setting defaults to disabled.
- Disabled chat uses the existing `IChatProvider` path.
- Enabled chat still requires a valid selected/effective local model, local loopback HTTP Ollama
  endpoint, runtime permission approval, and runtime safety compliance before invoking
  `runLocalInference`.
- User messages are appended before routing; exactly one assistant message is appended from either
  the inference result or a safe refusal/error.
- QML receives only a boolean setting plus routing status/summary strings.
- Streaming, model management/download/pull/delete UI, cloud provider routing, API keys,
  tools/plugins, filesystem/system actions, subprocess launch, and autonomous loops remain out of
  scope.

## 57. Streaming Chat Requires A Separate Local-Only Opt-In

Decision: Chat streaming uses `ILocalInferenceStreamClient` and a separate persisted local
streaming opt-in.

Reason: Streaming has different runtime behavior from non-streaming inference. It needs ordered
chunk metadata, live QML-safe text exposure, cancellation/error/refusal handling, and duplicate
message protection without weakening the existing local-only inference guards.

Boundary rules:

- Streaming defaults to disabled.
- Streaming may run only when local chat inference is enabled, streaming is enabled and available,
  the effective model is valid, the endpoint is local loopback HTTP, and permission/safety checks
  pass.
- `OllamaLocalInferenceStreamClient` may call only local loopback `/api/generate` streaming with
  manual redirect policy.
- No cloud endpoints, API keys, model downloads/pulls/deletes, subprocess launch,
  filesystem/system actions, tools/plugins, or autonomous loops are introduced.
- Malformed chunks are ignored with metadata; timeout, cancellation, refusal, and client errors
  produce safe summaries.
- Streaming chunks update live view-model text, but the final assistant message is appended and
  persisted once.

## 58. Model Selection UX Is Not Model Management

Decision: Settings may expose discovered local Ollama models for selection and status visibility,
but it must not expose model management actions.

Reason: Phase 10.6-10.8 improves the local runtime workflow now that explicit local inference,
model discovery, and guarded streaming exist. The UX should make installed-model state clear
without adding downloads, deletion, process ownership, cloud setup, or autonomous behavior.

Boundary rules:

- The selected local model remains a persisted setting.
- Settings can select only from already discovered local model names.
- Controller validation remains authoritative and rejects known invalid selected/effective models
  before inference or streaming clients are called.
- Model summaries are QML-safe strings containing name, size when available, modified date when
  available, and Local Only status.
- Runtime management UX is read-only/action-light apart from selected model and existing local
  chat/streaming toggles.
- No model downloads, pulls, deletes, installs, cloud providers, API keys, filesystem/system
  actions, subprocess launch, tools/plugins, autonomous loops, or broad model-management workflow
  are introduced.

## 59. Model Management Readiness Is Metadata-Only

Decision: Add `IModelManagementService` and a deterministic `StaticModelManagementService` for
readiness metadata, recommendations, requirements, and unavailable action results only.

Reason: The UI needs to prepare users for future local model management without quietly granting
Sentinel authority to download, delete, install, scan, or launch anything.

Boundary rules:

- Recommendations and RAM/disk requirements are static, approximate, descriptive metadata.
- Pull, delete, and install requests return unavailable/not implemented results.
- Controller and view-model exposure is limited to strings and string lists.
- Existing Ollama model discovery remains the only installed-model metadata source.
- No model downloads, pulls, deletes, installs, filesystem/system scans or actions, process launch,
  cloud calls, API keys, tools/plugins, autonomous loops, or broad setup workflow is introduced.

## 60. Local AI Runtime UX Must Finalize Through Chat History

Decision: Local inference and streaming UX may expose transient runtime state, but final assistant
responses must be represented by normal chat history messages.

Reason: Chat history is the durable transcript boundary. Live streaming text is useful while a
response is in progress, but keeping it after completion risks duplicate or stale assistant text in
QML and weakens transcript correctness.

Boundary rules:

- Live streaming text is QML-readable only while streaming is active.
- When streaming completes, the live preview is cleared and the accumulated final response is
  appended once through `ChatSession` and `IChatHistoryStore` when available.
- Refusals and errors are converted to safe user-facing summaries before they become assistant
  chat messages.
- Summaries may include technical categories such as missing model, invalid model, endpoint
  blocked, permission blocked, safety blocked, timeout, invalid response, and client unavailable.
- Summaries must not expose stack traces, secrets, filesystem paths, raw internal objects, provider
  credentials, or broad endpoint details.
- No real cancellation button, downloads, pulls, deletes, installs, cloud providers, API keys,
  filesystem/system actions, subprocess launch, tools/plugins, or autonomous loops are introduced.

## 61. Phase 11 Local AI Checkpoint Is Runtime QA Only

Decision: Close Phase 11 with a local AI usability and runtime QA checkpoint instead of adding new
local AI features.

Reason: The current local Ollama path now has opt-in chat inference, opt-in streaming, model
selection, runtime summaries, safe refusal/error states, and fake-client testability. Before Phase
12, the priority is confirming that these paths stay bounded and predictable.

Boundary rules:

- Checkpoint work may add focused tests, documentation, safe wording, and QML-safe exposure checks.
- Checkpoint work must preserve existing local chat inference, streaming, model validation,
  endpoint, permission, and safety gates.
- Selected model, local chat inference, and streaming settings remain settings persistence only.
- Streaming live-preview text must clear after completion, refusal, cancellation, or error; final
  assistant output remains a normal chat history message.
- QML must not expose raw local inference clients, stream clients, requests, responses, runtime
  policies, Ollama internals, model-management service objects, stores, paths, secrets, or
  credentials.
- No new product features, model pull/delete/install, cloud providers/API keys,
  filesystem/system actions, subprocess launch, tools/plugins, UI redesign, or autonomous behavior
  are introduced by the checkpoint.

## 62. Voice Starts As A Disabled Provider Boundary

Decision: Add text-to-speech and speech-to-text provider interfaces with deterministic null
providers before adding any real voice runtime.

Reason: Voice affects microphones, speakers, local binaries, model files, permissions, privacy, and
runtime lifecycle. Sentinel needs a testable architecture boundary and UI readiness surface before
any audio I/O or Piper/Whisper integration is allowed.

Boundary rules:

- `ITextToSpeechProvider` and `ISpeechToTextProvider` own future voice provider behavior.
- Current `NullTextToSpeechProvider` and `NullSpeechToTextProvider` implementations are disabled
  and return safe refusals.
- `VoiceCapability`, `VoiceProviderDescriptor`, `VoiceProviderStatus`, `VoiceRuntimeMode`,
  `VoiceRequest`, `VoiceResponse`, and `VoiceReadinessReport` are value-only metadata.
- Controller and view-model exposure is limited to strings, string lists, and booleans.
- Settings may display read-only voice readiness metadata only.
- Future Piper integration must stay behind `ITextToSpeechProvider`.
- Future Whisper integration must stay behind `ISpeechToTextProvider`.
- No microphone access, audio playback, recording, synthesis, transcription, subprocess/process
  launch, filesystem/system action, download, cloud call, API key, voice button, record button,
  speak button, or broad UI redesign is introduced.

## 63. Voice Runtime Coordination Is Metadata-Only

Decision: Add voice session and pipeline orchestration as deterministic metadata before any audio
runtime is allowed.

Reason: Voice runtime work needs session ownership, pipeline state, trace visibility, and readiness
reporting before Sentinel can safely define microphone capture, playback, Piper, Whisper, process
ownership, or cancellation behavior.

Boundary rules:

- `VoiceSession`, `VoiceSessionId`, `VoiceSessionState`, `VoicePipelineStage`,
  `VoicePipelineStatus`, and `VoicePipelineTrace` are value-only metadata.
- `IVoiceRuntimeCoordinator` owns future voice runtime/session coordination.
- `StaticVoiceRuntimeCoordinator` emits deterministic idle, preparing, awaiting-input,
  transcribing-placeholder, inference-placeholder, synthesis-placeholder, completed, blocked, and
  error metadata.
- Runtime summaries explicitly report runtime unavailable, TTS unavailable, STT unavailable,
  microphone disabled, playback disabled, local-only policy active, and process execution disabled.
- Controller and view-model exposure remains limited to strings, string lists, and booleans.
- Settings may display read-only voice session/runtime/pipeline metadata only.
- Future Piper integration must stay behind `ITextToSpeechProvider` and the runtime coordinator
  boundary.
- Future Whisper integration must stay behind `ISpeechToTextProvider` and the runtime coordinator
  boundary.
- No microphone access, audio playback, recording, synthesis, transcription, subprocess/process
  launch, filesystem/system action, download, cloud call, API key, voice activation, autonomous
  loop, or broad UI redesign is introduced.

## 64. Phase 12 Voice Checkpoint Plans Integration Before Execution

Decision: Close Phase 12 with a voice architecture checkpoint and local Piper/Whisper integration
plan instead of adding real audio behavior.

Reason: The provider boundaries and runtime/session metadata are now in place. Before Phase 13 can
add any executable local voice capability, Sentinel needs a durable record of ownership,
limitations, safety guardrails, and readiness criteria.

Boundary rules:

- Checkpoint work may update documentation, clarify architecture decisions, and confirm existing
  test coverage.
- `ITextToSpeechProvider`, `ISpeechToTextProvider`, and `IVoiceRuntimeCoordinator` remain the
  required boundaries for any future Piper, Whisper, playback, capture, or voice session work.
- `ApplicationController` owns injected/default voice providers and the runtime coordinator.
- `DesktopShellViewModel` and QML remain limited to read-only strings, string lists, and booleans.
- Future Piper work must define local binary ownership, model path ownership, playback lifecycle,
  cancellation, permission prompts, and safety checks before execution.
- Future Whisper work must define microphone permission, capture lifecycle, local binary/model
  ownership, transcription privacy, cancellation, and error handling before execution.
- No microphone access, audio playback, Piper execution, Whisper execution, subprocess/process
  launch, filesystem/system action, download, cloud call, API key, voice button, activation flow,
  autonomous voice loop, or broad UI redesign is introduced by the checkpoint.

## 65. Local Voice Runtime Environment Owns Binary And Model Readiness Before Execution

Decision: Add a metadata-only voice runtime environment boundary for future local Piper/Whisper
binary, model, permission, and safety ownership before any execution is allowed.

Reason: Piper and Whisper integration requires local binary paths, model paths, permission gates,
and runtime safety checks. These must be visible and testable before Sentinel can safely execute
voice binaries or touch audio devices.

Boundary rules:

- `VoiceBinaryDescriptor` and `VoiceModelDescriptor` describe expected future Piper/Whisper binary
  and model ownership only.
- `VoiceRuntimePermission` describes denied/default-off microphone, playback, process execution,
  and model-read posture as metadata.
- `VoiceRuntimeSafetyReport` blocks execution by default and reports no microphone, playback,
  process execution, filesystem-wide scan, download, cloud call, or API-key behavior.
- `IVoiceRuntimeEnvironment` is separate from `ITextToSpeechProvider`,
  `ISpeechToTextProvider`, and `IVoiceRuntimeCoordinator`.
- `NullVoiceRuntimeEnvironment` and `StaticVoiceRuntimeEnvironment` are deterministic and
  non-operational.
- Controller and view-model exposure remains limited to strings, string lists, and booleans.
- Settings may display read-only environment, binary, model, permission, and safety metadata only.
- No microphone access, audio playback, Piper execution, Whisper execution, subprocess/process
  launch, filesystem-wide scan, model loading, download, cloud call, API key, setup button, path
  picker, or broad UI redesign is introduced.

## 66. Piper TTS Adapter Starts As A Non-Executable Provider Boundary

Decision: Add a Piper text-to-speech adapter skeleton behind the voice provider boundary without
enabling audio playback, file-output synthesis, or Piper subprocess execution.

Reason: Piper integration needs typed request/result/configuration metadata, binary/model readiness
checks, and deterministic refusal behavior before Sentinel can safely define any executable local
TTS flow.

Boundary rules:

- `PiperTtsConfig`, `PiperVoiceModelDescriptor`, `PiperTtsRequest`, `PiperTtsResult`, and
  `PiperTtsStatus` are value-only Piper TTS metadata.
- `IPiperTtsClient` is the future low-level client boundary, and `NullPiperTtsClient` is the
  default deterministic non-operational implementation.
- `PiperTextToSpeechProvider` stays behind `ITextToSpeechProvider` and reports status/readiness
  metadata only.
- The default Piper adapter is disabled/not configured.
- Missing Piper binary or voice model metadata causes deterministic refusal before any client
  boundary is reached.
- The sketched execution path remains non-callable by default; even metadata-ready requests refuse
  until a later explicit phase defines controlled file-output synthesis.
- Controller and view-model exposure remains limited to QML-safe strings, string lists, and
  booleans.
- Settings may display read-only Piper readiness only.
- No audio playback, microphone access, Whisper/STT, Piper execution, subprocess/process launch,
  model loading, download, cloud/API-key behavior, filesystem-wide scan, speak button, model/path
  picker, or broad UI redesign is introduced.

## 67. Piper TTS File Output Is Explicit, Local, And Controlled

Decision: Add controlled Piper text-to-audio file output behind `ITextToSpeechProvider` and
`IPiperTtsClient`, while keeping Piper disabled by default and keeping playback/microphone
behavior out of scope.

Reason: Sentinel can safely advance Piper integration only by separating synthesis-to-file from
playback, requiring explicit local policy gates, and keeping test execution injectable without a
real Piper binary or voice model.

Boundary rules:

- The default Piper configuration remains disabled/not configured.
- File-output synthesis requires an enabled Piper config, an existing executable Piper binary, an
  existing readable voice model, an app-controlled output directory, explicit process execution
  permission in request/config/safety policy, a local-only request, playback disabled, microphone
  disabled, downloads blocked, cloud/API-key behavior blocked, and filesystem-wide scans blocked.
- Output files are generated only inside the configured controlled cache/temp output directory.
- `ProcessPiperTtsClient` is the real local process client and is reachable only after provider
  gates pass; tests can inject deterministic fake clients.
- Results report configured, refused, missing, safety-blocked, succeeded, failed, and timeout
  metadata with safe output path summaries, timeout values, exit code, and error summaries.
- Controller and view-model exposure remains QML-safe strings, string lists, and booleans.
- Settings may display Piper configured/missing/file-output readiness only.
- No automatic playback, microphone access, Whisper/STT, model download, cloud/API-key behavior,
  broad filesystem scan, autonomous voice loop, speak/play button, path picker, model picker, or
  broad UI redesign is introduced.

## 68. Phase 13 Voice/Piper Checkpoint Preserves The Safety Boundary

Decision: Close Phase 13 with a Voice/Piper checkpoint and readiness review before adding any
voice setup, playback, STT, or conversation-loop behavior.

Reason: Phase 13 introduced Piper ownership metadata, a provider/client boundary, and controlled
file-output synthesis. The project needs a durable review of the resulting safety boundary before
Phase 14 work can choose whether to focus on configuration UX, playback, Whisper, or voice-loop
planning.

Boundary rules:

- Checkpoint work may update documentation and confirm existing architecture/test coverage only.
- The current TTS path is `text -> Piper provider -> gated file-output metadata`.
- Piper remains disabled by default.
- File output remains reachable only after explicit enabled configuration, local-only request
  posture, process permission, executable binary, readable model, controlled output path, playback
  disabled, microphone disabled, no downloads, no cloud/API-key behavior, no filesystem-wide
  scans, and safety approval gates pass.
- `PiperTextToSpeechProvider` remains behind `ITextToSpeechProvider`; `ProcessPiperTtsClient`
  remains behind `IPiperTtsClient`.
- `ISpeechToTextProvider` remains a null-provider boundary; no Whisper adapter or execution is
  introduced.
- Controller and view-model exposure remains limited to QML-safe strings, string lists, and
  booleans.
- Settings remains read-only for voice/Piper readiness and does not expose setup, path/model
  picker, speak, play, record, or activation controls.
- No new runtime behavior, audio playback, microphone access, Whisper execution, downloads,
  cloud/API-key behavior, autonomous voice loop, voice conversation loop, or broad UI redesign is
  introduced by the checkpoint.

## 69. Local Voice Configuration Is Persisted Metadata, Not Execution

Decision: Store Piper and Whisper binary/model path configuration in settings and validate only
the exact configured paths as metadata.

Reason: Users need a visible setup surface before a future voice runtime phase, but path storage
must not grant Sentinel authority to run local voice binaries, open audio devices, download models,
or scan the filesystem broadly.

Boundary rules:

- `AppSettings` owns persisted strings for Piper binary path, Piper model path, Whisper binary
  path, and Whisper model directory/path.
- `ApplicationController` derives readiness summaries from those strings using exact-path
  metadata checks: configured/missing, exists/missing, readable/unreadable, and
  executable/non-executable for binaries.
- `DesktopShellViewModel` exposes QML-safe strings and string lists only.
- Settings may provide compact text entry for paths and readiness summaries.
- Piper readiness may reflect configured metadata, but Piper execution remains behind existing
  provider/client safety gates and remains blocked by default.
- Whisper remains configuration metadata only; no STT adapter or execution path is added.
- No Piper execution, Whisper execution, microphone access, playback, downloads,
  filesystem-wide scans, cloud/API keys, autonomous loops, or path picker integration are added in
  this decision.

## 70. Voice Auto-Detection Is Hint-Only And Non-Invasive

Decision: Expose Piper and Whisper path hints as read-only Settings suggestions, not automatic
configuration or runtime probing.

Reason: Users benefit from common-location hints, but Sentinel must not expand configuration UX
into filesystem discovery, binary execution, downloads, or automatic setup.

Boundary rules:

- Binary hints may check only fixed known locations: `/opt/homebrew/bin/piper`,
  `/usr/local/bin/piper`, `/opt/homebrew/bin/whisper`, and `/usr/local/bin/whisper`.
- Model hints may validate only already configured model paths for readability.
- Hints are QML-safe strings/string lists and are never applied to settings automatically.
- Status badges may report configured/missing, valid/missing, readable/unreadable, and
  executable/non-executable metadata only.
- Settings may show concise help text, compact badges, and short hint rows.
- No Piper execution, Whisper execution, microphone access, playback, downloads,
  filesystem-wide scans, recursive model discovery, cloud/API keys, autonomous loops, path
  pickers, or automatic settings writes are added in this decision.

## 71. Local Ollama Chat Activation Is Narrow And Explicit

Decision: Activate real local Ollama inference only for explicit user chat requests and only
through the existing local runtime/inference interfaces.

Reason: Phase 14.7-15.0 is the first phase where desktop chat may produce real model responses.
That activation must stay narrower than provider routing, agent execution, tool execution, voice
runtime work, or model management.

Boundary rules:

- The desktop app uses the persisted Ollama endpoint after loopback-only normalization.
- Health checks are limited to local loopback HTTP `/api/version`.
- Model discovery is limited to local loopback HTTP `/api/tags`.
- Inference is limited to local loopback HTTP `/api/generate` for an explicitly selected or
  safely resolved local model.
- `LocalOnlyRuntimePermissionPolicy` allows only `LocalInference` execution and denies provider
  invocation, tool invocation, external process, filesystem access, broader network access, and
  plugin invocation.
- `ApplicationController::sendMessage` remains the only chat path that may invoke local inference,
  and only when local chat inference is enabled by settings.
- QML receives only QML-safe strings, string lists, booleans, and chat models from
  `DesktopShellViewModel`.
- No cloud/API keys/providers, autonomous agents, tools, shell execution, filesystem-wide actions,
  model downloads/pulls/deletes, microphone access, playback, Piper execution, Whisper execution,
  or autonomous voice loop is added by this activation.
