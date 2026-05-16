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
- Chat, memory, settings, dashboard, sidebar, header, and status bar.
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

- Responsive behavior is limited to QML presentation tokens, spacing, wrapping, sidebar width, and
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

Decision: Treat `lovable-tasarim` as the visual identity source of truth while translating its
workspace ideas into native Qt/QML components.

Reason: Sentinel must remain a native Qt/C++ modular monolith while borrowing only stable UI/UX
direction from the reference.

Boundary rules:

- Do not integrate React, Vite, Tailwind, Node, WebView, or web runtime dependencies.
- Preserve the cinematic AI operating environment direction: presence-first composition,
  translucent floating surfaces, soft glow hierarchy, thin-line geometry, and generous negative
  space.
- `SentinelTheme.qml` may own mode-aware visual helpers, but those helpers remain presentation-only.
- `Atmosphere`, `WorkspacePresence`, `ShellPanel`, sidebar, header, dashboard, and chat panel
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

Decision: Reconstruct the Phase 5.x shell around `lovable-tasarim` as the visual source of truth
while keeping Sentinel native Qt/QML and C++.

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
