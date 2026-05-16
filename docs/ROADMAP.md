# Sentinel Roadmap

Sentinel is a cross-platform Qt/QML personal AI assistant desktop app, optimized first for Linux/Fedora KDE Plasma.

## Phase 1: Foundation

Create the C++20/Qt project structure, core interfaces, local echo provider, runtime memory, mode handling, and documentation base.

## Phase 2: UI Shell Foundation

Build the QML desktop shell, dashboard, chat panel, memory/settings pages, view models, and local-only UX foundation.

## Phase 3: Persistence + Architecture Stabilization

Add settings persistence, SQLite key-value memory, dedicated chat history persistence, diagnostics, lifecycle controls, and AI context docs.

## Phase 3.4: Cross-platform Architecture Readiness

Completed. Prepared architecture for Linux, Windows, and macOS without implementing platform integrations yet.

Focus:

- Portable core boundaries.
- Linux/Fedora KDE optimized path.
- Future platform service interfaces.
- Controlled dependency growth.
- No Phase 4 implementation.

Delivered in this phase:

- Platform boundary interfaces (`IPathProvider`, `IPlatformService`, `INotificationService`,
  `ISystemIntegrationService`).
- Default Qt `QStandardPaths` path ownership through `StandardPathProvider`.
- Local storage maintenance controls for memory/chat with confirmation UX.
- Explicit separation preserved across settings, memory, and chat history.

## Phase 3.5: Pre-agent Architecture Audit and Release Checkpoint

Completed. Stabilization checkpoint before starting Phase 4 implementation.

Focus:

- Architecture consistency audit across core, view-model, persistence, platform boundaries, and QML exposure safety.
- Small safe fixes only (docs, naming, status wording, minor test gaps).
- Verification gate for tests and formatting.
- Explicit no-Phase-4-runtime rule.

Readiness criteria for Phase 4:

- Boundaries and persistence separation remain intact.
- QML exposure remains generic and safe.
- Verification commands and format checks pass.
- Known limitations are documented and unchanged.

Candidate interfaces:

- `IPlatformService`
- `IPathProvider`
- `INotificationService`
- `ISystemIntegrationService`

## Phase 4: Agent Core & Tool System

Completed through Phase 4.11 with metadata-only agent/tool boundaries and no real execution.

## Phase 6.7: Orchestration Diagnostics and Readiness Checklist

Completed. Added deterministic readiness diagnostics over existing orchestration metadata only.

Delivered:

- Value-only diagnostic levels, diagnostic entries, readiness checks, and readiness reports.
- Static readiness generation for routing, selected route, catalogs, planner availability, snapshot
  health, local-only privacy posture, cloud provider unavailability, and disabled execution
  capability.
- Read-only controller/view-model status, summary, and diagnostic string exposure.
- Minimal Dashboard and Settings visibility without setup or execution controls.

Still out of scope:

- Provider/model execution, provider probing, networking/API keys, model downloads, filesystem or
  system scans/actions, real tool execution, plugin loading, embeddings, vector search, and
  autonomous workers.

## Phase 6.8: Runtime Context Session Layer

Completed. Added a metadata-only conversation/session context layer above chat history and agent
runtime metadata.

Delivered:

- Value-only conversation session id/status, interaction mode, attention state, context scope, and
  context-window metadata.
- Deterministic session context summaries from current routing mode, preferred agent, memory
  affinity, and orchestration snapshot summary.
- Read-only controller/view-model exposure and minimal Dashboard/Settings visibility.
- Tests for session defaults, context metadata, routing updates, QML-safe exposure, and separation
  from `ChatSession` and Phase 4 `RuntimeSession`.

Still out of scope:

- Provider/model execution, streaming, networking/API keys, model downloads, filesystem or system
  actions, real tool execution, plugin loading, embeddings, vector search, semantic search,
  autonomous workers, and multi-conversation persistence.

## Phase 6.9: Conversation State Graph Skeleton

Completed. Added a deterministic metadata-only conversation state graph for high-level
interaction flow.

Delivered:

- Value-only conversation states, transitions, transition results, transition status, graph
  interface, and static deterministic graph implementation.
- Safe transition rules from Idle through Listening, Planning, Routing, response readiness,
  response completion, approval-wait metadata, and Error.
- Invalid transition rejection with deterministic summaries.
- Read-only controller/view-model exposure and minimal Dashboard/Settings visibility.
- Tests for valid/invalid transitions, error transitions, QML-safe exposure, and separation from
  `ConversationSession`, `ChatSession`, and Phase 4 `RuntimeSession`.

Still out of scope:

- Provider/model execution, streaming, networking/API keys, model downloads, filesystem or system
  actions, real tool execution, approval actions, plugin loading, embeddings, vector search,
  semantic search, autonomous workers, and multi-conversation persistence.

## Phase 6.10: Pre-runtime Architecture Checkpoint

Completed. Checkpointed the Phase 6 metadata orchestration foundation before Phase 7.

Delivered:

- `docs/PHASE_6_CHECKPOINT.md` with completed scope, architecture findings, known limitations,
  Phase 7 readiness criteria, strict out-of-scope list, and recommended Phase 7 breakdown.
- Review across provider catalog, model router, task planner, agent registry, memory taxonomy,
  orchestration snapshot, diagnostics/readiness, conversation session, conversation state graph,
  controller ownership, and QML boundary.
- Phase/status/context docs updated to mark Phase 6 as checkpointed.
- Phase 7.0 defined as local runtime boundary planning/implementation, not full model execution
  unless explicitly scoped later.

Still out of scope:

- Provider/model execution, streaming, networking/API keys, model downloads, filesystem or system
  actions, real tool execution, approval actions, plugin loading, embeddings, vector search,
  semantic search, autonomous workers, and broad UI redesign.

### Phase 4.0: Agent Core Planning and Minimal Runtime Skeleton

Completed.

Delivered:

- `IAgentRuntime` boundary and request/response/status abstractions.
- Deterministic local `NullAgentRuntime` with no tool execution.
- Minimal controller/view-model agent status/placeholder request wiring.

Still out of scope:

- Real tool execution.
- Networking/API keys.
- Real provider integration.
- Plugin loading.
- Privileged or OS-level automation.

### Phase 4.1: Tool Descriptor and Registry Skeleton

Completed.

Delivered:

- Tool metadata abstractions (`ToolDescriptor`, parameters, risk level, execution mode).
- Tool metadata registry boundary (`IToolRegistry`, `InMemoryToolRegistry`).
- Deterministic register/list/find semantics and duplicate-id handling.
- Metadata-only runtime exposure of available tools.

Still out of scope:

- Tool execution runtime.
- Filesystem/system mutation tools.
- Shell command execution.
- Platform automation.
- Networking/API keys and provider integrations.

### Phase 4.2: Tool Invocation Planning Boundary

Completed. Added structured, value-only proposed tool invocation plans without adding execution.

Delivered:

- Proposed invocation value types (`ToolInvocationPlan`, `PlannedToolInvocation`,
  `ToolInvocationArgument`, `ToolInvocationPlanStatus`).
- Agent runtime support for metadata-only proposed plans.
- Deterministic local placeholder planning in `NullAgentRuntime`.
- Generic planning status/summary exposure through controller/view-model.
- Tests that preserve the no-execution boundary.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Sandbox runtime.

### Phase 4.3: Approval and Permission Metadata Skeleton

Completed. Added approval policy metadata for planned tool invocations without execution or
sandboxing.

Delivered:

- Approval metadata value types (`ApprovalStatus`, `ApprovalDecision`, `PermissionDescriptor`,
  `ToolApprovalRequest`).
- Approval policy boundary (`IApprovalPolicy`) and deterministic `StaticApprovalPolicy`.
- Metadata-only evaluation of planned tool invocations.
- Generic approval status/summary exposure through controller/view-model.
- Tests for safe plans, risky approval requirements, approved/denied states, empty plans, and
  deterministic policy behavior.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Sandbox runtime.

### Phase 4.4: Sandbox and Capability Boundary Skeleton

Completed. Added sandbox capability policy metadata for planned and approved invocations without
execution or real sandbox enforcement.

Delivered:

- Capability metadata abstractions (`CapabilityDescriptor`, sandbox status/result records).
- Sandbox policy boundary (`ISandboxPolicy`) and deterministic `StaticSandboxPolicy`.
- Metadata-only evaluation of planned capabilities against allowed capability ids.
- Generic sandbox status/summary exposure through controller/view-model.
- Tests for metadata-only allowance, unknown capability denial, empty plans,
  approved-but-not-capable blocking, and deterministic policy behavior.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.5: Execution Boundary Skeleton

Completed. Added a placeholder execution ownership boundary without real tool execution.

Delivered:

- Execution abstractions (`ToolExecutionRequest`, `ToolExecutionResult`,
  `ToolExecutionStatus`).
- Execution interface boundary (`IToolExecutor`) and deterministic `NullToolExecutor`.
- Controller/view-model placeholder execution status and summary exposure.
- Tests for placeholder success, blocked paths, empty plans, unknown tools, deterministic results,
  and status exposure.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.6: Agent Runtime Pipeline Stabilization

Completed. Stabilized the metadata-only controller pipeline after the placeholder execution
boundary.

Delivered:

- Aggregate `AgentPipelineResult` value model for planning, approval, sandbox, execution, and
  summary state.
- Centralized safe summary/status exposure for controller and desktop view-model use.
- Clear controller route through:
  registry -> planning -> approval -> sandbox capability metadata -> placeholder execution.
- Focused tests for success, approval-blocked, sandbox-blocked, empty-plan, unknown-tool, and
  deterministic status/summary output.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.7: Runtime Context and Tool Session Skeleton

Completed. Added deterministic local runtime context ownership without adding execution.

Delivered:

- `AgentRuntimeContext` value state for latest pipeline metadata, active planned tool ids, status,
  summary, session id, and revision.
- `RuntimeSession` as the in-memory owner for runtime context metadata.
- Controller/view-model read-only runtime context status, summary, session id, and active planned
  tool id exposure.
- Tests for deterministic context creation, pipeline attachment, planned tool ordering, reset
  behavior, and controller/view-model exposure.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Provider/network integrations.
- API key handling.
- Plugin loading.
- Runtime context persistence.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.8: Agent Activity Log and Audit Trail Skeleton

Completed. Added in-memory metadata-only activity logging for the agent pipeline.

Delivered:

- `AgentActivityEntry`, `AgentActivityType`, and `AgentActivityStatus` value metadata.
- `AgentActivityLog` with deterministic in-memory sequence ordering and clear behavior.
- Controller logging for request received, plan created, approval evaluated, sandbox evaluated,
  placeholder execution evaluated, and pipeline completed/blocked.
- Desktop view-model read-only activity count and latest summary exposure.
- Tests for deterministic ordering, clear behavior, successful and blocked pipeline logging, and
  controller/view-model exposure.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Activity log persistence/export.
- Secret capture or API key handling.
- Provider/network integrations.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.9: Agent Pipeline UI Visibility

Completed. Added simple read-only desktop visibility for the metadata-only pipeline.

Delivered:

- Dashboard status section for latest pipeline status/summary.
- Runtime context status/summary and active planned tool id visibility.
- Activity count and latest activity summary visibility.
- Focused view-model tests for successful and blocked pipeline visibility.
- QML boundary checks that keep agent visibility properties simple and read-only.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem mutation.
- Execution or approval controls.
- Activity log persistence/export.
- Secret capture or API key handling.
- Provider/network integrations.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.10: Architecture Checkpoint and Cleanup

Completed. Closed Phase 4 with an architecture consistency checkpoint before Phase 5.

Delivered:

- Phase 4 completed-scope and limitation checkpoint documentation.
- Phase 5 readiness criteria.
- Focused reset/fallback tests for existing metadata-only behavior.
- Minor Dashboard label clarity.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Execution or approval controls.
- Real sandbox runtime or OS permission enforcement.

### Phase 4.11: AI Orchestration Planning Checkpoint

Completed. Planned future AI routing architecture before Phase 5.

Delivered:

- `ModelRouter` and `RoutingPolicy` direction.
- Provider capability profile and task classification concepts.
- Local/cloud fallback, device-aware selection, user routing modes, and privacy-aware routing.
- Future model-management UI metadata scope.
- Clear separation from current Phase 4 runtime, providers, agent runtime, tool execution, and UI
  screens.

Still out of scope:

- Real provider integrations.
- Networking/API keys.
- Ollama/OpenAI/Anthropic integration.
- Model downloads or model execution.
- Plugin loading.
- Filesystem/system actions.
- Real tool execution.

## Phase 5: Advanced UI/UX & Motion System

Evolve the Qt Quick experience with responsive layouts, adaptive themes, assistant-like interaction, animated panels, dashboard cards, and reusable components.

### Phase 5.0: UI/UX Planning and Design System Foundation

Completed. Established design direction and shared QML presentation tokens.

Delivered:

- `docs/UI_UX_PLAN.md` for visual direction, design-system scope, and motion constraints.
- `SentinelTheme.qml` QML singleton for palette, spacing, radius, and typography tokens.
- Minimal QML token adoption while preserving current Dashboard, Chat, Memory, Settings, and
  runtime visibility behavior.

Still out of scope:

- Heavy animation or particle assistant visuals.
- Provider/model integrations or model management execution.
- Real tool execution, approval actions, sandbox runtime, plugin loading, networking, or
  filesystem/system actions.

### Phase 5.1: Motion and Interaction Foundation

Completed. Added lightweight interaction motion without advanced visuals.

Delivered:

- Motion duration/easing tokens in `SentinelTheme.qml`.
- Sidebar hover/focus polish.
- Tokenized command button hover/focus styling.
- Text-field focus-ring transitions.
- Lightweight page opacity transition hooks.
- Updated motion guidelines.

Still out of scope:

- Heavy animations, particle systems, assistant-face rendering, or custom rendering systems.
- Provider/model execution, networking/API keys, plugin loading, filesystem/system actions, real
  tool execution, approval actions, or sandbox runtime.

### Phase 5.2: Adaptive Layout and Responsive Shell Foundation

Completed. Added responsive layout behavior without changing runtime capabilities.

Delivered:

- Compact and wide breakpoint tokens in `SentinelTheme.qml`.
- Adaptive shell margins, sidebar width, header/status behavior, and page spacing.
- Dashboard/card/chat wrapping for narrower windows.
- Memory and settings forms that stack at compact widths.
- UI/UX guidance for practical Linux/KDE and macOS resizing behavior.

Still out of scope:

- Heavy animations, particle systems, assistant-face rendering, or custom rendering systems.
- Provider/model execution, networking/API keys, plugin loading, filesystem/system actions, real
  tool execution, approval actions, or sandbox runtime.

### Phase 5.3: Component Consistency and Visual QA Pass

Completed. Tightened presentation consistency and added manual visual QA guidance.

Delivered:

- Shared `SentinelTextField` and `InfoRow` QML components.
- Consistent input/button sizing and card padding tokens.
- Dashboard and settings read-only status rows aligned to the same component pattern.
- `docs/UI_QA_CHECKLIST.md` with compact, normal, wide, Fedora KDE, and macOS check points.

Still out of scope:

- Heavy animations, particle systems, assistant-face rendering, or custom rendering systems.
- Provider/model execution, networking/API keys, plugin loading, filesystem/system actions, real
  tool execution, approval actions, sandbox runtime, or automated visual driving.

Readiness criteria:

- Preserve Phase 4 no-execution boundaries.
- Treat AI orchestration and model-management screens as metadata-only planning until an explicit
  later implementation phase.
- Keep QML behind view-models and avoid business logic in presentation code.
- Treat agent/runtime visibility as read-only unless a later explicit phase changes it.
- Keep full tests and formatting passing through UI changes.

### Phase 5.4: Workspace UX Integration

Completed. Used `lovable-tasarim` as a design reference only and translated its workspace direction
into native Qt/QML.

Delivered:

- Left navigation/status rail, central Sentinel presence workspace, right chat panel, and compact
  top status composition.
- Centralized colors, spacing, radius, opacity, border, glow, and animation tokens.
- Mode-aware visual treatment for Companion, Focus, Mission, System, Minimal, and Tactical modes.
- Lightweight QML atmosphere and presence motion without web, heavy rendering, or runtime behavior
  changes.

Still out of scope:

- React/Vite/Tailwind/Node/WebView integration.
- Real tool execution, approval actions, sandbox runtime, automation, voice, hardware, provider
  execution, networking/API keys, plugin loading, advanced particle systems, or Qt Quick 3D.

### Phase 5.4.5: Architecture and UI Risk Audit

Stabilization checkpoint before Phase 5.5.

Focus:

- Re-audit core/view-model/persistence/provider-agent/tool-boundary separation after the Phase 5.4
  workspace changes.
- Re-audit QML component structure, design-token usage, mode-aware visuals, responsive behavior,
  and manual QA expectations.
- Fix only small safe documentation, naming, duplicated styling, binding, or checklist issues.

Still out of scope:

- New product features, UI redesign, provider/model execution, real tools, approval controls,
  sandbox runtime, automation, voice, hardware, plugin loading, networking/API keys, filesystem or
  system actions, React/WebView/Node/Tailwind/Vite integration, assistant-face rendering, advanced
  particles, heavy motion, or Qt Quick 3D.

### Phase 5.5: Visual Identity Reconstruction

Completed. Rebuilt the native QML shell around the `lovable-tasarim` visual source of truth.

Delivered:

- Presence-first Dashboard/Core composition with a larger central Sentinel orb scene.
- Ultra-thin left rail, floating bottom dock navigation, and a softer right AI bridge surface.
- Reusable QML primitives for the dock, orb, and floating telemetry readouts.
- Expanded visual tokens for glass surfaces, ambient glow, spacing, and dock sizing.
- Reduced utility-dashboard density by moving runtime/status information into small scene readouts.

Still out of scope:

- React/Vite/Tailwind/Node/WebView integration.
- Provider/model execution, networking/API keys, real tool execution, approval actions, sandbox
  runtime, automation, voice, hardware, plugin loading, filesystem/system actions, assistant-face
  rendering, advanced particles, heavy custom rendering, or Qt Quick 3D.

## Phase 6: Functional Workspace And Model Orchestration

### Phase 6.0: Metadata-Only Model Routing Skeleton

Completed. Added a local-safe model/provider routing architecture without execution.

Delivered:

- `ModelDescriptor`, `ProviderDescriptor`, `ProviderCapabilityProfile`, `RoutingMode`,
  `TaskType`, task classification, and route result metadata.
- `IModelRouter` boundary and deterministic `StaticModelRouter`.
- Minimal controller/view-model read-only routing status and selected model/provider summary.
- Tests for descriptor metadata, deterministic local routing, local-only behavior, unknown task
  fallback, and QML-safe exposure.

Still out of scope:

- Provider integrations, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, model-management UI, real tool execution, plugin loading, filesystem/system
  actions, or broad UI redesign.

### Phase 6.1: Routing Mode Settings and Persistence

Completed. Persisted routing mode preference as local settings metadata.

Delivered:

- `AppSettings` routing mode preference with `Local Only` default.
- JSON-backed persistence through the existing settings store.
- Mutable metadata routing mode in `StaticModelRouter`.
- Controller/view-model route metadata refresh when routing mode changes.
- Minimal Settings page selector for Auto, Fast, Balanced, Quality, Local Only, Cloud Allowed, and
  Battery Saver.
- Tests for defaults, persistence, invalid fallback, route summary updates, and QML-safe exposure.

Still out of scope:

- Provider setup, credentials/API keys, networking, Ollama/OpenAI/Anthropic calls, model downloads,
  model execution, model-management UI, real tool execution, plugin loading, filesystem/system
  actions, or broad UI redesign.

### Phase 6.2: Provider Catalog Metadata Skeleton

Completed. Added the first provider/model catalog boundary without provider integration.

Delivered:

- `IProviderCatalog`, provider/model catalog entry metadata, and deterministic
  `StaticProviderCatalog`.
- Available local metadata placeholder plus not-configured placeholders for Ollama Local,
  OpenAI Cloud, and Anthropic Cloud.
- Read-only controller/view-model and Settings page catalog summaries.
- Router seeding from available catalog metadata while cloud placeholders remain unavailable and
  non-routable.

Still out of scope:

- Provider setup, credentials/API keys, endpoints, networking, provider calls, model downloads,
  model execution, model-management actions, real tool execution, plugin loading, filesystem/system
  actions, or broad UI redesign.

### Phase 6.3: Capability Graph and Task Planner Skeleton

Completed. Added metadata-only capability graph and task planning boundaries.

Delivered:

- `CapabilityNode`, `CapabilityGraph`, `TaskPlan`, `PlannedTaskStep`, and `TaskPlanStatus`.
- `ITaskPlanner` and deterministic `StaticTaskPlanner`.
- Planner metadata over task type, routing mode, provider/model catalog availability, local/cloud
  classification, privacy sensitivity, and resource hints.
- Read-only controller/view-model and Settings page task plan status, summary, and step count.
- Tests for deterministic planning, privacy-local behavior, cloud-unavailable fallback, unknown
  fallback, unavailable cloud blocking, and step ordering.

Still out of scope:

- Provider setup, credentials/API keys, endpoints, networking, provider calls, model downloads,
  model execution, tool execution, plugin loading, filesystem/system actions, or broad UI redesign.

### Phase 6.4: Agent System Skeleton

Completed. Added static agent metadata and registry boundaries without autonomous behavior.

Delivered:

- `AgentDescriptor`, role/state/priority enums, capability profile metadata, and task affinity
  metadata.
- `IAgentRegistry` and deterministic `StaticAgentRegistry`.
- Static descriptors for Atlas, Orin, Vela, Kaze, Nyx, and Sol.
- Planner annotation with preferred agent metadata where task affinity is available.
- Read-only controller/view-model and QML exposure for registered agent count, active agent
  summaries, and current preferred agent summary.
- Tests for registry determinism, unique ids, task mapping, planner interaction, and QML-safe
  exposure.

Still out of scope:

- Real agent execution, autonomous loops, threads/background workers, provider integrations,
  credentials/API keys, networking, downloads, model execution, tool execution, plugin loading,
  filesystem/system actions, or broad UI redesign.

Later Phase 6 work may revisit permission prompts, auditability, sandboxing strategy, secret
handling, and safe local automation constraints only after explicit approval.

### Phase 6.5: Memory Taxonomy and Semantic Metadata Skeleton

Completed. Added static memory taxonomy metadata without semantic memory execution.

Delivered:

- `MemoryType`, memory shard status, retention, privacy, recall hint, affinity, and association
  metadata.
- `IMemoryCatalog` and deterministic `StaticMemoryCatalog`.
- Static categories for Episodic, Semantic, Procedural, Reflective, and Ambient memory.
- Planner annotation with preferred memory affinity metadata.
- Read-only controller/view-model and QML exposure for memory category count, taxonomy summaries,
  and current memory affinity summary.
- Tests for catalog determinism, unique ids/types, retention/privacy preservation, planner
  affinity metadata, and QML-safe exposure.

Still out of scope:

- Vector databases, embeddings, semantic search, autonomous memory writes, provider/model
  execution, networking/API keys, downloads, tool execution, plugin loading, filesystem/system
  actions, or replacing `IMemoryStore`/`SQLiteMemoryStore` key-value persistence.

### Phase 6.6: Orchestration Snapshot and Workspace State Skeleton

Completed. Added a deterministic metadata aggregation read model without execution behavior.

Delivered:

- `OrchestrationSnapshot`, `WorkspaceStateSummary`, `OrchestrationHealthStatus`, and
  `OrchestrationSignal` value metadata.
- Snapshot aggregation over current routing mode/status, selected provider/model summary, task plan
  status/summary, preferred agent summary, memory affinity summary, provider catalog count, agent
  count, memory taxonomy count, runtime context status, and activity metadata.
- Read-only controller/view-model exposure for snapshot status, summary, and signal strings.
- Minimal Dashboard snapshot visibility using existing shell panel patterns.
- Tests for deterministic snapshot construction, routing-mode updates, included provider/agent/
  memory counts, preferred agent/memory/task summaries, and QML-safe exposure.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, tool execution, filesystem/system actions, plugin loading, vector databases,
  embeddings, semantic search, autonomous background workers, timers, or broad UI redesign.

## Phase 7.0: Local Runtime Boundary Skeleton

Completed. Added the future local runtime ownership boundary without treating Phase 7.0 as full
model execution.

Delivered:

- `ILocalRuntime` boundary for future local inference/runtime ownership.
- Value-only local runtime descriptor, status, health, capability, request, and response metadata.
- `NullLocalRuntime` deterministic metadata and placeholder refusal behavior.
- Read-only controller/view-model exposure and minimal Settings visibility.
- Tests for metadata determinism, safe refusal, controller exposure, and QML-safe view-model
  properties.

Still out of scope:

- Cloud provider integration, API keys, networking, model downloads, model execution, streaming,
  real tool execution, plugin loading, vector databases, embeddings, semantic search, autonomous
  workers, filesystem/system actions, or broad UI redesign.

## Phase 7.1: Local Runtime Session Ownership Skeleton

Completed. Added deterministic local runtime session ownership/lifecycle metadata without real
runtime execution.

Delivered:

- `LocalRuntimeSession`, session id/status/health metadata, allocation metadata, and reservation
  metadata.
- `ILocalRuntimeSessionManager` and `NullLocalRuntimeSessionManager`.
- One deterministic placeholder reserved session for read-only visibility.
- Controller/view-model QML-safe session status, health, count, summary, allocation, reservation,
  and summary-list exposure.
- Tests for lifecycle names, deterministic placeholder metadata, summary ordering, and QML-safe
  exposure.

Still out of scope:

- Provider calls, API keys, networking, downloads, model execution, streaming, process/subprocess
  launch, filesystem/system actions, tools/plugins, autonomous workers, or broad UI redesign.

## Phase 7.2: Runtime Capability Negotiation Layer

Completed. Added deterministic runtime capability negotiation metadata without enabling
capability activation or runtime execution.

Delivered:

- `RuntimeCapabilityDescriptor`, capability state/group metadata, negotiation profile, and
  negotiation result values.
- `IRuntimeCapabilityRegistry` and `StaticRuntimeCapabilityRegistry`.
- Static descriptors for future local inference, streaming, multimodal, embeddings, semantic
  memory, tool bridge, plugin bridge, memory binding, filesystem access, external process
  execution, cloud relay support, local-only enforcement, and privacy-safe mode.
- Read-only controller/view-model exposure for capability counts, enabled/disabled summaries,
  negotiation summaries, and local-only enforcement metadata.
- Minimal Settings visibility without toggles, activation actions, setup flows, downloads, or
  execution controls.
- Tests for deterministic metadata, disabled/unavailable handling, local-only enforcement, and
  QML-safe exposure.

Still out of scope:

- Capability activation, provider calls, API keys, networking, downloads, model execution,
  streaming, process/subprocess launch, filesystem/system actions, tools/plugins, autonomous
  workers, or broad UI redesign.

## Phase 7.3: Runtime Permission Metadata Skeleton

Completed. Added runtime permission metadata boundaries (`RuntimePermission`,
`RuntimePermissionLevel`, `RuntimePermissionDecision`, `RuntimePermissionRequest`) plus
`IRuntimePermissionPolicy` and `StaticRuntimePermissionPolicy`.

Delivered:

- Deterministic default-deny runtime permission policy for execution-level requests.
- Read-only controller/view-model permission decision and summary exposure.
- Tests for deterministic naming and default-deny policy behavior.

Still out of scope:

- Any runtime execution, provider/model calls, process/tool/plugin execution, filesystem/system
  actions, networking/API keys, or approval/setup UI.

## Phase 7.4: Runtime Request Pipeline Skeleton

Completed. Added metadata-only runtime request pipeline boundaries (`RuntimePipelineRequest`,
`RuntimePipelineStage`, `RuntimePipelineResult`, `RuntimePipelineStatus`, `RuntimePipelineTrace`)
plus `IRuntimePipeline` and `StaticRuntimePipeline`.

Delivered:

- Ordered deterministic pipeline traces for request, permission, safety, and blocked execution
  boundary stages.
- Read-only controller/view-model pipeline status/summary/trace exposure.
- Tests for deterministic stage naming, ordered traces, and blocked execution metadata.

Still out of scope:

- Provider/model runtime execution, process launch, networking, filesystem/system actions, or real
  tool/plugin execution.

## Phase 7.5: Runtime Safety Policy Skeleton

Completed. Added runtime safety posture metadata (`RuntimeSafetyPolicy`, `RuntimeSafetyRule`,
`RuntimeSafetyDecision`, `RuntimeSafetyReport`) plus `IRuntimeSafetyPolicy` and
`StaticRuntimeSafetyPolicy`.

Delivered:

- Deterministic local-only and no-execution safety report metadata.
- Read-only settings/dashboard-adjacent runtime safety visibility through existing view-model
  boundaries.
- Tests for deterministic report output and QML-safe exposure through controller/view-model.

Still out of scope:

- Sandbox runtime enforcement, provider/model/tool execution, process launch, networking, and
  filesystem/system actions.

## Phase 7.6: Runtime Architecture Checkpoint and Cleanup

Completed. Closed the Phase 7 metadata-only local runtime foundation before Phase 8.

Delivered:

- `docs/PHASE_7_CHECKPOINT.md` covering completed Phase 7 scope, known limitations, runtime
  guardrails, Phase 8 readiness criteria, and strict out-of-scope work.
- Architecture/status/roadmap/decision/context updates for completed Phase 7.3 through Phase 7.6
  work.
- Small QML read-only exposure consistency fix for the accepted page list.
- Focused desktop view-model test update for the runtime/navigation boundary.

Still out of scope:

- Real provider/model execution, networking/API keys, downloads, streaming, process/subprocess
  launch, filesystem/system actions, real tools, plugin loading, sandbox runtime enforcement,
  embeddings/vector DB/semantic search, autonomous workers, or execution/setup UI.

## Phase 8.0-8.2: Execution Lifecycle And Session Coordination

Completed. Added metadata-only execution lifecycle/session coordination for future runtime
integration while keeping execution disabled.

Delivered:

- Execution request/intent/priority and lifecycle state/status/result/trace metadata.
- Execution session id/status/ownership/coordination-mode metadata.
- `IExecutionLifecycle` with deterministic `StaticExecutionLifecycle`.
- `ExecutionCoordinator` and read-only `ExecutionCoordinationSnapshot`.
- Ordered lifecycle traces ending in blocked/non-executable state.
- Invalid transition rejection.
- Read-only controller/view-model Dashboard and Settings exposure.
- Tests for deterministic lifecycle behavior, blocked-by-default posture, transition validation,
  session ownership metadata, read-only coordination snapshots, and QML-safe exposure.

Still out of scope:

- Provider/model calls, Ollama launch, networking/API keys, downloads, streaming, process launch,
  filesystem/system actions, tools/plugins, autonomous workers, timers/background loops, execution
  controls, or setup UI.

## Phase 8.3-8.5: Local Runtime Adapter / Provider Bridge Readiness

Completed. Prepared the architecture for future Ollama/local runtime integration with metadata-only
adapter, bridge, and readiness boundaries.

Delivered:

- `ILocalRuntimeAdapter` and deterministic `StaticLocalRuntimeAdapter` placeholder metadata.
- Provider bridge boundary with not-connected and not-executable request/response metadata.
- Runtime integration readiness report with ordered checks for adapter contract, endpoint
  configuration, model discovery, provider bridge connection, and execution permission.
- Read-only controller/view-model Dashboard and Settings exposure.
- Tests for adapter metadata, bridge not-connected behavior, readiness ordering, controller
  exposure, and QML-safe view-model properties.

Still out of scope:

- Ollama/provider calls, endpoint setup fields, networking/API keys, downloads, streaming,
  process/subprocess launch, filesystem/system actions, model discovery, model execution,
  tools/plugins, embeddings/vector DB, autonomous workers, and model selection UI.

## Phase 9.0-9.2: Ollama Local Health And Discovery Boundary

Completed. Added the first controlled Ollama/local integration surface while keeping chat
inference and execution disabled.

Delivered:

- Ollama endpoint/config/status/model metadata.
- `IOllamaRuntimeClient`, deterministic `NullOllamaRuntimeClient`, and injectable
  `OllamaHttpRuntimeClient`.
- Safe default endpoint: `http://127.0.0.1:11434`.
- Loopback-HTTP-only endpoint normalization with fallback to the safe default.
- `AppSettings` persistence for normalized endpoint only; no API keys or cloud endpoint default.
- Health boundary limited to local `/api/version`.
- Optional installed model discovery limited to local `/api/tags` read-only metadata.
- Read-only controller/view-model exposure for endpoint, health, connection, model count, and model
  summaries.
- Settings/Dashboard local status visibility without setup or execution controls.

Still out of scope:

- Prompt execution, model generation, streaming, downloads/pulls/deletes/runs, subprocess/process
  launch, cloud calls, API keys, tool/plugin execution, filesystem/system scans/actions, background
  workers, model selection UI, and routing chat requests to Ollama.

## Phase 9.3: Inference Boundary Planning

Planned future boundary work. Any executable Ollama inference path must be separately scoped,
policy-gated, interface-owned, injectable/testable, and documented. Phase 9.0-9.2 health/discovery
does not authorize prompt execution or chat routing.

## Later Phase 7: Packaging / Ecosystem / Extensions

Prepare packaging, update channels, plugin/extension lifecycle, platform-specific integration packages, and distribution workflows.
