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
