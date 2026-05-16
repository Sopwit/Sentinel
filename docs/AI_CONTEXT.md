# AI Context

## Vision

Sentinel is a cross-platform Qt/QML-based personal AI assistant desktop application, optimized first for Linux/Fedora KDE Plasma. The current product is a Desktop Alpha foundation for local UI, provider boundaries, memory storage, settings storage, and future extension points.

## Target Platform

- Primary optimization target: Fedora KDE Plasma.
- Compatibility goal: keep Windows and macOS possible.
- Current implementation: Qt/QML desktop app with portable C++ core boundaries.

## Core Principles

- Native desktop over web shell.
- C++ core with QML presentation.
- Clear interfaces before feature expansion.
- Portable core before platform-specific integration.
- Local-first defaults.
- Minimal dependencies.
- Storage responsibilities stay separated.
- QML must not own business logic or persistence.

## Platform Strategy

- Keep core application logic platform-neutral.
- Use Qt APIs where they preserve portability.
- Put platform-specific behavior behind future service interfaces:
  - `IPlatformService`
  - `IPathProvider`
  - `INotificationService`
  - `ISystemIntegrationService`
- Linux/Fedora KDE integrations may be deeper, but must not leak into shared core logic.

## UI Direction

Current UI foundation:

- Qt Quick/QML shell.
- Dashboard, chat, memory, settings, sidebar, header, and status bar.
- QML-safe view models.

Future UI vision:

- Modern Qt Quick UX.
- Smooth animations and animated panels.
- Responsive layouts and adaptive themes.
- Assistant-like chat-oriented interaction.
- Dashboard cards and extensible components.
- Modern desktop aesthetics without over-styling.

## Current Technical Foundation

- C++20 and Qt 6.
- CMake build.
- QML UI shell.
- `ApplicationController` coordinates core behavior.
- `DesktopShellViewModel` is the QML boundary.
- `IChatProvider` isolates provider behavior.
- `IAgentRuntime` isolates future agent orchestration behavior.
- `IToolRegistry` isolates tool metadata registration and lookup behavior.
- `ToolInvocationPlan` models proposed tool-use intent without execution.
- `IApprovalPolicy` evaluates planned tool invocations as approval metadata without execution.
- `ISandboxPolicy` evaluates planned and approved invocation metadata against capability boundaries
  without execution.
- `IToolExecutor` owns the future execution boundary but currently returns placeholder-only results
  without executing tools.
- `AgentPipelineResult` consolidates the metadata-only runtime pipeline result for planning,
  approval, sandbox, and placeholder execution status exposure.
- `AgentRuntimeContext` and `RuntimeSession` own local in-memory runtime context metadata derived
  from the latest pipeline result without persistence or execution authority.
- `AgentActivityLog` records in-memory metadata-only audit/activity events for the agent pipeline
  without persistence, secrets, paths, or OS interaction.
- Desktop UI shows read-only agent/runtime visibility for latest pipeline state, runtime context,
  active planned tool ids, and activity count/latest summary without execution or approval controls.
- Phase 4 checkpoint documentation records completed scope, known limitations, and Phase 5
  readiness criteria while keeping execution non-operational.
- AI orchestration planning now documents future `ModelRouter`, `RoutingPolicy`, provider
  capability profiles, task classification, routing modes, privacy-aware routing, and model
  management UI metadata without implementing providers or model execution.
- Phase 6.0 implements the first metadata-only model routing skeleton: model/provider descriptors,
  provider capability profiles, task classification, routing modes, `IModelRouter`, and a
  deterministic local-only `StaticModelRouter`. This remains separate from `IChatProvider` and
  `IAgentRuntime` and performs no provider calls, model execution, downloads, networking, API key
  handling, plugin loading, filesystem/system actions, or real tool execution.
- Phase 6.1 persists the user routing mode preference through `AppSettings` and the existing
  `JsonSettingsStore`. The default is privacy-safe `Local Only`; changing the setting only updates
  metadata route selection and does not enable cloud calls, API keys, provider setup, downloads, or
  execution.
- Phase 6.2 adds a metadata-only provider/model catalog boundary. `StaticProviderCatalog`
  describes local and future provider/model placeholders, availability, task support, privacy
  level, and rough resource hints without credentials, endpoints, downloads, networking, or
  execution. `StaticModelRouter` seeds route metadata from available catalog entries while cloud
  placeholders remain not configured and unselectable.
- Phase 6.3 adds a metadata-only capability graph and task planner boundary. `StaticTaskPlanner`
  builds deterministic high-level task plan metadata from task type, routing mode, provider catalog
  availability, local/cloud classification, privacy sensitivity, and resource hints without
  calling models, providers, networks, tools, plugins, or system services.
- Phase 6.4 adds a metadata-only agent registry skeleton. `StaticAgentRegistry` exposes static
  descriptors for Atlas, Orin, Vela, Kaze, Nyx, and Sol, and `StaticTaskPlanner` may annotate plans
  with a preferred agent metadata match. No autonomous loops, background workers, provider calls,
  model execution, tools, plugins, memory writes, networking, or filesystem/system actions are
  enabled.
- Phase 6.5 adds a metadata-only memory taxonomy skeleton. `StaticMemoryCatalog` describes
  Episodic, Semantic, Procedural, Reflective, and Ambient memory categories with retention,
  privacy, recall hint, association, and task-affinity metadata. This remains separate from
  `IMemoryStore`/`SQLiteMemoryStore` key-value persistence and adds no embeddings, vector database,
  semantic search, autonomous memory writes, provider/model calls, tools, networking, plugins, or
  filesystem/system actions.
- Phase 6.6 adds a metadata-only orchestration snapshot read model. `OrchestrationSnapshot` and
  `WorkspaceStateSummary` aggregate existing routing, provider catalog, task plan, preferred agent,
  memory affinity, runtime context, and activity metadata into deterministic read-only summaries and
  signals. It is not an execution system and adds no background refresh, threads, timers, provider
  calls, model calls, memory search, networking, embeddings, vector database, tools, plugins, or
  filesystem/system actions.
- Phase 6.7 adds deterministic orchestration diagnostics and readiness metadata.
  `StaticOrchestrationDiagnostics` inspects existing snapshot and catalog values for routing,
  selected route, catalogs, planner availability, snapshot health, local-only privacy posture, cloud
  provider unavailability, and disabled execution capability. It does not scan the filesystem, probe
  providers/models, call networks, start workers, execute tools, or run external processes.
- Phase 6.8 adds a higher-level metadata-only conversation/session context layer.
  `ConversationSession` and `RuntimeContextWindow` describe the current interaction session id,
  status, interaction mode, attention state, routing mode, preferred agent, memory affinity, and
  latest orchestration snapshot summary. This is separate from `ChatSession` message history and
  the Phase 4 `RuntimeSession` agent pipeline metadata, and it adds no provider/model execution,
  streaming, networking, filesystem scans, tools, embeddings, vector search, or autonomous workers.
- Phase 6.9 adds a deterministic metadata-only conversation state graph skeleton.
  `ConversationStateGraph` tracks high-level states such as Idle, Listening, Planning, Routing,
  Waiting For Approval, Ready To Respond, Responding, Completed, and Error. Transitions produce
  accepted/rejected metadata summaries only and do not execute providers, models, tools, approval
  actions, streaming, networking, filesystem/system actions, semantic search, or autonomous work.
- Phase 5.0 adds UI/UX planning and a small QML design-token singleton without adding advanced
  motion, provider integration, model execution, or runtime behavior.
- Phase 5.1 adds lightweight motion and interaction tokens plus subtle hover/focus/page-transition
  polish without heavy animation, assistant-face rendering, provider integration, or runtime
  behavior.
- Phase 5.2 adds adaptive layout tokens and responsive shell/page behavior so the desktop UI
  remains readable at compact, normal, and wide widths without adding execution or advanced visual
  systems.
- Phase 5.3 normalizes small QML component patterns and adds a manual visual QA checklist without
  adding execution-related UI or advanced assistant visuals.
- Phase 5.4 translates the useful `lovable-tasarim` design-reference direction into native Qt/QML
  with a left rail, central workspace presence, right chat panel, mode-aware visual tokens, and
  lightweight ambient motion. React/Vite/Tailwind/Node/WebView remain outside the production app.
- `IMemoryStore` isolates memory storage.
- `IChatHistoryStore` isolates chat history storage.
- `ISettingsStore` isolates settings storage.
- `SQLiteMemoryStore` persists explicit key-value memory entries.
- `SQLiteChatHistoryStore` persists chat messages.
- `JsonSettingsStore` persists application settings.
- Desktop UI exposes generic chat history status without SQLite details.
- Tests cover core behavior and persistence boundaries.

## Current Phase State

- Completed: Phase 3.1, Phase 3.1.5, Phase 3.2, Phase 3.3, Phase 3.4, Phase 3.5, Phase 4.0, Phase 4.1, Phase 4.2, Phase 4.3, Phase 4.4, Phase 4.5, Phase 4.6, Phase 4.7, Phase 4.8, Phase 4.9, Phase 4.10, Phase 4.11, Phase 5.0, Phase 5.1, Phase 5.2, Phase 5.3, Phase 5.4, Phase 6.0, Phase 6.1, Phase 6.2, Phase 6.3, Phase 6.4, Phase 6.5, Phase 6.6, Phase 6.7, Phase 6.8, and Phase 6.9.
- Current: Desktop alpha with a stabilized metadata-only agent pipeline and metadata-only model
  routing skeleton:
  registry -> planning -> approval -> sandbox capability metadata -> placeholder execution boundary,
  with local runtime context/session, in-memory activity logging for pipeline metadata, and
  read-only dashboard visibility for that state. Model/provider routing is descriptor-only and
  currently resolves to a deterministic local placeholder.
- Next: Phase 6.x stabilization and later explicitly approved provider/model work.
- Recent: Phase 6.9, Conversation State Graph Skeleton.

Current runtime still has no real tool execution, shell/process launch, filesystem mutation, networking, API keys, real provider integrations, plugin loading, privileged automation, multi-conversation support, encryption, export, pruning, real sandbox runtime, subprocess execution, or platform-specific service implementations.
