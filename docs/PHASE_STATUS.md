# Phase Status

## Completed / Stable

### Phase 9.6-9.8: Model Selection, Runtime UX State, And Streaming Skeleton

Completed. Adds selected local model metadata, runtime inference UX state, and a disabled
streaming skeleton while keeping chat routing and local execution explicitly guarded.

Scope:

- Added a persisted selected local model setting.
- Controller resolves an effective local model from an explicit request, selected model, or safe
  discovered-model fallback when metadata is available.
- Selected model validation uses discovered local model metadata when available and rejects known
  invalid selections before invoking inference.
- Runtime UX metadata now includes idle/busy/error state, active local runtime/model badge,
  last-response latency summary, and existing trace summaries.
- Added `LocalInferenceStreamChunk`, `LocalInferenceStreamStatus`, `LocalInferenceStreamResult`,
  `ILocalInferenceStreamClient`, and deterministic disabled stream client behavior.
- Desktop view model exposes QML-safe strings, lists, and booleans only.
- Settings and chat surfaces show model/runtime readiness metadata without adding model-management
  actions or routing chat through Ollama.

Still out of scope:

- Model downloads, pulls, deletes, broad model management UI, automatic chat-to-Ollama routing,
  real token streaming UI, cloud calls, API keys, subprocess launch, filesystem/system actions,
  tools/plugins, and autonomous loops.

### Phase 9.3-9.5: Controlled Local Inference Boundary

Completed. Adds the first explicit local-only inference path behind the Ollama/local inference
boundary while keeping default behavior permission-blocked and non-autonomous.

Scope:

- Added `LocalInferenceRequest`, `LocalInferenceResponse`, `LocalInferenceStatus`,
  `LocalInferenceOptions`, `LocalInferenceError`, and `LocalInferenceTrace`.
- Added `ILocalInferenceClient`, deterministic `NullLocalInferenceClient`, and
  `OllamaLocalInferenceClient`.
- Restricted Ollama inference to local loopback HTTP and non-streaming `/api/generate`.
- Controller local inference calls evaluate runtime permission and safety policy before invoking
  the client.
- Desktop view model exposes QML-safe status, summary, last-response summary, and trace strings
  only.
- Tests cover null refusal, blank prompt rejection, missing/unavailable model rejection, injected
  fake success, QML-safe exposure, and default permission blocking.

Still out of scope:

- Cloud inference, API keys, provider routing automation, streaming, model pull/download/delete,
  subprocess launch, filesystem/system actions, tools/plugins, autonomous loops, and UI redesign.

### Phase 1: Desktop Alpha Foundation

Created the native Qt/QML desktop shell, C++ core interfaces, local provider boundary, runtime memory store, mode handling, and initial documentation.

### Phase 2: Provider And Application Core Foundation

Established provider, controller, chat session, settings, plugin, integration, and context interfaces without adding real network providers.

### Phase 3: SQLite Memory Foundation

Added SQLite-backed memory persistence through `SQLiteMemoryStore` while keeping the `IMemoryStore` contract storage-backend independent.

### Phase 3.1: Persistence Stabilization

Added `JsonSettingsStore`, kept settings and memory persistence separate, expanded SQLite persistence tests, and documented storage boundaries.

### Phase 3.1.5: AI Context & Agent Instruction Layer

Adds lightweight repository-local AI instructions and context documents for Codex, Claude, ChatGPT, and similar coding agents.

Scope:

- Prompt compression.
- Stable architecture guardrails.
- Phase status memory.
- Decision history.
- Agent working instructions.

This phase must not change application logic, SQLite logic, QML architecture, build behavior, or dependencies.

### Phase 3.2: Persistent Chat History Preparation

Added a dedicated chat history persistence boundary and SQLite-backed implementation.

Scope:

- `IChatHistoryStore`.
- `SQLiteChatHistoryStore`.
- Separate chat history database.
- Optional controller persistence loading and appending.
- Runtime chat remains available if persistence is unavailable.
- Tests for chat history storage and controller integration.

Chat history persistence is separate from key-value memory storage.

### Phase 3.3: Chat History UX and Lifecycle Controls

Completed. Adds minimal chat history lifecycle UX on top of the Phase 3.2 persistence boundary.

Scope:

- Generic chat history availability/status in the desktop view model.
- Status bar and chat panel visibility for chat history state.
- Clear-chat confirmation before clearing runtime and persistent history.
- Startup-loaded messages exposed through `ChatMessageListModel`.
- Tests for desktop view-model chat history status and restored messages.

Current limitation:

- Chat history is one local transcript.
- No multi-conversation/thread support.
- No encryption, export, or pruning.

## Current Alignment Work

### Phase 3.4: Cross-platform Architecture Readiness

Completed. Added architecture boundaries and storage maintenance surfaces without platform-specific runtime integrations.

Scope:

- Added platform boundary interfaces:
  - `IPathProvider`
  - `IPlatformService`
  - `INotificationService` (lightweight placeholder)
  - `ISystemIntegrationService` (lightweight placeholder)
- Added default `StandardPathProvider` with Qt `QStandardPaths` ownership for:
  - settings path
  - memory database path
  - chat history database path
- Added storage maintenance controls:
  - clear memory store
  - clear chat history store with runtime-safe fallback
  - settings persistence remains separate and unaffected
- Added controller/view-model generic maintenance status surfaces for QML.
- Added settings page controls for local memory/chat clear actions with confirmation dialogs.
- Added tests for path behavior, clear behavior, unavailable stores, and settings isolation.

Candidate future interfaces:

- `IPlatformService`
- `IPathProvider`
- `INotificationService`
- `ISystemIntegrationService`

### Phase 3.5: Pre-agent Architecture Audit and Release Checkpoint

Completed. Stabilization checkpoint before Phase 4.

Scope:

- Full architecture consistency review across:
  - core boundaries
  - desktop/view-model boundaries
  - persistence separation
  - platform boundary separation
  - QML exposure safety
  - test coverage gaps
  - documentation accuracy
- Small safe fixes only:
  - stale wording and status consistency
  - documentation updates and phase references
  - minor focused tests where obvious behavior needed coverage
- Explicitly no Phase 4 runtime implementation.

Known limitations (unchanged):

- No networking or API key handling.
- No real provider integrations.
- No plugin loading.
- No privileged automation.
- No multi-conversation/thread chat model.
- No encryption, export, or pruning.

Phase 4 readiness criteria:

- Core boundaries remain interface-driven and portable.
- QML receives only generic status/action surfaces.
- Settings, memory, and chat history remain separate persistence categories.
- Path ownership remains behind `IPathProvider`.
- Full test suite and formatting checks pass.

Must remain out of scope until explicitly approved:

- Agent core/tool runtime behavior.
- Real external provider integration.
- Platform-specific automation/integration implementation.

## Completed Agent Foundation

### Phase 4: Agent Core & Tool System

Completed through Phase 4.11 and remains local-safe.

### Phase 4.0: Agent Core Planning and Minimal Runtime Skeleton

Completed. Introduced minimal local-safe agent runtime boundaries.

Scope:

- Added `IAgentRuntime`, `AgentRequest`, `AgentResponse`, `AgentStatus`, and capability descriptors.
- Added deterministic `NullAgentRuntime`:
  - local-only
  - no networking
  - no tool execution
  - no filesystem/system-modifying actions
- Kept provider and agent concepts separate:
  - `IChatProvider` remains chat-response oriented
  - `IAgentRuntime` is reserved for future orchestration boundaries
- Minimal controller/view-model surfaces:
  - generic agent status
  - deterministic placeholder agent request/response
  - no raw runtime object exposure to QML

Out of scope (unchanged):

- Real tool execution.
- OS/platform automation.
- Networking/API keys.
- Real provider integrations.
- Plugin loading.
- Sandbox/permission enforcement runtime.

### Phase 4.1: Tool Descriptor and Registry Skeleton

Completed. Added metadata-only tool modeling and registry boundaries.

Scope:

- Added tool descriptor abstractions:
  - `ToolDescriptor`
  - `ToolParameterDescriptor`
  - `ToolRiskLevel`
  - `ToolExecutionMode`
- Added registry boundary:
  - `IToolRegistry`
  - `InMemoryToolRegistry`
  - deterministic `register/list/find` behavior
  - duplicate-id rejection
- Connected runtime metadata only:
  - `NullAgentRuntime` publishes `availableTools()` metadata
  - no tool execution path
- Minimal controller/view-model metadata exposure:
  - available tool count
  - available tool ids
  - no mutable registry exposure in QML

Out of scope (unchanged):

- Any real tool execution.
- Filesystem/system mutation.
- Shell/process execution.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Sandbox/permissions runtime.

### Phase 4.2: Tool Invocation Planning Boundary

Completed. Added metadata-only tool invocation planning without execution.

Scope:

- Added value-only structures for proposed tool invocation intent:
  - `ToolInvocationPlan`
  - `PlannedToolInvocation`
  - `ToolInvocationArgument`
  - `ToolInvocationPlanStatus`
- `IAgentRuntime` can produce metadata-only invocation plans through `plan()`.
- Invocation plans remain separate from `ToolDescriptor` registry metadata.
- `NullAgentRuntime` produces deterministic fake plans from registered tool metadata.
- Controller/view-model exposure is limited to latest plan status and summary strings.
- Tests cover deterministic output, empty-tool state, unknown requested tool ids, metadata
  preservation, and controller/view-model status exposure.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Permission prompts that approve real execution.
- Sandbox runtime.

Reference plan:

- `docs/PHASE_4_2_PLAN.md`

### Phase 4.3: Approval and Permission Metadata Skeleton

Completed. Added approval/permission metadata evaluation for planned tool invocations without
execution or sandboxing.

Scope:

- Added approval metadata abstractions:
  - `ApprovalStatus`
  - `ApprovalDecision`
  - `PermissionDescriptor`
  - `ToolApprovalRequest`
- Added approval policy boundary:
  - `IApprovalPolicy`
  - `StaticApprovalPolicy`
- Approval policy evaluates `ToolInvocationPlan` data only.
- Low-risk metadata-only plans can be marked as not requiring approval.
- Medium/high-risk planned invocations are represented as requiring approval.
- Explicit approved/denied policy states are representable as metadata.
- Controller/view-model exposure is limited to latest approval status and summary strings.
- Tests cover safe plan behavior, risky plan approval requirements, approved/denied state
  representation, empty plan behavior, deterministic ordering, and controller/view-model status
  exposure.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Sandbox runtime.
- Permission prompts that approve real execution.

### Phase 4.4: Sandbox and Capability Boundary Skeleton

Completed. Added sandbox/capability metadata evaluation for planned and approved tool invocations
without execution or real sandbox enforcement.

Scope:

- Added sandbox/capability metadata abstractions:
  - `CapabilityDescriptor`
  - `SandboxStatus`
  - `SandboxCapabilityDecision`
  - `SandboxEvaluationResult`
- Added sandbox policy boundary:
  - `ISandboxPolicy`
  - `StaticSandboxPolicy`
- Sandbox policy evaluates `ToolInvocationPlan` and `ApprovalDecision` data only.
- Low-risk metadata-only plans can be marked as allowed by metadata capability policy.
- Unknown or high-risk capabilities are represented as denied unless future explicit support is
  added.
- Approval is an input to sandbox evaluation but does not grant runtime capability.
- Controller/view-model exposure is limited to latest sandbox status and summary strings.
- Tests cover metadata-only capability allowance, unknown capability denial, empty plan behavior,
  approved-but-not-capable blocking, deterministic ordering, and controller/view-model status
  exposure.

Still out of scope:

- Tool execution runtime.
- Shell/process execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.5: Execution Boundary Skeleton

Completed. Added a deterministic placeholder execution boundary for planned, approved, and
sandbox-allowed tool invocation metadata without real execution.

Scope:

- Added execution boundary abstractions:
  - `ToolExecutionRequest`
  - `ToolExecutionResult`
  - `ToolExecutionStatus`
  - `IToolExecutor`
  - `NullToolExecutor`
- `NullToolExecutor` returns placeholder-only deterministic results.
- Controller flow now routes metadata through:
  - planning
  - approval
  - sandbox capability evaluation
  - placeholder execution boundary
- Approved and sandbox-allowed plans can produce placeholder success.
- Denied, unapproved, sandbox-blocked, empty, or unknown-tool plans remain blocked or safely
  rejected at the placeholder boundary.
- Controller/view-model exposure is limited to latest execution status and summary strings.
- Tests cover placeholder success, blocked execution, empty plans, unknown tools, deterministic
  results, and controller/view-model status exposure.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.6: Agent Runtime Pipeline Stabilization

Completed. Consolidated the metadata-only agent runtime pipeline result without expanding tool
capabilities.

Scope:

- Added a value-based aggregate pipeline result for:
  - planning status
  - approval status
  - sandbox status
  - placeholder execution status
  - deterministic summary text
- Centralized safe stage and pipeline summaries for controller and view-model exposure.
- Clarified `ApplicationController::runAgentRequest` routing through the full Phase 4 pipeline:
  - registry
  - planning
  - approval
  - sandbox capability metadata evaluation
  - placeholder execution boundary
- Added generic pipeline status/summary exposure to the desktop view model.
- Added tests for successful placeholder routing, approval-blocked routing, sandbox-blocked routing,
  empty-plan routing, unknown-tool routing, deterministic summary/status output, and view-model
  status exposure.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.7: Runtime Context and Tool Session Skeleton

Completed. Added deterministic local runtime context/session ownership without adding execution or
OS interaction.

Scope:

- Added runtime context/session abstractions:
  - `AgentRuntimeContext`
  - `RuntimeSession`
  - `RuntimeSessionId`
  - `RuntimeContextStatus`
- Runtime context safely aggregates value metadata from the latest pipeline result:
  - active planned tool ids
  - approval metadata
  - sandbox metadata
  - placeholder execution metadata
  - deterministic session id and revision
- `ApplicationController` owns the runtime session and attaches each completed pipeline result.
- Desktop view model exposes generic read-only runtime context status, summary, session id, and
  active planned tool ids.
- Tests cover deterministic context creation, pipeline result attachment, planned tool ordering,
  reset behavior, controller exposure, and view-model exposure.

Boundary clarification:

- Runtime context is not execution.
- Runtime context is not persistence.
- Runtime context is not planning.
- Runtime context is not approval policy.
- Runtime context is not sandbox enforcement.
- Runtime context is not plugin loading.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Runtime context persistence.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.8: Agent Activity Log and Audit Trail Skeleton

Completed. Added in-memory metadata-only activity logging for agent pipeline observability.

Scope:

- Added activity/audit metadata abstractions:
  - `AgentActivityEntry`
  - `AgentActivityType`
  - `AgentActivityStatus`
  - `AgentActivityLog`
- Activity entries are value-based and deterministic.
- Activity log is in-memory only and has deterministic sequence ordering.
- `ApplicationController` appends metadata events for:
  - request received
  - plan created
  - approval evaluated
  - sandbox evaluated
  - placeholder execution evaluated
  - pipeline completed or blocked
- Desktop view model exposes generic read-only activity count and latest summary.
- Tests cover deterministic ordering, clear behavior, successful pipeline logging, blocked pipeline
  logging, and controller/view-model exposure.

Boundary clarification:

- Activity logging is metadata only.
- Activity logging is not persistence.
- Activity logging is not execution.
- Activity logging is not sandbox enforcement.
- Activity logging is not provider integration.
- Activity logging is not plugin loading.
- Future durable audit, export, redaction, pruning, and security review are later-phase work.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem/system mutation.
- Platform automation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Activity log persistence/export.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.9: Agent Pipeline UI Visibility

Completed. Added read-only QML visibility for the metadata-only agent pipeline.

Scope:

- Dashboard shows latest pipeline status and summary.
- Dashboard shows runtime context status and summary.
- Dashboard shows active planned tool ids when present.
- Dashboard shows activity count and latest activity summary.
- Desktop view-model remains the QML boundary and exposes simple read-only values only.
- Tests cover visible pipeline values, activity values, successful request updates, blocked
  pipeline updates, and QML-facing property safety.

Boundary clarification:

- UI visibility is read-only.
- UI visibility is not execution.
- UI visibility is not approval.
- UI visibility is not persistence.
- UI visibility is not provider integration.
- UI visibility is not plugin loading.

Still out of scope:

- Real tool execution.
- Shell/process execution.
- Subprocess execution.
- Filesystem mutation.
- Networking/API keys.
- Provider integrations.
- Plugin loading.
- Execution buttons or approval controls.
- Activity log persistence/export.
- Real sandbox runtime or OS permission enforcement.
- Privileged automation.

### Phase 4.10: Architecture Checkpoint and Cleanup

Completed. Stabilized Phase 4 architecture documentation and small consistency gaps before Phase 5.

Scope:

- Reviewed provider vs agent separation.
- Reviewed registry, planning, approval, sandbox metadata, and placeholder execution separation.
- Reviewed runtime context/session ownership.
- Reviewed activity log scope.
- Reviewed QML read-only exposure.
- Added Phase 4 checkpoint documentation for completed scope, limitations, Phase 5 readiness, and
  explicit out-of-scope work.
- Added focused tests for summary fallback and reset behavior.
- Clarified minor Dashboard labels without adding controls.

Boundary clarification:

- This checkpoint adds no product features.
- This checkpoint does not start Phase 5 implementation.
- Real execution, approval UX, sandbox runtime, provider integrations, plugin loading,
  networking/API keys, filesystem/system actions, shell/process launch, subprocess launch, and
  privileged automation remain out of scope.

### Phase 4.11: AI Orchestration Planning Checkpoint

Completed. Documented future model/provider routing direction before Phase 5 UI work.

Scope:

- Added future `ModelRouter` and `RoutingPolicy` planning.
- Documented provider capability profiles and task classification.
- Documented local/cloud fallback and device-aware model selection.
- Documented user routing modes: Auto, Fast, Balanced, Quality, Local Only, Cloud Allowed, and
  Battery Saver.
- Documented future model-management UI metadata: installed/downloadable/recommended models,
  RAM/disk requirements, and local/cloud badges.
- Documented privacy-aware routing: sensitive data should prefer local-only, and cloud use must
  require explicit user permission.
- Clarified that this planning remains separate from current Phase 4 runtime, `IChatProvider`,
  `IAgentRuntime`, tool execution, and future UI model-management screens.

Still out of scope:

- Real provider integrations.
- Networking/API keys.
- Ollama/OpenAI/Anthropic integration.
- Model downloads or model execution.
- Plugin loading.
- Filesystem/system actions.
- Real tool execution.
- Runtime behavior changes.

## Active UI Foundation

### Phase 5: Advanced UI/UX & Motion System

Started with Phase 5.0 design-system foundation and remains lightweight/local-safe.

### Phase 5.0: UI/UX Planning and Design System Foundation

Completed. Added UI planning and a minimal QML design-token layer without changing runtime
behavior.

Scope:

- Added `docs/UI_UX_PLAN.md`.
- Documented Linux/Fedora KDE-friendly, cross-platform Qt/QML design direction.
- Documented future motion and assistant visual guidelines without implementation.
- Added `ui/qml/theme/SentinelTheme.qml` for palette, spacing, radius, and typography tokens.
- Registered the theme singleton in the existing QML module.
- Refactored existing QML styling to consume shared tokens where safe.

Still out of scope:

- Real tool execution.
- Approval UX that triggers actions.
- Sandbox runtime.
- Networking/API keys.
- Provider integrations.
- Model downloads or model execution.
- Plugin loading.
- Filesystem/system actions.
- Heavy animation or particle assistant visual implementation.
- Broad QML rewrite.

### Phase 5.1: Motion and Interaction Foundation

Completed. Added lightweight motion tokens and subtle interaction polish without changing runtime
behavior.

Scope:

- Added duration/easing motion tokens to `SentinelTheme.qml`.
- Added lightweight hover/focus behavior for sidebar navigation.
- Added `SentinelButton.qml` for tokenized command button hover/focus states.
- Added focus-ring transitions for text fields.
- Added low-cost page opacity transition hooks.
- Extended UI/UX docs with motion philosophy and future assistant visual boundaries.

Still out of scope:

- Heavy animations.
- Particle systems.
- Assistant-face rendering.
- Custom OpenGL/Vulkan rendering systems.
- Provider/model execution.
- Networking/API keys.
- Plugin loading.
- Filesystem/system actions.
- Broad QML rewrite.

### Phase 5.2: Adaptive Layout and Responsive Shell Foundation

Completed. Added compact/normal/wide layout behavior while preserving the current QML/view-model
boundaries.

Scope:

- Added layout breakpoint and responsive spacing tokens to `SentinelTheme.qml`.
- Lowered the desktop shell minimum width for narrower resizable windows.
- Added compact sidebar behavior with stable eliding labels.
- Updated header/status surfaces to avoid crowding at compact widths.
- Updated dashboard, chat, memory, and settings layouts to wrap with lightweight `GridLayout`
  column changes.
- Extended UI/UX docs with adaptive layout guidance for Linux/Fedora KDE and macOS.

Still out of scope:

- Heavy animations.
- Particle systems.
- Assistant-face rendering.
- Custom OpenGL/Vulkan rendering systems.
- Provider/model execution.
- Networking/API keys.
- Plugin loading.
- Filesystem/system actions.
- Execution-related UI or approval actions.
- Broad QML rewrite.

### Phase 5.3: Component Consistency and Visual QA Pass

Completed. Normalized small visual patterns and documented manual QA expectations without changing
runtime behavior.

Scope:

- Added `SentinelTextField.qml` for shared text-field styling and focus behavior.
- Added `InfoRow.qml` for read-only label/value status rows.
- Normalized command button and input heights through shared theme tokens.
- Normalized section heading wrapping, metric card padding, dashboard status rows, settings status
  rows, and compact input usage.
- Added `docs/UI_QA_CHECKLIST.md` for compact/normal/wide manual screen checks and platform notes.

Still out of scope:

- Heavy animations.
- Particle systems.
- Assistant-face rendering.
- Custom OpenGL/Vulkan rendering systems.
- Provider/model execution.
- Networking/API keys.
- Plugin loading.
- Filesystem/system actions.
- Execution or approval controls.
- Automated visual driving or screenshot QA.

### Phase 5.4: Workspace UX Integration

Completed. Translated the useful `lovable-tasarim` UI/UX direction into native Qt/QML while keeping
React, Vite, Tailwind, Node tooling, and WebView integration out of the production app.

Scope:

- Added mode-aware graphite/glass/cyan visual tokens in `SentinelTheme.qml`.
- Added ambient Qt/QML shell background and central `WorkspacePresence` component.
- Refined the shell into a left status/navigation rail, central workspace/presence area, and right
  chat/interaction panel.
- Added lightweight QML breathing/orbit/opacity motion for workspace state cues.
- Added mode-aware visual behavior for Companion, Focus, Mission, System, Minimal, and Tactical
  modes.
- Preserved existing view models, controllers, providers, memory stores, and execution boundaries.

Follow-up visual migration:

- Move the completed QML workspace closer to the `lovable-tasarim` visual identity source of truth.
- Emphasize cinematic presence-first composition, softer translucent surfaces, reduced dashboard
  density, larger central AI presence, thinner visual hierarchy, and more negative space.
- Keep the implementation native Qt/QML and preserve all no-execution boundaries.

Still out of scope:

- React architecture or web runtime integration.
- Real tool execution.
- Approval actions or sandbox runtime.
- Filesystem/system mutation or automation.
- Provider/model execution, networking/API keys, voice pipeline, hardware integration, plugin
  loading, advanced particle systems, assistant-face rendering, or Qt Quick 3D.

### Phase 5.4.5: Architecture and UI Risk Audit

Completed. This phase audited architecture and UI risk before Phase 5.5 without adding product
features or runtime behavior.

Scope:

- Re-audit C++ core boundaries, desktop view-model exposure, persistence separation,
  provider/agent separation, tool planning/approval/sandbox/execution boundaries, runtime context,
  activity log scope, QML component structure, design-token usage, and the Phase 5.4 workspace UX
  translation.
- Fix only small safe issues such as stale docs, naming inconsistencies, duplicated simple QML
  styling, fragile bindings, minor label clarity, and checklist gaps.
- Keep `lovable-tasarim` as a design reference only.

Known risks to monitor before Phase 5.5:

- `SentinelTheme.qml` now owns more visual helpers; avoid turning mode-aware visual helpers into
  product logic.
- `WorkspacePresence` and `Atmosphere` add continuous lightweight animation; keep them cheap and
  avoid advanced particle/assistant-face systems.
- Compact/normal/wide manual QA is still important because automated screenshot driving is not in
  place.
- `qmllint` may be unavailable in some developer environments; build-time QML cache compilation
  and startup smoke checks should remain fallback verification.

Phase 5.5 readiness criteria:

- Full tests, formatting, and available QML verification pass.
- QML remains presentation-only behind `DesktopShellViewModel`.
- No UI control implies real execution, approval, networking, provider/model execution, plugin
  loading, filesystem/system actions, voice, hardware integration, WebView, React, Node, Tailwind,
  Vite, assistant-face rendering, advanced particles, or Qt Quick 3D.

### Phase 5.5: Visual Identity Reconstruction

Completed. Reconstructed the Dashboard/Core visual shell to match `lovable-tasarim` more closely
while preserving Sentinel's native Qt/QML and C++ architecture boundaries.

Scope:

- Moved the main workspace from a dashboard-panel composition toward a cinematic presence-first
  scene.
- Added an ultra-thin ambient left rail and bottom floating dock navigation.
- Expanded QML visual primitives for a central orb, floating telemetry readouts, and dock-led
  navigation.
- De-emphasized rigid metric/status panels on the Dashboard/Core page.
- Refined the right AI bridge surface to feel more translucent, spacious, and atmospheric.
- Preserved `DesktopShellViewModel`, provider/agent separation, persistence boundaries, and the
  Phase 4 no-execution architecture.

Still out of scope:

- React/Vite/Tailwind/Node/WebView integration.
- Provider/model execution, networking/API keys, real tool execution, approval actions, sandbox
  runtime, filesystem/system actions, plugin loading, voice, hardware integration, assistant-face
  rendering, advanced particles, heavy custom rendering, or Qt Quick 3D.

## Current Functional Architecture Work

### Phase 6.0: Functional Workspace and Model-Orchestration Skeleton

Completed. Added a metadata-only model/provider routing skeleton without adding provider
integration or model execution.

Scope:

- Added value descriptors for provider capability profiles, providers, models, routing modes, task
  classification, and route results.
- Added `IModelRouter` as a separate routing boundary.
- Added `StaticModelRouter` with deterministic local-only placeholder routing.
- Added minimal `ApplicationController` and `DesktopShellViewModel` read-only exposure for routing
  mode, routing status, and selected placeholder model/provider summary.
- Added focused tests for descriptor preservation, local-only routing, unknown task fallback,
  deterministic static routing, and controller/view-model exposure.

Still out of scope:

- Real provider integration.
- Ollama/OpenAI/Anthropic calls.
- API keys, networking, model downloads, or model execution.
- Real tool execution, approval actions, sandbox runtime, plugin loading, filesystem/system
  actions, broad UI redesign, or model-management UI.

### Phase 6.1: Routing Mode Settings and Persistence

Completed. Added a persisted routing mode preference while keeping model routing metadata-only.

Scope:

- Added routing mode preference to `AppSettings`.
- Persisted routing mode through the existing `JsonSettingsStore`.
- Kept the default privacy-safe: `Local Only`.
- Normalized invalid/unknown routing mode values back to `Local Only`.
- Added mutable metadata routing mode support to `IModelRouter`/`StaticModelRouter`.
- Added `ApplicationController` and `DesktopShellViewModel` route metadata updates when routing
  mode changes.
- Added a minimal Settings page routing mode selector plus read-only route status/summary.
- Added tests for defaults, persistence, invalid fallback, controller/view-model updates, and route
  summary changes.

Still out of scope:

- Real provider integration.
- Ollama/OpenAI/Anthropic calls.
- API keys, networking, model downloads, or model execution.
- Real tool execution, approval actions, sandbox runtime, plugin loading, filesystem/system
  actions, broad UI redesign, provider setup UI, or model-management UI.

### Phase 6.2: Provider Catalog Metadata Skeleton

Completed. Added a deterministic metadata-only catalog for current and future providers/models.

Scope:

- Added `IProviderCatalog` and value catalog entries for providers and models.
- Added `StaticProviderCatalog` with Local Metadata Provider / Sentinel Local Placeholder as
  available local metadata.
- Added Ollama Local, OpenAI Cloud, and Anthropic Cloud placeholders as not configured.
- Captured availability, local/cloud classification, supported task metadata, privacy labels, and
  rough RAM/disk hints.
- Seeded `StaticModelRouter` defaults from available catalog metadata only.
- Exposed read-only provider catalog count and summaries through `ApplicationController` and
  `DesktopShellViewModel`.
- Added a minimal Settings page provider catalog section with text-only local/cloud/status
  summaries.
- Added tests for deterministic catalog entries, local/cloud classification, unavailable cloud
  placeholders, local-only/cloud-placeholder route exclusion, and QML-safe exposure.

Still out of scope:

- Real provider integration.
- Ollama/OpenAI/Anthropic calls.
- API keys, endpoints, networking, model downloads, or model execution.
- Provider setup UI, API key fields, download buttons, model-management actions, real tool
  execution, approval actions, sandbox runtime, plugin loading, filesystem/system actions, or broad
  UI redesign.

### Phase 6.3: Capability Graph and Task Planner Skeleton

Completed. Added deterministic high-level task planning metadata without adding provider/model/tool
execution.

Scope:

- Added value-only task planning metadata:
  - `CapabilityNode`
  - `CapabilityGraph`
  - `TaskPlan`
  - `PlannedTaskStep`
  - `TaskPlanStatus`
- Added `ITaskPlanner` and deterministic `StaticTaskPlanner`.
- Planner consumes task classification, routing mode, provider/model catalog availability,
  local/cloud metadata, privacy sensitivity, and resource hints.
- Sensitive/private tasks require local metadata.
- Unknown tasks use a safe local metadata fallback.
- Cloud-allowed planning falls back to local metadata while cloud catalog entries remain not
  configured.
- Unavailable providers/models are not selected for executable routes.
- `ApplicationController` and `DesktopShellViewModel` expose read-only latest task plan status,
  summary, and planned step count.
- Settings page shows minimal read-only task planning status.
- Tests cover deterministic planning, sensitive local planning, cloud-unavailable fallback, unknown
  fallback, blocked unavailable cloud metadata, step ordering, and controller/view-model exposure.

Still out of scope:

- Real provider integration.
- Ollama/OpenAI/Anthropic calls.
- API keys, endpoints, networking, model downloads, or model execution.
- Real tool execution, approval actions, sandbox runtime, plugin loading, filesystem/system
  actions, provider setup UI, executable model-management actions, or broad UI redesign.

### Phase 6.4: Agent System Skeleton

Completed. Added deterministic static agent metadata without autonomous behavior or runtime
execution.

Scope:

- Added value-only agent metadata:
  - `AgentDescriptor`
  - `AgentRole`
  - `AgentState`
  - `AgentPriority`
  - `AgentCapabilityProfile`
  - `AgentTaskAffinity`
  - `AgentRuntimeSnapshot`
- Added `IAgentRegistry` and deterministic `StaticAgentRegistry`.
- Registry exposes Atlas, Orin, Vela, Kaze, Nyx, and Sol as static metadata only.
- Agent metadata includes id, display name, role, capability summary, preferred task types,
  local/cloud affinity, privacy affinity, state, and priority.
- `StaticTaskPlanner` may annotate a plan with preferred agent metadata based on task affinity.
- `IModelRouter`, `ITaskPlanner`, `IAgentRuntime`, and `IChatProvider` remain separate boundaries.
- `ApplicationController` and `DesktopShellViewModel` expose read-only registered agent count,
  active agent summaries, and current preferred agent summary.
- Dashboard and Settings show text-only agent metadata without execution controls.
- Tests cover registry determinism, unique ids, task affinities, planner metadata interaction, and
  controller/view-model exposure.

Still out of scope:

- Real agent execution, autonomous loops, threads/background workers, provider integration,
  Ollama/OpenAI/Anthropic calls, API keys, networking, downloads, model execution, tool execution,
  memory writes, plugin loading, filesystem/system actions, or broad UI redesign.

### Phase 6.5: Memory Taxonomy and Semantic Metadata Skeleton

Completed. Added deterministic static memory taxonomy metadata without semantic memory execution or
changes to key-value memory persistence.

Scope:

- Added value-only memory taxonomy metadata:
  - `MemoryType`
  - `MemoryShardDescriptor`
  - `MemoryShardStatus`
  - `MemoryAffinity`
  - `MemoryRetentionPolicy`
  - `MemoryPrivacyLevel`
  - `MemoryRecallHint`
  - `MemoryAssociationDescriptor`
- Added `IMemoryCatalog` and deterministic `StaticMemoryCatalog`.
- Catalog exposes Episodic, Semantic, Procedural, Reflective, and Ambient categories as static
  metadata only.
- Memory metadata includes retention, privacy, recall hint, tags, task affinities, and simple
  association labels.
- `IMemoryStore` and `SQLiteMemoryStore` remain the explicit key-value memory persistence boundary.
- `StaticTaskPlanner` may annotate a plan with preferred memory affinity metadata.
- `ApplicationController` and `DesktopShellViewModel` expose read-only memory category count,
  memory taxonomy summaries, and current memory affinity summary.
- Memory and Settings pages show text-only memory taxonomy metadata without semantic search or graph
  execution controls.
- Tests cover catalog determinism, unique ids/types, retention/privacy preservation, planner
  affinity metadata, and controller/view-model exposure.

Still out of scope:

- Vector databases, embeddings, semantic search, autonomous memory writes, semantic recall
  execution, provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, downloads,
  model execution, tool execution, plugin loading, filesystem/system actions, replacing
  `IMemoryStore`/`SQLiteMemoryStore`, or broad UI redesign.

### Phase 6.6: Orchestration Snapshot and Workspace State Skeleton

Completed. Added deterministic orchestration/workspace state aggregation without provider,
model, memory, tool, or autonomous execution.

Scope:

- Added value-only orchestration snapshot metadata:
  - `OrchestrationSnapshot`
  - `WorkspaceStateSummary`
  - `OrchestrationHealthStatus`
  - `OrchestrationSignal`
- Snapshot aggregates read-only metadata from routing mode/status, selected provider/model summary,
  latest task plan status/summary, preferred agent summary, memory affinity summary, provider
  catalog count, agent count, memory taxonomy count, runtime context status, and activity metadata.
- `ApplicationController` builds the current snapshot deterministically on demand and emits snapshot
  change notifications alongside relevant metadata-only changes.
- `DesktopShellViewModel` exposes only QML-safe snapshot status, summary, and compact signal
  strings.
- Dashboard shows a minimal read-only orchestration snapshot panel using existing presentation
  patterns.
- Tests cover deterministic snapshot value behavior, controller aggregation, routing-mode updates,
  provider/agent/memory counts, preferred agent/memory/task summaries, and view-model QML-safe
  exposure.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, real tool execution, filesystem/system actions, plugin loading, vector
  databases, embeddings, semantic search, autonomous background workers, timers, threads, or broad
  UI redesign.

### Phase 6.7: Orchestration Diagnostics and Readiness Checklist

Completed. Added deterministic diagnostics/readiness metadata over the existing orchestration
snapshot and catalogs without probing providers, models, filesystems, networks, or system services.

Scope:

- Added value-only orchestration diagnostics metadata:
  - `OrchestrationDiagnostic`
  - `OrchestrationDiagnosticLevel`
  - `OrchestrationReadinessCheck`
  - `OrchestrationReadinessReport`
- Added `StaticOrchestrationDiagnostics` for ordered, deterministic readiness report generation.
- Diagnostics inspect metadata-only readiness for routing mode, selected provider/model route,
  provider catalog, agent registry, memory taxonomy, task planner, snapshot health, local-only
  privacy posture, cloud provider unavailability/not-configured posture, and disabled execution
  capability.
- `ApplicationController` and `DesktopShellViewModel` expose only QML-safe readiness status,
  summary, and diagnostic `QStringList` values.
- Dashboard and Settings show minimal read-only readiness visibility without actions or setup UI.
- Tests cover deterministic ordering, local-only healthy metadata, cloud not-configured behavior,
  execution disabled checking, and controller/view-model exposure.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, provider/model
  probing, model downloads, model execution, real tool execution, filesystem/system scans/actions,
  plugin loading, vector databases, embeddings, semantic search, autonomous background workers,
  timers, threads, external process calls, or broad UI redesign.

### Phase 6.8: Runtime Context Session Layer

Completed. Added deterministic conversation/session context metadata without adding provider,
model, memory, tool, streaming, or autonomous execution.

Scope:

- Added value-only runtime context session metadata:
  - `ConversationSession`
  - `ConversationSessionId`
  - `ConversationSessionStatus`
  - `RuntimeContextWindow`
  - `InteractionMode`
  - `AttentionState`
  - `ContextScope`
- Added `ConversationSessionStore` and `ConversationSessionContextBuilder` for deterministic
  session/context-window ownership and summary generation.
- `ApplicationController` owns the conversation session separately from `ChatSession` message
  history and Phase 4 `RuntimeSession` agent runtime metadata.
- Context windows summarize current routing mode, preferred agent, memory affinity, and latest
  orchestration snapshot summary.
- `DesktopShellViewModel` exposes only QML-safe read-only strings for conversation session id,
  status, interaction mode, attention state, and context-window summary.
- Dashboard and Settings show minimal read-only session/context metadata.
- Tests cover deterministic session creation, status defaults, context-window metadata,
  controller/view-model exposure, routing update reflection, and separation from `ChatSession` and
  `RuntimeSession`.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, streaming, real tool execution, filesystem/system actions, plugin loading,
  vector databases, embeddings, semantic search, autonomous background workers, timers, threads,
  external process calls, multi-conversation persistence, or broad UI redesign.

### Phase 6.9: Conversation State Graph Skeleton

Completed. Added deterministic conversation state-machine metadata without adding provider, model,
tool, approval, streaming, or autonomous execution.

Scope:

- Added value-only conversation state graph metadata:
  - `ConversationState`
  - `ConversationTransition`
  - `ConversationTransitionResult`
  - `ConversationTransitionStatus`
  - `IConversationStateGraph`
  - `StaticConversationStateGraph`
- Defined deterministic high-level states: Idle, Listening, Planning, Routing,
  Waiting For Approval, Ready To Respond, Responding, Completed, and Error.
- Added safe transition rules for the current metadata flow and deterministic rejection summaries
  for invalid transitions.
- `ApplicationController` owns the state graph separately from `ConversationSession`,
  `ChatSession`, and Phase 4 `RuntimeSession`.
- `DesktopShellViewModel` exposes only QML-safe read-only strings for current state, transition
  status, and transition summary.
- Dashboard and Settings show minimal read-only state metadata.
- Tests cover valid transitions, invalid transition rejection, error transitions,
  controller/view-model exposure, QML-safe properties, and separation from chat/runtime metadata.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, streaming, real tool execution, approval controls/actions, filesystem/system
  actions, plugin loading, vector databases, embeddings, semantic search, autonomous background
  workers, timers, threads, external process calls, multi-conversation persistence, or broad UI
  redesign.

### Phase 6.10: Pre-runtime Architecture Checkpoint and Stabilization

Completed. Checkpointed the completed Phase 6 metadata orchestration foundation before Phase 7.

Scope:

- Reviewed the Phase 6 architecture across provider catalog, model router, task planner, agent
  registry, memory taxonomy, orchestration snapshot, diagnostics/readiness, conversation session,
  conversation state graph, `ApplicationController` ownership, and `DesktopShellViewModel` QML
  exposure.
- Confirmed Phase 6 surfaces remain deterministic, value-based, and metadata-only.
- Added `docs/PHASE_6_CHECKPOINT.md` with completed scope, architecture findings, known
  limitations, Phase 7 readiness criteria, strict out-of-scope list, and recommended Phase 7
  breakdown.
- Updated roadmap/status/context/orchestration docs to mark Phase 6 as checkpointed.
- Defined Phase 7.0 as local runtime boundary planning/implementation, not full model execution
  unless explicitly scoped later.
- No product features, runtime execution, provider integrations, or UI redesign were added.

Architecture findings:

- Provider catalog, model routing, task planning, agent registry, memory taxonomy, snapshot,
  diagnostics, conversation session, and conversation state graph remain separate metadata
  responsibilities.
- `ChatSession`, `ConversationSession`, Phase 4 `RuntimeSession`, and `ConversationStateGraph`
  remain separate.
- QML exposure remains QML-safe through `DesktopShellViewModel`.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, streaming, real tool execution, approval controls/actions, filesystem/system
  actions, plugin loading, vector databases, embeddings, semantic search, autonomous background
  workers, timers, threads, external process calls, multi-conversation persistence, or broad UI
  redesign.

### Phase 7.0: Local Runtime Boundary Skeleton

Completed. Added the future local inference/runtime boundary without enabling execution.

Scope:

- Added value-only local runtime metadata:
  - `LocalRuntimeDescriptor`
  - `LocalRuntimeStatus`
  - `LocalRuntimeHealth`
  - `LocalRuntimeCapability`
  - `LocalRuntimeRequest`
  - `LocalRuntimeResponse`
- Added `ILocalRuntime` as a separate future local runtime boundary.
- Added `NullLocalRuntime` with deterministic metadata and safe placeholder refusal for requests.
- `ApplicationController` owns the local runtime boundary and exposes status, health, summary,
  capability summaries, and refusal summary strings.
- `DesktopShellViewModel` forwards only QML-safe read-only strings and string lists.
- Settings shows minimal read-only local runtime metadata without actions.
- Tests cover deterministic metadata, safe refusal, controller exposure, and view-model QML-safe
  exposure.

Separation:

- `ILocalRuntime` is not `IChatProvider`.
- `ILocalRuntime` is not `IModelRouter`.
- `ILocalRuntime` is not `IAgentRuntime`.
- `ILocalRuntime` is not `IToolExecutor`.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, streaming, process/subprocess launch, real tool execution, approval
  controls/actions, filesystem/system scans/actions, plugin loading, vector databases, embeddings,
  semantic search, autonomous background workers, timers, threads, external process calls,
  multi-conversation persistence, or broad UI redesign.

### Phase 7.1: Local Runtime Session Ownership Skeleton

Completed. Added deterministic local runtime session ownership/lifecycle metadata without enabling
runtime execution.

Scope:

- Added value-only local runtime session metadata:
  - `LocalRuntimeSession`
  - `LocalRuntimeSessionId`
  - `LocalRuntimeSessionStatus`
  - `LocalRuntimeSessionHealth`
  - `LocalRuntimeAllocation`
  - `LocalRuntimeReservation`
- Added `ILocalRuntimeSessionManager` as the future local runtime session ownership boundary.
- Added `NullLocalRuntimeSessionManager` with one deterministic placeholder reserved session.
- Session lifecycle states are representable as metadata: Not Started, Reserved, Active,
  Suspended, and Released.
- `ApplicationController` exposes local runtime session count, status, health, summary,
  allocation summary, reservation summary, and session summary strings.
- `DesktopShellViewModel` exposes only QML-safe read-only strings, counts, and string lists.
- Settings shows minimal read-only local runtime session metadata without actions.
- Tests cover deterministic session metadata, lifecycle naming, summary ordering,
  controller/view-model exposure, and QML-safe property boundaries.

Separation:

- Local runtime sessions are not chat sessions.
- Local runtime sessions are not Phase 4 agent runtime execution/context.
- Local runtime sessions are not provider/model execution.
- Local runtime sessions are not tool execution or plugin ownership.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, model downloads,
  model execution, streaming, process/subprocess launch, real tool execution, approval
  controls/actions, filesystem/system scans/actions, plugin loading, vector databases, embeddings,
  semantic search, autonomous background workers, timers, threads, external process calls,
  multi-conversation persistence, or broad UI redesign.

### Phase 7.2: Runtime Capability Negotiation Layer

Completed. Added deterministic runtime capability negotiation metadata without enabling runtime
execution or capability activation.

Scope:

- Added value-only runtime capability negotiation metadata:
  - `RuntimeCapabilityDescriptor`
  - `RuntimeCapabilityState`
  - `RuntimeCapabilityGroup`
  - `RuntimeNegotiationProfile`
  - `RuntimeNegotiationResult`
- Added `IRuntimeCapabilityRegistry` as the future runtime capability metadata boundary.
- Added `StaticRuntimeCapabilityRegistry` with deterministic descriptors for local inference,
  streaming, multimodal, embeddings, semantic memory, tool bridge, plugin bridge, memory binding,
  filesystem access, external process execution, cloud relay support, local-only enforcement, and
  privacy-safe mode.
- Enabled capabilities are safety metadata only: local-only enforcement and privacy-safe mode.
- Disabled or unavailable capabilities remain non-executable and are not activated automatically.
- `ApplicationController` exposes capability count, enabled/disabled summaries, negotiation
  profile summary, negotiation result summary, and local-only enforcement summary.
- `DesktopShellViewModel` exposes only QML-safe read-only strings, counts, and string lists.
- Settings shows minimal read-only runtime negotiation metadata without controls.
- Tests cover deterministic registry metadata, disabled/unavailable capability handling,
  local-only enforcement metadata, controller exposure, and QML-safe view-model properties.

Separation:

- Runtime capability negotiation is not runtime execution.
- Runtime capability negotiation is not provider routing.
- Runtime capability negotiation is not agent execution.
- Runtime capability negotiation is not tool execution.
- Runtime capability negotiation is not permission approval.
- Runtime capability negotiation is not plugin management.

Still out of scope:

- Capability activation, real provider integration, Ollama/OpenAI/Anthropic calls, API keys,
  networking, model downloads, model execution, streaming, process/subprocess launch, real tool
  execution, approval controls/actions, filesystem/system scans/actions, plugin loading, vector
  databases, embeddings, semantic search, autonomous background workers, timers, threads, external
  process calls, multi-conversation persistence, or broad UI redesign.

### Phase 7.3: Runtime Permission Metadata Skeleton

Completed. Added deterministic runtime permission metadata boundaries without enabling runtime
execution.

Scope:

- Added value-only runtime permission metadata:
  - `RuntimePermission`
  - `RuntimePermissionLevel`
  - `RuntimePermissionDecision`
  - `RuntimePermissionRequest`
- Added `IRuntimePermissionPolicy` and `StaticRuntimePermissionPolicy`.
- Default policy denies runtime execution permission requests in metadata-only mode.
- `ApplicationController` and `DesktopShellViewModel` expose QML-safe read-only permission
  decision/summary strings.
- Tests cover deterministic naming and default-deny behavior.

Separation:

- Runtime permission metadata is not runtime execution.
- Runtime permission metadata is not provider/model invocation.
- Runtime permission metadata is not tool/plugin execution.

### Phase 7.4: Runtime Request Pipeline Skeleton

Completed. Added a deterministic metadata-only runtime request pipeline with trace visibility and
blocked execution status.

Scope:

- Added value-only runtime pipeline metadata:
  - `RuntimePipelineRequest`
  - `RuntimePipelineStage`
  - `RuntimePipelineResult`
  - `RuntimePipelineStatus`
  - `RuntimePipelineTrace`
- Added `IRuntimePipeline` and `StaticRuntimePipeline`.
- Pipeline evaluates request metadata, permission decision metadata, and safety report metadata in
  ordered trace stages.
- Execution remains blocked and non-operational; pipeline returns status/summary/trace metadata
  only.
- `ApplicationController` and `DesktopShellViewModel` expose QML-safe read-only pipeline
  status/summary/trace string lists.
- Tests cover deterministic stage naming, ordered traces, and blocked execution metadata.

Separation:

- Runtime request pipeline is not provider runtime.
- Runtime request pipeline is not model execution.
- Runtime request pipeline is not process/tool/plugin execution.

### Phase 7.5: Runtime Safety Policy Skeleton

Completed. Added deterministic runtime safety posture metadata and rule reporting without sandbox
runtime execution.

Scope:

- Added value-only runtime safety metadata:
  - `RuntimeSafetyPolicy`
  - `RuntimeSafetyRule`
  - `RuntimeSafetyDecision`
  - `RuntimeSafetyReport`
- Added `IRuntimeSafetyPolicy` and `StaticRuntimeSafetyPolicy`.
- Safety report enforces local-only and no-execution posture metadata with deterministic rules.
- Settings page shows read-only runtime permission/safety/pipeline status and trace metadata without
  controls.
- Tests cover deterministic safety decision/report output and QML-safe controller/view-model
  exposure.

Separation:

- Runtime safety policy metadata is not sandbox runtime enforcement.
- Runtime safety policy metadata is not provider/model/tool execution.
- Runtime safety policy metadata is not filesystem/network/process behavior.

Still out of scope:

- Real provider integration, Ollama/OpenAI/Anthropic calls, API keys, networking, downloads, model
  execution, streaming, process/subprocess launch, filesystem/system scans/actions, real tools,
  plugin loading, embeddings/vector DB/semantic search, autonomous workers, and approval/setup UI.

### Phase 7.6: Runtime Architecture Checkpoint and Cleanup

Completed. Checkpointed the Phase 7 local runtime boundary foundation before Phase 8.

Scope:

- Reviewed `ILocalRuntime`, local runtime session manager, capability registry, permission policy,
  safety policy, request pipeline, `ApplicationController` ownership, and
  `DesktopShellViewModel` QML exposure.
- Added `docs/PHASE_7_CHECKPOINT.md` with completed scope, architecture findings, known
  limitations, runtime guardrails, Phase 8 readiness criteria, and strict out-of-scope work.
- Updated roadmap/status/architecture/decision/context docs to mark Phase 7.3 through Phase 7.6 as
  completed metadata-only work.
- Tightened QML read-only navigation exposure so the view-model page list includes every accepted
  page name.
- Added focused view-model test coverage for the updated read-only page list.

Architecture findings:

- Phase 7 runtime surfaces are deterministic metadata only.
- Local runtime, session ownership, capability negotiation, permission policy, safety policy, and
  request pipeline remain separate responsibilities.
- `ApplicationController` owns runtime boundaries through interfaces and exposes only summary/status
  values.
- `DesktopShellViewModel` exposes QML-safe strings, counts, and string lists; raw runtime objects
  remain hidden.

Still out of scope:

- Real provider/model execution, provider calls, API keys, networking, downloads, streaming,
  process/subprocess launch, filesystem/system actions, real tools, plugin loading, sandbox
  runtime enforcement, embeddings/vector DB/semantic search, autonomous workers, and execution or
  setup UI.

### Phase 8.0-8.2: Execution Lifecycle And Session Coordination

Completed. Added a metadata-only execution lifecycle/session coordination layer without enabling
execution.

Scope:

- Added execution value metadata:
  - `ExecutionRequest`
  - `ExecutionIntent`
  - `ExecutionPriority`
  - `ExecutionLifecycleState`
  - `ExecutionLifecycleStatus`
  - `ExecutionLifecycleResult`
  - `ExecutionLifecycleTrace`
  - `ExecutionTraceLevel`
- Added execution session metadata:
  - `ExecutionSession`
  - `ExecutionSessionId`
  - `ExecutionSessionStatus`
  - `ExecutionOwnership`
  - `ExecutionCoordinationMode`
- Added `IExecutionLifecycle`, `StaticExecutionLifecycle`, `ExecutionCoordinator`, and
  `ExecutionCoordinationSnapshot`.
- Lifecycle evaluation is deterministic and ordered:
  requested -> validating -> permission-check -> safety-check -> coordination ->
  ready-placeholder -> blocked.
- Execution remains blocked and non-executable even when the lifecycle reaches
  ready-placeholder metadata.
- Invalid transitions are rejected safely.
- `ApplicationController` owns lifecycle/coordinator interfaces.
- `DesktopShellViewModel` exposes QML-safe strings and string lists only.
- Dashboard and Settings show read-only lifecycle/session/snapshot metadata with no controls.

Separation:

- Execution lifecycle is not local runtime, chat provider, agent runtime, tool executor, provider
  adapter, or plugin/runtime worker.
- Coordination snapshot is a read-only metadata view, not a scheduler.

Still out of scope:

- Real provider/model execution, Ollama launch, API keys, networking, downloads, streaming,
  subprocess/process launch, filesystem/system actions, real tools, plugin loading, autonomous
  workers, timers/background loops, and execution/setup UI.

### Phase 8.3-8.5: Local Runtime Adapter, Provider Bridge, And Pre-integration Readiness

Completed. Added metadata-only local runtime adapter, provider bridge, and runtime integration
readiness boundaries to prepare for future Ollama/local runtime integration without calling,
launching, discovering, or executing anything.

Scope:

- Added local runtime adapter metadata:
  - `ILocalRuntimeAdapter`
  - `LocalRuntimeAdapterDescriptor`
  - `LocalRuntimeAdapterStatus`
  - `LocalRuntimeAdapterHealth`
  - `LocalRuntimeAdapterCapabilitySummary`
  - `StaticLocalRuntimeAdapter`
- Added provider bridge metadata:
  - `IProviderRuntimeBridge`
  - `ProviderRuntimeBridgeStatus`
  - `ProviderRuntimeBridgeSummary`
  - `ProviderRuntimeBridgeRequest`
  - `ProviderRuntimeBridgeResponse`
  - `StaticProviderRuntimeBridge`
- Added pre-integration readiness metadata:
  - `RuntimeIntegrationReadiness`
  - `RuntimeIntegrationCheck`
  - `RuntimeIntegrationReport`
  - `StaticRuntimeIntegrationReadiness`
- Adapter metadata is descriptive placeholder data only.
- Provider bridge reports not connected and not executable.
- Readiness report deterministically explains missing endpoint configuration, model discovery,
  provider bridge connection, and execution permission before real Ollama/local runtime integration.
- `ApplicationController` owns adapter/bridge/readiness boundaries.
- `DesktopShellViewModel` exposes QML-safe strings and string lists only.
- Dashboard and Settings show read-only integration readiness metadata with no controls.

Separation:

- Adapter contract is not execution.
- Provider bridge is not an `IChatProvider` implementation.
- Readiness report is not probing.
- Execution lifecycle remains blocked.
- Local runtime remains placeholder-only.

Still out of scope:

- Ollama/OpenAI/Anthropic calls, API keys, networking, downloads, streaming, process/subprocess
  launch, filesystem/system actions, model discovery, model execution, real tools/plugins,
  embeddings/vector DB, autonomous workers, endpoint fields, setup controls, and model selection UI.

### Phase 9.0-9.2: Ollama Local Health And Discovery Boundary

Completed. Added the first controlled local-provider integration boundary for Ollama health and
installed-model metadata only.

Scope:

- Added Ollama value/config metadata:
  - `OllamaEndpoint`
  - `OllamaConfig`
  - `OllamaConnectionStatus`
  - `OllamaHealthStatus`
  - `OllamaModelSummary`
- Added runtime client boundary:
  - `IOllamaRuntimeClient`
  - `NullOllamaRuntimeClient`
  - `OllamaHttpRuntimeClient`
- Default endpoint is `http://127.0.0.1:11434`.
- Endpoint normalization accepts loopback HTTP only and safely falls back to the default for cloud,
  non-HTTP, malformed, query, fragment, or non-loopback endpoints.
- `AppSettings` can persist the normalized Ollama endpoint; no API key or credential setting
  exists.
- HTTP health checks are limited to the loopback Ollama `/api/version` endpoint.
- Optional model discovery is limited to the loopback Ollama `/api/tags` endpoint and returns
  installed model metadata only.
- `ApplicationController` exposes endpoint, connection status, health status, health summary, model
  count, and model summary strings.
- `DesktopShellViewModel` exposes QML-safe strings/counts/lists only.
- Dashboard and Settings show read-only Ollama local status.

Separation:

- Ollama client is not an `IChatProvider`.
- Ollama client is not execution lifecycle, model router execution, agent runtime, tool executor, or
  plugin runtime.
- Chat requests are not routed to Ollama.
- Execution lifecycle remains blocked and non-executable.

Still out of scope:

- Prompt/model generation, streaming, model downloads/pulls/deletes/runs, subprocess/process
  launch, cloud calls, API keys, tool/plugin execution, filesystem/system scans/actions, background
  workers, setup UI, and model selection UI.
