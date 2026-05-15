# Phase Status

## Completed / Stable

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
